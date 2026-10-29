// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#import <AppKit/AppKit.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreGraphics/CoreGraphics.h>
#include <libproc.h>
#include "SonoCraftApplicationAudioState.h"

API_AVAILABLE(macos(13.0))
@interface SonoCraftAppAudioOutput : NSObject<SCStreamOutput, SCStreamDelegate> {
@public
    std::shared_ptr<SonoCraftApplicationAudioState> state;
    uint64_t token;
    float samples[2][4096];
}
@end
@implementation SonoCraftAppAudioOutput
- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error {
    if (state->generation.load() != token) return;
    state->active = false; state->setStatus(juce::String::fromUTF8(error.localizedDescription.UTF8String));
}
- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sample ofType:(SCStreamOutputType)type {
    if (type != SCStreamOutputTypeAudio || !CMSampleBufferIsValid(sample) || state->generation.load() != token || !state->active.load()) return;
    const auto* format = CMAudioFormatDescriptionGetStreamBasicDescription(CMSampleBufferGetFormatDescription(sample));
    if (!format || !(format->mFormatFlags & kAudioFormatFlagIsFloat) || format->mBitsPerChannel != 32) return;
    struct { UInt32 count; ::AudioBuffer buffers[2]; } storage {};
    auto* list = reinterpret_cast<AudioBufferList*>(&storage);
    CMBlockBufferRef retained = nullptr;
    if (CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(sample, nullptr, list, sizeof(storage), kCFAllocatorDefault, kCFAllocatorDefault, 0, &retained) != noErr || list->mNumberBuffers == 0) {
        if (retained) CFRelease(retained); return;
    }
    const int total = int(CMSampleBufferGetNumSamples(sample));
    for (int offset = 0; offset < total; offset += 4096) {
        const int count = juce::jmin(4096, total-offset);
        for (int ch = 0; ch < 2; ++ch) {
            const auto& buffer = list->mBuffers[juce::jmin(ch, int(list->mNumberBuffers)-1)];
            const int channels = int(buffer.mNumberChannels);
            const int sourceChannel = list->mNumberBuffers == 1 ? juce::jmin(ch, channels-1) : 0;
            if (!buffer.mData || channels < 1 || (offset+count)*channels*int(sizeof(float)) > int(buffer.mDataByteSize)) { juce::FloatVectorOperations::clear(samples[ch], count); continue; }
            const float* input = static_cast<const float*>(buffer.mData);
            for (int i = 0; i < count; ++i) samples[ch][i] = input[(offset+i)*channels+sourceChannel];
        }
        float* pointers[] { samples[0], samples[1] }; juce::AudioBuffer<float> audio(pointers, 2, count);
        state->feed(audio, count, token);
    }
    if (retained) CFRelease(retained);
}
@end

struct MacCaptureSession {
    dispatch_queue_t queue = dispatch_queue_create("audio.sonocraft.appcapture", DISPATCH_QUEUE_SERIAL);
    SCStream* stream API_AVAILABLE(macos(13.0)) = nil;
    SonoCraftAppAudioOutput* delegate API_AVAILABLE(macos(13.0)) = nil;
};
class MacApplicationAudio final : public SonoCraftApplicationAudioBackend {
public:
    MacApplicationAudio(std::shared_ptr<SonoCraftApplicationAudioState> value) : state(std::move(value)), session(std::make_shared<MacCaptureSession>()) {}
    ~MacApplicationAudio() override { stop(); }
    void start(const SonoCraftApplicationAudio::Application& app, uint64_t generation) override {
        if (@available(macOS 13.0, *)) {
            if (!CGPreflightScreenCaptureAccess()) {
                state->permissionRequired=true;
                state->setStatus(juce::String::fromUTF8("Permissão não reconhecida para esta instalação. Clique em Autorizar captura."));
                return;
            }
            const auto shared = state; const auto handle = session; const auto pid = app.processId;
            // Content enumeration is asynchronous and can request the system's
            // screen/system-audio permission. No image output is registered.
            [SCShareableContent getShareableContentExcludingDesktopWindows:YES onScreenWindowsOnly:NO completionHandler:^(SCShareableContent* content, NSError* error) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (shared->generation.load() != generation) return;
                    if (error || content.displays.count == 0) { shared->setStatus(juce::String::fromUTF8("Permita gravar áudio/tela nos Ajustes do macOS e tente novamente. ") + juce::String::fromUTF8(error.localizedDescription.UTF8String)); return; }
                    SCRunningApplication* selected = nil;
                    for (SCRunningApplication* running in content.applications) if (running.processID == pid) { selected = running; break; }
                    if (!selected) { shared->setStatus(juce::String::fromUTF8("O aplicativo foi fechado ou deixou de estar disponível.")); return; }
                    auto* filter = [[SCContentFilter alloc] initWithDisplay:content.displays.firstObject includingApplications:@[selected] exceptingWindows:@[]];
                    auto* config = [[SCStreamConfiguration alloc] init]; config.capturesAudio = YES; config.excludesCurrentProcessAudio = YES;
                    config.sampleRate = 48000; config.channelCount = 2; config.width = 2; config.height = 2;
                    auto* output = [[SonoCraftAppAudioOutput alloc] init]; output->state = shared; output->token = generation;
                    auto* created = [[SCStream alloc] initWithFilter:filter configuration:config delegate:output];
                    NSError* addError = nil;
                    [created addStreamOutput:output type:SCStreamOutputTypeAudio sampleHandlerQueue:handle->queue error:&addError];
                    if (addError) { shared->setStatus(juce::String::fromUTF8(addError.localizedDescription.UTF8String)); return; }
                    handle->stream = created; handle->delegate = output;
                    [created startCaptureWithCompletionHandler:^(NSError* startError) {
                        if (shared->generation.load() != generation) { [created stopCaptureWithCompletionHandler:^(NSError*) {}]; return; }
                        shared->active = startError == nil;
                        shared->setStatus(startError ? juce::String::fromUTF8(startError.localizedDescription.UTF8String) : juce::String::fromUTF8("Capturando somente o áudio do aplicativo selecionado."));
                    }];
                });
            }];
        } else state->setStatus("Captura por aplicativo requer macOS 13 ou mais recente.");
    }
    void stop() override {
        if (@available(macOS 13.0, *)) {
            if (session->stream) [session->stream stopCaptureWithCompletionHandler:^(NSError*) {}];
            session->stream = nil; session->delegate = nil;
            dispatch_sync(session->queue, ^{});
        }
    }
private:
    std::shared_ptr<SonoCraftApplicationAudioState> state;
    std::shared_ptr<MacCaptureSession> session;
};
std::unique_ptr<SonoCraftApplicationAudioBackend> createSonoCraftApplicationAudioBackend(std::shared_ptr<SonoCraftApplicationAudioState> state) { return std::make_unique<MacApplicationAudio>(std::move(state)); }
juce::Array<SonoCraftApplicationAudio::Application> listSonoCraftAudioApplications() {
    juce::Array<SonoCraftApplicationAudio::Application> result;
    for (NSRunningApplication* application in NSWorkspace.sharedWorkspace.runningApplications) {
        if (application.processIdentifier == NSProcessInfo.processInfo.processIdentifier || application.activationPolicy != NSApplicationActivationPolicyRegular) continue;
        result.add({juce::String::fromUTF8(application.localizedName.UTF8String), uint32_t(application.processIdentifier),juce::String::fromUTF8(application.bundleIdentifier.UTF8String)});
    }
    return result;
}

bool requestSonoCraftCapturePermission() {
    if (@available(macOS 10.15, *)) {
        if (CGPreflightScreenCaptureAccess()) return true;
        // Only explicit user actions reach here; discovery never requests access.
        static std::atomic<bool> asked{false};
        if (!asked.exchange(true)) CGRequestScreenCaptureAccess();
        else juce::URL("x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture").launchInDefaultBrowser();
        return CGPreflightScreenCaptureAccess();
    }
    return false;
}
SonoCraftApplicationAudio::Discovery discoverSonoCraftAudioApplications() {
    @autoreleasepool {
        SonoCraftApplicationAudio::Discovery result;
        for(NSRunningApplication* app in NSWorkspace.sharedWorkspace.runningApplications)
            result.runningProcessIds.add(uint32_t(app.processIdentifier));
        result.runningApplications=listSonoCraftAudioApplications();result.hasProcessList=true;
        if (@available(macOS 14.2, *)) {
            AudioObjectPropertyAddress list{kAudioHardwarePropertyProcessObjectList,kAudioObjectPropertyScopeGlobal,kAudioObjectPropertyElementMain};
            UInt32 size=0;
            if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,&list,0,nullptr,&size)!=noErr) {
                result.message=juce::String::fromUTF8("Não foi possível consultar os aplicativos com áudio."); return result;
            }
            std::vector<AudioObjectID> objects(size/sizeof(AudioObjectID));
            if (size && AudioObjectGetPropertyData(kAudioObjectSystemObject,&list,0,nullptr,&size,objects.data())!=noErr) return result;
            const auto apps=listSonoCraftAudioApplications();
            for (const auto object:objects) {
                AudioObjectPropertyAddress address{kAudioProcessPropertyIsRunningOutput,kAudioObjectPropertyScopeGlobal,kAudioObjectPropertyElementMain};
                UInt32 active=0; size=sizeof(active);
                if (AudioObjectGetPropertyData(object,&address,0,nullptr,&size,&active)!=noErr || !active) continue;
                address.mSelector=kAudioProcessPropertyPID; pid_t pid=0;size=sizeof(pid);
                if (AudioObjectGetPropertyData(object,&address,0,nullptr,&size,&pid)!=noErr) continue;
                // Browser audio lives in a helper process. Resolve its owning GUI app.
                for (int depth=0;pid>1 && depth<16;++depth) {
                    if (pid==getpid()) break;
                    const SonoCraftApplicationAudio::Application* match=nullptr;
                    for (const auto& app:apps) if (app.processId==uint32_t(pid)) {match=&app;break;}
                    if (match) {
                        bool known=false;for(const auto& app:result.applications)known|=app.processId==match->processId;
                        if (!known) result.applications.add(*match); break;
                    }
                    proc_bsdinfo info{};
                    if (proc_pidinfo(pid,PROC_PIDTBSDINFO,0,&info,sizeof(info))!=sizeof(info) || info.pbi_ppid==pid) break;
                    pid=pid_t(info.pbi_ppid);
                }
            }
        } else result.message=juce::String::fromUTF8("Detecção automática requer macOS 14.2+. Use Adicionar fonte neste sistema.");
        return result;
    }
}

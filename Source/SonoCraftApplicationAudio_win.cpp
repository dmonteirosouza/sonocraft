// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Process-loopback declarations require the FE SDK surface in this translation
// unit. Activation itself is loaded dynamically so older Windows can still run
// the app and use browser-tab capture without importing a newer system entry.
#if defined(_WIN32)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#undef WINVER
#define WINVER 0x0A00
#undef NTDDI_VERSION
#define NTDDI_VERSION 0x0A00000A
#endif
#include "SonoCraftApplicationAudioState.h"
#if JUCE_WINDOWS
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <tlhelp32.h>
#include <map>
#include <audioclientactivationparams.h>
#include <wrl.h>
#include <set>

namespace {
using Microsoft::WRL::ComPtr;
struct ActivationResult {
    HANDLE completed = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HRESULT error = E_PENDING;
    ComPtr<IAudioClient> client;
    ~ActivationResult() { if (completed) CloseHandle(completed); }
};
class ActivationCallback final : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IActivateAudioInterfaceCompletionHandler, Microsoft::WRL::FtmBase> {
public:
    explicit ActivationCallback(std::shared_ptr<ActivationResult> value) : result(std::move(value)) {}
    STDMETHOD(ActivateCompleted)(IActivateAudioInterfaceAsyncOperation* operation) override {
        ComPtr<IUnknown> unknown; HRESULT activation = E_FAIL;
        result->error = operation->GetActivateResult(&activation, &unknown);
        if (SUCCEEDED(result->error)) result->error = activation;
        if (SUCCEEDED(result->error)) result->error = unknown.As(&result->client);
        SetEvent(result->completed); return S_OK;
    }
private:
    std::shared_ptr<ActivationResult> result;
};
class WindowsApplicationAudio final : public SonoCraftApplicationAudioBackend, private juce::Thread {
public:
    explicit WindowsApplicationAudio(std::shared_ptr<SonoCraftApplicationAudioState> value) : Thread("SonoCraft application capture"), state(std::move(value)) {}
    ~WindowsApplicationAudio() override { stop(); }
    void start(const SonoCraftApplicationAudio::Application& app, uint64_t value) override {
        stop(); processId = app.processId; token = value;
        if (processId == GetCurrentProcessId()) { state->setStatus(juce::String::fromUTF8("O SonoCraft não pode capturar a própria saída.")); return; }
        startThread();
    }
    void stop() override { signalThreadShouldExit(); stopThread(3000); }
private:
    std::shared_ptr<SonoCraftApplicationAudioState> state;
    DWORD processId = 0; uint64_t token = 0;
    void fail(HRESULT error) {
        state->active = false;
        state->setStatus(juce::String::fromUTF8("Não foi possível capturar este aplicativo. Requer Windows 11 ou versão com build 20348 ou posterior. Código: ") + juce::String::toHexString(int(error)));
    }
    void run() override {
        const auto com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(com)) { fail(com); return; }
        capture(); CoUninitialize();
    }
    void capture() {
        auto activation = std::make_shared<ActivationResult>();
        if (!activation->completed) { fail(HRESULT_FROM_WIN32(GetLastError())); return; }
        auto callback = Microsoft::WRL::Make<ActivationCallback>(activation);
        AUDIOCLIENT_ACTIVATION_PARAMS parameters {};
        parameters.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
        parameters.ProcessLoopbackParams.TargetProcessId = processId;
        parameters.ProcessLoopbackParams.ProcessLoopbackMode = PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;
        PROPVARIANT variant {}; variant.vt = VT_BLOB;
        variant.blob.cbSize = sizeof(parameters); variant.blob.pBlobData = reinterpret_cast<BYTE*>(&parameters);
        ComPtr<IActivateAudioInterfaceAsyncOperation> operation;
        struct ScopedModule {
            HMODULE value = LoadLibraryW(L"mmdevapi.dll");
            ~ScopedModule() { if (value) FreeLibrary(value); }
        } module;
        const auto activate = module.value ? reinterpret_cast<decltype(&ActivateAudioInterfaceAsync)>(GetProcAddress(module.value,"ActivateAudioInterfaceAsync")) : nullptr;
        if (!activate) { fail(E_NOTIMPL); return; }
        auto error = activate(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK, __uuidof(IAudioClient), &variant, callback.Get(), &operation);
        if (FAILED(error)) { fail(error); return; }
        const auto deadline = juce::Time::getMillisecondCounterHiRes()+10000;
        while (WaitForSingleObject(activation->completed, 20) != WAIT_OBJECT_0) {
            if (threadShouldExit()) return;
            if (juce::Time::getMillisecondCounterHiRes() > deadline) { fail(HRESULT_FROM_WIN32(ERROR_TIMEOUT)); return; }
        }
        if (FAILED(activation->error) || !activation->client) { fail(activation->error); return; }
        auto client = activation->client;
        WAVEFORMATEX format {}; format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 2; format.nSamplesPerSec = 44100; format.wBitsPerSample = 16;
        format.nBlockAlign = 4; format.nAvgBytesPerSec = format.nSamplesPerSec*format.nBlockAlign;
        error = client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM, 0, 0, &format, nullptr);
        if (FAILED(error)) { fail(error); return; }
        HANDLE ready = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!ready) { fail(HRESULT_FROM_WIN32(GetLastError())); return; }
        ComPtr<IAudioCaptureClient> input;
        error = client->GetService(IID_PPV_ARGS(&input));
        if (SUCCEEDED(error)) error = client->SetEventHandle(ready);
        if (SUCCEEDED(error)) error = client->Start();
        if (FAILED(error)) { CloseHandle(ready); fail(error); return; }
        state->rate = 44100; state->active = true;
        state->setStatus(juce::String::fromUTF8("Capturando o aplicativo e seus processos de áudio. A reprodução original continua normal."));
        juce::AudioBuffer<float> audio(2,2048);
        while (!threadShouldExit() && state->generation.load() == token) {
            WaitForSingleObject(ready, 20);
            UINT32 frames = 0; error = input->GetNextPacketSize(&frames);
            if (FAILED(error)) break;
            while (frames > 0 && !threadShouldExit()) {
                BYTE* data = nullptr; DWORD flags = 0;
                error = input->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
                if (FAILED(error)) break;
                for (UINT32 offset = 0; offset < frames; offset += 2048) {
                    const int count = int(juce::jmin<UINT32>(2048, frames-offset));
                    for (int ch = 0; ch < 2; ++ch)
                        for (int sample = 0; sample < count; ++sample)
                            audio.setSample(ch, sample, (flags & AUDCLNT_BUFFERFLAGS_SILENT) || !data ? 0.0f : reinterpret_cast<const int16_t*>(data)[(offset+sample)*2+ch]/32768.0f);
                    state->feed(audio, count, token);
                }
                input->ReleaseBuffer(frames);
                error = input->GetNextPacketSize(&frames);
                if (FAILED(error)) break;
            }
            if (FAILED(error)) break;
        }
        client->Stop(); CloseHandle(ready); state->active = false;
        if (FAILED(error) && !threadShouldExit()) fail(error);
    }
};
struct AppList { juce::Array<SonoCraftApplicationAudio::Application> result; std::set<DWORD> pids; };
BOOL CALLBACK collectApplication(HWND window, LPARAM context) {
    if (!IsWindowVisible(window) || GetWindowTextLengthW(window) == 0) return TRUE;
    DWORD pid = 0; GetWindowThreadProcessId(window, &pid);
    auto& apps = *reinterpret_cast<AppList*>(context);
    if (pid == 0 || pid == GetCurrentProcessId() || !apps.pids.insert(pid).second) return TRUE;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return TRUE;
    wchar_t path[4096] {}; DWORD length = 4096;
    const bool named = QueryFullProcessImageNameW(process, 0, path, &length) != FALSE; CloseHandle(process);
    if (named) apps.result.add({juce::File(juce::String(path)).getFileNameWithoutExtension(), uint32_t(pid),juce::String(path).toLowerCase()});
    return TRUE;
}
}
std::unique_ptr<SonoCraftApplicationAudioBackend> createSonoCraftApplicationAudioBackend(std::shared_ptr<SonoCraftApplicationAudioState> state) { return std::make_unique<WindowsApplicationAudio>(std::move(state)); }
juce::Array<SonoCraftApplicationAudio::Application> listSonoCraftAudioApplications() { AppList apps; EnumWindows(collectApplication, reinterpret_cast<LPARAM>(&apps)); return apps.result; }

bool requestSonoCraftCapturePermission() {return true;}
SonoCraftApplicationAudio::Discovery discoverSonoCraftAudioApplications() {
    SonoCraftApplicationAudio::Discovery result;
    struct ComScope { HRESULT value=CoInitializeEx(nullptr,COINIT_MULTITHREADED); ~ComScope(){if(SUCCEEDED(value))CoUninitialize();} } com;
    if(FAILED(com.value) && com.value!=RPC_E_CHANGED_MODE)return result;
    ComPtr<IMMDeviceEnumerator> enumerator;
    if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,IID_PPV_ARGS(&enumerator))))return result;
    ComPtr<IMMDeviceCollection> devices;
    if(FAILED(enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE,&devices)))return result;
    const auto apps=listSonoCraftAudioApplications();result.runningApplications=apps;
    std::map<DWORD,DWORD> parents;
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(snapshot!=INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);
        if(Process32FirstW(snapshot,&entry)) {
            result.hasProcessList=true;
            do {parents[entry.th32ProcessID]=entry.th32ParentProcessID;result.runningProcessIds.add(uint32_t(entry.th32ProcessID));}while(Process32NextW(snapshot,&entry));
        }
        CloseHandle(snapshot);
    }
    UINT count=0;devices->GetCount(&count);
    for(UINT deviceIndex=0;deviceIndex<count;++deviceIndex) {
        ComPtr<IMMDevice> device;ComPtr<IAudioSessionManager2> manager;ComPtr<IAudioSessionEnumerator> sessions;
        if(FAILED(devices->Item(deviceIndex,&device)) || FAILED(device->Activate(__uuidof(IAudioSessionManager2),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(manager.GetAddressOf())))
            || FAILED(manager->GetSessionEnumerator(&sessions)))continue;
        int sessionCount=0;sessions->GetCount(&sessionCount);
        for(int i=0;i<sessionCount;++i) {
            ComPtr<IAudioSessionControl> control;ComPtr<IAudioSessionControl2> session;AudioSessionState state=AudioSessionStateInactive;DWORD pid=0;
            if(FAILED(sessions->GetSession(i,&control)) || FAILED(control->GetState(&state)) || state==AudioSessionStateExpired
                || FAILED(control.As(&session)) || session->IsSystemSoundsSession()==S_OK || FAILED(session->GetProcessId(&pid)) || !pid)continue;
            SonoCraftApplicationAudio::Application selected;
            DWORD current=pid;bool own=false;
            for(int depth=0;current && depth<24;++depth) {
                if(current==GetCurrentProcessId()){own=true;break;}
                for(const auto& app:apps)if(app.processId==current){selected=app;break;}
                if(selected.processId)break;
                const auto found=parents.find(current);if(found==parents.end() || found->second==current)break;
                current=found->second;
            }
            if(own)continue;
            if(!selected.processId) {
                HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!process)continue;
                wchar_t path[4096]{};DWORD length=4096;
                const bool named=QueryFullProcessImageNameW(process,0,path,&length)!=FALSE;CloseHandle(process);
                if(!named)continue;
                selected={juce::File(juce::String(path)).getFileNameWithoutExtension(),uint32_t(pid),juce::String(path).toLowerCase()};
            }
            bool runningKnown=false;for(const auto& app:result.runningApplications)runningKnown|=app.processId==selected.processId;
            if(!runningKnown)result.runningApplications.add(selected);
            if(state!=AudioSessionStateActive)continue;
            bool known=false;for(const auto& app:result.applications)known|=app.identifier==selected.identifier;
            if(!known)result.applications.add(selected);
        }
    }
    return result;
}
#elif !JUCE_MAC
class UnsupportedApplicationAudio final : public SonoCraftApplicationAudioBackend {
public:
    explicit UnsupportedApplicationAudio(std::shared_ptr<SonoCraftApplicationAudioState> value) : state(std::move(value)) {}
    void start(const SonoCraftApplicationAudio::Application&, uint64_t) override { state->setStatus(juce::String::fromUTF8("Captura por aplicativo disponível no macOS 13+ e Windows compatível.")); }
    void stop() override {}
private: std::shared_ptr<SonoCraftApplicationAudioState> state;
};
std::unique_ptr<SonoCraftApplicationAudioBackend> createSonoCraftApplicationAudioBackend(std::shared_ptr<SonoCraftApplicationAudioState> state) { return std::make_unique<UnsupportedApplicationAudio>(std::move(state)); }
juce::Array<SonoCraftApplicationAudio::Application> listSonoCraftAudioApplications() { return {}; }
bool requestSonoCraftCapturePermission(){return false;}
SonoCraftApplicationAudio::Discovery discoverSonoCraftAudioApplications(){return {{},juce::String::fromUTF8("Detecção automática indisponível neste sistema.")};}
#endif

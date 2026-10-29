// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#include "SonoCraftApplicationAudio.h"
#include <array>
#include <atomic>
#include <cmath>

// Fixed capture lifetimes: the audio thread never owns/deletes a capture, takes
// the control lock, or allocates while rendering an already prepared block.
class SonoCraftSourceMixer : private juce::Thread {
public:
    static constexpr int capacity = 32;
    SonoCraftSourceMixer() : juce::Thread("SonoCraft audio discovery") {}
    ~SonoCraftSourceMixer() override {signalThreadShouldExit();notify();stopThread(5000);}
    void startDiscovery(){if(!isThreadRunning())startThread();}
    void pollDiscovery(){
        SonoCraftApplicationAudio::Discovery snapshot;
        {const juce::ScopedLock lock(discoveryLock);if(consumedDiscovery==discoveryRevision)return;consumedDiscovery=discoveryRevision;snapshot=discovered;}
        updateDiscovered(snapshot.applications,snapshot.hasProcessList?&snapshot.runningProcessIds:nullptr,&snapshot.runningApplications);
    }
    juce::String discoveryStatus() const {const juce::ScopedLock lock(discoveryLock);return discovered.message;}
    // Reconcile on the GUI thread: enumeration never opens captures or changes the send mode.
    void updateDiscovered(const juce::Array<SonoCraftApplicationAudio::Application>& apps,
                          const juce::Array<uint32_t>* running=nullptr,
                          const juce::Array<SonoCraftApplicationAudio::Application>* openApps=nullptr){
        const juce::ScopedLock lock(control);
        for(auto& s:slots){
            s.producing=false;
            if(!s.occupied.load() || s.tab || !running)continue;
            if(!s.application.processId && openApps) {
                for(const auto& app:*openApps)if(sameApplication(s.application,app)){s.application=app;break;}
            }
            if((s.application.processId && !running->contains(s.application.processId))
                || (!s.application.processId && openApps)) {
                ++s.revision;s.send=false;s.occupied=false;s.audio.stop();
            }
        }
        for(int i=hidden.size();--i>=0;) {
            bool present=false;for(const auto& app:apps)present|=applicationKey(app)==hidden[i];
            if(!present)hidden.remove(i);
        }
        for(const auto& app:apps){
            if(running && !running->contains(app.processId))continue;
            if(hidden.contains(applicationKey(app)))continue;
            Slot* existing=nullptr;
            for(auto& s:slots)if(s.occupied.load() && !s.tab && sameApplication(s.application,app)){existing=&s;break;}
            if(existing){
                existing->producing=true;
                if(existing->application.processId!=app.processId){
                    ++existing->revision;existing->send=false;existing->audio.stop();existing->application=app;
                }
                continue;
            }
            if(auto* s=allocate(false,app.name,false)){s->application=app;s->automatic=true;s->producing=true;}
        }
    }
    struct View {
        juce::String id, name, applicationName, status;
        bool tab = false, sending = false, muted = false, active = false, requested = false;
        float gain = 1, level = 0;
        bool automatic=false,producing=false,blockedByTabs=false,permissionRequired=false;
        float leftLevel=0,rightLevel=0;
    };
    struct Added { juce::String id, url, error; };
    static bool isBrowser(const juce::String& name) {
        const auto n = name.toLowerCase();
        return n.contains("chrome") || n.contains("chromium") || n.contains("edge") || n.contains("firefox")
            || n.contains("safari") || n.contains("brave") || n.contains("opera") || n == "arc" || n.contains("vivaldi");
    }
    Added addApplication(const SonoCraftApplicationAudio::Application& app) {
        const juce::ScopedLock lock(control);
        for (auto& s : slots) if (s.occupied.load()) {
            if (!s.tab && sameApplication(s.application,app))
                return {{},{},juce::String::fromUTF8("Esse aplicativo já tem um canal. Use os controles dele na mesa.")};
            if (s.tab && isBrowser(app.name)) return {{},{},browserOverlap()};
        }
        auto* s = allocate(false, app.name); if (!s) return {{},{},fullMessage()};
        s->application = app; s->audio.start(app);
        return {s->id,{},{}};
    }
    Added addTab() {
        const juce::ScopedLock lock(control);
        // An automatically discovered browser stays visible, but cannot overlap tabs.
        for (auto& s : slots) if (s.occupied.load() && !s.tab && isBrowser(s.application.name)) {
            s.send=false;++s.revision;s.audio.stop();
        }
        auto* s = allocate(true, "Aba " + juce::String(++tabNumber)); if (!s) return {{},{},fullMessage()};
        const auto url = s->audio.startBrowserTab();
        return {s->id,url,url.isEmpty() ? s->audio.status() : juce::String()};
    }
    Added restart(const juce::String& id) {
        const juce::ScopedLock lock(control);
        auto* s = find(id); if (!s) return {{},{},"Canal removido."};
        ++s->revision; s->send = false; // A fresh permission/selection must be explicitly sent.
        if (s->tab) { const auto url=s->audio.startBrowserTab(); return {id,url,url.isEmpty()?s->audio.status():juce::String()}; }
        auto apps = SonoCraftApplicationAudio::applications();
        const auto outputApps=SonoCraftApplicationAudio::applicationsWithAudio();
        for(const auto& app:outputApps.applications)apps.add(app);
        for (const auto& app : apps) if (sameApplication(app,s->application)) {
            s->application = app; s->audio.start(app); return {id,{},{}};
        }
        return {id,{},juce::String::fromUTF8("Abra o aplicativo antes de iniciar a captura.")};
    }
    void stop(const juce::String& id) {
        const juce::ScopedLock lock(control); if (auto* s=find(id)) { ++s->revision; s->send=false; s->audio.stop(); }
    }
    void remove(const juce::String& id) {
        const juce::ScopedLock lock(control); if (auto* s=find(id)) { if(s->automatic)hidden.addIfNotAlreadyThere(applicationKey(s->application)); ++s->revision; s->send=false; s->occupied=false; s->audio.stop(); }
        // Never resume the full system feed implicitly when the last source goes away.
    }
    void setSend(const juce::String& id, bool value) {
        const juce::ScopedLock lock(control); if (auto* s=find(id)) {
            if(value && !s->tab && isBrowser(s->application.name) && hasTabs())return;
            if (value) individual=true;
            s->send=value;
        }
    }
    void setMute(const juce::String& id, bool value) { const juce::ScopedLock lock(control); if (auto* s=find(id)) s->muted=value; }
    void setGain(const juce::String& id, float value) {
        const juce::ScopedLock lock(control); if (auto* s=find(id)) s->gain=std::isfinite(value)?juce::jlimit(0.0f,2.0f,value):0.0f;
    }
    void rename(const juce::String& id, juce::String name) {
        const juce::ScopedLock lock(control); if (auto* s=find(id)) { name=name.trim().substring(0,80); if (name.isNotEmpty()) s->name=name; }
    }
    bool usesIndividualSources() const { return individual.load(); }
    void useIndividualSources() { individual=true; }
    void useCompleteInput() {
        const juce::ScopedLock lock(control);
        for (auto& s:slots) s.send=false;
        individual=false;
    }
    bool hasChannels() const { for (auto& s:slots) if (s.occupied.load()) return true; return false; }
    bool anyActive() const { for (auto& s:slots) if (s.occupied.load() && s.audio.isActive()) return true; return false; }
    juce::Array<View> views() const {
        const juce::ScopedLock lock(control); juce::Array<View> result;
        for (auto& s:slots) if (s.occupied.load()) result.add({s.id,s.name,s.application.name,s.audio.status(),s.tab,
            s.send.load() && individual.load(),s.muted.load(),s.audio.isActive(),s.audio.isCaptureRequested(),s.gain.load(),s.level.load(),s.automatic,s.producing,
            !s.tab && isBrowser(s.application.name) && hasTabs(),s.audio.needsCapturePermission(),s.leftLevel.load(),s.rightLevel.load()});
        return result;
    }
    void prepare(int samples) { if (scratch.getNumSamples()<samples) scratch.setSize(2,samples,false,false,true); }
    void render(juce::AudioBuffer<float>& output, int samples, double rate) {
        output.clear(0,samples);
        if (scratch.getNumSamples()<samples) return;
        for (auto& s:slots) {
            if (!s.occupied.load()) { s.lastGain=0; s.level=0; s.leftLevel=0; s.rightLevel=0; continue; }
            const auto revision=s.revision.load();
            if (revision!=s.renderedRevision) { s.lastGain=0; s.renderedRevision=revision; }
            s.audio.render(scratch,samples,rate); // Drain even unsent/muted sources: no stale playback later.
            const auto target=(individual.load() && s.send.load() && !s.muted.load()) ? s.gain.load() : 0.0f;
            const bool active=s.audio.isActive();
            s.leftLevel=active?scratch.getMagnitude(0,0,samples):0.0f;
            s.rightLevel=active?scratch.getMagnitude(1,0,samples):0.0f;
            s.level=juce::jmax(s.leftLevel.load(),s.rightLevel.load());
            for (int ch=0;ch<2;++ch) scratch.applyGainRamp(ch,0,samples,s.lastGain,target);
            s.lastGain=target;
            // Disabling send/mute is immediate, including a simultaneous UI removal.
            if (s.revision.load()==revision && s.occupied.load() && individual.load() && s.send.load() && !s.muted.load())
                for (int ch=0;ch<2;++ch) output.addFrom(ch,0,scratch,ch,0,samples);
        }
    }
    juce::ValueTree save() const {
        const juce::ScopedLock lock(control); juce::ValueTree tree("SourceMixer");
        tree.setProperty("individual",individual.load(),nullptr);
        for (auto& s:slots) if (s.occupied.load()) {
            juce::ValueTree child("Source"); child.setProperty("id",s.id,nullptr); child.setProperty("name",s.name,nullptr);
            child.setProperty("application",s.application.name,nullptr); child.setProperty("applicationId",s.application.identifier,nullptr); child.setProperty("automatic",s.automatic,nullptr); child.setProperty("tab",s.tab,nullptr);
            child.setProperty("gain",s.gain.load(),nullptr); child.setProperty("muted",s.muted.load(),nullptr);
            tree.addChild(child,-1,nullptr);
        }
        return tree;
    }
    void restore(const juce::ValueTree& tree) {
        const juce::ScopedLock lock(control);
        for (auto& s:slots) { ++s.revision; s.send=false; s.occupied=false; s.audio.stop(); }
        hidden.clear();
        individual=bool(tree.getProperty("individual",false));
        for (const auto& child:tree) {
            if (!child.hasType("Source")) continue;
            auto* s=allocate(bool(child.getProperty("tab")),child.getProperty("name").toString()); if (!s) break;
            s->application.name=child.getProperty("application").toString();s->application.identifier=child.getProperty("applicationId").toString();
            s->automatic=bool(child.getProperty("automatic",false));
            const auto gain=float(child.getProperty("gain",1.0f)); s->gain=std::isfinite(gain)?juce::jlimit(0.0f,2.0f,gain):1.0f;
            s->muted=bool(child.getProperty("muted",false));
            // No PID reuse, OS capture or transmission until an explicit user action.
        }
        individual=bool(tree.getProperty("individual",hasChannels()));
    }
private:
    struct Slot {
        SonoCraftApplicationAudio audio;
        juce::String id,name;
        SonoCraftApplicationAudio::Application application;
        bool tab=false,automatic=false,producing=false;
        std::atomic<bool> occupied{false},send{false},muted{false};
        std::atomic<float> gain{1},level{0},leftLevel{0},rightLevel{0};
        std::atomic<uint64_t> revision{0};
        uint64_t renderedRevision=0;
        float lastGain=0; // Audio thread only.
    };
    std::array<Slot,capacity> slots;
    mutable juce::CriticalSection control;
    std::atomic<bool> individual{false};
    juce::AudioBuffer<float> scratch;
    int tabNumber=0;
    juce::StringArray hidden;
    mutable juce::CriticalSection discoveryLock;
    SonoCraftApplicationAudio::Discovery discovered;
    uint64_t discoveryRevision=0,consumedDiscovery=0;
    void run() override {
        while(!threadShouldExit()){
            auto value=SonoCraftApplicationAudio::applicationsWithAudio();
            {const juce::ScopedLock lock(discoveryLock);discovered=std::move(value);++discoveryRevision;}
            wait(1500);
        }
    }
    static juce::String applicationKey(const SonoCraftApplicationAudio::Application& app){return app.identifier.isNotEmpty()?app.identifier:app.name;}
    static bool sameApplication(const SonoCraftApplicationAudio::Application& a,const SonoCraftApplicationAudio::Application& b){
        return a.identifier.isNotEmpty() && b.identifier.isNotEmpty() ? a.identifier==b.identifier : a.name.equalsIgnoreCase(b.name);
    }
    bool hasTabs() const {for(const auto& s:slots)if(s.occupied.load() && s.tab)return true;return false;}
    Slot* find(const juce::String& id) { for (auto& s:slots) if (s.occupied.load() && s.id==id) return &s; return nullptr; }
    Slot* allocate(bool tab,const juce::String& name,bool chooseIndividual=true) {
        for (auto& s:slots) if (!s.occupied.load()) {
            ++s.revision; s.id=juce::Uuid().toString(); s.name=name; s.application={}; s.tab=tab;s.automatic=false;s.producing=false;
            s.send=false; s.muted=false; s.gain=1; s.level=0; s.leftLevel=0; s.rightLevel=0; if(chooseIndividual)individual=true; s.occupied=true; return &s;
        }
        return nullptr;
    }
    static juce::String browserOverlap() { return juce::String::fromUTF8("Use abas individuais OU o navegador inteiro. Remova o canal sobreposto antes de adicionar, para não duplicar áudio."); }
    static juce::String fullMessage() { return juce::String::fromUTF8("A mesa aceita até 32 fontes. Remova um canal para adicionar outro."); }
};

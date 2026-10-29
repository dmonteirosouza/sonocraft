// Exercises real authenticated tab sockets and the same mixer used by application captures.
static bool testSourceMixer() {
    auto owner=std::make_unique<SonoCraftSourceMixer>(); auto& mixer=*owner;
    mixer.prepare(128);
    auto first=mixer.addTab(), second=mixer.addTab();
    if (first.url.isEmpty() || second.url.isEmpty()) return false;
    auto connect=[](juce::StreamingSocket& socket,const juce::String& address) {
        const juce::URL url(address); const auto host="127.0.0.1:"+juce::String(url.getPort());
        if (!socket.connect("127.0.0.1",url.getPort(),3000)) return false;
        const auto request="GET /stream?key="+url.getParameterValues()[0]+" HTTP/1.1\r\nHost: "+host
            +"\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\nOrigin: http://"+host+"\r\n\r\n";
        socket.write(request.toRawUTF8(),int(request.getNumBytesAsUTF8()));
        if (!readHttpHeaders(socket).contains("101 Switching")) return false;
        const juce::String rate="rate=48000";
        return sendBrowserFrame(socket,1,rate.toRawUTF8(),int(rate.getNumBytesAsUTF8()));
    };
    juce::StreamingSocket a,b;
    if (!connect(a,first.url) || !connect(b,second.url)) return false;
    auto deadline=juce::Time::getMillisecondCounterHiRes()+3000;
    while (juce::Time::getMillisecondCounterHiRes()<deadline) {
        const auto views=mixer.views(); if(views[0].active && views[1].active)break;
        juce::Thread::sleep(1);
    }
    if (!mixer.views()[0].active || !mixer.views()[1].active) return false;
    // All channels use the same render path, regardless of app/tab backend.
    juce::AudioBuffer<float> mixed(2,128); float packetA[2048],packetB[2048];
    std::fill(std::begin(packetA),std::end(packetA),0.1f);
    std::fill(std::begin(packetB),std::end(packetB),0.2f);
    const double expected[] {0.0,0.25,0.05,0.2,0.3,0.0};
    deadline=juce::Time::getMillisecondCounterHiRes();
    for(int phase=0;phase<6;++phase) {
        if(phase==1) {mixer.setGain(first.id,0.5f);mixer.setSend(first.id,true);mixer.setSend(second.id,true);}
        if(phase==2)mixer.setMute(second.id,true);
        if(phase==3) {mixer.setMute(second.id,false);mixer.setSend(first.id,false);}
        if(phase==4) {mixer.setGain(second.id,1.5f);}
        if(phase==5)mixer.useCompleteInput();
        double measured=0; int observed=0;
        for(int block=0;block<96;++block) {
            if(block%8==0 && (!sendBrowserFrame(a,2,packetA,sizeof(packetA)) || !sendBrowserFrame(b,2,packetB,sizeof(packetB)))) return false;
            mixer.render(mixed,128,48000);
            if(block>48) {measured+=mixed.getRMSLevel(0,0,128);++observed;}
            advanceAudioClock(deadline,128);
        }
        measured/=observed;
        if(std::abs(measured-expected[phase])>0.02) {
            std::cerr << "FAIL: source independence phase=" << phase << " expected=" << expected[phase] << " measured=" << measured << "\n";return false;
        }
        if(mixer.views()[0].level<0.08f || mixer.views()[1].level<0.16f) {
            std::cerr << "FAIL: source meter stopped when send was disabled\n";return false;
        }
    }
    mixer.rename(first.id,juce::String::fromUTF8("Música ambiente"));mixer.setMute(first.id,true);mixer.setSend(first.id,true);
    const auto saved=mixer.save(); a.close();b.close();
    mixer.restore(saved);
    const auto restored=mixer.views();
    if(restored.size()!=2 || restored[0].name!=juce::String::fromUTF8("Música ambiente") || restored[0].gain!=0.5f || !restored[0].muted
       || restored[0].sending || restored[0].active || restored[0].requested || !mixer.usesIndividualSources()) return false;
    // Stale widget callbacks must not operate on a new channel in a reused slot.
    mixer.setSend(first.id,true);mixer.remove(first.id);
    if(mixer.views().size()!=2 || mixer.views()[0].sending) return false;
    mixer.remove(restored[0].id);mixer.remove(restored[1].id);
    if(mixer.hasChannels() || !mixer.usesIndividualSources()) return false;
    juce::ValueTree storedApp("SourceMixer"), app("Source");
    app.setProperty("name","Chrome",nullptr);app.setProperty("application","Google Chrome",nullptr);app.setProperty("tab",false,nullptr);storedApp.addChild(app,-1,nullptr);
    mixer.restore(storedApp);
    if(mixer.addApplication({"Google Chrome",123456}).error.isEmpty()) {
        std::cerr << "FAIL: duplicate application accepted\n";return false;
    }
    const auto browserChannel=mixer.views()[0].id;
    mixer.setSend(browserChannel,true);
    const auto separateTab=mixer.addTab();
    if(separateTab.id.isEmpty() || mixer.views()[0].sending || !mixer.views()[0].blockedByTabs) {
        std::cerr << "FAIL: whole browser not suspended for individual tabs\n";return false;
    }
    mixer.setSend(browserChannel,true);
    if(mixer.views()[0].sending)return false;
    mixer.restore({});mixer.useCompleteInput();
    juce::Array<SonoCraftApplicationAudio::Application> detected;detected.add({"Chrome",101,"browser"});detected.add({juce::String::fromUTF8("Música"),202,"music"});
    mixer.updateDiscovered(detected);const auto automatic=mixer.views();
    if(automatic.size()!=2 || mixer.usesIndividualSources() || automatic[0].sending || automatic[0].requested || !automatic[0].producing || !automatic[0].automatic)return false;
    mixer.setGain(automatic[1].id,0.35f);mixer.setSend(automatic[1].id,true);
    mixer.updateDiscovered(detected);
    if(mixer.views().size()!=2 || mixer.views()[1].id!=automatic[1].id || mixer.views()[1].gain!=0.35f || !mixer.views()[1].sending)return false;
    auto restarted=detected;restarted.getReference(1).processId=303;mixer.updateDiscovered(restarted);
    if(mixer.views()[1].sending || mixer.views()[1].requested || mixer.views()[1].gain!=0.35f)return false;
    mixer.remove(automatic[0].id);mixer.updateDiscovered(detected);
    if(mixer.views().size()!=1)return false;
    mixer.updateDiscovered({});mixer.updateDiscovered(detected);
    if(mixer.views().size()!=2)return false;
    juce::Array<uint32_t> running;running.add(101);running.add(202);
    mixer.updateDiscovered({},&running);
    if(mixer.views().size()!=2 || mixer.views()[0].producing || mixer.views()[1].producing)return false;
    running.clear();mixer.updateDiscovered({},&running);
    if(mixer.hasChannels() || mixer.anyActive() || !mixer.usesIndividualSources()){
        std::cerr << "FAIL: closed applications remained in the mixer or resumed the complete input\n";return false;
    }
    mixer.updateDiscovered(detected);
    const auto automaticSaved=mixer.save();mixer.restore(automaticSaved);
    for(const auto& view:mixer.views())if(!view.automatic || view.requested || view.sending)return false;
    mixer.updateDiscovered({},&running,&detected);
    if(mixer.hasChannels())return false;
    std::cout << "PASS: pause keeps channels; process exit closes captures/removes channels without full-input fallback" << std::endl;
    std::cout << "PASS: automatic discovery preserves mode, gains, stable channels and requires opt-in after app restart" << std::endl;
    std::cout << "PASS: concurrent sources, independent gains/send/mute, application overlap prevention, safe restore/removal" << std::endl;
    return true;
}

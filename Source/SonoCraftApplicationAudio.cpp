// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#include "SonoCraftApplicationAudioState.h"
#include "SonoCraftBrowserAudioBridge.h"
SonoCraftApplicationAudio::SonoCraftApplicationAudio() : state(std::make_shared<SonoCraftApplicationAudioState>()), backend(createSonoCraftApplicationAudioBackend(state)), browserBridge(std::make_unique<SonoCraftBrowserAudioBridge>(state)) {}
SonoCraftApplicationAudio::~SonoCraftApplicationAudio() { stop(); }
juce::Array<SonoCraftApplicationAudio::Application> SonoCraftApplicationAudio::applications() { return listSonoCraftAudioApplications(); }
void SonoCraftApplicationAudio::start(const Application& application) {
    stop();
    { const juce::ScopedLock lock(state->textLock); state->name = application.name; }
    state->reset = true; state->setStatus(juce::String::fromUTF8("Iniciando captura do aplicativo…"));
    backend->start(application, state->generation.load());
}
void SonoCraftApplicationAudio::stop() {
    state->active = false; ++state->generation; backend->stop(); browserBridge->stop(); state->peak = 0;
    state->setStatus("Captura parada.");
}
bool SonoCraftApplicationAudio::isActive() const { return state->active.load(); }
juce::String SonoCraftApplicationAudio::status() const { const juce::ScopedLock lock(state->textLock); return state->message; }
juce::String SonoCraftApplicationAudio::sourceName() const { const juce::ScopedLock lock(state->textLock); return state->name; }
float SonoCraftApplicationAudio::level() const { return state->peak.load(); }
void SonoCraftApplicationAudio::render(juce::AudioBuffer<float>& output, int samples, double rate) {
    output.clear(0, samples);
    if (!isActive()) return;
    if (state->reset.exchange(false)) state->queue.discard();
    state->queue.render(output.getArrayOfWritePointers(), output.getNumChannels(), samples, state->rate.load(), rate, 1);
}

juce::String SonoCraftApplicationAudio::startBrowserTab() {
    stop();
    { const juce::ScopedLock lock(state->textLock); state->name = "Aba do navegador"; }
    state->reset = true; state->setStatus(juce::String::fromUTF8("Escolha a aba na página aberta no Chrome ou Edge e ative Compartilhar áudio."));
    return browserBridge->start(state->generation.load());
}

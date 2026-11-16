#pragma once
#include "JuceHeader.h"

// Runtime-only state. Hosted plug-ins leave device ownership to their host.
enum class SonoCraftAudioHealth { unmonitored, starting, running, stopped, recovering, unavailable, disabled };
inline bool sonoCraftAudioReady(SonoCraftAudioHealth state) {
    return state == SonoCraftAudioHealth::running || state == SonoCraftAudioHealth::unmonitored;
}
inline juce::String sonoCraftAudioHealthText(SonoCraftAudioHealth state) {
    switch (state) {
        case SonoCraftAudioHealth::starting: return juce::String::fromUTF8("Iniciando áudio");
        case SonoCraftAudioHealth::stopped: return juce::String::fromUTF8("Áudio interrompido");
        case SonoCraftAudioHealth::recovering: return juce::String::fromUTF8("Recuperando áudio");
        case SonoCraftAudioHealth::unavailable: return juce::String::fromUTF8("Dispositivo indisponível");
        case SonoCraftAudioHealth::disabled: return juce::String::fromUTF8("Áudio desativado");
        case SonoCraftAudioHealth::running: return juce::String::fromUTF8("Áudio ativo");
        default: return "Controlado pelo host";
    }
}

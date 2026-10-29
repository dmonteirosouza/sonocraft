// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#include "JuceHeader.h"

namespace SonoCraftAudioRouting {
inline bool isVirtualCable(const juce::String& name) {
    const auto text = name.toLowerCase();
    return text.contains("vb-cable") || text.contains("vb-audio") || text.contains("v-cable")
        || text.contains("blackhole") || text.contains("soundflower")
        || text.contains("cable input") || text.contains("cable output")
        || text.contains("voicemeeter") || text.contains("virtual cable")
        || text.contains("virtual audio") || text.contains("vb-matrix")
        || text.contains("loopback") || text.contains("sonar");
}
inline juce::String cableIdentity(juce::String name) {
    name = name.toLowerCase().replace("input", "").replace("output", "")
        .replace("entrada", "").replace(juce::String::fromUTF8("saída"), "");
    return name.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789");
}
inline bool feedsBack(const juce::String& input, const juce::String& output) {
    return input.isNotEmpty() && output.isNotEmpty() && isVirtualCable(input)
        && isVirtualCable(output) && cableIdentity(input) == cableIdentity(output);
}
inline juce::String listeningError(const juce::String& headphones, const juce::String& mainOutput) {
    if (isVirtualCable(headphones))
        return juce::String::fromUTF8("Escolha fones ou alto-falantes físicos. Saídas de áudio virtual não são permitidas.");
    if (headphones.isNotEmpty() && headphones.equalsIgnoreCase(mainOutput))
        return juce::String::fromUTF8("A saída principal já reproduz os sons nesse dispositivo. Escolha outra saída física para a escuta adicional.");
    return {};
}
}

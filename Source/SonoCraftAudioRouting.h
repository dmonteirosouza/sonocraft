// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#include "JuceHeader.h"

namespace SonoCraftAudioRouting {
inline bool isVirtualCable(const juce::String& name) {
    const auto text = name.toLowerCase();
    return text.contains("vb-cable") || text.contains("vb-audio") || text.contains("v-cable")
        || text.contains("blackhole") || text.contains("soundflower")
        || text.contains("cable input") || text.contains("cable output")
        || text.contains("voicemeeter") || text.contains("virtual cable");
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
        return juce::String::fromUTF8("Para escutar, escolha fones ou alto-falantes físicos. O cabo virtual entrega som a outros aplicativos.");
    if (headphones.isNotEmpty() && headphones.equalsIgnoreCase(mainOutput))
        return juce::String::fromUTF8("A escuta e a saída principal usam o mesmo dispositivo. Para escutar e enviar a aplicativos, use um cabo virtual como saída principal e fones físicos aqui.");
    return {};
}
}

// UI-only descriptions. They never enable a capture or change audio routing.
#pragma once
#include "SonoCraftSourceMixer.h"

namespace SonoCraftPresentation {
enum class Tone { neutral, active, attention };
struct Source {
    juce::String state, detail, listening, sending;
    Tone tone = Tone::neutral;
};
inline Source describeSource(const SonoCraftSourceMixer::View& s, bool hasRecipients, bool sendPaused,
                             const juce::String& error = {}) {
    Source result;
    result.listening = juce::String::fromUTF8(s.tab ? "Escuta: no navegador" : "Escuta: no aplicativo");
    result.sending = "Envio: desligado";
    if (s.blockedByTabs) {
        result.state = "Use as abas"; result.tone = Tone::attention;
        result.detail = juce::String::fromUTF8("O navegador inteiro fica fora do envio enquanto houver abas individuais, para evitar áudio duplicado.");
    } else if (error.isNotEmpty() || s.permissionRequired) {
        result.state = s.permissionRequired ? "Autorizar captura" : "Verificar captura"; result.tone = Tone::attention;
        result.detail = error.isNotEmpty() ? error : s.status;
    } else if (s.muted || (s.sending && s.gain <= 0.00001f)) {
        result.state = s.muted ? "Envio silenciado" : "Volume zerado"; result.tone = Tone::attention;
        result.detail = juce::String::fromUTF8("O som original continua no aplicativo. Os conectados não recebem áudio deste canal.");
        result.sending = "Envio: silenciado";
    } else if (s.active) {
        result.state = "Capturando";
        result.detail = juce::String::fromUTF8("Captura ativa. A escuta original continua no aplicativo; o SonoCraft não a reproduz novamente.");
        if (s.sending) {
            if (sendPaused) {
                result.sending = "Envio: pausado"; result.tone = Tone::attention;
                result.detail = juce::String::fromUTF8("O envio geral está silenciado. Este canal permanece selecionado para enviar quando você retomar.");
            } else if (!hasRecipients) {
                result.sending = juce::String::fromUTF8("Envio: sem conexão");
                result.detail = juce::String::fromUTF8("Canal pronto para enviar. Conecte um participante com recepção habilitada.");
            } else {
                result.state = "Enviando"; result.sending = "Envio: aos conectados"; result.tone = Tone::active;
                result.detail = juce::String::fromUTF8("Captura e envio ativos. O volume deste canal ajusta apenas o áudio enviado.");
            }
        }
    } else if (s.requested) {
        result.state = "Aguardando captura"; result.tone = Tone::attention;
        result.detail = s.status;
        if (s.sending) result.sending = "Envio: aguardando";
    } else if (s.meterStatus.isNotEmpty()) {
        result.state = juce::String::fromUTF8("Verificando áudio");
        result.detail = s.meterStatus;
    } else {
        const bool playing = s.systemMeter ? s.level > 0.0001f : s.producing;
        result.state = playing ? "Tocando" : "Sem som agora";
        result.detail = juce::String::fromUTF8(playing ? "Áudio detectado no aplicativo. Ative Enviar para compartilhar com os conectados."
                                                                     : "O aplicativo continua aberto. Seus ajustes foram mantidos.");
    }
    return result;
}
}

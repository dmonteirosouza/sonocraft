// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#include "SonoCraftApplicationAudioState.h"

// RFC 6455 mandates SHA-1 only for its public handshake, not authentication.
inline juce::String sonoCraftWebSocketAccept(const juce::String& key) {
    const auto text = key+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    const auto length = size_t(text.getNumBytesAsUTF8());
    const auto padded = ((length+9+63)/64)*64;
    juce::MemoryBlock storage(padded, true); auto* data = static_cast<uint8_t*>(storage.getData());
    std::memcpy(data,text.toRawUTF8(),length); data[length] = 0x80;
    const auto bits = uint64_t(length)*8;
    for (int i=0;i<8;++i) data[padded-1-i] = uint8_t(bits>>(i*8));
    uint32_t digest[] {0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0};
    auto rotate = [](uint32_t value,int count) { return (value<<count)|(value>>(32-count)); };
    for (size_t block=0;block<padded;block+=64) {
        uint32_t words[80];
        for (int i=0;i<16;++i) words[i] = juce::ByteOrder::bigEndianInt(data+block+i*4);
        for (int i=16;i<80;++i) words[i] = rotate(words[i-3]^words[i-8]^words[i-14]^words[i-16],1);
        auto a=digest[0], b=digest[1], c=digest[2], d=digest[3], e=digest[4];
        for (int i=0;i<80;++i) {
            const uint32_t f=i<20 ? (b&c)|((~b)&d) : i<40 ? b^c^d : i<60 ? (b&c)|(b&d)|(c&d) : b^c^d;
            const uint32_t k=i<20 ? 0x5a827999 : i<40 ? 0x6ed9eba1 : i<60 ? 0x8f1bbcdc : 0xca62c1d6;
            const auto next=rotate(a,5)+f+e+k+words[i]; e=d;d=c;c=rotate(b,30);b=a;a=next;
        }
        digest[0]+=a;digest[1]+=b;digest[2]+=c;digest[3]+=d;digest[4]+=e;
    }
    uint8_t output[20]; for (int i=0;i<20;++i) output[i]=uint8_t(digest[i/4]>>(24-(i%4)*8));
    return juce::Base64::toBase64(output,sizeof(output));
}

// A local, per-session authenticated audio-only endpoint. The browser owns the
// tab-selection permission; the app never receives video frames or microphone.
class SonoCraftBrowserAudioBridge final : private juce::Thread {
public:
    explicit SonoCraftBrowserAudioBridge(std::shared_ptr<SonoCraftApplicationAudioState> value)
        : Thread("SonoCraft tab audio"), state(std::move(value)) {}
    ~SonoCraftBrowserAudioBridge() override { stop(); }
    juce::String start(uint64_t value) {
        stop(); generation = value; key = juce::Uuid().toString()+juce::Uuid().toString();
        if (!listener.createListener(0, "127.0.0.1")) { state->setStatus("Nao foi possivel abrir a captura local de abas."); return {}; }
        origin = "127.0.0.1:"+juce::String(listener.getBoundPort()); startThread();
        return "http://"+origin+"/?key="+key;
    }
    void stop() { signalThreadShouldExit(); listener.close(); stopThread(2000); }
private:
    std::shared_ptr<SonoCraftApplicationAudioState> state;
    juce::StreamingSocket listener;
    juce::String key, origin;
    uint64_t generation = 0;
    bool readExact(juce::StreamingSocket& socket, void* output, int bytes, int timeout = 3000) {
        auto* data = static_cast<char*>(output); int done = 0;
        const auto deadline = juce::Time::getMillisecondCounterHiRes()+timeout;
        while (done < bytes && !threadShouldExit()) {
            const auto ready = socket.waitUntilReady(true, 100);
            if (ready < 0 || juce::Time::getMillisecondCounterHiRes() > deadline) return false;
            if (ready == 0) continue;
            const auto count = socket.read(data+done, bytes-done, false);
            if (count <= 0) return false; done += count;
        }
        return done == bytes;
    }
    static juce::String headerValue(const juce::StringArray& lines, const juce::String& field) {
        for (const auto& line : lines) if (line.startsWithIgnoreCase(field+":")) return line.fromFirstOccurrenceOf(":", false, false).trim();
        return {};
    }
    void reply(juce::StreamingSocket& socket, const juce::String& type, const juce::String& body) {
        const auto data = body.toUTF8();
        const auto header = "HTTP/1.1 200 OK\r\nConnection: close\r\nCache-Control: no-store\r\nX-Frame-Options: DENY\r\nReferrer-Policy: no-referrer\r\nContent-Type: "+type+"; charset=utf-8\r\nContent-Length: "+juce::String(data.sizeInBytes()-1)+"\r\nContent-Security-Policy: default-src 'none'; script-src 'self' 'nonce-"+key+"'; style-src 'unsafe-inline'; connect-src ws://"+origin+"; worker-src 'self'; frame-ancestors 'none'; base-uri 'none'\r\n\r\n";
        socket.write(header.toRawUTF8(), int(header.getNumBytesAsUTF8())); socket.write(data.getAddress(), int(data.sizeInBytes()-1));
    }
    juce::String html() const {
        return juce::String::fromUTF8(R"HTML(<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>SonoCraft — compartilhar uma aba</title><style>body{font:16px system-ui;background:#f3f5f8;color:#192f39;max-width:660px;margin:50px auto;padding:24px}main{background:white;border:1px solid #dce1e7;border-radius:16px;padding:28px}h1{font-size:25px}button{background:#167f91;color:white;border:0;border-radius:8px;padding:13px 20px;font:inherit;margin:8px 8px 8px 0;cursor:pointer}button:disabled{opacity:.5}p{line-height:1.6}#status{min-height:50px}small{color:#526a77}</style><main><h1>Compartilhar somente o som de uma aba</h1><p>Use Chrome ou Edge. Clique abaixo, escolha <b>uma aba do navegador</b> e marque <b>Compartilhar áudio da aba</b>. Outras abas continuam privadas. <b>Adicione cada aba uma única vez</b>: o navegador não permite reconhecer com segurança uma aba repetida.</p><button id="start">Escolher uma aba</button><button id="stop" disabled>Parar compartilhamento</button><p id="status">Nenhum áudio está sendo compartilhado.</p><small>Mantenha esta página aberta. O áudio vai somente para o SonoCraft deste computador. Não enviamos imagens nem o microfone. Os destinos são escolhidos no SonoCraft.</small></main><script nonce=")HTML")+key+juce::String::fromUTF8(R"HTML(">
let stream,context,socket,node;const status=document.querySelector('#status'),start=document.querySelector('#start'),stop=document.querySelector('#stop');
function finish(message='Compartilhamento encerrado.'){start.disabled=false;stop.disabled=true;if(node){node.disconnect();node=null}if(stream){stream.getTracks().forEach(t=>t.stop());stream=null}if(context){context.close();context=null}if(socket){socket.close();socket=null}status.textContent=message;}
stop.onclick=()=>finish();window.addEventListener('pagehide',()=>finish());
start.onclick=async()=>{start.disabled=true;status.textContent='Escolha uma aba no seletor do navegador e compartilhe com áudio.';try{
 if(!navigator.mediaDevices?.getDisplayMedia||!window.AudioWorkletNode)throw Error('Abra esta página no Chrome ou Edge atualizado.');
 stream=await navigator.mediaDevices.getDisplayMedia({video:{displaySurface:'browser'},audio:{echoCancellation:false,noiseSuppression:false,autoGainControl:false,suppressLocalAudioPlayback:false},systemAudio:'exclude',monitorTypeSurfaces:'exclude',selfBrowserSurface:'exclude',surfaceSwitching:'exclude'});
 if(stream.getVideoTracks()[0]?.getSettings().displaySurface!=='browser')throw Error('Escolha uma ABA, não uma janela ou tela inteira.');
 if(!stream.getAudioTracks().length)throw Error('Não veio áudio. Tente novamente e marque Compartilhar áudio da aba.');
 stream.getVideoTracks()[0].onended=()=>finish();stream.getAudioTracks()[0].onended=()=>finish();
 context=new AudioContext({sampleRate:48000});await context.resume();const key=new URLSearchParams(location.search).get('key');
 await context.audioWorklet.addModule('/processor.js?key='+key);
 socket=new WebSocket('ws://'+location.host+'/stream?key='+key);await new Promise((resolve,reject)=>{socket.onopen=resolve;socket.onerror=()=>reject(Error('Não foi possível conectar ao SonoCraft. Abra a captura novamente pelo aplicativo.'));});
 socket.send('rate='+context.sampleRate);socket.onclose=()=>finish('A conexão com o SonoCraft foi encerrada.');
 const source=context.createMediaStreamSource(new MediaStream(stream.getAudioTracks()));node=new AudioWorkletNode(context,'sonocraft-tab',{numberOfInputs:1,numberOfOutputs:1,outputChannelCount:[2]});
 node.port.onmessage=event=>{if(socket?.readyState===1&&socket.bufferedAmount<131072)socket.send(event.data);};source.connect(node);node.connect(context.destination);
 stop.disabled=false;status.textContent='Áudio da aba selecionada conectado ao SonoCraft. As outras abas não são capturadas.';
}catch(error){finish(error.name==='InvalidStateError'?'Deixe esta página em primeiro plano e clique novamente em Escolher uma aba.':error.name==='NotAllowedError'?'A aba não foi compartilhada. Você pode escolher novamente quando quiser.':error.message||'Não foi possível compartilhar esta aba.');}};
</script></html>)HTML");
    }
    static juce::String worklet() {
        return R"JS(class SonoCraftTab extends AudioWorkletProcessor{constructor(){super();this.audio=new Float32Array(2048);this.at=0;}process(inputs,outputs){for(const channel of outputs[0]||[])channel.fill(0);const input=inputs[0];if(!input?.length)return true;for(let i=0;i<input[0].length;i++){this.audio[this.at++]=input[0][i]||0;this.audio[this.at++]=(input[1]||input[0])[i]||0;if(this.at===2048){this.port.postMessage(this.audio.buffer,[this.audio.buffer]);this.audio=new Float32Array(2048);this.at=0;}}return true;}}registerProcessor('sonocraft-tab',SonoCraftTab);)JS";
    }
    void websocket(juce::StreamingSocket& socket, const juce::String& handshakeKey) {
        const auto accept = sonoCraftWebSocketAccept(handshakeKey);
        const auto response = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "+accept+"\r\n\r\n";
        socket.write(response.toRawUTF8(), int(response.getNumBytesAsUTF8()));
        juce::AudioBuffer<float> audio(2,4096); std::array<uint8_t,32768> payload;
        while (!threadShouldExit() && state->generation.load() == generation) {
            uint8_t prefix[2]; if (!readExact(socket, prefix, 2, 60000)) break;
            if (!(prefix[0]&0x80) || (prefix[0]&0x70) || !(prefix[1]&0x80)) break; // client frames must be final and masked
            const int opcode = prefix[0]&15; uint64_t size = prefix[1]&127;
            if (size == 126) { uint8_t bytes[2]; if (!readExact(socket,bytes,2)) break; size = juce::ByteOrder::bigEndianShort(bytes); }
            else if (size == 127) break;
            if (size > payload.size()) break;
            uint8_t mask[4]; if (!readExact(socket, mask,4) || !readExact(socket,payload.data(),int(size))) break;
            for (size_t i=0;i<size;++i) payload[i] ^= mask[i%4];
            if (opcode == 8) break;
            if (opcode == 9) { // RFC 6455 ping; mirror the decoded bytes as pong.
                if (size>125) break;
                const uint8_t pong[] {0x8a,uint8_t(size)};
                socket.write(pong,2); socket.write(payload.data(),int(size)); continue;
            }
            if (opcode == 1) {
                const auto text = juce::String::fromUTF8(reinterpret_cast<const char*>(payload.data()), int(size));
                const int rate = text.startsWith("rate=") ? text.substring(5).getIntValue() : 0;
                if (rate < 8000 || rate > 192000) break;
                state->rate = rate; state->active = true; state->setStatus(juce::String::fromUTF8("Compartilhando somente a aba selecionada. Mantenha a página auxiliar aberta."));
            } else if (opcode == 2 && size%8 == 0 && state->active.load()) {
                const int frames = int(size/8);
                for (int ch=0;ch<2;++ch) for (int i=0;i<frames;++i) {
                    const auto bits = juce::ByteOrder::littleEndianInt(payload.data()+(i*2+ch)*4); float value; std::memcpy(&value,&bits,sizeof(value));
                    audio.setSample(ch,i,std::isfinite(value) ? juce::jlimit(-1.0f,1.0f,value) : 0);
                }
                state->feed(audio, frames,generation);
            } else if (opcode != 10) break;
        }
        if (state->generation.load() == generation) { state->active = false; state->peak = 0; state->setStatus("Compartilhamento da aba encerrado."); }
    }
    void run() override {
        while (!threadShouldExit()) {
            if (listener.waitUntilReady(true,100) <= 0) continue;
            std::unique_ptr<juce::StreamingSocket> socket(listener.waitForNextConnection()); if (!socket) continue;
            juce::String request;
            const auto headerDeadline = juce::Time::getMillisecondCounterHiRes()+5000;
            while (!threadShouldExit() && juce::Time::getMillisecondCounterHiRes()<headerDeadline && request.length() < 8192 && !request.endsWith("\r\n\r\n")) { char byte; if (!readExact(*socket,&byte,1)) break; request += juce::String::charToString(byte); }
            const auto lines = juce::StringArray::fromLines(request);
            if (lines.isEmpty() || headerValue(lines,"Host") != origin) continue;
            const auto path = lines[0].fromFirstOccurrenceOf(" ",false,false).upToFirstOccurrenceOf(" ",false,false);
            const auto query = "?key="+key;
            if (path == "/"+query) reply(*socket,"text/html",html());
            else if (path == "/processor.js"+query) reply(*socket,"application/javascript",worklet());
            else if (path == "/stream"+query && headerValue(lines,"Origin") == "http://"+origin
                     && headerValue(lines,"Upgrade").equalsIgnoreCase("websocket") && headerValue(lines,"Sec-WebSocket-Version") == "13") {
                const auto handshake = headerValue(lines,"Sec-WebSocket-Key");
                if (handshake.isNotEmpty() && handshake.length() < 128) websocket(*socket,handshake);
            }
        }
    }
};

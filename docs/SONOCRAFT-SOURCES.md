# Escolher os sons que serão compartilhados

A partir da prévia 0.4, **Fontes** permite capturar uma aba específica ou um aplicativo inteiro. Cada computador tem uma captura selecionada por vez, além de seus pads e sua entrada física. Nada é capturado automaticamente ao abrir o SonoCraft.

## Configurar os dispositivos

Em **Áudio e reunião → Reunião e fones**, escolha a função deste computador e clique em **Aplicar configuração**.

- **Participo da reunião aqui:** entrada = microfone físico do headset; saída = CABLE Input. No Teams, microfone = CABLE Output e alto-falante = headset. Em **Onde eu ouço**, escolha o headset físico e ligue a escuta para ouvir pads locais e recebidos sem a própria voz.
- **Envio sons para outro computador:** entrada pode ser seu microfone ou Sem entrada; saída pode ser um fone/alto-falante ou Sem saída. É preciso manter pelo menos uma entrada ou saída de áudio para o processamento funcionar. Não use o mesmo cabo virtual na entrada e na saída.

Para compartilhar somente uma aba ou aplicativo, reproduza os aplicativos normalmente no fone/alto-falante escolhido no sistema operacional. Não direcione a saída geral do navegador para CABLE Input: isso enviaria outros sons diretamente ao cabo, fora das escolhas do SonoCraft.

A entrada virtual antiga é silenciada ao iniciar uma captura selecionada, para que outras abas não vazem pela entrada agregada. Os pads continuam funcionando. O microfone físico não é alterado por essa proteção.

## Uma aba privada e outra compartilhada

1. Abra **Fontes** e escolha **Uma aba do navegador**.
2. Marque o destino: **grupo / conexão** para enviar a outro computador; **reunião** para enviar ao cabo virtual neste computador. Pode marcar os dois.
3. Clique em **Escolher uma aba no navegador**. Uma página auxiliar abre no Chrome ou Edge.
4. Com essa página em primeiro plano, clique em **Escolher uma aba**, selecione a aba desejada e compartilhe com áudio.
5. Mantenha a página auxiliar aberta. Você continua ouvindo a reprodução original; outras abas não são capturadas. Use **Parar captura** no SonoCraft ou **Parar compartilhamento** na página para encerrar.

A autorização do navegador inclui o conteúdo da aba, mas o SonoCraft recebe somente amostras de áudio. O vídeo não é encaminhado nem gravado. Não é possível escolher a aba automaticamente: a seleção e o compartilhamento são confirmados no navegador.

## Enviar um vídeo ou música de outro aplicativo

Escolha **Um aplicativo inteiro**, selecione um aplicativo aberto e clique em **Capturar aplicativo selecionado**. Essa opção inclui o áudio de todas as janelas desse aplicativo. Para separar abas do mesmo navegador, utilize o modo de aba.

- macOS: captura por aplicativo requer macOS 13 ou posterior e autorização de áudio/tela do sistema. A captura exclui o próprio SonoCraft.
- Windows: captura por processo requer uma versão compatível com o recurso de loopback por aplicativo (build 20348 ou posterior; normalmente Windows 11). Em versões incompatíveis, a captura de abas pelo Chrome/Edge é uma alternativa para conteúdos do navegador.

Não capture o aplicativo ou a aba da própria reunião para reenviá-lo à mesma reunião, pois isso devolve as vozes dos participantes.

## Receber do Mac e escolher o que vai ao Teams

No Mac, marque **Enviar esta fonte ao grupo / conexão**. No Windows conectado:

- A escuta nos fones recebe os pads e os outros sons recebidos.
- Os pads locais e os pads recebidos identificados entram na reunião por padrão.
- Marque **Enviar abas e aplicativos recebidos à reunião** para incluir a captura selecionada no Mac.
- Desmarque para mantê-la somente na sua escuta.
- Para uma captura feita no próprio Windows, use **Enviar esta fonte à reunião**.

A reprodução original do aplicativo local não é duplicada pela escuta independente: ela continua saindo pelo dispositivo escolhido no próprio aplicativo/sistema. Pads e fontes recebidas usam a escuta dos fones do SonoCraft.

## Proteções e validação

O envio dos pads preserva canais separados mesmo quando havia uma preferência mono/estéreo antiga. A saída principal é pausada quando entrada e saída identificadas pertencem ao mesmo cabo virtual; o painel explica a correção e a transmissão pela rede permanece disponível. A escuta independente rejeita cabos virtuais e o uso duplicado do mesmo destino principal.

Testes automatizados verificam pads com escuta ligada/desligada, prevenção de retorno ao cabo, captura de áudio via conexão local autenticada, exclusão por padrão da reunião e inclusão opcional, além da interrupção independente do envio à rede. O teste manual de navegador utiliza duas abas sintéticas de frequências diferentes para verificar o isolamento da aba selecionada. Isso não substitui a validação com headset, VB-CABLE e Teams no equipamento real.

A página auxiliar e a conexão de áudio usam somente 127.0.0.1, uma chave aleatória por sessão e verificação de origem. Não há envio de vídeo, gravação em arquivo nem serviço externo de captura.

Referências de implementação: [ScreenCaptureKit, Apple](https://developer.apple.com/documentation/screencapturekit), [loopback por aplicativo, Microsoft](https://learn.microsoft.com/en-us/samples/microsoft/windows-classic-samples/applicationloopbackaudio-sample/), [seleção de abas, Chrome](https://developer.chrome.com/docs/web-platform/screen-sharing-controls).

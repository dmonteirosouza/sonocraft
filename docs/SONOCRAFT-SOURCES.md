# Ouvir o Mac inteiro e escolher o que entra na reunião

Há dois caminhos independentes:

| Destino no Windows | Sons |
| --- | --- |
| Fone ou alto-falante | Áudio completo recebido do Mac e os pads. |
| Microfone virtual da reunião | Microfone físico do Windows, pads e somente a aba/aplicativo que você incluir. |

Escolher uma aba não interrompe o áudio completo do Mac. A faixa selecionada é transmitida separadamente para o Windows poder incluí-la na reunião. Quando existe uma faixa identificada de áudio completo, a seleção não é reproduzida novamente nos fones: ela já está dentro do áudio completo.

## Dispositivos no Mac que envia

Para manter o caminho por cabo virtual que já funciona:

1. No macOS, direcione o som dos aplicativos para **VB-Cable**. Aplicativos com saída própria também devem usar esse destino para fazer parte dessa escuta completa.
2. No SonoCraft, use **VB-Cable** como entrada e **Sem saída/Nenhum** como saída principal. Isso impede o retorno do som recebido para o cabo.
3. Em **Reunião e fones**, a função desse Mac é **Envio sons para outro computador**.
4. Deixe a entrada ativa para continuar ouvindo o Mac inteiro no Windows. Silenciar essa entrada silencia a escuta completa; não é necessário silenciá-la para compartilhar uma seleção.
5. No Soundboard, ative **Enviar pads a todos os conectados**.

## Dispositivos no Windows da reunião

Em **Áudio e reunião → Reunião e fones**, escolha **Participo da reunião aqui**:

- Entrada do SonoCraft: **microfone físico do headset**.
- Saída principal: **CABLE Input**.
- Escuta independente: **headset físico**, com **Ouvir sons recebidos e pads nos meus fones** ligado.
- No Teams: microfone **CABLE Output**; alto-falante **headset**.

O CABLE Output é a entrada do Teams, não a entrada do SonoCraft. No Windows, deixe desligado **Escutar este dispositivo** do cabo virtual, para não ouvir sua própria voz.

## Uma aba só para escuta e outra também na reunião

No Mac:

1. Abra **Fontes → Uma aba do navegador**.
2. Marque **Enviar seleção separada ao outro computador**.
3. Clique em **Escolher uma aba no navegador**. A página auxiliar deve ser usada no Chrome ou Edge.
4. Com essa página em primeiro plano, clique em **Escolher uma aba**, selecione a aba desejada e compartilhe com áudio.
5. Mantenha a página auxiliar aberta. Só essa aba entra na faixa selecionada. As demais continuam chegando pelo áudio completo, para sua escuta.

No Windows, marque **Na reunião, incluir a seleção recebida do Mac/PC** para que a aba escolhida entre no microfone virtual. Desmarque para deixá-la apenas na escuta completa. **Parar captura** interrompe a seleção, mantendo o áudio completo do Mac e os pads.

O navegador pede autorização sobre o conteúdo da aba, mas o SonoCraft encaminha somente amostras de áudio. Nenhum vídeo é encaminhado ou gravado pelo recurso. A seleção da aba precisa ser confirmada pelo usuário no navegador.

## Aplicativos e fontes do próprio Windows

Escolha **Um aplicativo inteiro**, selecione um aplicativo aberto e clique em **Capturar aplicativo selecionado**. Inclui o áudio de todas as janelas desse aplicativo. Para separar abas de um navegador, use o modo de aba.

No próprio Windows, marque **Incluir esta seleção na reunião deste computador**. Continue reproduzindo o aplicativo no headset normal do Windows: você o ouve por sua reprodução original, enquanto uma cópia segue para o cabo virtual. Essa captura não é reproduzida novamente pela escuta independente, evitando duplicação.

Cada computador pode manter uma seleção de aba/aplicativo de cada vez, além de sua entrada e dos pads. Nenhuma seleção é capturada automaticamente ao abrir o SonoCraft.

Captura de aplicativo: macOS 13+ com permissão de áudio/tela; Windows com loopback por processo (build 20348+, normalmente Windows 11). A captura de abas pelo Chrome/Edge é uma alternativa separada. Não capture o aplicativo ou a aba da própria reunião para reenviá-la à mesma reunião.

## O que é enviado à reunião

Por padrão, o áudio geral do cabo virtual recebido do Mac fica fora da reunião. Só entram os pads, o microfone físico local e as seleções que você marcar. A opção avançada de incluir todos os demais sons recebidos envia também o áudio completo; deixe-a desligada quando quiser escolher somente uma aba/aplicativo.

O microfone físico não é confundido com a entrada de um cabo virtual. A entrada virtual local serve à escuta completa, não é somada automaticamente ao microfone da reunião. O envio dos pads preserva canais separados mesmo quando havia uma preferência mono/estéreo antiga.

A saída principal é pausada quando entrada e saída pertencem ao mesmo cabo virtual, para evitar retorno. O painel explica a correção; a transmissão pela rede continua. A escuta independente rejeita cabos virtuais e o uso duplicado do mesmo destino principal.

## Validação

A regressão verifica pads com escuta ligada/desligada, prevenção de retorno ao cabo, áudio completo nos fones enquanto a seleção entra separadamente na reunião, ausência de duplicação da seleção nos fones, interrupção da seleção sem interromper a escuta completa e preservação do microfone físico. Um teste manual com duas abas sintéticas verificou o isolamento da aba escolhida no Chrome do Mac. Não substitui teste com headset, VB-CABLE, Teams e captura nativa de aplicativos no equipamento real.

A página auxiliar usa somente 127.0.0.1, chave aleatória por sessão e verificação de origem. Não há serviço externo de captura, envio de vídeo ou gravação em arquivo pelo recurso.

Referências: [Apple ScreenCaptureKit](https://developer.apple.com/documentation/screencapturekit), [Microsoft loopback por aplicativo](https://learn.microsoft.com/en-us/samples/microsoft/windows-classic-samples/applicationloopbackaudio-sample/), [Chrome: seleção de abas](https://developer.chrome.com/docs/web-platform/screen-sharing-controls).

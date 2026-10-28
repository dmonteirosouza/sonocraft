# Ouvir tudo e escolher o que compartilhar

O SonoCraft separa quatro caminhos. Os nomes são os mesmos em qualquer sistema:

| Caminho | Para que serve |
| --- | --- |
| Entrada | Recebe um microfone ou o som do sistema por um cabo virtual. Essa entrada pode ser enviada aos conectados. |
| Escuta | Reproduz o áudio recebido e os pads nos seus fones ou alto-falantes, sem sua própria voz. |
| Envio aos conectados | Leva a entrada, os pads habilitados e uma fonte selecionada aos participantes da conexão ou grupo. |
| Saída para aplicativos | Entrega uma mistura separada a um cabo virtual. Outro aplicativo pode usá-la como microfone. |

Escolher uma fonte não substitui a entrada completa. Por exemplo: você continua ouvindo todos os sons enviados por uma conexão e inclui somente uma aba específica na saída para aplicativos. Quando a seleção já faz parte de uma entrada identificada como áudio completo, ela não toca duas vezes na escuta.

## 1. Enviar todo o som de um dispositivo

- Nas configurações de som do sistema, escolha o cabo virtual como saída. Aplicativos que usam uma saída própria também precisam usar esse cabo para fazer parte da entrada completa.
- No SonoCraft, escolha o cabo virtual como **entrada**.
- Em **Áudio e saídas → Escuta e saídas**, escolha **Saída para escuta** e **Sem saída principal (só transmitir)**; clique em **Aplicar dispositivos e saída**. Uma saída física também é possível quando desejar reprodução local.
- Mantenha a entrada ativa e conecte por IP ou grupo.
- Para enviar os pads, ative **Enviar pads a todos os conectados** na mesa de sons.

Não use o mesmo cabo na entrada e na saída principal do SonoCraft: isso pode fazer o áudio voltar à entrada. A proteção do aplicativo pausa essa saída quando identifica o mesmo cabo; a transmissão continua.

## 2. Ouvir tudo e usar uma mistura como microfone

No dispositivo que terá a saída para aplicativos:

1. Em **Escuta e saídas**, escolha **Saída para aplicativos (microfone virtual)**.
2. Entrada: seu **microfone físico**, ou **Sem entrada** se não quiser incluir sua voz.
3. Saída principal: ponta de reprodução do cabo virtual, por exemplo **CABLE Input**.
4. Clique em **Aplicar dispositivos e saída** e mantenha **Receber áudio dos conectados** ligado.
5. Em **Escuta independente**, escolha seus fones ou alto-falantes e ative **Ouvir áudio recebido e pads**.
6. No aplicativo que receberá o som, escolha a ponta de gravação do cabo como microfone, por exemplo **CABLE Output**. Para ouvir esse aplicativo, use os fones físicos.

Os nomes Input e Output pertencem ao driver: CABLE Input recebe a saída do SonoCraft; CABLE Output entrega esse som à entrada de outro aplicativo. Não escolha CABLE Output como entrada do SonoCraft nesse cenário.

Deixe desligado o recurso do sistema de escutar o próprio cabo virtual, para não ouvir sua voz. A escuta independente já permite ouvir os sons compartilhados.

## 3. Escolher uma aba ou aplicativo adicional

A escolha é feita onde a fonte está tocando. Quem recebe decide se inclui essa seleção na saída para aplicativos; não há seleção remota das abas do outro dispositivo.

Em **Fontes**:

1. Escolha **Uma aba do navegador** ou **Um aplicativo inteiro**. O aplicativo inteiro inclui todas as janelas e abas dele.
2. Marque **Enviar esta fonte aos conectados** para encaminhar a seleção separadamente. Para usá-la diretamente no cabo de saída local, marque **Usar esta fonte na saída para aplicativos**.
3. Clique no botão de captura. Para uma aba, use a página auxiliar no Chrome/Edge, escolha a aba com áudio e mantenha a página aberta. Para um aplicativo, escolha-o na lista e inicie a captura.
4. No destino, marque **Usar fontes recebidas na saída para aplicativos**.

A fonte selecionada segue em faixa separada, além da entrada completa e dos pads. **Parar captura** interrompe apenas essa seleção. Cada dispositivo pode capturar uma seleção de cada vez. Nenhuma captura começa automaticamente ao abrir o aplicativo.

Se a fonte for local, mantenha sua reprodução original nos fones do sistema: o SonoCraft não reproduz uma segunda cópia local. Se ela vier de uma conexão, a escuta independente permite ouvi-la.

O navegador solicita autorização sobre o conteúdo da aba; o recurso encaminha somente áudio, sem enviar vídeo ou gravá-lo em arquivo. Evite capturar o aplicativo que está recebendo essa mesma saída para não criar retorno.

## 4. O que entra na saída para aplicativos

Por padrão: **microfone físico local + pads locais e recebidos + fontes que você autorizar em Fontes**.

Deixe **Enviar também todo o áudio recebido** desligado para manter o restante dos sons apenas na escuta. Ao ligá-lo, a saída para aplicativos recebe também o áudio completo das conexões.

A entrada virtual local continua disponível para envio aos conectados, mas não é somada automaticamente à saída para aplicativos como se fosse sua voz. Silenciar a entrada não silencia os pads nem a seleção adicional.

## Compatibilidade e validação

Atualize os dois lados para a mesma versão. Captura de aplicativo: macOS 13+ com permissão de áudio/tela; Windows com loopback por processo (build 20348+, normalmente Windows 11). Captura de abas pelo Chrome/Edge é uma alternativa separada. Instale o driver de cabo virtual à parte.

Os testes automatizados verificam os caminhos separados, pads com escuta ligada/desligada, proteção contra retorno e preservação do áudio completo ao iniciar/parar uma seleção. Um teste manual com duas abas sintéticas confirmou o isolamento de uma aba no Chrome do Mac. Isso não substitui a validação dos dispositivos físicos, do cabo virtual, da captura nativa de aplicativos e do aplicativo de destino.

A página auxiliar usa apenas 127.0.0.1, chave aleatória por sessão e verificação de origem. Não usa serviço externo de captura.

Referências: [Apple ScreenCaptureKit](https://developer.apple.com/documentation/screencapturekit), [Microsoft loopback por aplicativo](https://learn.microsoft.com/en-us/samples/microsoft/windows-classic-samples/applicationloopbackaudio-sample/), [Chrome: seleção de abas](https://developer.chrome.com/docs/web-platform/screen-sharing-controls).

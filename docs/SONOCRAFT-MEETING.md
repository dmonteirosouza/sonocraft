# Áudio recebido, pads e reunião

## No Windows que participa da reunião

Abra **Áudio e reunião → Reunião e fones**.

1. Clique em **Escolher microfone e saída para a reunião**. Na aba Áudio, escolha **microfone do headset** como entrada e **CABLE Input** como saída principal.
2. Volte a **Reunião e fones** e marque **Ativar mistura para reunião** e **Receber áudio dos conectados**.
3. Selecione o **headset físico** na lista dos fones e marque **Ouvir recebidos e pads nos fones**. O volume abaixo controla apenas essa escuta.
4. No Teams, selecione **CABLE Output** como microfone e **headset** como alto-falante.

| Destino | Conteúdo |
| --- | --- |
| Fones independentes | Áudio dos participantes recebidos, incluindo o Mac, e pads locais. Sem seu próprio microfone. |
| Saída principal com modo reunião ativo, por padrão | Seu microfone, pads locais e pads recebidos identificados. |
| Saída principal com **Enviar também músicas, vídeos e voz recebidos à reunião** ativado | Seu microfone, todos os sons recebidos e pads locais. |

Não é preciso ativar **Monitorar** para incluir sua voz na reunião. **Silenciar** no canal Sua entrada corta somente seu microfone, mantendo os pads. O silenciamento individual e a recepção global continuam controlando quais participantes são incluídos.

O retorno do Teams deve sair pelo headset escolhido no Teams, sem retornar à entrada do SonoCraft. No Windows, deixe desativado **Escutar este dispositivo** do CABLE Output, pois ele reproduziria sua voz nos fones.

O cabo virtual deve ser instalado separadamente; o SonoCraft não instala um driver de microfone virtual. A escolha dos fones é lembrada, mas a escuta independente precisa ser ativada após iniciar o aplicativo. Se o headset for mono, a saída combina os lados esquerdo e direito.

## Escolher se músicas e vídeos recebidos entram na reunião

Por padrão, os sons gerais recebidos ficam apenas nos fones. Marque **Enviar também músicas, vídeos e voz recebidos à reunião** para que os participantes da reunião também os ouçam. Desmarque para voltar a enviar somente sua voz e os pads identificados.

Atualize o SonoCraft nos dois computadores. Um envio mono ou estéreo misturado de uma versão antiga não permite reconhecer quais sons são pads e fica fora da reunião por padrão. No computador que envia, use **Separar minhas fontes ao enviar para outro computador**, em **Reunião e fones**, ou ative novamente **Enviar pads a todos os conectados**. Evite alterar o envio para uma mistura mono/estéreo nos controles avançados quando precisar dessa separação.

Esta versão não captura automaticamente o áudio dos aplicativos do próprio computador. Músicas e vídeos precisam chegar ao SonoCraft pela entrada de áudio configurada no computador que envia. A opção de reunião não adiciona uma nova fonte de captura local.

## No Mac ou computador que dispara os pads

No Soundboard, marque **Enviar pads a todos os conectados**. O controle habilita o envio dos pads em canais separados e retoma um envio geral que estivesse pausado. Silenciar o microfone não interrompe os pads.

O envio depende de conexões ativas. Um bloqueio individual de envio continua sendo respeitado. O volume do Soundboard afeta os pads enviados e ouvidos; o volume da escuta independente afeta somente seus fones.

## Mesas e pads

A organização continua no seletor de mesas e no menu existentes. Use o card **+** para adicionar um som. O card inteiro usa a cor escolhida, com texto em contraste. No menu **⋮** do pad, **Reutilizar em outra mesa** conserva arquivo, nome, cor e volume, com ajustes independentes. Mesas que já contêm o arquivo ficam desabilitadas nesse menu.

## Validação

A regressão utiliza duas instâncias por UDP local e arquivos WAV reais para verificar: retomada após silenciamento, mistura de microfone e recebidos na saída da reunião, ausência do microfone local na escuta independente, pads locais e recebidos, envio de pads mesmo com voz remota ativa sem incluí-la na reunião, bloqueio e inclusão opcional do áudio geral recebido e persistência da reutilização de sons. O funcionamento com dispositivos físicos, Teams e uso prolongado ainda requer teste no equipamento real.

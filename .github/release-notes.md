SonoCraft 0.4 — escolher o som de uma aba ou aplicativo.

- Nova área **Fontes**, com captura de uma aba específica do Chrome/Edge ou de um aplicativo inteiro.
- Destinos independentes para a captura local: grupo/conexão e reunião pelo cabo virtual.
- No computador da reunião, opção para incluir as abas/aplicativos recebidos do outro computador ou mantê-los somente nos fones.
- A reprodução local dos aplicativos continua normal e não é duplicada pela escuta independente. A captura não usa o microfone.
- Configuração por função: **Participo da reunião aqui** ou **Envio sons para outro computador**, com escolha direta dos dispositivos.
- Proteção contra retorno da saída ao mesmo cabo virtual da entrada. Mantido **Sem saída** para quem apenas transmite com uma entrada ativa.
- Pads enviados separadamente mesmo com configurações mono/estéreo antigas; escuta dos fones e envio à reunião continuam independentes.

Para uma aba, selecione o destino no SonoCraft, abra a página auxiliar no Chrome/Edge e escolha a aba com áudio. Mantenha essa página aberta. Outras abas não são capturadas. Há uma captura selecionada por computador de cada vez, além dos pads e do microfone.

No Windows da reunião: entrada SonoCraft = microfone físico; saída principal = CABLE Input; fones independentes = headset. No Teams: microfone = CABLE Output; alto-falante = headset. Mantenha a saída geral dos aplicativos no dispositivo físico, não no cabo virtual.

Captura de aplicativo: macOS 13+ com autorização do sistema; Windows com loopback por processo compatível (build 20348+, normalmente Windows 11). A seleção de abas pelo navegador é uma opção separada. O VB-CABLE deve ser instalado à parte.

Downloads: Mac universal (Apple Silicon/Intel), assinatura ad hoc, sem notarização; Windows x64 portátil, WASAPI, sem ASIO e sem assinatura comercial.

Os pacotes só são liberados após compilação e regressão nos dois sistemas. A regressão verifica áudio por UDP local e os destinos da captura; um teste manual com duas abas sintéticas verificou o isolamento de uma aba no Chrome do Mac. Dispositivos físicos, Teams, captura nativa de aplicativos e uso prolongado precisam de validação no equipamento real. Esta é uma prévia experimental.

Consulte docs/SONOCRAFT-SOURCES.md. Licença GPLv3 e exceção de lojas preservadas.

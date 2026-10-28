SonoCraft 0.4 — ouvir o Mac inteiro e escolher o que entra na reunião.

- Dois caminhos separados: áudio completo do Mac para os fones do Windows; voz, pads e seleção de aba/aplicativo para o microfone virtual.
- Nova área **Fontes**, com seleção de uma aba específica do Chrome/Edge ou captura de um aplicativo inteiro.
- O áudio completo da entrada virtual continua na escuta enquanto a seleção é enviada em faixa separada.
- A seleção não toca duas vezes nos fones quando já existe uma faixa de áudio completo.
- Destinos independentes para a seleção local: outro computador e/ou reunião neste computador. No Windows, escolha se a seleção recebida do Mac entra na reunião.
- **Parar captura** interrompe somente a seleção; mantém o áudio completo e os pads.
- Configuração por função e escolha direta dos dispositivos. Proteção contra retorno da saída ao mesmo cabo virtual da entrada.
- Pads preservados em canais separados mesmo com configurações mono/estéreo antigas.

Mac: entrada SonoCraft = VB-Cable; saída = Nenhum/Sem saída; som dos aplicativos direcionado ao VB-Cable no macOS. Windows: entrada = microfone físico; saída principal = CABLE Input; escuta = headset. No Teams: microfone CABLE Output e alto-falante headset. Atualize os dois computadores.

Para uma aba: no Mac, marque **Enviar seleção separada ao outro computador**, abra a página auxiliar no Chrome/Edge e escolha a aba com áudio. No Windows, marque **Na reunião, incluir a seleção recebida do Mac/PC**. O restante do Mac continua apenas nos fones. Há uma seleção por computador de cada vez, além da entrada completa e dos pads.

Captura de aplicativo: macOS 13+ com autorização do sistema; Windows com loopback por processo (build 20348+, normalmente Windows 11). O modo de aplicativo inclui todas as janelas dele; para separar abas, use o modo de aba. O VB-CABLE deve ser instalado à parte.

Downloads: Mac universal Apple Silicon/Intel, assinatura ad hoc e sem notarização; Windows x64 portátil, WASAPI, sem ASIO e sem assinatura comercial.

Os pacotes são liberados após compilação e regressão nos dois sistemas. A regressão utiliza UDP local e fontes de áudio sintéticas; um teste manual com duas abas verificou o isolamento de uma aba no Chrome do Mac. Headset físico, VB-CABLE, Teams, captura nativa de aplicativos e uso prolongado precisam de validação no equipamento real. Esta é uma prévia experimental.

Consulte docs/SONOCRAFT-SOURCES.md. Licença GPLv3 e exceção de lojas preservadas.

SonoCraft 0.4.0-preview.6 — escuta local e transmissão entre usuários, sem mistura para microfone virtual.

- Removidos o modo **Saída para apps**, sua mistura de áudio e todos os controles de inclusão de pads, recebidos e fontes em microfone virtual.
- Mantidos os pads, a captura de abas/aplicativos, a transmissão por IP/grupo, a gravação e a escuta física.
- Saídas virtuais reconhecidas são ocultadas nos seletores principal, avançado e de escuta adicional. Uma saída virtual salva anteriormente recebe silêncio, mesmo com uma entrada física.
- Parâmetros antigos da função removida são descartados ao carregar configurações ou presets.
- Textos, diagnóstico e guia dos pacotes atualizados para os caminhos restantes.

Após atualizar: no SonoCraft, escolha seus fones/alto-falantes como saída local. No Teams ou outro aplicativo de chamada, escolha o microfone físico do headset. As configurações de outros aplicativos não são alteradas automaticamente.

O bloqueio usa nomes conhecidos de dispositivos virtuais. Não controla drivers renomeados/desconhecidos, captura externa do áudio do sistema, compartilhamento de tela com som nem áudio de alto-falantes captado pelo microfone.

Prévia experimental. Pacotes publicados após compilação e testes em macOS e Windows. Mac universal com assinatura ad hoc, sem notarização; Windows x64 portátil, WASAPI, sem ASIO e sem assinatura comercial. GPLv3 e exceção de lojas preservadas.

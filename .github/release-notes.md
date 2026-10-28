SonoCraft 0.4 — captura respeita a entrada silenciada e diagnóstico dos caminhos de áudio.

- Corrigido um comportamento da tela Fontes que reativava uma entrada virtual silenciada ao iniciar uma captura de aba/aplicativo. Agora a seleção respeita o silêncio escolhido pelo usuário.
- Novo botão **Copiar diagnóstico de áudio** em **Áudio e saídas → Escuta e saídas**: reúne dispositivos, controles, canais e medidores para investigar configurações reais. Não grava áudio, não copia nomes de abas ou endereços de conexões e não envia dados automaticamente.
- Nova regressão: pad recebido continua na saída principal e nos fones ao ligar/desligar repetidamente o envio de uma aba, com a entrada física local silenciada.

Esta atualização corrige o acionamento indevido da entrada; não é uma confirmação de correção do corte de pads no Teams. O cenário real ainda exige diagnóstico no dispositivo afetado. Não altera a supressão de ruído de outros aplicativos.

Mantidos os caminhos separados: entrada completa para escuta; microfone físico, pads e fontes autorizadas para a saída virtual. Uma seleção de aba/aplicativo por dispositivo de cada vez. Atualize os dois lados e consulte **COMO-USAR-FONTES.md**, incluído nos pacotes.

Prévia experimental. Pacotes liberados após compilação e testes de áudio UDP local nos dois sistemas. Captura nativa, dispositivos físicos e uso prolongado ainda precisam de validação real.

Mac universal Apple Silicon/Intel com assinatura ad hoc, sem notarização. Windows x64 portátil, WASAPI, sem ASIO e sem assinatura comercial. Driver de cabo virtual instalado à parte. GPLv3 e exceção de lojas preservadas.

SonoCraft 0.4 — escuta completa e seleção independente para outros aplicativos.

- Interface genérica: **Entrada**, **Escuta**, **Envio aos conectados** e **Saída para aplicativos**, sem vincular o uso a reuniões ou a um sistema específico.
- Área **Fontes** organizada em seleção local, destinos e fontes recebidas.
- A entrada completa continua chegando aos fones enquanto uma aba/aplicativo selecionado segue em faixa separada.
- Corrigida a indicação de saída desativada na aba Áudio: um dispositivo somente de entrada podia aparecer também como saída, mesmo sem estar aberto nessa direção.
- Preservadas as proteções contra retorno ao cabo, duplicação na escuta e eventos antigos que desligavam a recepção.
- Guia **COMO-USAR-FONTES.md** atualizado dentro dos dois pacotes.

Para ouvir tudo e compartilhar apenas uma seleção: mantenha **Enviar também todo o áudio recebido** desligado. Na origem, escolha uma fonte e marque **Enviar esta fonte aos conectados**. No destino, marque **Usar fontes recebidas na saída para aplicativos**. Microfone físico local e pads também compõem essa saída; a escuta é independente.

Cabo virtual de saída: selecione a ponta de reprodução no SonoCraft (ex.: CABLE Input) e a ponta de gravação como microfone do aplicativo de destino (ex.: CABLE Output). Fones físicos ficam em Escuta independente. Quando a entrada captura todo o sistema por um cabo, evite usar esse mesmo cabo como saída do SonoCraft; escolha Sem saída principal para somente transmitir.

Uma seleção por dispositivo de cada vez. Para separar abas do mesmo navegador, use **Uma aba do navegador** pelo Chrome/Edge; aplicativo inteiro inclui todas as janelas dele. Captura nativa: macOS 13+ com autorização; Windows build 20348+, normalmente Windows 11. Driver de cabo virtual instalado à parte.

Prévia experimental. Os pacotes são liberados após compilação e regressão nos dois sistemas. Os testes usam UDP local e fontes sintéticas; isolamento de aba foi verificado manualmente no Chrome do Mac. Dispositivos físicos, captura nativa de aplicativos e uso prolongado precisam de validação real.

Mac universal Apple Silicon/Intel com assinatura ad hoc, sem notarização. Windows x64 portátil, WASAPI, sem ASIO e sem assinatura comercial. GPLv3 e exceção de lojas preservadas.

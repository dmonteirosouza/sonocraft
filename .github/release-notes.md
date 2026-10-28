SonoCraft 0.3 — duas saídas para reunião e fones.

- Estabilidade ao criar mesas e reutilizar sons no Windows, preservando as referências dos pads.
- Correção de uma condição em que o encerramento da negociação temporária interrompia o fluxo de áudio já conectado.
- **Fones:** áudio recebido dos participantes (incluindo o Mac) e pads locais, sem sua própria voz.
- **Reunião:** seu microfone, áudio recebido e pads locais na saída principal, que pode ser conectada ao Teams por VB-CABLE.
- Nova área **Áudio e reunião → Reunião e fones**, com instruções, resumo dos dispositivos e seleção independente dos fones.
- **Silenciar** na entrada corta somente o microfone e mantém o envio dos pads.
- Controle visível **Enviar pads a todos os conectados** no Soundboard.
- Pads em grade com preenchimento completo da cor escolhida. Adição pelo card **+**.
- **⋮ → Reutilizar em outra mesa**, preservando arquivo e ajustes iniciais, com configurações independentes.
- Mantidos os controles do metrônomo, chat, remoção segura de conexões e as correções de áudio e encerramento do Soundboard.

Configuração no Windows: entrada SonoCraft = microfone do headset; saída principal = CABLE Input; modo reunião ativo; saída independente = headset. No Teams: microfone = CABLE Output; alto-falante = headset. Desative **Escutar este dispositivo** do cabo virtual no Windows. No Mac que dispara os pads, ative **Enviar pads a todos os conectados**.

O VB-CABLE ou outro dispositivo virtual compatível deve ser instalado separadamente. Consulte docs/SONOCRAFT-MEETING.md no código-fonte.

Downloads:
- **SonoCraft-mac-universal.zip:** Apple Silicon e Intel, com assinatura ad hoc; sem notarização.
- **SonoCraft-windows-x64.zip:** executável portátil Windows x64, WASAPI; sem ASIO e sem assinatura comercial.

Os pacotes só são publicados após compilar e passar os testes nos dois sistemas. Os testes utilizam UDP local e arquivos de áudio reais; não substituem a validação com headset físico, Teams, Mac → Windows e uso prolongado. Esta versão é uma prévia experimental.

Licença GPLv3 e exceção de lojas preservadas. Consulte SONOCRAFT.md e os avisos das dependências.

SonoCraft 0.2 — nova mesa de áudio e modo reunião.

Novidades:
- Interface de mesa com canais lado a lado, faders verticais, medidores, entrada local e master separados.
- Tema claro padronizado nas abas internas e configurações em janela nativa.
- Botões com nomes completos: Silenciar, Solo e Monitorar.
- Chat e Soundboard acessíveis pela nova interface; metrônomo com ativação, BPM, volume e envio ao grupo.
- Remoção manual por conexão estável, evitando que uma ação antiga remova outro participante após reordenar a lista. A reconexão ainda não substitui automaticamente uma entrada antiga.
- Silenciamento local do participante, com retomada sem renegociar a conexão.
- Ajuste do fluxo de áudio ao estabilizar o tamanho dos blocos, evitando desligar e religar a recepção nessa transição.
- **Modo reunião:** saída principal com voz local e Soundboard, excluindo o retorno dos participantes, o transporte de arquivos e o metrônomo.

Para usar o modo reunião, configure um dispositivo virtual compatível (como VB-CABLE) separadamente. Em Dispositivos, selecione microfone físico como entrada e cabo virtual como saída. No aplicativo da reunião, escolha esse cabo como microfone e seus fones como saída. O SonoCraft não instala um microfone virtual próprio nem abre uma segunda saída independente para monitorar voz e pads. Consulte docs/SONOCRAFT-MEETING.md no código-fonte.

Downloads:
- **SonoCraft-mac-universal.zip:** macOS, Apple Silicon e Intel, com assinatura local ad hoc; sem notarização.
- **SonoCraft-windows-x64.zip:** Windows x64 portátil, com WASAPI; sem ASIO e sem assinatura comercial.

Os pacotes são publicados após compilação e testes de regressão nos dois sistemas. Os testes cobrem áudio UDP local, blocos de tamanho variável, silenciamento e retomada, remoção por identidade, voz local sem retorno remoto no modo reunião e reprodução de um arquivo real do Soundboard.

Esta é uma prévia experimental. Ainda requer validação com dispositivos reais, numa reunião real, Mac → Windows, grupo/senha e uso prolongado. Os testes automáticos não comprovam que todas as falhas foram resolvidas.

O aplicativo usa configurações separadas das do SonoBus. Ao conectar por IP, informe IP:porta exibido no outro computador.

Licença GPLv3 e exceção de lojas preservadas. Consulte SONOCRAFT.md e os avisos das dependências no código-fonte deste tag.

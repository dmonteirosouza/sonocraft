SonoCraft 0.4.0-preview.13 — medidores e estados de áudio mais claros

- Setas dos menus desenhadas sem depender de caracteres da fonte, corrigindo o quadrado no Windows.
- Medidor do participante considera também pads e aplicativos recebidos em canais separados, respeitando grupos mono e estéreo.
- No Windows, aplicativos detectados mostram sua atividade de áudio mesmo sem captura ou envio ativados. A leitura dos medidores não liga o compartilhamento.
- Cada canal distingue o estado do áudio (Tocando, Capturando, Enviando ou Silenciado) da configuração de escuta e envio.
- Indicações consideram ausência de conexão, silêncio geral, volume zerado, solo e captura aguardando autorização.
- O alcance de Silenciar recebidos está explicado: pads, arquivos e metrônomo locais continuam independentes.
- Reprodução de arquivo, gravação e metrônomo ativos permanecem visíveis com seus painéis fechados. Clique no indicador para abrir os controles.
- Layout dos canais revisado em janelas amplas e compactas, com textos e controles alinhados.
- A saída para microfone virtual permanece removida.

**Onde configurar:** Ajustes → Áudio para entrada e saídas; Conexões para grupo/IP; Fontes para aplicativos, abas e modo de envio. A saída principal já reproduz participantes e pads. A segunda saída é opcional, para outro dispositivo físico.

**Validação da publicação:** builds macOS Universal e Windows x64, regressão de áudio UDP e verificação dos ZIPs. No Mac, conferência de Intel/Apple Silicon e assinatura após extrair o pacote. Testes automatizados não substituem o uso com dispositivos físicos e captura real de aplicativos.

**Compatibilidade e limites:** use a mesma prévia nos computadores conectados. Detecção automática requer macOS 14.2+ ou Windows; captura por aplicativo requer macOS 13+ ou Windows compatível (normalmente Windows 11). Abas usam Chrome/Edge e a página auxiliar aberta. Os controles individuais ficam na origem; o destinatário recebe a mistura estéreo. As novas telas priorizam português brasileiro; a tradução integral para outros idiomas ainda não foi revisada.

Prévia experimental, sem assinatura comercial/notarização. O Mac usa assinatura provisória e pode exigir nova autorização de captura após a atualização. Leia COMO-USAR-FONTES.md dentro do pacote.

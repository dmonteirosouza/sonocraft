SonoCraft 0.4.0-preview.12 — mesa e configurações padronizadas

- Canais da mesa com o mesmo desenho, escala em dB, medidores e controles alinhados. Silenciar, Ouvir entrada, Ouvir só este e Enviar têm nomes e explicações próprios.
- Barra superior única: Conexões, Fontes, Pads, Conversa, Ferramentas e Ajustes. Atalhos duplicados e a barra inferior foram removidos.
- Arquivo de áudio, gravação e metrônomo abrem painéis identificados. Fechar o painel apenas oculta os controles; uma gravação em andamento continua indicada.
- Configurações organizadas em Áudio, Conexão, Gravação e Preferências. Entrada, saída principal e segunda saída opcional ficam juntas. A mesa de pads não repete a escolha de dispositivos.
- Dispositivos são aplicados ao selecionar, sem redefinir silenciamentos ou ligar envios. A segunda saída é desligada se passar a usar o mesmo dispositivo da principal, para evitar som duplicado.
- Nomes, explicações, acentos e traduções em português revisados, inclusive nas opções de reprodução dos pads. Ajustes avançados expandem corretamente e os textos se adaptam a janelas menores.
- Removida a opção de atualização automática que não estava disponível nesta prévia.
- Aplicativos com áudio continuam aparecendo automaticamente. Pausar preserva ajustes; fechar o programa remove o canal. A saída para microfone virtual permanece removida.

**Onde configurar:** Ajustes → Áudio para entrada e saídas; Conexões para grupo/IP; Fontes para aplicativos, abas e modo de envio. A saída principal já reproduz participantes e pads. A segunda saída é opcional, para outro dispositivo físico.

**Validação da publicação:** builds macOS Universal e Windows x64, regressão de áudio UDP e verificação dos ZIPs. No Mac, conferência de Intel/Apple Silicon e assinatura após extrair o pacote. Testes automatizados não substituem o uso com dispositivos físicos e captura real de aplicativos.

**Compatibilidade e limites:** use a mesma prévia nos computadores conectados. Detecção automática requer macOS 14.2+ ou Windows; captura por aplicativo requer macOS 13+ ou Windows compatível (normalmente Windows 11). Abas usam Chrome/Edge e a página auxiliar aberta. Os controles individuais ficam na origem; o destinatário recebe a mistura estéreo. As novas telas priorizam português brasileiro; a tradução integral para outros idiomas ainda não foi revisada.

Prévia experimental, sem assinatura comercial/notarização. O Mac usa assinatura provisória e pode exigir nova autorização de captura após a atualização. Leia COMO-USAR-FONTES.md dentro do pacote.

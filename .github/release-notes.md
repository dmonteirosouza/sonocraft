SonoCraft 0.4.0-preview.14 — pads e mesa renovados, correção de recebimento

- Corrige o recebimento de áudio quando a saída principal está em Nenhum: a segunda saída física pode reproduzir participantes, pads e fontes recebidas.
- Mantém a proteção contra saída para microfone virtual sem impedir o envio pela rede.
- Edição de pads com opções de reprodução legíveis, campos com nomes completos, explicações e botão Concluído.
- Pads maiores, com número, atalho configurado, progresso e alça dedicada para ordenar sem disparar o som.
- Ordenação por arraste ou menu (antes, depois, início e final), com ordem salva entre sessões. Ordenar interrompe os pads em reprodução.
- Reutilização de sons em outra mesa preservada; a ação de copiar opções agora informa que afeta os outros pads da mesa.
- Canais com faders mais definidos, escala graduada e medidores segmentados. Status com texto e pequeno indicador colorido, sem faixa de fundo.
- Espaçamento corrigido entre o seletor de aplicativo e Atualizar aplicativos em Fontes.
- Diagnóstico de áudio ampliado com níveis de envio/fontes, estado de transmissão e contagem de pacotes por conexão.

**Onde configurar:** Ajustes → Áudio para entrada e saídas; Conexões para grupo/IP; Fontes para aplicativos, abas e modo de envio. A saída principal já reproduz participantes e pads. A segunda saída é opcional, para outro dispositivo físico.

**Validação da publicação:** builds macOS Universal e Windows x64, regressão de áudio UDP e verificação dos ZIPs. No Mac, conferência de Intel/Apple Silicon e assinatura após extrair o pacote. Testes automatizados não substituem o uso com dispositivos físicos e captura real de aplicativos.

**Compatibilidade e limites:** use a mesma prévia nos computadores conectados. Detecção automática requer macOS 14.2+ ou Windows; captura por aplicativo requer macOS 13+ ou Windows compatível (normalmente Windows 11). Abas usam Chrome/Edge e a página auxiliar aberta. Os controles individuais ficam na origem; o destinatário recebe a mistura estéreo. As novas telas priorizam português brasileiro; a tradução integral para outros idiomas ainda não foi revisada.

Prévia experimental, sem assinatura comercial/notarização. O Mac usa assinatura provisória e pode exigir nova autorização de captura após a atualização. Leia COMO-USAR-FONTES.md dentro do pacote.

SonoCraft 0.4.0-preview.15 — medidores isolados e interface padronizada

- Corrige o medidor automático do Windows que podia acompanhar a saída inteira do computador no canal de um aplicativo de chamada, como o Teams. A medição passa a usar o áudio reproduzido pelo processo do aplicativo, separado das sessões usadas para cancelamento de eco.
- A medição automática não grava, reproduz nem envia áudio. O botão Enviar continua independente. Medidores são liberados quando a sessão fica inativa; falhas não voltam a usar o medidor misturado.
- Diagnóstico identifica os nomes das fontes e dos participantes, a origem da medição e o estado da captura.
- Configurações, conexões, conversa, pads e editores usam painéis consistentes, com título e botão de fechar à direita, sem moldura permanente. Canais de entrada e saída usam checkbox e texto proporcionais aos demais campos.
- Preserva edições ao fechar e reabrir painéis; mantém o salvamento das opções dos pads.
- Inclui no código as melhorias mobile: identificador net.sonocraft.audio, layout para toque, seletores legíveis e menor custo de atualização da interface. Os downloads desta publicação são para Windows e macOS; não inclui instalador iOS ou nova versão Android.

**Downloads:** Windows x64 e macOS Universal (Apple Silicon e Intel). Extraia o ZIP antes de executar. Use a mesma prévia nos computadores conectados.

**Validação:** a publicação depende das compilações dos dois sistemas, testes de áudio/UDP e ciclo de vida dos painéis, e verificação dos pacotes. No Mac, o pacote é conferido nas duas arquiteturas e recebe assinatura provisória. Os testes automatizados não substituem a validação com o Teams e os dispositivos físicos; essa confirmação continua necessária no Windows.

**Como verificar a correção:** com o Teams em chamada e sem áudio dos participantes, reproduza e pause o Chrome do outro computador. O canal do participante remoto deve acompanhar o som; o canal local do Teams deve acompanhar apenas o áudio reproduzido pelo próprio Teams, inclusive com o envio desligado.

**Compatibilidade:** a medição isolada e a captura por aplicativo no Windows requerem build 20348 ou posterior (normalmente Windows 11). Em sistemas incompatíveis, o medidor informa que a leitura está indisponível. No Mac, detecção automática requer macOS 14.2+ e captura por aplicativo requer macOS 13+. A saída para microfone virtual permanece removida. Captura local de aplicativos/abas não está disponível no mobile.

Prévia experimental, sem assinatura comercial/notarização. O Mac usa assinatura provisória e pode exigir nova autorização de captura após a atualização. Leia COMO-USAR-FONTES.md dentro do pacote.

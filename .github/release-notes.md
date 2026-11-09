SonoCraft 0.4.0-preview.21 — correção de áudio recebido em headsets

- Corrige uma situação em que o participante aparecia conectado e recebia pacotes, mas seus medidores e a saída permaneciam em silêncio. O buffer de conversão agora considera a diferença entre as taxas de áudio dos computadores.
- Cobre especificamente o envio a 48 kHz para um headset operando a 16 kHz, inclusive quando aplicativos e pads são transmitidos em canais separados. A correção deve ser instalada no computador que recebe o áudio.
- Preserva o alinhamento dos canais ao circular pelo buffer e inclui testes de sete combinações de taxas, canais mono/estéreo/multicanal e transmissão real por UDP, verificando também que a frequência do som é preservada.
- Mantém os painéis integrados, o mixer de fontes e a remoção da saída para microfone virtual.

**Downloads:** Windows x64 e macOS Universal (Apple Silicon e Intel). Extraia o ZIP antes de executar. Feche todas as instâncias antigas e abra apenas uma cópia da nova versão. Um nome com sufixo “2” indica que o nome anterior estava ocupado no servidor; esta prévia não altera essa regra.

**Validação:** publicação condicionada às compilações, testes de áudio/UDP, testes de painéis e verificação dos pacotes nos dois sistemas. A reprodução do defeito e a correção foram verificadas em teste automatizado; o teste no headset físico do usuário ainda está pendente.

**Compatibilidade:** medição isolada e captura por aplicativo no Windows requerem build 20348 ou posterior (normalmente Windows 11). No Mac, detecção automática requer macOS 14.2+ e captura por aplicativo requer macOS 13+. Esta publicação não inclui um novo APK ou instalador iOS.

Prévia experimental, sem assinatura comercial/notarização. O Mac usa assinatura provisória e pode exigir nova autorização de captura após a atualização. Leia COMO-USAR-FONTES.md dentro do pacote.

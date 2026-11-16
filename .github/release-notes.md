SonoCraft 0.4.0-preview.22 — recuperação automática do áudio

- Corrige o status “Enviando” quando o motor de áudio está parado. A mesa passa a distinguir áudio interrompido, recuperação em andamento, dispositivo indisponível e captura sem sinal.
- No aplicativo desktop, detecta quando o dispositivo para ou deixa de processar áudio e tenta reabrir o mesmo dispositivo automaticamente. Mantém a sessão, as fontes e os controles de volume e envio.
- Aguarda antes de tentar novamente em caso de falha, respeita a seleção “Nenhum” e não escolhe outro dispositivo como substituto durante a recuperação. Silêncio normal não provoca reinícios.
- Amplia o diagnóstico de áudio com o estado do motor, o número de tentativas de recuperação e o erro do dispositivo.
- Preserva a correção de áudio entre taxas diferentes, os painéis integrados, o mixer de fontes e a remoção da saída para microfone virtual.

**Downloads:** Windows x64 e macOS Universal (Apple Silicon e Intel). Extraia o ZIP antes de executar. Feche todas as instâncias antigas e abra apenas uma cópia da nova versão.

**Validação:** publicação condicionada às compilações, testes de recuperação simulada do dispositivo, áudio/UDP, conversão de taxas, painéis e verificação dos pacotes nos dois sistemas. A recuperação foi testada com dispositivo simulado; a recorrência da falha física ainda depende de acompanhamento no uso.

**Compatibilidade:** medição isolada e captura por aplicativo no Windows requerem build 20348 ou posterior (normalmente Windows 11). No Mac, detecção automática requer macOS 14.2+ e captura por aplicativo requer macOS 13+. Esta publicação não inclui um novo APK ou instalador iOS.

Prévia experimental, sem assinatura comercial/notarização. O Mac usa assinatura provisória e pode exigir nova autorização de captura após a atualização. Leia COMO-USAR-FONTES.md dentro do pacote.

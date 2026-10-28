# Configuração de áudio

Na prévia 0.4, use o passo a passo de [Fontes e destinos de áudio](SONOCRAFT-SOURCES.md). A função do computador, o microfone e a saída principal são escolhidos diretamente em **Áudio e reunião → Reunião e fones**.

## Mesas e pads

A organização continua no seletor de mesas e no menu existentes. Use o card **+** para adicionar um som. O card inteiro usa a cor escolhida, com texto em contraste. No menu **⋮** do pad, **Reutilizar em outra mesa** conserva arquivo, nome, cor e volume, com ajustes independentes. Mesas que já contêm o arquivo ficam desabilitadas nesse menu.

## Validação

A regressão utiliza duas instâncias por UDP local e arquivos WAV reais para verificar: retomada após silenciamento, mistura de microfone e recebidos na saída da reunião, ausência do microfone local na escuta independente, pads locais e recebidos, envio de pads mesmo com voz remota ativa sem incluí-la na reunião, bloqueio e inclusão opcional do áudio geral recebido e persistência da reutilização de sons. O funcionamento com dispositivos físicos, Teams e uso prolongado ainda requer teste no equipamento real.

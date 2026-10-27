# Modo reunião — prévia local

O modo reunião muda a saída principal do SonoCraft para uma mistura de sua voz e dos sons do Soundboard. O áudio recebido dos participantes, a reprodução de arquivos do transporte e o metrônomo ficam fora dessa saída.

## Configuração

1. Instale e configure um dispositivo de áudio virtual compatível com seu sistema, como VB-CABLE. O SonoCraft ainda não instala nem cria um microfone virtual próprio.
2. Em **Dispositivos**, selecione o microfone físico como entrada e o cabo virtual como saída. Não use o mesmo cabo virtual como entrada e saída do SonoCraft.
3. No aplicativo da reunião, selecione o cabo virtual como microfone e seus fones como saída.
4. Ative **Modo reunião** no SonoCraft.
5. Ajuste sua voz no canal **Sua entrada** e os sons no volume próprio do **Soundboard**.

**Silenciar** no canal de entrada corta a voz também da mistura da reunião. **Monitorar** fica desativado nesse modo porque a voz já faz parte da saída. O controle **Enviar ao grupo** do Soundboard continua controlando apenas o envio aos participantes do SonoCraft; os pads em reprodução entram na mistura local da reunião independentemente dele.

Esta prévia usa a saída principal configurada no SonoCraft. Ela não abre uma segunda saída independente para monitorar localmente os pads e a voz nos fones. O áudio de retorno da reunião deve sair pelos fones selecionados no próprio aplicativo da reunião.

## Silenciamento de participantes

**Silenciar** em um canal de participante aplica um bloqueio local ao áudio desse canal. O fluxo de rede permanece ativo, permitindo restaurar o som sem uma nova conexão. Esse bloqueio é independente do controle global de recepção.

## Verificação

O teste de regressão verifica áudio real por UDP local, silenciamento e retomada do participante, voz local no modo reunião, exclusão do retorno dos participantes e reprodução de um arquivo WAV pelo Soundboard. A validação em um aplicativo de reunião real e entre macOS e Windows deve ser feita antes da publicação.

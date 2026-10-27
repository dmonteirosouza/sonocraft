# Modo reunião — prévia local

O modo reunião muda a saída principal do SonoCraft para uma mistura de sua voz e dos sons do Soundboard. O áudio recebido dos participantes, a reprodução de arquivos do transporte e o metrônomo ficam fora dessa saída.

## Configuração

1. Instale e configure um dispositivo de áudio virtual compatível com seu sistema, como VB-CABLE. O SonoCraft ainda não instala nem cria um microfone virtual próprio.
2. Em **Dispositivos**, selecione o microfone físico como entrada e o cabo virtual como saída. Não use o mesmo cabo virtual como entrada e saída do SonoCraft.
3. No aplicativo da reunião, selecione o cabo virtual como microfone e seus fones como saída.
4. Ative **Modo reunião** no SonoCraft.
5. Ajuste sua voz no canal **Sua entrada** e os sons no volume próprio do **Soundboard**.

**Silenciar** no canal de entrada corta a voz também da mistura da reunião. **Monitorar** fica desativado nesse modo porque a voz já faz parte da saída. O controle **Enviar ao grupo** do Soundboard continua controlando apenas o envio aos participantes do SonoCraft; os pads em reprodução entram na mistura local da reunião independentemente dele.

## Ouvir os pads nos fones sem ouvir sua voz

O painel Soundboard agora oferece uma segunda saída, exclusiva para os pads:

1. Mantenha a saída principal do SonoCraft no cabo virtual usado como microfone da reunião.
2. Abra o **Soundboard** e selecione o headset físico em **Selecione seu headset ou fone**. Essa saída deve ser o fone, não o VB-CABLE.
3. Marque **Ouvir somente pads nos fones** e ajuste o volume logo abaixo. Esse volume não altera o volume enviado à reunião.
4. No Windows, deixe desativada a opção **Escutar este dispositivo** do CABLE Output; ela reproduziria também a voz presente na mistura principal.

O áudio de retorno do Teams continua saindo pelos fones selecionados no Teams. A saída adicional do SonoCraft recebe exclusivamente o Soundboard, sem microfone, participantes recebidos, metrônomo ou reprodução do transporte. Ela fica desligada ao iniciar o aplicativo até que o dispositivo seja selecionado e ativado.

A organização das mesas foi mantida no seletor e no menu existentes. Os sons aparecem em pads em grade, com o card inteiro na cor escolhida, progresso de reprodução e acesso às opções pelo botão de três pontos. Para adicionar um som, use o card com o sinal de mais. No menu de cada pad, **Reutilizar em outra mesa** inclui o mesmo arquivo na mesa escolhida, preservando nome, cor e volume, com ajustes independentes. Mesas que já contêm esse arquivo ficam desabilitadas no menu.

## Silenciamento de participantes

**Silenciar** em um canal de participante aplica um bloqueio local ao áudio desse canal. O fluxo de rede permanece ativo, permitindo restaurar o som sem uma nova conexão. Esse bloqueio é independente do controle global de recepção.

## Verificação

O teste de regressão verifica áudio real por UDP local, silenciamento e retomada do participante, voz local no modo reunião, exclusão do retorno dos participantes reprodução de um arquivo WAV pelo Soundboard e processamento da fila exclusiva dos pads com ganho, silêncio e taxas de 44,1/48/96 kHz. A validação em um aplicativo de reunião real e entre macOS e Windows deve ser feita antes da publicação.

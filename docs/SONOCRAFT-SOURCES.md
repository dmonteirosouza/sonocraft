# Entrada, escuta e transmissão

O SonoCraft transmite áudio entre seus participantes e reproduz os sons localmente. A função de misturar sons em um microfone virtual foi removida.

| Caminho | Para que serve |
| --- | --- |
| Entrada | Recebe um microfone ou captura do sistema por um cabo virtual, para transmitir aos conectados. |
| Saída local | Reproduz os sons recebidos e os pads em fones ou alto-falantes. |
| Escuta adicional | Reproduz os recebidos e pads em uma segunda saída física, sem a voz local. É opcional. |
| Envio aos conectados | Transmite entrada, pads habilitados e uma fonte selecionada para a conexão ou grupo do SonoCraft. |

## Ouvir em outro computador

No computador que transmite:

1. Para enviar o som completo do sistema, use o cabo virtual como saída do sistema e como **entrada** do SonoCraft. Aplicativos com saída própria também precisam usar esse cabo.
2. Em **Áudio e saídas → Escuta e saídas**, escolha **Sem saída principal (só transmitir)**, ou uma saída física caso também queira ouvir localmente.
3. Clique em **Aplicar dispositivos e saída**, mantenha a entrada ativa e conecte por IP ou grupo.
4. Na mesa de sons, **Enviar pads a todos os conectados** controla o envio dos pads pelo SonoCraft.

No computador que recebe:

1. Escolha o microfone físico ou **Sem entrada**, conforme queira transmitir sua voz aos conectados.
2. Escolha seus **fones ou alto-falantes como saída local**.
3. Clique em **Aplicar dispositivos e saída** e mantenha **Receber áudio dos conectados** ligado.
4. A escuta adicional só é necessária para uma segunda saída física. Não use a mesma saída nos dois caminhos.

## Escolher uma aba ou aplicativo

Em **Fontes**, escolha **Uma aba do navegador** ou **Um aplicativo inteiro**, marque **Enviar esta fonte aos conectados** e inicie a captura. O aplicativo inteiro inclui todas as janelas e abas dele.

Para uma aba, abra a página auxiliar no Chrome/Edge, escolha a aba com áudio e mantenha a página aberta. O navegador solicita autorização; o SonoCraft encaminha apenas áudio, sem enviar vídeo ou microfone por essa captura.

A seleção é feita no computador onde o som toca. Uma seleção por dispositivo de cada vez. Nenhuma captura começa automaticamente ao abrir o aplicativo. **Parar captura** ou desligar o envio interrompe apenas a seleção; a entrada e os pads mantêm seus controles independentes.

Quando o áudio completo da entrada já contém a seleção, ela não toca duas vezes na escuta remota. Para ouvir uma fonte local, mantenha sua reprodução original nos fones do sistema.

## Reuniões e configurações antigas

O SonoCraft não oferece mais saída para aplicativos/microfone virtual, nem controles para misturar pads, sons recebidos ou fontes selecionadas nesse destino. Configurações antigas desses recursos são descartadas ao carregar.

Saídas reconhecidas como cabos ou dispositivos virtuais são removidas dos seletores principal, avançado e de escuta adicional. Se uma saída virtual antiga ainda estiver salva, o áudio dessa saída é bloqueado até você escolher uma saída física. Cabos virtuais continuam disponíveis como **entrada**, para transmissão entre computadores.

No Teams ou outro aplicativo de chamada, selecione o **microfone físico do headset** como microfone e seus fones como alto-falante. O SonoCraft não altera as configurações desses aplicativos.

Esta remoção elimina o envio por microfone virtual oferecido pelo SonoCraft. Ela não controla captura de áudio do sistema/compartilhamento de tela feita por outros aplicativos, roteamentos externos ou som de alto-falantes captado pelo microfone. A identificação de dispositivos virtuais usa os nomes conhecidos; dispositivos renomeados ou drivers desconhecidos podem não ser reconhecidos.

## Diagnóstico e compatibilidade

Em **Áudio e saídas → Escuta e saídas**, **Copiar diagnóstico de áudio** copia dispositivos, controles e medidores. Não grava áudio nem envia o diagnóstico automaticamente.

Captura de aplicativo: macOS 13+ com permissão de áudio/tela; Windows com loopback por processo (build 20348+, normalmente Windows 11). Captura de abas pelo Chrome/Edge é uma alternativa separada. Um driver de cabo virtual só é necessário para a captura completa por essa entrada.

Os testes cobrem transmissão UDP, silenciamento de participantes, pads, fontes selecionadas, escuta física, remoção de parâmetros antigos e bloqueio de saída virtual. Dispositivos físicos e uso prolongado continuam exigindo validação real.

# Entrada, escuta e transmissão

O SonoCraft transmite áudio entre seus participantes e reproduz os sons localmente. A função de misturar sons em um microfone virtual foi removida.

| Caminho | Para que serve |
| --- | --- |
| Entrada | Recebe um microfone ou captura do sistema por um cabo virtual, para transmitir aos conectados. |
| Saída local | Reproduz os sons recebidos e os pads em fones ou alto-falantes. |
| Escuta adicional | Reproduz os recebidos e pads em uma segunda saída física, sem a voz local. É opcional. |
| Envio aos conectados | Transmite entrada, pads habilitados e as fontes habilitadas para a conexão ou grupo do SonoCraft. |

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

## Mixer de fontes

Os aplicativos com reprodução de áudio aparecem **automaticamente na mesa**, sem precisar cadastrá-los. A detecção consulta sessões de saída ativas no Windows e processos de áudio no macOS 14.2+. Não grava áudio nem solicita permissão de captura. Uma sessão ativa pode estar momentaneamente silenciosa, como acontece no mixer do sistema.

Cada canal aparece com o envio desligado. Ajuste o volume e ative **Enviar** para iniciar a captura e compartilhar com os conectados. Canais que pausam permanecem na mesa, preservando os ajustes. **Quando o programa é fechado, sua captura é encerrada e o canal sai automaticamente da mesa**, normalmente na próxima atualização (cerca de 1,5 segundo). Reabrir o programa cria um novo canal com envio desligado. A entrada completa do sistema não é reativada quando o último programa fecha. Se remover um canal automático, ele permanece oculto até a reprodução parar e começar novamente.

Use **Adicionar fonte** para escolher uma aba ou antecipar um aplicativo que ainda não começou a tocar. É possível manter até **32 canais**. A aba Fontes foi retirada dos Ajustes. Em macOS anterior a 14.2, use a adição manual.

| Controle do canal | Efeito |
| --- | --- |
| Volume (dB) | 0 dB mantém o volume original; valores negativos reduzem e positivos aumentam. O mínimo silencia. Arraste, digite o valor ou dê duplo clique para voltar a 0 dB. Ajusta o envio, sem mudar o volume original do programa no sistema. |
| Medidor | Mostra o nível após iniciar a captura. Antes disso, “COM ÁUDIO” indica a atividade detectada pelo sistema. |
| Enviar | Inicia a captura quando necessário e inclui somente este canal na transmissão. Começa desligado. |
| Silenciar | Suspende o áudio enviado por este canal sem perder seu volume. |
| Parar / Iniciar captura | Libera ou reabre a captura. Ao reiniciar, o envio começa desligado. |
| Remover | Retira o canal da mesa. Não exclui arquivos nem fecha o programa de origem. |
| Nome | Duplo clique para renomear. |

Os volumes, nomes e canais são salvos; ao reabrir o SonoCraft, as capturas ficam paradas e o envio desligado. Inicie e habilite os canais que quiser compartilhar. No destino, as fontes chegam em uma mistura estéreo chamada **Fontes**; seus controles independentes ficam no computador que transmite.

Para uma aba, abra a página auxiliar no **Chrome ou Edge**, escolha a aba e marque **Compartilhar áudio da aba**. Mantenha a página auxiliar aberta. Não são enviados vídeo nem microfone pela captura. **Adicione cada aba apenas uma vez:** o navegador não expõe uma identificação estável que permita detectar a mesma aba escolhida novamente. Essa limitação está indicada no seletor e na página auxiliar.

O aplicativo inteiro inclui todas as janelas/abas dele. O SonoCraft bloqueia aplicativo repetido. Adicionar uma aba desliga a captura/envio do navegador inteiro; o canal do navegador continua visível, mas fica bloqueado enquanto houver abas configuradas. Para voltar ao navegador inteiro, remova os canais de abas. Essa restrição é conservadora e vale mesmo que sejam navegadores diferentes.

## Padrão dos canais

Entrada, participantes, aplicativos, abas e saída principal usam o mesmo desenho, escala em dB, medidores e posição dos controles. **Silenciar** age no canal indicado. **Ouvir entrada** permite escutar sua entrada local; **Ouvir só este** isola participantes na escuta; **Enviar** compartilha uma fonte com os conectados. São funções diferentes, identificadas por texto completo e explicações ao passar o mouse. Na saída principal, **Silenciar recebidos** afeta os participantes, mantendo os pads locais. O volume da saída principal continua controlando a mistura local.

## Evitar áudio duplicado

Quando a entrada é um cabo virtual com o sistema completo, a mesa oferece dois modos:

- **Fontes individuais:** transmite os aplicativos/abas habilitados; a entrada completa do sistema fica fora do envio. Adicionar manualmente uma fonte ou habilitar Enviar seleciona este modo. A detecção automática não altera o modo.
- **Entrada completa:** transmite a entrada do sistema e desliga o envio de todos os canais individuais. Escolha **Usar entrada completa** para voltar a esse modo.

Desligar Enviar, parar uma captura ou remover o último canal **não reativa automaticamente a entrada completa**. Isso evita enviar outros programas por acidente. A detecção da entrada completa usa os nomes conhecidos de cabos virtuais; interfaces de loopback renomeadas ou desconhecidas não são identificadas automaticamente.

O microfone físico e os pads mantêm seus controles próprios. Para ouvir aplicativos locais, mantenha a reprodução original deles nos fones do sistema; o mixer não toca outra cópia local das capturas. No computador conectado, a saída física reproduz os canais enviados.

## Reuniões e configurações antigas

O SonoCraft não oferece mais saída para aplicativos/microfone virtual, nem controles para misturar pads, sons recebidos ou fontes selecionadas nesse destino. Configurações antigas desses recursos são descartadas ao carregar.

Saídas reconhecidas como cabos ou dispositivos virtuais são removidas dos seletores principal, avançado e de escuta adicional. Se uma saída virtual antiga ainda estiver salva, o áudio dessa saída é bloqueado até você escolher uma saída física. Cabos virtuais continuam disponíveis como **entrada**, para transmissão entre computadores.

No Teams ou outro aplicativo de chamada, selecione o **microfone físico do headset** como microfone e seus fones como alto-falante. O SonoCraft não altera as configurações desses aplicativos.

Esta remoção elimina o envio por microfone virtual oferecido pelo SonoCraft. Ela não controla captura de áudio do sistema/compartilhamento de tela feita por outros aplicativos, roteamentos externos ou som de alto-falantes captado pelo microfone. A identificação de dispositivos virtuais usa os nomes conhecidos; dispositivos renomeados ou drivers desconhecidos podem não ser reconhecidos.

## Diagnóstico e compatibilidade

Em **Áudio e saídas → Escuta e saídas**, **Copiar diagnóstico de áudio** copia dispositivos, controles e medidores. Não grava áudio nem envia o diagnóstico automaticamente.

Captura de aplicativo: macOS 13+ com permissão de áudio/tela; Windows com loopback por processo (build 20348+, normalmente Windows 11). Captura de abas pelo Chrome/Edge é uma alternativa separada. Um driver de cabo virtual só é necessário para a captura completa por essa entrada.

Os testes cobrem transmissão UDP, silenciamento de participantes, pads, fontes simultâneas com volume/envio/silêncio independentes, alternância dos modos completo/individual, escuta física, remoção de parâmetros antigos e bloqueio de saída virtual. Dispositivos físicos e uso prolongado continuam exigindo validação real.

## Permissão no macOS

O aplicativo consulta a autorização antes de abrir a captura. Quando a permissão não é reconhecida, mostra **Autorizar captura**; a detecção automática e as atualizações da mesa não repetem solicitações. O botão solicita acesso uma vez por execução; tentativas seguintes levam aos Ajustes do sistema.

Use uma única instalação em um caminho fixo, por exemplo `/Applications/SonoCraft.app`, substituindo-a ao atualizar. As prévias usam assinatura ad hoc: a identidade de assinatura muda entre compilações, portanto uma autorização antiga pode não valer para a nova versão, mesmo que “SonoCraft” apareça habilitado nos Ajustes. Pode ser necessário atualizar a autorização e reabrir o aplicativo. Sem certificado Developer ID, não há garantia de preservar essas autorizações entre versões. O SonoCraft não altera nem remove as permissões do macOS.

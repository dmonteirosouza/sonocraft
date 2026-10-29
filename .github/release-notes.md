SonoCraft 0.4.0-preview.8 — mixer automático de aplicativos

- Aplicativos com reprodução de áudio aparecem automaticamente, com envio inicialmente desligado. Até 32 fontes na mesa principal, com volume, medidor, envio, silêncio e captura independentes.
- Volume e canais mantidos nas pausas. Reiniciar um aplicativo desliga seu envio. A detecção não abre capturas nem altera o modo de transmissão.
- Contraste corrigido nos valores dos faders e textos UTF-8 nas mensagens do macOS.
- Verificação de permissão antes da captura; nenhuma solicitação automática em loop. A assinatura provisória ainda pode exigir nova autorização após atualizar.
- “Adicionar fonte” substitui a aba Fontes dos Ajustes. Nomes editáveis e canais/volumes salvos; capturas e envios recomeçam desligados.
- Proteção contra aplicativo repetido, navegador inteiro junto com abas e entrada completa do sistema junto com fontes individuais.
- “Usar entrada completa” desliga os envios individuais. Desligar/remover fontes nunca reativa a entrada completa sozinho.
- A saída para microfone virtual permanece removida e os dispositivos virtuais conhecidos continuam bloqueados nas saídas.

**Como usar:** reproduza áudio em um aplicativo → ele aparece na mesa → ajuste o volume → ligue Enviar. Para uma aba, use Adicionar fonte. O volume ajusta o que os conectados recebem, sem mudar a reprodução original do programa.

Abas exigem Chrome/Edge e a página auxiliar aberta. Adicione cada aba apenas uma vez: o navegador não permite identificar com segurança uma aba selecionada novamente. As fontes chegam juntas em estéreo ao destinatário; os controles individuais ficam na origem. Detecção automática requer macOS 14.2+ ou Windows; em versões anteriores use adição manual. Captura por aplicativo requer macOS 13+ ou Windows com loopback por processo (normalmente Windows 11).

Inclui testes de fontes simultâneas, ganhos, silêncio/envio, persistência, modos de captura, transmissão UDP, pads, recepção e bloqueio de saídas virtuais. Prévia experimental; macOS e Windows sem assinatura comercial/notarização. Leia COMO-USAR-FONTES.md dentro do pacote.

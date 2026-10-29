# SonoCraft 0.4 — prévia experimental

Derivado do SonoBus 1.7.2, com avisos de autoria, GPLv3 e exceção de distribuição em lojas preservados.

## Começar

O SonoCraft reproduz áudio localmente e transmite para outros participantes do SonoCraft. A saída para microfone virtual permanece removida.

Leia [o guia de fontes e dispositivos](COMO-USAR-FONTES.md) incluído no pacote. No repositório, ele está em [docs/SONOCRAFT-SOURCES.md](docs/SONOCRAFT-SOURCES.md).

- Configure entrada e saída física em **Áudio e saídas → Escuta e saídas**.
- Aplicativos com áudio aparecem automaticamente na mesa. Use **Adicionar fonte** para abas ou adição manual. Cada canal tem volume, medidor, Enviar e Silenciar.
- Ative **Enviar** somente nas fontes que deseja compartilhar com os conectados. O volume original do programa não muda.
- Uma entrada completa do sistema por cabo virtual fica fora do envio enquanto o modo de fontes individuais está ativo. Para voltar, escolha **Usar entrada completa**.
- Use a mesma versão nos dispositivos conectados.

## Novidades desta prévia

- Até 32 fontes simultâneas, com controles independentes e nomes editáveis na mesa principal.
- Seletores curtos de aplicativo/aba substituem a antiga aba Fontes nos Ajustes.
- Aplicativos detectados automaticamente, com volume preservado nas pausas e captura iniciada somente ao enviar. Bloqueio de aplicativo repetido e de navegador inteiro junto com abas; modos completo/individual exclusivos para evitar sobreposição.
- Canais e volumes salvos. Ao reabrir, as capturas e os envios ficam desligados.
- Saídas virtuais conhecidas continuam bloqueadas, inclusive quando restauradas por configurações antigas.

## Compilar

Requisitos: CMake 3.x >=3.15; Xcode no macOS; Visual Studio com C++ no Windows.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUniversalBinary=OFF -DSONOCRAFT_BUILD_TESTS=ON
cmake --build build --target SonoBus_Standalone SonoCraftAudioRegression --config Release -j 4
ctest --test-dir build -C Release --output-on-failure
```

O nome interno do alvo é `SonoBus_Standalone`; o produto se chama SonoCraft.
No Windows, use o gerador Visual Studio instalado. A compilação e o teste de áudio UDP local foram executados com sucesso no Windows x64 e no macOS. A validação entre dois computadores e com dispositivos físicos ainda é necessária.

## Limites

Até 32 fontes por dispositivo. A detecção automática usa sessões de áudio no Windows e requer macOS 14.2+; versões anteriores mantêm a adição manual. Captura nativa de aplicativos requer sistema compatível e permissão. Abas usam Chrome/Edge com página auxiliar aberta; adicione cada aba uma única vez, pois o navegador não fornece identificação estável para reconhecer repetições. No destinatário, as fontes chegam como uma mistura estéreo; os controles individuais ficam na origem.

Os testes com UDP local e fontes sintéticas não substituem o teste com dispositivos físicos e aplicativos de destino. Captura nativa e uso prolongado ainda precisam de validação real. Grupo/senha depende do serviço de conexão existente; não há novo servidor público ou relay SonoCraft.

Mac universal com assinatura ad hoc, sem notarização. Windows x64 portátil com WASAPI, sem ASIO e sem assinatura comercial. O cabo virtual é instalado separadamente.

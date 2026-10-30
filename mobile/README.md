# SonoCraft mobile

A prévia atual usa **net.sonocraft.audio** no Android e no iOS. O projeto CMake da raiz é a fonte de verdade: compila os mesmos arquivos de `Source/` e recursos do desktop, incluindo as alterações entre `addb33eb825fa67a674d2d6a8e1b19b0637e4faf` e `74c5e099` (prévia 14).

## Compilar localmente

Android: abra `mobile/Builds/Android` no Android Studio ou execute `bash scripts/build-mobile.sh android` na raiz. Configure `ANDROID_HOME` e `JAVA_HOME`. Requer SDK 36, NDK 28.2.13676358 e CMake 3.22.1. O Gradle Wrapper instala Gradle 8.14. Para o emulador neste Mac Apple Silicon, use `SONOCRAFT_ABIS=arm64-v8a` antes do comando; o padrão inclui também x86_64.

O APK otimizado de prévia fica em `mobile/Builds/Android/app/build/outputs/apk/preview/app-preview.apk`. Usa código nativo Release, sem depuração, e é assinado com a mesma chave local de desenvolvimento para atualizar a prévia instalada. Para depuração explícita, use `:app:assembleDebug` no Gradle. Instale com `adb install -r CAMINHO_DO_APK`. As versões release são **não assinadas**, sem usar credenciais do projeto original.

Simulator iOS: instale o runtime pelo Xcode e execute `bash scripts/build-mobile.sh ios ../work/sonocraft-ios-xcode`. O comando gera um projeto Xcode compartilhando o mesmo código e compila o target `SonoBus_Standalone` (nome interno legado; nome visível SonoCraft). `CMAKE` permite informar o caminho do executável CMake. Abra o projeto gerado, selecione um iPhone/iPad simulado e execute. Não requer conta paga para Simulator. Instalação em aparelho físico e distribuição precisam de configuração de assinatura própria no Xcode.

Não regenere o Android com o Projucer. `SonoBusMobile.jucer`, `JuceLibraryCode`, `Builds/iOS` e os scripts antigos dentro de `Builds/Android` são preservados apenas como **legado do SonoBus**, não constituem a versão mobile atual. Use o script da raiz acima. Não use certificados, identificadores ou scripts de publicação antigos.

## Escopo da prévia

- Código compartilhado para conexão/reconexão, envio e recepção de áudio, mute, níveis, chat, gravação e metrônomo.
- Interface própria para toque: canais em cartões, volume horizontal, navegação compacta, pads/conversa em painéis com margens e editor de reprodução com seletores largos. O desktop mantém sua mesa vertical.
- Sem saída dedicada para misturar áudio em microfone virtual.
- Entrada, saída, seleção de arquivos e permissões seguem as APIs de cada sistema.
- Captura **local** por aplicativo/aba ainda não foi implementada em Android/iOS. O mobile recebe essas fontes enviadas pelo desktop; o diálogo informa a limitação e não oferece controles de captura sem suporte.
- A execução em simuladores não certifica latência, Bluetooth, interrupções de chamadas ou áudio em segundo plano em celulares reais. Esses testes precisam de aparelhos.

O domínio informado foi usado como base do identificador reverso. Não foram publicados aplicativos, alterado DNS ou configuradas contas nas lojas.

## Validação local (29/10/2026)

Android arm64: APK instalado e aplicativo iniciado no emulador Android 16; mesa e participantes renderizados. Compilação iOS Simulator arm64 passou, mas o acesso ao CoreSimulator para empacotar recursos e instalar o app está bloqueado na sessão automatizada. Portanto o pacote manual em `work/sonocraft-ios-local` não deve ser tratado como instalador iOS final; use o fluxo Xcode acima para concluir os recursos e a execução.

A suíte `SonoCraftAudioRegression` do desktop passou após as alterações compartilhadas. Não houve publicação nas lojas ou GitHub.

## Prévia Android 2 (30/10/2026)

- Seletores de dispositivos em largura completa, rótulos acima e teste de saída em linha própria.
- Fechamento dos painéis de ajustes, conversa, conexão e pads com o mesmo ícone vetorial discreto à direita.
- Medidores atualizam somente sua região; a mesa deixa de atualizar enquanto estiver encoberta por painéis ou durante a edição de texto no mobile.
- Explicação das limitações de captura local disponível em Mais → Fontes no celular.
- APK ARM64 `dist/mobile/SonoCraft-android-mobile-preview.2.apk`, versão 108, assinado com a chave de desenvolvimento existente. Release nativo com `-O3`, `NDEBUG` e sem flag Android `DEBUGGABLE`.

Verificação: assemblePreview e lint vital passaram; assinatura APK validada; atualização sobre a prévia 1 e inicialização no emulador Android 16 concluídas sem erros AndroidRuntime no trecho de log inspecionado; regressão de áudio 1/1 passou. A automação não conseguiu operar os controles na janela destacada do emulador, portanto a conferência visual completa dos modais e a fluidez em aparelho físico ainda precisam de reteste. Não houve publicação.

## Prévia Android 3 (30/10/2026)

Corrige o preenchimento indevido do ícone de fechar: as diagonais usam somente contorno, com fundo discreto e área de toque preservada. Desenho conferido por renderização do próprio componente JUCE. APK ARM64 `dist/mobile/SonoCraft-android-mobile-preview.3.apk`, versão 109.

## Prévia Android 4 — painéis e canais (30/10/2026)

Canais de entrada/saída mantêm linhas de toque confortáveis, com fonte e checkbox no tamanho dos demais campos. Painéis usam a mesma superfície: centralizada no desktop, com margens no mobile, título e X sem moldura permanente no canto direito. Configurações, conexão, conversa, pads, fontes, efeitos, roteamento e editores usam esse padrão. Menus de escolha e seletores de arquivo do sistema mantêm seus comportamentos próprios.

A mudança preserva a propriedade dos componentes e o salvamento dos pads ao fechar o editor. `SonoCraftDialogRegression` cobre fechamento, reabertura com texto preservado, modais empilhados, redimensionamento e descarte do painel quando separado da janela. Prévia local; não publicada.

Validação da prévia 4: Android ARM64 (assemblePreview e assinatura), macOS Standalone e as duas suítes de regressão passaram. A aparência dos canais e do cabeçalho foi conferida por renderização dos componentes JUCE; navegação e toque no aparelho físico ainda precisam de validação. APK: `dist/mobile/SonoCraft-android-mobile-preview.4.apk` (versão 110).

# SonoCraft 0.4 — prévia experimental

Derivado do SonoBus 1.7.2, com avisos de autoria, GPLv3 e exceção de distribuição em lojas preservados.

## Começar

O áudio completo pode chegar à escuta enquanto apenas microfone, pads e fontes autorizadas seguem para outros aplicativos. A interface usa os mesmos caminhos em todos os sistemas: **Entrada**, **Escuta**, **Envio aos conectados** e **Saída para aplicativos**.

Leia [o guia de fontes e dispositivos](COMO-USAR-FONTES.md) incluído no pacote. No repositório, ele está em [docs/SONOCRAFT-SOURCES.md](docs/SONOCRAFT-SOURCES.md).

- Configure entrada, saída principal e escuta em **Áudio e saídas → Escuta e saídas**.
- Em **Fontes**, escolha uma aba ou aplicativo e marque seus destinos.
- Para ouvir tudo e compartilhar apenas a seleção, deixe **Enviar também todo o áudio recebido** desligado.
- Use a mesma versão em todos os dispositivos conectados.

## Novidades desta prévia

- Controles genéricos de escuta e saída para aplicativos, sem associar funções a um sistema ou finalidade.
- Fontes organizadas por seleção local, destinos e seleções recebidas.
- Seleção independente de uma aba do Chrome/Edge ou aplicativo inteiro, além da entrada completa e dos pads.
- Indicação corrigida de entrada/saída desativada na tela de dispositivos.
- Eventos de conexões temporárias ou antigas não desligam a recepção ativa.
- Guia atualizado incluído nos pacotes.

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

Uma seleção de aba/aplicativo por dispositivo de cada vez. Captura nativa de aplicativos requer sistema compatível e permissão. A captura não começa automaticamente.

Os testes com UDP local e fontes sintéticas não substituem o teste com dispositivos físicos e aplicativos de destino. Captura nativa e uso prolongado ainda precisam de validação real. Grupo/senha depende do serviço de conexão existente; não há novo servidor público ou relay SonoCraft.

Mac universal com assinatura ad hoc, sem notarização. Windows x64 portátil com WASAPI, sem ASIO e sem assinatura comercial. O cabo virtual é instalado separadamente.

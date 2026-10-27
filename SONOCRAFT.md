# SonoCraft 0.1.0 — prévia de desenvolvimento

Derivado de SonoBus 1.7.2, commit 35f1062dab196b9838a4bb529c4bf6592b7f5987.
Mantém os avisos de autoria e a licença GPLv3 com a exceção de distribuição em lojas.

Foco inicial: Mac enviando áudio para Windows na mesma rede, via IP.

## Mudanças

- Nome, identificadores de plugins e pasta de configurações próprios.
- Proteção contra informação de tempo ausente no processamento de áudio.
- Buffer remoto dimensionado para o bloco efetivamente recebido, inclusive quando o tamanho muda antes de estabilizar.
- Convite de conexão não confirma antecipadamente que o participante respondeu.
- Desconexão manual cancela a recuperação automática pendente.
- Falha imediata de reconexão libera o estado pendente para permitir nova tentativa.
- Conexão adiada usa referência segura à interface e uma cópia dos dados da tentativa.
- Atualizador do SonoBus desativado nesta derivação.
- Ajuste de captura de janela do JUCE para compilação com SDK macOS 15 ou superior; captura somente a área interna da janela nesse caminho.

Os nomes internos das classes e alvos de compilação continuam os originais. O ícone e parte dos textos herdados ainda são do SonoBus. Não há novo serviço público SonoCraft nem relay de áudio.

## Compilar

Requisitos: CMake 3.x >=3.15; Xcode no macOS; Visual Studio com C++ no Windows.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUniversalBinary=OFF -DSONOCRAFT_BUILD_TESTS=ON
cmake --build build --target SonoBus_Standalone SonoCraftAudioRegression --config Release -j 4
ctest --test-dir build -C Release --output-on-failure
```

O nome interno do alvo é `SonoBus_Standalone`; o produto se chama SonoCraft.
No Windows, use o gerador Visual Studio instalado. A compilação Windows ainda precisa ser executada e validada nesse sistema.

## Primeiro ensaio Mac → Windows

1. Abra apenas uma instância por computador durante o primeiro teste.
2. No Windows, copie o endereço completo **IP:porta** exibido na conexão direta. Não presuma que a porta permaneceu em 11000.
3. No Mac, selecione a entrada de áudio correta, confirme atividade no medidor de entrada e informe o endereço completo do Windows.
4. No Windows, confirme a saída correta e o recebimento habilitado.
5. Verifique se o aplicativo está autorizado na rede privada pelo Firewall do Windows. Não é necessário desligar o firewall.
6. Compare cabo e Wi-Fi, mantendo as demais opções iguais. Registre codec, buffer, taxa de amostragem e contadores de perda durante as interrupções.

A configuração do SonoCraft é independente. É necessário selecionar os dispositivos de áudio novamente. Compatibilidade com SonoBus é esperada pelo protocolo preservado, mas precisa ser confirmada entre os dois aparelhos.

## Limites desta prévia

Uma compilação e um teste local não demonstram que as falhas Mac/Windows foram eliminadas. Grupo/senha depende do servidor de conexão e da troca de eventos; ainda requer reprodução com o servidor e a rede usados no problema. Não houve alteração do algoritmo de senha nem comprovação de defeito nele.

O motor ainda contém alocações e bloqueios no processamento de áudio, e o ajuste automático de buffer precisa ser medido sob perdas e variação de rede. Não foram prometidos áudio sem interrupções, baixa latência específica ou correção de bloqueios externos ao aplicativo.

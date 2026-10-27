Prévia SonoCraft 0.2: tema claro e minimalista, novo ícone e gerenciamento mais acessível das conexões.

Novidades:
- **Remover** diretamente no cabeçalho de cada participante.
- Identificação estável da conexão para impedir que uma ação antiga remova outro cliente depois de reordenar a lista.
- Correção do menu de participantes e atualização da lista após a remoção.
- Endereço IP:porta visível e contraste ajustado para o tema claro.

A remoção é manual e afeta somente suas conexões. A reconexão ainda não substitui automaticamente uma entrada antiga.

Downloads:
- **SonoCraft-mac-universal.zip**: aplicativo macOS, Apple Silicon e Intel, com assinatura local ad hoc; sem notarização.
- **SonoCraft-windows-x64.zip**: executável portátil Windows x64, com WASAPI; sem ASIO e sem assinatura comercial.

Inclui ajustes no dimensionamento de buffers, tratamento de tempo ausente, confirmação da conexão por IP e reconexão. Os pacotes são publicados após compilar e passar o teste de áudio UDP local nos dois sistemas.

**Validação ainda necessária:** abertura e uso com dispositivos reais, Mac → Windows na mesma rede, grupo/senha e estabilidade prolongada. Esses testes automáticos não comprovam que todas as falhas relatadas foram resolvidas.

O aplicativo usa configurações separadas das do SonoBus. Ao conectar por IP, informe o endereço completo IP:porta exibido no outro computador.

Licença GPLv3 e exceção de lojas preservadas. Consulte SONOCRAFT.md e os avisos das dependências no código-fonte deste tag.

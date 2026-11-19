# Testes do OnyxOS

Este documento registra como validar o estado atual do OnyxOS e quais testes ainda precisam ser executados.

> **Status atual:** nenhum teste de compilação, instalação no Xbox ou inicialização de sistema convidado foi confirmado nesta documentação. Marque um item como concluído somente depois de executar o teste e registrar o resultado.

## 1. Estado do projeto

O OnyxOS está em fase inicial de desenvolvimento. O pacote APPX foi estruturado para conter sete entradas de aplicativo em um único instalador:

1. OnyxOS Launcher
2. Windows 10
3. Windows 11
4. Debian
5. Ubuntu
6. Arch Linux
7. macOS (experimental)

No estado atual, as sete entradas usam o mesmo executável de host provisório. Isso valida apenas a estrutura planejada; não significa que os sete ambientes estejam implementados ou inicializem.

## 2. Verificação do repositório

Antes de testar:

- [ ] Confirmar que o branch usado é `main`.
- [ ] Conferir `Package.appxmanifest` e verificar as sete entradas `Application`.
- [ ] Conferir `CMakeLists.txt` e os alvos de compilação.
- [ ] Conferir `.github/workflows/build-uwp.yml`.
- [ ] Conferir `config/os-profiles.json`.
- [ ] Conferir se os arquivos HLSL e GLSL estão presentes em `graphics/shaders/`.

## 3. Teste de compilação

### GitHub Actions

1. Abra a aba **Actions** do repositório.
2. Execute o workflow de build UWP.
3. Abra a execução e confira os logs de cada etapa.
4. Verifique se o workflow terminou com sucesso.
5. Baixe o artefato `OnyxOS-Xbox-UWP-x64` e confirme a presença de `OnyxOS-x64.appx`.

Resultado:

- [ ] Workflow executou com sucesso.
- [ ] Os sete executáveis foram gerados.
- [ ] O APPX foi empacotado.
- [ ] O artefato foi publicado.

Registre aqui o link da execução e eventuais erros:

```text
Execução do GitHub Actions:
Resultado:
Erros:
```

### Build local no Windows

Siga [BUILDING.md](../BUILDING.md) para preparar Visual Studio, CMake e Windows SDK. Não considere a compilação validada apenas porque a configuração do CMake terminou; confirme também a compilação, o empacotamento e a assinatura.

## 4. Teste de instalação

Siga [INSTALLING.md](../INSTALLING.md).

### Windows

- [ ] Certificado de desenvolvimento corresponde ao Publisher `CN=Bluey683`.
- [ ] Certificado foi confiado no ambiente de teste.
- [ ] APPX foi instalado sem erro.
- [ ] Entrada OnyxOS aparece no menu Iniciar.
- [ ] Aplicativo abre e permanece em execução.

### Xbox — Developer Mode

- [ ] Console iniciou em Developer Mode.
- [ ] Device Portal está habilitado.
- [ ] APPX foi enviado pelo Device Portal.
- [ ] Instalação terminou sem erro.
- [ ] OnyxOS aparece na lista de aplicativos.
- [ ] Aplicativo abre no console.

Registre dispositivo, versão do sistema, arquitetura, resultado e mensagens de erro:

```text
Dispositivo:
Versão do sistema:
Versão do pacote:
Resultado da instalação:
Resultado da inicialização:
Logs:
```

## 5. Testes funcionais planejados

Os itens abaixo dependem de implementação. Não marque como aprovados enquanto os recursos não existirem e forem testados.

- [ ] Launcher apresenta os perfis de sistemas operacionais.
- [ ] Cada entrada abre o perfil correto.
- [ ] Seletor de ISO acessa arquivos autorizados pelo usuário.
- [ ] Arquivos ISO são reconhecidos e validados.
- [ ] Discos virtuais podem ser criados e persistidos.
- [ ] QEMU inicializa e encerra uma VM corretamente.
- [ ] Tela do convidado é apresentada pelo renderer.
- [ ] Teclado e mouse são encaminhados ao convidado.
- [ ] Controle Xbox é reconhecido e encaminhado.
- [ ] Áudio do convidado é reproduzido pelo backend XAudio2.
- [ ] Rede do convidado funciona.
- [ ] Logs e mensagens de erro são gravados em LocalState/Logs.

## 6. Testes de shaders

Arquivos de origem:

- `graphics/shaders/onyxos.hlsl`
- `graphics/shaders/onyxos.vert`
- `graphics/shaders/onyxos.frag`

- [ ] HLSL compila com o compilador/SDK de destino.
- [ ] GLSL compila em uma implementação OpenGL compatível.
- [ ] Textura de framebuffer é apresentada corretamente.
- [ ] Redimensionamento e proporção da imagem são tratados.
- [ ] Erros de compilação e carregamento são reportados.

A presença dos arquivos de shader não comprova que estejam compilados ou conectados ao QEMU.

## 7. Compatibilidade dos sistemas convidados

Execute estes testes somente depois da integração funcional do QEMU. Use imagens ISO obtidas legalmente e registre versão, configuração da VM, memória atribuída e resultado.

| Perfil | Estado |
| --- | --- |
| Windows 10 | Não testado |
| Windows 11 | Não testado |
| Debian | Não testado |
| Ubuntu | Não testado |
| Arch Linux | Não testado |
| macOS | Experimental; não testado |

## 8. Registro de falhas

Para cada falha, registre:

- Etapa em que ocorreu.
- Dispositivo e versão do sistema.
- Commit ou execução do workflow.
- Mensagem de erro completa.
- Log relevante em `LocalState/Logs`, se disponível.
- Passos para reproduzir.

```text
Título:
Etapa:
Dispositivo / sistema:
Commit:
Como reproduzir:
Resultado esperado:
Resultado obtido:
Mensagem / log:
```

## 9. Critério de aprovação

Uma etapa só deve ser considerada aprovada quando o teste correspondente tiver sido executado no ambiente indicado e o resultado estiver registrado. Build bem-sucedido não comprova instalação; instalação bem-sucedida não comprova inicialização de uma VM; e inicializar uma VM não comprova funcionamento de áudio, entrada, rede ou aceleração gráfica.

Consulte também:

- [README.md](../README.md)
- [BUILDING.md](../BUILDING.md)
- [INSTALLING.md](../INSTALLING.md)
- [Estrutura do APPX](../docs/APPX-STRUCTURE.md)
- [Perfis de sistemas operacionais](../docs/OS-PROFILES.md)

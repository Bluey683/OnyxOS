# Dependências QEMU para OnyxOS

O OnyxOS usa o host UWP do projeto [QEMU-UWP-Host](https://github.com/rodrigoandrigo/QEMU-UWP-Host).

## Arquivos esperados

Coloque o conjunto de binários compatíveis (x64/UWP) nestas pastas antes de compilar:

- `Dependencies/Qemu/qemu-system-x86_64.dll`
- `Dependencies/Runtime/*.dll` — dependências runtime exigidas por essa compilação do QEMU
- `Dependencies/qemu/**` — firmware/BIOS e recursos exigidos pela configuração escolhida

Obtenha os binários de uma mesma versão/build. Não misture DLLs de builds diferentes. Consulte o README e os arquivos de dependências do projeto upstream para a lista exata de runtime e firmware.

## Integração atual

`QemuHostAdapter` usa `LoadPackagedLibrary` para carregar a DLL do pacote e valida os exports da API host 1.x. A validação não inicia a VM. Para iniciar o loop QEMU com segurança ainda é necessário implementar os protótipos exatos de callbacks e lifecycle da mesma revisão da DLL, além do renderizador, áudio e entrada.

**Não coloque uma DLL aleatória nem renomeie uma DLL desktop para esse caminho.** O binário precisa ser compilado para UWP/AppContainer e x64, com a API host esperada.

O binário não é versionado neste repositório. O Build.ps1 o inclui automaticamente quando estiver presente.

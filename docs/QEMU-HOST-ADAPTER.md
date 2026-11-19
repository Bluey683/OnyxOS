# QEMU Host Adapter

O OnyxOS agora tem uma camada nativa para carregar e fazer preflight da DLL QEMU UWP empacotada.

## DLL e ABI

O alvo esperado é `Dependencies/Qemu/qemu-system-x86_64.dll`, da build UWP x64 do [QEMU-UWP-Host](https://github.com/rodrigoandrigo/QEMU-UWP-Host). A API host documentada pelo projeto upstream usa ABI major 1. A integração verifica os exports de inicialização, loop, controle, entrada e callback de vídeo; se existir, consulta `qemu_host_get_api_version`.

## O que já está implementado

- Carregamento por `LoadPackagedLibrary` (DLL dentro do pacote UWP).
- Verificação dos exports obrigatórios e compatibilidade da API major 1.
- Mensagem de erro e lista de exports ausentes.
- Build.ps1 copia a DLL e as dependências runtime/firmware quando as pastas estão presentes.

## O que ainda falta

Esta camada **não inicia uma VM ainda**. A API upstream declara callbacks com layouts específicos; não é seguro inventar protótipos C++ e chamar funções sem copiar os headers da mesma revisão exata da DLL. Ainda faltam ligar init/start/main-loop/shutdown, callbacks de vídeo, XAudio2, teclado, mouse e controle ao Launcher.

## Colocação dos binários

Veja [Dependencies/Qemu/README.md](../Dependencies/Qemu/README.md). Os binários não são incluídos no Git por padrão; devem ser obtidos como um conjunto UWP x64 compatível e conferidos antes da compilação.

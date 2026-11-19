# Dependências QEMU para OnyxOS

O host usa a build x64/UWP do [QEMU-UWP-Host](https://github.com/rodrigoandrigo/QEMU-UWP-Host), branch `x86-64`.

## DLL principal

`Dependencies/Qemu/qemu-system-x86_64.dll`

## Runtime

A pasta upstream [Dependencies/Runtime](https://github.com/rodrigoandrigo/QEMU-UWP-Host/tree/x86-64/Dependencies/Runtime) contém as DLLs auxiliares necessárias ao host, incluindo SDL2, GLib, Pixman, zlib, libslirp e outras bibliotecas do conjunto MSYS2/UCRT.

Não copie apenas SDL2: mantenha o conjunto completo da mesma branch/build. O script `scripts/Fetch-QemuRuntime.ps1` consulta a listagem oficial do GitHub e baixa automaticamente a DLL principal e todos os arquivos da pasta Runtime para as pastas correspondentes do OnyxOS.

Execute manualmente:

```powershell
.\scripts\Fetch-QemuRuntime.ps1
```

O `Build.ps1` executa esse script automaticamente antes da compilação. Use `-SkipQemuDownload` somente se os binários já estiverem presentes.

## Firmware

Os arquivos de firmware/BIOS ficam na pasta `qemu` do repositório upstream, fora de `Dependencies/Runtime`. Eles são recursos separados das DLLs de runtime. O Build.ps1 copia `Dependencies/qemu` se essa pasta local existir; o download automático do firmware ainda não está habilitado.

## Compatibilidade e licença

Use os binários da mesma revisão e arquitetura x64/UWP. Consulte o README, os arquivos de licença e as instruções do projeto upstream antes de redistribuir os binários. Não renomeie DLLs desktop para fazê-las parecer UWP.

O carregador `QemuHostAdapter` faz apenas o carregamento e a verificação preliminar dos exports. A inicialização real do QEMU e os callbacks ainda precisam ser ligados usando os headers exatos dessa revisão.

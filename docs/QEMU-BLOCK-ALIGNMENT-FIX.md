# Correção QEMU UWP: alinhamento do disco

O crash `block.c:1725` ocorre porque `block/file-win32.c:raw_probe_alignment()`
atribuía `bytesPerSector` a `request_alignment` mesmo quando
`GetDiskFreeSpace()` falhava. Em UWP/Xbox, essa consulta pode falhar no sandbox;
nesse caso o valor de saída não é válido (frequentemente zero), e QEMU aborta
ao validar que o alinhamento é uma potência de dois.

A correção mantém o tamanho de setor retornado somente quando a chamada tem
sucesso e o valor é uma potência de dois. Caso contrário, usa
`BDRV_SECTOR_SIZE` (512 bytes), preservando o alinhamento padrão seguro.

## Aplicar e recompilar a DLL

1. Obtenha o código-fonte QEMU 11.0.2.
2. Sobreponha nele os arquivos de port UWP indicados pelo projeto
   [Qemu-Dll-shadps4](https://github.com/rodrigoandrigo/Qemu-Dll-shadps4).
3. Execute em PowerShell:
   ```powershell
   .\scripts\Apply-QemuBlockAlignmentFix.ps1 -QemuSourceRoot C:\src\qemu-11.0.2
   ```
4. Compile a DLL no MSYS2/UCRT64 conforme as instruções do port UWP:
   ```sh
   mkdir -p build && cd build
   ../configure --target-list=x86_64-softmmu --disable-gtk --disable-werror --disable-docs --disable-libnfs --disable-qga-vss --disable-sdl --disable-sdl-image --disable-dsound --enable-uwp --enable-xaudio2 --audio-drv-list=xaudio2
   make -j2
   strip --strip-unneeded qemu-system-x86_64.dll
   ```
5. Substitua `Dependencies/Qemu/qemu-system-x86_64.dll` pela DLL recompilada,
   mantendo as DLLs auxiliares da mesma build. Recrie o APPX.

**Importante:** o script atual de download obtém uma DLL pré-compilada upstream.
A correção no arquivo-fonte só entra em execução depois de recompilar e empacotar
a DLL. Não basta recompilar somente o C++ do OnyxOS.

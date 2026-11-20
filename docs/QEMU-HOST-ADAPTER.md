# QEMU Host Adapter

O OnyxOS carrega a DLL QEMU UWP x64 empacotada e executa a VM dentro do processo UWP.

## Inicialização

- Carrega `qemu-system-x86_64.dll` com `LoadPackagedLibrary`.
- Confere os exports da API QEMU Host e a versão principal da ABI.
- Executa `qemu_host_init` e o ciclo `qemu_host_main_loop_step` em uma thread de trabalho.
- Aguarda até 45 segundos pelo primeiro framebuffer real antes de declarar a VM iniciada.
- Usa `-display host` para o backend de vídeo do QEMU UWP e envia os quadros ao renderer Direct3D 11/HLSL.
- Executa a inicialização fora da thread da interface para manter a tela responsiva.
- O argumento `-L qemu` aponta para o diretório de BIOS/firmware incluído no APPX.

## Arquivos do runtime

`scripts/Fetch-QemuRuntime.ps1` obtém a DLL, as dependências e a pasta de firmware `qemu` do projeto upstream. `Build.ps1` inclui esses arquivos no pacote e valida a presença de `qemu/bios-256k.bin`.

## Limitações

QEMU usa TCG para emular a CPU convidada. Direct3D 11/HLSL apresenta o framebuffer na GPU do Xbox; isso não é passthrough de GPU para o sistema convidado. A compilação e a validação do pacote não substituem o teste de boot no console.

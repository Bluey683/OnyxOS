# OnyxOS graphics shaders

OnyxOS includes equivalent starter shaders for two possible host-rendering paths:

- `shaders/onyxos.hlsl`: HLSL vertex and pixel shader entry points (`VSMain`, `PSMain`) for a Direct3D renderer.
- `shaders/onyxos.vert` and `shaders/onyxos.frag`: GLSL 3.30 vertex and fragment shaders for an OpenGL renderer.

Both paths sample a guest framebuffer texture and apply a tint. The host renderer must upload the QEMU framebuffer into a GPU texture, provide vertex data (position and UV), set the projection matrix and tint, and bind the texture/sampler.

## Important

HLSL and GLSL are alternative shader languages/backends; adding both source sets does not make Direct3D execute GLSL or OpenGL execute HLSL. The application must select and implement a renderer appropriate to the target platform. On Xbox UWP, Direct3D/HLSL is the intended initial path. GLSL is an optional path only if a supported OpenGL/EGL implementation is integrated.

These files are shader source scaffolding only. They are not yet compiled or connected to the QEMU video callback, and they do not provide guest GPU passthrough or 3D acceleration.

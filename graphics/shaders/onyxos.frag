#version 330 core

in vec2 vTexCoord;

uniform sampler2D uGuestFramebuffer;
uniform vec4 uTint;

out vec4 outColor;

void main()
{
    outColor = texture(uGuestFramebuffer, vTexCoord) * uTint;
}

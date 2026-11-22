// OnyxOS HLSL guest framebuffer shader.
// Vertex shader uses SV_VertexID to draw one full-screen triangle. No vertex
// buffer or XAML/SwapChainPanel dependency is required by the shader.

Texture2D GuestFramebuffer : register(t0);
SamplerState GuestSampler : register(s0);

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    VSOutput output;
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    output.Position = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.0f, 1.0f);
    output.TexCoord = uv;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    return GuestFramebuffer.Sample(GuestSampler, input.TexCoord);
}

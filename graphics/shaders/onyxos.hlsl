// OnyxOS HLSL framebuffer shader
// Compile as separate stages with entry points VSMain and PSMain.
// The host renderer supplies the texture and sampler bound to t0/s0.

cbuffer FrameConstants : register(b0)
{
    float4x4 Projection;
    float4 Tint;
};

Texture2D GuestFramebuffer : register(t0);
SamplerState GuestSampler : register(s0);

struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), Projection);
    output.TexCoord = input.TexCoord;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 color = GuestFramebuffer.Sample(GuestSampler, input.TexCoord);
    return color * Tint;
}

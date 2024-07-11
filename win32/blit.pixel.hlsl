
Texture2D tex;
SamplerState textureSampler;

float4 main(float2 uv : TexCoord) : SV_Target {
    const float states = 14.0;
    float4 textureSample = tex.Sample(textureSampler, uv).xxxx / states;
    float4 result = float4(textureSample);
    return result;
}

Texture2D tex;
SamplerState textureSampler;

float4 main(float2 uv : TexCoord) : SV_Target {
    float4 textureSample = tex.Sample(textureSampler, uv);
    float4 result = textureSample.xxxx;
    return result;
}
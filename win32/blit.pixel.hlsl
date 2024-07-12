
cbuffer Constants : register(b0)
{
    uint States;
    uint Threshold;
    uint Search;
    uint UseNeumannSearch;
};

Texture2D tex;
SamplerState textureSampler;

float4 RGBA(float R, float G, float B, float A) {
    return float4(
        R / 255.0,
        G / 255.0,
        B / 255.0,
        1.0
    );
}

float4 main(float2 uv : TexCoord) : SV_Target {
    float textureSample = floor(tex.Sample(textureSampler, uv).x);
    float4 result = 0.0;
#if 0
    if (textureSample < 0.01) {
        result = RGBA(214, 41, 41, 1);
    // } else if (textureSample < 1.01) {
    //     result = RGBA(54, 214, 227, 1);
    } else {
        result = RGBA(52, 46, 55, 1);
    }
#else
    result = textureSample / States;
#endif
    return result;
}

cbuffer Constants : register(b0)
{
    uint States;
    uint Threshold;
    uint Search;
    uint UseNeummanSearch;
    int MousePosX;
    int MousePosY;
};

RWTexture2D<float> writeTexture : register(u0);

float random_patterned(float2 n) {
    return frac(cos(dot(n, float2(4.6348199823, 5.19890287499) * 0.31901827394810948)) * 29.31881923968742);
}
float random(float2 n) {
    return frac(cos(dot(n, float2(6582.63481998232, 8891.198902874995) * 0.001901827394810948)) * 10892.31881923968742);
}

[numthreads(32,32,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    const float states = float(States);
    writeTexture[dispatchID.xy] = floor(random(dispatchID.xy + 1024.9823) * states);
    // writeTexture[dispatchID.xy] = 0.0;
}
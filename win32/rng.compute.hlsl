
RWTexture2D<float> writeTexture : register(u0);

float random_patterned(float2 n) {
    return frac(cos(dot(n, float2(4.6348199823, 5.19890287499) * 0.31901827394810948)) * 29.31881923968742);
}
float random(float2 n) {
    return frac(cos(dot(n, float2(43.6348199823, 38.19890287499) * 0.01901827394810948)) * 10892.31881923968742);
}

[numthreads(32,32,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    const float states = 14.0;
    writeTexture[dispatchID.xy] = floor(random_patterned(dispatchID.xy + 1024.9823) * states);
}
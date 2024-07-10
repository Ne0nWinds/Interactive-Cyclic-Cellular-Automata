
RWTexture2D<float> writeTexture : register(u0);

float random(float2 n) {
    return frac(cos(dot(n, float2(13875.6348199823, 11829.19890287499) * 0.1590182739)) * 81934.31923968742);
}

[numthreads(32,32,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    writeTexture[dispatchID.xy] = random(dispatchID.xy);
}
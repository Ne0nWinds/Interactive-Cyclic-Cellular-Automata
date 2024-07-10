
RWTexture2D<float> readTexture : register(u0);
RWTexture2D<float> writeTexture : register(u1);

[numthreads(32,32,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    writeTexture[dispatchID.xy] = (readTexture[dispatchID.xy] + 0.01) % 1.0;
}
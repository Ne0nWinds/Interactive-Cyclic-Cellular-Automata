
RWTexture2D<float> output : register(u0);

[numthreads(1,1,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    output[dispatchID.xy] = 0.75f;
}
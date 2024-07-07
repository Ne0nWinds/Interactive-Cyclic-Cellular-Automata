
RWTexture2D<float> output : register(u0);

[numthreads(32,32,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    output[dispatchID.xy] = dispatchID.x / 720.0;
}
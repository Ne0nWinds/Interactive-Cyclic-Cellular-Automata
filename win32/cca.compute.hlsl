
RWTexture2D<float> readTexture : register(u0);
RWTexture2D<float> writeTexture : register(u1);

[numthreads(32,32,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    const uint states = 14;
    const uint threshold = 1;
    const int search = 1;

    const uint current_state = floor(readTexture[dispatchID.xy]);
    const uint next_state = (current_state + 1) % states;
    uint search_results_found = 0;

    const bool neumann_search = true;

    if (neumann_search) {
        for (int y = -search; y <= search; ++y) {
            for (int x = -search; x <= search; ++x) {
                if (x == 0 && y == 0) continue;
                if (abs(x) + abs(y) > search) continue;

                float2 sampleLocation = dispatchID.xy + float2(x, y);
                if (uint(readTexture[sampleLocation]) == next_state) {
                    search_results_found += 1;
                }
            }
        }
    } else {
        for (int y = -search; y <= search; ++y) {
            for (int x = -search; x <= search; ++x) {
                if (x == 0 && y == 0) continue;

                float2 sampleLocation = dispatchID.xy + float2(x, y);
                if (uint(readTexture[sampleLocation]) == next_state) {
                    search_results_found += 1;
                }
            }
        }
    }

    if (search_results_found >= threshold) {
        writeTexture[dispatchID.xy] = float(next_state % states);
    } else {
        writeTexture[dispatchID.xy] = float(current_state);
    }
}
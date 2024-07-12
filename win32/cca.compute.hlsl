#include "cca_flags.h"

cbuffer Constants : register(b0)
{
    uint States;
    uint Threshold;
    uint Search;
    uint Flags;
    int MousePosX;
    int MousePosY;
};

#define CHECK_FLAG(Flag) ((Flags & Flag) != 0)

RWTexture2D<float> readTexture : register(u0);
RWTexture2D<float> writeTexture : register(u1);

[numthreads(32,32,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    const uint states = States;
    const uint threshold = Threshold;
    const int search = Search;

    const uint current_state = floor(readTexture[dispatchID.xy]);
    const uint next_state = (current_state + 1) % states;
    uint search_results_found = 0;

    const bool neumann_search = CHECK_FLAG(CCA_NEUMANN_SEARCH);

    if (!CHECK_FLAG(CCA_PAUSED)) {
        if (neumann_search) {
            for (int y = -search; y <= search; ++y) {
                int x_search = abs(abs(y) - search);
                // int x_search = abs(y - search);
                for (int x = -x_search; x <= x_search; ++x) {
                    if (x == 0 && y == 0) continue;
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
    }

    uint final_result = current_state;

    if (search_results_found >= threshold) {
        final_result += 1;
    }
    float2 Mouse = float2(MousePosX, MousePosY);
    if (abs(Mouse.x - dispatchID.x) + abs(Mouse.y - dispatchID.y) < 75.0) {
        // final_result += 1;
        final_result = 0;
    }

    writeTexture[dispatchID.xy] = float(final_result % States);
}
#include "cca_flags.h"

cbuffer Constants : register(b0)
{
    uint States;
    uint Threshold;
    uint Search;
    uint Flags;
    int MousePosX;
    int MousePosY;
    float TimeElapsed;
};

#define CHECK_FLAG(Flag) ((Flags & Flag) != 0)

RWTexture2D<float> readTexture : register(u0);
RWTexture2D<float> writeTexture : register(u1);

float random(float n) {
    return frac(cos(n * 3816.51901827394810948) * 10892.31881923968742);
}

[numthreads(32,32,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    const uint states = States;
    const uint threshold = Threshold;
    const int search = Search;

    const uint current_state = floor(readTexture[dispatchID.xy]);
    const uint next_state = (current_state + 1) % states;
    uint search_results_found = 0;

    const bool neumann_search = CHECK_FLAG(CCA_NEUMANN_SEARCH);
    const bool random_search = CHECK_FLAG(CCA_RANDOM_SEARCH);

    if (!CHECK_FLAG(CCA_PAUSED)) {

        if (random_search) {
            int range = search * 2;
            for (int i = 0; i < search; ++i) {
                int x = int(round((random(dispatchID.x + TimeElapsed + i * 108.81393832) - 0.5) * range));
                int y = int(round((random(dispatchID.y + TimeElapsed + i * 29.7781982115) - 0.5) * range));
                float2 sampleLocation = dispatchID.xy + float2(x, y);
                if (uint(readTexture[sampleLocation]) == next_state) {
                    search_results_found += 1;
                }
            }
        } else if (neumann_search) {
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
    if (distance(Mouse, dispatchID.xy) < 75.0) {
    // if (abs(Mouse.x - dispatchID.x) + abs(Mouse.y - dispatchID.y) < 75.0) {
        // final_result += 1;
        final_result = 0;
    }

    writeTexture[dispatchID.xy] = float(final_result % States);
}
#pragma once
#include "base.h"

static inline u32 PopCount32(u32 Value);
static inline u64 PopCount64(u32 Value);

static inline u32 RoundUp32(u32 Value, u32 PowerOf2) {
	Assert(PopCount32(PowerOf2) == 1);
	u32 PW2MinusOne = PowerOf2 - 1;
	Value += PW2MinusOne;
	Value &= ~PW2MinusOne;
	return Value;
}

static inline u64 RoundUp64(u64 Value, u64 PowerOf2) {
	Assert(PopCount64(PowerOf2) == 1);
	u64 PW2MinusOne = PowerOf2 - 1;
	Value += PW2MinusOne;
	Value &= ~PW2MinusOne;
	return Value;
}

#if defined(CPU_X64)

#include <immintrin.h>

static inline u32 PopCount32(u32 Value) {
	return _mm_popcnt_u32(Value);
}
static inline u64 PopCount64(u32 Value) {
	return _mm_popcnt_u64(Value);
}

#elif defined(CPU_WASM)

#else
	#error "math.h not supported on this platform
#endif
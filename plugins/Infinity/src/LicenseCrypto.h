#pragma once
// Monocypher 4.0.2, pinned upstream, dual BSD-2-Clause/CC0-1.0.
// One translation unit per plugin/test in this project. The vendor supports C++ namespace wrapping.
#define MONOCYPHER_CPP_NAMESPACE libertycrypto
#include "vendor/monocypher.c"
#undef MONOCYPHER_CPP_NAMESPACE
#undef FOR_T
#undef FOR
#undef COPY
#undef ZERO
#undef WIPE_CTX
#undef WIPE_BUFFER
#undef MIN
#undef MAX
#undef QUARTERROUND
#undef BLAKE2_G
#undef BLAKE2_ROUND
#undef LSB
#undef G
#undef ROUND
#undef FE_CARRY
#undef P_W_WIDTH
#undef B_W_WIDTH
#undef P_W_SIZE

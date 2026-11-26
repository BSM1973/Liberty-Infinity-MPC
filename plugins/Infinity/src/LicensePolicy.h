#pragma once
// Development policy ONLY. Production must provision a public key and disable DEMO.
// Never store a private seed in this repository or in a distributed plugin.
constexpr bool allowUnsignedDemo=true;
constexpr std::array<uint8_t,32> productionPublicKey{};

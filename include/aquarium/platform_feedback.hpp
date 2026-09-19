#pragma once
#include <optional>
namespace aq {
// Unsupported devices return no preference and ignore haptics.
std::optional<bool> systemReducedMotion();
void playHaptic();
}

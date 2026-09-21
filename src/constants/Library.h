#pragma once
#include "Design.h"
#include <array>

namespace auralis::library
{
inline constexpr std::array<const char*, 3> categoryNames{"Instruments", "Sounds", "Effects"};
inline constexpr std::array<const char*, 3> categoryKinds{"Instrument", "Sound", "Effect"};
inline constexpr std::array<std::uint32_t, 3> categoryInks{design::colour::violet, design::colour::amber, design::colour::mint};
}

#pragma once
#include "Design.h"
#include <array>

namespace auralis::library
{
inline constexpr std::array<const char*, 4> categoryNames{"Instruments", "Sounds", "Effects","Plug-Ins"};
inline constexpr std::array<const char*, 4> categoryKinds{"Instrument", "Sound", "Effect","Plugin"};
inline constexpr std::array<std::uint32_t, 4> categoryInks{design::colour::violet, design::colour::amber, design::colour::mint,design::colour::blue};
}

#ifndef AQUILA_COLOR_H
#define AQUILA_COLOR_H

#include "Aquila/Foundation/Math/MathTypes.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Foundation::Color {

// RGBA clear colors
namespace RGBA {
constexpr vec4 Black     = { 0.0F, 0.0F, 0.0F, 1.0F };
constexpr vec4 White     = { 1.0F, 1.0F, 1.0F, 1.0F };
constexpr vec4 Red       = { 1.0F, 0.0F, 0.0F, 1.0F };
constexpr vec4 Green     = { 0.0F, 1.0F, 0.0F, 1.0F };
constexpr vec4 Blue      = { 0.0F, 0.0F, 1.0F, 1.0F };
constexpr vec4 Yellow    = { 1.0F, 1.0F, 0.0F, 1.0F };
constexpr vec4 Cyan      = { 0.0F, 1.0F, 1.0F, 1.0F };
constexpr vec4 Magenta   = { 1.0F, 0.0F, 1.0F, 1.0F };
constexpr vec4 Orange    = { 1.0F, 0.5F, 0.0F, 1.0F };
constexpr vec4 Gray      = { 0.5F, 0.5F, 0.5F, 1.0F };
constexpr vec4 LightGray = { 0.75F, 0.75F, 0.75F, 1.0F };
constexpr vec4 DarkGray  = { 0.25F, 0.25F, 0.25F, 1.0F };
} // namespace RGBA

// RGB colors
constexpr vec3 Black_v = { 0.0F, 0.0F, 0.0F };
constexpr vec3 White_v = { 1.0F, 1.0F, 1.0F };
constexpr vec3 Red_v = { 1.0F, 0.0F, 0.0F };
constexpr vec3 Green_v = { 0.0F, 1.0F, 0.0F };
constexpr vec3 Blue_v = { 0.0F, 0.0F, 1.0F };
constexpr vec3 Yellow_v = { 1.0F, 1.0F, 0.0F };
constexpr vec3 Cyan_v = { 0.0F, 1.0F, 1.0F };
constexpr vec3 Magenta_v = { 1.0F, 0.0F, 1.0F };
constexpr vec3 Orange_v = { 1.0F, 0.5F, 0.0F };
constexpr vec3 Gray_v = { 0.5F, 0.5F, 0.5F };
constexpr vec3 LightGray_v = { 0.75F, 0.75F, 0.75F };
constexpr vec3 DarkGray_v = { 0.25F, 0.25F, 0.25F };

// Console colors
constexpr const char *Reset = "\033[0m";
constexpr const char *Bold = "\033[1m";
constexpr const char *Dim = "\033[2m";

constexpr const char *Black = "\033[30m";
constexpr const char *Red = "\033[31m";
constexpr const char *Green = "\033[32m";
constexpr const char *Yellow = "\033[33m";
constexpr const char *Blue = "\033[34m";
constexpr const char *Magenta = "\033[35m";
constexpr const char *Cyan = "\033[36m";
constexpr const char *White = "\033[37m";

constexpr const char *BrightBlack = "\033[90m";
constexpr const char *BrightRed = "\033[91m";
constexpr const char *BrightGreen = "\033[92m";
constexpr const char *BrightYellow = "\033[93m";
constexpr const char *BrightBlue = "\033[94m";
constexpr const char *BrightMagenta = "\033[95m";
constexpr const char *BrightCyan = "\033[96m";
constexpr const char *BrightWhite = "\033[97m";

} // namespace Aquila::Foundation::Color

#endif

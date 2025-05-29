// dear imgui, v1.91.9b
// (drawing and font code)

/*

Index of this file:

// [SECTION] STB libraries implementation
// [SECTION] Style functions
// [SECTION] ImDrawList
// [SECTION] ImTriangulator, ImDrawList concave polygon fill
// [SECTION] ImDrawListSplitter
// [SECTION] ImDrawData
// [SECTION] Helpers ShadeVertsXXX functions
// [SECTION] ImFontConfig
// [SECTION] ImFontAtlas
// [SECTION] ImFontAtlas: glyph ranges helpers
// [SECTION] ImFontGlyphRangesBuilder
// [SECTION] ImFont
// [SECTION] ImGui Internal Render Helpers
// [SECTION] Decompression code
// [SECTION] Default font data (ProggyClean.ttf)

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_internal.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.h"
#endif

#include <stdio.h>      // vsnprintf, sscanf, printf

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4505)     // unreferenced local function has been removed (stb stuff)
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#pragma warning (disable: 26451)    // [Static Analyzer] Arithmetic overflow : Using operator 'xxx' on a 4 byte value and then casting the result to a 8 byte value. Cast the value to the wider type before calling operator 'xxx' to avoid overflow(io.2).
#pragma warning (disable: 26812)    // [Static Analyzer] The enum type 'xxx' is unscoped. Prefer 'enum class' over 'enum' (Enum.3). [MSVC Static Analyzer)
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"                    // warning: comparing floating point with == or != is unsafe // storing and comparing against same constants ok.
#pragma clang diagnostic ignored "-Wglobal-constructors"            // warning: declaration requires a global destructor         // similar to above, not sure what the exact difference is.
#pragma clang diagnostic ignored "-Wsign-conversion"                // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                    // some standard header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wcomma"                          // warning: possible misuse of comma operator here
#pragma clang diagnostic ignored "-Wreserved-id-macro"              // warning: macro name is a reserved identifier
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#pragma clang diagnostic ignored "-Wreserved-identifier"            // warning: identifier '_Xxx' is reserved because it starts with '_' followed by a capital letter
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"            // warning: 'xxx' is an unsafe pointer used for buffer access
#pragma clang diagnostic ignored "-Wnontrivial-memaccess"           // warning: first argument in call to 'memset' is a pointer to non-trivially copyable type
#pragma clang diagnostic ignored "-Wcast-qual"                      // warning: cast from 'const xxxx *' to 'xxx *' drops const qualifier
#pragma clang diagnostic ignored "-Wswitch-default"                 // warning: 'switch' missing 'default' label
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                          // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunused-function"                  // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wfloat-equal"                      // warning: comparing floating-point with '==' or '!=' is unsafe
#pragma GCC diagnostic ignored "-Wdouble-promotion"                 // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"                       // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wstack-protector"                  // warning: stack protector not protecting local variables: variable length buffer
#pragma GCC diagnostic ignored "-Wstrict-overflow"                  // warning: assuming signed overflow does not occur when simplifying division / ..when changing X +- C1 cmp C2 to X cmp C2 -+ C1
#pragma GCC diagnostic ignored "-Wclass-memaccess"                  // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#pragma GCC diagnostic ignored "-Wcast-qual"                        // warning: cast from type 'const xxxx *' to type 'xxxx *' casts away qualifiers
#endif

//-------------------------------------------------------------------------
// [SECTION] STB libraries implementation (for stb_truetype and stb_rect_pack)
//-------------------------------------------------------------------------

// Compile time options:
//#define IMGUI_STB_NAMESPACE           ImStb
//#define IMGUI_STB_TRUETYPE_FILENAME   "my_folder/stb_truetype.h"
//#define IMGUI_STB_RECT_PACK_FILENAME  "my_folder/stb_rect_pack.h"
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION

#ifdef IMGUI_STB_NAMESPACE
namespace IMGUI_STB_NAMESPACE
{
#endif

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)                             // declaration of 'xx' hides previous local declaration
#pragma warning (disable: 6011)                             // (stb_rectpack) Dereferencing NULL pointer 'cur->next'.
#pragma warning (disable: 6385)                             // (stb_truetype) Reading invalid data from 'buffer':  the readable size is '_Old_3`kernel_width' bytes, but '3' bytes may be read.
#pragma warning (disable: 28182)                            // (stb_rectpack) Dereferencing NULL pointer. 'cur' contains the same NULL value as 'cur->next' did.
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"        // warning: 'xxxx' defined but not used
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"              // warning: comparison is always true due to limited range of data type [-Wtype-limits]
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"     // warning: this statement may fall through
#endif

#ifndef STB_RECT_PACK_IMPLEMENTATION                        // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION          // in case the user already have an implementation in another compilation unit
#define STBRP_STATIC
#define STBRP_ASSERT(x)     do { IM_ASSERT(x); } while (0)
#define STBRP_SORT          ImQsort
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#ifdef IMGUI_STB_RECT_PACK_FILENAME
#include IMGUI_STB_RECT_PACK_FILENAME
#else
#include "imstb_rectpack.h"
#endif
#endif

#ifdef  IMGUI_ENABLE_STB_TRUETYPE
#ifndef STB_TRUETYPE_IMPLEMENTATION                         // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION           // in case the user already have an implementation in another compilation unit
#define STBTT_malloc(x,u)   ((void)(u), IM_ALLOC(x))
#define STBTT_free(x,u)     ((void)(u), IM_FREE(x))
#define STBTT_assert(x)     do { IM_ASSERT(x); } while(0)
#define STBTT_fmod(x,y)     ImFmod(x,y)
#define STBTT_sqrt(x)       ImSqrt(x)
#define STBTT_pow(x,y)      ImPow(x,y)
#define STBTT_fabs(x)       ImFabs(x)
#define STBTT_ifloor(x)     ((int)ImFloor(x))
#define STBTT_iceil(x)      ((int)ImCeil(x))
#define STBTT_strlen(x)     ImStrlen(x)
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#else
#define STBTT_DEF extern
#endif
#ifdef IMGUI_STB_TRUETYPE_FILENAME
#include IMGUI_STB_TRUETYPE_FILENAME
#else
#include "imstb_truetype.h"
#endif
#endif
#endif // IMGUI_ENABLE_STB_TRUETYPE

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#ifdef IMGUI_STB_NAMESPACE
} // namespace ImStb
using namespace IMGUI_STB_NAMESPACE;
#endif

//-----------------------------------------------------------------------------
// [SECTION] Style functions
//-----------------------------------------------------------------------------

void ImGui::StyleColorsDark(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabSelected]            = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline]    = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed]              = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected]      = ImLerp(colors[ImGuiCol_TabSelected],  colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink]               = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void ImGui::StyleColorsClassic(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.85f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImGuiCol_Border]                 = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.40f, 0.61f, 0.62f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    colors[ImGuiCol_Separator]              = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabSelected]            = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline]    = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed]              = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected]      = ImLerp(colors[ImGuiCol_TabSelected],  colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.53f, 0.53f, 0.87f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImGuiCol_TextLink]               = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]              = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

// Those light colors are better suited with a thicker font than the default one + FrameBorder
void ImGui::StyleColorsLight(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.90f);
    colors[ImGuiCol_TabSelected]            = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline]    = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed]              = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected]      = ImLerp(colors[ImGuiCol_TabSelected],  colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.26f, 0.59f, 1.00f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
    colors[ImGuiCol_TextLink]               = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavCursor]              = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawList
//-----------------------------------------------------------------------------

ImDrawListSharedData::ImDrawListSharedData()
{
    memset(this, 0, sizeof(*this));
    InitialFringeScale = 1.0f;
    for (int i = 0; i < IM_ARRAYSIZE(ArcFastVtx); i++)
    {
        const float a = ((float)i * 2 * IM_PI) / (float)IM_ARRAYSIZE(ArcFastVtx);
        ArcFastVtx[i] = ImVec2(ImCos(a), ImSin(a));
    }
    ArcFastRadiusCutoff = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC_R(IM_DRAWLIST_ARCFAST_SAMPLE_MAX, CircleSegmentMaxError);
}

void ImDrawListSharedData::SetCircleTessellationMaxError(float max_error)
{
    if (CircleSegmentMaxError == max_error)
        return;

    IM_ASSERT(max_error > 0.0f);
    CircleSegmentMaxError = max_error;
    for (int i = 0; i < IM_ARRAYSIZE(CircleSegmentCounts); i++)
    {
        const float radius = (float)i;
        CircleSegmentCounts[i] = (ImU8)((i > 0) ? IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, CircleSegmentMaxError) : IM_DRAWLIST_ARCFAST_SAMPLE_MAX);
    }
    ArcFastRadiusCutoff = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC_R(IM_DRAWLIST_ARCFAST_SAMPLE_MAX, CircleSegmentMaxError);
}

ImDrawList::ImDrawList(ImDrawListSharedData* shared_data)
{
    memset(this, 0, sizeof(*this));
    _Data = shared_data;
}

ImDrawList::~ImDrawList()
{
    _ClearFreeMemory();
}

// Initialize before use in a new frame. We always have a command ready in the buffer.
// In the majority of cases, you would want to call PushClipRect() and PushTextureID() after this.
void ImDrawList::_ResetForNewFrame()
{
    // Verify that the ImDrawCmd fields we want to memcmp() are contiguous in memory.
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, ClipRect) == 0);
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, TextureId) == sizeof(ImVec4));
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, VtxOffset) == sizeof(ImVec4) + sizeof(ImTextureID));
    if (_Splitter._Count > 1)
        _Splitter.Merge(this);

    CmdBuffer.resize(0);
    IdxBuffer.resize(0);
    VtxBuffer.resize(0);
    Flags = _Data->InitialFlags;
    memset(&_CmdHeader, 0, sizeof(_CmdHeader));
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.resize(0);
    _TextureIdStack.resize(0);
    _CallbacksDataBuf.resize(0);
    _Path.resize(0);
    _Splitter.Clear();
    CmdBuffer.push_back(ImDrawCmd());
    _FringeScale = _Data->InitialFringeScale;
}

void ImDrawList::_ClearFreeMemory()
{
    CmdBuffer.clear();
    IdxBuffer.clear();
    VtxBuffer.clear();
    Flags = ImDrawListFlags_None;
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.clear();
    _TextureIdStack.clear();
    _CallbacksDataBuf.clear();
    _Path.clear();
    _Splitter.ClearFreeMemory();
}

ImDrawList* ImDrawList::CloneOutput() const
{
    ImDrawList* dst = IM_NEW(ImDrawList(_Data));
    dst->CmdBuffer = CmdBuffer;
    dst->IdxBuffer = IdxBuffer;
    dst->VtxBuffer = VtxBuffer;
    dst->Flags = Flags;
    return dst;
}

void ImDrawList::AddDrawCmd()
{
    ImDrawCmd draw_cmd;
    draw_cmd.ClipRect = _CmdHeader.ClipRect;    // Same as calling ImDrawCmd_HeaderCopy()
    draw_cmd.TextureId = _CmdHeader.TextureId;
    draw_cmd.VtxOffset = _CmdHeader.VtxOffset;
    draw_cmd.IdxOffset = IdxBuffer.Size;

    IM_ASSERT(draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w);
    CmdBuffer.push_back(draw_cmd);
}

// Pop trailing draw command (used before merging or presenting to user)
// Note that this leaves the ImDrawList in a state unfit for further commands, as most code assume that CmdBuffer.Size > 0 && CmdBuffer.back().UserCallback == NULL
void ImDrawList::_PopUnusedDrawCmd()
{
    while (CmdBuffer.Size > 0)
    {
        ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
        if (curr_cmd->ElemCount != 0 || curr_cmd->UserCallback != NULL)
            return;// break;
        CmdBuffer.pop_back();
    }
}

void ImDrawList::AddCallback(ImDrawCallback callback, void* userdata, size_t userdata_size)
{
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    IM_ASSERT(curr_cmd->UserCallback == NULL);
    if (curr_cmd->ElemCount != 0)
    {
        AddDrawCmd();
        curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    }

    curr_cmd->UserCallback = callback;
    if (userdata_size == 0)
    {
        // Store user data directly in command (no indirection)
        curr_cmd->UserCallbackData = userdata;
        curr_cmd->UserCallbackDataSize = 0;
        curr_cmd->UserCallbackDataOffset = -1;
    }
    else
    {
        // Copy and store user data in a buffer
        IM_ASSERT(userdata != NULL);
        IM_ASSERT(userdata_size < (1u << 31));
        curr_cmd->UserCallbackData = NULL; // Will be resolved during Render()
        curr_cmd->UserCallbackDataSize = (int)userdata_size;
        curr_cmd->UserCallbackDataOffset = _CallbacksDataBuf.Size;
        _CallbacksDataBuf.resize(_CallbacksDataBuf.Size + (int)userdata_size);
        memcpy(_CallbacksDataBuf.Data + (size_t)curr_cmd->UserCallbackDataOffset, userdata, userdata_size);
    }

    AddDrawCmd(); // Force a new command after us (see comment below)
}

// Compare ClipRect, TextureId and VtxOffset with a single memcmp()
#define ImDrawCmd_HeaderSize                            (offsetof(ImDrawCmd, VtxOffset) + sizeof(unsigned int))
#define ImDrawCmd_HeaderCompare(CMD_LHS, CMD_RHS)       (memcmp(CMD_LHS, CMD_RHS, ImDrawCmd_HeaderSize))    // Compare ClipRect, TextureId, VtxOffset
#define ImDrawCmd_HeaderCopy(CMD_DST, CMD_SRC)          (memcpy(CMD_DST, CMD_SRC, ImDrawCmd_HeaderSize))    // Copy ClipRect, TextureId, VtxOffset
#define ImDrawCmd_AreSequentialIdxOffset(CMD_0, CMD_1)  (CMD_0->IdxOffset + CMD_0->ElemCount == CMD_1->IdxOffset)

// Try to merge two last draw commands
void ImDrawList::_TryMergeDrawCmds()
{
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (ImDrawCmd_HeaderCompare(curr_cmd, prev_cmd) == 0 && ImDrawCmd_AreSequentialIdxOffset(prev_cmd, curr_cmd) && curr_cmd->UserCallback == NULL && prev_cmd->UserCallback == NULL)
    {
        prev_cmd->ElemCount += curr_cmd->ElemCount;
        CmdBuffer.pop_back();
    }
}

// Our scheme may appears a bit unusual, basically we want the most-common calls AddLine AddRect etc. to not have to perform any check so we always have a command ready in the stack.
// The cost of figuring out if a new command has to be added or if we can merge is paid in those Update** functions only.
void ImDrawList::_OnChangedClipRect()
{
    // If current command is used with different settings we need to add a new command
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && memcmp(&curr_cmd->ClipRect, &_CmdHeader.ClipRect, sizeof(ImVec4)) != 0)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && ImDrawCmd_AreSequentialIdxOffset(prev_cmd, curr_cmd) && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }
    curr_cmd->ClipRect = _CmdHeader.ClipRect;
}

void ImDrawList::_OnChangedTextureID()
{
    // If current command is used with different settings we need to add a new command
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && curr_cmd->TextureId != _CmdHeader.TextureId)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && ImDrawCmd_AreSequentialIdxOffset(prev_cmd, curr_cmd) && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }
    curr_cmd->TextureId = _CmdHeader.TextureId;
}

void ImDrawList::_OnChangedVtxOffset()
{
    // We don't need to compare curr_cmd->VtxOffset != _CmdHeader.VtxOffset because we know it'll be different at the time we call this.
    _VtxCurrentIdx = 0;
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    //IM_ASSERT(curr_cmd->VtxOffset != _CmdHeader.VtxOffset); // See #3349
    if (curr_cmd->ElemCount != 0)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);
    curr_cmd->VtxOffset = _CmdHeader.VtxOffset;
}

int ImDrawList::_CalcCircleAutoSegmentCount(float radius) const
{
    // Automatic segment count
    const int radius_idx = (int)(radius + 0.999999f); // ceil to never reduce accuracy
    if (radius_idx >= 0 && radius_idx < IM_ARRAYSIZE(_Data->CircleSegmentCounts))
        return _Data->CircleSegmentCounts[radius_idx]; // Use cached value
    else
        return IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, _Data->CircleSegmentMaxError);
}

// Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
void ImDrawList::PushClipRect(const ImVec2& cr_min, const ImVec2& cr_max, bool intersect_with_current_clip_rect)
{
    ImVec4 cr(cr_min.x, cr_min.y, cr_max.x, cr_max.y);
    if (intersect_with_current_clip_rect)
    {
        ImVec4 current = _CmdHeader.ClipRect;
        if (cr.x < current.x) cr.x = current.x;
        if (cr.y < current.y) cr.y = current.y;
        if (cr.z > current.z) cr.z = current.z;
        if (cr.w > current.w) cr.w = current.w;
    }
    cr.z = ImMax(cr.x, cr.z);
    cr.w = ImMax(cr.y, cr.w);

    _ClipRectStack.push_back(cr);
    _CmdHeader.ClipRect = cr;
    _OnChangedClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
    PushClipRect(ImVec2(_Data->ClipRectFullscreen.x, _Data->ClipRectFullscreen.y), ImVec2(_Data->ClipRectFullscreen.z, _Data->ClipRectFullscreen.w));
}

void ImDrawList::PopClipRect()
{
    _ClipRectStack.pop_back();
    _CmdHeader.ClipRect = (_ClipRectStack.Size == 0) ? _Data->ClipRectFullscreen : _ClipRectStack.Data[_ClipRectStack.Size - 1];
    _OnChangedClipRect();
}

void ImDrawList::PushTextureID(ImTextureID texture_id)
{
    _TextureIdStack.push_back(texture_id);
    _CmdHeader.TextureId = texture_id;
    _OnChangedTextureID();
}

void ImDrawList::PopTextureID()
{
    _TextureIdStack.pop_back();
    _CmdHeader.TextureId = (_TextureIdStack.Size == 0) ? (ImTextureID)NULL : _TextureIdStack.Data[_TextureIdStack.Size - 1];
    _OnChangedTextureID();
}

// This is used by ImGui::PushFont()/PopFont(). It works because we never use _TextureIdStack[] elsewhere than in PushTextureID()/PopTextureID().
void ImDrawList::_SetTextureID(ImTextureID texture_id)
{
    if (_CmdHeader.TextureId == texture_id)
        return;
    _CmdHeader.TextureId = texture_id;
    _OnChangedTextureID();
}

// Reserve space for a number of vertices and indices.
// You must finish filling your reserved data before calling PrimReserve() again, as it may reallocate or
// submit the intermediate results. PrimUnreserve() can be used to release unused allocations.
void ImDrawList::PrimReserve(int idx_count, int vtx_count)
{
    // Large mesh support (when enabled)
    IM_ASSERT_PARANOID(idx_count >= 0 && vtx_count >= 0);
    if (sizeof(ImDrawIdx) == 2 && (_VtxCurrentIdx + vtx_count >= (1 << 16)) && (Flags & ImDrawListFlags_AllowVtxOffset))
    {
        // FIXME: In theory we should be testing that vtx_count <64k here.
        // In practice, RenderText() relies on reserving ahead for a worst case scenario so it is currently useful for us
        // to not make that check until we rework the text functions to handle clipping and large horizontal lines better.
        _CmdHeader.VtxOffset = VtxBuffer.Size;
        _OnChangedVtxOffset();
    }

    ImDrawCmd* draw_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd->ElemCount += idx_count;

    int vtx_buffer_old_size = VtxBuffer.Size;
    VtxBuffer.resize(vtx_buffer_old_size + vtx_count);
    _VtxWritePtr = VtxBuffer.Data + vtx_buffer_old_size;

    int idx_buffer_old_size = IdxBuffer.Size;
    IdxBuffer.resize(idx_buffer_old_size + idx_count);
    _IdxWritePtr = IdxBuffer.Data + idx_buffer_old_size;
}

// Release the number of reserved vertices/indices from the end of the last reservation made with PrimReserve().
void ImDrawList::PrimUnreserve(int idx_count, int vtx_count)
{
    IM_ASSERT_PARANOID(idx_count >= 0 && vtx_count >= 0);

    ImDrawCmd* draw_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd->ElemCount -= idx_count;
    VtxBuffer.shrink(VtxBuffer.Size - vtx_count);
    IdxBuffer.shrink(IdxBuffer.Size - idx_count);
}

// Fully unrolled with inline call to keep our debug builds decently fast.
void ImDrawList::PrimRect(const ImVec2& a, const ImVec2& c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv(_Data->TexUvWhitePixel);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimRectUV(const ImVec2& a, const ImVec2& c, const ImVec2& uv_a, const ImVec2& uv_c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimQuadUV(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, ImU32 col)
{
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

// On AddPolyline() and AddConvexPolyFilled() we intentionally avoid using ImVec2 and superfluous function calls to optimize debug/non-inlined builds.
// - Those macros expects l-values and need to be used as their own statement.
// - Those macros are intentionally not surrounded by the 'do {} while (0)' idiom because even that translates to runtime with debug compilers.
#define IM_NORMALIZE2F_OVER_ZERO(VX,VY)     { float d2 = VX*VX + VY*VY; if (d2 > 0.0f) { float inv_len = ImRsqrt(d2); VX *= inv_len; VY *= inv_len; } } (void)0
#define IM_FIXNORMAL2F_MAX_INVLEN2          100.0f // 500.0f (see #4053, #3366)
#define IM_FIXNORMAL2F(VX,VY)               { float d2 = VX*VX + VY*VY; if (d2 > 0.000001f) { float inv_len2 = 1.0f / d2; if (inv_len2 > IM_FIXNORMAL2F_MAX_INVLEN2) inv_len2 = IM_FIXNORMAL2F_MAX_INVLEN2; VX *= inv_len2; VY *= inv_len2; } } (void)0

// TODO: Thickness anti-aliased lines cap are missing their AA fringe.
// We avoid using the ImVec2 math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddPolyline(const ImVec2* points, const int points_count, ImU32 col, ImDrawFlags flags, float thickness)
{
    if (points_count < 2 || (col & IM_COL32_A_MASK) == 0)
        return;

    const bool closed = (flags & ImDrawFlags_Closed) != 0;
    const ImVec2 opaque_uv = _Data->TexUvWhitePixel;
    const int count = closed ? points_count : points_count - 1; // The number of line segments we need to draw
    const bool thick_line = (thickness > _FringeScale);

    if (Flags & ImDrawListFlags_AntiAliasedLines)
    {
        // Anti-aliased stroke
        const float AA_SIZE = _FringeScale;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;

        // Thicknesses <1.0 should behave like thickness 1.0
        thickness = ImMax(thickness, 1.0f);
        const int integer_thickness = (int)thickness;
        const float fractional_thickness = thickness - integer_thickness;

        // Do we want to draw this line using a texture?
        // - For now, only draw integer-width lines using textures to avoid issues with the way scaling occurs, could be improved.
        // - If AA_SIZE is not 1.0f we cannot use the texture path.
        const bool use_texture = (Flags & ImDrawListFlags_AntiAliasedLinesUseTex) && (integer_thickness < IM_DRAWLIST_TEX_LINES_WIDTH_MAX) && (fractional_thickness <= 0.00001f) && (AA_SIZE == 1.0f);

        // We should never hit this, because NewFrame() doesn't set ImDrawListFlags_AntiAliasedLinesUseTex unless ImFontAtlasFlags_NoBakedLines is off
        IM_ASSERT_PARANOID(!use_texture || !(_Data->Font->ContainerAtlas->Flags & ImFontAtlasFlags_NoBakedLines));

        const int idx_count = use_texture ? (count * 6) : (thick_line ? count * 18 : count * 12);
        const int vtx_count = use_texture ? (points_count * 2) : (thick_line ? points_count * 4 : points_count * 3);
        PrimReserve(idx_count, vtx_count);

        // Temporary buffer
        // The first <points_count> items are normals at each line point, then after that there are either 2 or 4 temp points for each line point
        _Data->TempBuffer.reserve_discard(points_count * ((use_texture || !thick_line) ? 3 : 5));
        ImVec2* temp_normals = _Data->TempBuffer.Data;
        ImVec2* temp_points = temp_normals + points_count;

        // Calculate normals (tangents) for each line segment
        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
            float dx = points[i2].x - points[i1].x;
            float dy = points[i2].y - points[i1].y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i1].x = dy;
            temp_normals[i1].y = -dx;
        }
        if (!closed)
            temp_normals[points_count - 1] = temp_normals[points_count - 2];

        // If we are drawing a one-pixel-wide line without a texture, or a textured line of any width, we only need 2 or 3 vertices per point
        if (use_texture || !thick_line)
        {
            // [PATH 1] Texture-based lines (thick or non-thick)
            // [PATH 2] Non texture-based lines (non-thick)

            // The width of the geometry we need to draw - this is essentially <thickness> pixels for the line itself, plus "one pixel" for AA.
            // - In the texture-based path, we don't use AA_SIZE here because the +1 is tied to the generated texture
            //   (see ImFontAtlasBuildRenderLinesTexData() function), and so alternate values won't work without changes to that code.
            // - In the non texture-based paths, we would allow AA_SIZE to potentially be != 1.0f with a patch (e.g. fringe_scale patch to
            //   allow scaling geometry while preserving one-screen-pixel AA fringe).
            const float half_draw_size = use_texture ? ((thickness * 0.5f) + 1) : AA_SIZE;

            // If line is not closed, the first and last points need to be generated differently as there are no normals to blend
            if (!closed)
            {
                temp_points[0] = points[0] + temp_normals[0] * half_draw_size;
                temp_points[1] = points[0] - temp_normals[0] * half_draw_size;
                temp_points[(points_count-1)*2+0] = points[points_count-1] + temp_normals[points_count-1] * half_draw_size;
                temp_points[(points_count-1)*2+1] = points[points_count-1] - temp_normals[points_count-1] * half_draw_size;
            }

            // Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
            // This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx; // Vertex index for start of line segment
            for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
            {
                const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1; // i2 is the second point of the line segment
                const unsigned int idx2 = ((i1 + 1) == points_count) ? _VtxCurrentIdx : (idx1 + (use_texture ? 2 : 3)); // Vertex index for end of segment

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                dm_x *= half_draw_size; // dm_x, dm_y are offset to the outer edge of the AA area
                dm_y *= half_draw_size;

                // Add temporary vertexes for the outer edges
                ImVec2* out_vtx = &temp_points[i2 * 2];
                out_vtx[0].x = points[i2].x + dm_x;
                out_vtx[0].y = points[i2].y + dm_y;
                out_vtx[1].x = points[i2].x - dm_x;
                out_vtx[1].y = points[i2].y - dm_y;

                if (use_texture)
                {
                    // Add indices for two triangles
                    _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 1); // Right tri
                    _IdxWritePtr[3] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[4] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 0); // Left tri
                    _IdxWritePtr += 6;
                }
                else
                {
                    // Add indexes for four triangles
                    _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 2); // Right tri 1
                    _IdxWritePtr[3] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[4] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 0); // Right tri 2
                    _IdxWritePtr[6] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[7] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[8] = (ImDrawIdx)(idx1 + 0); // Left tri 1
                    _IdxWritePtr[9] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1); // Left tri 2
                    _IdxWritePtr += 12;
                }

                idx1 = idx2;
            }

            // Add vertexes for each point on the line
            if (use_texture)
            {
                // If we're using textures we only need to emit the left/right edge vertices
                ImVec4 tex_uvs = _Data->TexUvLines[integer_thickness];
                /*if (fractional_thickness != 0.0f) // Currently always zero when use_texture==false!
                {
                    const ImVec4 tex_uvs_1 = _Data->TexUvLines[integer_thickness + 1];
                    tex_uvs.x = tex_uvs.x + (tex_uvs_1.x - tex_uvs.x) * fractional_thickness; // inlined ImLerp()
                    tex_uvs.y = tex_uvs.y + (tex_uvs_1.y - tex_uvs.y) * fractional_thickness;
                    tex_uvs.z = tex_uvs.z + (tex_uvs_1.z - tex_uvs.z) * fractional_thickness;
                    tex_uvs.w = tex_uvs.w + (tex_uvs_1.w - tex_uvs.w) * fractional_thickness;
                }*/
                ImVec2 tex_uv0(tex_uvs.x, tex_uvs.y);
                ImVec2 tex_uv1(tex_uvs.z, tex_uvs.w);
                for (int i = 0; i < points_count; i++)
                {
                    _VtxWritePtr[0].pos = temp_points[i * 2 + 0]; _VtxWritePtr[0].uv = tex_uv0; _VtxWritePtr[0].col = col; // Left-side outer edge
                    _VtxWritePtr[1].pos = temp_points[i * 2 + 1]; _VtxWritePtr[1].uv = tex_uv1; _VtxWritePtr[1].col = col; // Right-side outer edge
                    _VtxWritePtr += 2;
                }
            }
            else
            {
                // If we're not using a texture, we need the center vertex as well
                for (int i = 0; i < points_count; i++)
                {
                    _VtxWritePtr[0].pos = points[i];              _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;       // Center of line
                    _VtxWritePtr[1].pos = temp_points[i * 2 + 0]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col_trans; // Left-side outer edge
                    _VtxWritePtr[2].pos = temp_points[i * 2 + 1]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col_trans; // Right-side outer edge
                    _VtxWritePtr += 3;
                }
            }
        }
        else
        {
            // [PATH 2] Non texture-based lines (thick): we need to draw the solid line core and thus require four vertices per point
            const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;

            // If line is not closed, the first and last points need to be generated differently as there are no normals to blend
            if (!closed)
            {
                const int points_last = points_count - 1;
                temp_points[0] = points[0] + temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[1] = points[0] + temp_normals[0] * (half_inner_thickness);
                temp_points[2] = points[0] - temp_normals[0] * (half_inner_thickness);
                temp_points[3] = points[0] - temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[points_last * 4 + 0] = points[points_last] + temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
                temp_points[points_last * 4 + 1] = points[points_last] + temp_normals[points_last] * (half_inner_thickness);
                temp_points[points_last * 4 + 2] = points[points_last] - temp_normals[points_last] * (half_inner_thickness);
                temp_points[points_last * 4 + 3] = points[points_last] - temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
            }

            // Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
            // This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx; // Vertex index for start of line segment
            for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
            {
                const int i2 = (i1 + 1) == points_count ? 0 : (i1 + 1); // i2 is the second point of the line segment
                const unsigned int idx2 = (i1 + 1) == points_count ? _VtxCurrentIdx : (idx1 + 4); // Vertex index for end of segment

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                float dm_out_x = dm_x * (half_inner_thickness + AA_SIZE);
                float dm_out_y = dm_y * (half_inner_thickness + AA_SIZE);
                float dm_in_x = dm_x * half_inner_thickness;
                float dm_in_y = dm_y * half_inner_thickness;

                // Add temporary vertices
                ImVec2* out_vtx = &temp_points[i2 * 4];
                out_vtx[0].x = points[i2].x + dm_out_x;
                out_vtx[0].y = points[i2].y + dm_out_y;
                out_vtx[1].x = points[i2].x + dm_in_x;
                out_vtx[1].y = points[i2].y + dm_in_y;
                out_vtx[2].x = points[i2].x - dm_in_x;
                out_vtx[2].y = points[i2].y - dm_in_y;
                out_vtx[3].x = points[i2].x - dm_out_x;
                out_vtx[3].y = points[i2].y - dm_out_y;

                // Add indexes
                _IdxWritePtr[0]  = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[1]  = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[2]  = (ImDrawIdx)(idx1 + 2);
                _IdxWritePtr[3]  = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[4]  = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[5]  = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr[6]  = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[7]  = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[8]  = (ImDrawIdx)(idx1 + 0);
                _IdxWritePtr[9]  = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr[12] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[13] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[14] = (ImDrawIdx)(idx1 + 3);
                _IdxWritePtr[15] = (ImDrawIdx)(idx1 + 3); _IdxWritePtr[16] = (ImDrawIdx)(idx2 + 3); _IdxWritePtr[17] = (ImDrawIdx)(idx2 + 2);
                _IdxWritePtr += 18;

                idx1 = idx2;
            }

            // Add vertices
            for (int i = 0; i < points_count; i++)
            {
                _VtxWritePtr[0].pos = temp_points[i * 4 + 0]; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col_trans;
                _VtxWritePtr[1].pos = temp_points[i * 4 + 1]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
                _VtxWritePtr[2].pos = temp_points[i * 4 + 2]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
                _VtxWritePtr[3].pos = temp_points[i * 4 + 3]; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col_trans;
                _VtxWritePtr += 4;
            }
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // [PATH 4] Non texture-based, Non anti-aliased lines
        const int idx_count = count * 6;
        const int vtx_count = count * 4;    // FIXME-OPT: Not sharing edges
        PrimReserve(idx_count, vtx_count);

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
            const ImVec2& p1 = points[i1];
            const ImVec2& p2 = points[i2];

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            dx *= (thickness * 0.5f);
            dy *= (thickness * 0.5f);

            _VtxWritePtr[0].pos.x = p1.x + dy; _VtxWritePtr[0].pos.y = p1.y - dx; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr[1].pos.x = p2.x + dy; _VtxWritePtr[1].pos.y = p2.y - dx; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
            _VtxWritePtr[2].pos.x = p2.x - dy; _VtxWritePtr[2].pos.y = p2.y + dx; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
            _VtxWritePtr[3].pos.x = p1.x - dy; _VtxWritePtr[3].pos.y = p1.y + dx; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col;
            _VtxWritePtr += 4;

            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + 1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + 2);
            _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx + 2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx + 3);
            _IdxWritePtr += 6;
            _VtxCurrentIdx += 4;
        }
    }
}

// - We intentionally avoid using ImVec2 and its math operators here to reduce cost to a minimum for debug/non-inlined builds.
// - Filled shapes must always use clockwise winding order. The anti-aliasing fringe depends on it. Counter-clockwise shapes will have "inward" anti-aliasing.
void ImDrawList::AddConvexPolyFilled(const ImVec2* points, const int points_count, ImU32 col)
{
    if (points_count < 3 || (col & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;

    if (Flags & ImDrawListFlags_AntiAliasedFill)
    {
        // Anti-aliased Fill
        const float AA_SIZE = _FringeScale;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;
        const int idx_count = (points_count - 2)*3 + points_count * 6;
        const int vtx_count = (points_count * 2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx + 1;
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + ((i - 1) << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx + (i << 1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        _Data->TempBuffer.reserve_discard(points_count);
        ImVec2* temp_normals = _Data->TempBuffer.Data;
        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i0].x = dy;
            temp_normals[i0].y = -dx;
        }

        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            float dm_x = (n0.x + n1.x) * 0.5f;
            float dm_y = (n0.y + n1.y) * 0.5f;
            IM_FIXNORMAL2F(dm_x, dm_y);
            dm_x *= AA_SIZE * 0.5f;
            dm_y *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (i0 << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx + (i1 << 1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count - 2)*3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + i - 1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + i);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

void ImDrawList::_PathArcToFastEx(const ImVec2& center, float radius, int a_min_sample, int a_max_sample, int a_step)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }

    // Calculate arc auto segment step size
    if (a_step <= 0)
        a_step = IM_DRAWLIST_ARCFAST_SAMPLE_MAX / _CalcCircleAutoSegmentCount(radius);

    // Make sure we never do steps larger than one quarter of the circle
    a_step = ImClamp(a_step, 1, IM_DRAWLIST_ARCFAST_TABLE_SIZE / 4);

    const int sample_range = ImAbs(a_max_sample - a_min_sample);
    const int a_next_step = a_step;

    int samples = sample_range + 1;
    bool extra_max_sample = false;
    if (a_step > 1)
    {
        samples            = sample_range / a_step + 1;
        const int overstep = sample_range % a_step;

        if (overstep > 0)
        {
            extra_max_sample = true;
            samples++;

            // When we have overstep to avoid awkwardly looking one long line and one tiny one at the end,
            // distribute first step range evenly between them by reducing first step size.
            if (sample_range > 0)
                a_step -= (a_step - overstep) / 2;
        }
    }

    _Path.resize(_Path.Size + samples);
    ImVec2* out_ptr = _Path.Data + (_Path.Size - samples);

    int sample_index = a_min_sample;
    if (sample_index < 0 || sample_index >= IM_DRAWLIST_ARCFAST_SAMPLE_MAX)
    {
        sample_index = sample_index % IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        if (sample_index < 0)
            sample_index += IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
    }

    if (a_max_sample >= a_min_sample)
    {
        for (int a = a_min_sample; a <= a_max_sample; a += a_step, sample_index += a_step, a_step = a_next_step)
        {
            // a_step is clamped to IM_DRAWLIST_ARCFAST_SAMPLE_MAX, so we have guaranteed that it will not wrap over range twice or more
            if (sample_index >= IM_DRAWLIST_ARCFAST_SAMPLE_MAX)
                sample_index -= IM_DRAWLIST_ARCFAST_SAMPLE_MAX;

            const ImVec2 s = _Data->ArcFastVtx[sample_index];
            out_ptr->x = center.x + s.x * radius;
            out_ptr->y = center.y + s.y * radius;
            out_ptr++;
        }
    }
    else
    {
        for (int a = a_min_sample; a >= a_max_sample; a -= a_step, sample_index -= a_step, a_step = a_next_step)
        {
            // a_step is clamped to IM_DRAWLIST_ARCFAST_SAMPLE_MAX, so we have guaranteed that it will not wrap over range twice or more
            if (sample_index < 0)
                sample_index += IM_DRAWLIST_ARCFAST_SAMPLE_MAX;

            const ImVec2 s = _Data->ArcFastVtx[sample_index];
            out_ptr->x = center.x + s.x * radius;
            out_ptr->y = center.y + s.y * radius;
            out_ptr++;
        }
    }

    if (extra_max_sample)
    {
        int normalized_max_sample = a_max_sample % IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        if (normalized_max_sample < 0)
            normalized_max_sample += IM_DRAWLIST_ARCFAST_SAMPLE_MAX;

        const ImVec2 s = _Data->ArcFastVtx[normalized_max_sample];
        out_ptr->x = center.x + s.x * radius;
        out_ptr->y = center.y + s.y * radius;
        out_ptr++;
    }

    IM_ASSERT_PARANOID(_Path.Data + _Path.Size == out_ptr);
}

void ImDrawList::_PathArcToN(const ImVec2& center, float radius, float a_min, float a_max, int num_segments)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }

    // Note that we are adding a point at both a_min and a_max.
    // If you are trying to draw a full closed circle you don't want the overlapping points!
    _Path.reserve(_Path.Size + (num_segments + 1));
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        _Path.push_back(ImVec2(center.x + ImCos(a) * radius, center.y + ImSin(a) * radius));
    }
}

// 0: East, 3: South, 6: West, 9: North, 12: East
void ImDrawList::PathArcToFast(const ImVec2& center, float radius, int a_min_of_12, int a_max_of_12)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }
    _PathArcToFastEx(center, radius, a_min_of_12 * IM_DRAWLIST_ARCFAST_SAMPLE_MAX / 12, a_max_of_12 * IM_DRAWLIST_ARCFAST_SAMPLE_MAX / 12, 0);
}

void ImDrawList::PathArcTo(const ImVec2& center, float radius, float a_min, float a_max, int num_segments)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }

    if (num_segments > 0)
    {
        _PathArcToN(center, radius, a_min, a_max, num_segments);
        return;
    }

    // Automatic segment count
    if (radius <= _Data->ArcFastRadiusCutoff)
    {
        const bool a_is_reverse = a_max < a_min;

        // We are going to use precomputed values for mid samples.
        // Determine first and last sample in lookup table that belong to the arc.
        const float a_min_sample_f = IM_DRAWLIST_ARCFAST_SAMPLE_MAX * a_min / (IM_PI * 2.0f);
        const float a_max_sample_f = IM_DRAWLIST_ARCFAST_SAMPLE_MAX * a_max / (IM_PI * 2.0f);

        const int a_min_sample = a_is_reverse ? (int)ImFloor(a_min_sample_f) : (int)ImCeil(a_min_sample_f);
        const int a_max_sample = a_is_reverse ? (int)ImCeil(a_max_sample_f) : (int)ImFloor(a_max_sample_f);
        const int a_mid_samples = a_is_reverse ? ImMax(a_min_sample - a_max_sample, 0) : ImMax(a_max_sample - a_min_sample, 0);

        const float a_min_segment_angle = a_min_sample * IM_PI * 2.0f / IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        const float a_max_segment_angle = a_max_sample * IM_PI * 2.0f / IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        const bool a_emit_start = ImAbs(a_min_segment_angle - a_min) >= 1e-5f;
        const bool a_emit_end = ImAbs(a_max - a_max_segment_angle) >= 1e-5f;

        _Path.reserve(_Path.Size + (a_mid_samples + 1 + (a_emit_start ? 1 : 0) + (a_emit_end ? 1 : 0)));
        if (a_emit_start)
            _Path.push_back(ImVec2(center.x + ImCos(a_min) * radius, center.y + ImSin(a_min) * radius));
        if (a_mid_samples > 0)
            _PathArcToFastEx(center, radius, a_min_sample, a_max_sample, 0);
        if (a_emit_end)
            _Path.push_back(ImVec2(center.x + ImCos(a_max) * radius, center.y + ImSin(a_max) * radius));
    }
    else
    {
        const float arc_length = ImAbs(a_max - a_min);
        const int circle_segment_count = _CalcCircleAutoSegmentCount(radius);
        const int arc_segment_count = ImMax((int)ImCeil(circle_segment_count * arc_length / (IM_PI * 2.0f)), (int)(2.0f * IM_PI / arc_length));
        _PathArcToN(center, radius, a_min, a_max, arc_segment_count);
    }
}

void ImDrawList::PathEllipticalArcTo(const ImVec2& center, const ImVec2& radius, float rot, float a_min, float a_max, int num_segments)
{
    if (num_segments <= 0)
        num_segments = _CalcCircleAutoSegmentCount(ImMax(radius.x, radius.y)); // A bit pessimistic, maybe there's a better computation to do here.

    _Path.reserve(_Path.Size + (num_segments + 1));

    const float cos_rot = ImCos(rot);
    const float sin_rot = ImSin(rot);
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        ImVec2 point(ImCos(a) * radius.x, ImSin(a) * radius.y);
        const ImVec2 rel((point.x * cos_rot) - (point.y * sin_rot), (point.x * sin_rot) + (point.y * cos_rot));
        point.x = rel.x + center.x;
        point.y = rel.y + center.y;
        _Path.push_back(point);
    }
}

ImVec2 ImBezierCubicCalc(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, float t)
{
    float u = 1.0f - t;
    float w1 = u * u * u;
    float w2 = 3 * u * u * t;
    float w3 = 3 * u * t * t;
    float w4 = t * t * t;
    return ImVec2(w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x, w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y);
}

ImVec2 ImBezierQuadraticCalc(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, float t)
{
    float u = 1.0f - t;
    float w1 = u * u;
    float w2 = 2 * u * t;
    float w3 = t * t;
    return ImVec2(w1 * p1.x + w2 * p2.x + w3 * p3.x, w1 * p1.y + w2 * p2.y + w3 * p3.y);
}

// Closely mimics ImBezierCubicClosestPointCasteljau() in imgui.cpp
static void PathBezierCubicCurveToCasteljau(ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = (x2 - x4) * dy - (y2 - y4) * dx;
    float d3 = (x3 - x4) * dy - (y3 - y4) * dx;
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
    {
        path->push_back(ImVec2(x4, y4));
    }
    else if (level < 10)
    {
        float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
        float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
        float x34 = (x3 + x4) * 0.5f, y34 = (y3 + y4) * 0.5f;
        float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
        float x234 = (x23 + x34) * 0.5f, y234 = (y23 + y34) * 0.5f;
        float x1234 = (x123 + x234) * 0.5f, y1234 = (y123 + y234) * 0.5f;
        PathBezierCubicCurveToCasteljau(path, x1, y1, x12, y12, x123, y123, x1234, y1234, tess_tol, level + 1);
        PathBezierCubicCurveToCasteljau(path, x1234, y1234, x234, y234, x34, y34, x4, y4, tess_tol, level + 1);
    }
}

static void PathBezierQuadraticCurveToCasteljau(ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float tess_tol, int level)
{
    float dx = x3 - x1, dy = y3 - y1;
    float det = (x2 - x3) * dy - (y2 - y3) * dx;
    if (det * det * 4.0f < tess_tol * (dx * dx + dy * dy))
    {
        path->push_back(ImVec2(x3, y3));
    }
    else if (level < 10)
    {
        float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
        float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
        float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
        PathBezierQuadraticCurveToCasteljau(path, x1, y1, x12, y12, x123, y123, tess_tol, level + 1);
        PathBezierQuadraticCurveToCasteljau(path, x123, y123, x23, y23, x3, y3, tess_tol, level + 1);
    }
}

void ImDrawList::PathBezierCubicCurveTo(const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments)
{
    ImVec2 p1 = _Path.back();
    if (num_segments == 0)
    {
        IM_ASSERT(_Data->CurveTessellationTol > 0.0f);
        PathBezierCubicCurveToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, _Data->CurveTessellationTol, 0); // Auto-tessellated
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
            _Path.push_back(ImBezierCubicCalc(p1, p2, p3, p4, t_step * i_step));
    }
}

void ImDrawList::PathBezierQuadraticCurveTo(const ImVec2& p2, const ImVec2& p3, int num_segments)
{
    ImVec2 p1 = _Path.back();
    if (num_segments == 0)
    {
        IM_ASSERT(_Data->CurveTessellationTol > 0.0f);
        PathBezierQuadraticCurveToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, _Data->CurveTessellationTol, 0);// Auto-tessellated
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
            _Path.push_back(ImBezierQuadraticCalc(p1, p2, p3, t_step * i_step));
    }
}

static inline ImDrawFlags FixRectCornerFlags(ImDrawFlags flags)
{
    /*
    IM_STATIC_ASSERT(ImDrawFlags_RoundCornersTopLeft == (1 << 4));
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    // Obsoleted in 1.82 (from February 2021). This code was stripped/simplified and mostly commented in 1.90 (from September 2023)
    // - Legacy Support for hard coded ~0 (used to be a suggested equivalent to ImDrawCornerFlags_All)
    if (flags == ~0)                    { return ImDrawFlags_RoundCornersAll; }
    // - Legacy Support for hard coded 0x01 to 0x0F (matching 15 out of 16 old flags combinations). Read details in older version of this code.
    if (flags >= 0x01 && flags <= 0x0F) { return (flags << 4); }
    // We cannot support hard coded 0x00 with 'float rounding > 0.0f' --> replace with ImDrawFlags_RoundCornersNone or use 'float rounding = 0.0f'
#endif
    */
    // If this assert triggers, please update your code replacing hardcoded values with new ImDrawFlags_RoundCorners* values.
    // Note that ImDrawFlags_Closed (== 0x01) is an invalid flag for AddRect(), AddRectFilled(), PathRect() etc. anyway.
    // See details in 1.82 Changelog as well as 2021/03/12 and 2023/09/08 entries in "API BREAKING CHANGES" section.
    IM_ASSERT((flags & 0x0F) == 0 && "Misuse of legacy hardcoded ImDrawCornerFlags values!");

    if ((flags & ImDrawFlags_RoundCornersMask_) == 0)
        flags |= ImDrawFlags_RoundCornersAll;

    return flags;
}

void ImDrawList::PathRect(const ImVec2& a, const ImVec2& b, float rounding, ImDrawFlags flags)
{
    if (rounding >= 0.5f)
    {
        flags = FixRectCornerFlags(flags);
        rounding = ImMin(rounding, ImFabs(b.x - a.x) * (((flags & ImDrawFlags_RoundCornersTop) == ImDrawFlags_RoundCornersTop) || ((flags & ImDrawFlags_RoundCornersBottom) == ImDrawFlags_RoundCornersBottom) ? 0.5f : 1.0f) - 1.0f);
        rounding = ImMin(rounding, ImFabs(b.y - a.y) * (((flags & ImDrawFlags_RoundCornersLeft) == ImDrawFlags_RoundCornersLeft) || ((flags & ImDrawFlags_RoundCornersRight) == ImDrawFlags_RoundCornersRight) ? 0.5f : 1.0f) - 1.0f);
    }
    if (rounding < 0.5f || (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone)
    {
        PathLineTo(a);
        PathLineTo(ImVec2(b.x, a.y));
        PathLineTo(b);
        PathLineTo(ImVec2(a.x, b.y));
    }
    else
    {
        const float rounding_tl = (flags & ImDrawFlags_RoundCornersTopLeft)     ? rounding : 0.0f;
        const float rounding_tr = (flags & ImDrawFlags_RoundCornersTopRight)    ? rounding : 0.0f;
        const float rounding_br = (flags & ImDrawFlags_RoundCornersBottomRight) ? rounding : 0.0f;
        const float rounding_bl = (flags & ImDrawFlags_RoundCornersBottomLeft)  ? rounding : 0.0f;
        PathArcToFast(ImVec2(a.x + rounding_tl, a.y + rounding_tl), rounding_tl, 6, 9);
        PathArcToFast(ImVec2(b.x - rounding_tr, a.y + rounding_tr), rounding_tr, 9, 12);
        PathArcToFast(ImVec2(b.x - rounding_br, b.y - rounding_br), rounding_br, 0, 3);
        PathArcToFast(ImVec2(a.x + rounding_bl, b.y - rounding_bl), rounding_bl, 3, 6);
    }
}

void ImDrawList::AddLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    PathLineTo(p1 + ImVec2(0.5f, 0.5f));
    PathLineTo(p2 + ImVec2(0.5f, 0.5f));
    PathStroke(col, 0, thickness);
}

// p_min = upper-left, p_max = lower-right
// Note we don't render 1 pixels sized rectangles properly.
void ImDrawList::AddRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (Flags & ImDrawListFlags_AntiAliasedLines)
        PathRect(p_min + ImVec2(0.50f, 0.50f), p_max - ImVec2(0.50f, 0.50f), rounding, flags);
    else
        PathRect(p_min + ImVec2(0.50f, 0.50f), p_max - ImVec2(0.49f, 0.49f), rounding, flags); // Better looking lower-right corner and rounded non-AA shapes.
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddRectFilled(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (rounding < 0.5f || (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone)
    {
        PrimReserve(6, 4);
        PrimRect(p_min, p_max, col);
    }
    else
    {
        PathRect(p_min, p_max, rounding, flags);
        PathFillConvex(col);
    }
}

// p_min = upper-left, p_max = lower-right
void ImDrawList::AddRectFilledMultiColor(const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left)
{
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;
    PrimReserve(6, 4);
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 1)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2));
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 3));
    PrimWriteVtx(p_min, uv, col_upr_left);
    PrimWriteVtx(ImVec2(p_max.x, p_min.y), uv, col_upr_right);
    PrimWriteVtx(p_max, uv, col_bot_right);
    PrimWriteVtx(ImVec2(p_min.x, p_max.y), uv, col_bot_left);
}

void ImDrawList::AddQuad(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathLineTo(p4);
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddQuadFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathLineTo(p4);
    PathFillConvex(col);
}

void ImDrawList::AddTriangle(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddTriangleFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathFillConvex(col);
}

void ImDrawList::AddCircle(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || radius < 0.5f)
        return;

    if (num_segments <= 0)
    {
        // Use arc with automatic segment count
        _PathArcToFastEx(center, radius - 0.5f, 0, IM_DRAWLIST_ARCFAST_SAMPLE_MAX, 0);
        _Path.Size--;
    }
    else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp(num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX);

        // Because we are filling a closed shape we remove 1 from the count of segments/points
        const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
        PathArcTo(center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    }

    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || radius < 0.5f)
        return;

    if (num_segments <= 0)
    {
        // Use arc with automatic segment count
        _PathArcToFastEx(center, radius, 0, IM_DRAWLIST_ARCFAST_SAMPLE_MAX, 0);
        _Path.Size--;
    }
    else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp(num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX);

        // Because we are filling a closed shape we remove 1 from the count of segments/points
        const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
        PathArcTo(center, radius, 0.0f, a_max, num_segments - 1);
    }

    PathFillConvex(col);
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgon(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgonFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(center, radius, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

// Ellipse
void ImDrawList::AddEllipse(const ImVec2& center, const ImVec2& radius, ImU32 col, float rot, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (num_segments <= 0)
        num_segments = _CalcCircleAutoSegmentCount(ImMax(radius.x, radius.y)); // A bit pessimistic, maybe there's a better computation to do here.

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathEllipticalArcTo(center, radius, rot, 0.0f, a_max, num_segments - 1);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddEllipseFilled(const ImVec2& center, const ImVec2& radius, ImU32 col, float rot, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (num_segments <= 0)
        num_segments = _CalcCircleAutoSegmentCount(ImMax(radius.x, radius.y)); // A bit pessimistic, maybe there's a better computation to do here.

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathEllipticalArcTo(center, radius, rot, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

// Cubic Bezier takes 4 controls points
void ImDrawList::AddBezierCubic(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathBezierCubicCurveTo(p2, p3, p4, num_segments);
    PathStroke(col, 0, thickness);
}

// Quadratic Bezier takes 3 controls points
void ImDrawList::AddBezierQuadratic(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathBezierQuadraticCurveTo(p2, p3, num_segments);
    PathStroke(col, 0, thickness);
}

void ImDrawList::AddText(ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    // Accept null ranges
    if (text_begin == text_end || text_begin[0] == 0)
        return;
    // No need to strlen() here: font->RenderText() will do it and may early out.

    // Pull default font/size from the shared ImDrawListSharedData instance
    if (font == NULL)
        font = _Data->Font;
    if (font_size == 0.0f)
        font_size = _Data->FontSize;

    IM_ASSERT(font->ContainerAtlas->TexID == _CmdHeader.TextureId);  // Use high-level ImGui::PushFont() or low-level ImDrawList::PushTextureId() to change font.

    ImVec4 clip_rect = _CmdHeader.ClipRect;
    if (cpu_fine_clip_rect)
    {
        clip_rect.x = ImMax(clip_rect.x, cpu_fine_clip_rect->x);
        clip_rect.y = ImMax(clip_rect.y, cpu_fine_clip_rect->y);
        clip_rect.z = ImMin(clip_rect.z, cpu_fine_clip_rect->z);
        clip_rect.w = ImMin(clip_rect.w, cpu_fine_clip_rect->w);
    }
    font->RenderText(this, font_size, pos, col, clip_rect, text_begin, text_end, wrap_width, cpu_fine_clip_rect != NULL);
}

void ImDrawList::AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end)
{
    AddText(_Data->Font, _Data->FontSize, pos, col, text_begin, text_end);
}

void ImDrawList::AddImage(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = user_texture_id != _CmdHeader.TextureId;
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimRectUV(p_min, p_max, uv_min, uv_max, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageQuad(ImTextureID user_texture_id, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& uv1, const ImVec2& uv2, const ImVec2& uv3, const ImVec2& uv4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = user_texture_id != _CmdHeader.TextureId;
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimQuadUV(p1, p2, p3, p4, uv1, uv2, uv3, uv4, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageRounded(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, float rounding, ImDrawFlags flags)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    flags = FixRectCornerFlags(flags);
    if (rounding < 0.5f || (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone)
    {
        AddImage(user_texture_id, p_min, p_max, uv_min, uv_max, col);
        return;
    }

    const bool push_texture_id = user_texture_id != _CmdHeader.TextureId;
    if (push_texture_id)
        PushTextureID(user_texture_id);

    int vert_start_idx = VtxBuffer.Size;
    PathRect(p_min, p_max, rounding, flags);
    PathFillConvex(col);
    int vert_end_idx = VtxBuffer.Size;
    ImGui::ShadeVertsLinearUV(this, vert_start_idx, vert_end_idx, p_min, p_max, uv_min, uv_max, true);

    if (push_texture_id)
        PopTextureID();
}

//-----------------------------------------------------------------------------
// [SECTION] ImTriangulator, ImDrawList concave polygon fill
//-----------------------------------------------------------------------------
// Triangulate concave polygons. Based on "Triangulation by Ear Clipping" paper, O(N^2) complexity.
// Reference: https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
// Provided as a convenience for user but not used by main library.
//-----------------------------------------------------------------------------
// - ImTriangulator [Internal]
// - AddConcavePolyFilled()
//-----------------------------------------------------------------------------

enum ImTriangulatorNodeType
{
    ImTriangulatorNodeType_Convex,
    ImTriangulatorNodeType_Ear,
    ImTriangulatorNodeType_Reflex
};

struct ImTriangulatorNode
{
    ImTriangulatorNodeType  Type;
    int                     Index;
    ImVec2                  Pos;
    ImTriangulatorNode*     Next;
    ImTriangulatorNode*     Prev;

    void    Unlink()        { Next->Prev = Prev; Prev->Next = Next; }
};

struct ImTriangulatorNodeSpan
{
    ImTriangulatorNode**    Data = NULL;
    int                     Size = 0;

    void    push_back(ImTriangulatorNode* node) { Data[Size++] = node; }
    void    find_erase_unsorted(int idx)        { for (int i = Size - 1; i >= 0; i--) if (Data[i]->Index == idx) { Data[i] = Data[Size - 1]; Size--; return; } }
};

struct ImTriangulator
{
    static int EstimateTriangleCount(int points_count)      { return (points_count < 3) ? 0 : points_count - 2; }
    static int EstimateScratchBufferSize(int points_count)  { return sizeof(ImTriangulatorNode) * points_count + sizeof(ImTriangulatorNode*) * points_count * 2; }

    void    Init(const ImVec2* points, int points_count, void* scratch_buffer);
    void    GetNextTriangle(unsigned int out_triangle[3]);     // Return relative indexes for next triangle

    // Internal functions
    void    BuildNodes(const ImVec2* points, int points_count);
    void    BuildReflexes();
    void    BuildEars();
    void    FlipNodeList();
    bool    IsEar(int i0, int i1, int i2, const ImVec2& v0, const ImVec2& v1, const ImVec2& v2) const;
    void    ReclassifyNode(ImTriangulatorNode* node);

    // Internal members
    int                     _TrianglesLeft = 0;
    ImTriangulatorNode*     _Nodes = NULL;
    ImTriangulatorNodeSpan  _Ears;
    ImTriangulatorNodeSpan  _Reflexes;
};

// Distribute storage for nodes, ears and reflexes.
// FIXME-OPT: if everything is convex, we could report it to caller and let it switch to an convex renderer
// (this would require first building reflexes to bail to convex if empty, without even building nodes)
void ImTriangulator::Init(const ImVec2* points, int points_count, void* scratch_buffer)
{
    IM_ASSERT(scratch_buffer != NULL && points_count >= 3);
    _TrianglesLeft = EstimateTriangleCount(points_count);
    _Nodes         = (ImTriangulatorNode*)scratch_buffer;                          // points_count x Node
    _Ears.Data     = (ImTriangulatorNode**)(_Nodes + points_count);                // points_count x Node*
    _Reflexes.Data = (ImTriangulatorNode**)(_Nodes + points_count) + points_count; // points_count x Node*
    BuildNodes(points, points_count);
    BuildReflexes();
    BuildEars();
}

void ImTriangulator::BuildNodes(const ImVec2* points, int points_count)
{
    for (int i = 0; i < points_count; i++)
    {
        _Nodes[i].Type = ImTriangulatorNodeType_Convex;
        _Nodes[i].Index = i;
        _Nodes[i].Pos = points[i];
        _Nodes[i].Next = _Nodes + i + 1;
        _Nodes[i].Prev = _Nodes + i - 1;
    }
    _Nodes[0].Prev = _Nodes + points_count - 1;
    _Nodes[points_count - 1].Next = _Nodes;
}

void ImTriangulator::BuildReflexes()
{
    ImTriangulatorNode* n1 = _Nodes;
    for (int i = _TrianglesLeft; i >= 0; i--, n1 = n1->Next)
    {
        if (ImTriangleIsClockwise(n1->Prev->Pos, n1->Pos, n1->Next->Pos))
            continue;
        n1->Type = ImTriangulatorNodeType_Reflex;
        _Reflexes.push_back(n1);
    }
}

void ImTriangulator::BuildEars()
{
    ImTriangulatorNode* n1 = _Nodes;
    for (int i = _TrianglesLeft; i >= 0; i--, n1 = n1->Next)
    {
        if (n1->Type != ImTriangulatorNodeType_Convex)
            continue;
        if (!IsEar(n1->Prev->Index, n1->Index, n1->Next->Index, n1->Prev->Pos, n1->Pos, n1->Next->Pos))
            continue;
        n1->Type = ImTriangulatorNodeType_Ear;
        _Ears.push_back(n1);
    }
}

void ImTriangulator::GetNextTriangle(unsigned int out_triangle[3])
{
    if (_Ears.Size == 0)
    {
        FlipNodeList();

        ImTriangulatorNode* node = _Nodes;
        for (int i = _TrianglesLeft; i >= 0; i--, node = node->Next)
            node->Type = ImTriangulatorNodeType_Convex;
        _Reflexes.Size = 0;
        BuildReflexes();
        BuildEars();

        // If we still don't have ears, it means geometry is degenerated.
        if (_Ears.Size == 0)
        {
            // Return first triangle available, mimicking the behavior of convex fill.
            IM_ASSERT(_TrianglesLeft > 0); // Geometry is degenerated
            _Ears.Data[0] = _Nodes;
            _Ears.Size    = 1;
        }
    }

    ImTriangulatorNode* ear = _Ears.Data[--_Ears.Size];
    out_triangle[0] = ear->Prev->Index;
    out_triangle[1] = ear->Index;
    out_triangle[2] = ear->Next->Index;

    ear->Unlink();
    if (ear == _Nodes)
        _Nodes = ear->Next;

    ReclassifyNode(ear->Prev);
    ReclassifyNode(ear->Next);
    _TrianglesLeft--;
}

void ImTriangulator::FlipNodeList()
{
    ImTriangulatorNode* prev = _Nodes;
    ImTriangulatorNode* temp = _Nodes;
    ImTriangulatorNode* current = _Nodes->Next;
    prev->Next = prev;
    prev->Prev = prev;
    while (current != _Nodes)
    {
        temp = current->Next;

        current->Next = prev;
        prev->Prev = current;
        _Nodes->Next = current;
        current->Prev = _Nodes;

        prev = current;
        current = temp;
    }
    _Nodes = prev;
}

// A triangle is an ear is no other vertex is inside it. We can test reflexes vertices only (see reference algorithm)
bool ImTriangulator::IsEar(int i0, int i1, int i2, const ImVec2& v0, const ImVec2& v1, const ImVec2& v2) const
{
    ImTriangulatorNode** p_end = _Reflexes.Data + _Reflexes.Size;
    for (ImTriangulatorNode** p = _Reflexes.Data; p < p_end; p++)
    {
        ImTriangulatorNode* reflex = *p;
        if (reflex->Index != i0 && reflex->Index != i1 && reflex->Index != i2)
            if (ImTriangleContainsPoint(v0, v1, v2, reflex->Pos))
                return false;
    }
    return true;
}

void ImTriangulator::ReclassifyNode(ImTriangulatorNode* n1)
{
    // Classify node
    ImTriangulatorNodeType type;
    const ImTriangulatorNode* n0 = n1->Prev;
    const ImTriangulatorNode* n2 = n1->Next;
    if (!ImTriangleIsClockwise(n0->Pos, n1->Pos, n2->Pos))
        type = ImTriangulatorNodeType_Reflex;
    else if (IsEar(n0->Index, n1->Index, n2->Index, n0->Pos, n1->Pos, n2->Pos))
        type = ImTriangulatorNodeType_Ear;
    else
        type = ImTriangulatorNodeType_Convex;

    // Update lists when a type changes
    if (type == n1->Type)
        return;
    if (n1->Type == ImTriangulatorNodeType_Reflex)
        _Reflexes.find_erase_unsorted(n1->Index);
    else if (n1->Type == ImTriangulatorNodeType_Ear)
        _Ears.find_erase_unsorted(n1->Index);
    if (type == ImTriangulatorNodeType_Reflex)
        _Reflexes.push_back(n1);
    else if (type == ImTriangulatorNodeType_Ear)
        _Ears.push_back(n1);
    n1->Type = type;
}

// Use ear-clipping algorithm to triangulate a simple polygon (no self-interaction, no holes).
// (Reminder: we don't perform any coarse clipping/culling in ImDrawList layer!
// It is up to caller to ensure not making costly calls that will be outside of visible area.
// As concave fill is noticeably more expensive than other primitives, be mindful of this...
// Caller can build AABB of points, and avoid filling if 'draw_list->_CmdHeader.ClipRect.Overlays(points_bb) == false')
void ImDrawList::AddConcavePolyFilled(const ImVec2* points, const int points_count, ImU32 col)
{
    if (points_count < 3 || (col & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;
    ImTriangulator triangulator;
    unsigned int triangle[3];
    if (Flags & ImDrawListFlags_AntiAliasedFill)
    {
        // Anti-aliased Fill
        const float AA_SIZE = _FringeScale;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;
        const int idx_count = (points_count - 2) * 3 + points_count * 6;
        const int vtx_count = (points_count * 2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx + 1;

        _Data->TempBuffer.reserve_discard((ImTriangulator::EstimateScratchBufferSize(points_count) + sizeof(ImVec2)) / sizeof(ImVec2));
        triangulator.Init(points, points_count, _Data->TempBuffer.Data);
        while (triangulator._TrianglesLeft > 0)
        {
            triangulator.GetNextTriangle(triangle);
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (triangle[0] << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (triangle[1] << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx + (triangle[2] << 1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        _Data->TempBuffer.reserve_discard(points_count);
        ImVec2* temp_normals = _Data->TempBuffer.Data;
        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i0].x = dy;
            temp_normals[i0].y = -dx;
        }

        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            float dm_x = (n0.x + n1.x) * 0.5f;
            float dm_y = (n0.y + n1.y) * 0.5f;
            IM_FIXNORMAL2F(dm_x, dm_y);
            dm_x *= AA_SIZE * 0.5f;
            dm_y *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (i0 << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx + (i1 << 1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count - 2) * 3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        _Data->TempBuffer.reserve_discard((ImTriangulator::EstimateScratchBufferSize(points_count) + sizeof(ImVec2)) / sizeof(ImVec2));
        triangulator.Init(points, points_count, _Data->TempBuffer.Data);
        while (triangulator._TrianglesLeft > 0)
        {
            triangulator.GetNextTriangle(triangle);
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx + triangle[0]); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + triangle[1]); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + triangle[2]);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawListSplitter
//-----------------------------------------------------------------------------
// FIXME: This may be a little confusing, trying to be a little too low-level/optimal instead of just doing vector swap..
//-----------------------------------------------------------------------------

void ImDrawListSplitter::ClearFreeMemory()
{
    for (int i = 0; i < _Channels.Size; i++)
    {
        if (i == _Current)
            memset(&_Channels[i], 0, sizeof(_Channels[i]));  // Current channel is a copy of CmdBuffer/IdxBuffer, don't destruct again
        _Channels[i]._CmdBuffer.clear();
        _Channels[i]._IdxBuffer.clear();
    }
    _Current = 0;
    _Count = 1;
    _Channels.clear();
}

void ImDrawListSplitter::Split(ImDrawList* draw_list, int channels_count)
{
    IM_UNUSED(draw_list);
    IM_ASSERT(_Current == 0 && _Count <= 1 && "Nested channel splitting is not supported. Please use separate instances of ImDrawListSplitter.");
    int old_channels_count = _Channels.Size;
    if (old_channels_count < channels_count)
    {
        _Channels.reserve(channels_count); // Avoid over reserving since this is likely to stay stable
        _Channels.resize(channels_count);
    }
    _Count = channels_count;

    // Channels[] (24/32 bytes each) hold storage that we'll swap with draw_list->_CmdBuffer/_IdxBuffer
    // The content of Channels[0] at this point doesn't matter. We clear it to make state tidy in a debugger but we don't strictly need to.
    // When we switch to the next channel, we'll copy draw_list->_CmdBuffer/_IdxBuffer into Channels[0] and then Channels[1] into draw_list->CmdBuffer/_IdxBuffer
    memset(&_Channels[0], 0, sizeof(ImDrawChannel));
    for (int i = 1; i < channels_count; i++)
    {
        if (i >= old_channels_count)
        {
            IM_PLACEMENT_NEW(&_Channels[i]) ImDrawChannel();
        }
        else
        {
            _Channels[i]._CmdBuffer.resize(0);
            _Channels[i]._IdxBuffer.resize(0);
        }
    }
}

void ImDrawListSplitter::Merge(ImDrawList* draw_list)
{
    // Note that we never use or rely on _Channels.Size because it is merely a buffer that we never shrink back to 0 to keep all sub-buffers ready for use.
    if (_Count <= 1)
        return;

    SetCurrentChannel(draw_list, 0);
    draw_list->_PopUnusedDrawCmd();

    // Calculate our final buffer sizes. Also fix the incorrect IdxOffset values in each command.
    int new_cmd_buffer_count = 0;
    int new_idx_buffer_count = 0;
    ImDrawCmd* last_cmd = (_Count > 0 && draw_list->CmdBuffer.Size > 0) ? &draw_list->CmdBuffer.back() : NULL;
    int idx_offset = last_cmd ? last_cmd->IdxOffset + last_cmd->ElemCount : 0;
    for (int i = 1; i < _Count; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (ch._CmdBuffer.Size > 0 && ch._CmdBuffer.back().ElemCount == 0 && ch._CmdBuffer.back().UserCallback == NULL) // Equivalent of PopUnusedDrawCmd()
            ch._CmdBuffer.pop_back();

        if (ch._CmdBuffer.Size > 0 && last_cmd != NULL)
        {
            // Do not include ImDrawCmd_AreSequentialIdxOffset() in the compare as we rebuild IdxOffset values ourselves.
            // Manipulating IdxOffset (e.g. by reordering draw commands like done by RenderDimmedBackgroundBehindWindow()) is not supported within a splitter.
            ImDrawCmd* next_cmd = &ch._CmdBuffer[0];
            if (ImDrawCmd_HeaderCompare(last_cmd, next_cmd) == 0 && last_cmd->UserCallback == NULL && next_cmd->UserCallback == NULL)
            {
                // Merge previous channel last draw command with current channel first draw command if matching.
                last_cmd->ElemCount += next_cmd->ElemCount;
                idx_offset += next_cmd->ElemCount;
                ch._CmdBuffer.erase(ch._CmdBuffer.Data); // FIXME-OPT: Improve for multiple merges.
            }
        }
        if (ch._CmdBuffer.Size > 0)
            last_cmd = &ch._CmdBuffer.back();
        new_cmd_buffer_count += ch._CmdBuffer.Size;
        new_idx_buffer_count += ch._IdxBuffer.Size;
        for (int cmd_n = 0; cmd_n < ch._CmdBuffer.Size; cmd_n++)
        {
            ch._CmdBuffer.Data[cmd_n].IdxOffset = idx_offset;
            idx_offset += ch._CmdBuffer.Data[cmd_n].ElemCount;
        }
    }
    draw_list->CmdBuffer.resize(draw_list->CmdBuffer.Size + new_cmd_buffer_count);
    draw_list->IdxBuffer.resize(draw_list->IdxBuffer.Size + new_idx_buffer_count);

    // Write commands and indices in order (they are fairly small structures, we don't copy vertices only indices)
    ImDrawCmd* cmd_write = draw_list->CmdBuffer.Data + draw_list->CmdBuffer.Size - new_cmd_buffer_count;
    ImDrawIdx* idx_write = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size - new_idx_buffer_count;
    for (int i = 1; i < _Count; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (int sz = ch._CmdBuffer.Size) { memcpy(cmd_write, ch._CmdBuffer.Data, sz * sizeof(ImDrawCmd)); cmd_write += sz; }
        if (int sz = ch._IdxBuffer.Size) { memcpy(idx_write, ch._IdxBuffer.Data, sz * sizeof(ImDrawIdx)); idx_write += sz; }
    }
    draw_list->_IdxWritePtr = idx_write;

    // Ensure there's always a non-callback draw command trailing the command-buffer
    if (draw_list->CmdBuffer.Size == 0 || draw_list->CmdBuffer.back().UserCallback != NULL)
        draw_list->AddDrawCmd();

    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &draw_list->CmdBuffer.Data[draw_list->CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount == 0)
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TextureId, VtxOffset
    else if (ImDrawCmd_HeaderCompare(curr_cmd, &draw_list->_CmdHeader) != 0)
        draw_list->AddDrawCmd();

    _Count = 1;
}

void ImDrawListSplitter::SetCurrentChannel(ImDrawList* draw_list, int idx)
{
    IM_ASSERT(idx >= 0 && idx < _Count);
    if (_Current == idx)
        return;

    // Overwrite ImVector (12/16 bytes), four times. This is merely a silly optimization instead of doing .swap()
    memcpy(&_Channels.Data[_Current]._CmdBuffer, &draw_list->CmdBuffer, sizeof(draw_list->CmdBuffer));
    memcpy(&_Channels.Data[_Current]._IdxBuffer, &draw_list->IdxBuffer, sizeof(draw_list->IdxBuffer));
    _Current = idx;
    memcpy(&draw_list->CmdBuffer, &_Channels.Data[idx]._CmdBuffer, sizeof(draw_list->CmdBuffer));
    memcpy(&draw_list->IdxBuffer, &_Channels.Data[idx]._IdxBuffer, sizeof(draw_list->IdxBuffer));
    draw_list->_IdxWritePtr = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size;

    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = (draw_list->CmdBuffer.Size == 0) ? NULL : &draw_list->CmdBuffer.Data[draw_list->CmdBuffer.Size - 1];
    if (curr_cmd == NULL)
        draw_list->AddDrawCmd();
    else if (curr_cmd->ElemCount == 0)
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TextureId, VtxOffset
    else if (ImDrawCmd_HeaderCompare(curr_cmd, &draw_list->_CmdHeader) != 0)
        draw_list->AddDrawCmd();
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawData
//-----------------------------------------------------------------------------

void ImDrawData::Clear()
{
    Valid = false;
    CmdListsCount = TotalIdxCount = TotalVtxCount = 0;
    CmdLists.resize(0); // The ImDrawList are NOT owned by ImDrawData but e.g. by ImGuiContext, so we don't clear them.
    DisplayPos = DisplaySize = FramebufferScale = ImVec2(0.0f, 0.0f);
    OwnerViewport = NULL;
}

// Important: 'out_list' is generally going to be draw_data->CmdLists, but may be another temporary list
// as long at it is expected that the result will be later merged into draw_data->CmdLists[].
void ImGui::AddDrawListToDrawDataEx(ImDrawData* draw_data, ImVector<ImDrawList*>* out_list, ImDrawList* draw_list)
{
    if (draw_list->CmdBuffer.Size == 0)
        return;
    if (draw_list->CmdBuffer.Size == 1 && draw_list->CmdBuffer[0].ElemCount == 0 && draw_list->CmdBuffer[0].UserCallback == NULL)
        return;

    // Draw list sanity check. Detect mismatch between PrimReserve() calls and incrementing _VtxCurrentIdx, _VtxWritePtr etc.
    // May trigger for you if you are using PrimXXX functions incorrectly.
    IM_ASSERT(draw_list->VtxBuffer.Size == 0 || draw_list->_VtxWritePtr == draw_list->VtxBuffer.Data + draw_list->VtxBuffer.Size);
    IM_ASSERT(draw_list->IdxBuffer.Size == 0 || draw_list->_IdxWritePtr == draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size);
    if (!(draw_list->Flags & ImDrawListFlags_AllowVtxOffset))
        IM_ASSERT((int)draw_list->_VtxCurrentIdx == draw_list->VtxBuffer.Size);

    // Check that draw_list doesn't use more vertices than indexable (default ImDrawIdx = unsigned short = 2 bytes = 64K vertices per ImDrawList = per window)
    // If this assert triggers because you are drawing lots of stuff manually:
    // - First, make sure you are coarse clipping yourself and not trying to draw many things outside visible bounds.
    //   Be mindful that the lower-level ImDrawList API doesn't filter vertices. Use the Metrics/Debugger window to inspect draw list contents.
    // - If you want large meshes with more than 64K vertices, you can either:
    //   (A) Handle the ImDrawCmd::VtxOffset value in your renderer backend, and set 'io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset'.
    //       Most example backends already support this from 1.71. Pre-1.71 backends won't.
    //       Some graphics API such as GL ES 1/2 don't have a way to offset the starting vertex so it is not supported for them.
    //   (B) Or handle 32-bit indices in your renderer backend, and uncomment '#define ImDrawIdx unsigned int' line in imconfig.h.
    //       Most example backends already support this. For example, the OpenGL example code detect index size at compile-time:
    //         glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
    //       Your own engine or render API may use different parameters or function calls to specify index sizes.
    //       2 and 4 bytes indices are generally supported by most graphics API.
    // - If for some reason neither of those solutions works for you, a workaround is to call BeginChild()/EndChild() before reaching
    //   the 64K limit to split your draw commands in multiple draw lists.
    if (sizeof(ImDrawIdx) == 2)
        IM_ASSERT(draw_list->_VtxCurrentIdx < (1 << 16) && "Too many vertices in ImDrawList using 16-bit indices. Read comment above");

    // Resolve callback data pointers
    if (draw_list->_CallbacksDataBuf.Size > 0)
        for (ImDrawCmd& cmd : draw_list->CmdBuffer)
            if (cmd.UserCallback != NULL && cmd.UserCallbackDataOffset != -1 && cmd.UserCallbackDataSize > 0)
                cmd.UserCallbackData = draw_list->_CallbacksDataBuf.Data + cmd.UserCallbackDataOffset;

    // Add to output list + records state in ImDrawData
    out_list->push_back(draw_list);
    draw_data->CmdListsCount++;
    draw_data->TotalVtxCount += draw_list->VtxBuffer.Size;
    draw_data->TotalIdxCount += draw_list->IdxBuffer.Size;
}

void ImDrawData::AddDrawList(ImDrawList* draw_list)
{
    IM_ASSERT(CmdLists.Size == CmdListsCount);
    draw_list->_PopUnusedDrawCmd();
    ImGui::AddDrawListToDrawDataEx(this, &CmdLists, draw_list);
}

// For backward compatibility: convert all buffers from indexed to de-indexed, in case you cannot render indexed. Note: this is slow and most likely a waste of resources. Always prefer indexed rendering!
void ImDrawData::DeIndexAllBuffers()
{
    ImVector<ImDrawVert> new_vtx_buffer;
    TotalVtxCount = TotalIdxCount = 0;
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        if (cmd_list->IdxBuffer.empty())
            continue;
        new_vtx_buffer.resize(cmd_list->IdxBuffer.Size);
        for (int j = 0; j < cmd_list->IdxBuffer.Size; j++)
            new_vtx_buffer[j] = cmd_list->VtxBuffer[cmd_list->IdxBuffer[j]];
        cmd_list->VtxBuffer.swap(new_vtx_buffer);
        cmd_list->IdxBuffer.resize(0);
        TotalVtxCount += cmd_list->VtxBuffer.Size;
    }
}

// Helper to scale the ClipRect field of each ImDrawCmd.
// Use if your final output buffer is at a different scale than draw_data->DisplaySize,
// or if there is a difference between your window resolution and framebuffer resolution.
void ImDrawData::ScaleClipRects(const ImVec2& fb_scale)
{
    for (ImDrawList* draw_list : CmdLists)
        for (ImDrawCmd& cmd : draw_list->CmdBuffer)
            cmd.ClipRect = ImVec4(cmd.ClipRect.x * fb_scale.x, cmd.ClipRect.y * fb_scale.y, cmd.ClipRect.z * fb_scale.x, cmd.ClipRect.w * fb_scale.y);
}

//-----------------------------------------------------------------------------
// [SECTION] Helpers ShadeVertsXXX functions
//-----------------------------------------------------------------------------

// Generic linear color gradient, write to RGB fields, leave A untouched.
void ImGui::ShadeVertsLinearColorGradientKeepAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1)
{
    ImVec2 gradient_extent = gradient_p1 - gradient_p0;
    float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    const int col0_r = (int)(col0 >> IM_COL32_R_SHIFT) & 0xFF;
    const int col0_g = (int)(col0 >> IM_COL32_G_SHIFT) & 0xFF;
    const int col0_b = (int)(col0 >> IM_COL32_B_SHIFT) & 0xFF;
    const int col_delta_r = ((int)(col1 >> IM_COL32_R_SHIFT) & 0xFF) - col0_r;
    const int col_delta_g = ((int)(col1 >> IM_COL32_G_SHIFT) & 0xFF) - col0_g;
    const int col_delta_b = ((int)(col1 >> IM_COL32_B_SHIFT) & 0xFF) - col0_b;
    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        float d = ImDot(vert->pos - gradient_p0, gradient_extent);
        float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);
        int r = (int)(col0_r + col_delta_r * t);
        int g = (int)(col0_g + col_delta_g * t);
        int b = (int)(col0_b + col_delta_b * t);
        vert->col = (r << IM_COL32_R_SHIFT) | (g << IM_COL32_G_SHIFT) | (b << IM_COL32_B_SHIFT) | (vert->col & IM_COL32_A_MASK);
    }
}

// Distribute UV over (a, b) rectangle
void ImGui::ShadeVertsLinearUV(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, bool clamp)
{
    const ImVec2 size = b - a;
    const ImVec2 uv_size = uv_b - uv_a;
    const ImVec2 scale = ImVec2(
        size.x != 0.0f ? (uv_size.x / size.x) : 0.0f,
        size.y != 0.0f ? (uv_size.y / size.y) : 0.0f);

    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    if (clamp)
    {
        const ImVec2 min = ImMin(uv_a, uv_b);
        const ImVec2 max = ImMax(uv_a, uv_b);
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = ImClamp(uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale), min, max);
    }
    else
    {
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale);
    }
}

void ImGui::ShadeVertsTransformPos(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, const ImVec2& pivot_in, float cos_a, float sin_a, const ImVec2& pivot_out)
{
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
        vertex->pos = ImRotate(vertex->pos- pivot_in, cos_a, sin_a) + pivot_out;
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontConfig
//-----------------------------------------------------------------------------

ImFontConfig::ImFontConfig()
{
    memset(this, 0, sizeof(*this));
    FontDataOwnedByAtlas = true;
    OversampleH = 0; // Auto == 1 or 2 depending on size
    OversampleV = 0; // Auto == 1
    GlyphMaxAdvanceX = FLT_MAX;
    RasterizerMultiply = 1.0f;
    RasterizerDensity = 1.0f;
    EllipsisChar = 0;
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontAtlas
//-----------------------------------------------------------------------------
// - Default texture data encoded in ASCII
// - ImFontAtlas::ClearInputData()
// - ImFontAtlas::ClearTexData()
// - ImFontAtlas::ClearFonts()
// - ImFontAtlas::Clear()
// - ImFontAtlas::GetTexDataAsAlpha8()
// - ImFontAtlas::GetTexDataAsRGBA32()
// - ImFontAtlas::AddFont()
// - ImFontAtlas::AddFontDefault()
// - ImFontAtlas::AddFontFromFileTTF()
// - ImFontAtlas::AddFontFromMemoryTTF()
// - ImFontAtlas::AddFontFromMemoryCompressedTTF()
// - ImFontAtlas::AddFontFromMemoryCompressedBase85TTF()
// - ImFontAtlas::AddCustomRectRegular()
// - ImFontAtlas::AddCustomRectFontGlyph()
// - ImFontAtlas::CalcCustomRectUV()
// - ImFontAtlasGetMouseCursorTexData()
// - ImFontAtlas::Build()
// - ImFontAtlasBuildMultiplyCalcLookupTable()
// - ImFontAtlasBuildMultiplyRectAlpha8()
// - ImFontAtlasBuildWithStbTruetype()
// - ImFontAtlasGetBuilderForStbTruetype()
// - ImFontAtlasUpdateSourcesPointers()
// - ImFontAtlasBuildSetupFont()
// - ImFontAtlasBuildPackCustomRects()
// - ImFontAtlasBuildRender8bppRectFromString()
// - ImFontAtlasBuildRender32bppRectFromString()
// - ImFontAtlasBuildRenderDefaultTexData()
// - ImFontAtlasBuildRenderLinesTexData()
// - ImFontAtlasBuildInit()
// - ImFontAtlasBuildFinish()
//-----------------------------------------------------------------------------

// A work of art lies ahead! (. = white layer, X = black layer, others are blank)
// The 2x2 white texels on the top left are the ones we'll use everywhere in Dear ImGui to render filled shapes.
// (This is used when io.MouseDrawCursor = true)
const int FONT_ATLAS_DEFAULT_TEX_DATA_W = 122; // Actual texture will be 2 times that + 1 spacing.
const int FONT_ATLAS_DEFAULT_TEX_DATA_H = 27;
static const char FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[FONT_ATLAS_DEFAULT_TEX_DATA_W * FONT_ATLAS_DEFAULT_TEX_DATA_H + 1] =
{
    "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX-     XX          - XX       XX "
    "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X-    X..X         -X..X     X..X"
    "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X-    X..X         -X...X   X...X"
    "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X-    X..X         - X...X X...X "
    "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X-    X..X         -  X...X...X  "
    "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X-    X..XXX       -   X.....X   "
    "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX-    X..X..XXX    -    X...X    "
    "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      -    X..X..X..XX  -     X.X     "
    "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       -    X..X..X..X.X -    X...X    "
    "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        -XXX X..X..X..X..X-   X.....X   "
    "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         -X..XX........X..X-  X...X...X  "
    "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          -X...X...........X- X...X X...X "
    "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           - X..............X-X...X   X...X"
    "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            -  X.............X-X..X     X..X"
    "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           -  X.............X- XX       XX "
    "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          -   X............X--------------"
    "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          -   X...........X -             "
    "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       -------------------------------------    X..........X -             "
    "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           -    X..........X -             "
    "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           -     X........X  -             "
    "      X..X  -       -  X...X  -         X...X         -  X..X           X..X  -           -     X........X  -             "
    "       XX   -       -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           -     XXXXXXXXXX  -             "
    "-------------       -    X    -           X           -X.....................X-           -------------------             "
    "                    ----------------------------------- X...XXXXXXXXXXXXX...X -                                           "
    "                                                      -  X..X           X..X  -                                           "
    "                                                      -   X.X           X.X   -                                           "
    "                                                      -    XX           XX    -                                           "
};

static const ImVec2 FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[ImGuiMouseCursor_COUNT][3] =
{
    // Pos ........ Size ......... Offset ......
    { ImVec2( 0,3), ImVec2(12,19), ImVec2( 0, 0) }, // ImGuiMouseCursor_Arrow
    { ImVec2(13,0), ImVec2( 7,16), ImVec2( 1, 8) }, // ImGuiMouseCursor_TextInput
    { ImVec2(31,0), ImVec2(23,23), ImVec2(11,11) }, // ImGuiMouseCursor_ResizeAll
    { ImVec2(21,0), ImVec2( 9,23), ImVec2( 4,11) }, // ImGuiMouseCursor_ResizeNS
    { ImVec2(55,18),ImVec2(23, 9), ImVec2(11, 4) }, // ImGuiMouseCursor_ResizeEW
    { ImVec2(73,0), ImVec2(17,17), ImVec2( 8, 8) }, // ImGuiMouseCursor_ResizeNESW
    { ImVec2(55,0), ImVec2(17,17), ImVec2( 8, 8) }, // ImGuiMouseCursor_ResizeNWSE
    { ImVec2(91,0), ImVec2(17,22), ImVec2( 5, 0) }, // ImGuiMouseCursor_Hand
    { ImVec2(0,3),  ImVec2(12,19), ImVec2(0, 0) },  // ImGuiMouseCursor_Wait       // Arrow + custom code in ImGui::RenderMouseCursor()
    { ImVec2(0,3),  ImVec2(12,19), ImVec2(0, 0) },  // ImGuiMouseCursor_Progress   // Arrow + custom code in ImGui::RenderMouseCursor()
    { ImVec2(109,0),ImVec2(13,15), ImVec2( 6, 7) }, // ImGuiMouseCursor_NotAllowed
};

ImFontAtlas::ImFontAtlas()
{
    memset(this, 0, sizeof(*this));
    TexGlyphPadding = 1;
    PackIdMouseCursors = PackIdLines = -1;
}

ImFontAtlas::~ImFontAtlas()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    Clear();
}

void    ImFontAtlas::ClearInputData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    for (ImFontConfig& font_cfg : Sources)
        if (font_cfg.FontData && font_cfg.FontDataOwnedByAtlas)
        {
            IM_FREE(font_cfg.FontData);
            font_cfg.FontData = NULL;
        }

    // When clearing this we lose access to the font name and other information used to build the font.
    for (ImFont* font : Fonts)
        if (font->Sources >= Sources.Data && font->Sources < Sources.Data + Sources.Size)
        {
            font->Sources = NULL;
            font->SourcesCount = 0;
        }
    Sources.clear();
    CustomRects.clear();
    PackIdMouseCursors = PackIdLines = -1;
    // Important: we leave TexReady untouched
}

void    ImFontAtlas::ClearTexData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    if (TexPixelsAlpha8)
        IM_FREE(TexPixelsAlpha8);
    if (TexPixelsRGBA32)
        IM_FREE(TexPixelsRGBA32);
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
    TexPixelsUseColors = false;
    // Important: we leave TexReady untouched
}

void    ImFontAtlas::ClearFonts()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    ClearInputData();
    Fonts.clear_delete();
    TexReady = false;
}

void    ImFontAtlas::Clear()
{
    ClearInputData();
    ClearTexData();
    ClearFonts();
}

void    ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Build atlas on demand
    if (TexPixelsAlpha8 == NULL)
        Build();

    *out_pixels = TexPixelsAlpha8;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 1;
}

void    ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Convert to RGBA32 format on demand
    // Although it is likely to be the most commonly used format, our font rendering is 1 channel / 8 bpp
    if (!TexPixelsRGBA32)
    {
        unsigned char* pixels = NULL;
        GetTexDataAsAlpha8(&pixels, NULL, NULL);
        if (pixels)
        {
            TexPixelsRGBA32 = (unsigned int*)IM_ALLOC((size_t)TexWidth * (size_t)TexHeight * 4);
            const unsigned char* src = pixels;
            unsigned int* dst = TexPixelsRGBA32;
            for (int n = TexWidth * TexHeight; n > 0; n--)
                *dst++ = IM_COL32(255, 255, 255, (unsigned int)(*src++));
        }
    }

    *out_pixels = (unsigned char*)TexPixelsRGBA32;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 4;
}

ImFont* ImFontAtlas::AddFont(const ImFontConfig* font_cfg)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    IM_ASSERT(font_cfg->FontData != NULL && font_cfg->FontDataSize > 0);
    IM_ASSERT(font_cfg->SizePixels > 0.0f && "Is ImFontConfig struct correctly initialized?");
    IM_ASSERT(font_cfg->RasterizerDensity > 0.0f && "Is ImFontConfig struct correctly initialized?");

    // Create new font
    if (!font_cfg->MergeMode)
        Fonts.push_back(IM_NEW(ImFont));
    else
        IM_ASSERT(Fonts.Size > 0 && "Cannot use MergeMode for the first font"); // When using MergeMode make sure that a font has already been added before. You can use ImGui::GetIO().Fonts->AddFontDefault() to add the default imgui font.

    Sources.push_back(*font_cfg);
    ImFontConfig& new_font_cfg = Sources.back();
    if (new_font_cfg.DstFont == NULL)
        new_font_cfg.DstFont = Fonts.back();
    if (!new_font_cfg.FontDataOwnedByAtlas)
    {
        new_font_cfg.FontData = IM_ALLOC(new_font_cfg.FontDataSize);
        new_font_cfg.FontDataOwnedByAtlas = true;
        memcpy(new_font_cfg.FontData, font_cfg->FontData, (size_t)new_font_cfg.FontDataSize);
    }

    // Round font size
    // - We started rounding in 1.90 WIP (18991) as our layout system currently doesn't support non-rounded font size well yet.
    // - Note that using io.FontGlobalScale or SetWindowFontScale(), with are legacy-ish, partially supported features, can still lead to unrounded sizes.
    // - We may support it better later and remove this rounding.
    new_font_cfg.SizePixels = ImTrunc(new_font_cfg.SizePixels);

    // Pointers to Sources data are otherwise dangling
    ImFontAtlasUpdateSourcesPointers(this);

    // Invalidate texture
    TexReady = false;
    ClearTexData();
    return new_font_cfg.DstFont;
}

// Default font TTF is compressed with stb_compress then base85 encoded (see misc/fonts/binary_to_compressed_c.cpp for encoder)
static unsigned int stb_decompress_length(const unsigned char* input);
static unsigned int stb_decompress(unsigned char* output, const unsigned char* input, unsigned int length);
static unsigned int Decode85Byte(char c)                                    { return c >= '\\' ? c-36 : c-35; }
static void         Decode85(const unsigned char* src, unsigned char* dst)
{
    while (*src)
    {
        unsigned int tmp = Decode85Byte(src[0]) + 85 * (Decode85Byte(src[1]) + 85 * (Decode85Byte(src[2]) + 85 * (Decode85Byte(src[3]) + 85 * Decode85Byte(src[4]))));
        dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);   // We can't assume little-endianness.
        src += 5;
        dst += 4;
    }
}
#ifndef IMGUI_DISABLE_DEFAULT_FONT
static const char* GetDefaultCompressedFontDataTTF(int* out_size);
#endif

// Load embedded ProggyClean.ttf at size 13, disable oversampling
ImFont* ImFontAtlas::AddFontDefault(const ImFontConfig* font_cfg_template)
{
#ifndef IMGUI_DISABLE_DEFAULT_FONT
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (!font_cfg_template)
    {
        font_cfg.OversampleH = font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
    }
    if (font_cfg.SizePixels <= 0.0f)
        font_cfg.SizePixels = 30.0f * 1.0f;
    if (font_cfg.Name[0] == '\0')
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "ProggyClean.ttf, %dpx", (int)font_cfg.SizePixels);
    font_cfg.EllipsisChar = (ImWchar)0x0085;
    font_cfg.GlyphOffset.y = 1.0f * IM_TRUNC(font_cfg.SizePixels / 20.0f);  // Add +1 offset per 13 units

    int ttf_compressed_size = 0;
    const char* ttf_compressed = GetDefaultCompressedFontDataTTF(&ttf_compressed_size);
    const ImWchar* glyph_ranges = font_cfg.GlyphRanges != NULL ? font_cfg.GlyphRanges : GetGlyphRangesDefault();
    ImFont* font = AddFontFromMemoryCompressedTTF(ttf_compressed, ttf_compressed_size, font_cfg.SizePixels, &font_cfg, glyph_ranges);
    return font;
#else
    IM_ASSERT(0 && "AddFontDefault() disabled in this build.");
    IM_UNUSED(font_cfg_template);
    return NULL;
#endif // #ifndef IMGUI_DISABLE_DEFAULT_FONT
}

ImFont* ImFontAtlas::AddFontFromFileTTF(const char* filename, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    size_t data_size = 0;
    void* data = ImFileLoadToMemory(filename, "rb", &data_size, 0);
    if (!data)
    {
        IM_ASSERT_USER_ERROR(0, "Could not load font file!");
        return NULL;
    }
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (font_cfg.Name[0] == '\0')
    {
        // Store a short copy of filename into into the font name for convenience
        const char* p;
        for (p = filename + ImStrlen(filename); p > filename && p[-1] != '/' && p[-1] != '\\'; p--) {}
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "%s, %.0fpx", p, size_pixels);
    }
    return AddFontFromMemoryTTF(data, (int)data_size, size_pixels, &font_cfg, glyph_ranges);
}

// NB: Transfer ownership of 'ttf_data' to ImFontAtlas, unless font_cfg_template->FontDataOwnedByAtlas == false. Owned TTF buffer will be deleted after Build().
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* font_data, int font_data_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    IM_ASSERT(font_data_size > 100 && "Incorrect value for font_data_size!"); // Heuristic to prevent accidentally passing a wrong value to font_data_size.
    font_cfg.FontData = font_data;
    font_cfg.FontDataSize = font_data_size;
    font_cfg.SizePixels = size_pixels > 0.0f ? size_pixels : font_cfg.SizePixels;
    if (glyph_ranges)
        font_cfg.GlyphRanges = glyph_ranges;
    return AddFont(&font_cfg);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedTTF(const void* compressed_ttf_data, int compressed_ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    const unsigned int buf_decompressed_size = stb_decompress_length((const unsigned char*)compressed_ttf_data);
    unsigned char* buf_decompressed_data = (unsigned char*)IM_ALLOC(buf_decompressed_size);
    stb_decompress(buf_decompressed_data, (const unsigned char*)compressed_ttf_data, (unsigned int)compressed_ttf_size);

    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontDataOwnedByAtlas = true;
    return AddFontFromMemoryTTF(buf_decompressed_data, (int)buf_decompressed_size, size_pixels, &font_cfg, glyph_ranges);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedBase85TTF(const char* compressed_ttf_data_base85, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges)
{
    int compressed_ttf_size = (((int)ImStrlen(compressed_ttf_data_base85) + 4) / 5) * 4;
    void* compressed_ttf = IM_ALLOC((size_t)compressed_ttf_size);
    Decode85((const unsigned char*)compressed_ttf_data_base85, (unsigned char*)compressed_ttf);
    ImFont* font = AddFontFromMemoryCompressedTTF(compressed_ttf, compressed_ttf_size, size_pixels, font_cfg, glyph_ranges);
    IM_FREE(compressed_ttf);
    return font;
}

int ImFontAtlas::AddCustomRectRegular(int width, int height)
{
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);
    ImFontAtlasCustomRect r;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    CustomRects.push_back(r);
    return CustomRects.Size - 1; // Return index
}

int ImFontAtlas::AddCustomRectFontGlyph(ImFont* font, ImWchar id, int width, int height, float advance_x, const ImVec2& offset)
{
#ifdef IMGUI_USE_WCHAR32
    IM_ASSERT(id <= IM_UNICODE_CODEPOINT_MAX);
#endif
    IM_ASSERT(font != NULL);
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);
    ImFontAtlasCustomRect r;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    r.GlyphID = id;
    r.GlyphColored = 0; // Set to 1 manually to mark glyph as colored // FIXME: No official API for that (#8133)
    r.GlyphAdvanceX = advance_x;
    r.GlyphOffset = offset;
    r.Font = font;
    CustomRects.push_back(r);
    return CustomRects.Size - 1; // Return index
}

void ImFontAtlas::CalcCustomRectUV(const ImFontAtlasCustomRect* rect, ImVec2* out_uv_min, ImVec2* out_uv_max) const
{
    IM_ASSERT(TexWidth > 0 && TexHeight > 0);   // Font atlas needs to be built before we can calculate UV coordinates
    IM_ASSERT(rect->IsPacked());                // Make sure the rectangle has been packed
    *out_uv_min = ImVec2((float)rect->X * TexUvScale.x, (float)rect->Y * TexUvScale.y);
    *out_uv_max = ImVec2((float)(rect->X + rect->Width) * TexUvScale.x, (float)(rect->Y + rect->Height) * TexUvScale.y);
}

bool ImFontAtlasGetMouseCursorTexData(ImFontAtlas* atlas, ImGuiMouseCursor cursor_type, ImVec2* out_offset, ImVec2* out_size, ImVec2 out_uv_border[2], ImVec2 out_uv_fill[2])
{
    if (cursor_type <= ImGuiMouseCursor_None || cursor_type >= ImGuiMouseCursor_COUNT)
        return false;
    if (atlas->Flags & ImFontAtlasFlags_NoMouseCursors)
        return false;

    IM_ASSERT(atlas->PackIdMouseCursors != -1);
    ImFontAtlasCustomRect* r = atlas->GetCustomRectByIndex(atlas->PackIdMouseCursors);
    ImVec2 pos = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][0] + ImVec2((float)r->X, (float)r->Y);
    ImVec2 size = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][1];
    *out_size = size;
    *out_offset = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][2];
    out_uv_border[0] = (pos) * atlas->TexUvScale;
    out_uv_border[1] = (pos + size) * atlas->TexUvScale;
    pos.x += FONT_ATLAS_DEFAULT_TEX_DATA_W + 1;
    out_uv_fill[0] = (pos) * atlas->TexUvScale;
    out_uv_fill[1] = (pos + size) * atlas->TexUvScale;
    return true;
}

bool    ImFontAtlas::Build()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");

    // Default font is none are specified
    if (Sources.Size == 0)
        AddFontDefault();

    // Select builder
    // - Note that we do not reassign to atlas->FontBuilderIO, since it is likely to point to static data which
    //   may mess with some hot-reloading schemes. If you need to assign to this (for dynamic selection) AND are
    //   using a hot-reloading scheme that messes up static data, store your own instance of ImFontBuilderIO somewhere
    //   and point to it instead of pointing directly to return value of the GetBuilderXXX functions.
    const ImFontBuilderIO* builder_io = FontBuilderIO;
    if (builder_io == NULL)
    {
#ifdef IMGUI_ENABLE_FREETYPE
        builder_io = ImGuiFreeType::GetBuilderForFreeType();
#elif defined(IMGUI_ENABLE_STB_TRUETYPE)
        builder_io = ImFontAtlasGetBuilderForStbTruetype();
#else
        IM_ASSERT(0); // Invalid Build function
#endif
    }

    // Build
    return builder_io->FontBuilder_Build(this);
}

void    ImFontAtlasBuildMultiplyCalcLookupTable(unsigned char out_table[256], float in_brighten_factor)
{
    for (unsigned int i = 0; i < 256; i++)
    {
        unsigned int value = (unsigned int)(i * in_brighten_factor);
        out_table[i] = value > 255 ? 255 : (value & 0xFF);
    }
}

void    ImFontAtlasBuildMultiplyRectAlpha8(const unsigned char table[256], unsigned char* pixels, int x, int y, int w, int h, int stride)
{
    IM_ASSERT_PARANOID(w <= stride);
    unsigned char* data = pixels + x + y * stride;
    for (int j = h; j > 0; j--, data += stride - w)
        for (int i = w; i > 0; i--, data++)
            *data = table[*data];
}

void ImFontAtlasBuildGetOversampleFactors(const ImFontConfig* src, int* out_oversample_h, int* out_oversample_v)
{
    // Automatically disable horizontal oversampling over size 36
    *out_oversample_h = (src->OversampleH != 0) ? src->OversampleH : (src->SizePixels * src->RasterizerDensity > 36.0f || src->PixelSnapH) ? 1 : 2;
    *out_oversample_v = (src->OversampleV != 0) ? src->OversampleV : 1;
}

#ifdef IMGUI_ENABLE_STB_TRUETYPE
// Temporary data for one source font (multiple source fonts can be merged into one destination ImFont)
// (C++03 doesn't allow instancing ImVector<> with function-local types so we declare the type here.)
struct ImFontBuildSrcData
{
    stbtt_fontinfo      FontInfo;
    stbtt_pack_range    PackRange;          // Hold the list of codepoints to pack (essentially points to Codepoints.Data)
    stbrp_rect*         Rects;              // Rectangle to pack. We first fill in their size and the packer will give us their position.
    stbtt_packedchar*   PackedChars;        // Output glyphs
    const ImWchar*      SrcRanges;          // Ranges as requested by user (user is allowed to request too much, e.g. 0x0020..0xFFFF)
    int                 DstIndex;           // Index into atlas->Fonts[] and dst_tmp_array[]
    int                 GlyphsHighest;      // Highest requested codepoint
    int                 GlyphsCount;        // Glyph count (excluding missing glyphs and glyphs already set by an earlier source font)
    ImBitVector         GlyphsSet;          // Glyph bit map (random access, 1-bit per codepoint. This will be a maximum of 8KB)
    ImVector<int>       GlyphsList;         // Glyph codepoints list (flattened version of GlyphsSet)
};

// Temporary data for one destination ImFont* (multiple source fonts can be merged into one destination ImFont)
struct ImFontBuildDstData
{
    int                 SrcCount;           // Number of source fonts targeting this destination font.
    int                 GlyphsHighest;
    int                 GlyphsCount;
    ImBitVector         GlyphsSet;          // This is used to resolve collision when multiple sources are merged into a same destination font.
};

static void UnpackBitVectorToFlatIndexList(const ImBitVector* in, ImVector<int>* out)
{
    IM_ASSERT(sizeof(in->Storage.Data[0]) == sizeof(int));
    const ImU32* it_begin = in->Storage.begin();
    const ImU32* it_end = in->Storage.end();
    for (const ImU32* it = it_begin; it < it_end; it++)
        if (ImU32 entries_32 = *it)
            for (ImU32 bit_n = 0; bit_n < 32; bit_n++)
                if (entries_32 & ((ImU32)1 << bit_n))
                    out->push_back((int)(((it - it_begin) << 5) + bit_n));
}

static bool ImFontAtlasBuildWithStbTruetype(ImFontAtlas* atlas)
{
    IM_ASSERT(atlas->Sources.Size > 0);

    ImFontAtlasBuildInit(atlas);

    // Clear atlas
    atlas->TexID = (ImTextureID)NULL;
    atlas->TexWidth = atlas->TexHeight = 0;
    atlas->TexUvScale = ImVec2(0.0f, 0.0f);
    atlas->TexUvWhitePixel = ImVec2(0.0f, 0.0f);
    atlas->ClearTexData();

    // Temporary storage for building
    ImVector<ImFontBuildSrcData> src_tmp_array;
    ImVector<ImFontBuildDstData> dst_tmp_array;
    src_tmp_array.resize(atlas->Sources.Size);
    dst_tmp_array.resize(atlas->Fonts.Size);
    memset(src_tmp_array.Data, 0, (size_t)src_tmp_array.size_in_bytes());
    memset(dst_tmp_array.Data, 0, (size_t)dst_tmp_array.size_in_bytes());

    // 1. Initialize font loading structure, check font data validity
    for (int src_i = 0; src_i < atlas->Sources.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontConfig& src = atlas->Sources[src_i];
        IM_ASSERT(src.DstFont && (!src.DstFont->IsLoaded() || src.DstFont->ContainerAtlas == atlas));

        // Find index from src.DstFont (we allow the user to set cfg.DstFont. Also it makes casual debugging nicer than when storing indices)
        src_tmp.DstIndex = -1;
        for (int output_i = 0; output_i < atlas->Fonts.Size && src_tmp.DstIndex == -1; output_i++)
            if (src.DstFont == atlas->Fonts[output_i])
                src_tmp.DstIndex = output_i;
        if (src_tmp.DstIndex == -1)
        {
            IM_ASSERT(src_tmp.DstIndex != -1); // src.DstFont not pointing within atlas->Fonts[] array?
            return false;
        }
        // Initialize helper structure for font loading and verify that the TTF/OTF data is correct
        const int font_offset = stbtt_GetFontOffsetForIndex((unsigned char*)src.FontData, src.FontNo);
        IM_ASSERT(font_offset >= 0 && "FontData is incorrect, or FontNo cannot be found.");
        if (!stbtt_InitFont(&src_tmp.FontInfo, (unsigned char*)src.FontData, font_offset))
        {
            IM_ASSERT(0 && "stbtt_InitFont(): failed to parse FontData. It is correct and complete? Check FontDataSize.");
            return false;
        }

        // Measure highest codepoints
        ImFontBuildDstData& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.SrcRanges = src.GlyphRanges ? src.GlyphRanges : atlas->GetGlyphRangesDefault();
        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
        {
            // Check for valid range. This may also help detect *some* dangling pointers, because a common
            // user error is to setup ImFontConfig::GlyphRanges with a pointer to data that isn't persistent,
            // or to forget to zero-terminate the glyph range array.
            IM_ASSERT(src_range[0] <= src_range[1] && "Invalid range: is your glyph range array persistent? it is zero-terminated?");
            src_tmp.GlyphsHighest = ImMax(src_tmp.GlyphsHighest, (int)src_range[1]);
        }
        dst_tmp.SrcCount++;
        dst_tmp.GlyphsHighest = ImMax(dst_tmp.GlyphsHighest, src_tmp.GlyphsHighest);
    }

    // 2. For every requested codepoint, check for their presence in the font data, and handle redundancy or overlaps between source fonts to avoid unused glyphs.
    int total_glyphs_count = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontBuildDstData& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.GlyphsSet.Create(src_tmp.GlyphsHighest + 1);
        if (dst_tmp.GlyphsSet.Storage.empty())
            dst_tmp.GlyphsSet.Create(dst_tmp.GlyphsHighest + 1);

        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
            for (unsigned int codepoint = src_range[0]; codepoint <= src_range[1]; codepoint++)
            {
                if (dst_tmp.GlyphsSet.TestBit(codepoint))    // Don't overwrite existing glyphs. We could make this an option for MergeMode (e.g. MergeOverwrite==true)
                    continue;
                if (!stbtt_FindGlyphIndex(&src_tmp.FontInfo, codepoint))    // It is actually in the font?
                    continue;

                // Add to avail set/counters
                src_tmp.GlyphsCount++;
                dst_tmp.GlyphsCount++;
                src_tmp.GlyphsSet.SetBit(codepoint);
                dst_tmp.GlyphsSet.SetBit(codepoint);
                total_glyphs_count++;
            }
    }

    // 3. Unpack our bit map into a flat list (we now have all the Unicode points that we know are requested _and_ available _and_ not overlapping another)
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        src_tmp.GlyphsList.reserve(src_tmp.GlyphsCount);
        UnpackBitVectorToFlatIndexList(&src_tmp.GlyphsSet, &src_tmp.GlyphsList);
        src_tmp.GlyphsSet.Clear();
        IM_ASSERT(src_tmp.GlyphsList.Size == src_tmp.GlyphsCount);
    }
    for (int dst_i = 0; dst_i < dst_tmp_array.Size; dst_i++)
        dst_tmp_array[dst_i].GlyphsSet.Clear();
    dst_tmp_array.clear();

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    // (We technically don't need to zero-clear buf_rects, but let's do it for the sake of sanity)
    ImVector<stbrp_rect> buf_rects;
    ImVector<stbtt_packedchar> buf_packedchars;
    buf_rects.resize(total_glyphs_count);
    buf_packedchars.resize(total_glyphs_count);
    memset(buf_rects.Data, 0, (size_t)buf_rects.size_in_bytes());
    memset(buf_packedchars.Data, 0, (size_t)buf_packedchars.size_in_bytes());

    // 4. Gather glyphs sizes so we can pack them in our virtual canvas.
    int total_surface = 0;
    int buf_rects_out_n = 0;
    int buf_packedchars_out_n = 0;
    const int pack_padding = atlas->TexGlyphPadding;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        src_tmp.Rects = &buf_rects[buf_rects_out_n];
        src_tmp.PackedChars = &buf_packedchars[buf_packedchars_out_n];
        buf_rects_out_n += src_tmp.GlyphsCount;
        buf_packedchars_out_n += src_tmp.GlyphsCount;

        // Automatic selection of oversampling parameters
        ImFontConfig& src = atlas->Sources[src_i];
        int oversample_h, oversample_v;
        ImFontAtlasBuildGetOversampleFactors(&src, &oversample_h, &oversample_v);

        // Convert our ranges in the format stb_truetype wants
        src_tmp.PackRange.font_size = src.SizePixels * src.RasterizerDensity;
        src_tmp.PackRange.first_unicode_codepoint_in_range = 0;
        src_tmp.PackRange.array_of_unicode_codepoints = src_tmp.GlyphsList.Data;
        src_tmp.PackRange.num_chars = src_tmp.GlyphsList.Size;
        src_tmp.PackRange.chardata_for_range = src_tmp.PackedChars;
        src_tmp.PackRange.h_oversample = (unsigned char)oversample_h;
        src_tmp.PackRange.v_oversample = (unsigned char)oversample_v;

        // Gather the sizes of all rectangles we will need to pack (this loop is based on stbtt_PackFontRangesGatherRects)
        const float scale = (src.SizePixels > 0.0f) ? stbtt_ScaleForPixelHeight(&src_tmp.FontInfo, src.SizePixels * src.RasterizerDensity) : stbtt_ScaleForMappingEmToPixels(&src_tmp.FontInfo, -src.SizePixels * src.RasterizerDensity);
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsList.Size; glyph_i++)
        {
            int x0, y0, x1, y1;
            const int glyph_index_in_font = stbtt_FindGlyphIndex(&src_tmp.FontInfo, src_tmp.GlyphsList[glyph_i]);
            IM_ASSERT(glyph_index_in_font != 0);
            stbtt_GetGlyphBitmapBoxSubpixel(&src_tmp.FontInfo, glyph_index_in_font, scale * oversample_h, scale * oversample_v, 0, 0, &x0, &y0, &x1, &y1);
            src_tmp.Rects[glyph_i].w = (stbrp_coord)(x1 - x0 + pack_padding + oversample_h - 1);
            src_tmp.Rects[glyph_i].h = (stbrp_coord)(y1 - y0 + pack_padding + oversample_v - 1);
            total_surface += src_tmp.Rects[glyph_i].w * src_tmp.Rects[glyph_i].h;
        }
    }
    for (int i = 0; i < atlas->CustomRects.Size; i++)
        total_surface += (atlas->CustomRects[i].Width + pack_padding) * (atlas->CustomRects[i].Height + pack_padding);

    // We need a width for the skyline algorithm, any width!
    // The exact width doesn't really matter much, but some API/GPU have texture size limitations and increasing width can decrease height.
    // User can override TexDesiredWidth and TexGlyphPadding if they wish, otherwise we use a simple heuristic to select the width based on expected surface.
    const int surface_sqrt = (int)ImSqrt((float)total_surface) + 1;
    atlas->TexHeight = 0;
    if (atlas->TexDesiredWidth > 0)
        atlas->TexWidth = atlas->TexDesiredWidth;
    else
        atlas->TexWidth = (surface_sqrt >= 4096 * 0.7f) ? 4096 : (surface_sqrt >= 2048 * 0.7f) ? 2048 : (surface_sqrt >= 1024 * 0.7f) ? 1024 : 512;

    // 5. Start packing
    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    const int TEX_HEIGHT_MAX = 1024 * 32;
    stbtt_pack_context spc = {};
    stbtt_PackBegin(&spc, NULL, atlas->TexWidth, TEX_HEIGHT_MAX, 0, 0, NULL);
    spc.padding = atlas->TexGlyphPadding; // Because we mixup stbtt_PackXXX and stbrp_PackXXX there's a bit of a hack here, not passing the value to stbtt_PackBegin() allows us to still pack a TexWidth-1 wide item. (#8107)
    ImFontAtlasBuildPackCustomRects(atlas, spc.pack_info);

    // 6. Pack each source font. No rendering yet, we are working with rectangles in an infinitely tall texture at this point.
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbrp_pack_rects((stbrp_context*)spc.pack_info, src_tmp.Rects, src_tmp.GlyphsCount);

        // Extend texture height and mark missing glyphs as non-packed so we won't render them.
        // FIXME: We are not handling packing failure here (would happen if we got off TEX_HEIGHT_MAX or if a single if larger than TexWidth?)
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
            if (src_tmp.Rects[glyph_i].was_packed)
                atlas->TexHeight = ImMax(atlas->TexHeight, src_tmp.Rects[glyph_i].y + src_tmp.Rects[glyph_i].h);
    }

    // 7. Allocate texture
    atlas->TexHeight = (atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) ? (atlas->TexHeight + 1) : ImUpperPowerOfTwo(atlas->TexHeight);
    atlas->TexUvScale = ImVec2(1.0f / atlas->TexWidth, 1.0f / atlas->TexHeight);
    atlas->TexPixelsAlpha8 = (unsigned char*)IM_ALLOC(atlas->TexWidth * atlas->TexHeight);
    memset(atlas->TexPixelsAlpha8, 0, atlas->TexWidth * atlas->TexHeight);
    spc.pixels = atlas->TexPixelsAlpha8;
    spc.height = atlas->TexHeight;

    // 8. Render/rasterize font characters into the texture
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontConfig& src = atlas->Sources[src_i];
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbtt_PackFontRangesRenderIntoRects(&spc, &src_tmp.FontInfo, &src_tmp.PackRange, 1, src_tmp.Rects);

        // Apply multiply operator
        if (src.RasterizerMultiply != 1.0f)
        {
            unsigned char multiply_table[256];
            ImFontAtlasBuildMultiplyCalcLookupTable(multiply_table, src.RasterizerMultiply);
            stbrp_rect* r = &src_tmp.Rects[0];
            for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++, r++)
                if (r->was_packed)
                    ImFontAtlasBuildMultiplyRectAlpha8(multiply_table, atlas->TexPixelsAlpha8, r->x, r->y, r->w, r->h, atlas->TexWidth * 1);
        }
        src_tmp.Rects = NULL;
    }

    // End packing
    stbtt_PackEnd(&spc);
    buf_rects.clear();

    // 9. Setup ImFont and glyphs for runtime
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        // When merging fonts with MergeMode=true:
        // - We can have multiple input fonts writing into a same destination font.
        // - dst_font->Sources is != from src which is our source configuration.
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontConfig& src = atlas->Sources[src_i];
        ImFont* dst_font = src.DstFont;

        const float font_scale = stbtt_ScaleForPixelHeight(&src_tmp.FontInfo, src.SizePixels);
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&src_tmp.FontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

        const float ascent = ImCeil(unscaled_ascent * font_scale);
        const float descent = ImFloor(unscaled_descent * font_scale);
        ImFontAtlasBuildSetupFont(atlas, dst_font, &src, ascent, descent);
        const float font_off_x = src.GlyphOffset.x;
        const float font_off_y = src.GlyphOffset.y + IM_ROUND(dst_font->Ascent);

        const float inv_rasterization_scale = 1.0f / src.RasterizerDensity;

        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
        {
            // Register glyph
            const int codepoint = src_tmp.GlyphsList[glyph_i];
            const stbtt_packedchar& pc = src_tmp.PackedChars[glyph_i];
            stbtt_aligned_quad q;
            float unused_x = 0.0f, unused_y = 0.0f;
            stbtt_GetPackedQuad(src_tmp.PackedChars, atlas->TexWidth, atlas->TexHeight, glyph_i, &unused_x, &unused_y, &q, 0);
            float x0 = q.x0 * inv_rasterization_scale + font_off_x;
            float y0 = q.y0 * inv_rasterization_scale + font_off_y;
            float x1 = q.x1 * inv_rasterization_scale + font_off_x;
            float y1 = q.y1 * inv_rasterization_scale + font_off_y;
            dst_font->AddGlyph(&src, (ImWchar)codepoint, x0, y0, x1, y1, q.s0, q.t0, q.s1, q.t1, pc.xadvance * inv_rasterization_scale);
        }
    }

    // Cleanup
    src_tmp_array.clear_destruct();

    ImFontAtlasBuildFinish(atlas);
    return true;
}

const ImFontBuilderIO* ImFontAtlasGetBuilderForStbTruetype()
{
    static ImFontBuilderIO io;
    io.FontBuilder_Build = ImFontAtlasBuildWithStbTruetype;
    return &io;
}

#endif // IMGUI_ENABLE_STB_TRUETYPE

void ImFontAtlasUpdateSourcesPointers(ImFontAtlas* atlas)
{
    for (ImFontConfig& src : atlas->Sources)
    {
        ImFont* font = src.DstFont;
        if (!src.MergeMode)
        {
            font->Sources = &src;
            font->SourcesCount = 0;
        }
        font->SourcesCount++;
    }
}

void ImFontAtlasBuildSetupFont(ImFontAtlas* atlas, ImFont* font, ImFontConfig* font_config, float ascent, float descent)
{
    if (!font_config->MergeMode)
    {
        font->ClearOutputData();
        font->FontSize = font_config->SizePixels;
        IM_ASSERT(font->Sources == font_config);
        font->ContainerAtlas = atlas;
        font->Ascent = ascent;
        font->Descent = descent;
    }
}

void ImFontAtlasBuildPackCustomRects(ImFontAtlas* atlas, void* stbrp_context_opaque)
{
    stbrp_context* pack_context = (stbrp_context*)stbrp_context_opaque;
    IM_ASSERT(pack_context != NULL);

    ImVector<ImFontAtlasCustomRect>& user_rects = atlas->CustomRects;
    IM_ASSERT(user_rects.Size >= 1); // We expect at least the default custom rects to be registered, else something went wrong.
#ifdef __GNUC__
    if (user_rects.Size < 1) { __builtin_unreachable(); } // Workaround for GCC bug if IM_ASSERT() is defined to conditionally throw (see #5343)
#endif

    const int pack_padding = atlas->TexGlyphPadding;
    ImVector<stbrp_rect> pack_rects;
    pack_rects.resize(user_rects.Size);
    memset(pack_rects.Data, 0, (size_t)pack_rects.size_in_bytes());
    for (int i = 0; i < user_rects.Size; i++)
    {
        pack_rects[i].w = user_rects[i].Width + pack_padding;
        pack_rects[i].h = user_rects[i].Height + pack_padding;
    }
    stbrp_pack_rects(pack_context, &pack_rects[0], pack_rects.Size);
    for (int i = 0; i < pack_rects.Size; i++)
        if (pack_rects[i].was_packed)
        {
            user_rects[i].X = (unsigned short)pack_rects[i].x;
            user_rects[i].Y = (unsigned short)pack_rects[i].y;
            IM_ASSERT(pack_rects[i].w == user_rects[i].Width + pack_padding && pack_rects[i].h == user_rects[i].Height + pack_padding);
            atlas->TexHeight = ImMax(atlas->TexHeight, pack_rects[i].y + pack_rects[i].h);
        }
}

void ImFontAtlasBuildRender8bppRectFromString(ImFontAtlas* atlas, int x, int y, int w, int h, const char* in_str, char in_marker_char, unsigned char in_marker_pixel_value)
{
    IM_ASSERT(x >= 0 && x + w <= atlas->TexWidth);
    IM_ASSERT(y >= 0 && y + h <= atlas->TexHeight);
    unsigned char* out_pixel = atlas->TexPixelsAlpha8 + x + (y * atlas->TexWidth);
    for (int off_y = 0; off_y < h; off_y++, out_pixel += atlas->TexWidth, in_str += w)
        for (int off_x = 0; off_x < w; off_x++)
            out_pixel[off_x] = (in_str[off_x] == in_marker_char) ? in_marker_pixel_value : 0x00;
}

void ImFontAtlasBuildRender32bppRectFromString(ImFontAtlas* atlas, int x, int y, int w, int h, const char* in_str, char in_marker_char, unsigned int in_marker_pixel_value)
{
    IM_ASSERT(x >= 0 && x + w <= atlas->TexWidth);
    IM_ASSERT(y >= 0 && y + h <= atlas->TexHeight);
    unsigned int* out_pixel = atlas->TexPixelsRGBA32 + x + (y * atlas->TexWidth);
    for (int off_y = 0; off_y < h; off_y++, out_pixel += atlas->TexWidth, in_str += w)
        for (int off_x = 0; off_x < w; off_x++)
            out_pixel[off_x] = (in_str[off_x] == in_marker_char) ? in_marker_pixel_value : IM_COL32_BLACK_TRANS;
}

static void ImFontAtlasBuildRenderDefaultTexData(ImFontAtlas* atlas)
{
    ImFontAtlasCustomRect* r = atlas->GetCustomRectByIndex(atlas->PackIdMouseCursors);
    IM_ASSERT(r->IsPacked());

    const int w = atlas->TexWidth;
    if (atlas->Flags & ImFontAtlasFlags_NoMouseCursors)
    {
        // White pixels only
        IM_ASSERT(r->Width == 2 && r->Height == 2);
        const int offset = (int)r->X + (int)r->Y * w;
        if (atlas->TexPixelsAlpha8 != NULL)
        {
            atlas->TexPixelsAlpha8[offset] = atlas->TexPixelsAlpha8[offset + 1] = atlas->TexPixelsAlpha8[offset + w] = atlas->TexPixelsAlpha8[offset + w + 1] = 0xFF;
        }
        else
        {
            atlas->TexPixelsRGBA32[offset] = atlas->TexPixelsRGBA32[offset + 1] = atlas->TexPixelsRGBA32[offset + w] = atlas->TexPixelsRGBA32[offset + w + 1] = IM_COL32_WHITE;
        }
    }
    else
    {
        // White pixels and mouse cursor
        IM_ASSERT(r->Width == FONT_ATLAS_DEFAULT_TEX_DATA_W * 2 + 1 && r->Height == FONT_ATLAS_DEFAULT_TEX_DATA_H);
        const int x_for_white = r->X;
        const int x_for_black = r->X + FONT_ATLAS_DEFAULT_TEX_DATA_W + 1;
        if (atlas->TexPixelsAlpha8 != NULL)
        {
            ImFontAtlasBuildRender8bppRectFromString(atlas, x_for_white, r->Y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, '.', 0xFF);
            ImFontAtlasBuildRender8bppRectFromString(atlas, x_for_black, r->Y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, 'X', 0xFF);
        }
        else
        {
            ImFontAtlasBuildRender32bppRectFromString(atlas, x_for_white, r->Y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, '.', IM_COL32_WHITE);
            ImFontAtlasBuildRender32bppRectFromString(atlas, x_for_black, r->Y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, 'X', IM_COL32_WHITE);
        }
    }
    atlas->TexUvWhitePixel = ImVec2((r->X + 0.5f) * atlas->TexUvScale.x, (r->Y + 0.5f) * atlas->TexUvScale.y);
}

static void ImFontAtlasBuildRenderLinesTexData(ImFontAtlas* atlas)
{
    if (atlas->Flags & ImFontAtlasFlags_NoBakedLines)
        return;

    // This generates a triangular shape in the texture, with the various line widths stacked on top of each other to allow interpolation between them
    ImFontAtlasCustomRect* r = atlas->GetCustomRectByIndex(atlas->PackIdLines);
    IM_ASSERT(r->IsPacked());
    for (int n = 0; n < IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 1; n++) // +1 because of the zero-width row
    {
        // Each line consists of at least two empty pixels at the ends, with a line of solid pixels in the middle
        int y = n;
        int line_width = n;
        int pad_left = (r->Width - line_width) / 2;
        int pad_right = r->Width - (pad_left + line_width);

        // Write each slice
        IM_ASSERT(pad_left + line_width + pad_right == r->Width && y < r->Height); // Make sure we're inside the texture bounds before we start writing pixels
        if (atlas->TexPixelsAlpha8 != NULL)
        {
            unsigned char* write_ptr = &atlas->TexPixelsAlpha8[r->X + ((r->Y + y) * atlas->TexWidth)];
            for (int i = 0; i < pad_left; i++)
                *(write_ptr + i) = 0x00;

            for (int i = 0; i < line_width; i++)
                *(write_ptr + pad_left + i) = 0xFF;

            for (int i = 0; i < pad_right; i++)
                *(write_ptr + pad_left + line_width + i) = 0x00;
        }
        else
        {
            unsigned int* write_ptr = &atlas->TexPixelsRGBA32[r->X + ((r->Y + y) * atlas->TexWidth)];
            for (int i = 0; i < pad_left; i++)
                *(write_ptr + i) = IM_COL32(255, 255, 255, 0);

            for (int i = 0; i < line_width; i++)
                *(write_ptr + pad_left + i) = IM_COL32_WHITE;

            for (int i = 0; i < pad_right; i++)
                *(write_ptr + pad_left + line_width + i) = IM_COL32(255, 255, 255, 0);
        }

        // Calculate UVs for this line
        ImVec2 uv0 = ImVec2((float)(r->X + pad_left - 1), (float)(r->Y + y)) * atlas->TexUvScale;
        ImVec2 uv1 = ImVec2((float)(r->X + pad_left + line_width + 1), (float)(r->Y + y + 1)) * atlas->TexUvScale;
        float half_v = (uv0.y + uv1.y) * 0.5f; // Calculate a constant V in the middle of the row to avoid sampling artifacts
        atlas->TexUvLines[n] = ImVec4(uv0.x, half_v, uv1.x, half_v);
    }
}

// Note: this is called / shared by both the stb_truetype and the FreeType builder
void ImFontAtlasBuildInit(ImFontAtlas* atlas)
{
    // Register texture region for mouse cursors or standard white pixels
    if (atlas->PackIdMouseCursors < 0)
    {
        if (!(atlas->Flags & ImFontAtlasFlags_NoMouseCursors))
            atlas->PackIdMouseCursors = atlas->AddCustomRectRegular(FONT_ATLAS_DEFAULT_TEX_DATA_W * 2 + 1, FONT_ATLAS_DEFAULT_TEX_DATA_H);
        else
            atlas->PackIdMouseCursors = atlas->AddCustomRectRegular(2, 2);
    }

    // Register texture region for thick lines
    // The +2 here is to give space for the end caps, whilst height +1 is to accommodate the fact we have a zero-width row
    if (atlas->PackIdLines < 0)
    {
        if (!(atlas->Flags & ImFontAtlasFlags_NoBakedLines))
            atlas->PackIdLines = atlas->AddCustomRectRegular(IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 2, IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 1);
    }
}

// This is called/shared by both the stb_truetype and the FreeType builder.
void ImFontAtlasBuildFinish(ImFontAtlas* atlas)
{
    // Render into our custom data blocks
    IM_ASSERT(atlas->TexPixelsAlpha8 != NULL || atlas->TexPixelsRGBA32 != NULL);
    ImFontAtlasBuildRenderDefaultTexData(atlas);
    ImFontAtlasBuildRenderLinesTexData(atlas);

    // Register custom rectangle glyphs
    for (int i = 0; i < atlas->CustomRects.Size; i++)
    {
        const ImFontAtlasCustomRect* r = &atlas->CustomRects[i];
        if (r->Font == NULL || r->GlyphID == 0)
            continue;

        // Will ignore ImFontConfig settings: GlyphMinAdvanceX, GlyphMinAdvanceY, PixelSnapH
        IM_ASSERT(r->Font->ContainerAtlas == atlas);
        ImVec2 uv0, uv1;
        atlas->CalcCustomRectUV(r, &uv0, &uv1);
        r->Font->AddGlyph(NULL, (ImWchar)r->GlyphID, r->GlyphOffset.x, r->GlyphOffset.y, r->GlyphOffset.x + r->Width, r->GlyphOffset.y + r->Height, uv0.x, uv0.y, uv1.x, uv1.y, r->GlyphAdvanceX);
        if (r->GlyphColored)
            r->Font->Glyphs.back().Colored = 1;
    }

    // Build all fonts lookup tables
    for (ImFont* font : atlas->Fonts)
        if (font->DirtyLookupTables)
            font->BuildLookupTable();

    atlas->TexReady = true;
}

//-------------------------------------------------------------------------
// [SECTION] ImFontAtlas: glyph ranges helpers
//-------------------------------------------------------------------------
// - GetGlyphRangesDefault()
// - GetGlyphRangesGreek()
// - GetGlyphRangesKorean()
// - GetGlyphRangesChineseFull()
// - GetGlyphRangesChineseSimplifiedCommon()
// - GetGlyphRangesJapanese()
// - GetGlyphRangesCyrillic()
// - GetGlyphRangesThai()
// - GetGlyphRangesVietnamese()
//-----------------------------------------------------------------------------

// Retrieve list of range (2 int per range, values are inclusive)
const ImWchar*   ImFontAtlas::GetGlyphRangesDefault()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    return &ranges[0];
}

const ImWchar*   ImFontAtlas::GetGlyphRangesGreek()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0370, 0x03FF, // Greek and Coptic
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesKorean()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3131, 0x3163, // Korean alphabets
        0xAC00, 0xD7A3, // Korean characters
        0xFFFD, 0xFFFD, // Invalid
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesChineseFull()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0xFFFD, 0xFFFD, // Invalid
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

static void UnpackAccumulativeOffsetsIntoRanges(int base_codepoint, const short* accumulative_offsets, int accumulative_offsets_count, ImWchar* out_ranges)
{
    for (int n = 0; n < accumulative_offsets_count; n++, out_ranges += 2)
    {
        out_ranges[0] = out_ranges[1] = (ImWchar)(base_codepoint + accumulative_offsets[n]);
        base_codepoint += accumulative_offsets[n];
    }
    out_ranges[0] = 0;
}

const ImWchar*  ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon()
{
    // Store 2500 regularly used characters for Simplified Chinese.
    // Sourced from https://zh.wiktionary.org/wiki/%E9%99%84%E5%BD%95:%E7%8E%B0%E4%BB%A3%E6%B1%89%E8%AF%AD%E5%B8%B8%E7%94%A8%E5%AD%97%E8%A1%A8
    // This table covers 97.97% of all characters used during the month in July, 1987.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,3,2,1,2,2,1,1,1,1,1,5,2,1,2,3,3,3,2,2,4,1,1,1,2,1,5,2,3,1,2,1,2,1,1,2,1,1,2,2,1,4,1,1,1,1,5,10,1,2,19,2,1,2,1,2,1,2,1,2,
        1,5,1,6,3,2,1,2,2,1,1,1,4,8,5,1,1,4,1,1,3,1,2,1,5,1,2,1,1,1,10,1,1,5,2,4,6,1,4,2,2,2,12,2,1,1,6,1,1,1,4,1,1,4,6,5,1,4,2,2,4,10,7,1,1,4,2,4,
        2,1,4,3,6,10,12,5,7,2,14,2,9,1,1,6,7,10,4,7,13,1,5,4,8,4,1,1,2,28,5,6,1,1,5,2,5,20,2,2,9,8,11,2,9,17,1,8,6,8,27,4,6,9,20,11,27,6,68,2,2,1,1,
        1,2,1,2,2,7,6,11,3,3,1,1,3,1,2,1,1,1,1,1,3,1,1,8,3,4,1,5,7,2,1,4,4,8,4,2,1,2,1,1,4,5,6,3,6,2,12,3,1,3,9,2,4,3,4,1,5,3,3,1,3,7,1,5,1,1,1,1,2,
        3,4,5,2,3,2,6,1,1,2,1,7,1,7,3,4,5,15,2,2,1,5,3,22,19,2,1,1,1,1,2,5,1,1,1,6,1,1,12,8,2,9,18,22,4,1,1,5,1,16,1,2,7,10,15,1,1,6,2,4,1,2,4,1,6,
        1,1,3,2,4,1,6,4,5,1,2,1,1,2,1,10,3,1,3,2,1,9,3,2,5,7,2,19,4,3,6,1,1,1,1,1,4,3,2,1,1,1,2,5,3,1,1,1,2,2,1,1,2,1,1,2,1,3,1,1,1,3,7,1,4,1,1,2,1,
        1,2,1,2,4,4,3,8,1,1,1,2,1,3,5,1,3,1,3,4,6,2,2,14,4,6,6,11,9,1,15,3,1,28,5,2,5,5,3,1,3,4,5,4,6,14,3,2,3,5,21,2,7,20,10,1,2,19,2,4,28,28,2,3,
        2,1,14,4,1,26,28,42,12,40,3,52,79,5,14,17,3,2,2,11,3,4,6,3,1,8,2,23,4,5,8,10,4,2,7,3,5,1,1,6,3,1,2,2,2,5,28,1,1,7,7,20,5,3,29,3,17,26,1,8,4,
        27,3,6,11,23,5,3,4,6,13,24,16,6,5,10,25,35,7,3,2,3,3,14,3,6,2,6,1,4,2,3,8,2,1,1,3,3,3,4,1,1,13,2,2,4,5,2,1,14,14,1,2,2,1,4,5,2,3,1,14,3,12,
        3,17,2,16,5,1,2,1,8,9,3,19,4,2,2,4,17,25,21,20,28,75,1,10,29,103,4,1,2,1,1,4,2,4,1,2,3,24,2,2,2,1,1,2,1,3,8,1,1,1,2,1,1,3,1,1,1,6,1,5,3,1,1,
        1,3,4,1,1,5,2,1,5,6,13,9,16,1,1,1,1,3,2,3,2,4,5,2,5,2,2,3,7,13,7,2,2,1,1,1,1,2,3,3,2,1,6,4,9,2,1,14,2,14,2,1,18,3,4,14,4,11,41,15,23,15,23,
        176,1,3,4,1,1,1,1,5,3,1,2,3,7,3,1,1,2,1,2,4,4,6,2,4,1,9,7,1,10,5,8,16,29,1,1,2,2,3,1,3,5,2,4,5,4,1,1,2,2,3,3,7,1,6,10,1,17,1,44,4,6,2,1,1,6,
        5,4,2,10,1,6,9,2,8,1,24,1,2,13,7,8,8,2,1,4,1,3,1,3,3,5,2,5,10,9,4,9,12,2,1,6,1,10,1,1,7,7,4,10,8,3,1,13,4,3,1,6,1,3,5,2,1,2,17,16,5,2,16,6,
        1,4,2,1,3,3,6,8,5,11,11,1,3,3,2,4,6,10,9,5,7,4,7,4,7,1,1,4,2,1,3,6,8,7,1,6,11,5,5,3,24,9,4,2,7,13,5,1,8,82,16,61,1,1,1,4,2,2,16,10,3,8,1,1,
        6,4,2,1,3,1,1,1,4,3,8,4,2,2,1,1,1,1,1,6,3,5,1,1,4,6,9,2,1,1,1,2,1,7,2,1,6,1,5,4,4,3,1,8,1,3,3,1,3,2,2,2,2,3,1,6,1,2,1,2,1,3,7,1,8,2,1,2,1,5,
        2,5,3,5,10,1,2,1,1,3,2,5,11,3,9,3,5,1,1,5,9,1,2,1,5,7,9,9,8,1,3,3,3,6,8,2,3,2,1,1,32,6,1,2,15,9,3,7,13,1,3,10,13,2,14,1,13,10,2,1,3,10,4,15,
        2,15,15,10,1,3,9,6,9,32,25,26,47,7,3,2,3,1,6,3,4,3,2,8,5,4,1,9,4,2,2,19,10,6,2,3,8,1,2,2,4,2,1,9,4,4,4,6,4,8,9,2,3,1,1,1,1,3,5,5,1,3,8,4,6,
        2,1,4,12,1,5,3,7,13,2,5,8,1,6,1,2,5,14,6,1,5,2,4,8,15,5,1,23,6,62,2,10,1,1,8,1,2,2,10,4,2,2,9,2,1,1,3,2,3,1,5,3,3,2,1,3,8,1,1,1,11,3,1,1,4,
        3,7,1,14,1,2,3,12,5,2,5,1,6,7,5,7,14,11,1,3,1,8,9,12,2,1,11,8,4,4,2,6,10,9,13,1,1,3,1,5,1,3,2,4,4,1,18,2,3,14,11,4,29,4,2,7,1,3,13,9,2,2,5,
        3,5,20,7,16,8,5,72,34,6,4,22,12,12,28,45,36,9,7,39,9,191,1,1,1,4,11,8,4,9,2,3,22,1,1,1,1,4,17,1,7,7,1,11,31,10,2,4,8,2,3,2,1,4,2,16,4,32,2,
        3,19,13,4,9,1,5,2,14,8,1,1,3,6,19,6,5,1,16,6,2,10,8,5,1,2,3,1,5,5,1,11,6,6,1,3,3,2,6,3,8,1,1,4,10,7,5,7,7,5,8,9,2,1,3,4,1,1,3,1,3,3,2,6,16,
        1,4,6,3,1,10,6,1,3,15,2,9,2,10,25,13,9,16,6,2,2,10,11,4,3,9,1,2,6,6,5,4,30,40,1,10,7,12,14,33,6,3,6,7,3,1,3,1,11,14,4,9,5,12,11,49,18,51,31,
        140,31,2,2,1,5,1,8,1,10,1,4,4,3,24,1,10,1,3,6,6,16,3,4,5,2,1,4,2,57,10,6,22,2,22,3,7,22,6,10,11,36,18,16,33,36,2,5,5,1,1,1,4,10,1,4,13,2,7,
        5,2,9,3,4,1,7,43,3,7,3,9,14,7,9,1,11,1,1,3,7,4,18,13,1,14,1,3,6,10,73,2,2,30,6,1,11,18,19,13,22,3,46,42,37,89,7,3,16,34,2,2,3,9,1,7,1,1,1,2,
        2,4,10,7,3,10,3,9,5,28,9,2,6,13,7,3,1,3,10,2,7,2,11,3,6,21,54,85,2,1,4,2,2,1,39,3,21,2,2,5,1,1,1,4,1,1,3,4,15,1,3,2,4,4,2,3,8,2,20,1,8,7,13,
        4,1,26,6,2,9,34,4,21,52,10,4,4,1,5,12,2,11,1,7,2,30,12,44,2,30,1,1,3,6,16,9,17,39,82,2,2,24,7,1,7,3,16,9,14,44,2,1,2,1,2,3,5,2,4,1,6,7,5,3,
        2,6,1,11,5,11,2,1,18,19,8,1,3,24,29,2,1,3,5,2,2,1,13,6,5,1,46,11,3,5,1,1,5,8,2,10,6,12,6,3,7,11,2,4,16,13,2,5,1,1,2,2,5,2,28,5,2,23,10,8,4,
        4,22,39,95,38,8,14,9,5,1,13,5,4,3,13,12,11,1,9,1,27,37,2,5,4,4,63,211,95,2,2,2,1,3,5,2,1,1,2,2,1,1,1,3,2,4,1,2,1,1,5,2,2,1,1,2,3,1,3,1,1,1,
        3,1,4,2,1,3,6,1,1,3,7,15,5,3,2,5,3,9,11,4,2,22,1,6,3,8,7,1,4,28,4,16,3,3,25,4,4,27,27,1,4,1,2,2,7,1,3,5,2,28,8,2,14,1,8,6,16,25,3,3,3,14,3,
        3,1,1,2,1,4,6,3,8,4,1,1,1,2,3,6,10,6,2,3,18,3,2,5,5,4,3,1,5,2,5,4,23,7,6,12,6,4,17,11,9,5,1,1,10,5,12,1,1,11,26,33,7,3,6,1,17,7,1,5,12,1,11,
        2,4,1,8,14,17,23,1,2,1,7,8,16,11,9,6,5,2,6,4,16,2,8,14,1,11,8,9,1,1,1,9,25,4,11,19,7,2,15,2,12,8,52,7,5,19,2,16,4,36,8,1,16,8,24,26,4,6,2,9,
        5,4,36,3,28,12,25,15,37,27,17,12,59,38,5,32,127,1,2,9,17,14,4,1,2,1,1,8,11,50,4,14,2,19,16,4,17,5,4,5,26,12,45,2,23,45,104,30,12,8,3,10,2,2,
        3,3,1,4,20,7,2,9,6,15,2,20,1,3,16,4,11,15,6,134,2,5,59,1,2,2,2,1,9,17,3,26,137,10,211,59,1,2,4,1,4,1,1,1,2,6,2,3,1,1,2,3,2,3,1,3,4,4,2,3,3,
        1,4,3,1,7,2,2,3,1,2,1,3,3,3,2,2,3,2,1,3,14,6,1,3,2,9,6,15,27,9,34,145,1,1,2,1,1,1,1,2,1,1,1,1,2,2,2,3,1,2,1,1,1,2,3,5,8,3,5,2,4,1,3,2,2,2,12,
        4,1,1,1,10,4,5,1,20,4,16,1,15,9,5,12,2,9,2,5,4,2,26,19,7,1,26,4,30,12,15,42,1,6,8,172,1,1,4,2,1,1,11,2,2,4,2,1,2,1,10,8,1,2,1,4,5,1,2,5,1,8,
        4,1,3,4,2,1,6,2,1,3,4,1,2,1,1,1,1,12,5,7,2,4,3,1,1,1,3,3,6,1,2,2,3,3,3,2,1,2,12,14,11,6,6,4,12,2,8,1,7,10,1,35,7,4,13,15,4,3,23,21,28,52,5,
        26,5,6,1,7,10,2,7,53,3,2,1,1,1,2,163,532,1,10,11,1,3,3,4,8,2,8,6,2,2,23,22,4,2,2,4,2,1,3,1,3,3,5,9,8,2,1,2,8,1,10,2,12,21,20,15,105,2,3,1,1,
        3,2,3,1,1,2,5,1,4,15,11,19,1,1,1,1,5,4,5,1,1,2,5,3,5,12,1,2,5,1,11,1,1,15,9,1,4,5,3,26,8,2,1,3,1,1,15,19,2,12,1,2,5,2,7,2,19,2,20,6,26,7,5,
        2,2,7,34,21,13,70,2,128,1,1,2,1,1,2,1,1,3,2,2,2,15,1,4,1,3,4,42,10,6,1,49,85,8,1,2,1,1,4,4,2,3,6,1,5,7,4,3,211,4,1,2,1,2,5,1,2,4,2,2,6,5,6,
        10,3,4,48,100,6,2,16,296,5,27,387,2,2,3,7,16,8,5,38,15,39,21,9,10,3,7,59,13,27,21,47,5,21,6
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0xFFFD, 0xFFFD  // Invalid
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00) * 2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesJapanese()
{
    // 2999 ideograms code points for Japanese
    // - 2136 Joyo (meaning "for regular use" or "for common use") Kanji code points
    // - 863 Jinmeiyo (meaning "for personal name") Kanji code points
    // - Sourced from official information provided by the government agencies of Japan:
    //   - List of Joyo Kanji by the Agency for Cultural Affairs
    //     - https://www.bunka.go.jp/kokugo_nihongo/sisaku/joho/joho/kijun/naikaku/kanji/
    //   - List of Jinmeiyo Kanji by the Ministry of Justice
    //     - http://www.moj.go.jp/MINJI/minji86.html
    //   - Available under the terms of the Creative Commons Attribution 4.0 International (CC BY 4.0).
    //     - https://creativecommons.org/licenses/by/4.0/legalcode
    // - You can generate this code by the script at:
    //   - https://github.com/vaiorabbit/everyday_use_kanji
    // - References:
    //   - List of Joyo Kanji
    //     - (Wikipedia) https://en.wikipedia.org/wiki/List_of_j%C5%8Dy%C5%8D_kanji
    //   - List of Jinmeiyo Kanji
    //     - (Wikipedia) https://en.wikipedia.org/wiki/Jinmeiy%C5%8D_kanji
    // - Missing 1 Joyo Kanji: U+20B9F (Kun'yomi: Shikaru, On'yomi: Shitsu,shichi), see https://github.com/ocornut/imgui/pull/3627 for details.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,3,3,2,2,1,5,3,5,7,5,6,1,2,1,7,2,6,3,1,8,1,1,4,1,1,18,2,11,2,6,2,1,2,1,5,1,2,1,3,1,2,1,2,3,3,1,1,2,3,1,1,1,12,7,9,1,4,5,1,
        1,2,1,10,1,1,9,2,2,4,5,6,9,3,1,1,1,1,9,3,18,5,2,2,2,2,1,6,3,7,1,1,1,1,2,2,4,2,1,23,2,10,4,3,5,2,4,10,2,4,13,1,6,1,9,3,1,1,6,6,7,6,3,1,2,11,3,
        2,2,3,2,15,2,2,5,4,3,6,4,1,2,5,2,12,16,6,13,9,13,2,1,1,7,16,4,7,1,19,1,5,1,2,2,7,7,8,2,6,5,4,9,18,7,4,5,9,13,11,8,15,2,1,1,1,2,1,2,2,1,2,2,8,
        2,9,3,3,1,1,4,4,1,1,1,4,9,1,4,3,5,5,2,7,5,3,4,8,2,1,13,2,3,3,1,14,1,1,4,5,1,3,6,1,5,2,1,1,3,3,3,3,1,1,2,7,6,6,7,1,4,7,6,1,1,1,1,1,12,3,3,9,5,
        2,6,1,5,6,1,2,3,18,2,4,14,4,1,3,6,1,1,6,3,5,5,3,2,2,2,2,12,3,1,4,2,3,2,3,11,1,7,4,1,2,1,3,17,1,9,1,24,1,1,4,2,2,4,1,2,7,1,1,1,3,1,2,2,4,15,1,
        1,2,1,1,2,1,5,2,5,20,2,5,9,1,10,8,7,6,1,1,1,1,1,1,6,2,1,2,8,1,1,1,1,5,1,1,3,1,1,1,1,3,1,1,12,4,1,3,1,1,1,1,1,10,3,1,7,5,13,1,2,3,4,6,1,1,30,
        2,9,9,1,15,38,11,3,1,8,24,7,1,9,8,10,2,1,9,31,2,13,6,2,9,4,49,5,2,15,2,1,10,2,1,1,1,2,2,6,15,30,35,3,14,18,8,1,16,10,28,12,19,45,38,1,3,2,3,
        13,2,1,7,3,6,5,3,4,3,1,5,7,8,1,5,3,18,5,3,6,1,21,4,24,9,24,40,3,14,3,21,3,2,1,2,4,2,3,1,15,15,6,5,1,1,3,1,5,6,1,9,7,3,3,2,1,4,3,8,21,5,16,4,
        5,2,10,11,11,3,6,3,2,9,3,6,13,1,2,1,1,1,1,11,12,6,6,1,4,2,6,5,2,1,1,3,3,6,13,3,1,1,5,1,2,3,3,14,2,1,2,2,2,5,1,9,5,1,1,6,12,3,12,3,4,13,2,14,
        2,8,1,17,5,1,16,4,2,2,21,8,9,6,23,20,12,25,19,9,38,8,3,21,40,25,33,13,4,3,1,4,1,2,4,1,2,5,26,2,1,1,2,1,3,6,2,1,1,1,1,1,1,2,3,1,1,1,9,2,3,1,1,
        1,3,6,3,2,1,1,6,6,1,8,2,2,2,1,4,1,2,3,2,7,3,2,4,1,2,1,2,2,1,1,1,1,1,3,1,2,5,4,10,9,4,9,1,1,1,1,1,1,5,3,2,1,6,4,9,6,1,10,2,31,17,8,3,7,5,40,1,
        7,7,1,6,5,2,10,7,8,4,15,39,25,6,28,47,18,10,7,1,3,1,1,2,1,1,1,3,3,3,1,1,1,3,4,2,1,4,1,3,6,10,7,8,6,2,2,1,3,3,2,5,8,7,9,12,2,15,1,1,4,1,2,1,1,
        1,3,2,1,3,3,5,6,2,3,2,10,1,4,2,8,1,1,1,11,6,1,21,4,16,3,1,3,1,4,2,3,6,5,1,3,1,1,3,3,4,6,1,1,10,4,2,7,10,4,7,4,2,9,4,3,1,1,1,4,1,8,3,4,1,3,1,
        6,1,4,2,1,4,7,2,1,8,1,4,5,1,1,2,2,4,6,2,7,1,10,1,1,3,4,11,10,8,21,4,6,1,3,5,2,1,2,28,5,5,2,3,13,1,2,3,1,4,2,1,5,20,3,8,11,1,3,3,3,1,8,10,9,2,
        10,9,2,3,1,1,2,4,1,8,3,6,1,7,8,6,11,1,4,29,8,4,3,1,2,7,13,1,4,1,6,2,6,12,12,2,20,3,2,3,6,4,8,9,2,7,34,5,1,18,6,1,1,4,4,5,7,9,1,2,2,4,3,4,1,7,
        2,2,2,6,2,3,25,5,3,6,1,4,6,7,4,2,1,4,2,13,6,4,4,3,1,5,3,4,4,3,2,1,1,4,1,2,1,1,3,1,11,1,6,3,1,7,3,6,2,8,8,6,9,3,4,11,3,2,10,12,2,5,11,1,6,4,5,
        3,1,8,5,4,6,6,3,5,1,1,3,2,1,2,2,6,17,12,1,10,1,6,12,1,6,6,19,9,6,16,1,13,4,4,15,7,17,6,11,9,15,12,6,7,2,1,2,2,15,9,3,21,4,6,49,18,7,3,2,3,1,
        6,8,2,2,6,2,9,1,3,6,4,4,1,2,16,2,5,2,1,6,2,3,5,3,1,2,5,1,2,1,9,3,1,8,6,4,8,11,3,1,1,1,1,3,1,13,8,4,1,3,2,2,1,4,1,11,1,5,2,1,5,2,5,8,6,1,1,7,
        4,3,8,3,2,7,2,1,5,1,5,2,4,7,6,2,8,5,1,11,4,5,3,6,18,1,2,13,3,3,1,21,1,1,4,1,4,1,1,1,8,1,2,2,7,1,2,4,2,2,9,2,1,1,1,4,3,6,3,12,5,1,1,1,5,6,3,2,
        4,8,2,2,4,2,7,1,8,9,5,2,3,2,1,3,2,13,7,14,6,5,1,1,2,1,4,2,23,2,1,1,6,3,1,4,1,15,3,1,7,3,9,14,1,3,1,4,1,1,5,8,1,3,8,3,8,15,11,4,14,4,4,2,5,5,
        1,7,1,6,14,7,7,8,5,15,4,8,6,5,6,2,1,13,1,20,15,11,9,2,5,6,2,11,2,6,2,5,1,5,8,4,13,19,25,4,1,1,11,1,34,2,5,9,14,6,2,2,6,1,1,14,1,3,14,13,1,6,
        12,21,14,14,6,32,17,8,32,9,28,1,2,4,11,8,3,1,14,2,5,15,1,1,1,1,3,6,4,1,3,4,11,3,1,1,11,30,1,5,1,4,1,5,8,1,1,3,2,4,3,17,35,2,6,12,17,3,1,6,2,
        1,1,12,2,7,3,3,2,1,16,2,8,3,6,5,4,7,3,3,8,1,9,8,5,1,2,1,3,2,8,1,2,9,12,1,1,2,3,8,3,24,12,4,3,7,5,8,3,3,3,3,3,3,1,23,10,3,1,2,2,6,3,1,16,1,16,
        22,3,10,4,11,6,9,7,7,3,6,2,2,2,4,10,2,1,1,2,8,7,1,6,4,1,3,3,3,5,10,12,12,2,3,12,8,15,1,1,16,6,6,1,5,9,11,4,11,4,2,6,12,1,17,5,13,1,4,9,5,1,11,
        2,1,8,1,5,7,28,8,3,5,10,2,17,3,38,22,1,2,18,12,10,4,38,18,1,4,44,19,4,1,8,4,1,12,1,4,31,12,1,14,7,75,7,5,10,6,6,13,3,2,11,11,3,2,5,28,15,6,18,
        18,5,6,4,3,16,1,7,18,7,36,3,5,3,1,7,1,9,1,10,7,2,4,2,6,2,9,7,4,3,32,12,3,7,10,2,23,16,3,1,12,3,31,4,11,1,3,8,9,5,1,30,15,6,12,3,2,2,11,19,9,
        14,2,6,2,3,19,13,17,5,3,3,25,3,14,1,1,1,36,1,3,2,19,3,13,36,9,13,31,6,4,16,34,2,5,4,2,3,3,5,1,1,1,4,3,1,17,3,2,3,5,3,1,3,2,3,5,6,3,12,11,1,3,
        1,2,26,7,12,7,2,14,3,3,7,7,11,25,25,28,16,4,36,1,2,1,6,2,1,9,3,27,17,4,3,4,13,4,1,3,2,2,1,10,4,2,4,6,3,8,2,1,18,1,1,24,2,2,4,33,2,3,63,7,1,6,
        40,7,3,4,4,2,4,15,18,1,16,1,1,11,2,41,14,1,3,18,13,3,2,4,16,2,17,7,15,24,7,18,13,44,2,2,3,6,1,1,7,5,1,7,1,4,3,3,5,10,8,2,3,1,8,1,1,27,4,2,1,
        12,1,2,1,10,6,1,6,7,5,2,3,7,11,5,11,3,6,6,2,3,15,4,9,1,1,2,1,2,11,2,8,12,8,5,4,2,3,1,5,2,2,1,14,1,12,11,4,1,11,17,17,4,3,2,5,5,7,3,1,5,9,9,8,
        2,5,6,6,13,13,2,1,2,6,1,2,2,49,4,9,1,2,10,16,7,8,4,3,2,23,4,58,3,29,1,14,19,19,11,11,2,7,5,1,3,4,6,2,18,5,12,12,17,17,3,3,2,4,1,6,2,3,4,3,1,
        1,1,1,5,1,1,9,1,3,1,3,6,1,8,1,1,2,6,4,14,3,1,4,11,4,1,3,32,1,2,4,13,4,1,2,4,2,1,3,1,11,1,4,2,1,4,4,6,3,5,1,6,5,7,6,3,23,3,5,3,5,3,3,13,3,9,10,
        1,12,10,2,3,18,13,7,160,52,4,2,2,3,2,14,5,4,12,4,6,4,1,20,4,11,6,2,12,27,1,4,1,2,2,7,4,5,2,28,3,7,25,8,3,19,3,6,10,2,2,1,10,2,5,4,1,3,4,1,5,
        3,2,6,9,3,6,2,16,3,3,16,4,5,5,3,2,1,2,16,15,8,2,6,21,2,4,1,22,5,8,1,1,21,11,2,1,11,11,19,13,12,4,2,3,2,3,6,1,8,11,1,4,2,9,5,2,1,11,2,9,1,1,2,
        14,31,9,3,4,21,14,4,8,1,7,2,2,2,5,1,4,20,3,3,4,10,1,11,9,8,2,1,4,5,14,12,14,2,17,9,6,31,4,14,1,20,13,26,5,2,7,3,6,13,2,4,2,19,6,2,2,18,9,3,5,
        12,12,14,4,6,2,3,6,9,5,22,4,5,25,6,4,8,5,2,6,27,2,35,2,16,3,7,8,8,6,6,5,9,17,2,20,6,19,2,13,3,1,1,1,4,17,12,2,14,7,1,4,18,12,38,33,2,10,1,1,
        2,13,14,17,11,50,6,33,20,26,74,16,23,45,50,13,38,33,6,6,7,4,4,2,1,3,2,5,8,7,8,9,3,11,21,9,13,1,3,10,6,7,1,2,2,18,5,5,1,9,9,2,68,9,19,13,2,5,
        1,4,4,7,4,13,3,9,10,21,17,3,26,2,1,5,2,4,5,4,1,7,4,7,3,4,2,1,6,1,1,20,4,1,9,2,2,1,3,3,2,3,2,1,1,1,20,2,3,1,6,2,3,6,2,4,8,1,3,2,10,3,5,3,4,4,
        3,4,16,1,6,1,10,2,4,2,1,1,2,10,11,2,2,3,1,24,31,4,10,10,2,5,12,16,164,15,4,16,7,9,15,19,17,1,2,1,1,5,1,1,1,1,1,3,1,4,3,1,3,1,3,1,2,1,1,3,3,7,
        2,8,1,2,2,2,1,3,4,3,7,8,12,92,2,10,3,1,3,14,5,25,16,42,4,7,7,4,2,21,5,27,26,27,21,25,30,31,2,1,5,13,3,22,5,6,6,11,9,12,1,5,9,7,5,5,22,60,3,5,
        13,1,1,8,1,1,3,3,2,1,9,3,3,18,4,1,2,3,7,6,3,1,2,3,9,1,3,1,3,2,1,3,1,1,1,2,1,11,3,1,6,9,1,3,2,3,1,2,1,5,1,1,4,3,4,1,2,2,4,4,1,7,2,1,2,2,3,5,13,
        18,3,4,14,9,9,4,16,3,7,5,8,2,6,48,28,3,1,1,4,2,14,8,2,9,2,1,15,2,4,3,2,10,16,12,8,7,1,1,3,1,1,1,2,7,4,1,6,4,38,39,16,23,7,15,15,3,2,12,7,21,
        37,27,6,5,4,8,2,10,8,8,6,5,1,2,1,3,24,1,16,17,9,23,10,17,6,1,51,55,44,13,294,9,3,6,2,4,2,2,15,1,1,1,13,21,17,68,14,8,9,4,1,4,9,3,11,7,1,1,1,
        5,6,3,2,1,1,1,2,3,8,1,2,2,4,1,5,5,2,1,4,3,7,13,4,1,4,1,3,1,1,1,5,5,10,1,6,1,5,2,1,5,2,4,1,4,5,7,3,18,2,9,11,32,4,3,3,2,4,7,11,16,9,11,8,13,38,
        32,8,4,2,1,1,2,1,2,4,4,1,1,1,4,1,21,3,11,1,16,1,1,6,1,3,2,4,9,8,57,7,44,1,3,3,13,3,10,1,1,7,5,2,7,21,47,63,3,15,4,7,1,16,1,1,2,8,2,3,42,15,4,
        1,29,7,22,10,3,78,16,12,20,18,4,67,11,5,1,3,15,6,21,31,32,27,18,13,71,35,5,142,4,10,1,2,50,19,33,16,35,37,16,19,27,7,1,133,19,1,4,8,7,20,1,4,
        4,1,10,3,1,6,1,2,51,5,40,15,24,43,22928,11,1,13,154,70,3,1,1,7,4,10,1,2,1,1,2,1,2,1,2,2,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,
        3,2,1,1,1,1,2,1,1,
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0xFFFD, 0xFFFD  // Invalid
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00)*2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesCyrillic()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesThai()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x2010, 0x205E, // Punctuations
        0x0E00, 0x0E7F, // Thai
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesVietnamese()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x0102, 0x0103,
        0x0110, 0x0111,
        0x0128, 0x0129,
        0x0168, 0x0169,
        0x01A0, 0x01A1,
        0x01AF, 0x01B0,
        0x1EA0, 0x1EF9,
        0,
    };
    return &ranges[0];
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontGlyphRangesBuilder
//-----------------------------------------------------------------------------

void ImFontGlyphRangesBuilder::AddText(const char* text, const char* text_end)
{
    while (text_end ? (text < text_end) : *text)
    {
        unsigned int c = 0;
        int c_len = ImTextCharFromUtf8(&c, text, text_end);
        text += c_len;
        if (c_len == 0)
            break;
        AddChar((ImWchar)c);
    }
}

void ImFontGlyphRangesBuilder::AddRanges(const ImWchar* ranges)
{
    for (; ranges[0]; ranges += 2)
        for (unsigned int c = ranges[0]; c <= ranges[1] && c <= IM_UNICODE_CODEPOINT_MAX; c++) //-V560
            AddChar((ImWchar)c);
}

void ImFontGlyphRangesBuilder::BuildRanges(ImVector<ImWchar>* out_ranges)
{
    const int max_codepoint = IM_UNICODE_CODEPOINT_MAX;
    for (int n = 0; n <= max_codepoint; n++)
        if (GetBit(n))
        {
            out_ranges->push_back((ImWchar)n);
            while (n < max_codepoint && GetBit(n + 1))
                n++;
            out_ranges->push_back((ImWchar)n);
        }
    out_ranges->push_back(0);
}

//-----------------------------------------------------------------------------
// [SECTION] ImFont
//-----------------------------------------------------------------------------

ImFont::ImFont()
{
    memset(this, 0, sizeof(*this));
    Scale = 1.0f;
}

ImFont::~ImFont()
{
    ClearOutputData();
}

void    ImFont::ClearOutputData()
{
    FontSize = 0.0f;
    FallbackAdvanceX = 0.0f;
    Glyphs.clear();
    IndexAdvanceX.clear();
    IndexLookup.clear();
    FallbackGlyph = NULL;
    ContainerAtlas = NULL;
    DirtyLookupTables = true;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
    memset(Used8kPagesMap, 0, sizeof(Used8kPagesMap));
}

static ImWchar FindFirstExistingGlyph(ImFont* font, const ImWchar* candidate_chars, int candidate_chars_count)
{
    for (int n = 0; n < candidate_chars_count; n++)
        if (font->FindGlyphNoFallback(candidate_chars[n]) != NULL)
            return candidate_chars[n];
    return 0;
}

void ImFont::BuildLookupTable()
{
    int max_codepoint = 0;
    for (int i = 0; i != Glyphs.Size; i++)
        max_codepoint = ImMax(max_codepoint, (int)Glyphs[i].Codepoint);

    // Build lookup table
    IM_ASSERT(Glyphs.Size > 0 && "Font has not loaded glyph!");
    IM_ASSERT(Glyphs.Size < 0xFFFF); // -1 is reserved
    IndexAdvanceX.clear();
    IndexLookup.clear();
    DirtyLookupTables = false;
    memset(Used8kPagesMap, 0, sizeof(Used8kPagesMap));
    GrowIndex(max_codepoint + 1);
    for (int i = 0; i < Glyphs.Size; i++)
    {
        int codepoint = (int)Glyphs[i].Codepoint;
        IndexAdvanceX[codepoint] = Glyphs[i].AdvanceX;
        IndexLookup[codepoint] = (ImU16)i;

        // Mark 4K page as used
        const int page_n = codepoint / 8192;
        Used8kPagesMap[page_n >> 3] |= 1 << (page_n & 7);
    }

    // Create a glyph to handle TAB
    // FIXME: Needs proper TAB handling but it needs to be contextualized (or we could arbitrary say that each string starts at "column 0" ?)
    if (FindGlyph((ImWchar)' '))
    {
        if (Glyphs.back().Codepoint != '\t')   // So we can call this function multiple times (FIXME: Flaky)
            Glyphs.resize(Glyphs.Size + 1);
        ImFontGlyph& tab_glyph = Glyphs.back();
        tab_glyph = *FindGlyph((ImWchar)' ');
        tab_glyph.Codepoint = '\t';
        tab_glyph.AdvanceX *= IM_TABSIZE;
        IndexAdvanceX[(int)tab_glyph.Codepoint] = (float)tab_glyph.AdvanceX;
        IndexLookup[(int)tab_glyph.Codepoint] = (ImU16)(Glyphs.Size - 1);
    }

    // Mark special glyphs as not visible (note that AddGlyph already mark as non-visible glyphs with zero-size polygons)
    if (ImFontGlyph* glyph = (ImFontGlyph*)(void*)FindGlyph((ImWchar)' '))
        glyph->Visible = false;
    if (ImFontGlyph* glyph = (ImFontGlyph*)(void*)FindGlyph((ImWchar)'\t'))
        glyph->Visible = false;

    // Setup Fallback character
    const ImWchar fallback_chars[] = { (ImWchar)IM_UNICODE_CODEPOINT_INVALID, (ImWchar)'?', (ImWchar)' ' };
    FallbackGlyph = FindGlyphNoFallback(FallbackChar);
    if (FallbackGlyph == NULL)
    {
        FallbackChar = FindFirstExistingGlyph(this, fallback_chars, IM_ARRAYSIZE(fallback_chars));
        FallbackGlyph = FindGlyphNoFallback(FallbackChar);
        if (FallbackGlyph == NULL)
        {
            FallbackGlyph = &Glyphs.back();
            FallbackChar = (ImWchar)FallbackGlyph->Codepoint;
        }
    }
    FallbackAdvanceX = FallbackGlyph->AdvanceX;
    for (int i = 0; i < max_codepoint + 1; i++)
        if (IndexAdvanceX[i] < 0.0f)
            IndexAdvanceX[i] = FallbackAdvanceX;

    // Setup Ellipsis character. It is required for rendering elided text. We prefer using U+2026 (horizontal ellipsis).
    // However some old fonts may contain ellipsis at U+0085. Here we auto-detect most suitable ellipsis character.
    // FIXME: Note that 0x2026 is rarely included in our font ranges. Because of this we are more likely to use three individual dots.
    const ImWchar ellipsis_chars[] = { Sources->EllipsisChar, (ImWchar)0x2026, (ImWchar)0x0085 };
    const ImWchar dots_chars[] = { (ImWchar)'.', (ImWchar)0xFF0E };
    if (EllipsisChar == 0)
        EllipsisChar = FindFirstExistingGlyph(this, ellipsis_chars, IM_ARRAYSIZE(ellipsis_chars));
    const ImWchar dot_char = FindFirstExistingGlyph(this, dots_chars, IM_ARRAYSIZE(dots_chars));
    if (EllipsisChar != 0)
    {
        EllipsisCharCount = 1;
        EllipsisWidth = EllipsisCharStep = FindGlyph(EllipsisChar)->X1;
    }
    else if (dot_char != 0)
    {
        const ImFontGlyph* dot_glyph = FindGlyph(dot_char);
        EllipsisChar = dot_char;
        EllipsisCharCount = 3;
        EllipsisCharStep = (float)(int)(dot_glyph->X1 - dot_glyph->X0) + 1.0f;
        EllipsisWidth = ImMax(dot_glyph->AdvanceX, dot_glyph->X0 + EllipsisCharStep * 3.0f - 1.0f); // FIXME: Slightly odd for normally mono-space fonts but since this is used for trailing contents.
    }
}

// API is designed this way to avoid exposing the 8K page size
// e.g. use with IsGlyphRangeUnused(0, 255)
bool ImFont::IsGlyphRangeUnused(unsigned int c_begin, unsigned int c_last)
{
    unsigned int page_begin = (c_begin / 8192);
    unsigned int page_last = (c_last / 8192);
    for (unsigned int page_n = page_begin; page_n <= page_last; page_n++)
        if ((page_n >> 3) < sizeof(Used8kPagesMap))
            if (Used8kPagesMap[page_n >> 3] & (1 << (page_n & 7)))
                return false;
    return true;
}

void ImFont::GrowIndex(int new_size)
{
    IM_ASSERT(IndexAdvanceX.Size == IndexLookup.Size);
    if (new_size <= IndexLookup.Size)
        return;
    IndexAdvanceX.resize(new_size, -1.0f);
    IndexLookup.resize(new_size, (ImU16)-1);
}

// x0/y0/x1/y1 are offset from the character upper-left layout position, in pixels. Therefore x0/y0 are often fairly close to zero.
// Not to be mistaken with texture coordinates, which are held by u0/v0/u1/v1 in normalized format (0.0..1.0 on each texture axis).
// 'src' is not necessarily == 'this->Sources' because multiple source fonts+configs can be used to build one target font.
void ImFont::AddGlyph(const ImFontConfig* src, ImWchar codepoint, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, float advance_x)
{
    if (src != NULL)
    {
        // Clamp & recenter if needed
        const float advance_x_original = advance_x;
        advance_x = ImClamp(advance_x, src->GlyphMinAdvanceX, src->GlyphMaxAdvanceX);
        if (advance_x != advance_x_original)
        {
            float char_off_x = src->PixelSnapH ? ImTrunc((advance_x - advance_x_original) * 0.5f) : (advance_x - advance_x_original) * 0.5f;
            x0 += char_off_x;
            x1 += char_off_x;
        }

        // Snap to pixel
        if (src->PixelSnapH)
            advance_x = IM_ROUND(advance_x);

        // Bake extra spacing
        advance_x += src->GlyphExtraAdvanceX;
    }

    int glyph_idx = Glyphs.Size;
    Glyphs.resize(Glyphs.Size + 1);
    ImFontGlyph& glyph = Glyphs[glyph_idx];
    glyph.Codepoint = (unsigned int)codepoint;
    glyph.Visible = (x0 != x1) && (y0 != y1);
    glyph.Colored = false;
    glyph.X0 = x0;
    glyph.Y0 = y0;
    glyph.X1 = x1;
    glyph.Y1 = y1;
    glyph.U0 = u0;
    glyph.V0 = v0;
    glyph.U1 = u1;
    glyph.V1 = v1;
    glyph.AdvanceX = advance_x;
    IM_ASSERT(Glyphs.Size < 0xFFFF); // IndexLookup[] hold 16-bit values and -1 is reserved.

    // Compute rough surface usage metrics (+1 to account for average padding, +0.99 to round)
    // We use (U1-U0)*TexWidth instead of X1-X0 to account for oversampling.
    float pad = ContainerAtlas->TexGlyphPadding + 0.99f;
    DirtyLookupTables = true;
    MetricsTotalSurface += (int)((glyph.U1 - glyph.U0) * ContainerAtlas->TexWidth + pad) * (int)((glyph.V1 - glyph.V0) * ContainerAtlas->TexHeight + pad);
}

void ImFont::AddRemapChar(ImWchar dst, ImWchar src, bool overwrite_dst)
{
    IM_ASSERT(IndexLookup.Size > 0);    // Currently this can only be called AFTER the font has been built, aka after calling ImFontAtlas::GetTexDataAs*() function.
    unsigned int index_size = (unsigned int)IndexLookup.Size;

    if (dst < index_size && IndexLookup.Data[dst] == (ImU16)-1 && !overwrite_dst) // 'dst' already exists
        return;
    if (src >= index_size && dst >= index_size) // both 'dst' and 'src' don't exist -> no-op
        return;

    GrowIndex(dst + 1);
    IndexLookup[dst] = (src < index_size) ? IndexLookup.Data[src] : (ImU16)-1;
    IndexAdvanceX[dst] = (src < index_size) ? IndexAdvanceX.Data[src] : 1.0f;
}

// Find glyph, return fallback if missing
ImFontGlyph* ImFont::FindGlyph(ImWchar c)
{
    if (c >= (size_t)IndexLookup.Size)
        return FallbackGlyph;
    const ImU16 i = IndexLookup.Data[c];
    if (i == (ImU16)-1)
        return FallbackGlyph;
    return &Glyphs.Data[i];
}

ImFontGlyph* ImFont::FindGlyphNoFallback(ImWchar c)
{
    if (c >= (size_t)IndexLookup.Size)
        return NULL;
    const ImU16 i = IndexLookup.Data[c];
    if (i == (ImU16)-1)
        return NULL;
    return &Glyphs.Data[i];
}

// Trim trailing space and find beginning of next line
static inline const char* CalcWordWrapNextLineStartA(const char* text, const char* text_end)
{
    while (text < text_end && ImCharIsBlankA(*text))
        text++;
    if (*text == '\n')
        text++;
    return text;
}

#define ImFontGetCharAdvanceX(_FONT, _CH)  ((int)(_CH) < (_FONT)->IndexAdvanceX.Size ? (_FONT)->IndexAdvanceX.Data[_CH] : (_FONT)->FallbackAdvanceX)

// Simple word-wrapping for English, not full-featured. Please submit failing cases!
// This will return the next location to wrap from. If no wrapping if necessary, this will fast-forward to e.g. text_end.
// FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)
const char* ImFont::CalcWordWrapPositionA(float scale, const char* text, const char* text_end, float wrap_width)
{
    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"
    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;
    wrap_width /= scale; // We work with unscaled widths to avoid scaling every characters

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    IM_ASSERT(text_end != NULL);
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + ImTextCharFromUtf8(&c, s, text_end);

        if (c < 32)
        {
            if (c == '\n')
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word = true;
                s = next_s;
                continue;
            }
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ImFontGetCharAdvanceX(this, c);
        if (ImCharIsBlankW(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end = s;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = (c != '.' && c != ',' && c != ';' && c != '!' && c != '?' && c != '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width > wrap_width)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < wrap_width)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
    // +1 may not be a character start point in UTF-8 but it's ok because caller loops use (text >= word_wrap_eol).
    if (s == text && text < text_end)
        return s + 1;
    return s;
}

ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining)
{
    if (!text_end)
        text_end = text_begin + ImStrlen(text_begin); // FIXME-OPT: Need to avoid this.

    const float line_height = size;
    const float scale = size / FontSize;

    ImVec2 text_size = ImVec2(0, 0);
    float line_width = 0.0f;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    const char* s = text_begin;
    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - line_width);

            if (s >= word_wrap_eol)
            {
                if (text_size.x < line_width)
                    text_size.x = line_width;
                text_size.y += line_height;
                line_width = 0.0f;
                word_wrap_eol = NULL;
                s = CalcWordWrapNextLineStartA(s, text_end); // Wrapping skips upcoming blanks
                continue;
            }
        }

        // Decode and advance source
        const char* prev_s = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
            s += 1;
        else
            s += ImTextCharFromUtf8(&c, s, text_end);

        if (c < 32)
        {
            if (c == '\n')
            {
                text_size.x = ImMax(text_size.x, line_width);
                text_size.y += line_height;
                line_width = 0.0f;
                continue;
            }
            if (c == '\r')
                continue;
        }

        const float char_width = ImFontGetCharAdvanceX(this, c) * scale;
        if (line_width + char_width >= max_width)
        {
            s = prev_s;
            break;
        }

        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (line_width > 0 || text_size.y == 0.0f)
        text_size.y += line_height;

    if (remaining)
        *remaining = s;

    return text_size;
}

// Note: as with every ImDrawList drawing function, this expects that the font atlas texture is bound.
void ImFont::RenderChar(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, ImWchar c)
{
    const ImFontGlyph* glyph = FindGlyph(c);
    if (!glyph || !glyph->Visible)
        return;
    if (glyph->Colored)
        col |= ~IM_COL32_A_MASK;
    float scale = (size >= 0.0f) ? (size / FontSize) : 1.0f;
    float x = IM_TRUNC(pos.x);
    float y = IM_TRUNC(pos.y);
    draw_list->PrimReserve(6, 4);
    draw_list->PrimRectUV(ImVec2(x + glyph->X0 * scale, y + glyph->Y0 * scale), ImVec2(x + glyph->X1 * scale, y + glyph->Y1 * scale), ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V1), col);
}

// Note: as with every ImDrawList drawing function, this expects that the font atlas texture is bound.
void ImFont::RenderText(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width, bool cpu_fine_clip)
{
    // Align to be pixel perfect
    float x = IM_TRUNC(pos.x);
    float y = IM_TRUNC(pos.y);
    if (y > clip_rect.w)
        return;

    if (!text_end)
        text_end = text_begin + ImStrlen(text_begin); // ImGui:: functions generally already provides a valid text_end, so this is merely to handle direct calls.

    const float scale = size / FontSize;
    const float line_height = FontSize * scale;
    const float origin_x = x;
    const bool word_wrap_enabled = (wrap_width > 0.0f);

    // Fast-forward to first visible line
    const char* s = text_begin;
    if (y + line_height < clip_rect.y)
        while (y + line_height < clip_rect.y && s < text_end)
        {
            const char* line_end = (const char*)ImMemchr(s, '\n', text_end - s);
            if (word_wrap_enabled)
            {
                // FIXME-OPT: This is not optimal as do first do a search for \n before calling CalcWordWrapPositionA().
                // If the specs for CalcWordWrapPositionA() were reworked to optionally return on \n we could combine both.
                // However it is still better than nothing performing the fast-forward!
                s = CalcWordWrapPositionA(scale, s, line_end ? line_end : text_end, wrap_width);
                s = CalcWordWrapNextLineStartA(s, text_end);
            }
            else
            {
                s = line_end ? line_end + 1 : text_end;
            }
            y += line_height;
        }

    // For large text, scan for the last visible line in order to avoid over-reserving in the call to PrimReserve()
    // Note that very large horizontal line will still be affected by the issue (e.g. a one megabyte string buffer without a newline will likely crash atm)
    if (text_end - s > 10000 && !word_wrap_enabled)
    {
        const char* s_end = s;
        float y_end = y;
        while (y_end < clip_rect.w && s_end < text_end)
        {
            s_end = (const char*)ImMemchr(s_end, '\n', text_end - s_end);
            s_end = s_end ? s_end + 1 : text_end;
            y_end += line_height;
        }
        text_end = s_end;
    }
    if (s == text_end)
        return;

    // Reserve vertices for remaining worse case (over-reserving is useful and easily amortized)
    const int vtx_count_max = (int)(text_end - s) * 4;
    const int idx_count_max = (int)(text_end - s) * 6;
    const int idx_expected_size = draw_list->IdxBuffer.Size + idx_count_max;
    draw_list->PrimReserve(idx_count_max, vtx_count_max);
    ImDrawVert*  vtx_write = draw_list->_VtxWritePtr;
    ImDrawIdx*   idx_write = draw_list->_IdxWritePtr;
    unsigned int vtx_index = draw_list->_VtxCurrentIdx;

    const ImU32 col_untinted = col | ~IM_COL32_A_MASK;
    const char* word_wrap_eol = NULL;

    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - origin_x));

            if (s >= word_wrap_eol)
            {
                x = origin_x;
                y += line_height;
                if (y > clip_rect.w)
                    break; // break out of main loop
                word_wrap_eol = NULL;
                s = CalcWordWrapNextLineStartA(s, text_end); // Wrapping skips upcoming blanks
                continue;
            }
        }

        // Decode and advance source
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
            s += 1;
        else
            s += ImTextCharFromUtf8(&c, s, text_end);

        if (c < 32)
        {
            if (c == '\n')
            {
                x = origin_x;
                y += line_height;
                if (y > clip_rect.w)
                    break; // break out of main loop
                continue;
            }
            if (c == '\r')
                continue;
        }

        const ImFontGlyph* glyph = FindGlyph((ImWchar)c);
        if (glyph == NULL)
            continue;

        float char_width = glyph->AdvanceX * scale;
        if (glyph->Visible)
        {
            // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clip_rect.y and exit once we pass clip_rect.w
            float x1 = x + glyph->X0 * scale;
            float x2 = x + glyph->X1 * scale;
            float y1 = y + glyph->Y0 * scale;
            float y2 = y + glyph->Y1 * scale;
            if (x1 <= clip_rect.z && x2 >= clip_rect.x)
            {
                // Render a character
                float u1 = glyph->U0;
                float v1 = glyph->V0;
                float u2 = glyph->U1;
                float v2 = glyph->V1;

                // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
                if (cpu_fine_clip)
                {
                    if (x1 < clip_rect.x)
                    {
                        u1 = u1 + (1.0f - (x2 - clip_rect.x) / (x2 - x1)) * (u2 - u1);
                        x1 = clip_rect.x;
                    }
                    if (y1 < clip_rect.y)
                    {
                        v1 = v1 + (1.0f - (y2 - clip_rect.y) / (y2 - y1)) * (v2 - v1);
                        y1 = clip_rect.y;
                    }
                    if (x2 > clip_rect.z)
                    {
                        u2 = u1 + ((clip_rect.z - x1) / (x2 - x1)) * (u2 - u1);
                        x2 = clip_rect.z;
                    }
                    if (y2 > clip_rect.w)
                    {
                        v2 = v1 + ((clip_rect.w - y1) / (y2 - y1)) * (v2 - v1);
                        y2 = clip_rect.w;
                    }
                    if (y1 >= y2)
                    {
                        x += char_width;
                        continue;
                    }
                }

                // Support for untinted glyphs
                ImU32 glyph_col = glyph->Colored ? col_untinted : col;

                // We are NOT calling PrimRectUV() here because non-inlined causes too much overhead in a debug builds. Inlined here:
                {
                    vtx_write[0].pos.x = x1; vtx_write[0].pos.y = y1; vtx_write[0].col = glyph_col; vtx_write[0].uv.x = u1; vtx_write[0].uv.y = v1;
                    vtx_write[1].pos.x = x2; vtx_write[1].pos.y = y1; vtx_write[1].col = glyph_col; vtx_write[1].uv.x = u2; vtx_write[1].uv.y = v1;
                    vtx_write[2].pos.x = x2; vtx_write[2].pos.y = y2; vtx_write[2].col = glyph_col; vtx_write[2].uv.x = u2; vtx_write[2].uv.y = v2;
                    vtx_write[3].pos.x = x1; vtx_write[3].pos.y = y2; vtx_write[3].col = glyph_col; vtx_write[3].uv.x = u1; vtx_write[3].uv.y = v2;
                    idx_write[0] = (ImDrawIdx)(vtx_index); idx_write[1] = (ImDrawIdx)(vtx_index + 1); idx_write[2] = (ImDrawIdx)(vtx_index + 2);
                    idx_write[3] = (ImDrawIdx)(vtx_index); idx_write[4] = (ImDrawIdx)(vtx_index + 2); idx_write[5] = (ImDrawIdx)(vtx_index + 3);
                    vtx_write += 4;
                    vtx_index += 4;
                    idx_write += 6;
                }
            }
        }
        x += char_width;
    }

    // Give back unused vertices (clipped ones, blanks) ~ this is essentially a PrimUnreserve() action.
    draw_list->VtxBuffer.Size = (int)(vtx_write - draw_list->VtxBuffer.Data); // Same as calling shrink()
    draw_list->IdxBuffer.Size = (int)(idx_write - draw_list->IdxBuffer.Data);
    draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ElemCount -= (idx_expected_size - draw_list->IdxBuffer.Size);
    draw_list->_VtxWritePtr = vtx_write;
    draw_list->_IdxWritePtr = idx_write;
    draw_list->_VtxCurrentIdx = vtx_index;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGui Internal Render Helpers
//-----------------------------------------------------------------------------
// Vaguely redesigned to stop accessing ImGui global state:
// - RenderArrow()
// - RenderBullet()
// - RenderCheckMark()
// - RenderArrowPointingAt()
// - RenderRectFilledRangeH()
// - RenderRectFilledWithHole()
//-----------------------------------------------------------------------------
// Function in need of a redesign (legacy mess)
// - RenderColorRectWithAlphaCheckerboard()
//-----------------------------------------------------------------------------

// Render an arrow aimed to be aligned with text (p_min is a position in the same space text would be positioned). To e.g. denote expanded/collapsed state
void ImGui::RenderArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale)
{
    const float h = draw_list->_Data->FontSize * 1.00f;
    float r = h * 0.40f * scale;
    ImVec2 center = pos + ImVec2(h * 0.50f, h * 0.50f * scale);

    ImVec2 a, b, c;
    switch (dir)
    {
    case ImGuiDir_Up:
    case ImGuiDir_Down:
        if (dir == ImGuiDir_Up) r = -r;
        a = ImVec2(+0.000f, +0.750f) * r;
        b = ImVec2(-0.866f, -0.750f) * r;
        c = ImVec2(+0.866f, -0.750f) * r;
        break;
    case ImGuiDir_Left:
    case ImGuiDir_Right:
        if (dir == ImGuiDir_Left) r = -r;
        a = ImVec2(+0.750f, +0.000f) * r;
        b = ImVec2(-0.750f, +0.866f) * r;
        c = ImVec2(-0.750f, -0.866f) * r;
        break;
    case ImGuiDir_None:
    case ImGuiDir_COUNT:
        IM_ASSERT(0);
        break;
    }
    draw_list->AddTriangleFilled(center + a, center + b, center + c, col);
}

void ImGui::RenderBullet(ImDrawList* draw_list, ImVec2 pos, ImU32 col)
{
    // FIXME-OPT: This should be baked in font.
    draw_list->AddCircleFilled(pos, draw_list->_Data->FontSize * 0.20f, col, 8);
}

void ImGui::RenderCheckMark(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float sz)
{
    float thickness = ImMax(sz / 5.0f, 1.0f);
    sz -= thickness * 0.5f;
    pos += ImVec2(thickness * 0.25f, thickness * 0.25f);

    float third = sz / 3.0f;
    float bx = pos.x + third;
    float by = pos.y + sz - third * 0.5f;
    draw_list->PathLineTo(ImVec2(bx - third, by - third));
    draw_list->PathLineTo(ImVec2(bx, by));
    draw_list->PathLineTo(ImVec2(bx + third * 2.0f, by - third * 2.0f));
    draw_list->PathStroke(col, 0, thickness);
}

// Render an arrow. 'pos' is position of the arrow tip. half_sz.x is length from base to tip. half_sz.y is length on each side.
void ImGui::RenderArrowPointingAt(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 col)
{
    switch (direction)
    {
    case ImGuiDir_Left:  draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Right: draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_Up:    draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Down:  draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_None: case ImGuiDir_COUNT: break; // Fix warnings
    }
}

static inline float ImAcos01(float x)
{
    if (x <= 0.0f) return IM_PI * 0.5f;
    if (x >= 1.0f) return 0.0f;
    return ImAcos(x);
    //return (-0.69813170079773212f * x * x - 0.87266462599716477f) * x + 1.5707963267948966f; // Cheap approximation, may be enough for what we do.
}

// FIXME: Cleanup and move code to ImDrawList.
void ImGui::RenderRectFilledRangeH(ImDrawList* draw_list, const ImRect& rect, ImU32 col, float x_start_norm, float x_end_norm, float rounding)
{
    if (x_end_norm == x_start_norm)
        return;
    if (x_start_norm > x_end_norm)
        ImSwap(x_start_norm, x_end_norm);

    ImVec2 p0 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_start_norm), rect.Min.y);
    ImVec2 p1 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_end_norm), rect.Max.y);
    if (rounding == 0.0f)
    {
        draw_list->AddRectFilled(p0, p1, col, 0.0f);
        return;
    }

    rounding = ImClamp(ImMin((rect.Max.x - rect.Min.x) * 0.5f, (rect.Max.y - rect.Min.y) * 0.5f) - 1.0f, 0.0f, rounding);
    const float inv_rounding = 1.0f / rounding;
    const float arc0_b = ImAcos01(1.0f - (p0.x - rect.Min.x) * inv_rounding);
    const float arc0_e = ImAcos01(1.0f - (p1.x - rect.Min.x) * inv_rounding);
    const float half_pi = IM_PI * 0.5f; // We will == compare to this because we know this is the exact value ImAcos01 can return.
    const float x0 = ImMax(p0.x, rect.Min.x + rounding);
    if (arc0_b == arc0_e)
    {
        draw_list->PathLineTo(ImVec2(x0, p1.y));
        draw_list->PathLineTo(ImVec2(x0, p0.y));
    }
    else if (arc0_b == 0.0f && arc0_e == half_pi)
    {
        draw_list->PathArcToFast(ImVec2(x0, p1.y - rounding), rounding, 3, 6); // BL
        draw_list->PathArcToFast(ImVec2(x0, p0.y + rounding), rounding, 6, 9); // TR
    }
    else
    {
        draw_list->PathArcTo(ImVec2(x0, p1.y - rounding), rounding, IM_PI - arc0_e, IM_PI - arc0_b); // BL
        draw_list->PathArcTo(ImVec2(x0, p0.y + rounding), rounding, IM_PI + arc0_b, IM_PI + arc0_e); // TR
    }
    if (p1.x > rect.Min.x + rounding)
    {
        const float arc1_b = ImAcos01(1.0f - (rect.Max.x - p1.x) * inv_rounding);
        const float arc1_e = ImAcos01(1.0f - (rect.Max.x - p0.x) * inv_rounding);
        const float x1 = ImMin(p1.x, rect.Max.x - rounding);
        if (arc1_b == arc1_e)
        {
            draw_list->PathLineTo(ImVec2(x1, p0.y));
            draw_list->PathLineTo(ImVec2(x1, p1.y));
        }
        else if (arc1_b == 0.0f && arc1_e == half_pi)
        {
            draw_list->PathArcToFast(ImVec2(x1, p0.y + rounding), rounding, 9, 12); // TR
            draw_list->PathArcToFast(ImVec2(x1, p1.y - rounding), rounding, 0, 3);  // BR
        }
        else
        {
            draw_list->PathArcTo(ImVec2(x1, p0.y + rounding), rounding, -arc1_e, -arc1_b); // TR
            draw_list->PathArcTo(ImVec2(x1, p1.y - rounding), rounding, +arc1_b, +arc1_e); // BR
        }
    }
    draw_list->PathFillConvex(col);
}

void ImGui::RenderRectFilledWithHole(ImDrawList* draw_list, const ImRect& outer, const ImRect& inner, ImU32 col, float rounding)
{
    const bool fill_L = (inner.Min.x > outer.Min.x);
    const bool fill_R = (inner.Max.x < outer.Max.x);
    const bool fill_U = (inner.Min.y > outer.Min.y);
    const bool fill_D = (inner.Max.y < outer.Max.y);
    if (fill_L) draw_list->AddRectFilled(ImVec2(outer.Min.x, inner.Min.y), ImVec2(inner.Min.x, inner.Max.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_U ? 0 : ImDrawFlags_RoundCornersTopLeft)    | (fill_D ? 0 : ImDrawFlags_RoundCornersBottomLeft));
    if (fill_R) draw_list->AddRectFilled(ImVec2(inner.Max.x, inner.Min.y), ImVec2(outer.Max.x, inner.Max.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_U ? 0 : ImDrawFlags_RoundCornersTopRight)   | (fill_D ? 0 : ImDrawFlags_RoundCornersBottomRight));
    if (fill_U) draw_list->AddRectFilled(ImVec2(inner.Min.x, outer.Min.y), ImVec2(inner.Max.x, inner.Min.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_L ? 0 : ImDrawFlags_RoundCornersTopLeft)    | (fill_R ? 0 : ImDrawFlags_RoundCornersTopRight));
    if (fill_D) draw_list->AddRectFilled(ImVec2(inner.Min.x, inner.Max.y), ImVec2(inner.Max.x, outer.Max.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_L ? 0 : ImDrawFlags_RoundCornersBottomLeft) | (fill_R ? 0 : ImDrawFlags_RoundCornersBottomRight));
    if (fill_L && fill_U) draw_list->AddRectFilled(ImVec2(outer.Min.x, outer.Min.y), ImVec2(inner.Min.x, inner.Min.y), col, rounding, ImDrawFlags_RoundCornersTopLeft);
    if (fill_R && fill_U) draw_list->AddRectFilled(ImVec2(inner.Max.x, outer.Min.y), ImVec2(outer.Max.x, inner.Min.y), col, rounding, ImDrawFlags_RoundCornersTopRight);
    if (fill_L && fill_D) draw_list->AddRectFilled(ImVec2(outer.Min.x, inner.Max.y), ImVec2(inner.Min.x, outer.Max.y), col, rounding, ImDrawFlags_RoundCornersBottomLeft);
    if (fill_R && fill_D) draw_list->AddRectFilled(ImVec2(inner.Max.x, inner.Max.y), ImVec2(outer.Max.x, outer.Max.y), col, rounding, ImDrawFlags_RoundCornersBottomRight);
}

// Helper for ColorPicker4()
// NB: This is rather brittle and will show artifact when rounding this enabled if rounded corners overlap multiple cells. Caller currently responsible for avoiding that.
// Spent a non reasonable amount of time trying to getting this right for ColorButton with rounding+anti-aliasing+ImGuiColorEditFlags_HalfAlphaPreview flag + various grid sizes and offsets, and eventually gave up... probably more reasonable to disable rounding altogether.
// FIXME: uses ImGui::GetColorU32
void ImGui::RenderColorRectWithAlphaCheckerboard(ImDrawList* draw_list, ImVec2 p_min, ImVec2 p_max, ImU32 col, float grid_step, ImVec2 grid_off, float rounding, ImDrawFlags flags)
{
    if ((flags & ImDrawFlags_RoundCornersMask_) == 0)
        flags = ImDrawFlags_RoundCornersDefault_;
    if (((col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) < 0xFF)
    {
        ImU32 col_bg1 = GetColorU32(ImAlphaBlendColors(IM_COL32(204, 204, 204, 255), col));
        ImU32 col_bg2 = GetColorU32(ImAlphaBlendColors(IM_COL32(128, 128, 128, 255), col));
        draw_list->AddRectFilled(p_min, p_max, col_bg1, rounding, flags);

        int yi = 0;
        for (float y = p_min.y + grid_off.y; y < p_max.y; y += grid_step, yi++)
        {
            float y1 = ImClamp(y, p_min.y, p_max.y), y2 = ImMin(y + grid_step, p_max.y);
            if (y2 <= y1)
                continue;
            for (float x = p_min.x + grid_off.x + (yi & 1) * grid_step; x < p_max.x; x += grid_step * 2.0f)
            {
                float x1 = ImClamp(x, p_min.x, p_max.x), x2 = ImMin(x + grid_step, p_max.x);
                if (x2 <= x1)
                    continue;
                ImDrawFlags cell_flags = ImDrawFlags_RoundCornersNone;
                if (y1 <= p_min.y) { if (x1 <= p_min.x) cell_flags |= ImDrawFlags_RoundCornersTopLeft; if (x2 >= p_max.x) cell_flags |= ImDrawFlags_RoundCornersTopRight; }
                if (y2 >= p_max.y) { if (x1 <= p_min.x) cell_flags |= ImDrawFlags_RoundCornersBottomLeft; if (x2 >= p_max.x) cell_flags |= ImDrawFlags_RoundCornersBottomRight; }

                // Combine flags
                cell_flags = (flags == ImDrawFlags_RoundCornersNone || cell_flags == ImDrawFlags_RoundCornersNone) ? ImDrawFlags_RoundCornersNone : (cell_flags & flags);
                draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col_bg2, rounding, cell_flags);
            }
        }
    }
    else
    {
        draw_list->AddRectFilled(p_min, p_max, col, rounding, flags);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Decompression code
//-----------------------------------------------------------------------------
// Compressed with stb_compress() then converted to a C array and encoded as base85.
// Use the program in misc/fonts/binary_to_compressed_c.cpp to create the array from a TTF file.
// The purpose of encoding as base85 instead of "0x00,0x01,..." style is only save on _source code_ size.
// Decompression from stb.h (public domain) by Sean Barrett https://github.com/nothings/stb/blob/master/stb.h
//-----------------------------------------------------------------------------

static unsigned int stb_decompress_length(const unsigned char *input)
{
    return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

static unsigned char *stb__barrier_out_e, *stb__barrier_out_b;
static const unsigned char *stb__barrier_in_b;
static unsigned char *stb__dout;
static void stb__match(const unsigned char *data, unsigned int length)
{
    // INVERSE of memmove... write each byte before copying the next...
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_out_b) { stb__dout = stb__barrier_out_e+1; return; }
    while (length--) *stb__dout++ = *data++;
}

static void stb__lit(const unsigned char *data, unsigned int length)
{
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_in_b) { stb__dout = stb__barrier_out_e+1; return; }
    memcpy(stb__dout, data, length);
    stb__dout += length;
}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

static const unsigned char *stb_decompress_token(const unsigned char *i)
{
    if (*i >= 0x20) { // use fewer if's for cases that expand small
        if (*i >= 0x80)       stb__match(stb__dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
        else if (*i >= 0x40)  stb__match(stb__dout-(stb__in2(0) - 0x4000 + 1), i[2]+1), i += 3;
        else /* *i >= 0x20 */ stb__lit(i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
    } else { // more ifs for cases that expand large, since overhead is amortized
        if (*i >= 0x18)       stb__match(stb__dout-(stb__in3(0) - 0x180000 + 1), i[3]+1), i += 4;
        else if (*i >= 0x10)  stb__match(stb__dout-(stb__in3(0) - 0x100000 + 1), stb__in2(3)+1), i += 5;
        else if (*i >= 0x08)  stb__lit(i+2, stb__in2(0) - 0x0800 + 1), i += 2 + (stb__in2(0) - 0x0800 + 1);
        else if (*i == 0x07)  stb__lit(i+3, stb__in2(1) + 1), i += 3 + (stb__in2(1) + 1);
        else if (*i == 0x06)  stb__match(stb__dout-(stb__in3(1)+1), i[4]+1), i += 5;
        else if (*i == 0x04)  stb__match(stb__dout-(stb__in3(1)+1), stb__in2(4)+1), i += 6;
    }
    return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
    unsigned long blocklen = buflen % 5552;

    unsigned long i;
    while (buflen) {
        for (i=0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0], s2 += s1;
            s1 += buffer[1], s2 += s1;
            s1 += buffer[2], s2 += s1;
            s1 += buffer[3], s2 += s1;
            s1 += buffer[4], s2 += s1;
            s1 += buffer[5], s2 += s1;
            s1 += buffer[6], s2 += s1;
            s1 += buffer[7], s2 += s1;

            buffer += 8;
        }

        for (; i < blocklen; ++i)
            s1 += *buffer++, s2 += s1;

        s1 %= ADLER_MOD, s2 %= ADLER_MOD;
        buflen -= blocklen;
        blocklen = 5552;
    }
    return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char *output, const unsigned char *i, unsigned int /*length*/)
{
    if (stb__in4(0) != 0x57bC0000) return 0;
    if (stb__in4(4) != 0)          return 0; // error! stream is > 4GB
    const unsigned int olen = stb_decompress_length(i);
    stb__barrier_in_b = i;
    stb__barrier_out_e = output + olen;
    stb__barrier_out_b = output;
    i += 16;

    stb__dout = output;
    for (;;) {
        const unsigned char *old_i = i;
        i = stb_decompress_token(i);
        if (i == old_i) {
            if (*i == 0x05 && i[1] == 0xfa) {
                IM_ASSERT(stb__dout == output + olen);
                if (stb__dout != output + olen) return 0;
                if (stb_adler32(1, output, olen) != (unsigned int) stb__in4(2))
                    return 0;
                return olen;
            } else {
                IM_ASSERT(0); /* NOTREACHED */
                return 0;
            }
        }
        IM_ASSERT(stb__dout <= output + olen);
        if (stb__dout > output + olen)
            return 0;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Default font data (ProggyClean.ttf)
//-----------------------------------------------------------------------------
// ProggyClean.ttf
// Copyright (c) 2004, 2005 Tristan Grimmer
// MIT license (see License.txt in http://www.proggyfonts.net/index.php?menu=download)
// Download and more information at http://www.proggyfonts.net or http://upperboundsinteractive.com/fonts.php
//-----------------------------------------------------------------------------

#ifndef IMGUI_DISABLE_DEFAULT_FONT

// File: 'ProggyClean.ttf' (41208 bytes)
// Exported using binary_to_compressed_c.exe -u8 "ProggyClean.ttf" proggy_clean_ttf
static const unsigned int gadugi_ttf_compressed_size = 171305;
static const unsigned char gadugi_ttf_compressed_data[171305] =
{
    87,188,0,0,0,0,0,0,0,3,179,252,0,4,0,0,37,0,1,0,0,0,24,130,4,62,4,0,128,68,83,73,71,66,203,54,167,0,3,146,100,0,0,33,152,71,68,69,70,19,90,24,14,0,3,112,12,130,37,40,54,71,80,79,83,
    28,27,47,48,130,15,47,68,0,0,32,30,71,83,85,66,175,169,79,139,0,3,144,130,47,55,1,164,76,84,83,72,112,229,148,209,0,0,22,28,0,0,4,243,77,69,82,71,0,22,130,97,34,3,146,8,130,63,44,12,
    79,83,47,50,121,222,252,80,0,0,2,8,130,15,59,96,86,68,77,88,244,110,222,197,0,0,27,16,0,0,11,186,99,109,97,112,155,59,2,47,0,0,123,130,31,8,45,15,54,99,118,116,32,56,151,52,76,0,0,
    152,224,0,0,1,242,102,112,103,109,189,60,42,255,0,0,138,64,0,0,9,131,103,97,115,112,0,27,0,35,0,3,111,252,130,79,55,16,103,108,121,102,190,98,18,95,0,0,174,148,0,2,149,236,104,100,
    109,120,93,173,118,130,27,45,38,204,0,0,84,60,104,101,97,100,245,88,82,11,130,75,32,140,130,47,33,54,104,130,16,35,16,222,10,241,130,15,32,196,130,15,8,44,36,104,109,116,120,57,208,
    180,202,0,0,2,104,0,0,19,180,107,101,114,110,189,86,188,156,0,3,68,128,0,0,32,190,108,111,99,97,6,161,177,116,0,0,154,212,130,31,40,192,109,97,120,112,9,2,10,188,130,63,32,232,130,
    63,44,32,109,101,116,97,205,3,183,162,0,3,146,20,130,15,43,77,110,97,109,101,72,8,159,50,0,3,101,130,175,41,10,154,112,111,115,116,255,81,0,119,130,175,32,220,130,31,43,32,112,114,
    101,112,54,47,105,96,0,0,147,130,127,35,5,25,0,1,130,21,46,1,33,72,172,103,86,94,95,15,60,245,0,27,8,0,131,0,35,175,245,60,175,131,7,43,221,251,169,85,254,25,254,26,10,32,7,166,130,
    15,32,9,130,47,131,49,130,126,47,0,0,8,162,253,254,0,0,10,132,254,25,253,131,10,32,132,73,130,34,135,2,33,4,235,131,17,43,4,239,0,128,0,16,0,135,0,5,0,2,130,7,42,47,0,101,0,0,3,133,
    9,131,0,3,130,29,36,3,5,33,1,144,130,25,36,8,5,154,5,51,130,82,32,27,133,7,44,3,209,0,102,2,18,8,5,2,11,5,2,4,130,1,34,2,3,128,130,52,33,2,0,131,0,32,48,131,4,55,0,77,83,32,32,0,64,
    0,13,255,255,5,211,254,81,1,13,8,162,2,2,0,0,0,132,153,35,4,0,5,154,130,11,38,32,0,2,5,42,0,166,130,9,38,0,0,199,0,0,2,49,130,3,62,70,0,180,3,35,0,148,4,186,0,33,4,80,0,162,6,140,0,
    80,6,103,0,119,1,215,0,148,2,106,0,140,130,3,48,22,3,86,0,76,5,121,0,232,1,188,0,39,3,51,0,144,130,7,36,112,3,30,255,228,130,47,32,86,130,3,131,7,32,109,130,7,32,123,130,3,32,14,130,
    3,32,164,130,3,32,111,130,3,32,98,130,3,32,90,130,3,32,94,131,47,130,3,36,39,5,121,1,16,131,71,131,7,53,3,150,0,131,7,164,0,172,5,41,0,22,4,150,0,188,4,244,0,94,5,156,130,7,40,12,0,
    188,3,232,0,188,5,125,130,15,44,174,0,188,2,33,0,48,2,219,0,20,4,164,130,23,36,196,0,188,7,47,130,27,40,252,0,188,6,8,0,94,4,123,134,7,32,201,130,55,52,64,0,121,4,49,0,41,5,127,0,170,
    4,248,0,18,7,121,0,26,4,184,130,3,38,108,0,18,4,144,0,33,130,199,36,200,3,8,255,230,130,7,38,53,5,121,0,230,3,82,130,251,48,37,0,82,4,18,0,90,4,180,0,166,3,178,0,96,4,182,130,3,38,
    47,0,96,2,129,0,53,132,11,44,135,0,166,1,240,0,144,1,240,255,58,3,250,132,11,37,166,6,228,0,166,4,130,23,33,4,176,130,43,130,55,131,39,33,2,200,130,63,38,101,0,104,2,182,0,43,130,27,
    50,144,3,213,0,14,5,200,0,24,3,172,0,26,3,223,0,14,3,158,132,127,40,92,1,234,0,172,2,106,0,66,130,127,33,209,5,130,247,132,251,130,247,32,4,130,243,32,5,134,211,132,191,131,147,146,
    3,132,163,130,159,32,4,138,3,130,159,32,88,130,159,32,236,130,3,32,211,130,3,32,201,136,155,131,159,139,3,34,135,0,144,130,27,136,3,43,3,0,0,48,3,4,0,108,4,80,0,184,130,3,46,106,3,
    150,0,131,3,64,0,164,3,170,0,84,4,90,130,207,38,51,0,144,7,31,0,162,131,3,59,6,47,0,76,2,66,0,135,3,80,0,125,5,123,0,232,6,226,0,10,6,8,0,94,6,137,0,82,130,203,32,232,131,19,131,3,
    130,71,44,68,4,158,0,166,4,72,0,78,4,239,0,51,130,35,56,188,4,81,0,190,3,35,0,82,3,114,0,78,6,10,0,100,6,168,0,90,4,176,0,49,130,111,32,143,66,127,7,132,67,34,51,0,123,130,63,32,94,
    130,71,40,209,5,40,0,51,4,12,0,92,130,3,36,90,5,221,0,140,65,43,7,130,79,45,94,7,114,0,94,7,108,0,96,4,0,0,0,8,130,3,130,195,32,106,130,3,36,110,1,213,0,106,130,3,32,110,131,75,47,
    4,171,0,108,3,223,0,14,4,108,0,18,1,98,254,200,130,87,36,102,2,135,0,92,131,3,35,4,112,0,53,131,3,131,255,35,1,188,0,112,130,51,32,88,130,63,36,88,9,174,0,80,131,103,130,115,32,188,
    135,7,131,11,39,2,33,0,48,2,33,255,252,130,3,32,241,130,11,32,27,132,247,32,8,130,251,130,3,35,5,127,0,170,135,3,37,1,240,0,166,2,248,130,99,59,178,0,35,2,123,0,62,2,110,0,37,1,156,
    0,108,2,62,0,82,1,164,0,51,2,152,0,64,1,131,23,38,248,0,102,3,196,0,33,130,251,58,22,4,64,0,121,3,101,0,104,4,144,0,33,3,158,0,33,1,234,0,172,5,156,0,28,4,121,130,223,130,187,132,195,
    38,123,0,188,4,180,0,166,131,215,43,5,121,1,12,2,207,0,127,2,238,0,104,130,3,38,129,7,114,0,127,7,64,130,3,34,158,0,129,130,23,40,47,4,114,0,100,4,193,0,102,130,67,34,188,5,124,130,
    55,42,49,0,41,5,249,0,64,7,45,0,94,130,187,32,48,130,111,32,23,130,15,40,84,4,47,0,48,4,150,0,48,132,231,34,73,0,64,132,243,52,186,0,24,4,182,0,164,5,161,0,2,3,170,0,188,3,198,0,188,
    6,222,130,59,38,121,0,26,4,204,0,94,130,3,38,188,5,186,0,94,7,47,130,27,32,219,130,7,40,55,0,10,6,48,0,114,5,174,130,103,40,4,0,124,6,36,0,8,6,8,130,39,46,104,0,43,5,125,0,94,5,211,
    0,10,5,74,0,201,131,219,37,4,254,0,40,6,227,130,231,130,143,57,8,31,0,84,7,34,0,64,8,210,0,10,8,82,0,92,4,119,0,96,5,127,0,170,7,227,130,135,36,80,0,14,4,156,130,95,40,246,0,24,5,136,
    0,114,4,201,130,91,50,73,0,176,6,130,0,25,4,124,0,121,5,250,0,34,4,145,0,74,132,3,34,248,0,18,130,207,32,121,130,87,34,94,6,64,130,59,32,2,68,31,5,33,4,244,130,123,34,158,0,44,130,
    187,46,24,4,123,0,188,5,184,0,94,6,71,0,32,5,61,130,75,32,164,130,3,32,126,130,187,32,79,130,19,40,130,0,94,8,163,0,64,6,8,130,11,32,221,130,51,32,62,132,127,32,110,131,159,33,4,233,
    130,187,34,137,0,20,131,191,33,6,202,130,211,35,150,0,188,0,150,0,39,5,121,0,232,5,234,0,40,139,3,35,6,38,0,100,130,3,32,70,134,3,131,11,131,3,32,40,130,15,131,39,32,10,134,43,140,
    7,39,7,16,0,10,7,76,0,100,134,7,132,59,130,15,32,10,130,15,32,40,135,7,131,63,45,4,176,0,100,4,24,0,100,3,149,0,100,2,179,133,3,33,3,69,130,15,131,3,130,19,131,23,33,3,193,130,23,36,
    60,0,100,1,168,133,39,132,19,130,27,43,8,27,0,40,6,83,0,40,7,108,0,100,131,11,211,183,147,179,130,127,34,106,1,68,130,147,38,249,0,120,5,194,0,100,139,3,35,6,98,0,100,152,3,36,172,
    0,10,6,172,130,31,142,7,132,99,34,76,0,100,155,7,130,115,34,100,6,162,130,59,130,3,33,7,66,130,43,130,3,35,4,61,0,100,158,3,36,72,5,39,0,10,130,3,32,100,174,7,132,55,33,2,130,130,227,
    130,3,33,5,29,130,227,138,3,158,115,131,147,196,119,231,103,135,231,131,111,65,187,24,65,215,10,65,167,11,131,47,131,15,135,55,39,3,149,0,100,3,152,0,115,131,7,175,75,130,71,66,3,8,
    134,87,32,100,135,7,135,99,135,7,132,103,130,107,131,31,33,3,233,130,115,157,3,36,72,4,211,0,10,130,3,32,100,143,7,130,59,131,27,32,100,135,7,142,31,132,55,37,2,130,0,100,2,236,66,
    11,5,131,119,132,15,66,27,5,35,91,8,128,0,132,3,35,6,91,0,91,132,3,35,26,0,100,6,134,3,35,7,189,0,100,140,3,36,4,0,10,7,4,130,19,142,7,35,8,167,0,10,130,3,32,100,151,7,132,235,133,
    223,32,39,154,235,152,227,142,211,65,3,15,136,227,130,103,38,99,3,129,0,90,3,24,67,195,25,35,4,167,0,100,138,3,32,99,130,11,132,3,43,5,144,0,10,5,145,0,99,2,176,0,100,131,71,37,6,98,
    0,100,5,88,141,3,66,15,15,66,71,7,130,47,68,67,12,68,79,27,147,51,135,87,33,8,153,68,139,9,135,19,33,5,238,130,19,150,3,33,3,62,130,203,32,254,130,175,131,239,32,137,130,243,150,3,
    33,4,206,68,103,8,32,99,67,147,6,32,99,143,87,35,8,78,0,100,135,3,68,227,6,136,15,33,4,117,130,91,32,171,130,67,32,111,132,3,33,99,4,134,7,65,15,15,67,51,27,131,27,33,4,102,130,63,
    150,3,33,2,230,130,27,32,118,130,203,32,70,141,3,135,175,130,7,32,98,130,3,34,99,4,161,130,39,33,161,0,131,7,32,98,130,11,34,99,4,166,130,15,138,3,35,6,41,0,95,130,3,32,200,135,7,70,
    83,7,35,6,93,0,155,134,3,32,120,130,7,71,31,8,138,23,131,31,171,23,32,132,130,59,38,40,6,119,0,117,6,98,130,3,35,88,0,200,6,137,3,32,95,132,23,130,27,35,5,244,0,100,138,3,136,23,35,
    6,58,0,120,138,3,40,95,3,18,0,60,7,64,0,117,131,3,35,5,104,0,145,138,3,32,100,136,23,35,159,0,200,5,137,3,137,23,35,144,0,165,5,137,3,36,100,3,86,0,50,130,115,32,150,130,3,131,99,32,
    85,130,103,135,3,34,95,7,61,130,99,130,3,35,5,39,0,80,138,3,32,100,130,203,32,150,130,3,36,110,6,103,0,160,138,3,35,130,5,247,0,136,3,35,7,83,0,200,142,3,34,130,5,225,130,71,38,225,
    0,110,7,37,0,70,138,3,34,130,6,81,130,223,36,81,0,150,6,157,130,79,137,3,32,95,130,15,131,19,36,150,6,177,0,55,136,3,34,157,0,95,130,123,131,131,36,100,6,198,0,200,138,3,36,95,6,183,
    0,100,130,3,32,70,130,19,139,3,36,95,7,75,0,80,131,3,35,6,173,0,70,138,3,34,100,6,73,130,47,131,3,32,102,130,7,137,3,36,95,3,14,0,120,131,3,33,6,22,149,31,35,7,30,0,100,131,3,35,5,
    138,0,150,138,3,132,19,132,23,32,222,130,23,137,3,34,100,2,236,133,47,132,27,35,236,0,110,5,137,3,137,51,32,98,130,23,137,3,137,23,32,181,130,23,137,3,34,100,5,66,67,195,9,37,8,78,
    0,100,10,132,133,3,35,8,226,0,100,131,3,135,15,37,4,211,0,100,5,80,133,3,70,11,6,136,15,70,79,8,73,235,5,40,40,6,38,0,40,7,76,0,10,73,251,6,34,40,7,16,130,11,32,98,68,7,5,39,5,39,0,
    10,4,61,255,230,67,191,11,69,151,7,69,239,7,35,8,167,0,10,135,19,35,4,167,0,99,72,219,27,70,247,10,32,10,130,11,35,10,5,144,0,136,3,135,151,70,115,10,74,43,5,32,130,130,243,32,192,
    130,143,32,27,130,139,36,27,0,100,2,112,74,31,5,130,63,32,100,130,111,32,70,130,3,36,99,6,172,0,100,131,15,131,7,135,171,33,5,39,134,195,32,89,130,7,42,111,0,100,6,41,0,95,8,43,0,117,
    67,47,6,46,100,6,52,0,100,5,247,0,130,7,237,0,130,2,105,130,87,32,191,130,103,32,30,130,91,8,32,188,0,112,1,0,0,0,4,222,0,80,4,108,0,56,5,43,0,90,4,105,0,166,5,107,0,96,4,61,0,166,
    4,36,130,15,32,102,130,7,8,40,67,0,166,3,72,0,30,4,202,0,80,5,126,0,96,2,33,0,48,3,101,0,20,4,144,0,70,3,117,0,76,3,223,0,33,3,192,0,10,2,252,130,15,32,86,130,63,34,42,0,10,130,15,
    34,110,4,186,130,19,32,228,130,59,32,35,130,19,130,211,49,5,200,0,24,3,219,0,86,3,165,0,143,4,66,0,96,5,158,130,27,43,162,0,120,5,6,0,20,6,129,0,99,4,131,15,32,151,130,115,36,170,0,
    38,4,176,130,131,32,68,130,115,32,60,130,7,32,139,130,107,32,135,130,43,36,158,0,33,4,39,130,55,34,239,0,96,131,143,33,7,7,130,143,52,89,0,10,7,220,0,10,6,86,0,80,3,145,0,87,4,135,
    0,145,7,203,130,135,51,94,0,56,4,8,0,166,2,144,255,236,5,78,0,86,4,6,0,164,3,130,119,41,5,165,0,20,3,184,0,104,4,5,130,155,42,200,0,12,2,248,0,60,3,213,0,14,130,207,34,104,5,157,130,
    107,40,166,0,46,3,205,0,143,3,16,80,111,5,39,5,114,0,20,5,27,0,11,131,75,33,4,71,130,123,36,0,0,31,5,21,130,27,32,250,130,3,36,196,0,86,6,53,130,19,36,105,0,96,6,158,130,163,130,179,
    33,5,193,130,67,39,55,0,40,3,185,0,106,7,130,159,35,4,248,0,18,136,3,32,114,130,11,42,249,0,166,5,48,0,94,5,107,0,94,130,243,32,114,131,3,37,5,59,0,49,5,141,130,3,38,127,0,147,5,4,
    0,188,130,3,32,18,132,7,43,37,0,110,6,94,0,80,6,125,255,236,6,78,207,6,35,4,239,0,166,130,3,44,20,5,28,0,90,5,240,0,95,5,245,0,40,131,3,35,4,235,0,80,133,3,35,255,216,5,171,81,99,5,
    35,3,252,0,72,130,159,32,136,131,3,39,4,180,0,60,4,242,0,33,132,235,131,239,131,3,36,79,0,20,3,237,130,79,32,82,130,179,32,139,130,3,32,228,134,3,36,54,0,90,4,111,130,3,36,53,0,101,
    4,32,130,31,34,32,0,39,132,7,43,0,0,106,5,44,0,100,4,194,255,236,4,130,235,131,3,35,3,207,0,166,130,3,34,20,4,14,130,127,32,173,130,51,35,157,0,70,4,130,3,130,123,32,90,133,3,49,255,
    250,5,40,0,30,4,70,0,144,3,98,0,70,5,56,0,152,131,3,35,3,197,0,50,130,3,36,33,0,0,255,128,130,3,36,2,0,0,254,221,130,7,56,158,5,196,0,86,4,47,0,82,5,41,0,22,4,18,0,90,2,33,0,48,1,240,
    0,55,131,255,131,111,135,7,35,4,12,0,188,130,39,40,96,5,127,0,170,4,135,0,144,131,35,130,63,34,67,3,233,130,155,34,5,255,236,73,83,9,33,255,231,130,19,36,231,5,154,0,100,146,3,34,72,
    5,11,130,23,130,3,35,6,20,0,200,131,3,39,2,249,0,95,2,132,0,75,131,3,37,3,44,0,120,3,162,130,11,50,168,0,140,3,130,0,90,3,18,0,60,1,154,0,140,2,222,0,130,130,27,33,95,0,131,0,49,51,
    0,51,0,10,254,25,0,0,4,239,25,1,37,1,1,1,34,130,3,132,2,34,12,1,12,132,7,133,4,32,12,134,12,40,12,13,1,1,12,44,1,1,36,130,22,32,3,132,34,35,13,9,34,39,137,42,37,12,1,46,1,12,46,130,
    44,38,12,1,13,12,12,12,46,130,38,33,12,25,130,52,32,22,133,70,36,13,1,3,12,9,138,86,131,10,130,38,134,2,133,24,45,13,35,1,42,42,1,1,36,28,1,12,42,1,28,130,5,32,15,132,23,33,12,4,132,
    130,38,31,1,28,39,1,1,14,130,133,33,1,8,131,23,33,25,25,130,5,33,12,27,132,34,34,1,12,25,140,178,34,12,9,9,130,108,32,47,136,19,130,53,32,1,130,136,131,203,32,12,130,20,33,16,16,135,
    25,48,13,3,35,1,1,3,1,1,1,43,1,45,46,1,13,1,1,130,98,34,45,1,29,130,16,131,4,37,44,1,32,1,5,19,65,25,5,32,9,130,19,33,2,48,130,39,34,34,3,1,130,17,34,19,1,1,131,46,32,1,131,139,33,
    9,30,130,12,65,65,5,32,32,65,59,12,140,239,34,48,1,48,130,15,132,5,43,43,49,41,41,12,12,49,49,1,35,1,49,130,5,131,67,140,42,135,39,131,44,32,49,133,25,32,48,133,0,142,38,130,28,33,
    28,28,142,19,197,14,133,110,130,5,133,139,35,48,49,11,49,139,18,138,161,33,49,49,131,215,149,117,33,27,1,134,190,131,30,32,47,130,0,141,38,138,28,141,24,131,54,133,110,32,50,132,0,
    130,29,133,101,131,20,32,1,135,84,136,12,39,48,48,21,1,1,48,48,30,133,0,35,1,5,1,42,133,0,132,84,131,20,40,14,14,14,1,1,14,14,36,1,130,130,32,47,131,54,134,204,33,49,31,133,0,32,41,
    66,70,6,132,48,33,1,38,130,0,133,9,32,23,130,0,33,1,1,144,5,153,240,142,198,135,14,33,23,23,134,9,32,7,131,0,33,18,18,133,13,32,27,132,0,130,11,32,27,136,31,130,12,133,29,33,50,50,
    133,35,32,24,134,7,137,88,136,170,131,61,135,22,32,42,130,223,137,34,33,1,49,66,53,8,130,20,130,239,32,1,130,248,130,9,142,75,132,37,132,41,34,42,1,1,132,112,33,48,48,130,37,67,54,
    8,32,17,66,204,5,36,40,1,1,29,31,130,23,32,9,130,3,130,2,34,5,1,25,67,17,5,34,29,1,32,133,17,32,19,130,6,34,23,1,22,134,152,32,38,130,13,32,1,130,51,133,27,34,25,1,1,132,16,131,13,
    35,49,2,1,1,131,250,36,38,30,1,34,34,68,110,6,33,40,1,133,86,32,46,131,32,32,19,134,20,33,9,4,67,224,5,130,60,33,1,23,139,145,130,11,40,1,1,14,14,4,4,4,39,1,141,241,132,25,34,1,1,9,
    68,186,11,130,19,33,24,24,132,24,67,160,6,133,6,37,0,0,1,0,2,0,130,139,130,26,32,0,130,0,48,18,5,230,0,248,8,255,0,8,0,8,255,254,0,9,0,11,130,5,32,10,132,5,34,11,0,12,130,11,38,12,
    0,13,255,253,0,13,132,5,34,14,0,14,130,11,38,15,0,15,255,252,0,16,132,5,34,17,0,16,130,11,34,18,0,17,130,5,34,19,0,18,130,5,40,20,0,19,255,251,0,21,0,20,130,5,34,22,0,21,130,5,34,23,
    0,22,130,5,32,24,130,5,42,250,0,25,0,23,255,250,0,26,0,24,130,5,34,27,0,25,130,5,34,28,0,27,130,5,32,29,130,5,42,249,0,30,0,28,255,249,0,31,0,30,130,5,34,32,0,32,130,5,38,33,0,33,255,
    248,0,34,132,5,34,35,0,34,130,11,34,36,0,35,130,5,38,37,0,36,255,247,0,38,132,5,34,39,0,37,130,11,34,40,0,38,130,5,34,41,0,39,130,5,38,42,0,41,255,246,0,43,132,5,34,44,0,42,130,11,
    34,45,0,43,130,5,40,46,0,44,255,245,0,47,0,45,130,5,32,48,132,5,34,49,0,46,130,11,46,50,0,47,255,244,0,51,0,49,255,243,0,52,0,50,130,5,34,53,0,51,130,5,34,54,0,52,130,5,40,55,0,53,
    255,242,0,56,0,54,130,5,32,57,132,5,34,58,0,55,130,11,40,59,0,56,255,241,0,60,0,58,130,5,34,61,0,59,130,5,32,62,132,5,34,63,0,60,130,11,40,64,0,61,255,240,0,65,0,62,130,5,34,66,0,63,
    130,5,32,67,132,5,40,68,0,64,255,239,0,69,0,66,130,5,34,70,0,67,130,5,34,71,0,68,130,5,40,72,0,69,255,238,0,73,0,70,130,5,34,74,0,71,130,5,34,75,0,72,130,5,34,76,0,73,130,5,32,77,130,
    5,42,237,0,78,0,75,255,237,0,79,0,76,130,5,34,80,0,77,130,5,38,81,0,78,255,236,0,82,132,5,34,83,0,79,130,11,34,84,0,80,130,5,40,85,0,81,255,235,0,86,0,82,130,5,34,87,0,83,130,5,34,
    88,0,84,130,5,34,89,0,85,130,5,40,90,0,86,255,234,0,91,0,87,130,5,34,92,0,88,130,5,34,93,0,89,130,5,40,94,0,90,255,233,0,95,0,91,130,5,34,96,0,93,130,5,32,97,132,5,40,98,0,94,255,232,
    0,99,0,95,130,5,34,100,0,96,130,5,34,101,0,97,130,5,32,102,132,5,40,103,0,98,255,231,0,104,0,99,130,5,34,105,0,100,130,5,34,106,0,102,130,5,32,107,130,5,42,230,0,108,0,103,255,230,
    0,109,0,104,130,5,34,110,0,105,130,5,38,111,0,106,255,229,0,112,132,5,34,113,0,108,130,11,34,114,0,109,130,5,34,115,0,111,130,5,38,116,0,112,255,228,0,117,132,5,34,118,0,113,130,11,
    34,119,0,114,130,5,40,120,0,115,255,227,0,121,0,116,130,5,32,122,132,5,34,123,0,117,130,11,40,124,0,119,255,226,0,125,0,120,130,5,34,126,0,121,130,5,32,127,132,5,34,128,0,122,130,11,
    40,129,0,123,255,225,0,130,0,124,130,5,34,131,0,125,130,5,32,132,132,5,40,133,0,128,255,224,0,134,0,129,130,5,34,135,0,130,130,5,34,136,0,131,130,5,32,137,130,5,48,223,0,138,0,132,
    255,223,0,139,0,133,255,222,0,140,0,134,130,5,34,141,0,135,130,5,34,142,0,136,130,5,40,143,0,137,255,221,0,144,0,138,130,5,34,145,0,139,130,5,32,146,132,5,40,147,0,140,255,220,0,148,
    0,141,130,5,34,149,0,142,130,5,34,150,0,143,130,5,40,151,0,144,255,219,0,152,0,145,130,5,34,153,0,146,130,5,34,154,0,148,130,5,38,155,0,149,255,218,0,156,132,5,34,157,0,150,130,11,
    34,158,0,151,130,5,34,159,0,152,130,5,38,160,0,154,255,217,0,161,132,5,34,162,0,155,130,11,34,163,0,156,130,5,40,164,0,157,255,216,0,165,0,158,130,5,32,166,132,5,34,167,0,159,130,11,
    40,168,0,160,255,215,0,169,0,161,130,5,34,170,0,163,130,5,32,171,132,5,40,172,0,164,255,214,0,173,0,165,130,5,34,174,0,167,130,5,34,175,0,168,130,5,32,176,132,5,40,177,0,169,255,213,
    0,178,0,170,130,5,34,179,0,172,130,5,34,180,0,173,130,5,32,181,130,5,42,212,0,182,0,174,255,212,0,183,0,175,130,5,34,184,0,176,130,5,38,185,0,177,255,211,0,186,132,5,34,187,0,178,130,
    11,34,188,0,180,130,5,34,189,0,181,130,5,38,190,0,182,255,210,0,191,132,5,34,192,0,183,130,11,34,193,0,184,130,5,40,194,0,185,255,209,0,195,0,187,130,5,32,196,132,5,34,197,0,189,130,
    11,40,198,0,190,255,208,0,199,0,191,130,5,34,200,0,192,130,5,32,201,132,5,34,202,0,193,130,11,40,203,0,194,255,207,0,204,0,195,130,5,34,205,0,196,130,5,34,206,0,197,130,5,37,207,0,
    198,255,206,0,130,55,130,5,34,209,0,200,130,5,34,210,0,201,130,5,32,211,130,5,42,205,0,212,0,202,255,205,0,213,0,203,130,5,34,214,0,204,130,5,34,215,0,207,130,5,32,216,130,5,42,204,
    0,217,0,208,255,204,0,218,0,209,130,5,34,219,0,210,130,5,38,220,0,211,255,203,0,221,132,5,34,222,0,212,130,11,34,223,0,213,130,5,40,224,0,215,255,202,0,225,0,216,130,5,32,226,132,5,
    34,227,0,217,130,11,34,228,0,218,130,5,40,229,0,219,255,201,0,230,0,220,130,5,32,231,132,5,40,232,0,221,255,200,0,233,0,222,130,17,34,234,0,224,130,11,40,235,0,225,255,199,0,236,0,
    226,130,5,34,237,0,227,130,17,34,238,0,228,130,5,34,239,0,229,130,17,32,240,130,5,42,198,0,241,0,230,255,198,0,242,0,231,130,17,34,243,0,233,130,11,38,244,0,234,255,197,0,245,132,5,
    34,246,0,235,130,17,34,247,0,236,130,17,40,248,0,237,255,196,0,249,0,238,130,5,32,250,130,5,36,197,0,251,0,239,130,11,40,252,0,241,255,195,0,253,0,242,130,5,34,254,0,243,130,5,32,255,
    132,5,16,5,211,5,211,32,0,130,0,58,17,0,0,4,244,11,14,7,0,1,3,3,4,7,6,9,9,3,3,3,5,8,3,4,3,4,6,136,0,45,3,3,8,8,8,5,11,7,7,7,8,6,6,8,130,28,47,6,5,10,8,9,6,9,7,6,5,8,7,11,6,5,6,130,
    46,48,8,5,3,6,7,5,7,6,4,7,7,3,3,6,3,9,7,130,0,40,4,5,4,7,5,9,5,5,5,130,83,32,8,130,15,34,6,8,9,130,64,131,86,32,5,131,4,130,21,32,3,130,21,134,2,55,4,4,6,6,5,4,5,6,4,10,10,9,3,5,8,
    9,9,9,8,8,8,6,7,6,130,115,36,4,5,9,9,7,130,68,34,8,7,6,130,2,130,119,39,7,9,10,10,6,11,4,4,130,152,36,6,5,5,2,6,130,113,37,6,4,3,3,4,13,130,46,33,6,6,131,87,133,62,53,3,4,4,3,3,2,3,
    2,4,2,4,5,3,6,5,6,5,3,8,7,5,5,130,82,39,8,4,4,4,10,10,10,4,131,10,41,5,8,10,3,6,8,6,6,7,6,130,5,42,8,5,5,9,11,7,7,8,10,5,10,130,171,35,8,8,6,8,130,113,41,7,9,6,11,10,12,11,6,8,11,130,
    162,130,15,32,9,133,46,37,9,9,6,5,7,8,130,12,39,9,7,6,6,10,9,12,8,130,7,38,11,7,8,7,9,7,0,132,0,32,8,144,0,130,114,33,10,8,132,4,130,72,34,4,4,5,131,0,33,3,2,130,5,35,11,9,10,11,148,
    45,132,44,34,5,2,4,131,28,130,211,137,2,131,24,132,3,36,5,9,9,10,10,65,170,8,65,79,9,132,9,33,3,3,131,6,153,29,153,25,34,3,3,6,136,108,130,103,37,9,10,9,9,5,5,130,107,134,20,132,122,
    32,10,130,21,32,10,131,4,130,24,132,203,131,4,132,119,33,7,5,130,1,132,9,42,3,4,3,5,3,3,3,12,12,9,9,130,195,32,11,130,0,133,58,82,171,6,32,12,136,55,32,5,143,186,34,5,5,4,133,108,133,
    164,36,8,8,4,5,9,131,35,131,20,34,10,10,5,65,46,5,136,12,36,9,9,12,8,8,132,99,131,24,33,4,7,134,59,33,9,7,131,61,131,18,130,121,37,9,9,11,11,6,9,131,16,131,54,134,204,32,5,133,94,34,
    6,4,6,131,138,65,16,8,130,8,137,97,131,89,32,9,130,191,131,54,133,11,133,153,133,111,133,11,35,4,10,10,7,130,0,34,10,10,8,130,0,133,5,130,163,133,159,135,182,132,47,132,255,34,8,8,
    10,130,0,65,209,12,138,12,33,10,10,137,12,34,4,4,8,132,12,139,88,32,4,135,12,137,113,130,240,38,11,14,14,12,12,14,14,65,71,6,32,5,130,125,33,8,10,130,103,35,9,6,7,6,130,65,39,5,5,11,
    11,12,5,5,6,66,56,8,132,67,48,5,5,3,3,3,5,3,5,6,6,3,2,10,6,6,9,10,130,41,32,7,130,48,43,8,11,7,7,9,8,11,3,4,4,3,1,131,15,33,7,6,130,0,34,5,7,8,130,41,43,5,5,5,4,5,7,5,7,4,4,7,8,130,
    79,37,8,5,7,9,6,5,65,55,5,49,5,6,8,5,10,7,11,9,5,6,11,5,6,4,7,6,5,8,130,6,32,4,130,137,32,6,131,50,44,7,6,6,7,7,5,5,9,7,9,6,8,6,130,39,32,7,130,88,38,7,7,9,9,7,8,8,65,32,5,132,7,32,
    8,134,5,35,5,9,9,6,130,45,32,5,130,98,130,54,133,95,132,8,131,16,33,6,6,130,23,130,95,131,79,32,0,130,0,38,8,6,7,6,3,3,8,130,87,39,6,6,8,7,3,0,5,6,130,31,66,34,6,39,7,7,8,8,4,3,3,4,
    130,130,35,4,2,4,4,130,45,132,2,36,12,16,8,0,1,130,251,39,7,6,10,10,3,4,4,5,130,218,32,3,134,198,131,108,130,77,39,8,8,5,11,8,7,8,8,130,82,37,9,3,4,7,6,11,130,170,32,9,130,179,34,8,
    7,11,68,220,5,35,8,5,3,6,130,15,68,243,7,131,21,38,7,4,5,4,7,6,9,130,127,34,4,3,4,130,202,34,8,6,9,68,243,7,132,194,68,243,13,131,202,130,3,43,7,5,11,11,9,3,5,8,10,9,10,8,130,110,130,
    82,32,9,130,191,35,9,10,7,5,131,133,32,6,130,18,41,6,9,8,8,9,11,11,6,12,5,131,18,130,32,37,2,6,4,4,7,7,130,12,33,5,15,132,245,68,243,12,33,4,4,68,243,5,35,6,3,6,5,130,35,33,8,7,130,
    48,130,199,37,4,4,4,11,11,11,130,54,41,8,8,7,9,11,3,6,9,6,7,130,219,32,9,131,4,32,10,130,187,33,9,11,131,225,130,175,55,7,8,9,8,7,7,10,6,12,11,13,12,7,8,12,6,7,6,8,7,6,10,7,9,130,244,
    38,6,10,9,6,6,8,8,130,12,131,34,34,11,10,13,130,177,34,6,12,7,131,17,32,0,132,0,66,171,5,66,189,9,34,9,9,11,130,0,133,4,37,7,6,5,4,4,5,130,0,34,6,3,2,130,5,35,12,9,11,12,67,0,16,32,
    11,134,0,35,9,5,2,4,67,59,5,66,175,10,134,27,39,11,11,5,10,10,11,11,6,135,0,69,56,14,33,4,4,131,16,153,29,153,25,33,4,4,67,29,9,130,107,35,10,11,10,10,130,167,136,128,133,126,130,21,
    33,11,11,130,4,33,5,5,69,117,15,35,6,7,6,7,132,0,32,4,130,230,130,3,35,13,13,10,10,130,195,32,12,130,0,133,58,32,13,134,0,66,102,9,68,243,14,32,4,136,110,133,24,35,8,8,4,5,68,79,6,
    131,241,32,5,130,77,130,2,131,144,33,11,11,130,12,36,10,10,13,9,9,132,99,130,21,35,9,5,7,5,134,170,32,7,68,226,7,130,121,37,10,10,12,12,7,10,68,39,5,130,206,133,33,32,5,133,94,35,7,
    4,7,5,130,0,131,45,134,16,32,7,131,72,131,83,131,85,68,161,5,137,5,133,59,133,111,133,5,35,5,11,11,8,130,0,139,5,134,163,133,18,133,49,130,83,132,255,66,11,6,130,177,65,82,8,65,220,
    12,137,105,32,5,130,69,133,229,133,94,130,65,36,9,4,11,11,9,130,0,133,88,133,11,45,8,5,10,12,16,16,13,13,16,16,7,8,8,7,130,3,131,55,33,9,11,130,103,35,10,6,8,6,130,64,39,6,6,12,12,
    13,6,6,7,66,56,8,132,61,48,5,5,4,4,4,5,4,6,6,6,4,2,11,7,7,10,11,130,41,42,8,6,8,7,9,12,8,8,9,9,12,130,27,49,3,2,7,7,8,7,8,6,6,7,6,5,7,8,3,5,7,5,130,43,50,5,8,6,7,4,5,8,9,6,5,6,8,5,
    8,10,7,5,7,7,130,32,130,26,45,9,5,11,8,12,10,5,7,12,5,6,4,8,6,130,38,34,6,4,4,130,6,130,59,35,6,8,8,6,132,3,32,9,130,47,36,9,6,6,11,7,131,0,36,8,8,10,10,8,133,0,39,10,10,9,9,7,7,8,
    9,131,5,37,7,9,8,6,9,9,131,84,130,97,32,6,130,40,32,6,130,95,32,6,130,63,131,10,32,6,132,16,130,12,130,88,35,8,6,6,0,130,0,40,9,6,8,6,3,3,9,7,9,130,38,35,8,7,3,0,130,42,70,180,8,131,
    200,32,4,130,200,38,5,4,5,5,2,4,5,130,45,132,2,36,13,17,8,0,1,130,22,36,8,7,11,10,3,130,7,35,9,3,5,3,71,119,10,130,77,35,9,9,6,12,130,249,33,9,7,130,138,32,3,130,202,41,12,10,10,7,
    10,8,7,7,9,8,130,22,44,7,4,5,4,9,5,3,7,8,6,8,7,4,130,114,39,3,6,3,11,7,8,8,8,130,247,130,47,130,124,35,4,3,4,9,130,15,32,7,130,214,132,233,67,16,5,32,3,130,0,131,38,32,8,130,71,35,
    7,7,5,5,130,226,48,5,6,7,5,12,12,10,4,5,9,11,10,11,9,9,9,7,130,115,44,10,7,5,6,10,11,8,6,4,4,9,8,7,130,2,130,3,39,8,10,12,12,7,13,5,5,130,152,36,8,6,7,2,7,68,243,7,32,16,130,46,33,
    7,7,130,86,33,3,10,130,106,37,9,9,3,5,4,4,130,123,44,3,4,2,5,6,3,7,6,7,6,3,9,7,130,6,36,8,9,9,5,5,130,103,34,12,5,7,130,10,39,7,10,12,3,7,10,7,7,132,205,44,9,6,6,11,12,8,8,9,12,6,12,
    10,9,130,175,32,7,132,132,40,11,7,13,12,14,14,7,9,13,130,61,130,129,32,11,133,46,41,11,10,7,6,8,9,11,7,9,10,130,19,34,12,11,14,130,177,33,7,13,130,80,32,11,73,231,6,66,208,17,36,11,
    12,11,12,10,130,3,45,11,10,8,7,6,4,4,5,5,6,6,6,4,3,130,4,35,13,10,12,13,66,252,16,131,45,132,44,34,6,2,5,68,243,10,66,180,5,73,39,7,37,12,6,11,11,12,12,65,85,5,130,5,68,243,20,153,
    29,153,25,34,4,4,7,136,144,130,107,35,10,12,10,10,130,167,139,18,133,21,32,12,130,208,130,25,68,243,9,133,129,34,6,8,6,133,8,34,4,5,4,130,233,34,4,14,14,132,58,32,13,130,0,133,191,
    32,14,134,0,66,157,5,73,140,7,138,190,35,4,6,6,5,132,51,67,19,6,36,9,9,4,6,10,65,6,7,33,12,12,74,133,6,131,33,132,12,35,10,10,14,9,65,112,9,34,5,8,5,133,102,33,11,8,68,226,7,130,111,
    37,11,11,14,14,7,11,135,16,32,6,133,93,69,155,6,34,7,5,7,74,165,7,32,8,67,132,8,136,238,144,8,32,11,132,17,32,11,132,6,133,5,34,5,12,12,131,157,139,5,134,191,33,12,12,131,80,65,58,
    13,34,10,10,12,130,0,33,10,10,134,165,130,6,139,11,33,12,12,75,16,9,32,5,136,69,133,94,131,71,130,107,133,248,137,113,38,9,5,11,14,17,17,14,130,3,35,8,9,9,8,130,3,32,6,131,223,39,12,
    10,10,11,10,7,8,7,130,45,39,6,6,13,13,14,6,6,8,66,56,8,132,169,48,6,6,4,4,4,6,4,6,7,7,4,3,12,8,8,11,12,130,209,35,8,7,9,7,70,65,5,38,13,4,4,5,3,2,8,130,64,33,9,7,130,0,45,5,8,9,3,6,
    7,6,6,6,5,5,8,6,8,130,4,39,9,6,6,7,9,6,8,11,130,13,32,8,130,31,60,7,6,7,10,5,11,9,13,10,6,7,13,5,7,4,9,7,6,9,6,7,5,5,6,6,9,8,6,5,130,4,35,7,7,8,8,130,150,34,9,11,8,130,26,35,11,8,8,
    8,130,15,35,9,11,11,9,131,176,33,8,8,130,169,33,10,8,130,6,34,10,10,8,132,191,131,7,32,6,130,107,130,155,130,41,66,169,5,130,25,130,65,130,54,130,5,36,6,8,7,6,8,130,7,32,0,130,0,40,
    9,7,8,7,3,3,10,8,10,130,134,35,9,7,3,0,132,165,66,34,6,36,8,8,10,10,5,130,200,130,229,34,5,3,5,68,243,8,43,15,20,10,0,1,4,4,6,9,8,12,12,130,22,36,6,10,3,6,3,66,179,6,131,101,130,77,
    37,10,10,7,14,10,9,130,173,44,7,10,11,4,5,9,7,13,11,11,8,11,9,130,78,56,9,14,9,8,9,5,6,5,10,6,4,8,9,7,9,8,5,9,8,4,4,7,4,13,8,130,201,130,22,34,8,7,11,130,173,41,5,4,5,10,10,10,9,8,
    11,11,66,192,6,131,181,130,39,33,4,4,131,38,133,242,32,6,78,70,5,42,8,6,13,13,12,4,6,10,13,11,12,130,50,32,8,130,92,44,11,8,6,6,11,12,9,7,4,4,10,10,8,130,2,42,8,11,10,10,11,14,14,8,
    15,6,6,130,152,130,237,35,3,8,5,5,130,252,35,3,3,6,18,130,32,32,8,132,87,32,11,130,106,48,10,10,4,6,5,5,5,3,4,3,5,3,6,7,4,8,6,130,67,56,11,8,8,7,8,9,10,10,5,6,6,14,14,14,5,8,9,11,10,
    8,11,13,4,8,11,130,20,35,8,8,11,9,130,173,42,13,14,9,9,11,13,7,14,12,11,8,130,2,45,10,11,10,9,9,13,8,15,13,17,16,8,10,15,130,226,38,10,9,8,12,8,11,9,130,180,35,13,12,8,7,130,41,32,
    8,130,169,41,9,8,14,12,16,11,13,10,8,15,130,91,34,13,9,0,132,0,33,10,11,130,0,66,98,6,66,219,5,36,13,14,13,14,12,130,3,37,13,12,9,8,7,5,130,131,36,7,7,7,4,3,130,4,35,15,12,14,15,148,
    45,132,44,34,7,2,6,138,28,32,13,132,0,68,50,6,38,14,14,7,12,12,14,14,68,43,8,67,156,14,67,48,5,153,29,153,25,33,5,5,67,29,9,38,14,14,14,12,14,12,12,130,167,134,205,33,12,12,133,126,
    130,21,132,26,33,7,7,69,117,9,73,29,5,33,7,9,73,38,6,42,5,5,5,7,5,5,5,16,16,12,12,130,195,32,15,130,0,132,190,33,13,16,134,0,66,203,5,131,5,74,231,14,35,5,7,7,6,133,110,133,24,36,10,
    10,5,7,12,67,233,5,131,126,32,7,69,250,9,33,14,14,130,12,36,12,12,16,11,11,132,99,131,24,33,6,9,134,59,33,12,9,131,226,131,18,130,111,32,13,130,117,33,8,13,131,16,131,88,32,7,133,93,
    32,7,65,3,6,33,5,8,69,120,7,133,111,33,9,9,74,148,5,149,5,133,59,133,111,133,11,32,6,69,214,5,33,14,14,131,108,133,11,32,6,131,145,131,172,135,182,132,47,32,14,131,0,34,11,11,13,130,
    0,133,57,33,12,12,133,10,65,24,5,131,5,33,14,14,137,29,34,6,6,11,132,38,33,13,13,131,69,33,13,13,131,94,32,5,133,6,139,18,45,10,6,12,16,20,20,17,17,20,20,9,10,10,9,130,3,32,7,131,223,
    32,14,130,91,46,12,8,10,8,12,12,12,7,7,15,15,16,7,7,9,132,126,133,4,130,80,52,7,7,5,5,5,7,5,7,8,8,5,3,14,9,9,13,14,13,12,12,10,131,48,43,15,10,10,12,11,15,5,5,6,3,2,9,130,15,33,10,
    8,130,0,8,32,6,9,10,4,6,9,6,7,7,6,6,10,7,9,5,6,10,11,7,7,8,11,7,9,12,9,7,9,9,8,8,9,8,130,13,59,6,13,10,15,12,7,8,15,6,8,5,10,8,7,11,7,8,5,6,7,6,11,9,7,6,7,10,10,130,34,32,10,130,150,
    39,10,12,9,11,8,7,13,9,130,51,130,167,36,12,12,10,10,10,130,11,39,10,12,12,11,11,9,9,10,130,223,36,9,9,9,11,10,130,190,34,9,9,7,130,107,33,7,8,69,20,6,35,8,8,8,10,130,10,74,11,8,35,
    10,8,6,10,130,79,32,0,130,0,41,11,8,10,8,4,4,11,9,11,9,130,35,36,8,4,0,7,8,130,57,66,34,6,36,9,9,11,11,6,130,200,38,7,5,7,6,3,5,6,130,45,132,2,39,16,21,10,0,2,4,5,6,130,244,40,13,4,
    5,5,7,11,3,6,3,81,206,9,40,9,3,3,11,11,11,7,15,10,130,144,35,8,8,11,11,130,247,43,8,14,12,12,9,12,10,9,8,11,10,15,130,134,37,5,6,5,11,7,4,68,243,6,37,9,4,4,8,4,14,130,21,38,9,6,7,5,
    9,8,12,130,127,43,5,4,5,11,10,10,10,8,12,12,11,8,132,0,68,243,8,66,232,9,33,6,6,132,226,36,9,6,14,14,12,130,129,34,14,12,13,133,253,33,10,12,130,71,35,12,13,9,7,131,67,46,9,11,10,8,
    8,12,10,10,12,15,15,8,16,6,6,131,230,44,8,9,3,9,5,5,9,9,6,3,4,6,19,68,243,8,32,12,130,106,131,169,68,243,8,35,8,4,9,7,130,67,46,11,9,9,8,9,9,11,11,6,6,6,15,15,15,6,130,209,41,11,8,
    12,14,4,9,12,8,9,10,130,205,41,9,11,7,8,14,15,10,10,11,14,130,225,130,25,47,12,9,11,12,11,9,10,14,8,16,14,18,17,9,11,16,130,61,45,11,10,9,13,9,12,9,9,10,9,14,13,8,8,130,41,49,9,11,
    13,10,9,9,15,13,17,12,14,10,9,16,10,11,11,14,68,243,6,66,171,5,130,137,136,2,36,14,15,14,15,12,130,3,32,14,68,243,5,32,7,130,0,34,8,4,3,130,5,35,16,13,15,16,136,37,135,8,32,14,130,
    41,132,44,34,7,3,6,131,19,67,19,9,130,9,130,189,133,2,36,7,13,13,15,15,68,187,25,32,10,130,0,153,29,153,25,34,5,5,8,136,111,38,15,15,15,13,15,13,13,130,167,139,18,133,21,132,26,34,
    7,7,13,142,73,32,8,84,183,5,44,10,10,5,6,5,7,5,5,5,17,17,13,13,130,195,130,51,32,15,67,111,5,32,17,134,0,32,7,142,56,69,180,8,35,5,7,7,6,133,110,66,35,5,37,11,11,5,7,13,11,130,0,131,
    20,33,15,15,68,177,6,136,12,36,13,13,17,12,12,132,99,130,102,34,12,6,10,134,59,34,13,10,8,130,0,131,18,130,111,130,35,34,17,9,13,131,78,131,54,32,7,133,93,69,155,6,35,9,6,9,7,130,0,
    85,55,11,65,135,5,131,37,131,89,131,91,137,11,131,9,133,13,135,117,34,6,15,15,130,156,32,11,139,5,134,163,69,0,5,133,49,65,3,6,35,15,12,12,14,130,0,65,209,12,32,13,133,17,131,5,33,
    15,15,137,25,33,6,6,130,214,130,14,69,76,7,132,222,34,14,14,12,130,0,135,18,131,119,45,11,6,13,17,21,21,18,18,21,21,10,11,11,10,130,3,32,7,131,125,131,103,35,13,8,10,8,130,63,39,8,
    8,15,15,17,8,8,9,66,53,5,132,5,130,55,68,243,6,130,246,52,5,3,15,9,9,13,15,13,13,13,10,8,11,9,12,16,11,11,12,12,16,83,231,5,35,9,10,9,11,130,57,8,32,9,7,10,11,4,7,9,7,8,8,6,7,10,8,
    9,6,6,10,12,8,7,9,11,7,10,13,9,7,9,9,9,8,9,130,26,51,12,7,14,11,16,13,7,9,16,7,8,5,11,8,7,11,7,8,6,6,130,6,36,9,8,6,7,11,130,51,50,10,10,8,8,12,11,13,9,12,8,7,14,10,10,10,9,10,10,11,
    130,111,131,176,37,10,10,13,13,12,12,130,19,130,215,130,5,42,11,11,8,12,12,9,10,8,8,8,7,130,87,130,54,32,9,130,10,33,8,8,130,50,131,8,131,109,130,12,38,9,7,10,10,8,8,0,130,0,40,12,
    8,10,8,4,4,12,9,12,130,29,36,11,9,4,0,8,132,0,130,223,130,2,35,10,10,12,12,68,243,8,79,10,5,130,49,45,0,17,22,11,0,2,5,5,7,10,9,14,14,4,130,7,35,12,4,7,4,66,11,7,130,95,130,77,38,12,
    12,8,16,11,10,11,130,83,48,12,12,5,6,10,8,15,13,13,10,13,10,9,9,12,11,16,130,185,51,5,6,5,12,7,5,9,10,8,10,9,5,10,10,4,4,8,4,15,10,130,0,33,6,7,130,41,32,12,130,126,42,5,4,5,12,11,
    11,11,9,13,13,12,66,192,5,32,8,131,6,32,4,130,0,130,37,134,2,51,6,6,9,9,8,7,8,9,7,15,15,13,5,7,12,15,13,14,12,12,130,245,46,9,10,13,9,7,7,13,14,10,8,5,5,12,11,9,130,2,130,3,39,11,13,
    16,16,9,17,6,6,130,152,32,10,68,243,8,41,4,4,6,21,11,9,11,9,9,5,130,0,34,13,13,13,130,62,8,33,4,6,6,5,5,3,5,3,6,3,6,8,5,9,7,10,8,4,12,10,9,8,10,10,12,12,6,6,6,16,15,16,6,9,130,10,39,
    9,13,15,5,9,13,9,10,130,57,42,12,10,12,8,8,15,16,10,10,12,15,130,225,50,12,9,13,13,9,12,12,11,10,11,15,9,17,15,19,18,9,12,17,130,226,35,12,10,9,14,130,250,130,46,58,15,13,9,8,11,12,
    15,10,12,13,11,10,10,16,14,18,13,15,11,9,17,10,12,11,14,10,0,132,0,66,171,5,66,189,9,34,13,13,15,130,115,32,13,130,3,33,15,13,130,134,40,6,6,7,7,8,8,8,5,4,130,4,35,17,13,16,17,67,0,
    13,130,13,32,15,130,41,132,44,32,8,68,243,5,71,161,10,33,14,14,73,38,6,38,16,16,8,14,14,15,15,65,85,5,130,5,77,43,9,132,9,33,5,5,131,6,153,29,153,25,33,5,5,72,17,9,130,107,35,14,16,
    14,14,130,167,136,130,133,126,130,21,33,16,16,130,4,33,8,8,74,105,15,68,243,11,39,8,5,5,5,18,18,14,14,130,210,131,54,67,116,5,32,18,134,0,68,242,8,32,8,74,162,15,34,8,7,7,72,142,8,
    130,27,35,12,12,6,8,73,67,6,131,253,32,8,68,145,5,131,33,33,16,16,130,12,36,14,14,18,12,12,132,99,130,102,34,13,7,11,134,59,33,14,10,68,226,7,130,111,130,35,34,18,9,14,131,16,32,14,
    130,0,32,8,133,93,32,8,65,3,6,68,243,5,131,12,135,136,65,144,5,68,137,9,32,13,68,149,10,133,105,139,5,32,7,68,237,7,131,152,33,15,15,131,5,32,7,133,65,133,24,133,109,65,3,6,35,16,13,
    13,15,130,0,133,81,65,215,12,132,12,33,16,16,137,117,33,6,6,133,37,133,82,133,5,34,6,15,15,69,236,5,69,101,5,131,107,130,240,38,18,22,22,19,19,22,22,68,243,6,33,8,8,130,34,131,103,
    35,14,9,11,9,130,78,39,8,8,16,16,18,8,8,10,66,56,8,33,16,16,130,55,48,8,8,5,5,5,8,5,8,9,9,5,4,16,10,10,14,16,130,41,48,11,9,11,9,13,17,11,11,13,13,17,5,6,7,4,2,10,130,15,33,12,9,130,
    0,45,7,10,12,5,7,10,7,8,8,6,7,11,8,10,130,4,41,12,8,8,9,12,8,11,14,10,8,74,91,5,8,34,8,9,13,7,15,11,17,13,8,10,17,7,9,5,11,9,8,12,8,9,6,6,8,7,12,10,8,7,8,12,11,9,9,11,11,130,150,47,
    12,14,10,12,9,8,15,11,11,11,9,11,11,12,14,14,75,158,6,37,14,14,13,13,10,10,130,117,130,5,34,10,12,12,130,190,32,10,130,45,77,150,5,32,10,65,164,6,35,11,10,10,10,130,115,132,5,35,8,
    11,9,7,131,79,32,0,130,0,34,12,9,11,130,192,68,146,6,37,10,4,0,8,9,8,130,0,130,223,130,2,131,200,41,6,5,5,7,8,6,7,7,3,6,82,119,6,38,0,0,19,25,12,0,2,130,22,42,11,10,16,15,4,6,6,8,13,
    4,8,81,206,10,8,40,10,4,4,13,13,13,9,18,12,11,12,13,10,9,13,13,5,7,11,9,17,14,14,11,14,11,10,10,13,12,18,11,11,11,6,7,6,13,8,5,10,130,195,40,10,6,11,11,5,5,9,5,16,130,21,32,11,130,
    102,47,11,9,14,9,9,9,6,5,6,13,12,12,12,10,14,14,131,207,69,224,5,34,10,10,5,130,0,131,38,133,3,49,7,7,10,10,9,8,9,10,8,17,17,15,5,8,13,16,14,16,130,116,41,10,11,10,12,14,10,7,8,14,
    16,130,211,34,5,13,12,130,111,43,10,10,14,12,12,14,18,18,10,19,7,7,130,152,130,115,37,3,10,6,6,11,11,130,12,36,7,23,12,10,12,133,87,71,68,5,46,5,7,6,6,6,4,5,4,6,4,7,9,5,10,8,130,67,
    33,13,11,130,48,42,11,13,13,7,7,7,18,17,18,7,11,130,10,39,10,14,17,5,10,14,10,11,130,57,8,42,14,11,13,9,9,16,18,11,11,14,17,9,17,15,13,10,15,14,10,13,14,13,11,12,16,10,19,17,21,20,
    11,13,19,10,11,9,13,11,10,15,11,14,11,130,46,54,16,15,10,9,12,13,16,11,14,15,12,11,11,17,15,21,14,16,12,10,19,12,13,130,17,32,0,132,0,32,13,71,178,5,32,15,72,220,13,33,15,17,130,0,
    44,15,11,10,9,6,6,8,8,9,9,9,5,4,130,4,35,19,15,18,19,148,45,132,44,34,9,3,7,138,28,66,180,5,73,38,6,38,17,17,9,16,16,17,17,82,44,13,65,235,5,131,5,33,6,6,82,54,5,134,31,74,74,14,33,
    6,6,153,55,130,27,132,184,131,4,130,107,35,15,17,15,15,130,167,139,18,133,21,33,17,17,130,26,33,9,9,69,117,15,35,9,11,9,11,132,0,32,6,85,209,5,35,20,20,15,15,130,195,32,18,130,0,133,
    185,32,21,134,0,66,204,6,69,174,17,35,6,9,8,7,133,110,132,71,36,11,13,13,6,9,67,233,6,33,15,15,130,246,130,77,130,2,133,158,37,9,14,14,15,15,20,131,4,133,21,35,14,8,12,7,65,38,5,33,
    16,11,131,226,131,21,40,20,20,20,16,16,20,20,11,16,68,39,5,130,206,133,93,32,9,65,3,6,34,7,11,8,130,0,32,10,130,0,133,111,33,11,11,131,31,131,83,32,15,130,185,143,11,133,59,132,211,
    130,122,131,13,35,7,17,17,13,130,0,139,5,32,8,133,61,34,17,17,12,130,0,133,49,130,137,132,255,33,14,14,131,6,33,15,15,68,129,9,139,11,33,17,17,131,23,133,49,33,7,7,133,96,135,88,130,
    65,32,14,130,107,131,6,139,18,130,240,42,20,25,25,21,21,25,25,11,13,13,11,130,3,33,9,9,130,63,39,17,15,15,17,15,10,12,10,130,10,39,9,9,18,18,21,9,9,11,66,56,8,132,61,8,37,9,9,6,6,6,
    9,6,9,10,10,6,4,17,11,11,16,17,16,15,15,12,10,13,11,15,19,13,13,15,14,19,6,7,7,4,2,12,11,130,17,32,10,130,0,37,8,11,13,5,8,11,73,207,5,46,9,11,7,7,12,14,9,9,10,13,9,12,15,11,9,74,91,
    5,8,34,9,10,14,8,17,13,19,15,8,11,19,8,10,6,13,10,9,13,9,10,7,7,9,8,13,11,9,7,9,13,12,10,10,12,12,130,150,45,13,16,11,14,10,9,17,12,12,12,11,12,12,13,130,111,33,13,13,130,11,40,12,
    15,15,14,14,12,12,12,14,132,5,37,13,13,9,15,15,11,130,45,35,9,8,9,10,130,38,34,10,11,10,131,0,33,12,11,78,255,10,39,12,10,8,12,12,9,9,0,130,0,37,14,10,12,10,5,5,68,146,6,36,11,5,0,
    9,10,70,234,7,131,91,45,14,14,7,6,6,8,9,6,8,7,4,7,8,0,134,0,36,21,28,14,0,2,130,22,42,12,11,17,17,5,6,6,9,14,5,8,130,220,68,143,8,130,77,36,14,14,9,20,14,130,160,50,11,10,14,15,6,8,
    12,10,19,16,16,12,16,13,11,11,14,13,20,130,167,50,6,8,6,14,9,6,11,12,10,12,11,7,12,12,5,5,10,5,18,130,21,52,12,7,9,7,12,10,15,10,10,10,6,5,6,14,14,14,13,11,16,16,14,133,84,32,10,131,
    6,32,5,130,0,131,38,133,3,33,8,8,133,206,35,8,19,19,16,130,129,34,18,16,17,130,50,46,11,12,11,13,16,11,8,9,16,17,12,9,6,6,14,130,212,130,64,41,15,14,14,16,20,19,11,21,8,8,130,152,130,
    115,37,4,11,7,7,12,12,130,12,33,8,25,130,32,34,11,11,6,130,0,32,16,130,106,32,14,130,198,49,7,7,6,4,6,4,7,4,8,10,6,11,9,12,10,5,15,12,130,48,40,12,14,14,7,8,8,20,19,20,130,54,41,15,
    14,11,16,19,6,11,16,11,12,131,89,32,12,130,173,37,18,20,13,13,15,19,130,225,32,15,130,175,47,12,14,15,14,12,13,18,11,21,19,23,22,12,14,21,11,130,207,8,34,13,11,17,12,16,12,12,13,11,
    18,16,11,10,13,15,18,12,15,16,14,12,12,19,17,23,16,18,14,11,21,13,15,14,18,12,65,86,5,73,139,9,130,141,132,2,32,19,130,0,32,16,132,4,37,12,11,9,7,7,9,130,0,34,10,6,4,130,5,35,21,17,
    19,21,66,254,9,134,9,131,40,132,44,34,9,3,8,67,219,5,32,17,131,0,73,32,5,135,28,37,19,9,17,17,19,19,65,170,8,72,51,14,33,7,7,88,53,9,130,35,144,29,153,55,33,7,7,67,29,9,38,19,19,19,
    17,19,17,17,130,167,67,48,8,133,126,130,21,132,26,33,9,9,69,117,9,131,112,34,13,13,10,130,1,132,9,86,255,6,35,22,22,17,17,130,214,32,20,130,0,133,191,32,23,134,0,68,114,7,33,10,10,
    74,231,14,35,7,9,9,8,133,110,66,31,5,36,15,15,7,9,17,67,233,5,36,17,17,19,19,9,68,17,5,131,33,132,12,36,17,17,23,15,15,132,99,130,102,34,16,9,13,134,59,33,17,13,83,203,6,41,16,22,22,
    22,18,18,22,22,12,18,131,78,131,54,32,9,133,93,32,9,133,94,34,12,8,12,130,223,32,9,85,55,10,32,12,65,134,5,131,37,33,16,16,143,5,133,59,133,111,133,5,34,8,19,19,131,157,33,19,19,131,
    152,133,5,32,9,133,65,133,24,133,49,130,17,32,19,131,0,33,15,15,131,5,133,19,33,17,17,70,9,5,130,5,134,2,33,19,19,70,28,9,33,8,8,132,124,32,17,139,88,34,8,19,19,130,72,32,16,139,113,
    45,14,8,17,22,28,28,23,23,28,28,13,14,14,13,130,3,32,9,131,125,130,38,36,19,17,11,14,11,130,104,39,10,10,20,20,23,10,10,12,132,126,133,4,130,169,8,73,9,9,7,7,7,9,7,10,11,11,6,4,19,
    12,12,18,19,18,17,17,14,11,14,12,16,21,14,14,16,16,21,6,7,8,5,3,13,12,14,12,14,11,11,12,11,9,13,14,6,9,12,9,10,10,8,9,14,10,12,8,8,13,15,10,10,11,15,10,13,17,12,9,12,12,130,32,8,49,
    12,10,11,16,9,18,14,21,17,9,12,20,9,11,7,14,11,10,15,10,11,7,8,10,9,15,12,10,8,10,14,13,11,11,13,13,10,10,16,14,17,12,15,11,10,18,13,13,13,12,130,167,130,111,32,15,130,176,37,13,14,
    17,17,16,16,130,19,130,216,130,5,37,15,14,10,16,16,12,130,45,41,10,9,10,11,12,13,13,11,12,11,131,0,38,14,12,12,12,10,10,11,132,5,40,10,14,11,9,14,14,10,10,0,130,0,42,15,11,14,11,6,
    5,16,12,16,12,11,130,188,33,5,0,69,34,5,130,223,130,2,37,13,13,16,16,8,7,130,134,35,7,9,8,4,79,10,5,130,49,53,0,24,32,16,0,2,7,7,9,14,13,20,19,6,7,7,10,16,5,10,5,9,66,131,9,33,5,5,
    130,136,51,11,23,15,14,15,17,12,12,16,17,6,9,14,11,22,18,18,13,18,14,130,78,56,15,22,14,13,14,7,9,7,16,10,6,12,14,11,14,13,8,14,14,6,6,12,6,21,14,130,0,37,8,10,8,14,12,17,130,174,35,
    7,6,7,16,130,125,35,12,18,18,16,66,97,5,32,11,131,92,32,6,130,0,67,133,9,33,9,9,130,217,38,10,11,13,10,21,21,19,130,129,51,21,18,20,16,16,16,13,14,13,15,18,13,9,10,18,20,14,11,7,7,
    130,15,8,32,16,15,12,12,18,15,15,18,22,22,12,24,9,9,6,6,16,14,12,13,4,13,8,8,13,13,9,5,6,9,29,15,12,130,31,130,86,33,6,18,130,106,35,16,16,6,9,130,236,8,57,5,7,5,8,4,9,11,7,13,10,14,
    11,6,17,13,13,12,13,14,16,16,8,9,9,22,22,23,8,13,14,17,16,13,18,22,6,13,18,13,14,15,13,12,17,14,17,11,11,21,22,14,14,17,22,12,22,19,17,130,175,47,13,16,17,16,14,15,21,13,24,21,26,25,
    13,16,24,13,130,207,37,14,13,20,13,18,14,130,46,58,21,19,12,11,15,17,21,13,17,19,16,14,13,22,20,26,18,21,16,13,24,15,17,16,20,14,0,132,0,32,16,66,196,9,134,9,36,21,22,21,22,18,130,
    3,45,21,18,14,12,11,8,8,10,10,11,11,11,7,5,130,4,35,24,19,22,24,148,45,132,44,34,11,4,9,67,219,5,32,19,131,0,32,20,132,0,32,22,135,0,36,11,20,20,22,22,65,171,8,65,230,5,136,5,33,8,
    8,131,10,153,29,153,25,34,8,8,13,67,29,8,38,22,22,22,19,22,19,19,130,167,139,18,133,21,132,26,34,11,11,19,65,215,5,130,5,65,209,5,34,12,14,12,133,8,34,8,9,8,130,233,36,8,26,26,19,19,
    130,224,32,23,130,0,73,224,5,32,26,134,0,32,11,142,56,66,16,8,35,8,11,11,9,133,110,133,18,35,17,17,8,11,67,233,6,35,19,19,22,22,69,145,6,131,33,132,12,36,19,19,26,17,17,132,99,130,
    102,36,18,10,15,9,20,133,0,32,14,131,226,131,18,40,25,25,25,20,20,25,25,13,20,131,16,32,19,130,0,32,11,133,93,32,11,65,3,6,33,9,13,88,35,5,33,13,13,133,111,33,14,14,65,144,5,131,37,
    33,18,18,143,5,133,59,133,111,133,11,35,9,22,22,16,130,0,34,22,22,17,130,0,133,5,32,10,133,55,33,22,22,69,6,5,133,67,33,18,22,131,0,34,18,18,21,130,0,33,19,19,133,164,131,5,139,11,
    33,22,22,133,17,131,49,33,9,9,132,124,34,19,21,21,130,87,34,17,21,21,131,149,33,9,21,132,6,33,21,21,130,112,32,16,133,24,45,16,9,20,25,32,32,27,27,32,32,14,16,16,14,130,3,33,11,11,
    130,41,32,22,130,103,35,19,13,15,13,130,72,39,12,12,23,23,26,12,12,14,66,56,8,132,163,8,35,11,11,8,8,8,11,8,11,12,12,7,5,22,14,14,20,22,20,19,19,15,13,16,13,18,25,16,16,19,18,24,7,
    8,9,5,3,131,15,8,94,16,13,12,13,13,10,14,16,6,10,14,10,12,11,9,10,16,11,14,9,9,15,17,12,11,13,17,11,15,20,14,11,14,14,13,13,14,14,11,12,18,10,21,16,24,19,11,14,23,10,12,8,16,12,11,
    17,11,12,8,9,12,10,17,14,11,9,11,16,15,12,13,15,15,12,11,19,16,20,14,17,13,11,21,15,15,15,13,15,16,16,20,20,16,17,16,65,32,5,33,18,18,130,7,130,181,130,5,45,17,16,12,19,19,14,15,12,
    12,12,10,12,13,14,130,41,74,11,5,33,16,14,130,100,32,11,74,11,6,39,15,13,10,16,16,11,11,0,130,0,48,17,13,15,12,6,6,18,14,18,14,12,13,16,14,6,0,12,132,0,70,82,5,39,15,15,18,18,9,8,8,
    10,130,229,35,9,5,9,10,130,45,132,2,52,27,35,17,0,3,7,8,11,16,15,22,22,6,8,8,11,18,6,11,6,11,67,56,8,32,15,130,77,52,18,18,12,26,17,15,17,19,14,13,19,19,7,10,16,13,24,20,20,15,20,130,
    129,8,41,19,17,25,16,15,15,8,10,8,18,11,7,14,16,12,16,14,8,16,15,7,7,13,7,23,15,16,16,16,9,11,9,15,13,20,12,13,12,8,6,8,18,130,128,35,14,20,20,19,66,80,5,33,12,14,130,0,32,7,130,0,
    75,220,5,131,106,8,51,10,10,15,15,12,11,12,15,11,24,24,21,8,11,19,23,20,22,18,19,19,15,16,14,17,20,15,11,12,20,22,16,12,7,8,18,18,15,19,17,14,14,20,17,17,20,25,25,14,27,10,10,130,152,
    40,16,13,15,5,15,9,9,15,15,130,12,35,10,33,17,14,130,31,130,86,33,7,20,130,106,131,169,130,236,8,39,5,8,6,9,5,10,13,7,14,11,15,12,6,19,15,15,13,15,16,18,18,9,10,10,25,24,26,9,15,16,
    19,19,14,20,24,7,14,20,14,15,130,57,8,42,19,16,19,12,13,23,25,16,16,19,24,13,24,21,19,14,21,20,15,19,20,18,15,17,23,14,27,24,30,28,15,19,27,15,16,13,19,16,14,22,15,20,15,130,46,58,
    23,21,14,13,17,19,23,15,19,21,18,16,15,25,22,29,20,23,18,15,27,17,19,18,23,15,0,132,0,32,18,130,132,32,20,67,225,5,32,21,131,10,38,20,20,24,25,24,25,21,130,3,52,24,21,16,14,12,9,9,
    11,11,12,12,13,8,6,12,12,12,27,21,25,27,148,45,132,44,34,12,4,10,67,219,5,32,22,131,0,73,32,5,32,25,135,0,36,12,22,22,25,25,65,85,5,77,56,7,65,235,5,131,5,33,8,8,131,5,153,29,153,25,
    33,8,8,67,29,9,130,106,35,22,25,22,22,130,167,67,48,8,133,125,130,21,33,25,25,130,4,33,12,12,69,117,9,32,16,132,0,34,13,16,13,132,7,42,16,8,10,8,12,8,8,8,29,29,21,131,0,32,26,130,0,
    32,24,132,0,32,29,134,0,32,12,69,118,8,133,47,136,5,35,8,12,12,10,133,110,133,18,37,19,19,9,12,22,18,130,0,131,20,34,25,25,12,68,105,5,136,12,36,22,22,29,19,19,68,43,5,36,20,20,20,
    11,17,134,59,33,22,16,131,226,130,17,33,20,28,73,5,5,34,15,23,15,130,0,131,54,32,12,133,93,32,12,130,14,131,2,34,10,15,11,75,9,6,133,111,33,16,16,131,176,33,20,20,149,5,33,22,22,131,
    23,133,111,133,11,34,10,24,24,130,156,34,18,24,24,131,152,133,5,32,11,133,61,34,24,24,17,130,0,33,21,21,131,111,130,137,32,25,131,0,34,20,20,24,130,0,133,19,36,22,22,23,23,23,130,79,
    65,216,5,131,5,33,25,25,131,5,133,29,33,10,10,133,7,133,82,33,24,24,130,65,33,20,10,133,6,139,113,45,18,10,22,28,35,35,30,30,35,35,16,18,18,16,130,3,33,12,12,130,161,39,25,21,21,24,
    22,14,17,14,130,122,39,13,13,26,26,29,13,13,16,66,56,8,33,25,25,130,169,8,58,12,12,8,8,8,12,8,13,14,14,8,6,25,16,16,23,25,23,22,22,17,14,18,15,21,28,18,18,21,20,27,8,9,11,6,3,16,15,
    17,15,18,14,14,15,14,11,16,19,7,11,15,12,13,13,10,11,17,13,16,130,4,8,73,20,13,12,14,19,12,17,22,16,12,16,16,14,14,15,15,12,14,20,11,24,18,27,21,12,15,26,11,14,9,18,14,12,19,13,14,
    9,10,13,11,19,16,13,10,12,18,17,14,14,17,17,13,13,21,18,22,16,19,14,13,24,17,17,17,15,17,18,18,22,22,18,19,19,17,130,0,35,21,22,20,20,130,6,32,20,132,5,37,19,19,13,21,21,16,130,45,
    38,13,11,13,15,15,17,17,130,125,32,14,130,0,38,17,16,16,16,13,13,14,132,5,40,13,17,14,11,18,18,13,13,0,130,0,49,19,14,17,14,7,7,20,16,20,16,14,14,19,15,7,0,13,14,70,217,7,131,91,44,
    21,21,10,8,8,11,12,9,12,10,5,10,11,130,45,132,2,36,29,38,19,0,3,130,22,44,17,16,24,23,7,9,9,12,20,6,12,6,11,66,125,8,34,16,6,6,130,136,8,47,13,28,19,17,18,20,15,14,20,21,8,10,17,14,
    26,22,22,16,22,17,15,15,20,18,27,17,16,17,9,11,9,20,12,8,15,17,13,17,15,9,17,16,7,7,14,7,25,16,130,190,37,10,12,10,16,14,21,130,127,42,9,7,9,20,19,19,18,15,22,22,20,66,97,5,33,13,15,
    130,0,32,7,130,0,131,38,32,17,131,201,33,16,11,130,112,45,13,12,13,16,12,26,26,22,8,12,20,25,22,24,130,116,62,16,17,16,18,22,16,11,12,22,24,17,13,8,8,20,19,16,20,19,15,15,21,19,19,
    22,27,27,15,29,11,11,130,230,47,17,14,16,5,16,9,9,16,16,11,6,7,11,35,19,15,130,31,32,8,130,0,32,22,130,106,8,34,20,20,7,11,10,9,9,6,8,6,9,5,11,14,8,15,12,17,13,7,20,16,16,14,16,17,
    20,20,10,11,11,27,26,28,10,131,10,53,15,22,26,8,15,22,15,17,19,16,15,21,17,20,13,14,25,27,17,17,21,26,130,225,32,21,130,175,53,16,20,21,19,17,18,25,15,29,26,32,30,16,20,29,16,17,14,
    20,17,16,24,130,250,61,17,18,15,25,23,15,14,18,20,25,16,21,23,19,17,16,27,24,31,22,25,19,16,29,18,20,19,25,17,0,132,0,32,20,67,22,5,130,138,67,21,5,34,21,21,26,130,0,32,22,132,4,50,
    17,15,13,10,10,12,12,13,13,14,8,6,13,13,13,29,23,27,29,148,45,132,44,34,13,5,11,131,28,67,19,6,32,24,132,0,73,38,6,38,26,26,13,24,24,26,26,65,85,5,130,5,68,9,5,136,5,33,9,9,131,10,
    153,29,153,25,34,9,9,15,67,131,8,38,26,26,26,23,26,23,23,130,167,139,18,133,21,132,26,34,13,13,23,69,61,14,35,14,17,14,17,132,0,46,9,11,9,13,9,9,9,31,31,23,23,22,22,22,28,130,0,67,
    116,5,32,31,134,0,73,85,8,134,47,69,175,8,35,9,13,13,11,133,110,133,18,36,20,20,10,13,23,131,205,131,20,130,246,32,21,132,0,136,12,36,23,23,31,21,21,132,99,130,102,36,22,12,18,11,24,
    133,0,32,17,68,226,7,41,30,30,30,24,24,30,30,16,24,16,130,0,32,23,130,0,32,13,130,3,130,2,69,155,6,35,16,11,16,12,130,0,32,15,130,0,133,111,33,17,17,131,72,131,83,33,23,23,145,5,133,
    59,133,111,133,11,35,11,26,26,20,130,0,139,5,34,12,22,22,133,159,135,182,132,47,32,27,131,0,66,11,5,33,23,23,133,164,131,5,34,23,23,25,130,0,33,24,24,130,4,34,25,26,26,133,17,131,61,
    33,11,11,132,68,32,23,135,88,32,21,130,0,130,107,70,65,5,133,113,131,11,45,19,11,24,30,38,38,32,32,38,38,17,19,19,17,130,3,33,13,13,130,191,39,26,22,22,26,23,15,19,15,130,72,39,14,
    14,28,28,31,14,14,17,66,56,8,132,80,8,134,13,13,9,9,9,13,9,14,15,15,9,6,26,17,17,24,26,24,23,23,19,15,19,16,22,30,20,20,22,22,29,9,10,11,6,4,18,16,19,16,20,15,15,16,15,12,17,20,8,12,
    17,13,14,14,11,12,19,14,17,10,11,19,21,14,13,15,20,13,18,24,17,13,17,17,15,15,16,16,13,15,22,12,25,19,28,23,13,16,28,12,15,9,19,15,13,20,13,15,10,11,14,12,20,17,14,11,13,20,19,15,16,
    18,18,14,14,23,20,24,17,21,15,14,25,18,18,18,16,18,19,20,24,24,19,20,20,130,11,40,19,23,24,22,22,18,18,19,22,131,5,38,18,21,20,14,23,23,17,130,45,41,14,12,14,16,16,18,18,15,16,15,131,
    0,130,217,37,17,14,14,15,17,17,130,5,40,14,19,15,12,19,19,14,14,0,130,0,47,21,15,19,15,8,7,22,17,22,17,15,15,20,16,7,0,69,34,5,32,20,132,0,45,18,18,22,22,11,9,9,12,13,10,13,11,6,10,
    82,119,6,38,0,0,32,42,21,0,3,130,251,44,19,17,26,26,7,10,10,13,22,7,13,7,12,66,125,8,34,17,7,7,130,136,8,69,14,31,21,18,20,22,16,16,22,23,9,11,19,15,29,24,24,18,24,19,17,17,22,20,30,
    19,18,18,10,12,10,22,13,9,16,19,15,19,17,10,19,18,8,8,16,8,28,18,19,19,19,11,14,11,18,15,23,15,15,14,10,8,10,22,21,21,20,16,24,24,130,64,32,16,130,0,131,181,33,17,8,130,0,75,183,5,
    32,18,130,0,32,12,130,112,45,14,13,15,17,13,28,28,25,9,13,22,28,24,26,130,116,62,17,18,17,20,24,17,13,14,24,27,19,14,9,9,22,21,17,22,21,16,16,23,21,21,24,30,30,16,32,12,12,130,152,
    40,19,15,18,6,17,10,10,18,18,130,12,35,12,39,21,16,130,31,32,9,130,0,32,24,130,106,8,34,22,22,8,12,11,10,10,6,9,7,10,6,12,15,9,17,14,18,14,8,22,18,18,15,18,19,22,22,11,12,12,30,29,
    30,11,131,10,8,83,17,24,29,9,17,24,17,18,21,17,16,23,19,23,15,15,27,30,19,19,23,29,15,29,25,23,16,25,24,18,22,23,21,18,20,28,17,32,29,35,33,18,22,32,17,18,16,22,19,17,26,18,24,18,18,
    20,17,28,25,16,15,20,22,27,18,23,25,21,19,18,29,26,35,24,27,21,17,32,20,22,21,27,18,0,132,0,32,22,130,132,32,24,67,225,5,32,25,66,219,5,36,28,29,28,29,25,130,3,52,28,25,19,16,14,11,
    11,13,13,14,14,15,9,7,14,14,14,32,25,30,32,148,45,132,44,34,14,5,12,67,59,5,32,26,131,0,32,27,132,0,73,38,6,38,29,29,14,27,27,29,29,65,171,8,32,21,141,0,34,10,10,20,130,0,153,29,153,
    25,33,10,10,67,29,9,130,107,35,26,29,26,26,130,167,67,48,8,133,126,130,21,33,29,29,130,4,34,14,14,26,71,33,8,69,111,5,34,16,19,16,133,8,42,10,12,10,14,10,10,10,34,34,25,25,130,224,
    32,31,130,0,32,28,132,0,32,35,134,0,84,244,9,69,174,14,35,10,14,14,12,133,110,133,24,36,22,22,11,14,26,67,233,5,33,26,26,130,246,68,105,5,131,33,132,12,36,26,26,34,23,23,68,43,5,130,
    103,33,13,20,134,59,34,26,19,17,130,0,130,17,42,24,33,33,33,27,27,33,33,18,27,18,130,0,131,54,32,14,133,93,78,194,6,34,18,12,18,88,130,5,33,17,17,133,111,33,19,19,68,131,9,68,137,5,
    139,5,33,26,26,68,155,9,133,11,35,12,29,29,22,130,0,139,5,32,13,133,49,74,227,5,33,25,25,74,228,6,32,29,131,0,33,24,24,131,5,133,19,36,26,26,27,27,27,130,79,130,5,134,2,33,29,29,131,
    8,133,29,34,12,12,24,132,57,33,28,28,130,93,35,22,28,28,23,130,0,34,12,28,28,131,210,139,18,45,21,12,26,33,42,42,36,36,42,42,19,21,21,19,130,3,33,14,14,130,161,39,29,25,25,28,26,17,
    21,17,130,122,39,16,16,31,31,35,16,16,19,66,56,8,132,169,8,73,14,14,10,10,10,14,10,15,16,16,10,7,29,19,19,27,29,27,26,26,21,17,21,18,25,33,22,22,25,24,32,10,11,12,7,4,19,18,21,18,22,
    17,17,18,17,13,19,22,9,14,18,14,15,15,12,13,21,15,19,12,13,20,23,15,15,17,22,15,20,26,18,14,19,19,130,32,8,52,18,14,17,24,13,28,21,31,25,14,18,31,13,16,10,21,16,15,23,15,16,11,12,15,
    14,22,19,15,12,15,22,20,16,17,20,20,16,15,25,22,26,19,23,17,15,28,20,20,20,18,20,21,22,130,111,41,22,22,20,20,20,21,25,26,24,24,130,7,32,24,132,5,47,23,22,16,25,25,19,20,15,15,15,13,
    16,17,18,20,20,130,125,35,17,17,17,16,130,217,49,19,15,15,16,19,18,18,16,16,16,21,17,14,21,21,15,15,0,130,0,48,23,17,21,16,9,8,24,19,24,19,16,17,22,18,8,0,16,132,0,32,22,132,0,46,20,
    20,24,24,12,10,10,13,15,11,14,12,6,11,13,130,45,132,2,33,33,43,68,243,5,45,20,18,27,26,8,10,10,14,23,7,13,7,13,18,136,0,8,39,7,7,23,23,23,15,32,21,19,20,23,17,16,23,23,9,12,19,16,30,
    25,25,18,25,20,18,17,23,21,31,19,18,19,10,13,10,23,14,9,17,68,243,5,38,19,8,8,16,8,28,19,68,243,5,48,19,16,24,15,16,15,10,8,10,23,21,21,20,17,25,25,23,67,101,5,68,243,8,66,95,7,51,
    19,19,12,12,18,18,15,13,15,18,13,29,29,26,9,14,23,28,25,27,130,116,8,38,18,19,18,20,25,18,13,14,25,27,19,15,9,9,23,21,18,23,21,17,17,24,21,21,25,31,31,17,33,12,12,8,8,23,19,16,18,6,
    18,68,243,5,36,8,12,40,21,17,130,31,32,9,130,0,32,25,130,106,8,63,23,23,8,12,11,10,10,7,9,7,11,6,12,16,9,18,14,19,15,8,23,18,18,16,18,19,23,23,12,12,12,31,30,31,12,18,20,23,23,17,25,
    30,9,18,25,17,19,21,18,17,24,19,23,15,16,28,31,20,20,24,30,16,30,26,130,25,57,25,18,23,24,22,19,21,28,17,34,29,36,34,18,23,33,18,19,16,23,20,18,27,19,25,19,130,46,58,28,26,17,16,20,
    23,28,18,24,26,22,19,19,30,27,36,25,28,22,18,34,20,23,22,28,19,0,132,0,32,23,68,197,16,36,29,30,29,30,25,130,3,45,29,25,19,17,15,11,11,13,13,15,15,15,9,7,130,4,35,33,26,31,33,148,45,
    132,44,34,15,5,12,131,28,67,163,5,33,26,28,132,0,32,30,135,0,36,15,27,27,30,30,68,187,25,32,21,130,0,153,29,153,25,68,243,11,130,106,35,26,30,26,26,130,167,68,243,8,133,125,130,21,
    33,30,30,130,4,33,15,15,68,243,9,32,20,132,0,34,16,20,16,132,7,47,20,10,12,10,15,10,10,10,35,35,26,26,25,25,25,32,130,0,67,116,5,32,36,134,0,80,220,5,32,16,130,0,82,218,9,132,9,35,
    10,15,14,13,68,243,11,37,23,23,11,15,26,22,130,0,131,20,34,30,30,15,70,69,5,136,12,38,26,26,35,24,24,26,26,133,21,36,24,13,21,13,27,133,0,32,20,68,243,7,40,34,34,34,28,28,34,34,18,
    28,68,243,7,32,15,133,93,32,15,66,230,6,33,12,18,89,32,9,66,149,5,130,175,32,25,131,83,131,85,68,175,5,137,5,32,27,131,10,35,26,27,27,25,130,0,32,27,132,12,34,13,30,30,130,156,35,22,
    30,30,23,130,0,133,5,32,14,68,181,5,33,30,30,69,170,7,33,26,26,130,46,32,30,131,0,68,243,5,32,26,130,65,131,166,34,28,28,28,130,67,130,5,134,2,34,30,30,28,130,0,132,98,35,26,13,13,
    25,132,8,33,29,29,130,87,34,23,29,29,131,225,32,12,133,6,32,29,69,101,6,131,18,45,22,13,27,34,43,43,37,37,43,43,20,22,22,20,130,3,33,15,15,131,107,34,25,25,29,68,243,8,37,32,32,36,
    16,16,19,132,126,133,4,130,80,8,36,15,15,10,10,10,15,10,15,17,17,10,7,30,19,19,28,30,28,26,26,21,17,22,18,25,34,22,22,26,25,33,10,11,13,7,4,20,68,243,6,8,81,18,14,20,23,9,14,19,14,
    16,15,12,14,21,15,20,12,13,21,24,16,15,18,23,15,21,27,19,15,19,19,18,17,19,19,15,17,24,14,29,22,32,26,15,19,32,14,17,11,22,17,15,23,15,17,11,12,16,14,23,19,16,13,15,22,21,17,18,21,
    21,16,16,26,22,27,19,24,17,15,29,21,21,21,130,15,38,22,27,27,22,23,23,21,130,0,39,26,27,25,25,20,20,21,25,131,5,47,20,23,23,16,26,26,19,20,16,16,16,14,16,18,19,20,68,243,6,38,17,21,
    20,19,19,16,16,130,105,130,5,40,16,21,18,14,22,22,16,16,0,130,0,49,24,17,21,17,9,8,25,19,25,19,17,17,23,19,8,0,16,17,75,205,7,131,91,33,25,25,68,243,6,34,13,7,12,68,243,8,52,37,49,
    24,0,4,10,11,15,22,20,30,30,9,11,11,15,25,8,15,8,14,66,131,9,33,8,8,130,136,8,70,17,35,24,21,23,26,19,18,25,26,10,13,21,17,33,28,28,21,28,22,20,19,25,23,35,22,20,21,11,14,11,25,15,
    10,19,22,17,22,19,12,22,21,9,9,18,9,32,21,22,22,22,13,16,13,21,18,27,17,18,17,11,9,11,25,24,24,23,19,28,28,25,66,78,5,33,17,19,130,0,32,9,130,0,131,38,77,249,5,32,14,130,112,45,17,
    15,17,20,15,33,33,29,10,15,25,32,28,30,130,116,44,20,21,20,23,28,20,15,16,28,31,22,17,10,130,67,46,20,25,24,19,19,27,24,24,28,34,34,19,37,14,14,130,152,40,22,18,20,6,20,12,12,21,21,
    130,12,35,14,45,24,19,130,31,32,10,130,0,32,28,130,106,8,64,25,25,9,14,12,11,11,7,10,8,12,7,14,17,10,20,16,21,17,9,26,21,20,18,21,22,25,25,13,14,14,34,34,35,13,21,22,26,25,19,28,33,
    10,20,28,19,21,24,20,19,26,22,26,17,17,32,35,22,22,26,33,18,33,29,26,130,175,34,20,25,27,130,246,8,48,32,19,38,33,41,38,21,25,36,20,21,18,26,22,20,30,21,28,21,21,23,20,32,29,19,17,
    23,26,32,21,26,29,24,21,21,34,30,40,28,32,24,20,38,23,26,24,31,21,0,132,0,33,25,27,130,0,66,197,6,67,128,5,36,33,34,33,34,28,130,3,45,33,28,22,19,17,12,12,15,15,17,17,17,10,8,130,4,
    35,37,29,34,37,148,45,132,44,34,17,6,14,131,28,66,168,5,33,30,31,132,0,32,34,135,0,36,17,31,31,34,34,65,171,8,32,24,141,0,33,12,12,131,15,153,29,153,25,34,12,12,20,67,29,8,130,106,
    35,30,34,30,30,130,167,139,18,133,21,33,34,34,130,26,34,17,17,30,68,57,6,34,18,18,22,132,0,33,18,22,134,8,34,12,14,12,130,233,40,12,39,39,29,29,28,28,28,36,130,0,32,32,132,0,32,40,
    134,0,32,17,142,56,132,61,131,4,35,12,17,16,14,133,110,133,18,37,26,26,12,17,30,25,68,51,6,34,34,34,17,65,81,5,131,33,132,12,38,30,30,40,27,27,30,30,133,21,34,27,15,23,134,59,34,30,
    22,20,130,0,131,21,41,38,38,38,31,31,38,38,21,31,21,130,0,131,54,32,17,133,93,32,17,65,3,6,34,13,21,15,130,0,131,12,130,31,32,21,73,83,5,36,28,28,27,27,29,130,0,145,5,33,30,30,130,
    22,32,29,68,155,5,133,11,34,14,34,34,130,156,35,25,34,34,26,130,0,133,5,34,15,28,28,131,37,34,33,33,24,130,0,32,29,130,45,132,47,32,34,131,0,34,27,27,33,130,0,34,29,29,31,136,0,139,
    11,33,34,34,131,22,133,49,34,14,14,28,132,57,33,33,33,130,87,34,26,33,33,131,203,32,14,133,6,33,33,33,131,113,133,24,45,24,14,30,38,49,49,41,41,49,49,22,25,25,22,130,3,44,17,17,28,
    28,28,34,28,28,33,30,20,24,20,130,234,39,18,18,36,36,40,18,18,22,132,126,133,4,130,80,8,36,17,17,12,12,12,17,12,17,19,19,11,8,34,22,22,31,34,31,30,30,24,20,25,21,28,38,25,25,29,28,
    37,11,13,14,8,5,23,130,64,8,89,25,20,19,20,20,15,22,25,10,16,21,16,18,17,14,15,24,17,22,13,15,24,27,18,17,20,26,17,23,30,21,17,22,22,20,20,21,21,17,19,27,15,33,25,36,29,17,21,36,16,
    19,12,25,19,17,26,17,19,13,14,18,16,26,22,18,14,17,25,24,19,20,23,24,18,17,29,25,31,22,27,20,17,33,23,23,23,21,23,24,25,130,111,33,26,25,130,11,40,24,29,30,28,28,23,23,24,27,131,5,
    51,23,26,25,18,29,29,22,23,18,18,18,15,18,20,21,23,23,19,21,19,131,0,41,24,22,22,22,18,18,19,22,21,21,130,25,39,24,20,16,24,24,17,17,0,130,0,49,27,19,24,19,10,9,28,22,28,22,19,19,25,
    21,9,0,18,19,70,217,7,48,26,26,23,23,28,28,14,12,12,15,17,12,16,14,7,13,15,130,45,132,2,52,42,55,27,0,4,12,12,16,25,23,34,34,10,13,13,18,29,9,17,9,16,76,60,5,131,5,8,74,9,9,29,29,29,
    19,40,27,24,26,29,21,21,29,30,11,15,24,20,38,31,32,24,32,25,22,22,29,26,39,25,23,24,13,16,13,29,17,11,21,25,19,25,22,13,25,24,10,10,21,10,36,24,25,25,25,15,18,14,24,20,30,19,20,19,
    13,10,13,29,27,27,26,21,31,32,130,64,32,21,130,0,32,19,130,187,33,22,10,130,0,131,38,34,25,25,24,130,0,32,16,130,112,45,19,17,19,23,17,37,37,32,12,17,29,36,32,34,130,116,8,49,23,24,
    22,26,32,23,16,18,32,35,25,19,12,12,29,27,23,29,27,21,21,31,27,27,32,39,39,21,42,16,16,10,10,29,25,20,23,7,23,13,13,23,23,16,9,10,16,51,27,21,130,31,32,11,130,0,72,79,5,8,33,10,16,
    14,13,13,8,12,9,14,8,16,20,12,22,18,24,19,10,29,23,23,20,24,25,29,29,15,15,15,39,38,40,15,23,130,10,44,22,31,38,11,22,31,22,24,27,23,21,30,25,130,173,8,67,36,39,25,25,30,38,20,38,32,
    30,21,32,32,23,29,31,28,24,26,36,22,43,37,46,44,23,29,41,23,24,21,29,25,23,34,24,31,24,24,26,22,36,33,21,20,26,29,36,24,30,33,28,24,24,38,34,45,32,36,28,23,43,26,29,28,36,24,0,132,
    0,33,29,31,130,0,130,136,131,2,66,218,5,36,37,38,37,38,32,130,3,52,37,32,25,22,19,14,14,17,17,19,19,20,12,9,19,19,19,43,33,39,43,148,45,132,44,34,19,7,16,67,219,5,32,34,131,0,32,35,
    132,0,32,38,135,0,36,19,35,35,38,38,68,41,8,67,243,6,135,6,33,13,13,131,9,153,29,153,25,33,13,13,67,29,9,130,106,35,34,38,34,34,130,167,67,48,8,133,125,130,21,33,38,38,130,4,35,19,
    19,34,21,135,0,79,204,5,34,21,25,21,133,8,46,13,15,13,19,13,13,13,45,45,33,33,32,32,32,41,130,0,32,37,132,0,32,45,134,0,32,19,142,56,133,53,130,5,35,13,19,18,16,133,110,32,24,132,0,
    37,29,29,14,19,34,28,130,0,131,20,33,38,38,72,175,6,136,12,38,34,34,45,30,30,34,34,65,103,5,34,31,17,26,134,59,33,34,25,131,226,131,21,41,44,44,44,35,35,44,44,23,35,23,130,0,131,54,
    32,19,133,93,32,19,66,231,6,34,15,23,17,130,0,131,45,132,110,130,4,65,183,5,32,33,130,0,33,31,31,143,5,33,34,34,131,17,133,111,133,11,34,16,38,38,130,156,35,28,38,38,30,130,0,34,38,
    38,29,130,0,32,18,133,55,34,38,38,27,130,0,33,33,33,131,111,130,137,32,38,131,0,33,31,31,131,5,34,33,33,35,136,0,34,33,33,36,130,0,33,35,35,130,4,34,36,38,38,131,22,133,49,34,16,16,
    32,132,57,33,37,37,130,81,34,29,37,37,130,65,33,31,15,133,6,33,37,37,131,113,33,37,37,130,112,46,30,28,16,34,44,55,55,47,47,55,55,25,28,28,25,130,3,33,19,19,130,191,39,38,32,32,37,
    34,22,27,22,130,122,39,21,21,41,41,45,21,21,24,66,56,8,132,163,8,129,19,19,13,13,13,19,13,20,22,22,13,9,38,24,24,35,38,35,34,34,27,22,28,23,32,43,28,28,33,31,42,13,14,16,9,5,26,23,
    27,23,28,22,22,23,22,17,25,29,11,18,24,18,20,20,16,18,27,20,25,15,16,27,30,20,19,22,29,19,26,34,24,19,24,25,22,22,24,24,19,22,31,17,37,28,41,33,19,24,41,18,21,13,28,21,19,30,20,21,
    15,16,20,18,29,24,20,16,19,29,27,21,22,26,27,21,20,33,28,35,25,30,22,20,37,26,26,26,23,26,27,28,130,111,33,29,29,130,11,39,27,33,34,32,32,26,26,27,130,223,130,13,47,30,29,21,33,33,
    25,26,20,20,20,17,21,23,24,26,26,130,125,56,22,22,22,21,27,25,25,25,20,20,21,25,24,24,21,21,21,27,22,18,27,27,20,20,0,130,0,47,30,22,27,21,11,10,32,25,32,25,21,22,29,24,10,0,130,29,
    130,2,76,81,5,46,26,26,32,32,16,13,13,17,19,14,18,16,8,15,17,130,45,132,2,52,46,60,30,0,4,13,13,18,27,25,38,37,11,14,14,19,31,10,18,10,18,66,125,8,34,25,10,10,130,136,8,70,21,44,30,
    26,28,32,23,22,32,33,12,16,27,22,41,34,35,26,35,28,24,24,32,29,43,27,25,26,14,17,14,31,19,12,23,27,21,27,24,14,27,26,11,11,23,11,40,26,27,27,27,16,20,16,26,22,33,21,22,21,14,11,14,
    31,30,30,28,23,34,35,32,66,97,5,33,21,24,130,0,32,11,130,0,131,38,76,27,5,8,70,17,17,25,25,21,19,21,25,18,41,41,36,13,19,32,40,35,38,31,32,32,25,27,25,28,35,25,18,20,35,38,27,21,13,
    13,31,30,25,32,30,23,23,34,30,30,35,43,43,23,46,17,17,11,11,31,27,22,25,8,25,15,15,26,26,17,10,11,17,56,30,23,130,31,32,12,130,0,8,126,35,35,35,32,32,32,11,17,16,14,14,9,13,9,15,9,
    17,22,13,24,20,26,21,11,32,26,25,22,26,27,31,31,16,17,17,43,42,44,16,26,27,32,32,24,34,41,12,24,34,24,26,30,25,23,33,27,32,21,22,39,43,28,28,33,41,22,41,36,33,23,35,35,25,32,33,30,
    26,29,40,24,47,41,51,48,26,32,45,25,27,23,32,28,25,37,26,34,26,26,29,24,40,36,23,22,28,32,39,26,33,36,30,27,26,42,37,50,35,39,30,25,47,28,32,30,39,26,0,132,0,32,31,68,161,9,32,35,67,
    95,5,36,41,42,41,42,35,130,3,52,41,35,27,24,21,16,16,19,19,21,21,22,13,10,21,21,21,47,36,43,47,148,45,132,44,35,21,7,17,33,130,0,32,37,133,0,66,180,5,32,42,135,0,37,21,38,38,42,42,
    24,135,0,32,30,141,0,34,14,14,29,130,0,153,29,153,25,34,14,14,24,133,110,130,5,130,106,35,37,42,37,37,130,167,139,18,133,21,33,42,42,130,26,34,21,21,37,69,61,8,74,216,5,34,22,28,22,
    133,8,46,14,17,14,21,14,14,14,49,49,37,37,35,35,35,45,130,0,73,224,5,32,50,134,0,32,21,142,56,133,53,130,5,35,14,21,20,18,133,110,69,192,5,37,32,32,15,21,37,31,130,0,131,20,35,42,42,
    21,33,132,0,136,12,38,37,37,49,33,33,37,37,65,103,5,35,34,19,29,18,65,38,5,33,38,28,131,226,131,21,41,48,48,48,38,38,48,48,26,38,26,130,0,131,54,32,21,133,93,69,155,6,35,25,17,26,19,
    130,0,131,45,133,111,33,27,27,130,175,34,35,34,34,131,37,145,5,34,37,37,36,130,0,133,111,133,11,34,18,42,42,130,156,35,31,42,42,32,130,0,133,5,34,19,34,34,130,36,35,36,42,42,30,130,
    0,133,109,130,137,32,42,131,0,34,34,34,41,130,0,33,36,36,68,129,9,34,37,37,39,136,0,33,42,42,131,23,33,36,36,131,51,34,18,18,35,132,7,33,41,41,130,87,34,32,41,41,130,65,33,34,17,133,
    6,33,41,41,131,113,34,41,41,33,130,0,45,30,18,38,48,60,60,51,51,60,60,28,31,31,28,130,3,33,21,21,130,191,39,42,35,35,41,37,24,30,24,130,72,39,22,22,45,45,50,22,22,27,132,126,133,4,
    130,80,8,129,21,21,14,14,14,21,14,22,24,24,14,10,42,27,27,38,42,38,37,37,30,24,31,26,35,47,31,31,36,34,46,14,16,18,10,6,28,25,30,25,31,24,24,25,25,19,28,32,12,20,26,20,22,22,17,19,
    30,22,27,17,18,29,33,22,21,24,32,21,29,37,27,21,27,27,25,24,26,26,21,24,34,19,40,31,45,36,21,26,45,19,23,15,31,23,21,32,21,23,16,17,22,20,32,27,22,18,21,31,29,23,25,29,29,23,22,36,
    31,38,27,33,24,21,40,29,29,29,26,29,30,31,130,111,33,32,32,130,11,39,30,37,37,35,35,28,28,29,130,223,53,28,28,28,33,32,23,36,36,27,28,22,22,22,19,23,25,26,28,28,24,26,24,130,0,39,23,
    30,27,27,27,22,22,23,130,5,42,23,23,23,30,25,19,30,30,22,22,0,130,0,47,33,24,30,23,12,11,35,27,35,27,23,24,32,26,11,0,69,34,5,130,223,130,2,46,29,29,35,35,17,14,14,18,21,15,20,18,9,
    16,18,130,45,132,2,52,50,66,32,0,5,14,14,20,30,27,41,40,12,15,15,21,34,11,20,11,19,72,50,9,33,11,11,130,136,8,47,22,48,32,29,31,35,25,24,34,36,13,18,29,24,45,37,38,28,38,30,27,26,34,
    31,47,30,28,29,15,19,15,34,21,13,25,29,23,29,26,16,29,28,12,12,25,12,43,28,130,198,51,17,21,17,28,24,36,23,24,23,15,12,15,34,32,32,31,25,37,38,34,69,73,5,33,23,26,130,0,32,12,130,0,
    131,38,33,29,29,130,228,33,28,19,130,112,8,47,22,20,23,27,20,45,45,39,14,21,34,43,38,41,34,34,34,27,29,27,31,38,27,20,22,38,42,29,22,14,14,34,33,27,34,32,25,25,37,32,32,38,47,46,25,
    50,19,19,130,152,40,29,24,28,9,27,16,16,28,28,130,12,35,19,61,32,25,130,31,32,13,130,0,34,38,38,38,130,62,8,120,12,19,17,16,15,10,14,10,16,9,19,24,14,27,21,29,23,12,35,28,28,24,28,
    29,34,34,18,18,18,47,45,48,18,28,30,35,34,26,37,45,13,27,37,26,29,32,27,25,36,29,35,23,24,43,47,30,30,36,45,24,45,39,36,25,38,38,28,34,36,33,29,31,43,26,51,45,55,52,28,34,49,27,29,
    25,35,30,27,41,28,37,29,29,31,27,43,39,25,24,31,35,43,28,36,39,33,29,28,46,41,54,38,43,33,27,51,31,35,33,42,29,0,132,0,33,34,37,130,0,66,212,6,66,241,5,36,44,46,44,46,38,130,3,52,44,
    38,29,26,22,17,17,20,20,22,22,23,14,10,22,22,22,51,40,46,51,148,45,132,44,35,22,8,19,36,130,0,68,44,5,32,40,66,175,5,32,46,135,0,36,22,41,41,45,45,83,103,8,65,230,5,136,5,33,16,16,
    131,10,153,29,153,25,34,16,16,26,134,111,33,40,40,130,106,35,40,46,40,40,130,167,139,18,133,21,33,46,46,130,26,34,22,22,40,69,61,14,35,24,30,24,30,132,0,46,16,18,16,22,16,16,16,53,
    53,40,40,38,38,38,48,130,0,32,44,132,0,32,54,134,0,32,22,142,56,69,179,8,35,16,22,22,19,133,110,71,174,5,37,35,35,17,22,40,33,130,0,131,20,34,46,46,22,84,15,5,136,12,38,40,40,54,36,
    36,40,40,65,103,5,36,37,20,31,19,41,133,0,32,30,68,226,7,41,52,52,52,42,42,52,52,28,42,28,130,0,131,54,32,22,133,93,32,22,66,231,6,34,18,28,20,130,0,131,45,133,111,34,29,29,39,130,
    0,33,37,37,131,37,145,5,133,59,133,111,33,40,40,130,40,36,39,19,45,45,34,130,0,34,45,45,35,130,0,133,5,32,21,68,199,5,34,45,45,32,130,0,133,49,130,159,32,46,131,0,34,37,37,45,130,0,
    33,39,39,133,164,36,42,42,42,41,40,65,216,6,32,42,130,0,33,46,46,130,4,34,42,39,39,131,51,34,19,19,38,132,7,33,44,44,130,87,34,35,44,44,130,65,33,37,18,133,6,33,44,44,130,112,35,34,
    44,44,36,130,0,45,33,19,41,52,66,66,56,56,66,66,30,33,33,30,130,3,44,22,22,38,38,38,46,38,38,44,40,26,32,26,130,72,39,24,24,48,48,54,24,24,29,66,56,8,33,46,46,130,80,8,134,22,22,16,
    16,16,22,16,23,26,26,15,10,46,29,29,42,46,42,40,40,32,26,33,28,39,51,34,34,39,37,50,15,17,19,11,6,30,28,32,28,34,26,26,27,27,21,30,34,13,21,29,22,24,23,19,21,32,23,30,18,20,32,36,24,
    23,27,35,23,31,41,29,22,29,29,27,26,28,28,23,26,37,21,44,33,49,40,22,28,49,21,25,16,33,25,23,35,23,25,17,19,24,21,35,29,24,19,23,34,32,25,27,31,32,25,24,39,34,41,29,36,26,23,44,31,
    31,31,28,31,32,34,41,41,33,35,34,130,11,39,32,40,41,38,38,31,31,32,130,223,130,13,50,35,34,25,39,39,29,31,24,24,24,21,25,27,28,31,31,26,28,26,130,0,53,25,32,30,29,29,24,24,25,29,29,
    29,25,25,25,32,27,21,33,33,24,24,0,130,0,49,36,26,32,25,13,12,38,29,38,29,25,26,34,28,12,0,24,25,130,57,32,24,75,67,5,46,32,32,38,38,19,16,16,20,23,17,22,19,10,18,20,130,45,135,2,32,
    4,130,8,32,3,130,3,34,36,0,1,130,5,36,0,2,254,0,3,131,9,33,5,8,130,7,40,10,0,0,7,226,0,4,2,218,130,25,8,126,120,0,64,0,5,0,56,0,13,0,126,0,255,1,5,1,25,1,47,1,49,1,66,1,83,1,97,1,115,
    1,120,1,126,1,143,1,146,1,237,2,89,2,199,2,201,2,221,3,1,3,4,3,11,3,40,3,88,19,244,19,245,19,253,22,127,24,245,29,163,32,10,32,20,32,26,32,30,32,34,32,38,32,48,32,58,32,68,32,116,32,
    172,33,34,33,38,34,2,34,6,34,15,34,18,34,26,34,30,34,43,34,54,34,72,34,96,34,101,37,202,37,204,171,191,251,2,255,255,130,129,42,13,0,32,0,160,1,4,1,24,1,46,130,121,38,65,1,82,1,96,
    1,114,130,121,32,125,132,121,32,234,130,121,32,198,130,121,32,216,138,121,32,160,130,121,36,248,20,0,24,176,132,121,38,19,32,24,32,28,32,32,132,121,32,57,144,121,34,17,34,25,138,121,
    32,100,132,121,8,70,112,251,1,255,255,255,244,255,227,0,0,3,189,3,177,3,149,255,165,255,160,255,94,255,131,3,89,255,67,255,104,3,48,255,21,2,219,2,103,0,0,254,16,0,0,1,186,1,184,1,
    178,1,166,1,102,237,89,240,39,240,37,237,84,235,36,231,60,228,17,224,159,130,187,130,2,53,224,134,224,150,224,133,224,120,224,130,224,17,223,107,223,121,222,151,222,163,222,140,130,
    24,52,0,222,117,222,113,225,228,222,96,222,48,222,49,218,239,219,44,88,179,5,191,65,141,5,33,0,116,131,31,151,3,33,1,22,130,117,130,3,152,31,36,6,1,10,1,14,150,29,34,252,0,254,150,
    25,8,228,163,0,164,0,132,0,133,0,247,0,151,0,231,0,134,0,143,0,140,0,157,0,170,0,165,0,138,0,139,0,217,0,131,0,148,0,241,0,242,0,142,0,152,0,136,0,195,0,221,0,240,0,158,0,171,0,244,
    0,243,0,245,0,162,0,173,0,201,0,199,0,174,0,98,0,99,0,145,0,100,0,203,0,101,0,200,0,202,0,207,0,204,0,205,0,206,0,232,0,102,0,210,0,208,0,209,0,175,0,103,0,239,0,146,0,213,0,211,0,
    212,0,104,0,234,0,236,0,137,0,106,0,105,0,107,0,109,0,108,0,110,0,160,0,111,0,113,0,112,0,114,0,115,0,117,0,116,0,118,0,119,0,233,0,120,0,122,0,121,0,123,0,125,0,124,0,184,0,161,0,
    127,0,126,0,128,0,129,0,235,0,237,0,186,0,215,0,224,0,218,0,219,0,220,0,223,0,216,0,222,0,182,0,183,0,196,0,180,0,181,0,197,0,130,0,194,0,135,0,154,0,238,130,183,37,166,0,6,2,10,0,
    130,0,33,1,0,65,109,6,130,11,134,2,34,1,0,2,130,1,131,12,33,2,0,157,0,32,1,132,30,8,188,3,0,4,0,5,0,6,0,7,0,8,0,9,0,10,0,11,0,12,0,13,0,14,0,15,0,16,0,17,0,18,0,19,0,20,0,21,0,22,0,
    23,0,24,0,25,0,26,0,27,0,28,0,29,0,30,0,31,0,32,0,33,0,34,0,35,0,36,0,37,0,38,0,39,0,40,0,41,0,42,0,43,0,44,0,45,0,46,0,47,0,48,0,49,0,50,0,51,0,52,0,53,0,54,0,55,0,56,0,57,0,58,0,
    59,0,60,0,61,0,62,0,63,0,64,0,65,0,66,0,67,0,68,0,69,0,70,0,71,0,72,0,73,0,74,0,75,0,76,0,77,0,78,0,79,0,80,0,81,0,82,0,83,0,84,0,85,0,86,0,87,0,88,0,89,0,90,0,91,0,92,0,93,0,94,0,
    95,0,96,0,97,130,193,8,112,98,0,99,0,100,0,101,0,102,0,103,0,104,0,105,0,106,0,107,0,108,0,109,0,110,0,111,0,112,0,113,0,114,0,115,0,116,0,117,0,118,0,119,0,120,0,121,0,122,0,123,0,
    124,0,125,0,126,0,127,0,128,0,129,0,130,0,131,0,132,0,133,0,134,0,135,0,136,0,137,0,139,0,140,0,141,0,142,0,143,0,144,0,145,0,146,0,147,0,148,0,149,0,150,0,151,0,152,0,153,0,154,0,
    155,130,115,8,94,156,0,157,0,158,0,159,0,160,0,161,0,162,0,164,0,165,0,166,0,167,0,168,0,169,0,170,0,171,0,172,0,163,0,173,0,174,0,175,0,176,0,177,0,178,0,179,0,180,0,181,0,182,0,183,
    0,184,0,185,0,186,0,187,0,188,0,189,0,190,0,191,0,192,0,193,0,194,0,195,0,196,0,197,0,198,0,199,0,200,0,201,0,202,0,203,66,109,6,54,207,0,208,0,209,0,0,0,210,0,211,0,212,0,213,0,214,
    0,215,0,216,0,217,66,33,6,38,221,0,222,0,223,0,224,16,4,227,2,218,33,12,0,130,0,33,7,84,130,4,131,2,32,155,130,4,32,13,134,3,32,1,130,7,32,32,130,3,32,126,130,3,32,3,130,3,32,160,130,
    3,32,161,130,3,32,163,130,3,32,162,134,7,32,132,130,7,32,164,134,3,32,247,130,7,32,165,134,3,32,151,130,7,32,166,134,3,32,231,130,7,32,167,134,3,32,134,130,7,32,168,134,3,32,143,130,
    7,32,169,134,3,32,140,130,7,32,170,134,3,32,157,130,7,32,171,134,3,131,19,32,172,130,11,131,3,32,165,130,7,32,173,130,3,32,174,130,3,32,138,130,3,32,175,134,3,32,217,130,7,32,176,134,
    3,32,131,130,7,32,177,134,3,32,148,130,7,32,178,130,3,32,179,130,3,32,241,130,3,32,180,134,3,32,142,130,7,32,181,134,3,32,152,130,7,32,182,134,3,32,136,130,7,32,183,134,3,32,195,130,
    7,32,184,134,3,32,221,130,7,32,185,134,3,32,240,130,7,32,186,134,3,32,158,130,7,32,187,134,3,131,175,32,188,130,11,131,3,32,244,130,7,32,189,134,3,32,243,130,7,32,190,134,3,32,245,
    130,7,32,191,134,3,32,162,130,7,32,192,134,3,32,173,130,7,32,193,134,3,32,201,130,7,32,194,134,3,32,199,130,7,32,195,134,3,131,243,32,196,130,11,32,197,130,3,32,98,130,3,32,198,134,
    3,32,145,130,7,32,199,134,3,32,100,130,7,32,200,134,3,32,203,130,7,32,201,134,3,32,101,130,7,32,202,134,3,32,200,130,7,32,203,134,3,32,202,130,7,32,204,134,3,32,207,130,7,32,205,130,
    3,32,207,134,15,32,208,130,7,131,3,32,232,130,7,32,209,134,3,32,102,130,7,32,210,138,3,32,211,130,11,32,212,134,39,32,213,130,7,131,3,32,175,130,7,32,214,134,3,32,103,130,7,32,215,
    134,3,32,239,130,7,32,216,134,3,32,146,130,7,32,217,134,3,32,213,130,7,32,218,130,3,32,219,130,3,131,79,32,220,130,7,131,3,32,104,130,7,32,221,134,3,32,234,130,7,32,222,134,3,32,236,
    130,7,32,223,134,3,32,137,130,7,32,224,134,3,32,106,130,7,32,225,134,3,32,105,130,7,32,226,134,3,32,107,130,7,32,227,134,3,32,109,130,7,32,228,134,3,32,108,130,7,32,229,134,3,32,110,
    130,7,32,230,134,3,32,160,130,7,32,231,134,3,32,111,130,7,32,232,134,3,32,113,130,7,32,233,134,3,32,112,130,7,32,234,130,3,32,235,130,3,32,114,130,3,131,147,131,3,32,117,130,11,32,
    237,134,3,32,116,130,7,32,238,130,3,32,239,130,3,32,118,130,3,32,240,134,3,32,233,66,139,6,32,241,130,7,32,120,130,3,32,242,134,3,32,122,130,7,32,243,134,3,32,121,130,7,32,244,134,
    3,32,123,130,7,32,245,134,3,32,125,130,7,32,246,134,3,32,124,130,7,32,247,134,3,32,184,130,7,32,248,134,3,32,161,130,7,32,249,134,3,32,127,130,7,32,250,134,3,32,126,130,7,32,251,130,
    3,32,252,130,3,32,128,130,3,32,253,134,3,32,235,130,7,32,254,134,3,32,237,130,7,32,255,134,3,44,186,0,0,1,4,0,0,1,5,0,0,4,193,130,7,32,24,130,3,32,25,130,11,32,201,130,7,32,46,130,
    3,32,47,130,11,32,195,130,7,32,49,133,3,33,0,214,130,7,32,65,130,3,32,66,65,131,5,33,1,82,130,11,32,83,67,147,5,33,1,96,130,11,32,97,65,131,5,33,1,114,130,11,32,115,130,59,32,203,130,
    7,32,120,133,3,33,0,187,130,7,32,125,130,3,131,163,32,229,130,7,32,143,133,3,33,4,191,130,7,32,146,133,3,33,0,167,130,7,32,234,130,3,130,163,37,4,197,0,0,2,89,133,3,33,4,192,130,7,
    32,198,133,3,33,0,215,130,7,32,199,133,3,33,0,224,130,7,32,201,133,3,33,0,217,130,7,32,216,130,3,32,218,66,107,5,33,2,219,130,11,130,3,33,0,223,130,7,32,220,133,3,33,0,216,130,7,32,
    221,133,3,37,0,222,0,0,3,1,133,3,32,4,130,155,33,3,4,130,11,130,3,33,4,188,130,7,32,11,133,3,33,4,189,130,7,32,40,133,3,33,4,206,130,7,32,88,133,3,37,4,190,0,0,19,160,130,3,32,244,
    65,131,5,33,19,245,130,11,130,3,33,4,28,130,7,32,248,130,3,32,253,130,251,35,29,0,0,20,130,30,41,22,127,0,0,1,84,0,0,24,176,130,3,130,35,37,3,212,0,0,29,163,133,3,37,4,223,0,0,32,10,
    133,3,33,4,27,130,7,32,19,130,3,131,52,32,178,130,7,32,24,130,3,32,25,68,179,5,33,32,26,130,11,130,3,33,0,196,130,7,130,99,32,32,130,91,33,0,180,130,11,32,30,133,3,33,0,197,130,7,32,
    32,133,3,37,0,130,0,0,32,33,133,3,33,0,194,130,7,32,34,133,3,33,0,135,130,7,32,38,133,3,33,0,172,130,7,32,48,133,3,33,0,198,130,7,32,57,130,3,32,58,68,191,5,33,32,68,130,11,130,3,33,
    0,188,130,7,32,116,133,3,33,0,246,130,7,131,51,130,3,36,0,189,0,0,33,130,83,131,3,33,0,141,130,11,130,83,32,33,130,3,37,0,159,0,0,34,2,133,3,33,0,153,130,7,32,6,133,3,33,0,169,130,
    7,32,15,133,3,33,0,155,130,7,32,17,133,3,33,0,154,130,7,32,18,133,3,33,0,238,130,7,130,235,32,34,130,3,33,0,195,130,11,130,235,32,34,130,3,33,0,166,130,11,130,227,32,34,130,3,33,0,
    147,130,11,32,43,133,3,33,0,156,130,7,32,54,133,3,32,4,130,39,33,34,72,130,11,130,3,33,0,168,130,7,32,96,133,3,33,0,144,130,7,32,100,130,3,40,101,0,0,0,149,0,0,37,202,133,3,33,0,185,
    130,7,32,204,133,3,37,0,248,0,0,171,112,130,3,40,191,0,0,4,35,0,0,251,1,130,3,40,2,0,0,0,192,0,1,4,176,130,3,32,211,130,23,32,115,130,7,32,216,130,3,32,251,130,11,35,151,0,1,26,130,
    23,32,26,131,47,8,123,207,0,0,64,81,100,84,82,79,78,77,76,75,74,73,72,71,70,67,66,65,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,
    32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,44,1,24,176,24,67,88,69,106,176,25,67,96,176,12,35,68,35,16,32,176,12,78,240,77,47,176,0,18,
    27,33,35,17,32,51,47,89,45,134,37,50,176,5,43,176,0,19,75,176,20,80,88,177,0,64,56,89,176,6,43,143,34,38,78,176,3,37,16,242,33,130,60,38,77,27,32,69,176,4,37,130,2,44,35,74,97,100,
    176,40,82,88,33,35,16,214,27,136,31,32,89,130,86,41,176,26,67,88,33,33,27,176,2,37,130,2,32,73,130,26,130,2,38,74,97,32,100,176,16,80,130,23,130,24,33,3,37,130,17,36,73,176,0,80,88,
    131,3,35,184,255,226,56,130,20,42,0,56,33,89,27,176,0,82,88,176,30,135,14,32,89,131,77,133,202,138,164,36,185,0,0,255,192,142,166,50,78,1,138,16,177,12,25,67,68,176,0,20,177,0,12,226,
    176,0,21,131,38,40,240,56,0,176,0,60,176,40,43,130,136,32,16,130,9,132,242,44,0,47,176,1,20,242,176,1,19,176,1,21,77,130,210,65,7,13,131,54,33,224,56,65,3,18,43,69,100,106,35,69,100,
    105,176,25,67,100,96,65,83,9,32,240,65,81,5,43,33,32,138,32,138,82,88,17,51,27,33,33,132,180,48,75,176,200,81,88,177,11,10,67,35,67,101,10,89,45,44,0,132,16,35,0,177,10,11,130,17,32,
    11,131,16,40,176,12,35,112,177,1,12,62,1,132,8,45,2,12,69,58,177,2,0,8,13,45,44,176,18,43,130,174,32,69,131,3,36,106,176,64,139,96,130,8,36,35,68,33,33,33,130,25,32,19,137,25,35,184,
    255,192,140,139,26,32,0,130,54,133,36,35,0,176,19,43,132,9,32,1,132,115,36,176,6,67,176,7,133,132,61,32,105,176,64,97,176,0,139,32,177,44,192,138,140,184,16,0,98,96,43,12,100,35,100,
    97,92,88,176,3,97,130,148,43,177,0,3,37,69,104,84,176,28,75,80,90,130,18,34,37,69,176,130,15,39,96,104,32,176,4,37,35,68,132,4,32,27,130,17,36,32,69,104,32,138,130,15,130,27,33,104,
    96,130,15,33,35,68,130,58,141,23,35,100,104,101,96,130,47,36,176,1,96,35,68,130,183,40,9,67,88,135,33,192,27,176,18,130,7,37,69,176,17,43,176,13,130,62,41,13,122,228,27,3,138,69,24,
    105,32,131,14,39,138,138,135,32,176,160,81,88,139,29,33,33,176,130,35,44,89,89,89,24,45,44,32,138,69,35,69,104,96,130,76,39,69,106,66,45,44,1,24,47,65,182,8,33,4,37,130,105,46,73,100,
    35,69,100,105,176,64,139,97,32,176,128,98,106,66,123,5,34,97,140,176,66,255,6,40,33,138,16,176,12,246,33,27,33,130,0,130,172,36,1,176,24,67,88,65,100,7,34,100,96,106,130,188,33,69,
    106,130,57,39,4,37,69,106,32,138,139,101,132,234,32,140,132,218,38,33,33,27,32,69,106,68,131,3,131,56,36,32,69,176,0,85,130,61,37,90,88,69,104,35,69,137,112,34,32,138,35,130,63,33,
    3,37,143,59,36,33,33,176,25,43,131,56,34,138,138,69,131,162,33,97,100,130,189,67,64,5,130,24,132,134,132,191,130,107,35,176,27,43,1,130,147,35,67,176,64,84,132,6,35,0,84,90,88,130,
    22,130,113,35,64,97,68,89,134,18,132,6,32,64,131,25,33,4,37,131,25,35,96,68,89,89,130,211,34,33,45,44,131,42,40,176,7,37,135,5,46,35,32,138,131,97,39,7,37,176,20,43,16,33,196,130,27,
    39,192,45,44,75,82,88,69,68,130,255,130,139,40,176,2,67,88,61,237,24,27,237,130,11,33,75,80,136,22,33,1,24,130,35,35,138,47,237,27,65,35,5,36,75,83,35,75,81,132,129,33,69,104,130,91,
    132,4,33,96,84,67,192,5,34,2,37,69,65,249,6,34,33,33,89,134,47,132,85,35,205,24,27,205,130,85,8,36,70,35,70,96,138,138,70,35,32,70,138,96,138,97,184,255,128,98,35,32,16,35,138,177,
    12,12,138,112,69,96,32,176,0,80,88,176,1,130,23,46,186,139,27,176,70,140,89,176,16,96,104,1,58,45,44,130,208,41,3,37,70,82,75,176,19,81,91,88,130,233,35,70,32,104,97,68,62,5,38,63,
    35,33,56,27,33,17,130,90,133,35,130,63,33,2,37,147,30,67,103,6,38,176,7,67,176,6,67,11,130,47,57,138,16,236,45,44,176,12,67,88,33,27,32,70,176,0,82,88,184,255,240,56,27,176,16,56,89,
    130,28,130,137,37,85,88,184,16,0,99,130,101,33,69,100,132,4,35,97,176,0,83,130,123,34,27,176,64,130,122,36,89,37,69,105,83,65,32,6,33,27,33,66,48,7,36,97,100,176,40,81,134,21,130,68,
    49,33,33,12,100,35,100,139,184,64,0,98,45,44,33,176,128,81,88,133,15,44,32,0,98,27,178,0,64,47,43,89,176,2,96,131,25,32,192,135,25,33,21,85,131,25,32,128,135,25,136,62,38,96,35,33,
    45,44,24,75,130,127,66,211,28,34,12,35,68,66,207,8,38,138,17,35,18,32,57,47,130,134,66,242,5,36,73,100,176,192,84,130,227,36,248,56,176,8,56,65,238,6,37,19,67,88,3,27,2,131,34,130,
    9,34,2,27,3,131,9,42,10,43,35,16,32,60,176,23,43,45,44,130,56,62,184,255,240,56,176,40,43,138,16,35,32,208,35,176,16,43,176,5,67,88,192,27,60,89,32,16,17,176,0,18,1,130,35,32,72,130,
    40,32,1,130,41,8,33,16,208,35,201,1,176,1,19,176,0,20,16,176,1,60,176,1,22,45,44,1,176,0,19,176,1,176,3,37,73,176,3,23,56,130,28,33,45,44,66,60,6,35,32,69,138,96,66,116,6,139,214,65,
    211,5,34,73,100,104,67,175,17,131,219,40,176,4,37,16,35,176,12,246,27,67,39,5,34,17,18,35,131,225,34,204,33,33,67,242,8,40,5,37,70,35,69,100,97,27,33,133,33,42,74,89,176,14,35,68,35,
    16,176,14,236,132,4,130,175,42,27,67,88,32,176,1,96,69,176,0,81,132,8,50,32,69,32,104,176,0,85,88,176,32,96,68,33,27,33,33,33,89,27,131,31,135,22,43,184,255,224,96,68,176,28,75,80,
    88,32,69,131,34,32,27,66,214,6,32,89,130,8,130,77,66,107,15,33,45,44,66,225,39,40,128,139,27,176,129,140,89,104,58,130,69,33,64,42,130,193,33,32,53,130,150,130,12,35,2,67,84,88,65,
    37,6,32,56,130,143,130,104,67,118,5,32,73,131,202,141,31,65,69,8,68,190,7,65,93,17,33,75,84,130,184,65,107,8,139,87,130,74,33,0,33,130,30,132,100,134,87,34,176,70,43,135,74,32,176,
    131,135,33,176,71,132,15,32,89,135,30,32,72,142,30,32,73,132,15,130,30,45,0,176,2,37,32,17,73,176,0,81,88,184,255,192,130,82,139,19,35,83,88,176,64,133,18,37,75,82,88,125,27,122,130,
    27,8,33,177,2,0,66,177,35,1,136,81,177,64,1,136,83,90,88,185,16,0,0,32,136,84,88,178,2,1,2,67,96,66,89,177,36,130,27,37,88,185,32,0,0,64,132,21,32,2,131,21,131,20,131,34,32,32,131,
    12,34,0,75,1,130,77,34,178,2,8,132,47,37,27,185,64,0,0,128,132,42,32,4,132,16,132,15,35,99,184,1,0,132,19,133,36,130,19,36,1,0,99,184,2,133,19,32,16,135,39,32,2,130,19,32,4,133,19,
    32,64,132,19,32,89,130,0,8,33,45,44,32,69,105,68,45,0,64,255,248,149,44,31,247,131,44,31,246,145,44,31,245,125,44,31,32,247,9,24,1,74,32,245,132,5,32,243,132,5,32,241,132,5,32,240,
    132,5,32,239,132,5,32,237,132,5,32,234,132,5,32,220,132,5,32,218,132,5,32,216,132,5,32,211,132,5,32,205,132,5,32,204,132,5,32,202,132,5,32,201,132,5,32,200,132,5,32,198,132,5,32,196,
    132,5,32,194,132,5,32,192,132,5,32,189,132,5,32,180,132,5,32,177,132,5,32,175,132,5,32,140,132,5,32,139,132,5,32,138,132,5,32,137,132,5,32,132,132,5,32,131,132,5,32,127,132,5,32,126,
    132,5,32,125,132,5,32,98,132,5,32,80,132,5,32,72,132,5,32,14,131,5,38,223,224,22,23,28,212,213,130,4,42,123,64,70,124,22,27,123,124,25,209,210,130,13,33,118,119,130,4,33,109,112,130,
    4,33,108,111,130,4,47,107,110,22,27,107,110,127,114,26,116,117,20,21,28,103,106,130,4,33,102,105,130,4,48,101,104,20,27,101,104,133,113,26,25,115,22,122,67,25,33,121,130,3,8,55,184,
    255,224,64,23,241,41,43,57,230,231,3,43,64,231,44,46,57,64,231,27,28,57,64,84,31,31,57,184,255,192,64,39,203,35,35,57,64,217,41,42,57,83,82,2,43,82,226,2,43,32,176,11,13,57,242,130,
    8,41,195,197,2,43,64,218,23,25,57,219,130,12,130,43,35,179,226,48,49,132,51,50,25,226,29,30,57,64,214,23,23,57,235,190,2,43,190,154,2,43,153,130,3,35,154,149,5,43,131,109,44,12,238,
    20,22,57,146,145,3,43,145,149,36,31,131,16,42,11,99,24,25,57,102,99,1,150,149,2,132,32,38,9,149,37,40,57,64,216,130,88,130,84,39,64,9,196,34,37,57,64,194,130,76,130,13,35,179,192,26,
    27,131,98,39,183,192,16,18,57,205,204,3,130,47,34,192,183,201,130,161,33,202,200,132,11,34,64,22,240,130,156,46,234,126,2,43,48,98,12,12,6,67,177,175,2,43,175,130,13,131,58,34,211,
    15,18,131,58,35,179,189,39,40,132,165,57,28,140,30,30,57,137,138,4,43,138,132,10,43,139,132,3,43,129,127,1,43,125,126,4,43,126,130,11,131,151,33,216,98,130,151,36,98,132,1,43,136,130,
    3,32,135,130,3,32,133,130,3,8,206,131,132,4,43,233,132,249,132,2,105,132,1,39,132,1,27,55,21,24,54,21,21,53,21,18,52,21,15,51,21,12,50,21,9,49,21,6,48,21,3,47,21,0,46,21,38,39,14,40,
    39,15,42,43,14,44,43,15,34,35,14,36,35,15,30,31,14,32,31,15,112,42,224,42,2,0,60,1,16,36,64,36,112,36,160,36,4,15,16,17,12,9,10,11,12,6,7,8,12,3,4,5,12,0,1,2,12,38,6,28,31,6,3,24,31,
    15,3,63,3,223,3,3,159,0,223,0,2,15,23,31,23,47,23,3,15,20,31,20,47,20,3,27,1,29,13,24,7,26,13,21,16,23,13,18,4,20,13,47,27,1,44,60,42,60,40,60,38,60,36,60,34,60,32,60,30,60,27,60,24,
    60,21,60,18,60,15,60,9,60,6,60,3,60,0,60,80,51,84,1,176,18,75,0,75,84,66,176,19,1,130,7,55,83,66,176,51,43,75,184,3,32,82,176,50,43,75,176,9,80,91,88,177,1,1,142,89,130,21,43,176,2,
    136,184,1,0,84,176,4,136,184,2,130,6,39,18,67,90,91,88,184,1,25,131,30,58,133,27,185,0,1,1,0,176,75,96,133,141,89,43,43,29,176,100,75,83,88,176,128,29,89,176,50,131,8,44,144,29,89,
    0,75,176,50,81,176,27,35,66,43,143,0,32,115,131,16,32,115,130,0,134,24,136,9,44,177,40,38,69,176,42,69,97,176,44,69,96,68,139,53,35,1,115,116,117,143,69,139,15,34,0,43,43,132,84,150,
    35,37,176,24,176,63,75,83,144,136,33,177,9,131,240,38,80,82,66,75,176,8,82,130,3,36,80,91,176,26,35,130,12,32,200,130,12,32,54,130,12,41,12,35,66,177,0,2,67,84,177,2,131,4,32,6,130,
    4,8,80,91,91,88,64,76,247,100,245,100,243,100,241,100,240,100,239,100,237,100,234,100,220,100,218,100,216,100,211,100,205,100,204,100,202,100,201,100,200,100,198,100,196,100,194,100,
    192,100,189,100,180,100,177,100,175,100,140,100,139,100,138,100,137,100,132,100,131,100,127,100,126,100,125,100,98,100,80,100,72,100,14,100,150,176,142,22,32,89,142,15,154,14,44,0,
    0,0,5,236,0,22,0,0,5,154,0,24,135,5,130,20,136,2,33,4,0,131,23,37,0,0,255,232,255,137,137,5,37,0,0,254,41,255,245,132,47,34,0,254,186,133,44,47,2,205,255,237,5,161,0,13,2,72,255,235,
    3,89,0,13,130,45,32,235,133,25,149,5,33,254,174,135,23,32,8,130,8,131,3,130,107,130,9,33,0,138,131,4,130,3,32,115,130,3,131,2,32,107,130,4,36,113,0,128,0,98,130,7,151,2,34,214,0,214,
    151,26,154,23,36,176,0,168,0,145,134,31,34,168,0,164,134,9,34,0,0,168,130,11,34,155,0,104,136,15,34,151,0,140,132,11,34,138,0,131,132,7,32,148,136,163,132,14,156,4,43,224,0,224,0,160,
    0,228,1,190,1,2,0,143,0,34,134,0,134,130,18,46,197,0,195,0,129,0,74,0,90,0,81,5,219,5,219,130,19,36,88,0,144,0,88,130,99,33,128,0,137,0,32,207,132,10,131,229,38,120,0,119,0,122,0,97,
    130,3,32,129,132,19,36,0,0,129,0,107,134,9,34,120,0,109,132,9,32,146,130,19,46,146,0,139,0,160,0,151,1,211,0,76,0,116,0,100,130,213,34,176,0,152,130,5,65,95,7,34,0,0,66,130,43,139,
    3,51,182,0,0,1,14,0,0,2,54,0,0,3,100,0,0,4,164,0,0,6,130,45,36,6,194,0,0,7,130,19,37,7,178,0,0,8,82,130,3,40,196,0,0,9,38,0,0,9,92,130,3,32,162,130,3,36,218,0,0,10,86,130,3,35,168,
    0,0,11,130,31,36,12,32,0,0,12,130,59,37,13,78,0,0,14,26,130,3,39,130,0,0,15,154,0,0,16,130,83,37,16,230,0,0,17,128,130,3,36,240,0,0,18,68,130,3,43,180,0,0,19,126,0,0,20,216,0,0,21,
    130,221,40,23,2,0,0,23,212,0,0,24,130,63,45,25,60,0,0,25,198,0,0,26,186,0,0,27,98,130,3,47,242,0,0,28,134,0,0,29,100,0,0,30,6,0,0,31,130,103,44,32,20,0,0,32,248,0,0,33,196,0,0,35,130,
    19,41,36,38,0,0,37,116,0,0,38,22,130,3,130,183,32,39,130,23,36,41,114,0,0,42,130,199,36,43,46,0,0,43,130,67,37,44,74,0,0,44,126,130,3,36,210,0,0,45,34,130,3,130,15,40,45,130,0,0,46,
    122,0,0,47,130,103,36,48,30,0,0,49,130,75,44,49,250,0,0,50,184,0,0,51,220,0,0,52,130,143,53,53,44,0,0,53,226,0,0,54,178,0,0,55,4,0,0,56,62,0,0,57,6,130,3,48,222,0,0,58,198,0,0,59,172,
    0,0,60,64,0,0,61,76,130,3,39,254,0,0,62,206,0,0,63,130,87,48,64,188,0,0,65,240,0,0,66,210,0,0,67,150,0,0,68,130,35,36,68,110,0,0,69,130,103,33,69,158,130,7,36,218,0,0,71,162,130,3,
    32,186,130,3,35,232,0,0,72,130,187,32,72,130,67,32,72,130,163,32,72,130,83,32,72,130,31,37,73,10,0,0,73,60,130,3,32,108,130,3,32,160,130,3,32,184,130,3,36,230,0,0,74,20,130,3,32,68,
    130,3,32,118,130,3,130,163,37,74,208,0,0,75,0,130,3,32,52,130,3,32,100,130,3,32,148,130,3,32,194,130,3,130,127,32,76,130,235,37,76,80,0,0,76,128,130,3,32,174,130,3,36,220,0,0,77,14,
    130,3,32,114,130,3,35,244,0,0,78,130,39,37,79,96,0,0,80,168,130,3,130,47,36,81,156,0,0,82,130,83,44,82,180,0,0,83,214,0,0,84,226,0,0,85,130,27,32,85,130,143,32,86,130,71,32,87,130,
    59,40,88,2,0,0,89,124,0,0,90,130,67,44,90,254,0,0,91,138,0,0,92,24,0,0,93,130,27,37,93,172,0,0,94,116,130,3,36,224,0,0,95,62,130,3,36,192,0,0,96,118,130,3,130,227,40,98,12,0,0,99,92,
    0,0,100,130,99,32,101,130,167,132,3,130,187,32,102,130,207,41,102,122,0,0,103,72,0,0,104,66,130,3,130,123,40,105,30,0,0,105,136,0,0,106,130,15,33,106,112,130,7,130,255,41,106,206,0,
    0,107,242,0,0,109,106,130,3,32,146,130,3,36,186,0,0,110,8,130,3,32,86,130,3,32,140,130,3,36,190,0,0,111,94,130,3,130,115,37,111,240,0,0,112,36,130,3,36,82,0,0,113,102,130,3,130,195,
    45,113,234,0,0,114,246,0,0,115,222,0,0,116,132,130,3,35,196,0,0,117,130,147,32,117,130,11,37,119,78,0,0,119,126,130,3,32,174,130,3,130,31,37,120,16,0,0,120,64,130,3,130,123,33,120,
    158,130,7,130,167,32,120,130,235,37,121,44,0,0,121,90,130,3,130,155,33,121,182,130,7,35,228,0,0,122,130,191,32,122,130,119,32,122,130,39,37,123,24,0,0,123,66,130,3,32,154,130,3,36,
    214,0,0,124,92,130,3,130,35,36,125,94,0,0,125,130,91,32,125,130,151,32,126,130,15,37,127,120,0,0,127,166,130,3,36,210,0,0,128,0,130,3,130,87,45,128,116,0,0,129,108,0,0,130,212,0,0,
    131,2,130,3,32,48,130,3,40,230,0,0,132,208,0,0,133,28,130,3,32,124,130,3,40,224,0,0,134,96,0,0,135,0,130,3,32,42,130,3,130,251,37,135,130,0,0,136,8,130,3,44,226,0,0,138,248,0,0,139,
    200,0,0,141,26,130,3,51,188,0,0,143,22,0,0,144,164,0,0,145,38,0,0,146,126,0,0,147,130,35,32,149,130,43,41,149,162,0,0,150,160,0,0,151,32,130,3,43,184,0,0,153,56,0,0,154,88,0,0,155,
    130,127,33,155,180,130,7,39,238,0,0,157,110,0,0,159,130,119,32,160,130,99,36,161,6,0,0,162,130,35,45,163,82,0,0,164,74,0,0,166,36,0,0,167,38,130,3,51,206,0,0,168,234,0,0,169,210,0,
    0,170,246,0,0,171,222,0,0,172,130,11,37,173,148,0,0,174,2,130,3,35,198,0,0,175,130,135,57,177,108,0,0,177,192,0,0,179,110,0,0,181,24,0,0,183,64,0,0,184,240,0,0,186,92,130,3,130,107,
    32,188,130,183,49,189,232,0,0,190,128,0,0,191,166,0,0,193,124,0,0,194,70,130,3,40,218,0,0,195,208,0,0,197,58,130,3,130,47,57,198,162,0,0,199,94,0,0,200,96,0,0,201,174,0,0,203,84,0,
    0,205,130,0,0,206,0,130,3,130,27,40,207,116,0,0,208,126,0,0,210,130,99,40,210,228,0,0,211,236,0,0,212,130,215,32,213,130,243,8,32,213,230,0,0,214,186,0,0,216,128,0,0,217,194,0,0,219,
    238,0,0,221,76,0,0,222,226,0,0,223,234,0,0,225,130,35,57,227,124,0,0,228,216,0,0,230,48,0,0,231,10,0,0,232,156,0,0,233,200,0,0,234,22,130,3,130,95,33,234,202,130,7,35,242,0,0,235,130,
    31,33,235,106,130,7,36,190,0,0,236,66,130,3,130,119,37,236,254,0,0,237,44,130,3,32,184,130,3,36,228,0,0,238,38,130,3,32,82,130,3,36,126,0,0,239,2,130,3,130,51,32,239,130,187,33,239,
    138,130,11,130,35,32,239,130,35,37,240,42,0,0,240,108,130,3,32,176,130,3,36,220,0,0,241,56,130,3,32,120,130,3,130,31,36,242,4,0,0,242,130,51,33,242,148,130,7,35,214,0,0,243,130,211,
    32,243,130,179,37,244,246,0,0,245,142,130,3,36,216,0,0,246,34,130,3,35,158,0,0,247,130,159,32,247,130,39,32,248,130,75,36,248,236,0,0,249,130,87,33,249,122,130,7,32,166,130,3,130,47,
    37,250,52,0,0,250,90,130,3,32,128,130,3,130,19,37,250,204,0,0,251,86,130,3,36,136,0,0,252,14,130,3,32,60,130,3,130,203,32,252,130,59,36,253,50,0,0,253,130,167,36,253,138,0,0,254,130,
    131,32,254,130,15,33,254,96,130,11,32,140,130,3,32,186,130,3,35,230,0,0,255,130,231,33,255,110,130,7,130,123,37,255,202,0,1,0,20,130,3,32,88,130,3,32,164,130,3,36,208,0,1,1,52,130,
    3,32,118,130,3,36,184,0,1,2,46,130,3,32,86,130,3,130,11,41,3,94,0,1,3,144,0,1,4,54,130,3,36,100,0,1,5,12,130,3,32,58,130,3,32,126,130,3,32,170,130,3,36,214,0,1,6,136,130,3,32,182,130,
    3,36,250,0,1,7,38,130,3,32,106,130,3,32,150,130,3,36,242,0,1,8,52,130,3,130,55,32,8,130,67,37,8,216,0,1,9,26,130,3,130,83,33,9,138,130,7,36,230,0,1,10,40,130,3,35,154,0,1,11,130,23,
    33,11,64,130,7,32,102,130,3,32,140,130,3,36,178,0,1,12,110,130,3,36,160,0,1,13,92,130,3,130,47,37,14,76,0,1,14,122,130,3,130,191,37,15,142,0,1,15,186,130,3,36,254,0,1,16,42,130,3,130,
    43,32,16,130,71,37,16,246,0,1,17,56,130,3,32,124,130,3,36,168,0,1,18,4,130,3,32,70,130,3,130,63,32,18,130,163,37,19,18,0,1,19,82,130,3,36,196,0,1,20,80,130,3,36,220,0,1,21,2,130,3,
    32,40,130,3,32,78,130,3,35,116,0,1,22,130,15,32,22,130,183,33,22,190,130,11,36,236,0,1,23,120,130,3,32,166,130,3,130,91,36,24,134,0,1,24,130,155,32,24,130,11,36,25,34,0,1,25,130,175,
    33,25,146,130,7,36,238,0,1,26,48,130,3,130,63,32,26,130,179,37,26,252,0,1,27,62,130,3,32,130,130,3,36,174,0,1,28,10,130,3,32,74,130,3,35,188,0,1,29,130,55,33,29,136,130,7,32,204,130,
    3,36,254,0,1,30,68,130,3,32,114,130,3,130,87,36,30,224,0,1,31,130,71,32,31,130,71,32,31,130,71,33,31,208,130,15,36,252,0,1,32,64,130,3,32,108,130,3,35,200,0,1,33,130,75,33,33,58,130,
    7,130,123,32,33,130,91,36,33,240,0,1,34,130,175,33,34,96,130,7,130,95,32,34,130,47,36,35,110,0,1,35,130,63,32,36,130,215,36,36,90,0,1,36,130,35,37,37,210,0,1,38,138,130,3,36,202,0,
    1,39,144,130,3,130,43,37,40,118,0,1,40,162,130,3,35,230,0,1,41,130,75,37,41,218,0,1,42,30,130,3,130,171,33,42,142,130,7,36,186,0,1,43,20,130,3,32,84,130,3,36,152,0,1,44,28,130,3,32,
    112,130,3,130,255,41,45,134,0,1,45,198,0,1,46,88,130,3,36,132,0,1,47,22,130,3,32,66,130,3,36,134,0,1,48,18,130,3,130,255,32,48,130,255,32,48,130,87,37,48,242,0,1,49,30,130,3,32,120,
    130,3,32,184,130,3,36,228,0,1,50,16,130,3,32,80,130,3,130,139,37,50,212,0,1,51,2,130,3,32,92,130,3,36,160,0,1,52,4,130,3,35,158,0,1,53,130,159,37,53,240,0,1,54,34,130,3,32,154,130,
    3,35,200,0,1,55,130,51,33,55,126,130,7,130,207,32,56,130,151,37,56,128,0,1,56,180,130,3,36,224,0,1,57,36,130,3,130,31,33,57,172,130,7,36,238,0,1,58,28,130,3,32,72,130,3,130,203,37,
    58,208,0,1,59,20,130,3,32,64,130,3,32,156,130,3,36,220,0,1,60,78,130,3,130,123,41,61,116,0,1,61,226,0,1,62,60,130,3,130,7,32,63,130,147,37,64,108,0,1,64,178,130,3,36,252,0,1,65,70,
    130,3,130,19,37,66,106,0,1,67,66,130,3,36,112,0,1,68,100,130,3,130,19,36,69,136,0,1,69,130,127,32,69,130,211,32,70,130,211,32,70,130,111,41,70,116,0,1,70,196,0,1,71,6,130,3,32,54,130,
    3,32,98,130,3,32,168,130,3,36,232,0,1,72,24,130,3,32,68,130,3,32,138,130,3,36,202,0,1,73,110,130,3,130,59,32,74,130,155,36,74,132,0,1,74,130,115,32,75,130,199,41,75,82,0,1,75,166,0,
    1,76,22,130,3,130,115,33,76,134,130,7,130,27,37,76,246,0,1,77,34,130,3,32,126,130,3,36,192,0,1,78,4,130,3,32,48,130,3,32,140,130,3,36,206,0,1,79,18,130,3,32,62,130,3,32,154,130,3,36,
    218,0,1,80,76,130,3,130,23,32,81,130,243,37,81,224,0,1,82,92,130,3,40,242,0,1,83,150,0,1,84,56,130,3,36,128,0,1,85,20,130,3,130,39,36,85,230,0,1,86,130,11,32,86,130,103,40,87,74,0,
    1,87,120,0,1,88,130,79,32,88,130,163,36,88,176,0,1,89,130,151,32,89,130,75,32,90,130,247,32,91,130,223,32,91,130,139,41,93,0,0,1,93,46,0,1,94,96,130,3,130,131,36,95,180,0,1,95,130,
    103,57,96,58,0,1,96,122,0,1,97,50,0,1,98,130,0,1,99,214,0,1,101,32,0,1,102,102,130,3,32,148,130,3,36,194,0,1,104,42,130,3,36,88,0,1,105,146,130,3,36,192,0,1,106,28,130,3,36,94,0,1,
    107,38,130,3,130,187,32,108,130,23,36,109,100,0,1,110,130,67,36,111,172,0,1,112,130,91,40,113,186,0,1,114,202,0,1,115,130,99,40,116,234,0,1,117,248,0,1,118,130,47,32,119,130,75,32,
    119,130,75,36,120,102,0,1,120,130,95,36,121,90,0,1,121,130,163,37,122,64,0,1,122,158,130,3,130,47,32,123,130,147,36,123,124,0,1,123,130,139,44,124,24,0,1,124,114,0,1,125,100,0,1,126,
    130,115,32,127,130,255,37,128,62,0,1,129,72,130,3,43,252,0,1,130,182,0,1,131,102,0,1,132,130,219,33,132,138,130,7,130,115,37,133,34,0,1,133,74,130,3,130,15,32,133,130,15,32,134,130,
    15,40,134,244,0,1,136,40,0,1,137,130,39,40,138,26,0,1,139,30,0,1,140,131,3,35,238,0,1,141,130,35,41,142,160,0,1,143,122,0,1,144,24,130,3,36,210,0,1,145,0,130,3,130,195,36,145,230,0,
    1,146,130,27,36,146,204,0,1,147,130,127,37,147,240,0,1,148,128,130,3,35,174,0,1,149,130,179,33,149,110,130,7,36,254,0,1,150,44,130,3,36,176,0,1,151,136,130,3,130,235,36,152,98,0,1,
    152,130,71,37,153,72,0,1,153,164,130,3,36,252,0,1,154,84,130,3,130,55,32,155,130,19,41,155,242,0,1,156,156,0,1,157,52,130,3,130,91,37,158,104,0,1,159,4,130,3,36,154,0,1,160,22,130,
    3,36,144,0,1,161,14,130,3,32,142,130,3,36,224,0,1,162,48,130,3,32,124,130,3,32,226,130,3,130,95,32,163,130,67,40,163,150,0,1,163,228,0,1,164,130,27,36,164,148,0,1,165,130,207,32,165,
    130,143,33,165,182,130,11,36,254,0,1,166,70,130,3,35,168,0,1,167,130,23,33,167,106,130,7,36,214,0,1,168,66,130,3,36,184,0,1,169,68,130,3,36,230,0,1,170,96,130,3,40,234,0,1,171,112,
    0,1,172,6,130,3,130,43,33,172,54,130,7,130,147,37,173,114,0,1,174,16,130,3,130,179,33,174,198,130,7,35,222,0,1,175,130,131,41,176,46,0,1,176,212,0,1,177,146,130,3,32,170,130,3,36,194,
    0,1,178,128,130,3,44,178,0,1,179,130,0,1,180,80,0,1,181,12,130,3,35,228,0,1,182,130,71,45,183,140,0,1,184,112,0,1,185,76,0,1,186,18,130,3,59,236,0,1,187,216,0,1,188,162,0,1,189,150,
    0,1,190,136,0,1,191,102,0,1,192,88,0,1,193,130,143,32,194,130,155,41,194,122,0,1,194,224,0,1,195,70,130,3,130,99,32,195,130,99,37,195,218,0,1,196,64,130,3,35,244,0,1,197,130,19,32,
    198,130,47,40,199,20,0,1,199,238,0,1,200,130,75,37,201,86,0,1,202,10,130,3,40,180,0,1,203,104,0,1,204,56,130,3,56,226,0,1,205,186,0,1,206,148,0,1,207,108,0,1,208,68,0,1,209,36,0,1,
    210,24,130,3,35,242,0,1,211,130,159,36,212,172,0,1,213,130,131,32,214,130,131,36,215,78,0,1,216,130,71,41,217,52,0,1,217,182,0,1,218,54,130,3,130,63,33,218,210,130,7,35,234,0,1,219,
    130,199,41,219,254,0,1,220,138,0,1,221,28,130,3,32,52,130,3,32,76,130,3,39,224,0,1,222,128,0,1,223,130,19,33,223,200,130,7,130,15,36,223,248,0,1,224,130,151,41,225,78,0,1,225,250,0,
    1,226,178,130,3,32,202,130,3,48,226,0,1,227,156,0,1,228,140,0,1,229,124,0,1,230,116,130,3,130,11,36,230,164,0,1,231,130,23,32,232,130,187,40,233,40,0,1,233,252,0,1,234,130,223,37,234,
    44,0,1,235,0,130,3,32,66,130,3,130,31,33,235,180,130,7,130,183,32,235,130,71,32,236,130,23,40,236,30,0,1,236,54,0,1,237,130,51,32,238,130,215,44,239,16,0,1,240,38,0,1,241,74,0,1,242,
    130,239,48,243,94,0,1,244,120,0,1,245,150,0,1,246,206,0,1,248,130,35,40,249,56,0,1,249,232,0,1,251,130,43,32,252,130,11,32,253,130,251,35,254,170,0,2,131,1,36,1,50,0,2,2,130,3,8,41,
    3,48,0,2,4,56,0,2,5,88,0,2,6,134,0,2,7,142,0,2,8,208,0,2,10,18,0,2,11,70,0,2,12,140,0,2,13,232,0,2,15,28,130,3,32,112,130,3,32,186,130,3,36,252,0,2,16,60,130,3,32,130,130,3,36,224,
    0,2,17,10,130,3,32,76,130,3,32,146,130,3,35,240,0,2,18,130,43,33,18,72,130,7,32,138,130,3,32,182,130,3,36,248,0,2,19,36,130,3,32,102,130,3,36,216,0,2,20,20,130,3,130,91,33,20,120,130,
    7,36,192,0,2,21,40,130,3,32,90,130,3,130,107,32,21,130,31,32,22,130,79,32,22,130,95,41,22,156,0,2,22,204,0,2,23,12,130,3,130,95,32,23,130,31,33,23,190,130,11,130,99,37,24,32,0,2,24,
    80,130,3,32,154,130,3,130,35,36,24,254,0,2,25,130,203,33,25,116,130,7,32,160,130,3,36,250,0,2,26,58,130,3,32,126,130,3,130,243,37,27,4,0,2,27,68,130,3,32,136,130,3,130,47,37,28,50,
    0,2,28,118,130,3,36,210,0,2,29,22,130,3,130,11,36,29,248,0,2,30,130,211,36,30,228,0,2,31,130,19,32,31,130,127,32,32,130,139,32,32,130,215,40,33,8,0,2,33,124,0,2,34,130,7,33,34,88,130,
    7,32,132,130,3,32,176,130,3,36,222,0,2,35,10,130,3,32,54,130,3,32,98,130,3,32,144,130,3,130,163,37,35,234,0,2,36,24,130,3,130,115,33,36,114,130,7,32,138,130,3,32,162,130,3,32,186,130,
    3,130,115,32,36,130,31,37,37,2,0,2,37,26,130,3,32,96,130,3,35,214,0,2,38,130,87,33,38,106,130,7,131,3,35,230,0,2,39,130,35,41,40,192,0,2,41,82,0,2,42,38,130,3,130,235,32,43,130,151,
    37,43,98,0,2,44,24,130,3,36,78,0,2,45,62,130,3,40,250,0,2,46,102,0,2,47,20,130,3,35,244,0,2,48,130,51,37,49,56,0,2,49,148,130,3,130,87,45,50,32,0,2,50,254,0,2,51,180,0,2,52,108,130,
    3,32,164,130,3,130,131,41,53,206,0,2,54,76,0,2,55,34,130,3,36,152,0,2,56,70,130,3,43,170,0,2,57,104,0,2,58,150,0,2,59,130,23,32,59,130,11,41,60,22,0,2,60,178,0,2,61,42,130,3,36,226,
    0,2,62,106,130,3,130,7,44,63,80,0,2,63,148,0,2,64,46,0,2,65,130,111,49,65,124,0,2,66,132,0,2,67,140,0,2,69,18,0,2,70,14,130,3,40,204,0,2,71,66,0,2,72,104,130,3,35,188,0,2,73,130,19,
    37,73,220,0,2,75,20,130,3,32,120,130,3,35,206,0,2,76,130,247,40,77,30,0,2,77,168,0,2,78,130,23,33,78,138,130,7,130,159,40,79,110,0,2,80,136,0,2,81,130,123,32,82,130,23,36,82,64,0,2,
    82,130,167,37,83,112,0,2,84,108,130,3,36,196,0,2,85,106,130,3,130,55,32,86,130,31,49,86,96,0,2,86,242,0,2,87,240,0,2,88,204,0,2,90,16,130,3,35,206,0,2,91,130,119,41,92,104,0,2,93,58,
    0,2,94,158,130,3,35,228,0,2,95,130,75,32,95,130,63,53,96,4,0,2,96,200,0,2,97,156,0,2,98,128,0,2,99,76,0,2,100,84,130,3,35,236,0,2,101,130,199,32,101,130,75,44,102,174,0,2,103,134,0,
    2,104,150,0,2,105,130,83,37,105,254,0,2,106,122,130,3,35,242,0,2,107,130,123,36,108,80,0,2,109,130,127,41,109,148,0,2,110,204,0,2,111,44,130,3,35,172,0,2,112,130,115,37,113,66,0,2,
    114,26,130,3,130,91,32,114,130,103,37,115,8,0,2,115,118,130,3,36,234,0,2,116,32,130,3,32,154,130,3,36,222,0,2,117,56,130,3,32,186,130,3,52,248,0,2,118,194,0,2,119,138,0,2,120,94,0,
    2,121,38,0,2,122,46,130,3,130,59,49,123,102,0,2,123,202,0,2,124,100,0,2,125,24,0,2,126,14,130,3,130,87,41,127,106,0,2,127,224,0,2,128,70,130,3,130,87,36,129,136,0,2,130,130,119,32,
    130,130,71,32,131,130,23,49,132,68,0,2,132,200,0,2,133,120,0,2,134,64,0,2,135,10,130,3,130,51,33,135,176,130,7,36,238,0,2,136,102,130,3,35,218,0,2,137,130,219,33,137,126,130,7,32,162,
    130,3,32,196,130,3,130,231,41,138,50,0,2,138,178,0,2,139,40,130,3,130,59,33,139,88,130,7,32,110,130,3,36,132,0,2,140,42,130,3,32,188,130,3,130,191,36,140,246,0,2,141,130,143,32,141,
    130,175,33,141,62,130,11,32,86,130,3,32,108,130,3,35,184,0,2,142,130,191,33,142,80,130,7,32,198,130,3,36,232,0,2,143,92,130,3,32,124,130,3,130,55,37,144,0,0,2,144,98,130,3,130,79,37,
    144,228,0,2,145,4,130,3,130,175,32,145,130,15,37,145,236,0,2,146,84,130,3,32,182,130,3,35,220,0,2,147,130,27,33,147,90,130,7,32,150,130,3,36,202,0,2,148,18,130,3,32,70,130,3,32,104,
    130,3,130,163,36,149,8,0,2,149,130,147,32,149,130,95,33,149,140,130,11,130,155,32,149,130,67,8,32,0,166,0,0,4,132,5,118,0,3,0,7,0,26,64,11,7,1,3,4,0,3,5,5,9,4,0,47,205,18,57,47,205,
    130,6,8,37,63,205,49,48,51,17,33,17,37,33,17,33,166,3,222,252,182,2,182,253,74,5,118,250,138,148,4,78,0,2,0,180,255,238,1,146,5,154,130,65,8,96,15,0,55,64,28,3,3,2,10,176,4,19,1,2,
    0,125,31,3,47,3,2,3,13,175,127,7,1,16,7,32,7,2,7,184,255,232,179,13,6,77,7,47,43,114,93,225,212,114,225,57,57,0,63,253,206,63,49,48,1,3,35,3,19,34,38,53,52,54,51,50,22,21,20,6,1,119,
    19,135,18,88,46,65,65,46,45,66,66,5,154,251,250,4,6,250,84,64,46,131,16,33,46,64,130,115,36,148,3,219,2,143,132,115,42,7,0,43,183,2,6,3,7,3,4,125,130,94,34,253,64,14,131,95,37,7,9,
    0,125,3,8,130,9,35,3,47,43,225,130,197,38,43,225,0,63,51,205,50,133,103,32,33,130,107,38,1,67,30,115,30,1,251,130,4,37,5,154,254,65,1,191,131,3,32,0,130,87,36,33,0,123,4,142,130,87,
    8,107,27,0,31,0,178,64,99,120,23,1,119,6,1,27,2,3,6,7,26,7,24,28,31,9,8,25,8,23,29,30,10,11,22,11,20,17,16,13,12,21,12,9,13,16,31,4,3,226,6,17,20,23,29,4,2,226,64,27,6,27,6,26,11,0,
    7,16,7,2,9,7,7,33,22,26,3,4,0,0,25,26,14,7,15,8,1,12,3,8,8,33,12,12,22,66,18,14,14,21,22,11,96,12,112,12,2,12,47,93,130,152,54,51,47,198,43,17,18,1,57,24,47,95,94,93,51,77,225,50,50,
    47,198,0,63,196,130,185,33,94,93,130,5,39,57,47,47,26,237,23,57,16,130,3,37,49,48,16,135,5,192,130,0,33,16,135,143,5,39,1,93,93,1,7,33,3,33,130,3,40,35,19,35,3,35,19,33,55,33,132,3,
    8,59,51,3,51,19,51,3,7,35,3,51,4,142,23,254,251,63,1,25,27,254,237,88,126,86,250,84,125,84,254,250,20,1,9,61,254,235,21,1,21,84,126,84,252,86,123,84,145,252,66,254,4,8,106,254,212,
    106,254,115,1,141,131,3,39,106,1,44,106,1,146,254,110,131,3,130,23,8,48,0,0,3,0,162,255,47,3,209,6,80,0,30,0,37,0,42,0,169,64,24,121,38,1,118,31,1,122,29,1,117,21,1,118,14,1,121,5,
    1,42,32,12,0,77,37,184,255,224,179,130,7,37,21,184,255,208,64,77,130,8,33,5,48,130,4,8,95,31,25,9,38,10,3,19,22,22,32,24,154,16,18,19,6,0,6,16,6,32,6,3,6,6,39,9,154,0,2,3,24,9,16,19,
    25,31,39,6,1,140,2,2,28,13,22,64,12,17,0,76,22,22,28,137,127,41,1,47,41,1,41,5,35,137,32,13,1,13,47,93,225,196,212,93,93,241,192,47,43,17,18,57,47,225,23,57,0,63,205,51,237,50,50,47,
    93,134,7,130,22,38,57,18,57,57,49,48,43,130,0,32,93,132,0,8,184,37,21,35,53,34,39,53,22,22,51,17,38,38,53,52,54,55,53,51,21,22,23,21,38,39,17,22,22,21,20,6,1,17,6,6,21,20,22,19,17,
    54,53,52,2,104,104,213,133,58,201,87,220,130,198,152,104,196,75,102,169,208,153,191,254,238,84,98,80,206,193,8,217,209,86,174,51,69,2,0,106,179,122,145,211,21,180,176,6,50,170,80,6,
    253,240,99,185,118,142,193,3,25,1,210,17,115,82,86,110,254,228,254,60,42,166,138,0,5,0,80,255,234,6,64,5,176,0,11,0,23,0,27,0,39,0,51,0,160,64,41,25,24,3,40,82,34,14,46,82,64,28,19,
    6,0,66,18,82,0,12,82,6,4,63,24,79,24,2,26,24,26,24,37,3,31,189,43,12,13,6,77,43,184,255,249,182,11,130,7,39,49,189,37,184,255,224,64,20,130,19,38,37,24,12,6,77,37,9,130,21,43,37,37,
    53,3,9,189,21,184,255,236,64,22,130,24,33,21,8,130,19,36,21,15,189,3,28,130,12,33,3,28,130,37,36,3,184,255,228,179,130,20,38,3,47,43,43,43,225,212,130,3,34,17,18,57,139,11,54,57,47,
    47,93,0,63,237,212,237,43,0,24,63,26,77,237,244,237,63,47,49,48,1,67,140,10,47,3,34,6,21,20,22,51,50,54,53,52,38,37,1,35,1,67,168,11,139,27,8,34,1,148,148,176,186,158,153,169,189,139,
    95,110,108,93,94,106,105,3,87,252,104,143,3,151,38,148,176,188,156,152,170,189,139,96,109,132,22,8,97,2,203,195,163,178,205,190,172,172,207,2,119,141,123,121,135,140,124,121,135,88,
    250,92,5,164,250,80,196,162,178,207,192,173,171,207,2,118,140,124,119,135,141,123,122,132,0,0,3,0,119,255,233,6,62,5,178,0,67,0,87,0,103,0,197,64,27,120,79,1,124,76,1,120,75,1,122,
    74,1,116,55,1,117,54,1,121,13,1,74,64,11,0,77,81,184,255,232,64,10,130,8,43,58,77,1,37,31,1,16,184,255,224,64,86,130,14,8,72,5,73,78,18,4,28,10,47,56,41,36,101,47,5,68,93,150,28,4,
    68,149,10,19,61,236,0,19,58,73,1,56,5,51,15,41,73,44,83,18,78,101,36,4,33,23,51,239,44,33,239,88,96,239,23,23,88,44,3,15,112,64,1,64,64,105,83,8,13,6,77,83,8,12,131,4,32,11,130,4,47,
    131,15,47,225,43,43,43,18,57,47,93,18,23,57,47,225,131,1,32,17,130,9,35,17,18,57,57,131,3,35,93,0,63,237,131,1,130,18,32,47,131,23,38,49,48,43,93,93,43,1,130,4,45,93,93,93,0,93,93,
    5,34,46,2,39,14,3,35,130,6,8,33,53,52,54,55,46,3,53,52,62,2,51,50,30,2,21,20,6,7,30,3,23,54,18,53,52,38,39,51,22,22,23,20,14,2,130,16,42,51,50,54,55,21,6,6,37,50,62,2,130,44,130,57,
    43,21,20,30,2,1,52,46,2,35,34,6,7,130,10,8,207,23,54,54,5,165,57,87,74,69,38,40,105,128,152,89,108,180,128,71,174,159,30,60,47,29,57,100,136,79,78,129,93,52,137,137,69,112,95,82,40,
    81,82,9,8,159,8,3,2,38,64,85,47,35,58,59,63,40,30,62,33,36,79,252,147,76,128,108,88,36,61,103,105,119,78,63,106,77,44,50,87,116,1,61,37,57,70,34,98,117,1,25,45,60,34,126,124,23,31,
    57,82,52,43,80,62,37,54,105,154,100,153,217,69,22,59,75,91,54,79,122,84,43,43,80,112,69,120,159,56,27,78,96,110,59,113,1,16,149,38,67,33,33,59,41,91,184,173,156,65,48,71,48,23,16,12,
    152,14,14,142,33,55,73,40,95,135,98,70,30,25,61,83,110,74,68,104,69,35,3,253,50,70,46,21,106,91,41,72,62,48,17,44,117,0,1,0,148,3,219,1,67,5,154,0,3,0,27,64,14,1,130,6,42,125,3,4,13,
    6,77,3,3,5,4,17,69,195,6,32,205,69,193,5,33,1,67,69,184,8,130,55,36,140,254,186,2,86,130,55,42,9,0,65,64,43,5,16,11,1,77,1,131,4,54,56,2,1,6,3,0,32,5,234,6,1,234,0,0,11,8,8,12,6,77,
    8,16,11,130,4,33,234,3,131,6,37,3,47,43,225,43,43,130,88,37,225,214,225,0,63,63,130,89,41,93,43,43,1,35,0,17,16,1,51,130,4,58,2,84,146,254,202,1,54,148,254,198,254,186,1,98,2,7,2,9,
    1,110,254,132,254,7,254,11,0,130,115,32,22,130,20,32,226,132,115,38,74,185,0,6,255,240,179,130,117,37,0,184,255,240,64,11,130,8,33,55,9,132,121,32,3,130,15,32,181,130,111,39,3,234,
    8,184,255,248,64,13,130,10,42,8,8,11,1,6,234,5,0,234,1,47,130,117,133,213,32,43,136,124,32,19,136,124,38,170,146,1,56,254,198,148,130,4,46,186,1,118,1,245,1,249,1,124,254,146,253,247,
    253,249,131,123,36,76,2,217,3,12,130,123,8,47,14,0,92,64,43,3,4,1,3,5,10,13,14,0,4,8,83,64,15,9,1,16,3,9,12,3,2,2,0,14,12,6,6,11,8,11,8,66,8,4,10,13,3,1,13,12,72,11,130,132,48,181,
    13,6,77,7,10,11,47,51,51,43,237,50,50,23,57,196,70,117,6,8,139,16,77,228,57,47,0,63,212,95,94,93,26,253,23,57,212,196,49,48,93,1,5,23,7,3,3,39,55,37,55,5,3,51,3,37,3,12,254,238,190,
    110,156,158,110,190,254,236,45,1,6,23,138,23,1,7,4,80,56,243,76,1,9,254,247,76,243,56,125,99,1,48,254,208,99,0,1,0,232,0,127,4,148,4,43,0,11,0,64,64,28,3,14,4,8,14,10,1,4,190,64,15,
    7,1,13,3,7,0,14,1,4,5,66,5,10,7,1,189,4,184,255,224,179,12,6,77,4,47,43,225,57,57,205,43,1,16,77,225,0,24,47,131,131,8,33,237,57,57,237,16,237,49,48,1,33,17,35,17,33,53,33,17,51,17,
    33,4,148,254,108,133,254,109,1,147,133,1,148,2,18,131,8,37,134,1,147,254,109,0,130,113,57,39,254,248,1,60,0,228,0,3,0,69,64,29,57,3,1,57,0,1,2,64,3,178,4,0,32,130,94,47,0,40,11,6,77,
    0,128,2,64,17,25,72,2,184,255,234,131,114,131,7,32,182,130,22,49,2,47,5,1,93,47,43,43,43,26,205,43,43,0,16,246,26,205,130,115,47,93,93,37,3,35,19,1,60,160,117,117,228,254,20,1,236,
    131,97,38,144,1,250,2,178,2,123,130,97,41,26,64,14,1,83,79,2,1,2,32,130,120,42,32,1,1,1,47,93,47,93,0,47,93,132,173,42,53,33,2,178,253,222,2,34,1,250,129,131,53,50,112,255,234,1,80,
    0,203,0,11,0,26,64,15,6,176,0,19,9,175,73,18,5,33,47,13,130,118,35,113,225,0,63,130,53,32,23,69,105,10,48,223,46,65,65,46,47,66,66,22,66,46,46,67,67,46,46,66,130,69,39,255,228,255,
    18,3,45,5,154,130,123,43,25,64,13,119,1,1,0,16,11,0,77,1,130,14,37,2,47,47,0,63,47,130,177,8,52,43,93,1,1,35,1,3,45,253,80,153,2,174,5,154,249,120,6,136,0,2,0,86,255,231,3,252,5,178,
    0,16,0,24,0,32,64,16,0,137,17,17,26,21,137,10,19,154,15,7,23,154,5,25,130,124,39,63,237,1,47,237,18,57,47,130,133,53,1,20,2,6,6,35,34,38,38,2,53,52,18,54,54,51,32,3,16,33,32,17,130,
    3,8,49,3,252,65,123,179,114,108,169,116,60,63,122,181,117,1,195,168,254,221,254,205,1,45,1,41,2,211,180,254,234,191,99,95,182,1,9,171,188,1,32,195,99,253,17,2,100,253,145,253,187,130,
    179,61,0,228,0,0,2,212,5,186,0,14,0,29,64,13,13,7,0,138,1,8,7,7,1,13,7,1,24,0,63,63,130,113,32,51,130,120,8,41,198,51,49,48,33,35,17,14,3,7,53,62,3,55,51,2,212,164,19,73,91,101,48,
    54,121,117,104,38,62,4,215,19,49,48,42,12,166,15,51,63,69,33,131,81,36,109,0,0,3,226,130,205,40,42,0,46,64,23,30,19,138,10,130,21,47,44,29,138,33,0,14,29,153,32,24,10,10,5,154,14,7,
    130,210,37,50,47,63,237,17,57,133,214,37,18,57,237,50,49,48,69,35,5,35,14,2,7,53,130,214,36,50,30,2,21,20,130,11,8,96,14,3,21,33,21,33,53,52,62,2,55,62,3,3,4,41,71,96,56,48,92,87,81,
    35,69,171,123,88,150,110,63,39,79,120,81,101,131,77,31,2,202,252,139,40,92,149,108,78,105,64,27,4,34,66,98,65,32,26,46,64,39,176,67,71,51,98,144,93,85,138,122,112,58,72,102,89,89,58,
    147,71,92,140,126,126,79,56,102,102,105,0,1,0,123,255,231,3,185,130,169,8,51,41,0,85,64,50,22,137,26,19,36,3,8,34,34,8,0,137,15,15,43,8,57,26,1,47,26,1,26,26,24,154,29,36,18,154,19,
    19,29,7,54,8,1,0,8,16,8,32,8,3,8,8,10,65,151,5,42,50,47,93,93,63,57,47,237,57,16,237,131,10,34,1,47,18,130,12,38,17,57,47,18,23,57,237,130,208,130,192,51,35,34,39,53,22,51,50,62,2,
    53,16,33,35,53,51,32,17,52,33,34,130,224,32,51,131,223,8,84,16,5,21,30,3,3,185,73,133,186,112,205,121,145,187,75,121,87,47,254,112,119,113,1,98,254,242,152,133,137,189,90,146,104,57,
    254,226,73,124,90,51,1,150,97,159,113,62,78,176,114,38,72,102,64,1,25,139,1,8,244,102,159,82,50,90,126,76,254,229,81,4,8,55,88,119,0,2,0,14,0,0,4,10,5,154,130,7,58,21,0,54,64,26,3,
    1,5,138,6,16,14,6,6,15,23,8,4,8,154,9,1,17,14,15,15,6,130,15,65,216,6,49,18,57,51,51,237,50,1,47,17,57,57,17,51,51,16,253,50,198,130,176,8,68,17,51,21,35,17,35,17,33,53,62,3,55,1,33,
    17,14,3,3,82,184,184,162,253,94,94,192,175,149,51,254,36,1,233,75,134,121,110,5,154,252,77,150,254,175,1,81,142,106,241,248,247,113,252,77,2,190,131,206,168,136,0,0,1,0,164,255,231,
    3,201,130,131,63,35,0,61,64,33,28,138,26,23,23,8,0,137,15,15,37,8,18,154,31,31,10,27,153,24,6,50,8,1,32,8,1,65,64,13,36,237,18,57,47,237,65,59,8,65,57,17,43,52,38,35,34,14,2,7,19,33,
    21,33,3,66,26,5,8,66,3,201,72,133,188,115,200,97,146,153,78,125,88,47,188,179,29,64,64,61,25,49,2,151,253,247,29,32,72,28,114,180,125,65,1,182,105,170,122,66,60,174,94,46,82,116,69,
    140,159,2,3,4,3,2,207,148,254,91,2,3,60,111,161,0,2,0,111,130,169,8,35,254,5,178,0,32,0,52,0,55,64,28,18,10,0,137,33,33,54,43,25,137,10,38,154,26,28,28,48,15,18,18,20,154,15,7,48,65,
    231,5,36,63,237,50,47,17,130,165,32,57,130,166,33,237,50,65,228,5,65,223,7,32,46,67,136,6,34,50,23,21,132,167,42,21,51,54,51,50,30,2,7,52,46,2,131,182,8,134,21,20,30,2,51,50,62,2,3,
    254,68,120,166,97,108,171,119,62,83,153,216,133,148,90,111,123,96,154,110,59,4,99,228,94,151,107,58,168,38,71,104,66,61,104,76,43,42,76,105,64,62,102,72,40,1,199,105,176,128,71,90,
    169,244,153,187,1,49,217,118,41,155,57,86,157,222,136,179,64,117,162,120,75,120,84,45,46,79,105,59,74,129,96,55,47,84,117,0,1,0,98,0,0,3,242,5,154,0,18,0,32,64,15,7,138,15,8,8,0,16,
    0,0,16,153,17,6,8,24,0,63,63,237,57,1,47,47,132,188,130,178,8,86,14,3,2,2,7,35,54,18,18,62,2,55,33,53,33,3,242,33,93,104,108,94,71,16,176,18,75,95,106,100,83,25,253,49,3,144,5,59,57,
    170,212,244,254,252,254,245,129,128,1,11,1,1,238,200,153,43,148,0,3,0,90,255,233,3,252,5,178,0,35,0,55,0,75,0,71,64,37,56,137,8,66,138,18,5,21,18,8,130,1,8,33,0,26,137,46,46,77,36,
    137,0,21,5,51,154,61,61,13,41,153,31,25,71,154,13,7,0,63,237,63,237,17,57,47,237,57,67,237,6,130,12,32,57,131,144,40,57,16,237,16,237,49,48,19,52,130,138,33,38,38,73,85,13,40,21,20,
    14,2,35,34,46,2,55,65,61,6,32,53,65,77,6,32,19,142,15,8,124,90,36,67,99,63,93,110,61,108,148,86,87,148,108,60,110,91,62,98,67,36,68,123,171,103,102,170,124,69,178,41,75,107,65,63,106,
    77,43,41,76,106,66,62,105,77,44,51,37,65,86,49,49,87,64,38,37,64,87,50,53,87,63,34,1,144,57,115,100,82,24,52,163,99,79,134,98,55,56,98,134,78,99,163,52,24,82,100,115,57,94,156,111,
    62,62,111,156,108,67,108,76,41,42,76,108,66,60,105,80,46,43,77,107,2,89,51,87,65,37,38,65,87,50,52,88,64,130,7,41,88,0,0,2,0,94,255,231,3,236,66,75,6,55,53,64,27,33,0,137,8,23,16,16,
    54,43,137,23,48,153,15,18,18,5,38,154,28,7,66,241,10,37,63,237,17,57,47,57,69,209,6,37,18,57,237,51,49,48,69,212,6,68,42,7,33,35,6,74,109,5,74,103,5,66,73,16,8,35,236,78,150,216,138,
    142,108,119,135,97,153,106,56,4,92,228,92,154,112,62,69,123,169,100,108,167,114,60,177,43,76,105,62,59,102,75,130,243,8,47,107,65,57,101,75,43,3,35,197,254,205,213,111,50,157,67,80,
    153,224,145,185,65,119,164,99,106,176,127,70,87,167,244,10,83,136,97,53,47,85,117,69,76,120,83,44,43,73,99,130,201,8,39,112,255,234,1,80,4,22,0,11,0,23,0,54,64,18,0,176,6,16,18,176,
    12,19,9,3,21,175,31,15,47,15,2,15,184,255,236,179,12,6,77,131,7,33,182,11,130,7,49,47,25,1,93,47,43,43,113,225,57,57,0,63,237,63,237,49,48,76,115,13,80,40,9,71,49,7,32,47,71,57,6,43,
    3,56,66,46,46,64,64,46,46,66,252,178,71,68,8,38,2,0,39,254,248,1,82,132,129,42,15,0,93,64,18,57,15,1,57,12,1,131,135,39,14,64,15,178,16,9,175,3,134,129,131,7,33,64,17,130,130,34,3,
    12,32,130,144,33,12,40,130,10,34,12,128,14,72,54,6,131,7,131,159,34,14,47,17,132,159,35,26,205,43,43,130,166,37,225,0,16,246,26,205,131,165,34,1,93,93,139,168,47,19,3,35,19,226,47,
    65,65,47,46,66,66,44,160,117,117,137,156,33,253,172,72,89,6,8,40,1,16,0,127,4,108,4,76,0,7,0,67,64,12,120,1,1,7,16,11,0,77,2,1,0,0,184,255,192,64,11,12,0,77,112,3,1,3,3,0,4,130,15,
    33,224,181,130,146,34,0,5,1,73,55,6,44,1,47,43,51,47,43,196,0,47,51,47,93,43,78,231,5,37,93,37,1,53,1,21,130,1,8,34,4,108,252,164,3,92,253,126,2,130,127,1,176,59,1,226,150,254,156,
    4,254,198,0,0,2,0,232,1,62,4,148,3,108,0,3,130,113,39,43,64,10,5,190,6,1,190,130,110,33,5,4,130,85,32,180,130,95,33,4,5,134,94,33,5,47,130,219,130,86,35,0,47,237,214,72,220,6,41,17,
    33,53,33,4,148,252,84,3,172,131,3,40,2,232,132,253,210,132,0,0,1,142,195,34,0,1,2,131,195,32,7,130,24,136,195,36,6,1,6,6,1,138,195,32,2,134,100,32,2,130,192,130,198,140,195,32,1,130,
    195,32,53,130,3,131,195,50,2,132,253,124,3,92,2,47,254,80,149,1,56,6,1,100,150,254,30,130,195,8,60,131,255,238,3,49,5,178,0,35,0,51,0,54,64,30,118,18,1,17,15,149,20,4,0,44,176,36,19,
    32,239,3,48,175,127,40,1,40,40,17,25,132,10,10,53,17,47,18,57,47,225,17,57,47,93,225,212,225,0,63,253,198,130,2,41,49,48,93,1,38,38,53,52,62,4,68,0,5,71,57,7,41,20,14,4,21,20,22,23,
    3,34,39,130,28,8,45,55,54,51,50,23,22,21,20,7,6,1,79,9,14,51,76,90,76,51,37,64,84,47,169,124,153,164,76,134,101,58,52,79,92,79,52,22,11,71,44,34,33,33,33,45,45,130,4,8,48,34,1,142,
    26,83,41,64,103,90,83,85,93,56,48,74,50,25,133,176,96,40,79,118,78,74,116,97,84,83,90,54,46,75,26,254,96,32,32,46,47,31,33,33,31,47,46,32,32,0,130,201,8,47,172,255,68,6,252,5,174,0,
    50,0,61,0,152,64,13,122,39,1,122,50,1,118,27,1,121,25,1,30,184,255,192,64,15,11,12,0,76,28,64,11,0,77,24,64,12,0,77,22,131,19,32,61,130,8,33,4,24,130,18,44,13,0,3,51,76,9,15,49,20,
    57,76,3,15,130,1,58,37,26,76,43,4,64,34,80,34,2,34,32,76,37,0,13,15,16,18,5,60,60,46,54,72,6,34,130,1,48,40,23,72,46,29,72,40,47,225,47,225,18,57,57,47,47,16,130,6,41,47,23,51,0,47,
    253,198,93,63,237,133,18,36,237,57,57,16,212,131,11,81,51,5,8,36,43,1,93,93,93,0,93,1,35,6,35,34,38,53,52,18,51,50,22,23,51,54,55,51,2,21,20,51,50,54,53,16,0,33,32,0,17,130,5,35,50,
    55,21,6,134,10,130,16,35,20,2,35,34,80,22,10,8,161,4,140,5,73,214,137,167,231,188,72,113,16,4,2,8,125,47,119,108,143,254,164,254,204,254,215,254,124,1,113,1,57,247,175,173,254,253,
    254,148,254,67,1,211,1,107,1,84,1,190,235,169,219,180,124,153,103,86,124,145,1,164,238,202,171,226,1,43,74,56,28,87,253,217,10,207,234,186,1,19,1,92,254,96,254,200,254,205,254,135,
    82,124,74,1,183,1,99,1,110,1,226,254,108,254,187,238,254,207,3,14,236,175,118,136,247,204,214,0,2,0,22,0,0,5,18,5,154,0,7,0,15,0,187,64,39,119,7,1,120,6,1,9,16,12,0,77,2,8,9,1,9,15,
    3,4,14,4,115,4,1,124,1,1,1,131,20,42,4,24,11,13,1,76,1,184,255,232,180,131,8,35,12,184,255,192,132,8,32,11,130,8,33,64,53,131,18,8,48,14,13,9,10,4,5,11,12,8,13,6,77,12,64,18,22,72,
    12,6,8,15,2,3,145,15,15,6,3,1,5,8,15,12,11,3,2,14,9,13,10,10,5,119,5,1,120,0,1,5,130,75,39,64,14,11,12,0,76,0,24,132,5,60,0,17,5,47,17,51,47,43,43,93,93,18,23,57,0,47,47,63,57,47,253,
    196,16,196,16,221,43,43,196,130,17,37,49,48,43,43,1,43,131,30,35,16,135,192,192,131,3,32,1,130,42,8,58,33,35,3,33,3,35,1,51,19,3,38,39,35,6,7,3,5,18,186,152,253,160,143,187,2,38,174,
    159,225,11,11,4,10,13,223,1,146,254,110,5,154,252,143,2,99,30,66,61,35,253,157,0,3,0,188,0,0,4,47,130,253,46,15,0,23,0,31,0,199,64,10,121,13,1,118,11,1,130,240,32,4,131,150,53,37,11,
    0,77,8,24,145,17,17,1,25,145,0,18,16,145,1,3,9,25,12,6,131,216,33,6,12,130,221,33,6,11,130,4,37,125,29,184,255,240,179,130,19,132,7,130,22,131,7,33,64,47,130,26,34,29,29,5,131,38,38,
    5,125,127,21,1,21,8,130,25,82,117,5,36,21,33,17,25,8,130,47,32,25,131,18,33,25,8,130,40,37,25,126,0,184,255,248,131,67,130,7,33,246,179,130,41,130,7,34,248,64,9,130,26,34,0,33,64,130,
    131,33,43,47,130,239,32,233,130,3,34,50,18,57,130,10,34,93,233,43,131,6,32,43,131,17,32,17,69,249,5,55,17,57,47,237,57,49,48,1,43,0,93,93,93,51,17,33,50,22,21,20,6,7,21,22,130,6,41,
    4,35,3,17,51,50,54,53,52,33,135,7,8,89,188,1,152,186,218,132,116,145,174,254,248,201,250,172,138,158,254,237,193,228,148,163,254,166,5,154,182,146,122,180,38,4,17,185,148,184,228,5,
    2,254,49,133,121,209,253,154,253,252,140,122,254,0,1,0,94,255,232,4,140,5,178,0,21,0,127,64,13,117,10,1,121,0,1,122,3,1,63,23,1,9,184,255,192,64,70,12,0,77,3,24,130,171,35,10,11,0,
    32,130,239,53,21,19,145,2,19,48,11,1,11,11,13,145,8,4,0,11,48,11,13,1,76,63,131,15,34,23,16,6,130,253,33,16,6,130,230,35,16,125,5,16,130,11,33,5,8,130,11,33,5,19,130,238,32,5,134,232,
    130,230,40,93,43,51,0,63,237,50,47,93,132,4,41,51,17,51,49,48,43,43,1,93,93,130,224,34,37,6,35,67,80,6,8,67,23,21,38,35,34,0,17,16,0,51,50,55,4,140,159,237,254,206,254,144,1,158,1,
    62,204,134,154,186,247,254,205,1,31,233,216,158,60,84,1,138,1,64,1,88,1,168,59,179,86,254,182,254,236,254,250,254,201,96,0,0,2,0,188,0,0,5,62,5,66,251,5,37,141,64,32,118,3,1,130,2,
    41,9,145,0,18,8,145,1,3,4,8,130,153,32,4,131,153,33,4,8,130,153,38,4,125,13,184,255,244,179,130,168,130,7,33,240,179,130,27,130,7,32,246,134,15,34,234,64,24,130,34,35,13,13,17,9,131,
    218,32,9,131,218,33,9,6,130,17,37,9,126,0,184,255,246,65,216,6,32,254,65,216,6,33,254,179,130,25,32,0,134,232,32,43,130,233,32,43,130,0,32,233,130,3,71,194,6,32,1,130,226,35,51,17,
    33,32,130,213,36,33,3,17,51,32,130,222,8,37,33,188,1,140,2,246,254,91,254,159,212,214,1,26,1,58,253,182,5,154,253,69,254,180,254,109,5,2,251,150,1,46,1,21,2,39,0,1,131,207,33,3,180,
    130,207,55,11,0,104,64,46,9,145,64,6,2,1,6,145,43,48,6,2,10,145,1,18,5,145,2,130,211,38,4,8,0,0,13,6,10,131,166,32,10,131,166,32,10,131,166,34,10,126,1,66,195,6,130,7,32,250,131,166,
    132,7,130,166,32,1,66,120,11,35,51,51,47,47,132,163,47,17,57,43,0,24,47,26,77,237,49,48,33,33,17,33,21,134,3,53,3,180,253,8,2,216,253,208,2,6,253,250,2,80,5,154,152,254,35,151,254,
    10,134,151,32,148,130,151,57,9,0,93,64,38,5,145,47,2,1,2,2,8,7,18,1,145,8,3,4,4,0,0,11,2,6,131,143,32,6,131,143,32,6,131,143,41,6,126,7,184,255,251,179,13,6,77,130,7,32,246,131,143,
    131,7,32,182,130,143,36,7,47,11,1,93,140,147,132,145,33,18,57,80,88,6,132,140,36,35,17,33,3,148,133,134,41,168,2,216,5,2,254,16,151,253,133,130,126,66,195,6,44,236,5,178,0,25,0,150,
    182,118,10,1,121,4,66,188,5,51,49,12,0,77,23,145,24,24,8,0,21,19,145,2,19,31,11,47,11,2,66,184,5,39,23,23,21,16,11,11,0,4,130,139,37,0,4,12,6,77,0,131,159,37,0,126,21,184,255,238,131,
    151,131,7,33,64,34,130,152,34,21,21,27,66,214,13,130,43,66,219,5,32,16,66,214,13,37,43,43,233,43,43,43,76,207,5,66,223,8,42,50,17,57,47,237,49,48,43,93,93,37,70,32,7,76,207,5,66,218,
    6,8,47,17,33,53,33,4,236,216,254,248,254,205,254,133,1,165,1,67,234,159,174,238,241,254,204,1,30,245,168,123,254,198,1,226,98,122,1,140,1,70,1,77,1,171,76,186,110,254,180,130,40,38,
    240,254,201,67,1,146,152,65,125,5,33,4,242,130,254,57,11,0,122,64,71,3,145,64,8,6,5,8,145,43,48,1,18,5,18,10,3,6,3,9,0,6,131,220,32,6,137,220,36,127,1,1,1,16,130,19,33,1,18,130,19,
    36,1,1,13,8,4,131,33,32,4,131,33,32,4,131,254,34,4,126,5,68,174,6,130,7,33,244,179,130,36,33,5,47,134,214,32,18,68,158,6,36,43,43,51,0,63,130,0,66,39,9,38,35,17,33,17,35,17,51,130,
    5,52,51,4,242,168,253,26,168,168,2,230,168,2,142,253,114,5,154,253,139,2,117,130,167,36,48,0,0,1,240,132,167,55,99,64,64,2,10,146,11,3,3,7,146,64,6,1,4,14,3,7,8,66,10,7,64,3,131,134,
    32,3,131,134,32,3,131,134,36,3,126,8,8,4,130,166,33,8,2,130,129,50,8,2,11,6,77,8,64,13,1,77,12,13,8,126,255,48,43,1,43,130,0,34,16,77,225,130,5,37,26,24,205,50,43,1,130,11,45,50,0,
    24,47,26,237,50,63,237,50,49,48,1,21,130,141,56,21,33,53,51,17,35,53,1,240,140,140,254,64,140,140,5,154,144,251,134,144,144,4,122,144,130,138,37,0,20,255,232,2,43,130,143,53,12,0,99,
    64,18,118,9,1,118,1,1,121,9,1,6,6,8,145,3,19,11,3,130,250,34,240,64,28,130,126,35,5,5,0,8,130,6,33,0,8,130,133,33,0,8,130,133,35,0,126,10,24,130,16,36,10,184,255,246,179,130,19,130,
    7,33,250,182,130,22,37,10,10,14,13,17,18,69,183,8,40,50,47,43,0,63,63,237,50,47,130,138,38,93,0,93,93,1,20,2,80,172,6,56,17,17,51,2,43,216,181,84,54,54,86,227,168,2,2,255,254,229,24,
    166,39,1,129,3,154,65,203,6,8,36,162,5,154,0,16,0,156,64,28,125,14,1,41,14,57,14,2,126,1,1,42,1,58,1,2,16,24,12,13,1,76,16,16,11,1,77,14,71,160,7,32,1,130,8,54,64,24,11,13,1,76,10,
    14,16,1,5,5,8,7,18,8,3,14,15,15,1,16,112,130,78,131,28,32,25,130,171,37,0,0,18,10,5,6,130,8,33,5,6,130,177,33,5,6,130,174,35,5,126,7,184,70,215,5,130,7,34,248,64,9,130,23,32,7,131,
    221,32,7,67,125,8,41,17,51,47,43,93,57,51,51,47,51,130,200,34,18,23,57,130,200,32,43,130,0,32,93,130,0,8,51,33,35,1,38,39,35,17,35,17,51,17,51,54,55,1,51,1,4,162,234,253,214,31,7,4,
    168,168,4,14,24,2,24,209,253,153,2,144,37,13,253,62,5,154,253,94,22,27,2,113,253,80,0,133,221,33,3,164,130,221,42,5,0,131,182,4,145,1,18,2,3,5,131,131,34,16,6,77,132,7,131,159,131,
    15,130,138,132,15,131,165,130,193,33,179,10,132,31,34,224,179,9,132,7,40,192,64,24,10,31,72,5,5,7,66,181,16,32,1,66,181,6,130,7,32,242,132,208,33,1,5,68,226,11,65,155,5,130,193,32,
    43,130,207,68,218,5,45,51,17,33,3,164,253,24,168,2,64,5,154,250,254,133,161,33,6,114,130,161,57,27,0,186,64,21,122,26,1,117,19,1,38,19,54,19,2,114,22,1,22,24,11,13,1,76,13,65,120,7,
    130,161,34,232,64,55,131,18,44,22,26,1,5,9,13,6,18,17,18,18,3,4,66,43,16,33,26,8,130,214,37,26,127,1,1,1,24,130,8,34,1,1,6,130,221,130,160,33,250,181,131,154,33,29,19,130,178,33,64,
    23,130,24,33,19,13,68,94,10,32,6,130,27,38,16,126,17,184,255,242,179,130,26,130,7,33,246,179,130,53,36,17,47,43,43,225,130,190,44,57,50,43,18,57,43,43,47,43,93,51,43,233,131,15,65,
    158,5,130,9,8,88,93,49,48,1,93,93,93,33,35,17,52,55,35,6,7,1,35,1,38,39,35,22,21,17,35,17,51,1,22,23,51,54,55,1,51,6,114,167,14,4,24,19,254,22,82,254,23,21,22,4,8,162,222,1,184,51,
    15,6,43,26,1,193,210,3,194,114,165,97,42,251,178,4,70,48,99,86,195,252,64,5,154,252,24,115,57,118,58,3,228,65,25,5,32,5,130,17,8,33,0,19,0,173,64,13,119,12,1,114,11,1,120,2,1,125,1,
    1,11,184,255,240,64,10,11,0,77,15,8,11,13,1,76,5,131,209,32,47,131,9,42,14,18,1,5,4,9,10,3,9,1,15,68,130,17,35,17,1,17,24,131,219,33,17,8,131,217,131,51,32,25,130,244,37,17,21,11,5,
    8,6,130,23,33,8,6,130,22,33,8,6,130,18,34,8,126,9,65,7,6,130,7,66,167,5,71,122,5,66,167,7,33,57,50,65,13,6,49,233,43,43,43,57,51,0,47,63,18,23,57,43,43,49,48,1,43,66,170,9,65,4,9,8,
    66,38,53,17,51,5,64,206,253,30,28,18,6,8,168,218,2,206,45,13,4,10,168,4,119,43,47,46,151,251,244,5,154,251,155,70,26,62,149,3,242,0,0,2,0,94,255,232,5,170,5,178,0,11,0,23,0,123,64,
    26,18,145,0,19,12,145,6,4,72,4,15,34,125,21,184,67,73,5,130,7,32,234,70,53,6,34,234,64,47,130,188,40,32,21,48,21,2,21,21,25,15,131,211,32,15,131,211,32,15,131,211,35,15,125,3,16,130,
    228,33,3,16,130,228,33,3,16,130,39,32,3,79,232,6,32,43,131,195,35,18,57,47,93,72,49,13,32,5,73,13,5,132,5,32,1,73,14,6,8,35,0,17,16,0,2,254,254,207,254,145,1,118,1,66,1,41,1,107,254,
    140,254,212,226,254,230,1,19,221,236,1,16,254,248,24,1,146,130,23,48,90,1,156,254,112,254,189,254,161,254,104,5,50,254,186,254,247,130,1,40,189,1,52,1,21,1,28,1,54,131,227,8,33,188,
    0,0,4,41,5,154,0,10,0,18,0,133,64,35,119,6,1,53,15,1,0,145,12,12,3,2,18,11,145,3,3,7,8,130,169,33,7,8,68,30,8,34,125,16,184,96,188,5,131,7,33,64,30,130,23,32,16,131,193,36,16,16,20,
    12,1,131,229,66,224,5,131,229,34,1,126,2,67,144,6,130,7,34,244,64,9,130,42,33,2,4,130,236,32,2,74,236,13,132,243,35,0,63,237,63,130,247,39,237,49,48,93,93,1,17,35,74,223,5,32,0,74,
    216,6,8,36,16,33,1,100,168,1,138,230,253,254,231,239,189,176,174,183,254,176,2,30,253,226,5,154,224,204,204,254,252,2,228,253,180,159,145,1,28,131,203,8,33,94,254,124,5,204,5,178,0,
    40,0,56,0,150,64,41,120,18,1,20,0,41,145,11,4,49,145,0,25,238,36,30,30,117,20,130,16,32,7,65,140,16,32,53,65,190,6,130,7,33,234,179,130,165,131,7,65,190,5,39,53,48,53,2,53,53,58,45,
    131,216,33,45,6,130,26,33,45,6,130,192,40,45,125,7,16,13,6,77,7,16,130,16,32,7,131,252,34,7,47,58,72,181,5,32,225,93,9,6,130,6,32,241,130,3,49,17,57,57,93,194,47,0,47,237,47,237,63,
    237,18,57,49,48,1,92,238,5,43,38,17,16,55,54,33,32,23,22,17,20,14,92,211,5,8,131,62,2,55,21,14,3,35,34,46,2,3,34,7,6,17,16,23,22,51,50,55,54,17,16,39,38,2,244,64,132,124,113,46,183,
    186,188,1,70,1,36,182,182,61,124,188,128,52,91,91,98,59,16,38,39,36,15,16,40,41,40,17,96,153,133,124,46,226,141,141,137,138,221,236,136,136,132,132,24,25,51,75,50,200,1,67,1,91,205,
    206,200,200,254,189,136,241,194,135,29,56,86,59,30,3,7,9,6,162,5,7,5,3,57,99,132,5,126,163,163,254,247,254,248,163,161,154,154,1,21,1,29,154,155,66,13,7,8,35,192,5,154,0,28,0,39,0,
    168,64,48,117,15,1,118,14,1,57,2,1,23,30,145,8,8,10,29,145,11,3,1,10,23,9,17,8,130,254,67,185,5,45,8,11,6,77,36,125,17,121,2,1,122,1,1,2,70,149,12,44,38,11,13,1,76,1,28,2,17,4,112,
    0,1,69,23,5,33,41,30,67,59,15,33,126,10,67,221,6,130,7,32,250,71,78,7,32,179,130,84,34,10,47,43,68,238,5,43,50,16,198,43,93,23,50,43,43,93,93,47,65,95,5,35,0,47,51,63,84,242,5,48,49,
    48,93,1,93,93,33,35,3,46,3,35,35,17,35,17,33,88,215,7,8,84,21,30,3,23,1,17,51,50,62,2,53,52,38,35,4,192,200,240,33,62,65,75,47,138,168,1,172,94,159,117,66,45,83,117,73,36,53,48,47,
    29,253,177,228,63,107,78,44,149,141,1,146,56,79,50,23,253,158,5,154,47,96,143,96,75,125,98,69,19,4,16,41,56,71,47,3,83,253,248,38,71,103,64,115,129,0,130,197,8,37,121,255,232,3,222,
    5,178,0,53,0,188,64,21,123,53,1,122,46,1,118,37,1,116,26,1,118,21,1,52,24,12,0,77,42,184,255,232,179,130,7,35,40,184,255,240,131,7,32,25,130,15,33,64,75,130,16,32,17,131,29,8,36,9,
    44,19,34,4,24,49,26,0,1,48,27,1,27,29,145,24,4,32,1,1,1,6,145,49,19,27,27,44,34,8,13,6,77,34,8,12,131,4,32,11,130,4,36,125,19,19,1,44,131,19,32,44,131,19,33,44,8,130,19,34,44,125,9,
    130,87,33,64,15,130,10,38,32,9,1,9,9,55,1,131,21,39,1,47,43,18,57,47,93,43,65,29,5,65,36,6,44,47,0,63,237,196,93,63,237,205,93,17,51,50,95,120,6,97,255,8,43,55,53,30,3,51,50,54,53,
    52,46,2,39,95,113,7,32,23,87,95,6,37,20,30,2,23,30,3,86,33,7,8,118,121,34,95,105,106,45,155,153,51,90,123,71,75,130,96,55,86,140,179,93,212,97,127,199,55,110,86,54,41,80,115,75,77,
    138,104,61,83,141,184,102,34,100,104,93,58,198,30,48,33,18,115,108,58,86,71,65,36,38,78,94,119,80,98,145,94,46,51,189,88,23,52,82,59,55,80,66,62,37,38,84,102,124,80,106,147,92,41,11,
    21,31,0,1,0,41,0,0,4,12,5,154,0,7,0,123,64,9,3,18,1,5,145,64,6,3,5,184,255,250,179,130,224,32,0,130,7,33,64,40,131,8,52,5,0,14,2,3,5,66,207,5,1,5,5,8,2,8,13,6,77,2,8,12,131,4,130,28,
    35,2,126,3,4,130,16,33,3,184,94,89,6,50,3,3,9,8,9,64,12,0,77,9,184,255,192,178,11,0,77,43,43,73,148,5,69,89,8,47,1,16,77,226,24,47,46,43,43,0,63,26,237,50,63,49,93,92,8,48,4,12,254,
    98,168,254,99,3,227,5,2,250,254,5,2,152,0,130,161,36,170,255,232,4,213,130,161,54,13,0,115,64,45,37,10,53,10,2,37,8,53,8,2,9,145,2,19,12,3,5,3,70,215,17,35,11,1,11,12,130,143,33,11,
    11,131,189,32,23,130,165,35,11,15,7,6,130,16,33,7,6,130,11,44,7,6,11,6,77,7,126,4,184,255,254,64,9,130,20,33,4,4,130,20,32,4,75,91,6,37,18,57,43,47,43,93,78,39,5,33,63,63,69,9,5,38,
    16,33,32,17,17,51,17,133,6,59,4,213,253,223,253,246,168,1,116,1,103,168,2,68,253,164,2,69,3,109,252,158,254,71,1,170,3,113,131,171,36,18,0,0,4,230,130,171,45,11,0,200,64,10,123,11,
    1,116,4,1,120,2,1,130,136,36,208,179,12,1,77,130,7,34,224,179,13,133,7,38,64,34,11,1,77,4,48,130,21,33,4,32,130,18,33,4,32,130,14,52,57,11,1,54,4,1,116,7,1,7,40,11,13,1,76,7,184,255,
    240,64,25,69,250,12,39,11,7,2,3,3,2,11,8,130,20,130,76,33,252,179,130,187,131,7,33,64,17,130,212,38,11,40,0,1,0,8,12,130,56,34,0,0,13,130,223,34,248,64,15,136,223,32,4,130,31,38,4,
    3,184,255,248,183,12,130,31,44,39,3,1,3,47,93,43,51,43,43,43,17,51,130,240,32,50,130,8,37,0,47,63,18,57,57,130,8,32,43,72,193,5,131,9,40,43,43,93,93,93,1,1,35,1,72,186,7,63,4,230,253,
    237,185,253,248,187,1,141,19,10,4,8,25,1,149,5,154,250,102,5,154,251,143,55,72,60,69,4,111,0,130,128,8,39,26,0,0,7,96,5,154,0,27,1,65,64,32,123,27,1,121,26,1,118,20,1,118,13,1,115,
    12,1,117,9,1,123,2,1,27,32,12,0,77,26,131,4,42,20,184,255,232,64,9,11,0,77,19,24,130,4,36,13,184,255,224,179,130,26,37,12,184,255,224,64,10,130,8,38,10,8,11,12,0,76,9,131,36,32,14,
    130,31,33,8,16,130,4,32,2,131,4,37,1,184,255,240,64,9,130,33,33,0,16,130,4,39,27,184,255,208,179,12,1,77,130,7,34,224,179,13,132,7,39,232,64,19,11,1,77,12,48,130,21,33,12,32,130,18,
    33,12,24,130,14,43,5,184,255,200,64,34,11,13,1,76,23,56,131,5,32,15,132,5,34,23,64,11,130,100,32,15,132,5,37,5,16,13,6,77,23,130,92,32,179,130,7,32,15,130,7,33,64,29,130,8,53,0,1,5,
    15,19,23,6,10,11,3,10,20,19,6,1,2,23,9,10,15,23,32,130,139,130,33,33,224,179,130,7,130,49,34,224,64,20,130,137,32,15,131,212,52,15,6,23,3,11,0,0,29,11,47,29,1,93,47,17,51,47,18,23,
    57,43,130,0,34,17,51,51,133,2,73,29,7,65,97,5,75,209,6,126,207,13,32,93,133,0,37,1,1,35,1,38,39,74,69,5,74,61,8,74,69,6,8,66,7,96,254,107,197,254,217,19,4,4,6,20,254,215,195,254,92,
    185,1,49,19,5,5,5,26,1,61,161,1,48,16,8,4,4,23,1,37,5,154,250,102,4,24,67,79,74,70,251,230,5,154,251,180,69,75,53,91,4,76,251,172,57,77,52,86,4,80,65,173,5,8,45,4,154,5,154,0,21,0,
    192,64,25,123,19,1,122,18,1,117,13,1,116,12,1,116,8,1,117,7,1,121,2,1,123,1,1,19,184,255,224,64,17,11,13,1,76,12,32,131,5,32,8,132,5,32,1,131,21,32,10,131,15,33,19,8,65,181,5,34,248,
    64,9,130,8,42,10,16,11,0,77,8,184,255,248,64,15,130,13,32,1,131,27,33,15,24,74,149,6,84,208,5,42,15,184,255,240,64,31,13,6,77,5,16,130,4,45,1,5,15,19,4,9,11,3,9,21,10,11,20,0,131,60,
    36,0,0,23,11,9,130,35,32,179,130,72,37,9,47,43,198,17,51,131,4,33,57,57,65,56,9,65,52,11,65,42,5,33,93,93,77,15,5,36,6,7,1,35,1,66,175,8,8,86,51,1,4,154,205,254,182,15,18,4,10,24,254,
    172,206,1,224,254,70,206,1,37,29,22,4,32,24,1,49,193,254,61,2,37,25,46,23,48,253,219,2,209,2,201,254,8,50,50,66,38,1,244,253,57,0,1,0,18,0,0,4,96,5,154,0,13,0,101,64,33,115,6,1,4,9,
    1,1,5,3,18,13,3,5,3,9,2,6,130,181,38,2,6,12,6,77,2,6,68,253,5,35,184,255,254,179,130,14,32,3,130,7,33,64,22,130,18,40,3,3,5,13,125,0,0,15,6,132,247,40,6,5,32,15,1,93,47,51,43,130,199,
    34,233,17,57,68,106,6,41,57,0,63,63,63,18,57,47,205,51,130,204,37,93,1,1,17,35,17,136,182,8,34,4,96,254,39,168,254,51,191,1,65,6,29,3,10,28,1,80,5,154,252,120,253,238,2,14,3,140,253,
    120,12,76,34,54,2,136,130,163,32,33,130,163,32,100,130,163,54,9,0,149,64,13,124,6,1,116,1,1,1,32,11,13,1,76,6,184,255,224,64,41,131,9,8,35,21,0,37,0,2,3,0,1,227,0,243,0,2,26,5,42,5,
    2,12,5,1,236,5,252,5,2,5,1,145,4,18,0,7,145,8,3,130,45,34,248,64,9,130,210,33,1,8,130,4,130,13,74,38,5,33,1,12,130,209,132,13,37,20,11,6,77,1,12,130,4,36,3,7,7,1,6,82,77,5,33,4,47,
    130,186,35,18,23,57,47,130,205,77,205,5,41,237,57,63,237,57,49,48,93,113,113,130,2,35,1,43,43,93,130,211,62,33,21,33,53,1,33,53,33,4,100,252,184,3,52,251,209,3,65,253,2,4,0,5,114,251,
    38,152,47,4,211,152,130,195,36,200,254,186,2,56,130,195,53,7,0,51,64,13,5,217,2,3,6,217,1,32,4,0,6,234,1,184,255,236,179,130,143,130,7,34,232,64,9,130,138,71,101,7,37,43,43,225,221,
    196,0,91,136,5,62,1,33,17,33,21,35,17,51,2,56,254,144,1,112,222,222,254,186,6,224,119,250,14,0,1,255,230,255,16,3,30,98,167,7,43,120,1,1,0,8,11,0,77,0,2,3,0,98,167,10,44,5,35,1,51,
    3,30,151,253,95,155,240,6,138,130,139,36,53,254,186,1,164,132,139,39,47,64,23,5,217,6,3,2,130,139,37,5,1,0,234,3,8,130,136,33,3,8,65,225,5,32,238,104,192,9,138,135,52,53,51,17,35,53,
    33,1,164,254,145,221,221,1,111,254,186,119,5,242,119,0,130,83,62,230,2,112,4,150,5,178,0,7,0,38,64,22,121,6,1,121,1,1,7,16,11,12,0,76,0,5,3,7,4,47,130,33,40,5,47,47,93,0,63,51,47,196,
    77,44,5,33,1,35,132,1,52,51,4,150,150,254,180,6,254,203,147,1,166,65,2,112,2,107,253,149,3,66,130,45,43,0,254,215,3,82,255,78,0,3,0,13,180,130,158,38,0,1,47,47,0,47,237,130,58,43,33,
    53,33,3,82,252,174,3,82,254,215,119,130,39,38,82,4,194,1,215,6,10,130,39,56,28,183,119,3,1,3,128,1,0,2,184,255,192,179,13,16,72,2,47,43,205,0,47,26,205,130,52,32,93,130,107,38,51,1,
    215,127,254,250,168,130,47,58,72,0,2,0,90,255,232,3,131,4,24,0,20,0,31,0,149,64,71,119,16,1,116,9,1,6,24,131,177,8,35,21,150,11,16,13,6,77,11,11,18,1,21,3,28,149,5,22,15,64,9,12,72,
    15,15,13,149,18,16,32,15,48,15,2,15,11,31,71,139,15,34,132,1,2,65,195,5,43,244,179,12,6,77,1,184,255,238,64,23,11,130,8,38,1,33,15,25,131,8,12,130,19,33,8,4,130,15,41,47,8,1,8,47,93,
    43,43,233,51,81,187,9,41,51,51,47,93,0,63,237,50,47,43,130,4,36,63,17,57,47,43,130,230,59,43,93,93,33,35,53,35,6,35,34,38,53,16,37,37,16,35,34,7,53,54,51,32,17,7,7,6,6,106,22,6,8,50,
    3,131,164,4,107,208,153,173,1,82,1,51,211,185,149,151,197,1,105,164,247,114,116,107,89,122,159,160,184,162,134,1,31,47,43,1,5,126,168,96,254,130,148,34,16,81,103,75,95,171,131,130,
    247,63,166,255,232,4,84,5,236,0,16,0,29,0,140,64,35,119,10,1,3,21,1,21,149,15,22,7,27,149,9,16,4,0,88,144,15,34,131,24,184,88,76,5,130,7,32,242,131,219,130,7,34,227,64,26,130,203,38,
    24,24,31,6,2,18,6,130,246,33,18,6,130,223,33,18,6,130,19,34,18,132,3,130,46,33,64,17,130,20,33,3,2,130,20,33,3,1,130,20,34,3,47,31,85,102,10,32,50,137,249,33,0,63,131,241,73,22,5,35,
    93,37,35,21,81,225,5,40,51,50,18,21,16,2,35,34,3,134,231,8,75,52,38,35,34,6,1,78,4,164,164,4,121,233,197,223,250,217,203,108,165,127,149,169,158,135,143,174,148,148,5,236,253,96,204,
    254,237,231,254,255,254,203,2,73,143,127,177,228,203,171,194,199,0,1,0,96,255,232,3,98,4,24,0,21,0,115,64,82,120,3,1,41,15,57,15,2,48,130,234,51,21,19,149,2,22,11,11,13,149,8,16,0,
    32,12,1,77,0,0,11,12,130,169,33,11,2,130,169,36,112,11,1,11,32,130,19,32,11,81,30,15,34,131,5,20,130,210,33,5,12,130,40,33,5,35,85,130,10,41,43,47,43,93,43,43,51,47,43,0,130,201,32,
    47,131,3,46,93,49,48,93,1,93,37,6,35,34,0,53,52,0,51,85,120,5,65,178,5,8,39,55,3,96,118,162,219,254,243,1,34,242,135,103,114,130,157,201,189,159,134,118,47,71,1,29,227,253,1,51,50,
    168,80,225,183,180,208,89,0,0,2,131,187,33,4,16,65,163,8,57,48,120,6,1,28,24,11,12,0,76,2,27,149,5,22,1,21,14,21,149,11,16,15,0,17,13,81,254,15,34,132,1,5,66,132,5,32,248,66,132,6,
    34,240,64,42,130,172,36,1,1,31,24,6,130,24,33,24,6,130,190,33,24,6,130,17,35,24,131,8,16,130,16,33,8,14,130,16,33,8,26,130,16,32,8,65,162,11,65,160,9,33,51,51,65,162,5,130,216,35,49,
    48,43,1,66,146,7,32,2,133,217,40,51,17,51,3,53,52,38,35,34,133,222,8,45,54,4,16,164,4,114,238,193,231,1,0,213,211,96,4,164,164,164,126,150,172,165,139,137,171,174,198,1,19,237,254,
    1,50,166,2,122,251,227,151,124,172,220,194,177,205,198,134,231,8,43,3,221,4,24,0,18,0,25,0,155,179,120,7,1,6,184,255,192,64,37,11,14,72,6,6,4,1,149,25,25,15,4,149,9,22,22,149,15,16,
    127,7,1,7,22,130,172,41,7,32,12,1,77,7,7,0,131,19,130,41,36,180,11,12,0,76,130,8,33,208,179,130,20,130,7,33,240,179,130,211,130,7,34,232,64,40,130,210,37,19,19,27,25,1,8,130,17,33,
    1,8,69,84,8,33,131,12,131,246,33,12,16,130,16,33,12,29,130,35,32,12,80,28,14,34,43,233,50,130,17,108,68,5,35,17,57,47,237,130,3,46,43,49,48,93,1,33,22,22,51,50,55,21,6,35,34,133,250,
    35,18,21,39,38,131,247,8,51,7,3,221,253,45,4,176,154,173,145,135,222,217,248,1,15,201,201,220,168,1,135,120,116,162,19,1,215,171,186,114,154,98,1,23,253,239,1,45,254,252,231,53,142,
    158,166,134,0,0,1,0,53,130,248,41,160,6,2,0,20,0,126,185,0,16,130,238,37,52,11,0,77,10,21,130,21,41,8,5,12,149,13,15,2,149,18,1,130,224,32,12,130,162,32,47,130,46,37,12,0,12,5,14,9,
    131,198,32,9,131,198,33,9,8,130,181,35,9,132,10,184,81,21,5,132,7,130,41,130,7,34,230,64,10,130,26,39,10,10,22,21,47,22,1,93,76,133,5,53,43,233,43,43,43,51,50,51,51,47,47,93,43,51,
    47,0,63,237,63,237,51,50,130,212,36,63,49,48,43,1,130,192,33,21,21,102,35,5,8,72,35,53,51,53,52,54,51,50,23,2,160,48,61,172,240,240,163,175,175,186,139,75,44,5,92,27,217,158,140,252,
    140,3,116,140,166,161,187,18,0,2,0,96,254,30,4,16,4,24,0,24,0,37,0,178,64,13,122,4,1,120,14,1,36,24,11,12,0,76,8,184,255,224,180,131,8,33,5,184,130,213,57,49,11,15,72,5,5,7,149,2,28,
    10,35,149,13,22,23,15,21,29,149,19,16,4,21,25,22,76,198,16,42,132,9,5,13,6,77,9,184,255,248,179,130,202,130,7,34,240,64,43,130,202,37,9,9,39,4,32,6,130,25,33,32,6,130,22,33,32,6,130,
    18,35,32,131,16,16,130,16,33,16,14,130,16,33,16,26,130,16,34,16,47,39,68,89,9,69,82,10,32,23,132,236,32,50,68,94,6,33,47,43,91,236,5,52,0,93,37,16,33,34,39,53,22,51,32,17,53,35,6,35,
    34,2,53,16,18,130,244,33,51,53,66,199,15,8,76,253,228,190,142,173,157,1,122,4,117,235,191,233,251,218,207,100,4,164,164,165,123,152,172,165,136,138,173,82,253,204,72,164,96,1,146,112,
    196,1,17,230,1,5,1,52,166,142,253,207,151,122,174,221,199,171,205,196,0,0,1,0,166,0,0,3,248,5,236,0,17,0,140,64,35,117,16,1,130,2,42,1,21,9,21,12,4,149,15,16,10,0,70,56,17,86,71,9,
    32,248,67,182,6,34,248,64,30,130,220,35,1,1,19,12,84,158,15,34,132,9,2,65,43,5,32,254,65,43,6,33,254,182,130,41,34,9,47,19,65,6,9,69,95,16,46,63,49,48,1,93,0,93,33,35,17,16,35,34,6,
    21,87,71,6,8,34,51,32,17,3,248,164,238,120,164,164,164,4,118,218,1,90,2,78,1,64,185,145,253,188,5,236,253,106,194,254,95,0,2,0,144,130,204,58,102,5,217,0,11,0,15,0,79,64,34,6,99,64,
    0,14,15,13,9,98,3,13,12,3,98,255,48,70,40,15,34,132,13,184,74,28,5,131,7,131,149,35,13,13,17,16,66,135,5,33,225,43,130,0,108,104,6,35,63,214,26,237,100,88,13,8,35,19,35,17,51,250,44,
    62,62,44,45,63,63,35,164,164,5,4,60,46,46,61,61,46,44,62,250,252,4,0,0,2,255,58,254,30,1,131,135,61,12,0,24,0,102,64,48,2,2,1,16,4,19,99,13,11,15,6,6,8,149,64,3,28,22,98,16,10,0,16,
    130,145,32,6,65,91,16,35,127,10,1,10,134,149,131,7,131,149,35,10,10,26,25,133,149,32,93,131,150,33,198,43,133,151,38,63,26,237,50,47,63,214,130,154,36,1,95,94,93,37,89,62,11,33,3,34,
    100,248,9,42,1,74,203,189,70,66,74,66,224,164,80,134,177,43,43,252,254,239,32,153,45,1,102,3,240,1,136,185,66,5,10,52,12,0,155,64,16,122,12,1,119,11,1,121,2,1,125,10,1,126,1,1,12,77,
    213,7,37,1,16,11,0,77,0,131,13,52,34,12,0,77,9,2,10,5,1,21,5,21,10,15,6,0,10,11,11,1,112,130,70,41,20,11,6,77,0,64,11,1,77,0,130,52,38,232,64,28,13,6,77,12,91,65,16,34,132,5,4,130,
    23,35,5,184,255,255,131,225,36,5,47,14,1,93,75,123,6,34,50,50,43,130,9,33,93,51,89,86,5,33,63,63,130,249,89,89,6,41,93,93,0,93,93,93,33,35,1,35,66,16,5,60,1,51,1,3,248,230,254,60,4,
    164,164,4,1,174,215,254,37,1,236,254,20,5,236,252,63,1,213,254,18,133,207,44,1,74,5,236,0,3,0,58,64,22,1,21,2,66,200,20,33,254,179,72,252,5,32,254,131,135,35,1,1,5,4,65,105,5,48,225,
    43,43,43,0,63,63,49,48,33,35,17,51,1,74,164,164,130,71,34,1,0,166,130,66,8,55,84,4,24,0,31,0,220,182,73,12,1,73,4,1,30,184,255,232,183,11,0,77,15,33,1,3,28,184,255,208,64,42,12,0,77,
    2,22,1,12,4,5,21,22,27,4,13,149,64,29,24,16,19,15,10,1,18,65,233,16,32,1,65,230,6,130,7,51,236,64,28,11,6,77,1,14,10,18,10,66,20,17,6,13,6,77,17,6,130,150,33,17,6,130,21,39,17,132,
    18,184,255,250,64,49,130,15,33,18,14,130,15,33,18,27,84,34,15,35,132,10,10,4,130,50,36,10,64,13,1,77,130,105,48,12,3,32,33,10,132,48,48,43,1,95,94,93,43,43,16,77,131,203,33,57,24,89,
    28,6,39,50,43,1,16,77,240,43,43,132,224,42,24,47,51,51,63,63,51,26,237,23,50,130,43,33,49,48,130,50,35,93,43,93,93,130,243,71,35,5,32,17,67,128,10,8,58,21,51,54,51,50,22,23,54,51,32,
    17,6,84,164,105,124,105,147,164,233,108,140,164,164,4,109,209,105,156,29,114,226,1,82,2,76,170,152,192,134,253,184,2,96,1,46,181,145,253,184,4,0,162,186,117,95,212,254,95,65,139,5,
    43,3,248,4,24,0,18,0,140,64,35,117,17,68,97,7,38,10,15,12,4,149,15,16,65,27,19,33,246,179,72,24,13,68,97,7,32,20,68,97,40,32,20,68,97,22,35,237,50,63,63,68,97,18,133,224,38,21,3,248,
    164,238,123,161,130,215,41,116,220,168,178,2,72,1,70,185,141,131,202,35,170,194,217,205,72,231,6,32,80,130,199,51,11,0,23,0,131,64,31,25,64,11,2,77,18,149,0,22,12,149,6,16,65,151,15,
    32,131,88,246,10,37,240,64,9,12,6,77,88,238,5,44,184,255,222,64,42,11,6,77,21,21,25,15,8,130,228,33,15,10,130,26,33,15,6,130,17,35,15,131,3,16,130,16,33,3,12,130,16,33,3,25,88,247,
    18,33,43,93,94,103,5,32,0,77,99,6,44,43,5,34,0,53,16,0,51,50,0,21,20,0,101,75,10,8,45,38,2,82,227,254,241,1,26,240,229,1,1,254,235,221,158,184,186,156,159,171,171,24,1,31,237,1,2,1,
    34,254,230,250,245,254,217,3,166,215,189,182,210,206,190,192,208,130,215,36,166,254,41,4,84,130,215,75,99,10,44,27,1,21,149,15,22,4,15,6,27,149,9,16,69,58,15,75,99,29,32,18,79,134,
    15,75,99,46,32,237,71,7,5,44,49,48,1,93,37,35,17,35,17,51,21,51,54,75,99,28,37,198,222,250,217,199,112,75,99,8,36,253,149,5,215,180,75,99,18,34,2,0,96,130,231,32,16,134,231,34,137,
    64,47,74,164,5,48,1,27,3,27,149,5,22,15,15,13,21,149,11,16,13,17,14,71,240,17,74,166,79,35,23,51,0,63,136,229,33,43,1,130,228,73,169,8,32,23,71,220,17,8,44,164,4,107,243,193,233,1,
    0,216,210,94,4,164,164,165,127,148,172,166,131,144,171,254,41,2,135,200,1,19,237,255,1,49,166,142,253,205,149,126,174,219,199,177,201,197,67,109,5,61,2,188,4,18,0,16,0,89,185,0,12,
    255,232,64,39,11,12,0,76,7,21,8,15,0,0,10,2,14,16,112,130,35,34,0,18,10,98,117,15,44,132,7,184,255,243,64,14,13,6,77,7,4,12,130,4,33,5,11,130,4,74,111,8,42,17,57,47,93,0,63,205,50,
    50,47,63,73,140,7,71,154,5,8,34,21,51,54,54,51,50,23,2,188,43,81,105,141,164,164,4,35,144,89,64,34,3,90,33,198,171,253,246,4,0,211,108,121,14,0,130,111,61,104,255,232,3,15,4,24,0,46,
    0,139,64,22,119,38,1,115,22,1,120,12,1,120,9,1,123,0,1,43,16,131,159,37,18,184,255,240,64,45,131,9,33,10,56,131,5,8,39,5,40,15,30,4,20,45,23,25,149,20,16,32,1,48,1,64,1,3,1,3,149,45,
    22,23,23,40,30,131,15,15,0,40,131,5,184,255,224,64,22,130,165,41,32,5,48,5,2,5,5,48,0,16,130,188,33,0,28,100,95,6,131,177,36,43,225,18,57,47,131,3,36,0,63,253,198,93,130,3,41,17,18,
    23,57,43,49,48,43,43,93,131,0,36,55,53,22,51,50,87,230,32,8,94,104,134,161,216,37,63,85,49,68,103,69,35,66,110,141,75,133,105,113,147,46,74,53,29,29,56,80,51,68,108,76,41,67,112,146,
    80,158,37,176,99,144,41,57,44,35,19,27,55,69,88,59,72,111,75,38,46,166,74,21,38,53,32,40,54,42,34,20,26,55,69,90,62,76,112,74,36,0,0,1,0,43,255,234,2,129,5,47,0,20,0,108,64,51,3,24,
    131,236,58,20,20,18,149,2,22,10,10,14,11,6,149,7,15,0,0,13,64,11,0,77,13,6,13,6,11,8,91,143,15,34,132,4,184,94,248,5,130,7,36,240,179,12,6,77,130,7,33,238,179,130,243,32,4,65,171,7,
    37,51,50,51,50,47,47,75,63,5,130,11,32,47,97,102,5,49,43,37,6,35,32,17,17,35,53,51,53,55,17,33,21,33,17,20,130,251,8,34,55,2,129,58,95,254,243,176,176,164,1,2,254,254,70,81,62,45,10,
    32,1,44,2,94,140,250,53,254,209,140,253,191,103,88,34,131,177,47,144,255,232,3,226,4,0,0,17,0,149,64,48,121,6,1,130,2,33,13,16,131,183,46,56,3,1,1,21,3,12,149,5,22,16,15,8,15,1,69,
    201,16,38,127,15,1,15,184,255,250,85,180,6,32,252,131,174,130,15,34,252,64,42,130,175,34,15,15,19,101,136,15,34,132,7,10,88,12,8,32,2,130,34,32,7,74,39,11,77,229,5,34,93,233,43,99,
    93,6,38,237,50,63,93,49,48,43,74,42,5,33,53,35,132,221,8,43,51,17,16,51,50,54,53,17,51,3,226,164,4,102,214,254,146,163,248,120,155,164,162,186,1,180,2,100,253,182,254,188,177,143,2,
    78,0,1,0,14,0,0,3,203,130,207,60,11,0,123,64,13,121,11,1,121,2,1,119,1,1,118,4,1,11,184,255,224,64,11,11,13,1,76,4,32,131,5,37,2,184,255,244,64,25,83,101,7,36,118,7,1,7,48,131,22,38,
    11,7,3,2,21,3,15,130,45,34,244,64,20,130,29,43,11,0,16,11,12,0,76,0,0,13,4,12,87,206,6,54,240,179,12,0,77,3,47,43,51,43,17,51,47,43,50,43,0,63,63,18,57,57,43,130,177,32,1,130,192,33,
    43,93,130,0,87,190,11,62,3,203,254,104,161,254,124,180,1,4,29,7,4,10,22,1,16,4,0,252,0,4,0,253,24,82,61,77,62,2,236,130,179,36,24,0,0,5,176,130,179,62,27,0,210,64,22,120,27,1,119,20,
    1,118,12,1,123,10,1,116,1,1,56,9,1,55,12,1,27,184,255,248,179,130,126,35,19,184,255,254,131,7,32,10,134,15,130,196,34,250,64,24,130,24,33,20,2,130,4,33,12,8,130,4,33,9,6,130,4,32,1,
    131,9,43,8,184,255,192,179,11,0,77,20,184,255,240,87,240,9,32,12,130,58,132,13,33,9,16,130,27,130,88,65,21,6,86,101,5,43,6,184,255,208,64,35,11,13,1,76,23,48,131,5,32,15,132,5,43,2,
    6,15,23,19,27,0,7,10,11,15,10,87,100,11,86,27,8,86,26,8,35,0,43,1,43,134,0,87,78,9,48,3,38,39,35,6,7,3,35,1,51,19,22,23,51,54,55,19,135,7,8,42,5,176,254,205,170,211,12,4,4,3,18,229,
    164,254,202,172,212,10,4,8,3,15,236,150,212,10,5,8,2,15,208,4,0,252,0,2,221,42,53,36,57,253,33,130,11,47,254,35,57,44,50,3,0,252,252,37,55,39,53,3,4,0,87,73,5,60,3,146,4,0,0,19,0,224,
    64,19,123,19,1,115,14,1,121,12,1,116,10,1,123,3,1,117,1,1,19,133,235,38,6,77,14,8,11,6,77,134,249,34,6,77,10,131,13,32,3,135,27,32,1,131,13,37,13,184,255,240,179,12,132,35,132,7,32,
    11,130,15,33,64,19,130,16,33,2,16,130,4,32,1,131,4,32,0,132,9,130,23,35,179,11,0,77,130,89,32,240,87,157,5,38,14,16,11,13,1,76,10,132,5,130,83,34,240,64,10,131,15,42,17,24,11,1,77,
    6,184,255,232,64,21,130,8,60,3,6,17,19,4,11,13,15,11,0,2,13,11,2,2,21,11,21,184,255,192,178,12,0,77,43,47,17,51,130,2,32,17,97,188,12,88,149,13,68,207,6,35,93,9,2,35,65,71,8,65,72,
    5,8,82,1,3,146,254,168,1,82,191,201,19,26,4,5,42,205,189,1,93,254,178,191,198,22,21,4,1,0,4,0,253,250,254,6,1,76,31,47,9,69,254,180,1,246,2,10,254,162,39,41,1,174,0,0,1,0,14,254,30,
    3,213,4,0,0,20,0,145,64,10,122,20,1,116,13,1,119,2,1,20,184,255,224,64,39,131,186,32,13,67,30,5,32,32,130,229,40,117,16,1,0,15,12,15,16,48,131,23,47,16,1,11,6,6,8,149,3,28,6,11,184,
    255,244,64,14,67,58,7,36,1,6,11,3,12,130,62,67,47,5,32,20,67,47,7,34,22,13,12,130,33,34,13,12,184,94,14,5,32,12,67,47,9,47,18,23,57,43,43,47,0,63,237,50,47,47,51,51,43,63,67,235,5,
    132,231,34,1,1,2,102,43,6,33,55,55,87,152,8,8,45,3,213,254,41,126,228,64,43,53,44,124,62,82,254,112,182,1,21,5,16,6,5,15,1,35,4,0,251,92,254,194,13,147,18,148,194,3,254,252,236,15,
    63,24,52,3,22,87,163,5,33,3,112,130,225,43,9,0,148,64,13,122,6,1,118,1,1,1,88,20,5,95,166,7,87,126,32,32,56,135,221,87,204,27,39,149,4,21,0,7,149,8,15,87,162,20,130,223,87,156,14,65,
    215,5,87,162,11,55,3,112,253,162,2,88,252,183,2,94,253,219,3,22,3,209,252,187,140,51,3,65,140,0,130,195,8,54,92,254,186,2,43,5,154,0,24,0,85,64,37,17,5,82,64,6,11,0,6,76,20,48,12,83,
    11,3,24,83,0,32,18,21,5,12,24,24,21,5,15,8,21,234,2,24,13,6,77,2,184,255,220,179,12,132,7,33,247,179,130,189,33,2,47,130,111,42,225,57,57,206,16,194,47,50,17,18,57,130,143,63,63,237,
    43,0,24,47,26,77,237,57,49,48,1,36,17,17,52,39,53,54,53,17,16,37,21,6,21,17,20,7,21,22,130,5,8,46,22,23,2,43,254,205,156,156,1,51,159,150,150,71,88,254,186,4,1,50,1,44,202,10,116,10,
    206,1,36,1,54,4,128,4,194,254,219,210,48,4,45,211,254,223,111,95,2,131,169,56,172,254,30,1,64,6,30,0,3,0,19,64,9,3,0,28,0,234,144,1,1,1,47,93,225,122,154,5,47,35,17,51,1,64,148,148,
    254,30,8,0,0,0,1,0,66,130,215,32,16,132,215,42,92,64,31,12,0,82,64,24,19,5,24,130,215,56,18,83,19,3,6,83,5,32,12,9,0,0,3,18,6,22,15,3,234,9,184,255,224,64,15,130,218,39,80,9,1,63,9,
    1,9,32,81,146,5,32,252,131,221,32,9,130,221,34,93,93,43,130,223,36,198,50,16,202,47,145,222,130,210,35,16,5,53,54,130,223,36,52,55,53,38,53,131,235,8,40,4,17,17,20,23,2,16,156,254,
    206,87,73,149,149,160,1,50,156,1,240,10,202,254,212,254,206,4,126,2,96,110,1,33,211,45,4,48,210,1,37,194,130,229,37,254,202,254,220,206,10,130,175,45,209,1,184,4,170,2,238,0,20,0,57,
    185,0,1,130,144,37,29,9,12,72,11,32,130,4,8,100,9,17,190,3,7,190,64,20,13,20,189,0,9,189,10,21,22,10,0,255,58,43,1,16,77,225,16,225,0,24,47,196,26,253,220,237,196,49,48,1,43,43,1,6,
    6,35,34,39,38,35,34,7,35,54,54,51,50,23,22,51,50,54,55,4,170,7,147,126,109,156,97,62,140,6,135,5,149,127,106,129,132,65,61,74,2,2,238,148,162,109,67,176,143,167,88,90,98,80,0,255,255,
    112,197,5,8,53,6,226,2,38,0,36,0,0,1,7,0,143,0,230,1,91,0,34,183,3,112,31,1,2,127,31,1,184,255,243,64,10,31,25,6,7,62,3,2,28,5,38,0,43,53,53,1,43,93,53,93,53,0,3,134,59,8,39,205,0,
    16,0,27,0,35,1,82,64,24,118,34,1,121,29,1,116,12,1,123,10,1,123,7,1,119,6,1,16,6,13,6,77,6,184,255,250,64,9,110,186,7,133,13,37,12,6,77,16,6,11,134,27,32,44,130,8,37,29,8,12,0,77,2,
    131,4,42,29,28,2,1,32,1,34,35,3,4,32,113,64,6,37,16,8,11,12,0,76,113,65,14,34,32,184,255,113,65,5,32,31,130,8,39,64,28,11,13,1,76,16,32,131,5,32,6,132,5,42,0,22,1,29,30,31,33,34,5,
    5,32,130,41,33,240,179,130,126,132,7,130,120,131,7,33,64,38,130,115,8,34,32,32,18,22,72,32,0,22,2,145,28,28,22,0,17,197,11,16,6,22,3,4,0,18,25,196,14,64,9,12,72,14,20,196,8,131,91,
    32,18,130,153,43,8,8,28,35,6,16,4,5,119,5,1,121,113,122,19,32,37,113,122,11,50,57,47,43,233,212,43,233,0,63,196,63,205,205,212,237,17,18,57,47,131,4,32,43,130,0,38,17,18,23,57,93,49,
    48,130,9,33,43,1,113,138,8,40,14,192,16,135,5,192,192,14,192,70,220,14,113,154,6,44,38,53,52,54,51,50,22,21,20,7,3,34,6,115,25,5,33,52,38,113,174,16,46,17,101,120,92,88,112,96,110,
    50,64,114,48,66,66,201,113,186,11,52,98,52,112,89,110,109,84,112,54,1,21,63,48,114,66,48,49,62,251,174,2,113,203,6,57,255,255,0,94,254,80,4,140,5,178,2,38,0,38,0,0,0,7,0,221,2,57,0,
    0,255,255,111,21,5,8,36,7,101,2,38,0,40,0,0,1,7,0,142,1,23,1,91,0,19,64,11,1,56,14,12,2,3,62,1,14,5,38,0,43,53,1,43,53,134,45,35,5,64,7,11,130,45,32,49,132,45,36,216,1,181,1,89,132,
    45,39,0,30,20,11,18,37,1,30,139,45,38,94,255,232,5,170,6,224,130,45,32,50,132,45,34,143,1,102,130,45,48,27,64,16,3,112,39,1,2,0,39,33,3,9,37,3,2,36,132,50,131,97,32,93,132,99,38,170,
    255,232,4,213,6,228,130,53,32,56,134,53,49,29,1,93,0,23,64,13,2,1,0,29,23,5,13,37,2,1,26,136,50,132,49,38,90,255,232,3,131,6,10,130,49,32,68,132,49,34,142,0,233,130,227,130,195,41,
    2,18,34,32,16,0,37,2,34,17,138,195,143,45,34,67,0,203,134,45,32,17,130,45,35,11,37,2,33,145,45,32,14,136,91,34,215,0,150,130,45,46,22,185,0,2,255,236,64,9,36,32,15,20,37,2,36,135,48,
    33,255,255,91,135,5,33,5,135,134,47,48,6,0,143,98,0,0,26,177,3,2,184,255,226,64,10,47,41,130,46,34,3,2,44,132,47,131,241,32,53,136,49,32,178,136,97,34,216,0,168,135,97,35,235,64,9,
    42,132,97,32,42,143,97,33,6,28,136,47,32,220,132,237,133,99,35,233,64,10,35,133,99,32,32,140,99,38,96,254,80,3,98,4,24,130,51,32,70,130,91,48,7,0,221,1,131,0,0,255,255,0,96,255,232,
    3,221,6,10,130,23,32,72,65,57,5,33,1,15,65,11,6,39,39,27,29,12,18,37,2,28,65,11,11,143,45,34,67,0,254,134,45,34,9,29,27,131,45,32,27,145,45,32,14,136,91,34,215,0,184,135,215,41,247,
    64,9,31,32,22,15,37,2,30,138,215,132,93,33,5,135,136,47,34,143,0,136,130,47,39,23,64,13,3,2,10,41,35,130,94,34,3,2,38,137,212,131,189,32,88,130,179,133,189,32,214,130,9,52,6,0,142,
    209,0,0,22,185,0,1,255,159,64,9,6,5,2,3,37,1,6,137,95,33,255,236,130,35,32,113,132,235,133,45,42,67,154,0,0,19,64,11,1,57,5,4,131,42,32,5,138,187,38,255,211,0,0,2,7,6,131,187,131,43,
    36,7,0,215,255,119,130,139,132,91,35,245,64,9,8,132,48,32,8,138,91,32,201,130,47,32,16,132,187,133,47,34,143,255,76,130,47,42,26,177,2,1,184,255,244,64,10,19,13,130,97,34,2,1,16,65,
    147,12,87,87,5,32,178,130,239,32,81,130,153,36,7,0,216,0,232,135,99,41,252,64,9,29,19,10,18,37,1,29,65,31,13,33,4,80,132,191,32,82,132,47,34,142,1,66,133,47,43,2,255,228,64,9,25,25,
    12,6,62,2,26,154,47,34,67,1,20,130,47,130,241,40,2,20,25,25,3,9,37,2,25,65,173,14,130,93,32,14,130,141,133,93,34,215,0,229,134,45,34,0,29,30,131,45,66,9,15,33,4,80,132,239,133,45,34,
    143,0,183,65,171,7,67,214,8,65,171,13,34,96,255,232,130,49,131,237,133,49,34,216,0,255,134,95,34,10,34,24,131,95,67,163,12,36,144,255,232,3,226,132,235,32,88,134,235,32,60,133,235,
    43,1,255,251,64,9,20,18,9,17,37,1,20,138,235,143,47,34,67,0,234,133,93,40,1,23,20,18,8,16,37,1,19,139,235,133,45,131,235,133,93,34,215,0,198,134,45,35,3,22,18,8,130,90,32,22,144,45,
    132,235,133,45,34,143,0,160,133,235,36,2,1,7,33,27,130,46,34,2,1,30,138,235,8,109,0,1,0,48,0,229,2,216,5,153,0,11,0,42,64,21,11,1,9,6,4,16,8,32,8,2,8,0,5,145,11,6,6,3,8,3,3,0,47,63,
    18,57,47,51,237,50,1,47,93,51,206,221,50,206,49,48,1,37,3,35,3,5,53,5,3,51,3,37,2,216,254,250,40,76,40,254,250,1,46,40,156,40,1,46,3,125,30,253,74,2,182,30,150,20,1,154,254,102,20,
    0,0,2,0,108,3,129,2,150,5,170,130,99,8,44,23,0,52,64,29,18,216,0,14,12,216,64,6,4,3,9,66,21,216,9,15,216,3,15,9,1,23,3,24,25,3,9,255,58,43,1,95,94,93,16,77,225,16,225,43,24,73,157,
    7,24,73,155,25,8,36,1,129,115,162,160,115,116,163,162,115,65,92,90,65,65,94,92,3,129,163,116,115,159,160,114,116,163,1,180,92,65,67,95,96,66,66,91,131,129,8,85,184,255,207,3,184,5,
    133,0,22,0,29,0,107,64,56,117,16,1,121,0,1,22,22,23,20,154,2,5,2,50,17,66,17,2,17,17,24,19,153,11,14,2,14,2,12,4,30,12,11,14,20,23,4,2,220,112,5,1,2,5,1,5,5,8,17,0,0,31,27,137,8,184,
    255,248,179,13,6,77,8,47,43,225,18,57,47,196,130,3,54,93,93,225,23,57,0,47,16,198,17,57,57,47,47,51,237,50,50,47,93,17,51,16,131,7,8,158,49,48,93,93,37,6,7,21,35,53,38,2,53,52,18,55,
    53,51,21,22,23,21,38,39,17,54,55,5,17,6,6,21,20,22,3,184,96,125,125,193,229,229,193,125,123,98,101,120,122,99,254,166,119,135,136,209,59,10,189,191,22,1,22,207,219,1,39,37,213,203,
    2,49,172,71,7,252,239,12,74,75,2,249,31,205,150,150,198,0,1,0,106,0,0,3,238,5,178,0,27,0,77,64,42,114,14,1,118,18,1,23,20,5,154,8,8,27,15,17,154,12,4,2,27,154,0,18,21,21,24,127,14,
    1,14,0,26,2,6,20,8,24,138,32,4,1,4,47,93,225,57,57,198,198,50,47,196,93,17,57,47,75,175,5,32,205,130,195,35,237,57,57,49,90,166,5,36,33,53,54,53,53,92,144,8,38,21,38,35,34,17,21,33,
    130,1,8,86,20,7,33,3,238,252,124,218,197,197,218,173,117,96,100,111,229,1,40,254,216,193,2,199,135,69,244,187,141,254,188,240,41,155,57,254,205,236,141,156,242,97,0,2,0,131,255,190,
    3,39,5,211,0,38,0,48,0,187,64,27,116,34,1,115,27,1,124,15,1,123,7,1,124,48,1,117,43,1,117,38,1,17,32,12,0,77,35,103,138,6,43,23,184,255,240,64,83,11,12,0,76,3,16,131,5,8,44,44,0,10,
    39,20,5,25,80,8,1,8,8,10,154,5,96,28,112,28,2,28,28,30,154,25,44,0,2,41,39,20,46,22,13,126,2,28,64,11,16,1,76,2,64,12,130,5,130,29,54,2,46,126,37,64,11,12,1,76,37,37,50,18,32,126,22,
    8,22,8,41,126,18,47,130,239,35,47,47,225,17,130,227,32,43,132,10,34,43,43,16,131,13,32,57,130,17,38,57,0,47,237,50,47,93,132,4,131,14,126,78,5,33,43,43,124,1,6,55,93,93,93,1,22,21,
    20,6,35,34,39,53,22,51,50,54,53,52,38,39,38,53,52,55,130,3,33,54,51,96,88,5,35,21,20,22,23,130,34,33,1,6,131,8,131,32,8,106,2,146,90,200,155,131,108,130,120,84,92,71,145,233,147,106,
    187,153,130,89,101,122,168,81,163,223,254,103,101,113,141,88,105,1,160,74,112,132,164,56,158,74,79,60,53,73,77,122,195,163,81,83,140,127,164,43,158,62,144,55,90,78,107,186,170,1,127,
    51,108,72,109,63,68,104,72,101,0,1,0,164,1,152,2,156,3,144,0,11,0,26,64,12,0,14,64,6,3,9,66,9,32,3,1,3,47,93,205,76,215,6,67,25,13,46,1,162,104,150,150,104,104,146,146,1,152,146,104,
    103,151,131,10,32,0,130,71,8,34,84,0,0,3,66,5,154,0,15,0,116,64,52,8,8,2,4,0,226,15,3,6,2,18,0,0,2,16,13,6,77,2,140,3,24,112,162,8,36,38,11,6,77,47,130,98,32,6,131,25,35,6,140,8,3,
    130,25,33,8,184,121,45,5,130,7,33,236,180,130,31,37,8,11,184,255,240,179,106,27,5,33,228,179,130,16,43,11,47,43,43,221,43,43,43,225,43,212,93,130,6,44,241,43,202,47,0,63,51,63,237,
    50,18,57,47,130,161,33,35,17,133,1,91,202,5,48,33,3,66,128,106,158,108,108,142,149,107,1,238,5,51,250,205,131,3,37,3,135,152,115,117,147,130,171,53,166,255,232,4,20,6,2,0,37,0,154,
    64,9,15,34,1,0,17,1,16,5,32,94,221,7,8,48,26,184,255,232,64,43,11,0,77,31,9,150,10,64,24,36,10,149,20,48,16,149,24,1,1,3,149,36,22,20,21,31,27,9,0,0,19,13,131,27,16,12,6,77,27,27,6,
    131,33,131,47,32,22,130,12,34,33,19,8,130,175,33,19,8,130,10,33,19,8,130,177,39,19,132,20,184,255,248,64,9,130,15,33,20,6,130,15,32,20,90,83,6,35,47,43,225,57,130,3,38,17,57,47,196,
    17,57,0,94,194,5,50,43,0,26,24,16,77,237,57,49,48,43,43,1,95,94,93,0,93,37,66,17,8,40,53,54,54,53,52,38,35,34,17,130,226,75,18,6,8,107,6,7,21,4,17,20,6,35,34,1,205,75,95,109,136,200,
    178,118,152,110,97,231,164,220,186,166,198,150,120,1,122,222,183,101,2,160,48,147,124,150,187,20,130,20,166,114,108,119,254,203,251,190,4,90,192,232,190,156,127,202,39,5,75,254,161,
    177,240,255,255,0,144,1,250,2,178,2,123,2,6,0,16,0,0,0,4,0,162,255,223,6,125,5,187,0,11,0,23,0,42,0,50,0,108,179,121,27,1,18,131,222,37,55,12,0,77,12,24,130,4,8,50,40,29,76,44,44,32,
    24,31,31,18,43,76,32,32,12,18,203,0,199,12,203,6,4,39,30,47,202,36,36,30,25,24,24,21,44,30,200,31,31,3,21,201,9,198,15,201,3,47,233,240,233,130,231,37,225,50,18,57,47,51,130,3,33,225,
    18,130,240,34,237,244,237,130,19,32,237,68,219,6,32,237,115,92,5,36,93,5,32,0,17,126,249,7,32,1,138,11,37,19,35,39,38,35,35,130,250,32,33,124,163,7,8,34,23,1,17,51,50,53,52,38,35,3,
    144,254,201,254,73,1,183,1,55,1,54,1,183,254,73,254,202,254,238,254,124,1,132,1,18,130,1,56,131,254,125,90,162,104,79,91,88,133,1,12,167,181,127,114,79,78,254,113,143,197,106,115,33,
    132,47,32,55,132,47,32,201,131,63,35,5,132,254,124,130,55,36,238,254,125,1,131,132,57,57,132,251,149,229,172,254,111,3,186,140,120,98,138,24,4,17,159,2,74,254,186,164,88,74,0,3,65,
    33,12,41,45,0,100,185,0,18,255,232,64,58,65,28,7,51,45,43,83,15,26,31,26,223,26,3,26,26,37,18,98,35,1,35,37,83,65,36,10,43,34,111,45,1,45,45,21,40,202,29,29,15,65,31,14,38,17,57,47,
    93,198,0,63,65,25,6,35,205,93,17,18,130,16,37,237,205,43,43,49,48,65,23,24,100,128,21,65,18,30,50,27,114,155,209,254,230,1,33,231,132,104,95,159,156,203,207,160,146,104,65,16,35,8,
    44,167,69,1,27,212,247,1,38,53,150,72,213,180,174,211,78,0,0,2,0,76,3,4,5,116,5,154,0,15,0,23,0,114,64,69,91,12,1,13,40,11,12,0,76,12,132,5,59,2,7,12,3,22,1,5,9,3,19,17,21,84,64,14,
    10,22,3,14,0,80,80,1,1,15,1,1,11,130,2,8,70,8,80,15,9,1,16,3,9,9,19,16,14,18,80,19,19,21,66,21,32,19,1,19,47,93,198,43,1,16,77,241,226,17,51,24,47,95,94,93,225,50,51,47,94,93,93,225,
    51,0,63,51,51,26,237,50,47,23,51,18,23,57,49,48,43,43,93,1,35,17,35,3,130,1,36,17,35,17,51,19,130,1,32,5,130,15,62,17,35,53,33,5,116,108,4,219,59,214,4,102,127,225,4,228,126,252,230,
    209,111,206,2,14,3,4,1,222,254,34,131,3,8,40,2,150,254,16,1,240,98,253,204,2,52,98,0,0,1,0,135,4,194,2,12,6,10,0,3,0,19,183,120,3,1,3,2,0,128,2,47,26,205,0,47,104,84,5,46,1,35,19,2,
    12,254,250,127,223,6,10,254,184,1,72,131,247,32,125,130,49,34,196,5,135,67,37,5,61,64,22,6,0,18,193,12,9,192,3,15,192,16,21,1,15,3,1,21,3,21,3,25,24,17,18,57,57,47,47,130,185,35,16,
    225,0,47,70,250,5,68,153,5,37,50,22,21,20,6,33,138,11,55,2,98,40,58,56,40,42,58,56,254,86,42,59,57,41,41,60,58,4,194,56,43,42,56,130,10,34,58,59,40,133,7,8,48,0,1,0,232,0,0,4,148,4,
    164,0,19,0,114,64,47,14,17,18,1,2,13,2,11,8,7,4,3,12,3,18,1,4,190,7,17,14,8,190,12,64,11,1,2,4,7,8,11,12,130,32,34,10,3,13,130,1,44,16,9,0,5,0,184,255,224,179,12,6,77,5,130,7,68,141,
    5,40,21,5,0,255,58,43,1,43,43,131,166,40,57,57,24,47,47,18,23,57,0,130,246,32,77,130,170,41,221,253,57,57,221,49,48,135,5,192,130,0,33,16,135,130,4,42,192,1,33,3,35,19,35,53,33,19,
    33,130,3,8,60,51,3,33,21,33,3,33,4,148,253,218,163,141,164,250,1,63,149,254,44,2,26,158,141,159,1,6,254,182,152,1,226,1,62,254,194,1,62,132,1,38,132,1,56,254,200,132,254,218,0,2,0,
    10,0,0,6,131,5,154,0,15,130,191,8,43,164,64,14,19,3,4,18,4,2,18,1,0,4,1,16,4,18,184,255,240,64,33,12,0,77,12,8,0,0,21,1,18,6,6,5,17,9,14,8,13,6,77,14,8,130,181,34,14,8,11,130,9,36,
    126,1,184,255,216,94,226,14,34,214,64,34,130,26,8,37,1,1,21,4,5,3,145,19,19,7,0,12,145,64,11,7,0,11,145,43,48,18,8,145,7,3,5,15,145,0,18,0,63,237,196,63,237,50,70,202,6,130,239,43,
    47,237,1,47,51,18,57,47,43,43,43,225,130,3,33,57,57,130,11,32,51,131,23,40,196,196,49,48,43,95,94,93,93,131,243,34,33,33,17,130,243,32,1,130,233,32,17,132,3,8,33,1,17,35,1,6,131,253,
    9,254,2,193,195,2,188,3,156,253,209,2,7,253,249,2,80,253,9,95,254,166,1,146,254,110,5,126,190,6,48,1,145,2,217,253,39,0,0,3,0,94,255,205,5,170,5,211,130,241,52,27,0,35,0,243,64,10,
    119,9,1,119,14,1,120,13,1,29,184,255,232,179,130,240,32,13,130,249,33,179,11,130,248,130,15,37,180,11,12,0,76,10,130,16,33,64,33,130,17,43,20,29,34,25,1,2,11,12,2,12,2,5,90,207,15,
    33,125,34,71,25,6,130,7,36,234,179,12,6,77,131,7,41,64,9,11,6,77,32,34,48,34,2,130,13,34,248,64,69,130,13,41,34,34,37,25,6,13,6,77,25,6,130,35,33,25,6,130,17,35,25,125,5,16,130,16,
    33,5,16,130,16,33,5,24,130,16,62,5,12,11,8,2,1,18,20,27,28,29,4,31,22,0,3,10,13,4,18,22,145,8,4,31,145,18,19,47,37,1,103,38,5,35,18,23,57,17,130,3,34,16,206,50,130,2,32,1,65,55,7,36,
    18,57,47,43,93,65,67,6,74,149,5,40,17,51,17,18,57,57,49,48,0,130,21,52,43,1,93,93,0,93,37,7,39,55,38,17,16,0,33,50,23,55,23,7,22,131,9,8,155,32,1,38,35,34,0,17,20,23,1,1,22,51,50,0,
    17,16,1,68,152,78,158,158,1,117,1,67,234,168,135,78,139,184,254,140,254,200,254,246,2,78,126,186,227,254,231,98,3,21,253,43,131,203,236,1,16,131,182,65,191,195,1,44,1,87,1,159,129,
    162,63,166,200,254,185,254,161,254,104,4,204,102,254,184,254,249,223,150,3,20,252,158,137,1,53,1,20,1,4,0,3,0,82,0,207,6,58,3,217,0,19,0,29,0,39,0,122,64,21,119,37,1,119,33,1,120,27,
    1,120,23,1,20,0,35,30,10,5,25,189,15,184,255,224,64,12,12,6,77,32,15,1,15,15,41,35,189,5,131,16,32,15,130,16,43,47,5,63,5,2,5,0,64,12,0,77,10,131,19,32,18,130,8,51,30,38,20,0,12,22,
    18,38,235,2,28,12,32,235,8,0,47,237,57,57,131,3,130,241,42,17,57,43,43,1,47,93,43,225,18,57,133,5,32,57,130,20,34,49,48,93,130,0,34,1,2,35,67,233,6,35,19,18,51,50,73,233,5,32,3,130,
    9,71,207,5,131,29,8,73,6,21,20,22,51,50,3,68,168,214,165,207,216,156,236,150,164,216,166,208,216,158,226,93,131,188,109,131,136,104,173,254,239,132,188,110,131,135,106,176,1,226,254,
    237,213,172,162,231,254,239,1,17,213,172,161,232,1,133,254,252,145,119,112,148,254,250,1,6,150,118,110,146,0,2,68,33,8,38,11,0,15,0,78,64,38,24,82,33,8,60,15,7,31,7,175,7,3,13,3,7,
    15,190,64,12,18,15,0,14,10,7,1,189,4,4,5,66,14,5,4,130,223,32,179,130,242,34,4,47,43,126,243,5,45,241,57,57,225,50,0,24,63,26,253,212,95,94,93,24,82,47,19,59,17,33,53,33,4,148,254,
    108,131,254,107,1,149,131,1,148,252,84,3,172,2,140,254,108,1,148,132,1,130,22,34,252,240,133,136,139,42,203,0,7,0,11,0,82,64,11,120,1,24,73,127,8,37,184,255,192,64,16,12,24,73,126,
    7,38,10,190,9,18,3,7,8,130,133,32,182,130,133,35,8,5,1,9,68,163,7,33,9,64,130,36,54,9,47,43,43,196,50,47,43,196,196,0,63,253,198,50,47,93,43,57,57,49,48,43,24,73,142,8,32,19,131,139,
    41,108,252,164,3,92,253,126,2,130,40,131,138,45,254,1,178,58,1,225,150,254,158,4,254,199,254,106,137,139,32,205,134,139,33,14,120,24,73,71,8,35,112,6,1,1,131,142,32,13,130,105,34,6,
    6,1,131,139,33,4,0,156,139,33,212,196,130,139,32,50,133,139,33,43,93,133,139,24,73,86,7,135,139,37,2,132,253,124,3,92,132,139,8,47,2,178,254,78,152,1,53,6,1,101,149,254,31,253,20,133,
    0,0,1,0,68,0,0,4,18,5,154,0,27,0,149,64,73,15,27,1,0,20,1,16,5,1,8,11,0,77,1,4,130,25,8,40,15,19,4,1,23,15,154,18,8,5,11,154,14,64,4,1,15,18,14,18,14,9,20,0,3,9,18,3,3,7,14,9,10,12,
    66,16,12,12,10,19,0,130,10,42,19,66,19,19,5,14,23,3,9,126,10,65,64,6,54,32,10,1,10,47,93,43,225,23,57,51,47,43,1,16,77,224,17,18,57,24,47,196,131,10,44,242,194,24,47,0,63,63,196,18,
    57,57,47,47,130,4,39,26,16,237,57,57,16,253,51,131,196,41,135,5,192,16,135,192,1,43,95,94,89,70,6,40,21,33,21,33,17,35,17,33,53,132,1,92,96,8,48,4,18,254,156,1,29,254,178,1,78,254,
    178,164,254,168,1,88,130,3,53,35,254,160,192,250,28,19,4,13,36,252,5,154,253,112,139,207,139,254,219,1,37,130,6,41,2,144,253,254,58,53,42,75,1,252,130,245,8,34,166,254,116,4,24,4,0,
    0,24,0,83,64,17,121,8,1,3,20,0,137,1,1,23,138,20,16,13,6,77,20,184,255,250,183,11,130,7,40,14,10,138,11,184,255,248,64,19,130,20,48,11,12,22,15,11,8,4,17,153,6,22,0,21,47,26,1,93,130,
    188,8,33,237,50,50,47,63,196,1,47,43,225,50,47,43,43,225,57,47,225,18,57,49,48,0,93,33,35,38,53,35,6,35,34,39,35,130,180,35,51,17,20,22,94,210,5,8,68,17,20,4,24,170,23,5,84,195,167,
    71,4,163,163,138,111,121,151,166,72,96,191,123,254,16,5,140,253,142,127,150,172,145,2,74,253,9,188,0,2,0,78,255,232,4,4,5,209,0,22,0,33,0,86,64,13,119,26,1,123,2,1,5,5,18,23,11,138,
    1,66,143,7,37,1,1,35,28,137,18,75,234,7,40,18,23,1,25,154,21,16,79,5,130,38,47,3,154,8,31,154,15,19,0,63,237,47,237,50,47,93,63,130,165,131,162,33,18,57,130,167,8,37,51,18,57,25,47,
    49,48,93,93,1,51,18,33,34,7,55,54,51,50,18,17,20,2,0,35,34,38,53,16,0,51,50,19,38,35,34,2,21,131,176,8,107,18,3,94,4,26,254,220,123,123,39,119,129,194,193,130,254,242,184,166,200,1,
    39,227,178,72,58,172,160,214,113,97,151,216,3,104,1,220,80,156,65,254,248,254,221,221,254,32,254,255,213,181,1,16,1,142,254,164,209,254,198,210,122,141,1,63,0,1,0,51,254,43,4,186,5,
    154,0,11,0,50,64,27,115,10,1,116,9,1,115,8,1,6,0,0,13,8,4,10,2,4,7,145,6,3,2,11,145,0,27,130,170,40,57,63,237,57,1,47,51,198,50,130,166,32,196,131,160,8,44,93,1,33,53,1,1,53,33,21,
    33,1,1,33,4,186,251,121,2,164,253,129,4,58,252,188,2,82,253,143,3,139,254,43,78,3,115,3,84,90,154,252,240,252,212,0,130,107,36,188,254,43,5,82,130,107,49,7,0,56,64,13,0,126,1,24,13,
    6,77,1,1,9,4,126,5,70,157,6,130,7,43,248,64,11,12,6,77,5,2,145,7,3,4,131,117,39,196,63,237,1,47,43,43,225,130,115,40,43,225,49,48,1,35,17,33,17,130,3,47,5,82,168,252,186,168,4,150,
    254,43,6,213,249,43,7,111,130,93,54,190,254,26,3,148,6,2,0,21,0,61,64,10,0,14,5,138,15,15,11,66,11,15,68,173,6,131,7,48,64,15,11,6,77,15,15,23,22,2,154,19,1,13,154,8,28,130,212,37,
    63,237,17,18,1,57,130,97,74,68,6,130,98,36,38,35,34,21,17,79,96,8,8,49,53,17,52,54,51,50,23,3,148,48,63,170,187,139,75,44,48,61,172,187,138,76,44,5,92,27,217,250,214,158,188,19,147,
    26,217,5,39,160,188,18,0,2,0,82,2,186,2,176,5,176,130,129,8,48,31,0,86,64,53,118,19,1,121,6,1,22,84,0,11,16,11,32,11,3,11,11,18,0,36,2,28,84,5,37,15,48,13,17,72,15,15,13,82,18,4,11,
    2,31,194,192,0,1,208,130,191,48,33,15,15,25,194,8,47,225,57,47,16,222,93,113,225,50,50,113,254,12,33,93,237,130,153,34,93,93,1,113,254,7,39,52,55,55,52,35,34,7,53,130,155,35,22,21,
    7,7,70,19,5,8,49,54,53,2,176,129,4,79,152,109,133,248,229,151,136,118,111,155,126,142,129,180,168,77,64,82,125,2,205,100,119,119,93,202,33,31,168,86,128,70,144,130,107,25,23,111,51,
    64,122,85,0,130,181,8,42,78,2,184,3,35,5,174,0,11,0,23,0,37,64,22,18,82,0,37,12,82,6,4,21,200,144,9,176,9,192,9,3,9,25,15,200,3,47,225,16,222,113,225,130,145,32,63,131,135,74,138,10,
    32,3,111,231,7,8,39,53,52,38,1,180,167,191,205,169,163,188,199,160,106,125,119,110,105,123,121,2,184,201,172,181,204,199,171,178,210,2,136,144,127,124,140,143,123,126,143,130,233,61,
    100,0,0,5,169,5,178,0,27,0,186,64,12,121,25,1,121,17,1,16,16,12,0,77,26,184,255,240,64,39,130,8,32,12,90,225,8,52,26,24,2,16,18,12,2,12,2,4,10,27,27,24,8,13,6,77,24,8,12,130,4,37,125,
    4,184,255,244,179,130,14,130,7,33,234,179,130,17,130,7,46,220,64,56,11,6,77,32,4,48,4,2,4,4,29,10,131,44,32,10,131,44,35,10,125,15,18,131,12,32,18,131,12,33,18,18,130,35,49,18,7,153,
    21,4,2,12,14,16,4,27,153,0,18,47,29,1,93,130,217,33,23,57,66,106,5,48,43,196,225,43,43,18,57,47,93,43,43,43,241,43,43,193,47,75,151,5,33,18,57,130,7,61,49,48,0,43,43,1,43,43,0,93,93,
    33,33,53,36,17,52,0,35,34,0,21,16,1,21,33,53,5,36,17,78,138,6,8,113,5,37,5,169,253,249,1,89,254,234,219,222,254,230,1,90,253,248,1,92,254,164,1,114,1,46,1,53,1,112,254,165,1,91,147,
    255,1,122,245,1,29,254,228,245,254,134,255,0,147,148,1,235,1,147,1,57,1,104,254,155,254,197,254,110,237,1,0,3,0,90,255,232,6,86,4,24,0,35,0,42,0,53,0,175,64,33,118,25,1,113,18,1,121,
    7,1,120,31,1,120,10,1,15,64,11,0,77,127,7,1,7,7,0,131,36,131,216,8,99,36,184,255,192,64,74,11,12,0,76,36,36,55,11,20,29,42,4,1,132,43,43,55,17,32,24,1,24,24,48,131,17,16,12,6,77,47,
    17,1,17,44,20,0,149,36,36,32,4,24,64,9,12,72,24,24,22,27,22,30,39,149,32,16,48,6,64,6,80,6,3,6,6,1,11,51,14,4,149,9,22,0,63,237,57,57,50,17,57,47,93,63,237,50,57,57,47,51,47,43,17,
    83,146,5,36,1,47,93,43,225,130,23,131,14,34,225,23,57,130,20,40,43,43,225,50,47,93,49,48,0,73,195,5,32,93,113,27,10,33,32,39,116,174,15,33,50,23,104,105,5,32,39,113,44,5,33,7,53,116,
    188,8,8,49,6,86,253,43,3,176,153,177,144,132,226,254,242,113,4,145,242,153,175,1,127,1,6,215,175,155,148,200,232,68,4,116,240,202,222,168,2,133,119,116,166,19,164,203,139,137,106,90,
    119,164,113,80,6,8,80,229,229,161,135,1,44,24,16,1,42,122,164,96,200,200,254,251,232,55,144,156,164,136,200,63,12,8,82,95,73,89,169,0,3,0,49,255,174,4,150,4,102,0,19,0,27,0,35,0,196,
    182,120,28,1,119,27,1,29,184,255,240,64,41,11,12,0,76,29,20,34,25,11,8,18,1,4,3,13,19,0,0,24,72,6,15,32,131,75,38,7,37,236,179,13,6,77,34,130,53,75,68,6,34,222,64,65,75,54,6,32,7,130,
    24,42,25,10,12,6,77,25,131,9,10,13,17,130,13,33,13,12,130,13,33,13,25,130,31,32,13,75,45,6,50,1,8,11,18,4,16,10,9,6,0,19,22,149,16,16,31,149,6,22,75,50,8,37,206,50,16,206,50,18,75,
    55,5,38,1,47,43,43,43,206,50,66,135,5,130,10,33,93,241,130,4,34,202,47,50,24,94,212,7,49,49,48,0,43,93,93,1,7,22,21,20,0,35,34,39,7,39,55,70,26,5,39,23,55,1,38,35,34,6,21,75,42,6,8,
    108,54,53,52,4,150,168,98,254,234,232,192,126,145,82,149,102,1,28,240,184,124,162,254,243,79,136,159,183,50,2,65,254,4,87,134,163,171,4,24,180,138,208,252,254,218,102,160,78,164,137,
    203,1,2,1,34,98,176,254,222,74,215,189,132,92,1,204,253,216,76,208,190,134,0,0,2,0,143,254,82,3,61,4,22,0,35,0,51,0,53,64,29,121,18,1,17,15,149,20,0,44,176,36,16,32,239,3,48,175,112,
    24,81,230,8,34,52,17,47,24,81,230,14,39,47,253,198,49,48,93,1,22,130,174,37,14,4,21,20,30,2,115,54,6,44,46,2,53,52,62,4,53,52,38,39,19,50,23,130,28,33,7,6,130,204,38,38,53,52,55,54,
    2,113,24,81,230,16,33,100,59,24,81,230,18,33,2,118,24,81,230,30,45,1,160,32,31,47,46,32,33,33,32,46,47,31,32,130,199,36,180,254,106,1,146,130,199,8,51,11,0,15,0,49,64,16,13,12,14,15,
    255,58,12,125,13,13,9,175,127,3,1,3,184,255,232,64,10,13,6,77,3,14,0,176,6,16,12,0,47,63,253,206,1,47,43,93,225,57,47,225,43,84,212,13,54,19,35,19,51,1,35,46,65,65,46,45,66,66,43,172,
    19,134,3,58,64,46,46,64,131,3,61,251,48,4,6,0,1,0,232,0,254,4,148,3,49,0,5,0,37,178,1,189,0,184,255,200,180,12,6,77,0,130,100,33,224,183,130,8,40,3,0,3,190,4,0,47,253,196,130,96,38,
    47,43,225,49,48,37,35,75,40,5,40,132,252,216,3,172,254,1,174,133,130,67,8,58,123,0,0,4,90,5,236,0,8,0,53,64,28,122,3,1,123,2,1,121,1,1,1,2,0,3,3,8,0,4,7,1,4,217,5,5,1,0,0,1,18,0,63,
    63,17,57,47,237,17,51,1,47,47,51,57,47,18,57,57,71,162,5,8,32,1,35,1,35,53,33,1,1,4,90,254,123,113,254,193,170,1,8,1,19,1,78,5,236,250,20,2,238,120,253,107,5,27,130,60,8,35,0,94,254,
    234,3,184,5,154,0,28,0,113,64,18,121,19,1,121,17,1,119,9,1,119,3,1,10,48,12,0,77,4,184,255,240,64,131,8,8,49,15,6,1,6,6,4,8,5,4,3,9,19,22,23,9,4,18,16,20,48,20,64,20,3,20,20,18,18,
    13,28,13,8,5,19,149,22,22,29,2,149,26,3,16,149,11,0,47,237,63,237,68,49,7,130,143,38,17,51,47,93,17,23,51,131,2,130,9,38,49,48,43,0,43,1,93,130,0,43,1,38,35,34,7,7,51,21,35,3,2,33,
    86,211,5,8,91,19,19,35,53,51,55,54,54,51,50,23,3,184,48,60,146,24,31,166,187,90,58,254,222,31,59,36,50,150,40,91,103,124,28,17,175,134,64,52,4,242,28,160,208,138,253,120,254,94,16,
    143,21,1,36,2,124,138,219,131,158,19,0,2,0,209,0,240,4,170,3,192,0,20,0,41,0,122,185,0,22,255,224,179,9,12,72,1,184,255,224,64,43,130,8,33,32,32,98,56,7,43,30,38,190,24,28,190,21,0,
    34,1,34,13,98,68,7,62,0,13,160,13,176,13,3,12,3,13,184,255,192,64,20,14,17,72,13,41,21,20,189,0,31,30,9,189,10,42,43,98,90,8,44,57,57,16,225,57,57,0,24,47,43,95,94,93,98,98,5,35,16,
    212,93,196,98,106,8,98,108,22,32,19,98,129,41,39,135,7,147,126,109,156,94,65,98,150,12,58,3,192,146,162,108,68,176,144,164,88,90,98,80,254,101,147,162,108,66,174,143,166,88,91,100,
    79,0,130,249,8,52,51,0,0,4,248,5,154,0,5,0,13,0,51,64,28,116,13,1,116,12,1,123,7,1,123,6,1,13,6,2,5,5,15,2,9,3,3,5,2,13,153,1,18,0,63,237,57,57,63,51,1,47,18,66,44,9,8,47,93,33,33,
    53,1,51,1,39,1,38,39,35,6,7,1,4,248,251,59,2,9,174,2,14,215,254,151,33,5,4,6,34,254,160,76,5,78,250,178,76,3,200,88,54,48,94,252,56,130,113,38,92,0,200,3,180,3,112,130,113,40,11,0,
    43,64,21,6,10,0,4,130,131,8,36,240,5,2,8,128,10,10,6,240,11,15,8,1,8,47,93,51,241,192,47,26,16,220,50,241,194,47,0,47,198,57,57,49,48,37,35,1,130,105,32,3,133,5,42,178,168,254,213,
    1,43,170,254,205,84,166,131,8,43,166,254,209,200,1,80,1,88,254,168,254,176,133,7,131,219,32,90,140,105,51,23,7,11,2,4,3,0,240,4,2,128,9,6,240,10,80,8,112,8,2,123,71,5,37,93,196,225,
    50,26,220,130,4,34,0,47,196,131,105,33,1,1,131,100,32,3,133,5,59,180,254,211,165,1,46,254,210,165,88,254,213,170,1,52,254,204,170,2,26,254,174,1,82,1,86,254,170,133,7,60,0,3,0,140,
    255,238,5,82,0,205,0,11,0,23,0,35,0,79,178,3,175,9,184,255,253,179,12,6,77,130,7,35,251,64,35,11,130,8,63,14,21,15,27,66,33,175,27,27,36,21,175,15,15,37,36,12,18,24,30,4,6,176,0,19,
    64,37,1,47,37,1,93,72,81,5,38,17,18,1,57,47,225,18,130,3,32,43,114,19,6,34,49,48,5,83,212,22,83,224,11,45,4,228,46,63,63,46,45,65,65,253,221,46,65,65,134,8,33,62,62,130,8,38,64,18,
    64,46,46,67,67,68,104,5,140,7,32,0,100,163,7,33,7,102,100,163,8,36,67,1,31,1,92,130,147,41,11,2,35,17,16,6,7,62,2,17,98,89,11,38,22,0,0,5,18,7,11,136,45,46,216,1,45,1,89,0,22,185,0,
    2,255,253,64,9,26,130,48,34,37,2,26,135,48,130,93,38,94,255,232,5,170,7,10,98,137,8,36,216,1,171,1,88,132,93,94,125,7,136,93,34,0,2,0,130,45,62,7,18,5,178,0,24,0,35,0,169,64,29,6,24,
    11,0,77,20,17,0,0,37,19,23,8,13,6,77,23,8,12,131,4,32,11,130,4,37,126,25,184,255,208,179,130,19,130,7,33,240,179,130,22,130,7,34,216,64,70,130,26,40,0,25,1,11,3,25,25,37,31,131,49,
    32,31,131,49,33,31,8,130,22,35,31,125,8,16,130,46,33,8,16,130,43,33,8,24,130,16,61,8,21,145,64,20,16,0,20,145,43,48,28,145,11,4,17,145,16,3,34,145,5,19,24,145,0,18,0,63,237,133,1,32,
    43,103,113,5,32,1,82,141,10,35,95,94,93,43,83,210,5,48,50,18,57,47,196,196,49,48,43,33,33,34,7,6,35,32,0,82,130,5,36,22,51,33,21,33,83,205,5,36,37,17,38,35,34,131,22,8,96,51,50,7,18,
    253,48,16,122,98,84,254,200,254,148,1,123,1,69,90,88,112,2,2,176,253,208,2,6,253,250,2,80,253,8,160,108,229,254,229,1,23,227,116,14,10,1,146,1,66,1,90,1,156,10,14,152,254,35,151,254,
    10,8,4,94,28,254,180,254,253,254,254,254,182,0,3,0,96,255,232,7,33,4,24,0,28,0,35,0,47,0,225,179,120,7,1,11,184,83,232,5,32,2,130,7,50,64,13,12,0,77,6,64,15,18,72,6,6,28,131,29,184,
    255,192,179,130,16,130,7,36,208,179,12,1,77,24,86,20,10,39,238,64,26,11,6,77,29,64,130,29,43,32,29,1,29,29,49,11,23,35,3,1,8,130,19,38,1,132,45,184,255,248,179,130,9,130,7,34,192,64,
    69,130,33,42,45,45,49,39,10,13,6,77,39,10,12,131,4,130,26,35,39,131,17,16,130,16,33,17,13,130,16,33,17,31,130,16,32,17,74,14,9,46,4,0,149,29,29,4,23,36,20,32,149,25,16,11,42,74,27,
    10,41,63,237,57,57,50,18,57,47,237,17,130,4,32,93,65,87,11,37,43,43,225,43,23,57,75,67,6,42,43,43,241,192,47,43,49,48,0,43,43,74,23,14,44,33,34,0,53,16,0,51,32,23,51,54,33,50,123,61,
    8,32,1,76,81,10,8,48,7,33,253,43,3,177,154,175,144,136,221,254,242,112,4,131,254,227,229,254,243,1,28,240,1,38,108,4,119,1,10,196,218,168,2,134,116,120,167,16,254,16,158,184,184,156,
    159,173,173,74,16,6,8,47,237,237,1,30,236,1,2,1,36,235,235,254,249,226,51,142,158,171,129,1,44,217,191,180,208,205,189,194,208,0,1,0,0,1,250,4,0,2,123,0,3,0,13,180,1,235,2,0,24,64,
    71,11,36,4,0,252,0,4,130,32,32,129,133,39,32,8,151,39,36,8,0,248,0,8,132,39,40,2,0,106,3,244,2,156,5,178,130,79,8,32,7,0,35,64,19,3,2,6,179,64,7,4,0,180,2,4,180,128,16,6,32,6,2,6,47,
    93,26,233,220,233,0,63,26,87,216,5,35,3,35,19,35,130,3,46,2,156,111,147,141,187,108,150,140,5,178,254,66,1,190,131,3,35,0,2,0,110,130,77,32,160,179,77,39,160,140,118,108,154,140,118,
    111,138,77,32,1,131,155,33,1,108,132,77,38,24,64,12,2,179,64,3,130,151,36,128,175,2,1,2,131,146,131,144,34,49,48,1,130,138,33,1,108,136,134,32,0,130,249,130,131,33,1,112,132,53,34,
    20,64,9,135,53,34,2,47,26,132,194,134,49,32,112,136,106,32,0,130,249,8,84,232,0,112,4,148,4,58,0,10,0,14,0,25,0,73,64,32,15,214,21,12,190,13,64,13,0,66,6,214,64,0,15,13,1,13,3,13,11,
    14,23,18,12,66,12,8,3,23,211,18,184,255,224,179,12,6,77,18,47,43,225,57,57,198,43,1,16,77,226,0,24,47,95,94,93,214,26,237,43,0,26,24,16,77,253,214,78,109,10,39,21,20,6,1,33,53,33,1,
    69,62,6,130,14,8,77,2,196,44,60,58,46,103,61,1,166,252,84,3,172,254,44,44,62,60,46,101,58,3,100,61,44,45,64,107,47,60,254,174,134,253,216,60,43,45,66,109,45,60,0,2,0,108,0,0,4,64,5,
    154,0,5,0,9,0,40,64,17,3,8,6,0,0,1,5,3,1,18,8,6,3,0,0,11,3,71,8,6,43,0,63,63,18,57,61,47,24,196,196,61,196,130,231,8,32,1,35,1,1,51,9,3,4,64,254,59,76,254,61,1,195,76,1,63,254,156,
    254,156,1,100,2,205,253,51,2,203,2,207,130,5,50,63,253,193,253,191,255,255,0,14,254,30,3,213,5,135,2,38,0,92,130,90,37,6,0,143,79,0,0,103,113,5,39,36,30,12,0,37,2,1,33,98,225,10,130,
    47,38,18,0,0,4,96,6,226,130,47,32,60,130,47,39,7,0,143,0,149,1,91,0,100,239,5,37,252,64,10,29,23,5,131,52,103,166,11,38,1,254,200,0,0,2,178,130,197,43,3,0,14,181,0,3,2,18,0,2,47,47,
    130,178,133,169,42,2,178,252,166,144,3,91,5,154,250,102,130,34,8,33,0,1,0,102,255,232,3,246,5,178,0,37,0,160,64,10,117,19,1,121,0,1,117,34,1,15,184,255,232,64,81,11,0,77,96,213,5,8,
    71,19,20,0,37,33,30,4,154,64,7,128,28,25,11,154,47,14,127,14,159,14,175,14,4,14,14,17,35,52,20,68,20,2,20,20,22,154,17,7,52,37,68,37,2,37,37,35,154,2,25,31,26,20,0,0,39,25,28,30,33,
    4,29,138,9,12,6,4,7,11,14,4,9,68,103,6,48,9,47,43,23,51,222,196,16,225,23,50,18,57,47,212,214,196,24,86,130,8,32,47,92,163,6,40,57,57,26,221,26,237,57,57,17,24,89,113,8,46,0,93,93,
    37,6,35,32,3,35,53,51,38,53,52,55,130,6,32,54,24,65,37,8,37,7,33,21,33,6,23,130,4,8,83,18,33,50,55,3,246,133,149,254,85,82,121,108,4,6,110,131,51,1,27,202,137,108,114,142,125,195,39,
    1,241,253,249,8,7,2,8,254,8,72,1,22,131,141,50,74,1,250,140,31,46,48,60,140,240,1,15,56,162,78,201,170,140,99,86,140,254,146,87,0,1,0,92,0,200,2,47,3,112,0,5,0,28,64,14,72,173,6,42,
    32,2,1,15,2,1,2,47,93,93,51,72,162,5,72,160,7,41,2,45,166,254,213,1,43,168,254,207,72,145,6,32,0,142,65,32,15,72,133,5,36,80,2,112,2,2,134,66,37,196,225,50,0,47,198,65,145,5,43,1,51,
    2,47,254,211,166,1,48,254,208,166,72,105,7,8,57,0,3,0,53,0,0,3,234,6,2,0,20,0,32,0,36,0,160,185,0,17,255,232,64,53,11,0,77,27,99,21,35,2,149,18,1,5,8,35,3,11,149,14,15,9,33,21,30,98,
    24,33,8,13,6,77,33,8,12,131,4,32,11,130,4,33,132,34,131,16,32,34,131,16,37,34,184,255,241,64,30,130,20,41,34,34,38,10,6,0,12,14,5,9,24,64,23,34,32,244,24,78,222,12,38,57,57,198,212,
    198,17,18,90,223,8,45,212,225,0,63,196,63,237,23,57,63,237,16,214,237,24,64,21,23,32,23,72,199,10,39,19,35,17,51,2,160,48,61,24,64,37,9,42,221,45,62,62,45,45,64,64,35,164,164,24,64,
    48,14,36,236,60,46,45,62,125,201,9,45,0,53,0,0,3,207,6,2,0,20,0,24,0,153,65,9,5,37,49,11,0,77,8,5,131,255,41,2,149,18,1,24,0,9,21,21,21,131,213,38,21,8,12,6,77,21,8,130,237,37,21,132,
    112,22,1,22,131,19,32,22,131,19,32,22,65,5,7,34,22,22,26,65,5,67,46,93,225,43,43,43,0,63,50,63,63,237,63,237,57,57,65,2,23,32,1,144,246,35,1,47,162,162,142,239,36,250,16,5,236,0,102,
    207,10,8,36,21,0,77,64,39,21,16,20,19,18,2,14,6,11,9,8,7,3,16,13,32,13,2,13,21,6,145,5,0,5,17,10,145,11,16,11,5,130,1,33,3,13,102,225,6,39,57,47,47,17,51,16,237,50,132,4,35,1,47,93,
    51,130,0,35,206,50,221,50,130,0,45,206,50,49,48,1,37,19,35,19,5,53,5,55,39,102,247,6,49,21,37,7,23,37,2,216,254,210,40,156,40,254,210,1,6,58,58,103,2,8,56,254,250,58,58,1,6,2,7,20,
    254,202,1,54,20,150,30,192,192,30,150,20,1,54,254,202,133,11,130,165,58,112,1,254,1,80,2,223,0,11,0,19,64,9,0,176,6,9,175,3,47,13,1,93,47,225,0,47,127,161,14,42,223,46,65,65,46,47,
    66,66,1,254,66,131,6,34,47,46,66,131,229,55,88,255,28,1,90,0,217,0,3,0,44,64,28,0,16,16,0,77,0,16,12,0,77,3,131,9,32,3,131,9,45,2,179,64,3,0,180,128,2,47,26,233,0,47,26,130,83,33,1,
    43,130,0,48,37,3,35,19,1,90,141,117,110,217,254,67,1,189,0,2,0,130,71,33,2,137,132,71,37,7,0,77,64,53,4,131,63,32,4,131,63,32,7,131,9,32,7,131,9,147,93,71,121,5,71,120,5,32,6,130,98,
    32,220,132,100,35,57,57,49,48,132,102,135,106,32,35,130,110,37,2,137,139,119,109,154,135,114,132,118,41,0,7,0,80,255,234,9,94,5,176,130,255,24,114,31,7,50,63,0,75,0,232,64,46,24,3,
    26,64,82,58,14,70,82,52,28,40,24,114,43,7,33,27,3,24,114,45,9,51,26,24,26,24,61,3,55,189,67,16,13,6,77,67,184,255,244,64,10,11,130,8,37,73,200,31,61,1,61,84,172,6,132,7,34,12,6,77,
    130,7,34,238,64,14,130,30,37,61,61,3,31,189,43,131,49,32,43,130,34,32,182,130,17,34,43,49,194,24,114,90,9,32,28,130,43,33,37,7,130,21,38,37,37,77,3,9,189,21,24,114,90,8,32,12,130,19,
    34,21,15,200,24,114,90,40,33,57,47,130,249,60,93,225,212,43,43,225,17,18,57,57,47,47,0,63,237,212,237,43,0,24,63,63,26,77,237,244,237,16,212,130,4,33,47,63,80,224,13,85,218,11,35,37,
    1,35,1,71,121,7,35,22,21,20,6,139,27,152,23,32,148,24,114,131,15,56,89,252,102,143,3,153,3,66,148,176,189,155,153,169,188,140,96,108,107,93,95,106,106,252,121,24,114,149,43,53,177,
    208,191,174,171,207,2,118,141,123,120,134,141,123,121,133,253,138,196,162,178,207,24,114,169,14,77,11,8,32,105,76,221,8,36,215,1,15,1,91,76,221,5,41,241,64,9,18,18,12,11,37,2,20,76,
    176,7,130,47,38,188,0,0,3,180,7,107,130,47,42,40,0,0,1,7,0,215,0,171,1,93,109,9,5,41,245,64,9,14,14,2,3,37,1,16,138,47,77,61,5,77,107,9,36,142,1,87,1,92,133,95,41,250,64,9,17,19,6,
    7,37,2,18,143,95,33,6,227,136,95,34,143,0,139,130,47,45,23,64,13,2,1,5,33,15,2,3,62,2,1,24,111,145,13,133,145,131,97,133,145,34,67,0,232,133,97,38,1,255,233,64,9,15,13,131,145,32,13,
    138,97,32,48,130,183,33,253,7,131,47,32,44,130,9,36,7,0,142,255,241,135,47,32,167,130,193,35,13,9,2,37,112,136,9,39,255,255,255,252,0,0,2,48,132,241,133,47,34,215,255,160,130,241,38,
    19,64,11,1,6,16,12,131,44,136,238,32,0,130,45,32,241,130,45,32,56,132,191,133,45,34,143,255,116,135,191,34,4,27,21,130,46,144,191,32,27,130,133,32,240,138,143,34,67,255,201,78,141,
    5,34,1,80,13,132,95,136,188,130,95,32,0,78,93,5,131,189,112,231,5,34,142,1,248,133,45,38,2,30,25,27,12,6,62,78,138,9,137,45,32,106,78,139,8,34,215,1,151,134,45,39,5,26,26,3,9,37,2,
    28,113,67,16,132,137,133,91,34,67,1,153,134,45,34,44,25,24,131,45,32,25,139,45,38,170,255,232,4,213,7,101,130,91,32,56,65,71,5,45,1,180,1,91,0,19,64,11,1,43,15,17,5,13,66,6,10,131,
    137,133,45,32,107,136,45,34,215,1,75,65,69,6,34,0,16,16,131,45,66,4,8,137,45,131,229,133,91,34,67,1,103,65,163,7,36,209,64,9,17,15,131,48,32,15,136,140,34,1,0,166,130,129,51,74,4,0,
    0,3,0,49,64,22,3,15,0,21,0,6,13,6,77,0,6,24,95,74,8,37,132,1,184,255,254,182,130,14,40,1,1,5,4,17,18,57,47,43,70,187,5,32,63,24,67,88,9,130,62,8,36,0,1,0,92,4,194,2,144,6,14,0,6,0,
    28,179,0,0,8,4,184,255,192,182,16,0,77,4,5,128,3,0,47,26,205,1,47,43,130,60,55,49,48,1,35,39,7,35,19,51,2,144,122,164,165,113,215,133,4,194,230,230,1,76,0,130,63,56,35,4,178,2,164,
    5,178,0,19,0,58,64,12,122,12,1,118,2,1,12,64,12,0,77,2,130,72,33,64,19,130,8,57,19,196,0,0,21,9,196,10,19,7,218,13,17,218,9,3,0,47,198,253,220,237,198,1,47,233,130,88,8,55,233,49,48,
    43,43,93,93,1,20,6,35,34,39,38,35,34,21,35,52,54,51,50,23,22,51,50,53,2,164,96,82,71,91,77,42,88,94,96,86,65,84,74,54,84,5,178,105,133,52,43,113,107,129,52,44,116,131,121,32,62,130,
    185,50,58,5,47,0,3,0,14,180,0,1,2,226,1,0,47,237,1,47,47,130,171,43,33,53,33,2,58,254,4,1,252,4,194,109,131,41,32,37,130,41,43,74,5,190,0,12,0,37,64,18,12,196,47,130,182,43,0,14,7,
    196,64,6,12,6,128,9,219,3,130,55,37,26,220,196,1,47,26,131,145,32,93,130,146,33,1,6,130,142,34,38,39,51,130,134,56,54,55,2,74,10,157,112,117,148,5,100,12,160,68,97,10,5,190,113,139,
    138,114,155,84,71,131,87,49,108,4,194,1,49,5,133,0,11,0,15,181,9,192,3,6,193,0,132,130,32,225,24,70,179,13,60,207,40,59,58,41,41,57,57,4,194,57,42,40,56,55,41,41,58,0,0,2,0,82,4,152,
    1,238,6,28,130,59,8,40,22,0,60,64,37,116,11,1,116,7,1,123,5,1,123,1,1,20,196,64,48,9,128,9,208,9,224,9,4,9,192,15,196,3,12,197,64,6,192,17,197,131,93,34,26,220,26,130,228,36,233,26,
    220,113,26,130,168,32,93,130,0,69,103,16,8,81,51,50,54,53,52,38,1,28,87,115,118,91,89,114,122,88,48,64,112,50,66,66,4,152,105,85,90,108,108,84,84,112,1,51,65,48,112,64,48,49,64,0,1,
    0,51,254,80,1,135,0,0,0,18,0,76,64,9,115,18,1,118,1,1,9,194,0,184,255,192,64,11,11,15,72,0,16,5,13,7,197,64,2,131,15,63,22,12,15,72,2,128,15,11,31,11,47,11,3,11,195,16,64,10,14,72,
    16,14,18,0,63,213,43,237,93,26,220,43,131,150,35,206,205,212,43,130,252,36,93,93,1,20,33,85,179,5,8,64,53,52,35,34,7,53,51,21,50,22,1,135,254,237,38,27,43,27,141,129,10,31,108,88,103,
    255,0,176,2,90,6,90,78,4,182,104,80,0,2,0,64,4,194,2,135,6,0,0,3,0,7,0,78,64,48,0,14,2,40,13,6,77,2,24,12,131,4,32,11,130,4,33,48,10,131,4,8,43,9,6,77,15,2,1,16,3,2,6,128,6,4,66,4,
    112,6,1,6,3,2,7,128,6,0,47,26,205,57,57,1,47,93,205,43,1,26,24,16,220,95,94,93,43,131,0,37,77,233,49,48,1,3,79,84,6,43,135,245,97,209,114,240,96,206,6,0,254,194,102,89,5,60,0,0,1,0,
    62,254,112,1,82,0,0,0,14,0,29,64,14,123,11,1,0,6,6,2,194,12,4,226,9,131,219,46,47,237,1,47,241,194,47,198,49,48,93,33,6,21,20,88,160,6,56,38,53,52,55,1,82,150,68,30,26,30,52,76,92,
    157,141,82,70,13,107,13,80,70,122,128,131,79,32,102,130,201,34,152,6,12,67,79,7,32,3,67,79,6,35,3,3,128,2,67,79,6,34,17,57,47,132,149,51,3,51,23,55,2,152,214,132,216,115,164,164,6,
    12,254,182,1,74,229,229,131,65,8,39,33,0,0,3,164,5,154,0,13,0,189,64,20,121,10,1,118,4,1,11,2,3,10,3,8,5,4,9,4,10,10,12,0,184,255,240,179,16,6,77,132,7,32,13,134,7,32,12,134,7,32,11,
    132,7,34,232,179,10,132,7,34,224,179,9,132,7,40,192,64,27,10,31,72,0,0,15,130,68,24,68,80,15,37,126,1,184,255,248,179,130,71,130,7,34,242,64,23,130,72,48,1,6,11,6,77,1,4,3,10,9,3,9,
    3,0,6,3,13,83,47,6,40,18,57,57,47,47,51,17,51,1,83,43,7,34,57,57,198,76,30,5,32,43,130,0,130,9,8,72,49,48,16,135,4,192,192,16,135,192,192,0,93,93,33,33,17,7,53,55,17,51,17,37,21,5,
    17,33,3,164,253,24,155,155,168,1,72,254,184,2,64,2,35,97,152,96,2,224,253,137,203,154,200,254,12,0,1,0,22,0,0,2,18,5,236,0,11,0,98,64,60,10,7,130,128,35,4,1,5,0,130,20,34,5,10,7,24,
    68,210,16,36,112,3,1,3,8,24,80,53,8,49,6,5,0,11,11,5,2,9,0,2,21,64,13,1,93,0,63,63,130,167,130,117,132,166,32,93,103,152,5,37,198,16,194,47,49,48,135,155,35,1,7,17,35,134,154,8,74,
    55,2,18,174,164,170,170,164,174,3,43,123,253,80,2,66,119,157,119,3,13,253,98,122,0,255,255,0,121,255,232,3,222,7,106,2,38,0,54,0,0,1,7,0,224,0,162,1,94,0,20,180,1,56,5,38,1,184,255,
    244,180,57,54,19,44,37,43,53,0,43,53,255,255,0,104,130,45,34,15,6,12,130,45,32,86,130,45,36,6,0,224,18,0,131,43,33,49,17,131,43,37,236,180,50,47,15,22,136,43,38,33,0,0,4,100,7,103,
    130,43,32,61,130,43,131,89,32,236,70,73,6,39,5,15,15,8,9,37,1,12,69,234,8,130,89,130,45,33,3,112,132,89,32,93,133,89,39,118,0,0,19,64,11,1,14,134,43,32,17,119,127,7,35,0,2,0,172,123,
    235,8,54,7,0,34,64,15,1,0,4,202,5,0,14,64,3,4,7,66,7,4,28,0,63,205,84,238,8,41,225,57,57,49,48,1,35,17,51,17,130,3,34,1,64,148,130,0,39,2,203,3,83,248,0,3,84,130,71,49,28,0,0,5,62,
    5,154,0,11,0,23,0,160,64,49,0,7,1,130,2,8,32,16,5,16,13,1,145,64,4,5,0,4,145,43,48,12,145,5,3,17,145,0,18,15,15,0,8,8,13,6,77,8,8,12,131,4,32,11,130,4,33,125,21,24,96,118,19,32,27,
    130,26,37,21,21,25,2,13,4,24,73,104,15,24,104,171,17,110,223,6,34,57,57,206,78,127,9,130,9,125,35,12,131,191,36,95,94,93,0,93,130,194,45,53,51,17,33,32,17,16,0,33,3,17,33,21,33,24,
    104,186,7,33,160,160,24,104,188,8,8,35,1,80,254,176,214,1,26,1,58,253,182,2,129,155,2,126,253,69,254,178,254,111,5,2,254,26,155,254,23,1,46,1,21,2,39,0,130,247,63,96,255,234,4,24,5,
    244,0,26,0,37,0,231,64,17,119,14,1,11,6,5,12,5,14,14,15,3,3,2,4,13,6,131,215,130,198,36,3,184,255,192,179,135,7,34,10,6,77,131,15,8,32,64,65,9,6,77,5,4,48,9,18,72,4,4,25,13,12,9,0,
    1,27,149,25,16,33,149,19,22,14,8,13,6,77,11,131,4,44,13,13,16,5,1,36,15,8,1,8,8,22,16,130,79,36,77,16,4,12,6,130,4,32,11,130,4,33,131,36,65,53,6,130,7,34,227,64,42,130,18,36,36,36,
    39,30,10,130,58,33,30,10,130,36,33,30,10,130,17,35,30,131,22,16,130,16,33,22,12,130,16,33,22,21,130,16,36,22,47,39,1,93,86,187,10,86,183,5,40,18,57,47,93,18,57,198,17,57,130,25,40,
    0,63,237,63,237,50,63,198,50,130,20,35,43,51,1,43,130,0,8,32,49,48,135,8,192,8,192,16,135,14,192,192,1,93,1,55,38,39,5,39,37,38,39,51,22,23,37,23,5,0,17,20,0,24,80,187,7,32,7,85,116,
    8,8,75,16,3,18,4,90,108,254,213,51,1,14,118,146,221,75,97,1,33,55,254,248,1,71,254,254,222,222,250,1,4,220,123,117,148,170,166,148,139,163,3,200,3,162,104,151,98,131,105,96,52,84,144,
    94,130,254,205,254,45,240,254,204,1,24,242,243,1,47,136,217,187,181,211,216,180,1,144,82,227,7,33,7,102,82,227,8,34,142,1,51,73,121,5,37,1,48,15,17,5,0,73,75,14,38,14,254,30,3,213,
    6,10,83,65,6,37,7,0,142,0,238,0,67,3,5,35,51,22,24,12,130,45,32,23,67,3,11,8,53,188,0,0,4,41,5,154,0,12,0,20,0,108,64,24,119,8,1,13,145,48,5,64,5,2,5,14,145,12,5,12,3,2,18,3,3,9,125,
    18,184,255,240,64,26,13,6,77,18,18,22,5,14,1,8,130,9,36,1,8,12,6,77,87,30,5,33,126,2,69,20,6,130,7,34,244,64,9,24,98,32,17,37,50,18,57,47,43,233,68,108,5,50,47,237,47,93,237,49,48,
    93,1,17,35,17,51,21,51,50,22,21,20,24,98,32,12,8,39,168,226,232,251,254,229,240,186,176,173,184,254,176,1,62,254,194,5,154,226,223,205,204,254,254,2,227,253,180,156,145,1,31,0,0,2,
    0,166,254,41,24,85,107,15,39,27,1,21,149,15,22,4,0,24,74,7,131,32,17,24,74,7,47,35,7,195,253,96,24,74,8,18,56,0,1,0,232,2,18,4,148,2,152,0,3,0,49,64,10,1,190,64,15,2,1,13,3,2,110,90,
    7,103,38,8,34,4,5,1,110,90,6,85,254,5,34,26,77,237,73,218,5,32,4,106,40,5,8,42,18,134,0,1,1,12,0,164,4,110,4,6,0,11,0,29,182,11,79,5,1,5,2,8,184,255,216,179,12,6,77,8,25,47,43,24,196,
    0,25,47,93,24,196,130,55,59,7,1,1,39,1,1,55,1,1,23,1,4,110,94,254,172,254,174,94,1,84,254,172,94,1,82,1,84,130,14,33,1,2,141,14,131,19,32,0,130,171,8,65,127,2,72,2,127,5,176,0,9,0,
    57,64,30,4,32,17,27,72,4,5,7,3,9,231,64,0,40,7,39,9,14,8,205,3,3,1,66,5,1,3,3,11,10,17,18,57,47,221,198,43,1,16,77,241,225,0,24,63,63,26,237,50,16,205,57,43,132,179,57,51,17,7,53,37,
    17,51,2,127,254,8,185,193,1,67,189,2,72,112,2,82,55,125,96,253,8,130,99,32,104,130,99,8,41,146,5,174,0,22,0,54,64,29,116,13,1,118,10,1,118,15,1,2,22,230,0,40,11,9,230,14,39,6,204,17,
    22,22,24,12,21,20,2,47,51,51,198,130,101,40,214,225,0,63,253,198,63,237,50,130,91,39,93,0,93,93,1,33,53,55,115,216,6,101,249,6,8,41,20,7,7,21,33,2,146,253,214,240,104,56,76,62,118,
    98,84,151,118,138,205,164,1,130,2,72,114,236,102,109,55,58,73,102,149,76,132,108,153,195,156,4,130,127,36,129,2,51,2,142,130,127,8,46,32,0,70,64,39,118,22,1,25,9,230,10,10,3,20,111,
    17,127,17,2,17,15,230,20,39,1,3,230,31,41,25,10,13,205,23,23,28,204,6,6,34,10,17,0,47,196,196,130,136,37,241,192,47,225,17,57,132,140,34,253,198,93,100,7,5,35,49,48,93,19,116,111,6,
    36,35,35,53,51,50,74,1,5,39,54,51,50,22,21,20,7,21,130,4,8,87,6,35,34,129,103,114,80,92,219,84,80,197,144,94,92,87,126,113,134,145,177,170,142,137,2,104,142,76,75,64,141,118,134,119,
    69,139,51,119,96,151,54,4,42,160,117,148,255,255,0,127,0,0,6,212,5,176,0,38,0,240,0,0,0,39,0,188,2,191,0,0,1,7,0,241,4,66,253,185,0,7,178,2,15,44,0,63,53,0,134,41,32,170,146,41,42,
    246,4,0,253,184,0,9,179,3,2,17,131,42,132,43,32,129,130,85,32,235,130,245,34,38,0,242,133,85,34,3,6,0,131,85,34,246,4,65,134,43,32,40,133,43,8,67,0,2,0,47,2,72,2,170,5,154,0,10,0,18,
    0,64,64,35,120,7,1,11,7,4,6,15,3,9,231,1,1,2,7,3,2,40,0,0,7,8,11,17,3,2,205,112,3,1,3,3,20,15,6,47,51,18,57,47,93,241,23,57,51,202,47,0,63,63,130,12,8,62,237,23,57,17,51,49,48,1,93,
    1,35,21,35,53,33,53,1,51,17,51,3,35,6,7,3,33,17,52,2,170,110,132,254,119,1,113,156,110,238,4,12,26,218,1,0,3,20,204,204,92,2,42,253,231,1,155,31,42,254,174,1,67,32,130,133,8,45,100,
    1,2,4,14,4,170,0,27,0,39,0,93,64,53,2,6,9,13,16,20,23,27,8,4,18,1,7,4,190,34,14,28,190,18,21,15,18,22,0,0,25,14,8,11,64,136,30,46,11,25,189,37,11,37,66,37,37,41,31,189,11,47,225,130,
    138,50,43,1,16,77,225,17,23,57,26,24,16,222,196,16,202,47,196,0,47,130,7,40,237,244,253,222,196,17,18,23,57,130,164,34,7,39,6,100,64,7,47,52,55,39,55,23,54,51,50,23,55,23,7,22,21,20,
    7,91,135,11,44,4,14,92,137,105,134,136,104,138,92,138,76,76,130,4,52,110,130,129,108,139,92,137,78,78,254,181,116,159,161,114,113,163,166,1,98,96,131,26,55,96,134,113,124,129,108,136,
    96,137,77,77,137,96,136,112,125,121,116,2,2,162,113,113,162,131,3,49,0,16,0,102,0,90,4,91,4,78,0,7,0,15,0,23,0,31,130,223,8,51,47,0,55,0,63,0,71,0,79,0,87,0,95,0,103,0,111,0,119,0,
    127,0,200,64,99,104,108,80,84,84,92,88,120,124,56,60,112,116,40,44,96,100,36,76,72,12,8,68,64,4,0,8,130,1,8,42,28,32,36,60,44,36,36,44,60,3,88,28,52,48,20,16,16,24,28,78,74,118,114,
    114,98,102,70,66,126,122,54,50,110,106,18,22,82,86,2,6,58,62,86,130,1,8,45,34,94,90,30,26,90,122,106,90,90,106,122,3,102,34,10,14,42,46,46,38,34,102,0,47,222,205,57,47,205,212,205,
    17,18,23,57,47,47,47,16,212,205,16,205,17,57,130,10,32,16,130,20,141,3,132,39,33,1,47,133,47,33,16,206,133,47,142,44,131,62,135,3,132,47,42,49,48,1,20,35,34,53,52,51,50,39,134,7,32,
    19,134,7,32,23,134,7,135,31,32,7,142,15,135,23,151,31,143,47,32,37,142,95,135,111,38,3,212,48,52,52,48,148,131,4,37,247,51,49,51,49,36,131,9,34,254,55,50,130,0,38,173,51,49,49,51,2,
    82,131,5,33,253,26,130,15,34,50,2,131,136,43,37,253,174,48,52,50,50,132,44,33,1,200,131,27,33,254,92,131,5,130,66,34,49,51,148,131,9,33,3,150,130,5,32,48,130,64,37,254,215,51,51,49,
    223,130,12,33,1,151,130,8,32,86,130,17,33,253,123,130,4,38,1,189,50,47,53,253,72,130,9,37,149,48,48,52,2,32,134,36,33,254,7,130,8,32,232,130,21,37,254,217,52,48,52,46,130,8,24,115,
    35,209,8,152,2,0,188,255,232,5,114,5,154,0,51,0,62,0,160,64,57,123,61,1,89,61,105,61,2,86,39,1,150,38,1,101,38,117,38,133,38,3,84,38,1,150,28,1,135,28,1,150,27,1,133,27,1,25,12,1,10,
    12,1,26,11,1,9,11,1,90,10,106,10,2,25,10,1,4,184,255,248,64,46,9,15,72,35,21,59,0,126,160,51,1,51,40,126,15,15,30,125,64,59,1,59,59,23,64,53,21,126,23,35,53,145,20,20,23,52,145,24,
    3,23,17,0,46,145,7,19,0,63,253,204,63,63,237,17,57,47,237,57,1,47,253,196,17,18,57,47,93,237,51,47,253,222,93,237,130,11,35,49,48,43,93,142,0,47,37,6,7,14,3,35,34,46,2,39,38,39,53,
    52,46,2,121,55,6,39,30,2,21,20,14,2,7,50,130,7,103,177,5,35,54,55,54,55,24,104,206,9,130,223,8,106,30,13,40,57,78,51,51,79,58,39,13,30,4,38,66,88,50,248,168,1,172,94,159,117,66,49,
    77,96,47,58,89,61,31,6,20,39,33,36,40,10,11,2,252,161,228,63,107,78,44,148,142,230,72,55,24,46,35,22,22,35,46,24,55,72,172,55,78,51,24,253,158,5,154,47,96,143,96,75,121,92,63,16,45,
    75,99,55,179,5,32,34,27,30,18,21,27,4,28,253,248,38,71,102,65,115,129,0,1,24,103,165,160,8,87,2,0,64,255,232,5,155,5,154,0,55,0,67,0,163,64,103,39,43,1,102,40,118,40,2,156,36,1,42,
    36,138,36,2,58,30,74,30,90,30,3,40,30,1,101,25,117,25,2,54,19,70,19,86,19,3,40,3,1,41,126,5,28,28,33,46,126,56,14,16,126,12,10,22,125,33,5,56,10,33,33,10,56,5,4,69,62,126,15,130,250,
    47,65,145,51,51,27,59,145,41,5,41,38,145,17,10,17,41,130,1,50,11,28,145,27,19,16,11,145,13,3,0,63,253,196,63,237,17,57,57,90,34,5,43,17,51,16,237,17,51,47,237,1,47,93,237,68,210,5,
    38,47,16,237,16,206,253,205,130,22,32,57,130,10,33,49,48,65,252,8,8,33,19,52,62,2,55,62,3,55,53,35,53,33,21,35,21,30,3,21,20,14,2,35,53,50,62,2,53,52,46,2,35,34,6,7,134,18,36,34,46,
    2,37,52,106,85,5,8,114,22,51,50,54,64,29,62,95,65,29,94,120,138,73,140,1,192,140,132,188,121,57,63,125,187,123,85,124,81,40,46,100,157,111,136,176,48,64,93,60,29,40,82,128,88,88,127,
    83,39,2,5,87,92,93,87,87,93,92,87,1,91,67,119,94,65,13,78,120,85,49,7,246,144,144,246,10,94,148,194,110,117,189,134,72,138,59,104,142,83,83,148,112,65,111,96,14,65,94,118,66,79,135,
    99,57,58,100,135,74,113,130,127,113,115,128,127,100,71,6,8,120,21,5,178,0,50,0,70,0,195,64,135,87,58,1,87,54,151,54,2,132,49,1,115,49,1,37,49,1,22,49,1,5,49,1,39,48,1,21,48,1,7,48,
    1,135,43,1,89,18,105,18,121,18,153,18,4,116,13,1,102,13,1,5,13,21,13,2,150,12,1,116,12,132,12,2,37,12,1,19,12,1,4,12,1,139,8,1,124,8,1,41,8,1,26,8,1,11,8,1,139,2,1,124,2,1,41,2,1,11,
    2,27,2,2,46,31,15,125,66,41,125,20,66,130,1,56,72,56,125,5,31,36,145,30,25,46,145,15,15,61,51,145,10,4,61,145,0,19,0,63,237,67,102,5,49,222,50,237,50,1,47,237,18,57,57,47,47,16,237,
    16,253,206,51,65,101,10,67,107,14,132,14,33,5,34,24,133,217,8,38,50,22,22,18,21,62,3,65,113,7,39,6,7,39,54,55,54,54,51,24,108,65,7,37,14,3,3,34,14,2,67,126,5,65,151,5,8,91,2,184,143,
    223,155,81,76,155,236,159,145,221,149,75,66,123,95,57,17,33,49,32,24,43,17,19,18,7,22,25,21,55,31,61,99,69,38,75,135,188,113,16,91,151,212,124,113,165,108,52,48,104,161,113,117,166,
    106,49,46,102,162,24,110,195,1,8,155,164,1,22,202,114,112,195,254,248,155,1,16,44,83,69,31,56,42,24,8,5,5,7,139,7,130,4,8,100,44,74,97,53,104,142,88,41,4,135,225,163,90,5,50,95,162,
    214,120,118,213,162,95,88,157,214,126,126,217,160,91,0,0,2,0,48,0,0,1,240,5,217,0,7,0,27,0,41,64,20,23,23,2,0,126,6,13,13,4,6,6,1,145,4,18,18,99,8,0,15,0,63,222,237,63,253,196,1,47,
    205,51,47,16,253,206,51,47,49,48,1,17,51,21,33,53,51,17,19,34,46,2,24,130,6,9,8,45,14,2,1,100,140,254,64,140,86,22,39,28,17,17,28,39,22,22,40,29,17,17,29,40,4,0,252,144,144,144,3,112,
    1,4,16,28,39,23,23,39,29,16,16,29,39,23,131,36,130,124,8,145,0,23,255,232,4,54,5,178,0,55,0,189,64,71,105,37,1,149,53,1,119,53,135,53,2,119,37,135,37,2,137,27,153,27,2,122,7,1,54,55,
    1,68,2,84,2,2,35,2,1,55,2,50,5,43,43,1,5,125,76,32,92,32,2,58,29,1,32,29,35,32,24,48,24,2,24,24,57,50,125,31,35,16,16,35,42,184,255,216,64,48,15,18,72,66,42,82,42,2,42,40,43,43,45,
    32,55,145,2,29,29,10,45,145,40,4,155,15,1,124,15,140,15,2,107,15,1,77,15,93,15,2,15,10,16,16,21,145,10,19,0,63,237,50,47,18,57,93,130,0,41,63,237,17,57,47,196,253,196,17,51,131,16,
    34,43,1,47,130,249,33,206,237,79,156,5,130,32,38,253,206,51,47,17,18,57,130,9,35,93,49,48,0,130,47,49,93,93,1,93,1,21,35,22,22,21,20,14,2,35,34,46,2,39,24,109,13,10,36,33,53,51,38,
    38,65,37,5,24,109,16,11,8,164,4,54,225,63,74,83,141,184,102,35,99,104,93,28,34,95,105,106,45,156,152,44,77,103,60,254,5,237,63,74,86,140,179,93,211,98,127,199,56,109,86,54,40,76,111,
    71,3,28,155,55,136,88,106,147,92,41,11,21,31,19,198,30,48,33,18,114,109,52,82,69,59,29,155,52,133,94,98,144,95,46,51,189,88,23,52,82,59,54,80,66,60,34,0,0,2,0,84,255,232,5,155,5,178,
    0,49,0,67,0,166,64,109,55,24,11,14,72,24,48,1,26,47,1,22,37,1,11,26,1,88,25,1,10,20,1,23,7,1,152,65,1,152,61,1,151,56,1,149,52,1,134,52,1,121,43,1,107,43,1,100,37,1,122,26,1,153,130,
    35,8,61,125,58,40,125,23,31,58,23,23,58,31,3,69,53,15,50,125,15,0,1,0,15,53,53,63,18,145,45,19,63,145,146,32,1,131,32,1,82,32,98,32,114,32,3,32,35,31,5,28,145,35,4,0,63,253,222,206,
    17,57,93,93,93,67,85,5,38,205,1,47,93,237,50,50,68,195,5,35,16,237,16,237,65,54,7,65,59,6,132,6,37,93,43,19,52,62,2,67,45,8,34,22,22,51,68,189,10,52,53,54,54,51,50,22,22,18,21,20,2,
    6,6,35,34,46,2,55,20,22,23,67,106,7,8,76,14,2,84,31,78,133,102,83,127,85,43,67,98,111,45,59,152,95,129,191,126,62,58,121,188,129,84,164,78,67,174,97,156,245,169,90,89,174,255,166,158,
    247,172,90,171,52,51,33,85,75,52,18,41,65,47,56,70,38,13,2,138,75,155,125,79,56,101,140,83,96,155,120,84,26,46,50,88,67,28,6,8,93,43,43,179,29,30,112,195,254,248,152,163,254,234,203,
    115,106,183,245,155,101,178,72,16,58,93,131,90,42,91,76,50,56,87,105,0,1,0,48,0,0,4,13,5,154,0,69,0,105,64,58,68,68,64,66,1,1,62,60,30,29,16,37,126,24,11,16,126,52,176,47,192,47,2,
    47,3,60,126,160,66,176,66,2,66,42,145,19,29,30,30,3,6,145,55,19,130,1,46,66,2,67,145,69,3,61,66,145,63,18,0,63,253,196,130,2,68,115,6,49,50,50,47,51,16,237,1,47,93,253,196,222,93,50,
    253,50,222,237,123,119,5,42,47,16,205,50,47,49,48,1,21,35,17,68,84,9,33,21,20,65,38,6,38,38,39,38,39,55,22,23,69,229,9,43,53,52,62,2,53,52,38,35,34,14,2,21,67,174,6,8,109,35,53,1,240,
    140,42,80,29,54,69,39,14,6,7,6,25,35,45,60,37,15,38,23,26,34,61,60,48,20,39,30,19,54,90,115,61,64,81,45,16,6,6,6,22,30,39,48,25,9,140,254,64,140,140,5,154,144,254,211,32,27,31,53,70,
    39,30,62,60,56,24,40,49,40,62,76,36,65,87,26,31,18,101,21,43,18,50,65,80,49,81,132,94,51,35,58,78,42,30,61,58,53,23,36,44,69,108,130,61,254,114,24,121,21,8,8,53,48,255,231,4,138,5,
    154,0,21,0,74,64,42,151,18,1,55,4,71,4,2,38,4,1,0,125,21,16,18,125,17,9,9,17,19,20,1,125,16,16,1,19,3,11,21,18,3,8,6,9,9,11,145,6,67,143,7,41,63,196,18,23,57,1,47,237,57,57,130,254,
    49,16,237,16,220,237,49,48,93,93,93,1,1,14,3,35,34,39,53,132,254,34,55,1,51,130,1,8,50,4,138,254,69,44,87,107,137,93,108,71,85,81,61,82,63,55,35,254,26,191,1,125,2,1,104,5,154,252,
    23,99,167,123,69,31,170,51,47,79,102,55,4,2,252,165,3,91,0,2,0,22,24,128,201,250,8,54,1,0,64,255,232,3,153,5,154,0,23,0,54,181,18,16,14,17,72,14,184,255,240,64,24,13,17,72,153,8,1,
    154,7,1,23,126,21,21,25,11,125,10,22,3,11,16,145,5,19,0,63,253,206,63,1,24,140,213,7,36,93,93,43,43,1,68,237,6,8,59,53,51,20,30,2,51,50,62,2,53,17,51,3,153,50,104,161,112,110,161,107,
    52,183,26,60,96,69,69,97,61,27,169,1,166,93,162,121,70,74,121,153,79,52,99,78,47,47,79,106,60,3,247,0,1,0,188,0,0,3,180,130,127,33,11,0,24,126,123,139,38,3,0,24,0,0,5,166,130,151,62,
    33,0,36,0,56,0,215,64,12,156,36,1,122,36,138,36,2,104,36,1,28,184,255,248,64,12,10,13,72,18,8,130,4,8,32,149,8,1,8,184,255,240,64,116,12,17,72,39,8,1,7,29,1,8,18,1,36,7,7,5,58,21,126,
    47,42,63,42,2,131,4,43,79,42,3,42,64,44,51,72,42,64,31,35,130,4,8,71,19,22,72,42,52,126,11,31,10,34,213,8,1,166,8,182,8,198,8,3,8,3,5,126,32,0,79,10,127,10,2,111,10,143,10,159,10,191,
    10,207,10,5,31,10,175,10,2,10,16,145,47,47,10,26,145,37,37,32,5,145,34,7,10,10,4,35,9,18,33,4,145,1,74,44,5,41,51,18,57,47,51,196,253,196,51,47,74,41,7,53,113,114,204,196,253,206,51,
    93,93,196,16,221,50,253,222,43,43,43,93,113,237,17,131,38,40,49,48,0,93,93,1,93,43,93,130,18,130,8,42,53,33,21,35,17,33,21,1,35,17,35,76,49,5,70,163,5,40,30,2,23,51,17,19,17,1,1,72,
    171,15,8,42,38,1,186,140,2,82,253,210,198,113,8,41,69,98,65,73,103,66,30,33,70,107,74,60,93,66,41,8,114,162,1,146,252,72,36,53,35,16,17,34,53,36,36,130,7,8,47,16,35,53,5,10,144,144,
    254,75,107,253,22,2,208,56,95,70,40,51,86,115,63,66,119,90,52,39,67,91,51,1,181,253,198,253,230,2,26,1,8,31,55,75,43,43,75,55,31,131,7,8,143,44,75,54,31,0,1,0,164,0,0,4,88,5,178,0,
    50,0,146,64,88,70,37,86,37,2,53,37,1,70,36,1,55,36,1,69,31,1,54,31,1,149,19,1,135,19,1,24,14,40,14,2,154,8,1,8,13,1,34,125,16,11,1,11,11,47,52,49,49,23,23,45,47,43,43,1,41,3,126,48,
    47,1,47,41,39,145,3,6,48,24,29,23,23,16,0,145,2,48,48,16,47,42,145,45,18,16,145,29,4,0,63,237,63,253,196,18,57,47,196,237,17,57,47,18,57,16,220,50,237,50,1,47,93,253,196,206,50,47,
    16,205,50,47,50,47,17,130,29,38,93,237,49,48,0,93,1,73,248,9,35,1,21,35,21,70,154,11,37,14,2,7,6,7,53,73,231,11,34,35,34,39,69,111,8,8,46,2,100,140,30,62,32,85,130,88,45,41,87,136,
    95,50,89,78,66,27,63,48,47,62,54,152,100,136,200,132,65,69,127,180,112,82,70,140,254,64,140,140,3,30,144,110,11,9,75,60,7,8,89,16,27,34,18,42,53,168,45,35,30,50,83,149,206,122,110,
    188,137,77,18,254,252,144,144,1,254,144,0,2,0,2,255,233,5,37,5,178,0,43,0,52,0,122,64,76,57,52,1,23,46,39,46,2,134,45,150,45,2,118,21,1,103,21,1,134,15,1,84,15,1,67,15,1,72,12,1,88,
    5,1,24,4,1,9,4,1,51,16,125,33,26,130,1,8,41,54,34,50,125,0,7,0,2,145,43,41,41,23,51,145,34,34,44,26,28,145,25,23,19,44,145,14,4,0,63,237,63,51,237,50,17,57,47,237,17,51,47,130,9,36,
    1,47,206,237,50,70,152,6,75,9,14,46,55,22,51,50,62,4,55,62,3,51,32,17,20,14,4,69,140,9,34,33,14,5,130,12,8,106,1,34,6,7,6,6,7,33,2,2,52,61,58,81,56,36,27,25,17,19,80,116,149,88,1,194,
    6,24,50,89,133,96,79,58,52,61,81,99,54,18,1,253,143,11,27,41,57,78,103,67,79,58,3,88,111,139,35,9,13,8,2,92,19,156,29,81,136,180,199,206,93,106,163,111,56,253,33,60,157,166,160,126,
    77,21,158,29,82,140,187,105,78,159,148,128,96,55,21,5,41,167,161,40,99,56,2,11,0,68,75,6,34,106,5,154,130,215,53,34,64,16,3,3,6,9,1,4,126,6,1,145,3,3,5,18,0,3,0,63,63,130,208,79,63,
    7,35,49,48,1,17,126,220,5,45,1,100,2,6,253,250,168,5,154,253,120,151,253,133,130,61,68,147,5,32,164,130,9,47,5,0,24,64,11,0,7,1,126,3,1,145,4,3,2,18,130,66,37,237,1,47,237,18,57,130,
    61,47,33,17,35,17,33,3,164,253,193,169,2,232,4,254,251,2,130,46,8,123,0,2,0,64,255,232,6,198,5,178,0,59,0,79,0,170,64,107,70,67,1,73,63,1,101,58,1,70,45,86,45,2,73,43,1,7,38,1,102,
    14,1,85,14,1,105,10,1,90,10,1,117,26,1,86,26,1,119,14,1,119,10,1,75,60,70,125,7,22,44,2,3,7,17,125,60,32,34,125,31,29,7,60,29,29,60,7,3,81,55,125,54,2,0,145,47,47,41,55,55,34,31,145,
    29,130,75,1,99,75,115,75,2,86,75,1,75,22,24,145,44,41,19,65,145,12,65,180,5,45,253,50,50,93,93,93,222,237,196,51,47,17,51,47,76,195,5,82,213,6,38,253,205,16,237,17,23,57,130,4,130,
    189,32,0,130,37,66,207,11,35,37,50,55,46,24,151,32,10,39,14,2,7,22,51,50,62,2,78,71,5,54,20,14,4,35,34,38,39,6,6,35,34,46,4,53,51,20,30,2,1,52,46,2,35,68,25,6,8,106,23,62,3,1,226,118,
    114,65,112,82,47,59,109,155,97,97,155,109,59,46,80,111,64,113,114,64,95,63,30,140,1,192,140,19,42,66,94,123,78,81,174,85,85,178,85,78,122,93,66,42,19,168,30,61,94,2,143,27,59,92,66,
    66,92,59,27,37,66,89,53,52,90,64,37,114,70,62,159,192,225,128,102,184,139,83,83,139,184,102,129,225,192,159,62,69,49,86,118,69,144,144,54,111,102,89,66,38,57,130,0,8,32,38,66,89,102,
    111,54,69,118,86,49,3,78,52,121,104,69,69,104,121,52,114,202,172,141,54,54,141,172,202,0,1,0,26,16,117,169,1,170,8,46,2,0,94,255,232,4,184,5,254,0,50,0,70,0,117,64,38,102,69,1,102,
    68,1,101,63,1,106,59,1,106,53,1,149,45,1,31,31,72,41,126,20,0,126,51,17,51,20,130,1,48,72,61,126,10,30,184,255,208,64,28,15,18,72,30,25,93,31,130,32,48,36,56,145,120,17,1,17,15,15,
    66,36,145,25,1,66,145,5,79,197,8,43,51,93,237,17,51,47,93,18,57,43,1,47,79,202,5,40,18,57,16,237,16,237,17,57,47,76,121,8,72,188,8,37,52,62,2,51,50,23,77,162,7,39,22,23,22,23,21,38,
    39,38,75,56,5,37,20,30,4,23,22,22,24,143,207,16,8,69,216,62,116,164,102,103,165,116,62,62,116,165,103,93,78,28,35,54,94,129,75,42,75,29,33,29,33,37,32,83,45,46,62,38,16,18,27,35,33,
    28,8,9,10,168,36,71,104,67,67,103,70,36,36,70,103,67,67,104,71,36,1,178,101,168,122,67,67,122,168,101,131,7,8,38,29,100,194,81,72,111,75,38,15,8,11,12,166,20,17,14,23,27,46,59,32,56,
    121,123,119,107,90,32,35,75,40,64,116,88,52,52,88,116,64,134,7,68,233,6,8,61,4,122,5,154,0,31,0,80,185,0,24,255,232,64,46,14,17,72,102,23,1,84,23,1,89,19,1,69,10,1,54,10,1,72,6,1,31,
    26,125,0,32,3,48,3,2,3,3,33,13,126,15,0,31,31,14,8,145,21,3,14,18,0,84,79,5,44,51,1,47,237,18,57,47,93,51,237,50,49,48,70,222,5,35,43,1,54,54,80,187,5,38,14,2,21,17,35,17,52,77,141,
    10,8,167,3,157,23,22,41,76,106,64,84,123,81,39,168,75,132,181,107,95,169,126,73,14,23,28,14,3,28,38,101,63,71,106,71,36,64,106,135,71,252,118,3,146,122,193,134,71,52,106,161,109,45,
    84,75,64,25,0,1,0,94,255,232,5,102,5,178,0,55,0,152,64,98,89,47,1,40,47,56,47,72,47,3,85,41,1,87,40,1,91,25,1,9,20,1,153,52,1,136,52,1,121,52,1,104,52,1,153,35,1,106,20,122,20,2,55,
    4,126,1,31,31,11,125,50,48,50,64,50,2,1,50,1,50,57,38,125,23,2,4,145,1,55,55,43,33,150,30,1,133,30,1,100,30,116,30,2,82,30,1,30,28,31,31,33,145,28,4,43,145,18,66,2,5,79,199,7,32,17,
    130,225,34,205,253,205,132,234,42,57,47,47,93,16,237,51,47,16,253,196,66,3,9,133,248,55,1,35,17,51,21,50,30,4,21,20,14,4,35,34,36,38,2,53,52,18,54,36,51,50,79,174,10,8,104,51,50,62,
    4,53,52,46,2,35,3,223,144,144,39,88,87,80,61,36,56,91,117,123,118,47,179,254,237,186,96,110,198,1,17,163,235,158,175,237,126,204,142,77,68,139,211,143,28,79,85,85,67,41,36,62,82,45,
    1,218,1,192,155,13,32,53,80,111,73,84,128,96,64,40,17,108,192,1,9,157,163,1,22,204,115,76,186,110,93,162,217,124,128,214,154,87,9,22,39,62,86,58,53,81,54,28,70,203,5,33,6,114,16,132,
    49,1,20,8,70,94,0,0,3,89,5,178,0,49,0,103,64,58,103,31,1,86,31,1,57,18,73,18,2,86,10,102,10,2,33,126,15,36,36,1,51,4,49,126,1,24,24,1,15,12,145,41,36,0,0,41,46,4,3,3,7,145,49,46,46,
    19,35,18,25,30,24,24,19,145,30,4,81,233,6,42,63,18,57,47,51,237,50,47,50,16,204,131,4,38,237,50,1,47,51,47,16,87,100,5,33,196,237,67,21,5,52,19,35,17,51,21,54,54,51,50,30,2,51,50,54,
    55,53,52,38,35,34,6,73,241,8,37,32,18,17,17,35,17,75,61,5,131,23,8,78,238,144,144,29,74,32,29,49,45,44,26,29,66,28,160,179,52,92,35,41,36,29,37,32,86,54,1,16,253,168,15,33,35,33,15,
    32,57,50,40,15,24,70,32,2,12,1,192,150,16,27,33,39,33,31,19,71,213,213,15,9,11,12,169,8,6,5,10,254,210,254,210,252,170,2,154,8,17,14,8,130,28,8,125,32,18,0,0,2,0,10,255,232,7,53,5,
    154,0,76,0,94,0,214,64,135,89,90,1,134,80,1,103,80,119,80,2,149,68,1,102,67,1,101,66,1,54,66,1,39,66,1,153,49,1,136,30,1,149,22,1,118,22,134,22,2,154,15,1,155,14,1,39,11,1,8,125,87,
    37,39,126,35,33,0,33,32,87,1,87,33,87,33,18,96,58,58,96,92,100,3,116,3,2,69,3,1,52,3,1,3,77,125,24,25,42,30,1,30,21,18,0,145,33,25,24,24,21,92,145,30,22,130,23,58,30,58,145,57,63,145,
    52,70,145,45,33,30,45,45,30,33,3,82,39,34,145,36,3,82,145,13,19,75,66,6,72,101,5,41,237,220,237,221,237,17,51,93,16,237,80,191,7,43,51,51,93,206,50,237,50,93,93,93,50,17,83,47,5,36,
    47,47,93,17,51,86,191,5,85,84,16,32,1,86,162,12,44,53,52,54,55,6,6,7,39,62,3,55,54,54,86,212,7,78,100,5,34,4,51,50,69,163,11,39,4,35,34,46,2,39,38,1,133,28,34,2,53,52,130,13,8,140,
    6,6,3,65,77,140,58,67,119,90,52,41,79,116,75,96,134,85,38,22,21,49,84,30,92,24,72,89,101,53,82,243,141,140,1,192,140,30,51,67,36,38,71,70,71,75,82,47,39,80,32,38,35,33,36,31,77,39,
    35,66,69,71,80,89,52,53,85,67,50,18,42,254,50,17,43,73,56,41,56,35,15,44,73,97,53,33,36,3,142,55,50,15,69,110,156,102,83,139,100,55,75,120,150,75,73,130,57,20,64,44,83,39,67,51,35,
    8,107,118,8,246,144,144,246,83,112,67,28,28,43,49,43,28,26,15,18,22,148,25,19,17,27,132,13,8,122,25,42,53,28,65,254,102,48,112,94,63,46,72,87,42,81,121,82,47,6,57,134,0,2,0,114,255,
    232,5,116,5,178,0,28,0,40,0,128,64,85,57,40,1,40,40,1,57,36,1,40,36,1,54,34,1,39,34,1,151,30,1,54,30,1,39,30,1,86,24,1,69,24,1,72,21,1,136,15,1,89,13,1,75,13,1,70,9,86,9,2,135,7,1,
    1,126,3,38,125,26,4,4,42,32,125,16,18,1,18,28,3,27,145,4,4,11,29,145,23,4,35,145,11,19,74,187,5,48,63,237,17,57,47,237,63,1,47,93,237,18,57,47,51,237,222,66,223,6,32,93,139,0,59,1,
    17,35,17,35,14,5,35,34,46,4,53,52,18,54,54,51,50,18,19,51,17,5,34,2,17,16,18,130,11,8,82,17,16,2,5,116,168,167,4,26,49,75,106,139,88,91,138,103,69,41,18,66,126,183,117,215,232,10,165,
    253,137,154,153,150,151,148,149,146,5,154,250,102,2,123,84,163,146,125,90,51,58,101,137,158,172,86,190,1,32,194,98,254,176,254,176,2,136,128,254,213,254,201,254,221,254,234,1,27,1,
    40,1,51,1,37,68,237,5,49,4,242,5,154,0,11,0,122,64,71,3,145,64,8,6,5,8,145,24,140,107,146,36,124,255,232,3,166,130,167,8,98,46,0,153,64,97,57,32,1,40,32,1,56,26,1,69,11,1,71,10,1,137,
    32,1,137,31,1,156,11,1,154,10,1,149,5,1,137,5,1,8,125,29,29,43,48,19,19,39,43,0,2,126,45,43,148,40,1,131,40,1,84,40,100,40,116,40,3,40,43,39,39,34,145,3,43,43,24,2,44,145,46,3,155,
    18,1,140,18,1,91,18,107,18,123,18,3,18,13,19,19,24,67,91,5,70,196,6,78,165,5,32,51,86,155,7,38,1,47,206,253,205,16,205,78,157,5,32,237,70,201,14,32,1,89,250,9,86,132,8,34,62,2,53,75,
    179,6,33,7,53,67,75,5,8,173,2,142,140,99,156,108,57,68,127,181,112,41,87,85,77,30,30,73,78,78,33,85,130,88,45,48,91,134,85,33,75,76,72,28,42,120,60,140,5,154,144,252,18,95,144,185,
    108,111,187,137,77,9,18,26,18,156,22,33,23,11,59,103,142,84,83,149,111,65,10,20,30,20,168,20,23,5,244,144,0,0,1,0,8,255,231,6,22,5,154,0,37,0,117,64,70,87,23,103,23,2,53,6,69,6,2,103,
    34,119,34,2,118,33,1,103,33,1,136,30,1,119,23,1,0,2,125,37,35,35,30,32,125,31,31,33,34,3,125,30,30,15,39,18,20,125,17,15,34,3,30,3,25,2,35,145,32,37,3,17,145,15,20,25,145,8,19,0,63,
    253,222,196,237,63,196,253,83,254,5,34,205,253,205,98,166,5,32,57,80,233,5,130,15,87,122,9,32,93,130,247,38,1,14,3,35,34,46,4,76,183,6,82,142,5,84,22,5,57,35,53,6,22,141,254,133,44,
    87,107,137,93,78,123,94,66,42,19,140,1,192,140,33,61,90,57,84,35,11,33,43,130,131,240,36,167,99,167,123,69,76,105,5,38,144,144,69,114,82,45,47,84,46,6,8,146,2,203,144,0,3,0,94,255,
    232,5,170,5,178,0,11,0,18,0,25,0,181,64,127,11,24,1,85,22,1,4,22,1,88,21,1,133,17,1,4,16,84,16,2,89,14,1,10,14,1,137,13,1,118,11,150,11,2,99,11,1,38,11,1,21,11,1,87,8,1,149,7,1,116,
    7,1,98,7,1,37,7,1,20,7,1,153,5,1,122,5,1,108,5,1,41,5,1,26,5,1,88,2,1,153,1,1,123,1,1,108,1,1,41,1,1,27,1,1,12,9,125,16,19,32,19,2,19,19,27,18,20,125,15,3,31,3,2,3,12,145,20,20,0,15,
    145,6,4,23,90,181,11,38,1,47,93,237,50,18,57,130,5,79,164,14,67,223,12,132,12,24,86,76,11,45,19,38,0,35,34,0,7,5,33,22,0,51,50,0,24,137,251,15,52,192,18,254,253,215,208,254,238,22,
    3,228,252,28,21,1,12,203,220,1,9,24,24,138,2,15,8,103,3,48,245,1,13,254,232,234,155,234,254,236,1,11,0,1,0,43,254,90,4,61,5,47,0,50,0,89,64,50,150,41,1,150,20,1,90,15,106,15,2,1,3,
    126,19,11,0,48,48,28,52,37,36,34,126,31,30,28,32,33,34,37,28,145,31,34,15,3,1,145,49,43,145,19,22,19,12,145,11,28,0,63,237,63,51,253,222,237,196,63,196,253,196,16,205,50,1,47,205,196,
    253,205,196,17,130,220,33,204,204,130,9,33,49,48,130,197,58,1,33,21,35,17,20,14,2,7,6,7,53,62,4,53,53,6,6,35,34,46,2,53,17,35,53,51,24,109,168,7,66,1,5,61,53,53,35,2,129,1,188,140,
    34,55,70,37,86,110,36,50,59,50,33,45,115,74,75,121,86,46,176,176,164,1,130,240,8,54,29,50,69,41,35,74,61,39,140,2,8,144,254,179,87,131,98,67,22,52,8,140,4,17,37,66,101,73,63,46,55,
    47,86,122,75,2,64,140,250,53,254,209,140,253,221,51,82,57,31,31,57,82,51,39,0,130,231,45,94,255,232,4,236,5,178,0,25,0,150,182,118,10,24,146,23,229,36,10,255,233,5,45,130,243,8,70,
    41,0,79,64,50,118,38,1,103,38,1,90,34,1,150,17,1,151,13,1,70,8,1,51,8,1,39,8,1,57,5,89,5,2,41,126,2,2,43,22,10,125,31,7,145,36,4,22,24,145,21,19,19,1,18,0,63,63,51,237,50,63,237,1,
    47,237,206,18,57,47,237,72,4,10,43,33,35,17,52,46,2,35,34,6,7,14,7,82,73,7,61,4,55,62,3,51,50,30,2,21,5,45,168,32,69,109,76,111,144,35,12,18,21,26,40,56,81,108,72,79,58,82,85,9,57,
    82,117,151,88,117,168,109,51,3,68,126,182,119,56,167,161,55,145,164,172,162,145,108,63,21,158,82,67,10,8,43,72,153,241,169,0,1,0,201,0,0,4,164,5,154,0,18,0,49,64,25,153,3,1,18,126,
    1,1,8,20,11,6,126,8,6,4,145,11,13,13,7,10,3,1,7,130,168,32,196,74,28,6,82,52,7,131,171,131,163,40,35,34,7,17,35,17,51,17,54,132,152,60,4,164,168,245,178,228,168,168,229,197,93,146,
    101,53,2,18,232,110,253,116,5,154,253,122,122,50,95,138,89,130,109,32,33,130,109,32,100,130,109,34,9,0,149,24,130,211,184,32,40,130,195,8,83,88,5,178,0,43,0,124,64,76,198,39,214,39,
    2,151,39,167,39,183,39,3,133,39,1,119,39,1,86,39,102,39,2,57,35,1,42,35,1,70,10,1,68,9,1,15,12,126,32,32,21,45,43,43,24,26,41,41,26,125,21,0,0,23,21,2,2,21,14,18,15,145,31,21,26,145,
    24,0,145,2,42,42,36,145,7,75,133,5,49,196,253,222,253,196,222,237,63,1,47,51,47,16,205,50,47,16,237,85,125,5,37,17,18,57,47,237,196,66,32,10,34,19,53,51,66,13,6,37,17,35,17,33,34,46,
    82,161,7,36,30,2,51,33,17,90,230,5,8,80,7,51,21,40,138,16,73,108,144,88,136,192,121,56,168,254,170,110,159,103,50,140,1,192,140,46,72,89,44,1,89,158,179,46,77,60,43,12,119,4,42,144,
    47,90,69,42,75,151,227,151,252,170,1,202,42,80,115,73,144,144,57,63,29,6,1,12,213,213,20,29,34,14,144,0,2,0,104,255,232,6,193,130,245,8,107,82,0,94,0,172,64,113,56,94,1,56,90,1,54,
    88,1,54,84,1,149,81,1,71,81,87,81,2,149,80,1,56,80,1,149,14,1,70,14,134,14,2,149,13,1,86,13,1,68,13,1,73,10,153,10,2,136,4,1,154,3,1,154,2,1,89,2,1,74,2,1,44,43,30,51,126,38,25,30,
    126,66,61,92,125,15,0,76,16,76,32,76,3,76,76,96,86,125,7,56,145,33,43,44,44,15,20,145,69,33,130,1,37,89,83,145,12,4,89,69,217,9,34,57,47,47,75,80,9,79,175,6,32,222,92,19,7,75,67,16,
    32,93,130,0,32,5,73,188,9,33,22,23,65,49,6,34,20,14,2,92,39,35,37,4,21,20,14,4,3,73,237,10,33,2,52,73,228,9,39,181,220,38,20,43,43,40,18,92,59,36,43,29,39,26,14,6,1,19,45,73,109,148,
    73,74,23,6,32,24,74,12,10,37,234,236,19,24,13,4,92,80,42,45,25,38,46,41,32,5,91,182,166,143,105,60,5,50,74,68,16,100,81,10,55,11,0,37,64,18,11,8,9,126,3,5,2,3,9,4,145,6,3,10,3,145,
    1,18,0,93,97,5,46,1,47,221,196,16,253,221,196,49,48,33,33,53,33,17,130,3,32,21,130,5,52,4,12,252,29,1,157,254,99,3,227,254,98,1,158,152,4,106,152,152,251,150,130,83,8,50,84,255,232,
    7,193,5,178,0,76,0,208,64,137,105,73,1,88,73,1,55,68,1,89,54,1,74,54,1,75,53,91,53,2,121,44,137,44,2,136,43,1,121,43,1,133,39,1,22,39,1,4,130,5,8,94,38,1,7,38,1,101,34,117,34,2,150,
    33,1,7,33,1,72,28,1,72,26,1,89,14,1,56,13,136,13,2,122,8,1,146,23,1,132,23,1,23,24,46,0,126,75,36,125,11,24,75,11,11,75,24,3,78,139,61,155,61,2,61,56,62,62,65,125,56,75,70,145,51,24,
    23,62,61,23,61,0,0,61,23,3,16,1,6,145,41,41,46,51,19,16,145,31,85,237,5,43,51,47,237,50,17,23,57,47,47,47,17,51,126,71,6,37,237,50,47,18,57,93,133,20,40,16,237,16,237,57,17,51,93,93,
    66,34,20,38,93,93,93,1,17,22,23,88,197,16,36,39,54,55,62,3,95,101,11,37,38,39,38,39,6,7,71,92,6,38,52,62,2,55,23,6,6,102,61,9,8,76,17,4,40,26,41,35,110,79,127,174,106,47,67,133,197,
    129,58,97,80,64,25,58,35,150,38,77,33,87,114,143,87,156,254,181,99,74,154,240,166,92,141,48,56,42,46,62,53,151,96,111,178,125,66,25,53,81,56,117,87,77,50,89,122,72,75,116,40,47,33,
    2,221,254,53,41,33,28,45,95,118,7,44,22,37,47,24,57,72,71,95,75,32,61,48,29,95,126,9,63,49,29,34,44,44,34,29,49,72,131,184,113,54,113,113,106,46,115,67,166,98,76,123,88,48,45,28,33,
    41,1,203,0,87,127,6,8,91,226,5,178,0,62,0,82,0,204,64,134,88,70,1,73,70,1,55,66,71,66,87,66,3,88,59,1,105,53,1,103,49,1,37,16,11,14,72,137,31,1,150,26,1,102,21,1,101,20,1,137,11,1,
    120,53,1,120,49,1,118,32,1,118,11,1,78,73,63,139,61,155,61,2,40,61,1,148,41,1,41,16,61,3,56,29,46,125,73,23,125,34,73,130,1,8,65,84,63,125,56,6,125,5,5,56,28,145,29,29,39,68,145,51,
    19,78,32,15,18,72,73,78,1,24,78,1,78,41,39,145,18,61,5,0,145,13,13,100,16,116,16,132,16,3,16,18,4,0,63,51,93,51,47,253,206,51,16,237,50,50,93,93,43,77,134,5,37,1,47,51,47,237,16,100,
    99,10,40,18,23,57,93,93,93,17,18,57,74,211,6,90,111,5,103,219,6,44,1,34,14,2,21,35,52,62,4,51,50,22,23,70,119,5,32,20,101,219,12,101,218,10,38,53,52,62,2,55,38,17,79,12,11,8,41,14,
    3,1,226,65,94,61,30,168,19,42,66,93,123,77,85,169,81,168,205,150,215,139,66,59,112,161,102,60,93,65,33,45,94,146,100,126,116,64,109,79,44,87,196,7,37,39,74,103,65,105,27,87,163,6,45,
    38,67,92,53,53,88,63,34,5,40,49,86,118,69,87,142,5,57,50,50,100,73,119,152,79,77,145,114,69,168,40,66,85,46,52,102,81,50,64,62,159,194,227,130,87,194,7,39,130,229,195,159,62,61,252,
    178,87,168,10,8,158,140,54,54,140,172,202,0,0,2,0,10,0,0,8,200,5,179,0,95,0,112,0,240,64,155,105,78,1,105,77,1,106,76,1,153,74,1,106,74,1,103,53,1,105,49,121,49,2,56,49,72,49,88,49,
    3,134,26,1,40,23,1,87,18,1,53,18,1,87,17,1,150,16,1,117,16,133,16,2,54,16,1,85,125,0,95,160,8,176,8,192,8,3,95,64,12,15,72,8,95,8,95,114,73,20,70,125,99,54,21,125,22,22,52,112,55,99,
    60,125,107,107,114,23,53,125,52,38,33,125,39,31,44,175,44,239,44,3,44,52,55,20,145,73,112,102,95,90,145,0,5,80,145,13,13,102,145,65,4,155,23,131,101,8,41,23,53,28,145,47,53,39,38,38,
    53,3,150,54,1,54,22,18,0,63,51,93,63,51,47,51,16,220,237,17,57,93,93,63,237,50,47,237,222,50,237,50,16,131,4,35,1,47,222,93,131,11,41,237,57,18,57,47,237,206,50,50,17,130,6,34,57,16,
    237,131,8,38,57,47,47,43,93,17,51,79,100,18,32,1,67,120,5,85,104,7,35,4,7,1,35,76,186,5,40,2,53,52,54,55,54,55,23,6,24,83,63,7,37,62,2,55,51,1,19,89,156,11,36,6,7,62,5,51,102,77,6,
    39,35,34,38,39,38,39,37,54,130,72,33,38,35,89,144,7,8,103,7,41,20,23,20,50,26,58,70,20,32,42,22,40,70,69,73,85,102,65,254,192,185,254,91,27,59,70,86,54,52,91,67,38,24,15,17,22,122,
    16,11,11,16,58,49,45,64,48,38,17,187,1,150,254,44,91,74,47,34,63,89,54,56,91,64,34,12,8,39,69,66,66,75,86,52,60,98,69,37,36,66,94,59,45,74,28,33,26,254,100,7,12,53,38,20,31,22,11,25,
    41,52,28,4,40,131,64,54,54,63,35,50,31,15,43,66,79,72,56,10,252,92,4,192,63,89,55,26,32,63,91,133,48,32,46,132,135,8,39,54,64,46,81,109,67,251,16,3,4,11,45,72,102,68,45,80,59,35,31,
    63,93,63,23,59,32,19,61,67,68,55,34,40,70,99,58,62,100,69,37,131,148,8,200,161,22,56,26,54,63,17,26,33,16,37,55,38,23,5,0,0,1,0,92,255,232,7,244,5,178,0,73,0,229,64,97,149,56,1,54,
    56,1,39,56,1,150,51,1,148,50,1,69,50,1,38,50,54,50,2,89,45,1,86,35,1,70,34,1,73,33,1,116,29,1,98,29,1,22,29,1,102,24,118,24,2,85,24,1,133,23,1,137,17,1,89,13,153,13,2,89,12,1,53,8,
    133,8,149,8,3,38,8,1,135,3,151,3,2,149,2,1,52,2,132,2,2,39,2,1,48,48,53,61,34,184,255,240,64,56,9,12,72,34,5,125,64,71,26,125,15,64,20,15,15,20,64,3,75,53,125,42,48,145,47,47,132,71,
    148,71,2,115,71,1,71,69,72,72,0,145,69,16,61,58,145,34,37,37,10,145,31,19,20,145,21,4,0,63,237,63,237,51,47,51,237,50,63,69,135,5,37,93,50,47,237,1,47,105,181,6,42,16,237,206,16,237,
    51,43,51,18,57,47,69,144,23,32,93,130,0,32,1,95,113,14,35,35,53,50,4,100,241,8,91,133,6,38,2,53,52,62,2,51,21,136,41,33,54,55,88,184,7,8,47,23,21,38,5,108,81,133,93,51,46,92,137,91,
    119,175,114,56,70,137,200,129,166,1,8,184,98,77,158,243,166,110,185,70,76,201,128,116,195,141,78,75,138,196,122,81,134,96,52,131,38,8,56,96,149,56,32,33,75,139,196,122,133,105,115,
    3,142,62,107,142,81,80,142,107,62,90,154,205,116,126,223,168,97,152,121,205,254,243,148,151,254,244,202,118,70,63,63,70,79,140,192,113,122,202,144,80,148,59,103,139,132,39,36,57,52,
    58,132,74,131,17,8,139,50,168,80,0,0,1,0,96,255,232,4,35,5,178,0,69,0,179,64,118,118,61,134,61,2,153,51,1,89,47,1,121,40,1,88,40,104,40,2,106,36,122,36,2,107,35,1,138,14,154,14,2,85,
    10,1,43,49,1,57,57,28,125,17,16,1,1,16,22,48,22,2,16,17,48,17,2,1,22,17,17,22,1,3,38,71,64,125,49,49,7,125,15,38,1,38,118,56,134,56,150,56,3,101,56,1,52,56,68,56,84,56,3,56,54,57,57,
    59,43,44,69,145,1,22,145,23,23,12,1,1,33,59,145,54,4,12,145,33,19,0,63,102,112,5,41,18,57,47,237,16,237,57,57,17,51,103,209,5,49,1,47,93,237,51,47,237,17,18,23,57,47,47,47,93,93,93,
    16,130,14,34,17,18,57,65,142,10,33,1,21,67,70,7,33,51,50,81,51,6,32,53,67,107,8,39,46,2,53,52,62,2,55,53,67,141,7,88,25,11,8,186,2,135,29,83,128,87,45,55,91,116,61,93,116,66,23,21,
    40,60,40,88,123,78,35,42,104,176,133,94,182,144,88,49,84,114,65,61,97,68,36,71,121,159,87,207,157,170,180,58,99,73,42,46,81,112,65,3,63,139,41,76,110,69,71,105,68,33,34,53,66,32,22,
    50,42,28,148,55,86,103,49,53,115,95,62,51,106,162,111,65,126,100,67,6,4,18,60,84,110,70,90,130,83,39,88,170,113,27,56,86,59,63,95,64,32,0,0,2,0,170,255,232,4,213,5,154,0,10,0,17,0,
    76,64,46,37,17,53,17,2,57,12,1,42,12,1,89,3,1,75,3,1,68,1,1,8,0,126,80,14,1,14,14,4,19,15,7,126,4,8,145,15,15,11,9,6,3,11,145,2,19,0,63,237,63,196,18,57,47,237,76,221,7,32,93,75,106,
    9,8,53,1,16,33,32,17,17,51,17,33,17,51,1,32,17,53,33,21,16,4,213,253,223,253,246,168,2,219,168,253,241,1,103,253,37,2,68,253,164,2,69,3,109,253,139,2,117,250,229,1,170,101,86,254,71,
    130,145,8,195,64,255,232,7,133,5,178,0,75,0,95,1,10,64,184,105,93,121,93,137,93,3,103,89,119,89,135,89,3,103,83,119,83,135,83,3,137,79,1,104,79,120,79,2,103,71,1,73,66,121,66,137,66,
    153,66,4,58,66,1,153,65,1,120,65,1,57,65,73,65,105,65,3,121,61,1,41,60,105,60,2,24,60,1,101,56,1,149,42,1,8,38,152,38,2,9,32,1,121,22,137,22,2,26,22,1,11,22,1,123,21,139,21,2,149,17,
    1,54,17,1,86,11,1,22,25,1,5,25,1,25,45,126,76,147,75,1,116,75,132,75,2,75,0,0,76,50,52,126,48,46,24,46,14,125,63,76,46,63,63,46,76,3,97,86,126,15,35,1,35,0,75,49,25,145,46,46,58,47,
    52,145,49,49,81,145,40,40,68,58,145,19,91,145,30,30,19,19,68,76,149,5,46,63,51,47,237,16,237,17,57,47,237,51,47,253,196,18,130,7,34,16,206,50,109,131,9,130,26,45,51,16,206,253,205,
    17,51,47,51,93,93,16,237,50,73,102,25,35,93,93,93,1,73,84,5,34,30,4,21,101,44,6,33,39,33,99,109,15,82,179,5,71,180,11,40,35,34,14,2,7,6,7,3,52,95,108,9,8,90,51,50,62,2,2,94,41,77,33,
    88,114,140,86,104,190,165,134,95,52,68,130,190,122,93,144,101,59,7,255,0,8,51,85,121,79,88,130,86,43,43,86,130,88,77,120,85,52,9,254,140,1,192,140,30,61,91,61,84,124,82,40,83,151,213,
    129,57,96,80,64,25,59,36,165,22,44,67,46,46,68,44,22,22,44,68,46,46,67,44,22,4,94,73,108,5,8,46,53,99,141,174,203,113,147,255,189,108,53,98,137,84,71,134,104,63,80,126,157,78,78,155,
    125,78,61,101,130,69,217,144,144,254,224,68,112,79,43,87,146,193,107,145,240,173,96,73,168,5,44,253,134,56,106,82,49,48,81,105,56,57,106,81,131,7,33,0,0,24,176,215,9,8,50,10,0,18,0,
    167,64,20,118,6,1,121,7,1,57,7,1,1,6,9,5,154,18,18,7,3,24,15,184,255,240,64,29,12,0,77,15,7,6,0,0,9,7,11,2,6,13,6,77,2,6,12,131,4,32,11,130,4,33,138,3,130,33,32,179,130,19,130,7,33,
    214,179,130,22,132,7,130,25,130,7,36,224,179,13,1,77,130,7,43,240,64,9,11,12,1,76,3,3,20,18,5,121,244,6,131,7,33,64,11,130,46,48,47,5,63,5,79,5,3,5,47,93,43,43,51,18,57,47,43,131,0,
    32,233,130,4,41,51,51,50,50,47,0,63,51,43,63,130,21,32,237,130,13,8,62,49,48,1,93,93,0,93,1,35,17,35,17,33,53,1,51,17,51,33,17,52,55,35,6,7,1,4,10,184,162,253,94,2,126,198,184,254,
    166,4,4,14,41,254,71,1,125,254,131,1,125,107,3,178,252,104,2,108,66,82,36,74,253,110,131,239,39,188,0,0,4,56,5,154,0,131,239,8,62,86,64,57,89,17,1,90,14,1,101,9,117,9,2,119,8,1,102,
    8,1,151,6,167,6,2,102,6,118,6,134,6,3,101,5,117,5,2,7,5,1,7,125,16,16,0,20,3,11,126,0,11,145,4,4,2,3,12,145,0,18,0,63,237,98,167,11,72,60,11,130,150,8,54,17,33,50,22,21,20,6,35,1,17,
    51,50,54,53,16,33,188,168,1,16,214,238,250,216,254,254,236,149,162,254,201,5,154,253,160,207,191,198,230,2,162,253,246,140,126,1,0,0,1,0,24,0,0,3,222,130,151,8,67,62,0,115,64,64,54,
    49,126,55,48,60,64,60,160,60,176,60,4,60,42,40,5,3,42,126,10,23,18,126,24,79,29,1,29,10,37,8,35,10,35,42,32,24,24,44,145,0,32,145,13,10,3,0,55,55,13,0,0,9,41,36,145,38,3,4,9,145,7,
    77,65,7,44,18,57,47,204,50,47,17,57,57,16,237,16,237,132,8,46,1,47,196,222,196,16,220,93,50,237,50,16,253,196,220,134,10,38,49,48,1,34,6,7,17,110,38,5,76,65,7,36,54,55,54,55,51,72,
    223,8,39,54,55,17,35,53,33,21,35,81,75,6,32,20,130,24,33,7,35,130,34,8,39,54,53,52,38,2,217,26,75,50,140,254,64,140,32,64,29,63,97,66,35,9,5,6,9,156,14,11,10,15,62,64,23,65,37,140,
    1,192,140,85,66,131,23,35,10,5,6,8,130,23,8,40,9,16,62,2,236,30,29,253,223,144,144,1,198,12,14,36,66,92,56,26,48,18,21,17,20,21,18,47,24,56,60,19,20,2,33,144,144,254,66,38,36,130,24,
    33,27,47,133,24,36,46,25,56,60,0,89,129,6,8,156,126,5,179,0,88,0,102,0,196,64,127,56,76,1,135,68,1,137,64,1,135,45,1,118,45,1,149,40,1,105,35,121,35,137,35,3,106,29,1,89,29,1,86,25,
    102,25,2,87,20,1,149,12,1,118,12,134,12,2,132,11,1,58,11,1,116,10,148,10,2,58,10,1,134,9,150,9,2,61,125,97,17,74,22,71,125,89,37,37,22,43,125,32,88,125,0,58,100,53,89,32,0,0,32,89,
    3,104,22,125,0,53,1,53,100,74,145,14,37,145,38,58,17,14,14,27,92,145,66,0,7,145,81,81,66,4,27,145,48,19,0,63,237,63,51,47,253,206,16,237,17,57,47,51,51,222,237,16,237,68,122,10,32,
    18,65,119,5,34,18,57,47,131,31,32,57,89,196,20,47,93,1,38,39,46,3,35,34,14,4,35,34,38,39,14,3,112,111,11,32,35,70,209,16,72,61,7,48,30,2,21,20,6,7,50,62,4,51,50,30,2,23,22,23,37,81,
    82,5,8,59,21,20,22,23,54,54,4,234,4,16,7,20,28,37,24,46,66,59,62,86,118,85,24,45,23,39,78,62,39,34,70,107,73,72,106,69,34,30,57,82,52,92,150,105,57,59,116,172,113,115,173,117,59,47,
    74,89,41,86,105,34,74,81,6,49,37,29,69,86,67,60,74,100,73,48,76,58,42,15,34,11,252,120,74,73,5,8,56,54,63,26,32,4,73,39,31,13,26,20,12,55,84,96,84,55,3,4,50,107,114,124,67,60,103,76,
    43,45,75,97,52,46,79,58,34,168,63,107,139,76,79,151,118,72,67,117,159,93,83,143,125,109,48,46,161,113,74,70,7,46,56,103,51,56,85,98,85,56,24,39,50,26,61,82,110,74,60,5,36,63,109,31,
    43,87,90,85,6,8,79,192,5,154,0,28,0,110,64,24,138,25,1,123,25,1,106,25,1,89,25,1,104,16,120,16,2,150,5,1,149,4,1,14,184,255,240,64,42,11,14,72,14,126,17,7,125,23,15,125,16,38,16,1,
    8,16,1,17,23,16,16,23,17,3,30,27,126,0,14,17,145,18,18,27,16,28,18,27,145,1,3,71,54,8,32,50,73,217,8,35,93,93,16,237,131,1,32,43,72,72,8,34,51,17,33,72,53,6,37,7,6,7,1,35,1,115,130,
    5,36,38,35,35,17,188,117,78,5,8,35,25,40,51,27,62,80,1,193,200,254,107,53,96,73,43,148,142,230,5,154,47,96,143,96,56,94,76,60,23,54,31,253,110,2,98,152,117,40,5,34,250,254,0,130,201,
    36,176,255,232,4,9,130,201,8,51,23,0,75,64,49,120,21,1,122,20,1,150,16,1,132,16,1,117,16,1,150,15,1,119,15,135,15,2,88,9,104,9,2,101,5,1,86,5,1,13,125,12,12,25,1,126,23,13,7,145,18,
    19,131,179,34,63,253,204,131,174,85,110,13,34,93,19,51,70,111,8,32,51,70,152,6,8,39,53,176,169,27,61,97,69,69,96,60,26,183,52,107,162,109,112,161,104,50,5,154,252,9,60,106,79,47,47,
    78,99,52,79,153,121,74,70,121,162,93,130,147,36,25,0,0,6,105,130,147,8,45,17,0,169,64,20,153,14,1,123,14,1,9,8,2,116,16,132,16,2,16,125,132,17,1,17,184,255,192,64,84,11,16,72,17,17,
    0,7,10,132,13,1,87,13,1,41,13,130,155,8,91,153,3,1,3,139,14,1,89,14,1,14,14,4,12,125,111,11,1,16,11,1,11,40,8,1,8,125,6,15,7,5,0,125,96,1,1,31,1,1,1,4,125,16,5,1,5,123,15,1,6,9,3,15,
    4,4,14,10,2,16,18,11,7,1,4,3,0,63,196,196,196,63,51,51,196,18,23,57,93,1,47,93,253,222,93,93,237,16,222,57,57,253,93,131,9,40,18,57,47,93,93,57,93,237,93,130,5,32,17,130,12,32,43,130,
    10,37,57,17,57,49,48,93,130,241,8,60,1,19,3,51,19,19,51,3,19,1,51,1,35,3,3,35,25,185,1,113,173,207,185,106,103,182,200,178,1,104,182,254,67,191,167,162,191,5,154,251,36,2,86,2,134,
    254,154,1,100,253,125,253,169,4,218,250,104,2,8,253,248,130,245,8,128,121,255,232,4,114,5,178,0,66,0,179,64,118,58,52,74,52,90,52,3,138,46,154,46,2,134,12,1,1,20,148,19,1,134,19,1,
    128,22,144,22,2,66,22,1,52,22,1,35,22,1,19,22,14,25,125,44,132,66,1,115,66,1,84,66,100,66,2,6,66,22,66,38,66,3,66,125,2,2,49,54,44,44,68,14,125,54,36,36,54,49,30,19,0,145,2,2,9,22,
    145,19,19,41,9,145,59,4,139,35,155,35,2,106,35,122,35,2,59,35,75,35,91,35,3,35,30,36,36,41,145,130,38,76,94,7,34,93,63,237,130,222,132,3,130,231,36,1,47,51,47,16,131,12,35,18,57,51,
    47,131,252,38,93,16,253,17,57,57,93,132,0,33,220,196,131,254,35,93,1,21,33,68,24,6,40,2,21,20,30,2,23,33,21,33,114,175,21,74,227,7,59,30,4,23,4,114,254,181,4,29,12,39,55,75,49,45,88,
    69,43,39,77,111,72,2,28,254,225,63,76,114,183,15,8,40,48,88,123,76,75,130,96,55,73,120,154,81,71,112,86,61,41,24,5,4,146,144,78,62,26,51,39,24,23,52,82,59,54,78,66,61,35,155,55,135,
    89,114,193,14,8,82,58,85,71,65,37,37,77,95,120,80,98,144,95,46,34,53,65,64,56,16,0,1,0,34,255,232,5,186,5,154,0,35,0,77,64,47,133,22,149,22,2,121,17,1,105,11,121,11,2,117,7,1,102,7,
    1,3,126,35,25,125,14,35,19,14,14,19,35,3,37,1,19,145,20,20,0,9,145,30,19,0,145,3,3,76,2,7,34,237,1,47,114,111,11,32,93,131,0,35,1,33,53,33,66,247,8,34,52,46,2,69,15,13,37,2,97,253,
    193,2,232,67,3,7,32,30,68,213,6,67,10,7,34,4,254,156,67,11,9,40,46,83,63,38,168,67,111,144,76,67,20,8,130,181,36,74,255,232,3,213,130,181,8,72,29,0,92,64,59,153,24,1,138,24,1,116,24,
    132,24,2,101,24,1,38,24,1,121,5,1,106,5,1,89,5,1,72,5,1,57,5,1,29,126,2,27,27,31,15,125,14,63,2,79,2,95,2,3,2,7,28,3,15,20,145,7,19,0,18,0,63,63,253,206,63,18,57,93,67,183,5,33,196,
    237,93,229,8,38,93,93,1,93,93,33,35,98,113,5,105,144,5,8,57,51,50,62,4,53,17,51,3,213,164,19,68,98,128,79,77,111,78,48,28,9,168,16,40,66,51,56,104,89,73,52,28,164,1,85,59,129,107,70,
    34,56,75,82,85,38,43,79,61,36,65,111,149,168,178,84,2,40,0,138,177,50,31,0,97,64,61,154,24,1,139,24,1,137,23,1,100,24,116,24,2,133,177,43,88,5,104,5,2,73,5,1,56,5,1,31,131,176,35,14,
    33,29,29,130,179,41,2,64,12,16,72,2,0,28,145,31,141,179,35,237,18,57,43,130,180,94,156,5,138,183,130,181,148,182,34,33,53,33,152,184,35,253,193,2,227,149,187,34,1,140,156,130,187,16,
    160,155,1,1,44,121,255,232,3,222,5,178,0,53,0,188,64,21,16,164,57,1,62,39,2,0,94,255,232,6,163,5,86,63,5,8,93,187,64,119,86,80,1,105,77,1,89,76,1,88,70,1,86,66,1,120,51,136,51,2,107,
    51,1,153,45,1,151,32,1,109,22,1,74,22,90,22,2,57,22,1,136,21,1,89,21,1,76,21,1,59,21,1,121,14,1,106,14,1,102,10,118,10,2,22,78,1,78,63,73,27,58,58,48,115,2,1,2,63,7,125,73,148,42,1,
    42,43,43,35,125,48,73,130,1,8,40,84,63,125,17,43,42,42,12,53,145,30,30,24,78,58,2,0,145,27,24,4,68,145,12,19,0,63,237,63,205,253,50,206,51,17,51,47,237,17,57,47,105,166,5,41,57,47,
    47,16,237,51,47,51,93,16,130,19,40,93,17,57,25,47,51,17,18,57,87,221,21,33,1,34,86,15,12,38,18,62,3,51,50,22,23,118,49,9,35,7,6,7,39,75,101,5,93,111,5,36,6,7,38,39,38,101,63,13,8,84,
    14,3,3,44,97,92,66,114,84,47,59,109,155,97,101,156,107,56,55,97,134,158,178,92,83,122,41,42,124,82,109,152,94,42,15,25,32,16,39,48,117,35,27,23,39,37,64,85,47,57,91,32,37,28,28,36,
    31,89,253,169,26,58,93,67,66,92,59,27,39,68,95,55,52,85,61,33,5,27,46,60,154,191,233,139,102,109,220,6,8,35,156,1,10,216,166,112,58,48,29,29,48,68,113,144,75,40,73,66,58,24,56,45,115,
    27,38,32,93,61,62,92,61,30,39,23,27,35,131,86,33,252,191,86,59,7,8,173,126,210,173,136,52,51,140,176,209,0,0,4,0,64,255,232,6,0,5,178,0,69,0,87,0,107,0,123,0,221,64,144,135,91,151,
    91,2,135,77,151,77,2,121,118,108,123,60,1,110,60,1,10,60,26,60,42,60,3,116,10,1,97,10,1,36,10,1,5,10,21,10,2,10,35,60,3,5,65,125,96,118,1,118,118,43,85,80,38,43,53,125,31,70,1,70,80,
    125,43,5,125,108,108,27,103,88,32,27,17,125,16,98,1,98,88,125,27,43,141,103,157,103,2,123,103,1,103,32,32,141,85,157,85,2,123,85,1,85,155,35,1,150,60,1,150,10,1,148,121,1,118,121,134,
    121,2,121,10,60,35,4,113,38,38,113,75,145,48,93,145,22,22,48,19,113,145,130,163,33,63,237,80,65,8,87,216,5,38,93,93,205,93,93,51,47,130,4,47,1,47,222,253,222,93,237,18,57,18,57,17,
    51,47,237,16,138,12,35,93,237,17,23,71,177,6,88,9,8,32,1,74,74,7,33,30,5,80,62,7,41,53,52,62,2,55,38,38,39,6,6,65,230,12,34,62,4,55,71,183,5,103,8,14,32,5,88,10,13,80,81,9,8,225,22,
    23,54,54,3,32,66,114,83,48,24,43,61,36,53,129,131,123,95,58,46,86,124,78,59,118,93,58,34,55,69,34,65,136,65,66,136,65,34,69,55,35,58,93,118,59,78,124,86,46,54,90,119,131,134,61,36,
    61,43,24,48,83,114,254,13,22,42,62,41,34,58,44,25,28,45,57,29,75,94,3,34,25,43,59,34,41,62,42,22,25,45,62,37,29,57,45,28,94,22,38,52,31,31,52,38,22,78,65,64,79,5,178,39,72,101,62,41,
    73,64,58,26,30,69,81,96,112,132,77,75,128,93,53,44,86,127,83,72,117,93,69,25,37,68,35,35,68,37,25,69,93,117,72,83,127,86,44,53,93,128,75,77,129,110,94,81,72,34,26,58,64,73,41,62,101,
    72,39,251,147,48,83,61,34,37,62,78,42,59,92,71,51,18,56,138,97,42,78,62,37,34,61,83,48,45,79,70,62,28,18,51,71,92,3,43,30,49,34,18,18,34,49,30,52,88,42,42,88,76,5,5,8,56,3,164,5,178,
    0,16,0,66,64,41,148,14,1,133,14,1,7,14,1,139,12,155,12,2,101,3,117,3,2,123,1,1,7,7,16,126,0,0,18,6,126,9,0,2,145,13,3,6,145,9,18,0,63,237,63,253,206,71,124,5,33,237,50,85,178,8,8,45,
    1,16,35,34,6,21,17,33,21,33,17,52,54,51,50,22,21,2,252,202,100,106,2,64,253,24,195,179,183,187,3,224,1,72,163,165,252,184,152,3,230,230,230,234,232,0,136,125,37,154,0,5,0,131,182,24,
    176,155,149,34,94,255,232,24,183,159,204,8,79,3,0,44,255,232,5,128,5,154,0,33,0,41,0,58,0,104,64,61,138,37,1,121,37,1,107,37,1,132,35,1,119,35,1,101,35,1,154,31,1,150,28,1,24,23,21,
    126,18,40,40,0,60,6,125,53,0,41,17,126,42,0,11,145,48,17,41,24,0,145,42,21,17,17,19,36,145,30,19,19,3,111,43,6,41,196,196,253,196,196,16,220,237,1,47,130,8,130,7,34,17,18,57,130,9,
    32,205,96,158,10,34,1,35,34,86,34,5,44,51,50,30,2,21,21,33,17,51,17,51,21,35,67,179,5,42,38,53,51,16,51,50,54,53,53,33,39,103,137,7,8,191,21,20,30,2,51,1,218,118,74,115,81,42,60,90,
    105,46,77,112,73,35,1,152,168,190,190,50,96,139,89,183,187,168,202,100,106,254,104,168,21,35,47,26,28,49,37,21,16,34,56,40,2,60,39,73,106,67,76,105,66,30,53,85,109,55,108,2,198,253,
    58,152,136,115,173,115,57,234,232,254,184,163,165,130,152,113,38,55,36,17,18,33,47,28,27,48,37,21,0,0,2,0,24,255,232,6,158,5,178,0,60,0,80,0,192,64,72,150,79,1,151,78,1,72,74,1,152,
    73,1,55,64,1,73,59,89,59,105,59,3,137,40,1,117,27,1,86,27,102,27,2,120,20,1,89,15,105,15,121,15,3,103,11,119,11,2,85,11,1,118,6,1,76,61,71,157,23,1,23,32,14,17,72,87,45,1,146,3,1,3,
    184,255,224,64,51,130,14,43,3,45,23,3,18,8,125,71,55,125,56,71,130,1,61,82,61,125,18,33,35,125,32,30,18,3,0,145,48,48,42,56,56,35,33,145,30,76,23,25,145,45,42,4,66,104,208,5,36,63,
    51,253,50,50,116,161,11,53,220,205,253,205,16,237,18,57,57,47,47,16,237,16,237,17,23,57,43,93,93,43,68,253,5,32,93,140,0,108,29,15,34,62,2,55,90,245,5,36,51,21,33,53,51,93,27,7,41,
    54,51,50,30,4,21,35,52,46,2,69,5,12,38,14,3,4,252,59,116,57,116,167,13,42,110,64,113,113,65,94,63,30,140,254,64,116,167,6,39,80,175,85,85,178,85,77,123,116,167,6,34,95,253,114,116,
    167,9,46,90,53,53,88,65,36,5,40,36,35,62,159,192,224,128,70,204,7,33,128,226,116,169,29,93,0,19,33,2,0,24,177,31,201,8,97,1,0,94,255,232,5,174,5,178,0,47,0,127,64,84,106,36,122,36,
    2,106,30,122,30,2,85,15,1,54,15,70,15,2,4,15,1,135,14,1,86,14,1,87,10,1,6,9,1,149,8,1,138,4,1,47,45,23,23,45,125,0,2,2,49,12,125,15,33,1,33,47,145,46,1,7,145,38,4,91,23,1,58,23,74,
    23,2,27,23,43,23,2,23,28,22,22,17,145,28,24,65,149,10,51,63,253,222,196,237,1,47,93,237,18,57,47,205,237,50,47,16,205,49,48,66,36,10,36,1,53,51,46,3,89,194,11,34,55,21,14,130,15,113,
    205,6,8,130,54,51,50,30,4,23,51,21,3,108,199,8,41,72,107,73,131,190,124,59,82,148,206,124,54,116,115,110,49,45,113,122,127,59,157,254,249,189,105,89,172,254,165,91,140,106,75,49,26,
    5,188,4,2,144,12,45,45,34,93,162,222,129,122,210,154,87,14,29,44,29,163,25,38,27,14,107,191,1,5,155,164,1,25,206,117,36,55,68,64,53,12,144,0,0,1,0,32,0,0,6,61,5,154,0,8,0,63,64,38,
    117,1,133,1,149,1,3,70,1,86,1,102,1,3,1,7,126,2,0,125,8,2,130,1,53,10,4,8,3,145,6,3,150,7,1,7,2,18,0,63,51,93,63,237,196,1,47,67,0,8,8,35,50,93,93,49,48,1,1,35,17,33,53,33,17,1,6,61,
    252,203,168,253,192,2,232,2,118,5,154,250,102,5,2,152,251,152,4,104,70,87,5,33,5,17,130,107,8,78,23,0,98,64,30,23,5,39,5,2,73,0,89,0,2,155,17,1,137,17,1,104,17,120,17,2,8,126,9,14,
    126,0,16,1,16,184,255,192,64,27,19,22,72,16,16,20,25,23,18,126,20,22,3,16,19,18,17,14,18,145,23,3,9,14,145,3,16,0,63,253,206,16,221,237,17,57,63,196,63,90,12,7,35,43,93,253,222,78,
    114,6,35,1,93,93,1,74,57,6,40,35,52,46,2,35,17,35,17,5,130,3,8,43,51,17,2,200,56,132,72,75,120,85,45,146,15,37,62,47,168,254,46,168,168,3,215,27,38,47,87,126,79,32,66,54,35,252,128,
    3,88,232,253,144,5,154,253,141,133,175,33,4,162,24,184,129,214,38,2,0,94,255,232,3,216,130,221,8,65,20,0,40,0,92,64,60,106,24,122,24,2,106,15,122,15,2,123,9,1,106,9,1,114,5,1,101,5,
    1,103,4,1,21,21,1,126,80,20,1,20,20,42,31,125,15,12,31,12,2,12,26,145,133,20,149,20,2,20,17,17,0,36,145,7,19,0,70,110,7,33,51,93,66,230,7,34,93,237,51,72,126,8,38,93,1,51,17,20,14,
    2,70,104,9,33,22,23,105,50,5,24,70,82,9,34,3,48,168,118,152,11,34,83,138,55,118,124,15,38,5,154,252,24,101,168,122,118,126,8,35,44,41,254,139,118,99,18,130,211,8,190,7,59,5,178,0,55,
    1,32,64,182,85,48,1,7,48,1,85,42,1,7,42,1,143,32,159,32,2,110,32,126,32,2,150,24,1,133,24,1,116,24,1,54,24,1,121,20,1,107,20,1,41,20,1,26,20,1,122,14,1,107,14,1,41,14,1,27,14,1,149,
    10,1,134,10,1,117,10,1,54,10,1,128,7,144,7,2,113,7,1,109,2,1,32,27,33,29,31,28,30,33,28,33,28,30,26,21,25,37,25,2,6,25,1,25,34,125,35,35,55,2,7,1,5,3,6,4,1,6,1,6,4,21,9,37,9,2,6,9,
    1,9,8,0,125,16,55,32,55,2,55,55,17,57,30,30,4,4,57,45,125,64,15,17,1,17,33,25,28,3,35,29,26,128,32,35,40,145,64,22,4,1,9,6,3,55,5,8,128,2,55,50,145,12,110,63,5,42,26,220,196,18,23,
    57,63,26,253,222,196,133,10,56,1,47,93,26,237,17,51,47,51,47,17,18,57,47,93,237,50,50,93,93,17,57,57,25,47,131,15,32,57,130,9,36,17,51,24,47,237,130,20,32,50,140,21,68,104,12,70,152,
    12,44,93,1,23,55,51,19,35,39,7,35,39,6,4,24,205,228,10,33,50,4,130,23,130,2,32,3,131,26,68,151,14,8,157,5,113,74,75,163,146,157,70,69,170,53,81,254,226,204,158,248,171,90,90,171,248,
    158,204,1,30,81,58,165,69,70,157,146,163,75,74,170,18,84,122,152,86,124,185,122,60,60,122,185,124,86,152,122,84,18,1,211,168,168,254,240,161,161,108,167,160,116,200,1,14,155,154,1,
    15,200,116,161,166,108,161,161,254,240,168,168,76,125,90,49,94,160,215,121,122,215,160,93,49,90,125,76,0,1,0,94,255,232,6,120,5,178,0,54,0,164,64,104,70,45,1,55,45,1,151,44,1,54,44,
    70,44,2,92,35,1,90,30,1,9,30,1,121,29,1,10,29,106,29,2,25,40,40,54,3,5,126,1,54,14,130,1,8,71,56,47,125,15,32,1,32,149,39,1,134,39,1,117,39,1,102,39,1,53,39,69,39,85,39,3,20,39,36,
    39,2,39,37,40,40,42,5,3,145,0,0,52,42,145,37,4,54,52,145,27,14,6,9,145,15,20,20,25,27,19,0,63,51,51,47,51,237,50,50,16,237,50,100,237,5,32,196,95,132,7,34,93,93,93,69,190,5,41,57,47,
    47,16,206,253,205,18,57,47,95,128,12,48,35,53,33,21,35,21,22,22,51,50,54,55,54,55,21,6,7,90,55,5,33,39,6,65,87,8,119,147,15,61,55,4,68,140,1,192,140,42,99,60,38,70,27,32,28,28,33,28,
    70,37,53,81,68,59,31,179,209,158,254,178,119,150,13,58,69,134,198,130,169,122,1,120,144,144,182,31,36,15,9,11,14,156,12,10,8,14,10,19,28,19,76,119,146,20,8,146,67,0,3,0,64,255,232,
    8,69,5,178,0,73,0,93,0,113,0,248,64,165,68,107,1,37,107,53,107,2,151,101,1,153,96,1,135,81,1,102,63,118,63,2,85,63,1,121,59,1,104,59,1,90,59,1,101,46,1,121,37,1,133,28,1,146,27,1,133,
    27,1,102,27,1,85,27,1,135,19,1,118,19,1,154,10,1,154,9,1,89,9,1,75,2,1,60,2,1,84,79,89,125,51,48,15,18,72,91,51,1,128,71,144,71,2,98,71,114,71,2,84,71,1,69,71,1,52,71,1,39,71,1,71,
    32,51,3,56,66,104,125,12,111,24,125,94,5,94,12,130,1,8,36,79,115,42,125,43,56,125,79,84,51,43,48,145,35,111,145,5,5,99,71,0,145,29,29,32,35,19,99,145,17,74,145,61,61,17,4,0,78,126,
    5,61,63,51,51,47,237,50,17,57,47,237,16,253,206,51,51,1,47,253,222,237,17,18,57,57,47,47,17,51,16,237,130,2,32,222,78,112,9,33,43,237,87,36,5,66,219,13,135,13,56,37,50,62,2,55,46,5,
    53,52,62,2,51,50,30,4,21,20,2,6,4,35,34,36,39,6,126,23,9,34,51,50,54,78,128,6,130,33,40,2,21,20,14,2,7,22,22,1,98,166,6,37,23,62,3,53,52,46,126,59,11,8,38,4,23,54,54,5,122,106,169,
    128,88,24,16,83,107,116,97,63,44,81,113,70,66,108,83,60,39,18,103,187,254,248,161,129,254,251,118,100,213,97,126,52,9,38,65,71,147,71,71,119,86,126,104,8,8,74,40,72,99,58,91,195,254,
    75,66,92,59,27,40,71,99,60,47,80,58,33,27,59,92,3,244,26,51,78,52,36,53,35,17,34,56,72,76,75,30,2,3,127,71,121,164,93,2,16,40,70,110,157,107,83,140,101,56,51,91,125,147,165,85,177,
    254,213,219,123,75,74,74,75,38,66,89,126,85,6,38,51,51,62,155,184,213,120,73,218,7,8,45,116,205,179,152,62,54,55,4,155,69,104,121,52,104,183,159,133,53,55,134,159,182,102,52,121,104,
    69,254,15,106,181,134,76,38,61,79,42,69,106,79,56,37,21,5,24,48,114,27,10,8,100,19,0,43,0,65,0,154,64,104,121,55,1,99,47,1,84,47,1,87,46,1,107,42,1,89,42,1,72,41,88,41,136,41,3,102,
    22,1,117,18,1,117,12,1,149,11,1,122,8,1,122,2,1,154,1,1,155,52,1,90,52,106,52,2,52,37,15,125,48,39,1,39,39,67,64,149,25,1,132,25,1,69,25,85,25,101,25,3,54,25,1,25,44,125,5,25,145,63,
    52,145,36,63,130,1,38,49,20,145,10,4,49,145,24,76,186,8,75,68,6,36,1,47,237,50,93,130,0,39,50,18,57,47,93,237,51,51,96,156,17,36,5,34,38,38,2,122,225,6,55,22,22,18,21,20,2,6,6,3,34,
    14,2,7,51,50,30,2,23,30,3,51,51,54,53,75,66,8,8,74,54,55,35,34,46,2,39,46,3,35,35,6,2,254,158,249,173,92,91,177,1,4,168,156,245,169,90,89,174,255,154,77,131,109,85,31,169,59,88,68,
    55,27,20,36,43,57,41,249,17,58,121,188,253,131,63,125,186,122,158,213,60,205,62,90,69,55,26,20,34,42,54,40,214,16,24,24,76,174,13,8,70,152,163,254,234,203,115,5,50,38,68,97,58,35,69,
    103,68,51,86,62,35,85,91,126,217,160,91,253,177,118,213,162,95,129,112,41,76,108,68,51,80,56,29,76,0,0,2,0,94,255,232,6,127,5,178,0,59,0,79,0,183,64,124,156,67,1,58,67,138,67,2,41,
    130,7,8,66,58,74,58,2,137,57,1,154,53,1,105,53,1,139,52,155,52,2,122,52,1,75,52,1,58,52,1,102,48,1,69,48,1,51,48,1,70,40,1,105,31,121,31,2,90,31,1,154,24,1,84,19,1,119,18,1,85,13,1,
    6,38,125,70,16,125,55,70,130,1,8,48,81,63,131,45,1,86,45,1,68,45,1,22,45,38,45,2,45,60,125,15,28,1,28,63,145,45,45,75,50,145,21,19,75,145,33,33,5,0,145,6,11,4,0,63,51,237,50,51,47,
    100,104,5,35,237,1,47,93,65,112,8,65,130,5,33,253,206,70,46,22,32,1,122,57,10,34,50,30,2,67,70,6,33,46,4,67,87,6,130,16,41,14,4,7,30,3,51,50,54,54,18,65,129,5,35,22,23,62,5,78,51,7,
    8,244,4,139,49,81,30,35,28,28,35,30,81,49,97,180,139,84,110,202,254,226,176,132,221,177,132,89,44,57,105,149,93,70,113,81,44,57,90,111,109,94,28,32,105,143,182,110,130,219,159,90,52,
    89,117,252,60,8,8,32,79,81,76,58,36,17,35,53,36,44,84,68,41,5,26,20,11,13,17,179,9,8,6,11,72,151,234,161,208,254,190,220,114,65,119,165,201,230,123,132,213,152,82,56,101,140,83,98,
    149,108,72,45,23,4,94,165,122,71,96,184,1,14,173,116,172,113,55,254,100,45,90,45,6,21,37,56,79,105,68,42,79,61,38,46,99,157,0,0,2,0,64,255,232,4,224,5,178,0,36,0,56,0,107,64,69,87,
    44,1,88,40,1,103,35,1,136,26,1,122,26,1,117,22,1,135,21,1,118,14,1,103,14,1,105,10,121,10,2,52,47,17,125,106,2,1,2,7,37,37,58,47,125,7,31,125,32,7,100,52,116,52,148,52,3,52,32,2,0,
    145,24,19,42,145,12,4,0,63,237,63,253,50,206,51,93,1,47,222,237,90,212,6,33,93,237,101,170,12,32,93,24,66,180,15,32,4,24,66,159,39,37,50,94,134,170,202,114,68,86,9,32,2,24,66,139,31,
    37,114,232,217,189,141,81,68,37,9,24,66,123,19,8,60,2,0,110,255,232,3,254,5,178,0,22,0,34,0,174,64,59,122,20,1,118,10,1,41,20,57,20,2,6,15,21,23,154,9,9,21,29,154,15,25,1,1,3,154,21,
    7,1,64,12,0,77,1,1,12,8,13,6,77,12,8,12,131,4,32,11,130,4,38,137,32,32,48,32,2,32,98,246,6,130,7,33,232,179,130,27,130,7,34,228,64,25,130,31,36,32,32,36,26,6,131,50,32,6,131,50,33,
    6,8,130,18,38,6,137,18,184,255,212,179,130,70,130,7,32,214,131,45,130,7,33,216,179,130,25,37,18,47,43,43,43,233,130,3,34,50,18,57,131,10,32,93,132,11,38,47,43,0,63,237,50,47,71,148,
    5,55,17,18,57,49,48,1,93,93,93,1,21,38,35,34,2,17,51,54,51,50,22,21,20,2,131,11,8,76,16,0,33,50,1,34,6,21,20,22,51,50,54,53,52,38,3,166,111,123,191,229,4,100,228,188,222,254,196,216,
    246,1,66,1,8,150,254,242,127,158,157,130,124,152,146,5,137,155,57,254,172,254,227,203,241,202,211,254,245,1,88,1,55,1,121,1,194,253,68,172,117,147,208,181,139,152,172,111,71,10,8,84,
    125,1,23,64,172,106,88,1,89,88,1,134,78,1,117,78,1,102,78,1,41,69,1,89,68,1,58,68,74,68,2,43,68,1,137,58,1,123,58,1,133,54,1,115,54,1,117,48,149,48,2,89,29,105,29,2,136,28,1,121,9,
    1,39,82,1,25,54,1,7,48,1,6,23,1,147,38,1,132,38,1,38,39,99,100,39,130,1,8,91,80,61,90,125,0,0,26,15,126,90,51,125,26,47,90,1,90,26,90,26,71,127,155,76,1,76,77,77,80,125,48,71,1,71,
    0,125,125,115,91,94,145,105,105,115,77,76,76,115,39,38,38,31,15,12,145,115,120,100,99,99,7,145,120,110,110,0,120,16,120,2,120,120,31,90,85,145,66,16,21,145,56,56,61,66,19,31,145,46,
    4,0,63,237,71,68,5,40,16,237,50,17,57,47,93,51,47,130,8,43,47,51,16,220,237,50,18,57,47,51,17,51,132,3,130,27,130,8,39,1,47,93,237,50,47,51,93,71,97,5,34,93,16,237,97,193,5,33,17,57,
    71,113,6,111,141,5,24,69,125,15,71,97,6,34,1,6,7,126,68,9,111,154,67,36,38,38,35,34,6,87,233,7,33,51,50,119,12,5,133,7,40,54,55,54,55,6,24,44,41,18,130,35,40,14,17,51,52,45,11,12,51,
    29,111,200,65,62,27,50,15,16,48,23,27,30,84,44,42,36,77,29,17,50,51,45,13,15,46,50,48,17,16,49,50,46,15,15,49,130,25,47,2,203,38,30,13,24,19,11,30,35,30,35,20,254,166,41,111,245,63,
    46,94,19,33,30,18,21,27,100,37,30,26,42,29,34,29,130,2,34,30,36,30,131,17,8,111,0,0,2,0,10,255,106,4,130,5,178,0,40,0,66,0,154,64,96,84,64,1,70,64,1,55,64,1,137,60,1,121,59,1,90,51,
    1,89,48,1,105,47,1,106,39,1,104,38,1,118,15,1,102,11,118,11,2,151,3,1,7,2,1,7,54,5,125,57,57,13,125,0,50,1,50,50,35,68,54,21,41,126,30,29,24,48,35,1,35,8,7,55,145,53,53,62,35,42,45,
    21,24,29,3,18,30,30,45,145,22,18,19,62,87,243,6,56,206,237,50,47,18,23,57,18,57,57,17,57,47,237,57,57,1,47,93,196,205,50,253,196,206,76,106,5,33,51,47,73,99,18,39,1,50,30,2,21,16,5,
    21,24,79,182,7,53,38,39,21,35,53,38,38,39,38,39,55,22,23,22,22,23,17,52,62,2,3,17,113,169,6,38,16,33,35,53,51,32,17,104,20,6,8,167,2,194,81,145,108,64,254,224,72,124,91,51,72,126,174,
    102,70,126,57,168,60,93,32,37,27,83,18,25,21,62,40,74,123,162,191,57,130,73,67,110,79,43,254,113,99,93,1,99,36,63,87,52,47,96,78,50,5,178,45,87,127,83,254,229,81,4,5,54,90,121,72,106,
    161,108,55,16,14,156,213,26,54,23,26,25,142,18,19,17,43,22,3,4,108,167,113,58,254,35,252,211,19,22,30,64,100,70,1,25,139,1,8,61,88,56,26,40,81,122,0,0,2,0,20,255,237,5,13,5,154,0,52,
    0,70,0,146,64,87,102,50,1,55,50,1,56,38,1,104,36,120,36,2,70,28,1,39,28,1,45,47,126,43,41,53,53,0,126,19,41,130,1,8,78,11,72,41,35,1,35,34,36,43,31,1,32,31,30,33,126,34,34,36,126,31,
    30,79,30,2,30,30,61,126,11,58,145,19,16,16,66,25,145,48,41,47,42,145,44,3,35,34,31,18,66,145,6,19,0,63,237,63,204,57,63,253,196,222,50,237,17,57,47,51,237,1,47,237,51,47,93,237,50,
    47,130,14,35,57,93,17,18,132,3,32,57,91,25,5,34,16,206,253,82,13,8,32,1,79,38,15,71,65,7,40,7,3,35,3,51,23,19,62,3,24,87,37,9,89,15,5,114,212,6,8,73,62,2,5,13,48,84,111,63,63,109,80,
    45,43,79,109,66,38,69,31,22,54,91,69,63,100,75,52,17,252,175,135,155,63,200,20,63,93,124,81,140,1,192,140,84,116,73,33,168,21,38,50,29,60,79,21,38,51,29,29,50,38,21,2,181,254,100,65,
    109,81,45,45,81,109,65,131,65,8,46,16,14,158,64,101,71,37,46,75,94,48,253,52,1,107,193,2,74,58,117,98,71,12,178,144,144,179,13,73,111,141,254,20,43,64,42,21,85,85,42,65,44,22,22,44,
    65,0,120,155,8,8,75,178,0,38,0,107,64,67,156,34,1,39,26,1,53,18,1,39,18,1,58,14,1,40,14,1,70,8,86,8,2,84,7,1,69,7,1,74,3,1,89,2,1,74,2,1,10,126,11,11,29,126,31,31,38,40,22,36,126,38,
    36,34,145,22,10,24,24,16,31,37,18,16,145,5,4,0,63,107,230,5,35,206,51,237,50,81,220,7,33,237,51,121,118,12,34,93,93,93,24,83,116,8,32,35,66,88,6,32,21,103,144,6,8,70,17,35,17,52,35,
    34,7,17,35,201,75,132,180,105,100,180,135,80,168,54,91,118,64,67,118,88,51,229,197,90,145,102,56,168,245,178,228,168,3,230,105,170,120,65,63,119,172,109,72,119,86,48,53,90,119,66,254,
    238,122,48,93,140,91,254,44,1,204,232,110,253,186,131,217,8,172,94,255,232,6,182,5,178,0,60,0,227,64,98,113,56,129,56,145,56,3,150,52,1,38,51,1,55,50,1,37,50,1,25,46,1,138,45,1,121,
    45,1,107,45,1,57,45,1,44,45,1,27,45,1,122,44,1,25,44,105,44,2,122,39,1,105,39,1,155,38,1,85,18,1,6,18,1,87,13,151,13,2,6,12,86,12,2,137,7,1,29,31,126,25,2,55,54,0,56,1,58,60,57,54,
    1,57,57,1,54,3,59,3,184,255,192,64,44,9,12,72,3,3,27,25,59,25,59,62,15,125,64,15,42,1,42,1,54,57,3,3,58,55,128,0,3,3,28,10,145,47,4,28,145,31,26,25,20,145,32,37,19,0,63,51,253,50,222,
    196,237,63,237,17,57,47,80,42,11,49,18,57,57,47,47,206,51,47,43,18,23,57,25,47,47,47,17,57,67,223,5,34,57,24,16,66,112,9,77,52,15,35,1,39,7,35,99,106,11,35,51,50,62,2,66,114,6,32,17,
    84,146,14,37,2,23,22,23,55,51,130,2,8,138,3,5,159,75,74,148,28,53,23,59,77,95,58,120,180,119,60,71,139,206,135,42,88,84,76,31,140,1,192,140,51,124,134,137,64,163,254,251,183,99,82,
    164,246,163,66,113,93,75,29,68,41,58,155,69,70,132,126,3,239,168,168,84,66,28,54,42,26,89,158,220,130,128,214,154,87,15,24,31,17,182,144,144,254,241,35,55,38,21,108,192,1,9,157,175,
    1,25,198,106,25,40,52,27,63,80,108,161,161,254,240,0,3,0,188,0,0,4,47,5,154,0,15,0,23,0,31,0,199,64,10,121,13,1,118,11,1,115,4,1,4,16,210,197,1,16,58,0,6,98,0,224,7,66,0,19,0,15,181,
    10,175,0,5,175,15,0,47,237,1,47,237,49,48,17,79,17,5,33,2,21,83,152,6,39,18,30,41,23,23,41,30,18,135,7,33,6,210,139,13,130,21,41,0,2,0,0,5,254,1,168,7,166,130,77,52,39,0,24,64,10,30,
    10,175,20,0,25,15,175,35,5,0,47,205,254,205,1,131,4,35,49,48,19,20,71,178,5,68,186,8,142,104,32,100,143,105,42,100,34,57,77,43,43,77,59,34,34,59,131,7,33,57,34,144,122,50,20,43,76,
    57,33,33,57,76,43,43,78,59,35,35,59,78,0,0,1,130,2,46,252,0,164,3,157,0,3,0,14,180,3,126,1,3,1,130,131,132,216,42,19,35,17,51,164,164,164,1,252,1,161,130,36,39,0,0,2,205,0,124,5,154,
    132,39,38,2,0,3,1,3,0,63,131,169,44,49,48,17,17,51,17,124,2,205,2,205,253,51,134,39,33,2,125,130,39,44,7,0,23,64,9,4,6,2,0,7,6,1,3,130,44,33,221,196,130,46,37,206,221,205,49,48,1,121,
    116,6,39,1,0,255,0,2,125,254,255,130,59,36,81,124,124,253,175,132,61,42,1,188,1,221,3,213,0,7,0,21,183,131,56,33,5,7,132,144,35,221,206,1,47,130,64,130,59,33,33,21,130,150,47,21,33,
    1,221,254,159,124,124,1,97,2,138,206,2,25,207,130,156,32,0,24,217,121,81,38,2,0,40,0,0,5,194,132,243,8,52,6,0,90,64,49,4,5,4,6,5,126,1,2,20,1,2,128,1,176,1,224,1,3,1,1,2,2,3,6,4,4,
    126,3,0,20,3,0,4,3,39,0,1,0,0,3,2,3,18,5,6,145,1,0,130,245,56,196,253,196,63,196,1,25,47,51,24,47,93,16,0,193,135,5,43,125,16,196,17,1,51,17,131,16,34,135,43,8,130,13,61,49,48,19,33,
    1,35,19,1,33,40,5,154,253,61,20,10,1,202,252,108,5,154,250,102,1,85,3,161,255,255,133,131,8,37,7,166,2,38,1,87,0,0,1,7,1,79,2,33,0,0,0,23,64,13,3,2,0,27,37,0,1,37,3,2,42,5,38,0,43,
    53,53,1,130,3,33,0,0,141,181,37,94,64,50,5,6,6,132,179,37,6,4,1,2,40,2,130,176,38,1,6,4,3,0,6,0,134,180,34,143,3,191,130,176,32,3,130,91,34,5,4,145,130,185,133,182,32,63,130,184,131,
    182,134,167,132,186,133,185,130,200,36,135,8,16,43,5,132,185,43,1,51,1,33,37,33,1,2,235,20,2,195,130,177,36,3,3,148,254,54,131,187,35,164,3,161,0,136,187,32,66,136,187,34,78,2,133,
    130,187,38,19,64,11,2,0,7,17,130,186,33,2,22,132,185,130,184,131,183,36,100,0,0,5,254,65,109,6,34,99,64,54,65,109,9,46,1,2,4,6,0,3,4,3,6,126,0,3,20,0,0,132,174,33,8,5,130,213,33,0,
    56,130,207,34,39,3,55,131,17,51,1,3,23,0,1,0,18,0,63,93,63,57,25,47,93,51,93,1,24,47,130,196,45,17,18,57,47,196,135,4,16,43,16,1,193,135,4,130,180,34,135,24,16,65,118,6,45,51,17,1,
    21,37,1,17,100,5,154,254,171,252,95,65,124,9,131,185,33,70,0,130,139,32,7,131,185,38,89,0,0,1,6,1,78,130,16,134,183,34,1,5,37,138,183,130,229,140,43,33,0,38,131,43,37,1,7,1,78,2,193,
    130,235,41,34,64,22,3,0,27,37,1,2,37,131,242,130,58,35,3,42,5,38,134,62,33,43,53,130,248,33,43,53,130,65,134,249,34,2,38,1,131,109,36,7,1,80,1,192,130,59,45,17,64,9,2,184,9,9,6,6,37,
    2,8,0,47,131,44,131,153,143,43,37,78,1,172,251,251,0,131,43,34,164,7,7,131,43,32,22,134,43,66,9,14,34,90,64,46,66,9,11,66,3,11,60,3,0,4,5,126,3,2,2,8,1,0,39,1,1,1,0,0,2,3,18,2,3,0,
    63,63,18,57,25,47,65,72,5,36,18,57,47,196,253,65,71,13,56,8,24,16,43,4,125,16,196,49,48,19,53,1,17,3,17,1,40,5,154,164,252,95,2,195,66,11,10,66,3,6,34,224,7,66,130,219,39,94,0,0,1,
    7,1,78,5,130,218,32,0,65,75,6,33,5,2,65,75,14,38,10,0,0,5,194,5,154,130,45,32,85,133,45,33,0,10,130,221,44,19,185,0,2,255,226,182,7,7,0,0,37,2,135,223,135,91,139,45,33,5,0,65,11,6,
    36,30,17,17,1,1,140,43,137,89,67,71,5,32,78,141,89,33,3,3,140,45,32,40,130,135,32,224,132,89,134,45,139,89,33,2,2,140,43,132,89,132,225,130,43,37,0,39,1,78,2,133,138,187,41,35,185,
    0,3,255,226,64,18,27,27,131,98,66,247,5,33,3,42,66,2,6,32,47,66,2,5,67,43,7,65,39,5,142,69,132,121,38,32,64,20,3,30,37,37,131,121,149,66,34,255,255,0,130,135,35,6,232,5,154,130,129,
    34,89,0,234,138,129,54,40,64,16,2,64,17,1,48,17,1,32,17,1,16,17,1,0,17,1,184,255,156,182,65,33,11,32,93,131,0,66,137,6,33,7,66,130,67,66,137,7,34,78,6,98,65,81,6,34,100,7,7,145,247,
    35,6,232,7,66,135,111,130,7,55,78,0,10,251,251,1,7,1,78,1,48,0,0,0,56,64,23,3,0,27,37,1,5,37,146,126,33,64,11,132,127,67,14,5,38,0,47,53,43,53,1,43,133,133,32,43,131,135,36,70,0,0,
    7,66,130,91,132,135,131,89,131,135,67,135,6,34,32,64,20,135,87,133,148,140,68,130,73,67,147,27,34,254,218,0,135,63,67,147,10,65,142,13,65,75,12,34,94,1,38,65,75,10,41,48,64,22,2,96,
    17,1,80,17,1,65,81,35,33,93,93,131,203,32,40,130,203,37,6,5,154,0,38,1,67,1,6,33,6,38,65,83,27,33,7,6,130,247,33,39,1,132,119,65,83,11,32,6,130,135,35,0,64,64,29,131,251,32,5,130,187,
    151,134,65,89,24,130,141,32,43,136,143,130,99,132,143,131,97,131,143,67,153,8,65,29,6,131,97,65,93,24,130,209,32,5,67,211,8,131,65,33,5,0,65,35,5,33,2,133,65,95,16,130,72,66,238,16,
    8,85,1,0,100,2,205,4,76,5,154,0,44,0,85,64,53,55,30,1,248,21,1,217,21,233,21,2,214,17,230,17,246,17,3,248,8,1,217,8,233,8,2,247,4,1,230,4,1,215,4,1,0,43,11,26,24,35,13,11,19,32,32,
    35,6,38,25,0,12,3,0,63,196,196,220,205,51,51,47,205,1,47,205,57,220,205,16,220,77,253,8,35,93,93,93,19,109,226,8,33,17,51,140,10,33,14,2,24,86,205,8,48,2,53,17,224,16,37,60,44,44,60,
    37,16,124,17,37,60,43,132,8,8,33,39,73,104,65,72,111,36,37,110,72,65,104,73,39,5,154,254,115,45,76,54,30,29,53,76,47,1,141,254,115,47,76,53,29,135,11,48,72,118,84,46,54,50,50,54,46,
    84,118,72,1,141,0,0,4,131,213,33,3,180,130,213,61,19,0,39,0,59,0,79,0,45,64,20,30,10,20,0,0,70,50,60,40,50,75,45,65,55,55,35,5,15,25,130,186,38,221,222,205,50,47,205,222,130,186,34,
    222,205,16,133,10,33,49,48,98,176,11,36,35,34,14,2,7,73,240,14,103,199,11,147,31,33,2,112,74,17,32,33,254,188,160,34,33,4,198,74,175,14,74,52,15,32,253,158,31,8,85,0,2,0,100,2,205,
    3,48,5,154,0,3,0,6,0,110,64,76,57,6,1,39,6,1,6,231,0,247,0,2,216,0,1,0,232,1,248,1,2,1,1,152,5,1,5,159,2,223,2,239,2,255,2,4,112,2,128,2,2,47,2,63,2,2,25,2,1,11,2,1,2,40,4,1,4,32,3,
    48,3,144,3,208,3,4,3,157,130,63,50,1,4,3,1,3,0,63,221,205,17,57,93,1,47,93,51,93,205,93,131,0,33,50,93,71,239,5,130,16,60,93,49,48,1,51,1,33,55,33,3,1,197,10,1,97,253,52,199,1,62,159,
    5,154,253,51,124,1,66,0,130,120,130,151,33,2,79,132,151,37,43,64,25,239,1,255,130,133,35,167,2,1,121,130,109,39,2,5,224,3,240,3,2,3,130,37,74,216,6,36,93,18,57,47,93,130,98,34,49,48,
    19,130,83,37,100,1,100,135,254,156,74,229,9,141,73,36,0,255,0,2,120,130,95,45,167,3,1,3,3,5,224,2,240,2,2,2,1,0,130,77,36,63,205,1,47,205,132,73,130,171,130,156,130,73,37,1,200,254,
    156,135,1,131,63,34,205,253,51,134,147,32,225,130,147,58,25,0,43,64,23,185,20,1,170,20,1,182,16,1,165,16,1,25,23,23,27,12,10,18,5,24,12,130,145,33,196,220,131,73,130,146,66,228,6,32,
    1,76,109,6,32,53,66,222,12,8,43,2,225,44,82,118,75,74,118,82,44,126,23,48,72,49,49,73,48,23,126,4,51,74,131,97,56,56,97,131,74,1,103,254,153,47,85,64,38,38,64,85,47,1,103,65,15,7,133,
    123,55,41,64,22,166,20,182,20,2,185,16,1,170,16,1,10,12,12,27,23,25,12,25,18,5,130,122,37,205,220,196,1,47,205,136,122,80,43,8,33,17,35,82,134,7,35,21,17,35,100,146,120,32,52,135,120,
    35,254,153,1,103,135,120,33,254,153,133,243,33,3,49,132,243,60,47,64,27,138,10,154,10,2,120,10,1,138,4,154,4,2,120,4,1,20,7,7,27,13,1,1,25,12,14,132,124,130,247,32,196,136,124,44,93,
    19,53,33,50,62,2,53,52,46,2,35,33,130,10,32,30,77,118,5,32,100,139,95,33,1,103,135,119,51,2,205,126,36,63,85,49,49,84,63,36,126,56,97,131,74,75,130,98,56,66,157,6,131,125,8,32,19,0,
    39,0,101,64,72,121,18,137,18,153,18,3,154,17,1,137,17,1,123,17,1,149,13,1,118,13,134,13,2,151,12,119,222,5,38,150,8,1,135,8,1,118,130,2,59,7,134,7,150,7,3,154,3,1,121,3,137,3,2,121,
    2,137,2,153,2,3,30,10,20,0,25,15,35,65,41,5,130,172,32,205,68,135,12,32,93,132,0,65,51,8,65,181,6,32,55,77,218,14,32,100,131,157,130,181,33,57,57,130,165,130,7,51,56,124,37,61,86,49,
    49,88,60,39,39,60,88,49,49,86,61,37,4,51,139,30,131,46,130,21,33,38,38,133,37,35,38,38,60,88,134,215,8,33,93,5,154,0,3,0,7,0,81,64,52,183,6,1,8,5,168,5,2,183,2,1,8,1,168,1,2,224,6,
    240,6,2,6,5,66,246,5,45,2,2,3,9,224,7,240,7,2,7,4,239,0,255,130,60,52,3,3,4,5,2,3,0,63,196,222,196,1,47,205,93,222,205,93,17,18,57,133,8,90,202,5,34,19,1,51,132,1,67,9,5,32,135,67,
    15,10,77,249,5,38,2,0,100,2,205,1,216,134,127,37,23,64,9,7,5,1,141,84,38,222,205,49,48,19,17,51,132,1,34,100,124,124,78,51,6,133,61,32,1,130,61,36,93,1,68,3,61,79,97,18,65,61,15,78,
    248,16,33,2,205,68,146,14,58,0,0,1,0,100,3,245,3,49,4,113,0,3,0,17,181,1,1,5,3,1,3,0,47,205,1,47,130,212,130,133,34,33,21,33,130,173,36,253,51,4,113,124,132,43,35,2,205,3,49,130,185,
    46,11,0,31,64,13,7,6,4,1,0,10,10,9,7,4,130,191,41,0,63,221,196,221,205,196,1,47,205,131,6,131,57,130,190,130,61,48,17,35,17,33,100,1,40,124,1,41,254,215,124,254,216,4,113,134,8,33,
    1,40,139,79,79,3,34,38,140,254,216,2,205,254,215,79,3,8,74,213,5,33,8,17,130,141,33,38,1,75,3,5,49,116,5,194,0,0,0,11,182,2,100,8,8,1,1,37,1,43,53,74,47,6,33,6,73,132,37,74,161,5,34,
    116,3,250,133,37,36,0,7,7,2,2,135,37,36,100,0,0,7,98,73,207,10,34,116,5,19,134,37,33,9,9,137,37,130,180,134,113,72,161,5,139,113,134,37,33,0,1,79,13,8,8,55,6,0,99,64,54,3,2,2,126,5,
    4,20,5,2,1,5,4,128,4,176,4,224,4,3,4,4,5,39,5,1,5,6,135,1,1,2,1,0,6,2,6,1,126,0,6,20,0,6,0,0,6,5,6,18,4,3,78,89,5,32,196,79,16,8,33,135,43,78,86,7,35,1,93,17,51,130,2,78,90,12,56,19,
    51,1,1,51,1,35,40,178,2,27,2,27,178,253,61,20,5,154,251,187,4,69,250,102,79,19,12,75,105,5,79,19,8,130,119,38,27,37,0,1,37,2,1,79,19,12,139,187,34,93,64,49,130,169,38,126,1,2,20,1,
    4,5,79,17,7,47,4,5,6,0,4,0,5,126,6,0,20,6,0,143,6,191,130,188,130,182,42,1,5,3,2,6,18,0,1,3,0,63,130,180,33,196,196,79,16,37,54,35,1,1,35,2,235,20,2,195,178,253,229,253,229,178,5,154,
    250,102,4,69,251,187,65,179,6,38,5,194,7,66,2,38,1,133,183,32,78,79,15,7,32,1,75,212,5,32,1,79,15,10,33,1,0,77,233,6,61,0,6,0,89,64,45,5,4,4,126,0,6,20,0,4,3,0,6,4,3,2,1,4,1,3,126,
    2,1,20,2,130,241,35,1,1,8,6,130,169,42,39,0,1,0,1,1,2,6,3,2,18,77,186,12,33,196,196,79,2,16,77,186,9,40,1,21,1,53,1,1,53,5,254,133,164,36,2,215,20,253,61,65,113,5,79,5,13,32,135,79,
    5,8,44,22,185,0,1,255,226,64,9,7,7,6,6,37,137,176,75,115,12,130,45,79,7,14,42,37,183,2,0,27,37,6,0,37,1,184,130,60,32,13,132,60,35,2,42,5,38,134,64,79,10,6,78,223,13,130,69,51,1,7,
    1,80,1,162,0,0,0,17,64,9,1,0,10,10,5,4,37,1,79,11,20,133,43,79,11,8,35,1,0,17,17,131,43,79,11,8,66,1,11,50,81,64,41,2,3,2,1,3,126,4,5,20,4,5,3,4,1,2,2,65,248,5,53,2,6,1,0,4,4,8,6,5,
    39,6,1,6,5,5,4,18,0,3,0,63,63,79,3,12,44,196,16,193,135,4,43,125,16,196,1,16,24,196,130,9,81,192,5,34,1,21,1,130,2,34,53,5,194,66,161,5,33,5,154,65,250,5,34,2,195,20,77,217,13,32,140,
    79,1,13,37,1,30,17,17,0,0,65,68,10,131,45,78,167,9,32,131,79,1,13,32,1,79,1,7,32,1,79,1,20,134,45,32,5,79,1,6,131,91,33,4,4,130,91,80,57,9,137,89,78,159,11,34,19,185,0,133,89,33,6,
    6,140,45,79,1,9,134,45,139,89,33,2,2,140,43,79,1,9,130,43,79,1,19,38,2,255,226,64,18,27,27,131,98,66,233,5,33,2,42,65,249,6,79,1,20,142,69,132,121,35,32,64,20,2,79,1,5,67,44,6,143,
    66,77,181,12,32,135,79,1,12,132,227,35,156,182,17,17,142,227,34,100,0,0,78,237,6,66,63,6,78,237,7,37,1,100,7,7,0,0,143,227,78,237,6,132,91,78,237,16,34,38,185,0,130,229,32,181,133,
    228,36,184,255,156,64,11,132,110,66,228,5,78,224,6,78,155,8,78,219,6,130,117,78,219,14,32,35,132,69,65,43,7,133,133,143,66,66,37,6,32,6,79,235,5,32,140,78,159,15,32,1,78,24,23,140,
    255,32,93,133,0,78,159,13,66,159,6,78,159,7,65,5,21,78,159,6,132,119,78,159,19,38,2,30,37,37,0,0,37,152,134,65,24,19,133,140,32,93,65,31,5,34,40,0,0,132,99,33,38,1,130,143,78,159,19,
    135,97,65,30,21,82,115,12,134,65,78,159,15,68,140,7,67,128,5,66,73,15,8,38,0,1,0,106,2,205,3,55,5,154,0,6,0,77,64,48,164,5,180,5,196,5,3,5,2,6,4,4,8,2,5,230,1,1,1,229,2,245,2,130,0,
    8,75,3,6,127,0,143,0,159,0,223,0,4,16,0,32,0,48,0,3,0,4,3,3,0,63,205,222,93,93,205,18,57,25,47,93,51,93,51,1,24,47,18,57,47,196,17,57,93,49,48,1,1,53,1,21,5,5,3,55,253,51,2,205,254,
    65,1,191,2,205,1,97,10,1,98,135,224,223,75,23,5,33,0,224,87,99,19,36,19,17,51,17,100,73,48,6,130,39,61,120,1,102,2,154,4,51,0,25,0,19,183,3,19,76,6,6,1,13,0,0,47,50,205,57,47,237,57,
    48,49,131,44,109,177,6,33,21,35,114,110,8,50,21,120,130,20,83,60,65,95,62,31,130,22,38,52,29,31,53,39,22,130,69,52,205,254,218,17,33,35,64,90,55,229,226,35,50,32,15,15,32,50,35,226,
    0,130,97,36,100,0,0,5,94,130,137,8,64,25,0,81,64,54,137,20,1,123,20,1,42,20,1,133,16,1,116,16,1,37,16,1,105,7,1,102,3,1,7,3,23,3,2,12,126,16,10,32,10,2,10,10,27,25,126,15,23,31,23,
    2,23,11,24,3,5,145,18,18,0,63,237,63,196,1,111,171,5,32,93,24,68,200,11,78,231,8,130,212,34,20,2,6,105,218,6,8,47,17,51,1,12,50,112,179,128,130,179,111,49,168,71,153,242,171,171,242,
    153,71,168,2,199,111,199,150,87,87,150,199,111,2,211,253,45,145,254,253,194,113,113,194,1,3,145,2,211,70,43,7,38,94,7,166,2,38,1,162,130,180,53,7,1,79,2,14,0,0,0,23,64,13,2,1,0,46,
    56,18,18,37,2,1,61,87,27,11,71,83,5,136,215,8,40,133,20,1,115,20,1,37,20,1,138,16,1,123,16,1,41,16,1,102,7,1,7,7,23,7,2,105,3,1,23,126,16,25,32,25,2,25,25,27,10,126,109,171,5,130,212,
    34,3,25,11,130,215,34,196,63,237,148,215,93,60,8,47,35,17,52,18,54,54,51,50,22,22,18,21,17,35,4,182,146,215,32,211,135,215,38,253,45,2,211,145,1,3,131,215,36,254,253,145,253,45,137,
    215,32,66,136,215,34,78,2,113,130,215,38,19,64,11,1,0,26,36,130,214,33,1,41,87,55,9,72,39,9,8,55,25,0,83,64,54,89,23,1,88,22,1,89,18,1,88,17,1,5,10,1,119,9,1,101,9,1,7,9,1,118,5,1,
    101,5,1,6,5,1,4,4,1,7,125,20,20,27,0,15,14,1,14,14,145,12,18,25,130,125,34,0,63,237,132,211,32,196,24,87,146,14,37,93,93,93,19,53,33,24,76,130,7,34,4,35,33,77,126,9,32,100,141,182,
    32,2,136,208,53,4,242,168,84,176,254,241,186,185,254,242,176,86,168,63,133,208,145,143,207,135,64,86,229,7,32,7,131,213,40,164,0,0,1,7,1,78,1,90,137,213,34,1,2,37,138,213,143,45,33,
    0,39,133,45,87,87,11,42,2,135,46,46,36,36,37,1,0,26,36,130,60,35,2,61,5,38,134,64,87,87,19,133,113,34,80,1,132,130,113,38,17,64,9,1,0,27,26,130,52,33,1,27,87,87,19,133,43,34,78,1,112,
    83,165,5,134,96,33,1,41,134,43,67,29,6,8,69,254,5,154,0,25,0,91,64,61,106,21,1,11,21,1,122,20,1,25,20,105,20,2,8,20,1,121,16,1,106,16,1,8,16,24,16,2,12,15,1,85,8,1,87,7,1,87,3,1,86,
    2,1,25,11,11,27,5,125,15,18,1,18,25,145,23,3,11,145,13,18,65,120,7,35,237,18,57,47,118,75,10,32,93,24,78,11,12,34,33,21,33,24,100,145,9,35,33,21,3,43,67,19,23,53,4,242,64,135,207,143,
    145,208,133,63,168,86,176,1,14,185,186,1,15,176,84,168,66,81,7,38,254,7,66,2,38,1,169,65,123,5,33,4,40,65,123,9,33,23,24,65,123,14,36,10,0,0,6,72,130,223,34,39,1,160,71,71,12,57,40,
    64,16,1,64,36,1,48,36,1,32,36,1,16,36,1,0,36,1,184,255,156,182,36,36,24,131,65,33,0,47,85,195,8,65,191,5,33,6,162,130,67,32,38,130,67,38,0,1,7,1,78,5,194,65,79,6,36,100,26,26,12,12,
    130,113,72,153,12,134,111,32,162,165,111,137,63,32,93,86,185,10,134,111,68,11,5,140,111,137,159,32,53,71,61,7,37,72,7,66,0,39,1,132,111,70,197,11,40,3,91,0,0,0,56,64,23,2,68,64,5,146,
    238,35,64,11,36,36,130,127,66,112,5,86,185,17,36,100,0,0,6,162,130,91,32,38,130,91,32,0,131,89,32,5,130,247,131,255,33,2,113,130,89,34,32,64,20,135,89,34,100,26,26,130,150,140,70,70,
    255,5,87,199,9,32,164,65,13,12,72,197,6,35,36,36,1,1,65,61,12,72,197,9,66,185,6,72,197,9,35,26,26,7,7,140,43,132,91,132,249,132,91,139,249,33,2,68,130,159,33,35,183,131,158,38,1,2,
    37,1,184,255,156,131,233,130,107,143,162,66,79,6,72,195,6,130,115,72,195,8,67,211,7,134,229,131,70,130,229,130,130,143,66,73,171,12,32,169,140,229,66,99,24,33,18,18,135,201,88,173,
    18,66,213,6,136,249,34,0,26,26,65,243,18,32,232,132,249,132,111,139,249,33,5,18,65,243,9,34,23,24,37,65,243,22,130,127,140,198,88,173,10,32,100,88,173,8,130,135,65,13,11,33,4,40,65,
    243,9,131,89,133,150,140,70,65,243,15,138,157,8,36,253,97,1,7,1,78,0,10,250,148,0,78,64,16,2,64,56,1,48,56,1,32,56,1,16,56,1,0,56,1,184,255,156,64,21,56,56,130,150,147,176,34,9,36,
    36,132,176,36,1,41,0,47,53,67,152,9,89,99,6,83,131,15,61,133,21,149,21,2,118,21,1,133,15,149,15,2,118,15,1,12,24,24,27,18,5,23,25,12,10,3,0,63,205,84,123,7,68,160,6,43,1,34,46,2,53,
    52,62,2,51,33,21,33,111,221,6,130,10,33,1,202,84,103,10,48,161,47,88,67,40,40,67,88,47,1,95,2,205,56,98,130,75,131,27,41,126,36,63,84,49,49,85,63,36,126,66,65,6,33,6,62,68,31,10,32,
    81,80,229,6,37,1,100,27,27,12,12,80,191,10,134,37,67,213,5,139,37,33,24,24,139,37,32,222,132,75,67,1,5,39,81,6,98,0,0,0,11,182,131,75,33,7,7,145,37,66,45,5,139,37,134,75,39,0,2,0,100,
    0,0,3,217,130,75,8,62,18,0,35,0,80,64,55,55,32,71,32,87,32,3,55,26,71,26,87,26,3,56,22,72,22,88,22,3,103,13,119,13,2,104,9,120,9,2,35,16,126,0,0,37,29,125,15,6,31,6,2,6,35,145,1,1,
    18,18,24,145,11,3,0,24,77,199,7,34,93,237,18,24,92,197,10,33,1,33,124,35,11,85,65,10,49,20,30,2,51,33,3,49,254,238,92,161,120,70,70,120,161,92,91,130,7,49,168,44,75,100,57,57,100,74,
    42,42,74,100,57,1,20,2,37,69,131,24,32,162,131,32,46,162,91,252,33,3,223,59,101,73,42,42,73,101,59,58,130,35,65,83,6,47,3,217,7,166,2,38,1,193,0,0,1,7,1,79,1,74,96,43,8,39,56,66,7,
    7,37,3,2,71,96,43,13,139,237,34,84,64,57,134,237,37,56,28,72,28,88,28,136,237,8,39,9,119,9,2,104,5,120,5,2,12,125,25,25,1,37,19,18,126,15,1,31,1,2,1,19,145,17,17,0,30,145,7,3,0,18,
    0,63,63,237,17,130,232,37,1,47,93,253,196,17,72,69,10,32,33,24,108,67,12,72,64,11,38,34,14,2,21,1,12,168,139,211,35,254,238,1,20,135,239,38,57,100,75,44,3,223,91,134,237,40,92,161,
    120,69,168,42,74,100,58,135,242,70,97,5,130,237,32,66,136,237,34,78,1,174,70,211,5,35,2,0,36,46,130,236,33,2,51,96,93,14,65,215,8,59,88,64,61,56,32,72,32,88,32,3,55,28,71,28,87,28,
    3,55,22,71,22,87,22,3,104,15,120,15,65,215,5,44,103,5,119,5,2,18,2,126,19,19,37,25,125,73,254,5,135,237,80,21,5,135,237,65,222,12,119,159,15,67,32,10,42,50,62,2,53,3,49,168,70,120,
    161,91,65,226,7,35,1,18,254,236,139,237,35,5,154,252,33,65,254,12,146,239,33,245,7,131,239,40,195,0,0,1,7,1,78,3,21,137,239,34,0,1,37,138,239,65,29,6,136,45,33,0,39,133,45,131,53,33,
    1,173,130,53,54,45,182,3,16,66,1,0,66,1,184,255,120,64,20,66,66,36,36,37,2,0,36,46,130,69,35,3,71,5,38,134,73,73,72,5,35,93,93,53,0,67,23,14,65,63,9,37,56,26,72,26,88,26,65,63,8,8,
    38,13,120,13,2,103,9,119,9,2,103,3,119,3,2,6,125,29,29,16,37,0,35,126,15,16,31,16,2,16,35,145,1,1,18,3,24,145,11,18,67,29,9,66,44,13,35,93,1,33,50,87,210,6,34,34,46,2,88,219,11,40,
    52,46,2,35,33,1,12,1,18,66,16,12,67,31,11,35,254,236,3,117,65,93,11,35,3,223,252,33,67,31,15,34,72,0,0,66,49,6,32,198,82,207,5,130,16,34,19,64,11,131,249,33,17,18,65,63,14,42,10,0,
    0,4,195,5,154,0,39,1,191,70,159,15,55,2,64,46,1,48,46,1,32,46,1,16,46,1,0,46,1,184,255,156,182,46,46,6,6,130,65,73,3,16,33,5,29,130,67,32,38,130,67,38,0,1,7,1,78,4,61,73,3,5,37,2,100,
    36,36,16,16,135,47,72,147,7,134,111,32,193,165,111,33,1,1,154,111,67,255,5,140,111,33,12,12,145,111,33,7,66,130,223,132,111,72,9,12,32,152,71,15,5,38,3,0,56,66,7,7,37,146,238,35,64,
    11,46,46,130,127,66,43,5,73,3,20,33,5,29,130,91,33,38,1,130,135,131,89,131,247,131,255,33,1,174,71,15,5,135,89,34,100,36,36,130,150,140,70,71,15,8,65,13,6,32,195,65,13,37,137,221,72,
    29,11,65,125,6,67,47,6,65,125,11,65,61,9,65,125,8,32,223,130,177,33,39,1,132,111,74,17,12,32,255,65,13,9,33,0,1,65,13,23,143,198,72,29,13,65,13,6,130,135,65,13,11,33,3,21,65,13,9,131,
    89,34,100,36,36,65,84,15,65,13,15,32,198,65,13,37,33,17,17,66,27,26,33,198,0,66,139,16,66,187,9,65,13,8,34,195,7,66,130,171,132,111,82,235,12,32,50,65,13,9,33,17,18,65,13,23,130,127,
    65,212,12,65,13,10,34,72,0,0,65,13,6,130,135,65,13,8,34,6,1,78,130,24,38,32,64,20,3,0,56,66,131,87,37,100,36,36,6,6,37,140,68,65,11,20,133,155,73,41,13,41,3,64,66,1,48,66,1,32,66,1,
    68,227,7,36,156,64,21,66,66,130,148,66,202,19,34,9,46,46,132,174,33,2,51,73,41,20,38,2,0,100,2,205,2,30,72,17,6,8,37,35,64,16,6,29,16,0,35,35,1,1,18,24,0,11,1,11,18,3,0,63,220,93,205,
    17,57,47,205,1,47,196,205,222,205,49,48,19,51,68,196,26,42,35,224,97,46,80,60,35,35,60,80,46,131,7,47,124,9,23,37,29,21,35,26,14,14,26,35,21,98,4,135,135,23,131,7,48,1,240,254,16,16,
    34,29,18,16,27,35,19,19,35,27,16,0,91,227,5,139,139,46,18,2,19,12,25,19,17,17,0,30,0,7,1,7,0,139,139,130,138,34,196,49,48,126,48,14,32,51,24,90,224,13,34,1,162,124,139,112,33,97,98,
    135,137,39,28,38,23,9,5,154,254,16,140,170,135,135,35,18,29,34,16,72,109,6,33,4,185,68,221,10,45,81,4,61,0,0,0,11,182,2,100,37,37,16,16,73,117,10,134,37,68,147,5,139,37,73,231,12,134,
    37,67,171,5,139,37,90,243,9,34,100,0,0,134,37,66,195,5,139,37,33,6,6,132,113,79,157,5,33,3,217,130,151,56,25,0,63,64,40,119,20,1,102,20,1,122,16,1,105,16,1,87,7,1,23,126,32,25,48,81,
    213,12,43,25,18,12,5,145,18,3,0,63,253,204,63,81,213,15,24,120,233,8,46,21,35,53,52,62,2,51,50,30,2,21,17,35,3,49,70,119,7,32,168,73,169,8,73,139,9,33,91,91,70,162,7,33,252,33,73,147,
    13,32,223,73,147,13,38,2,1,0,46,56,7,7,82,163,20,134,191,59,61,64,39,88,18,1,102,9,118,9,2,121,5,1,104,5,1,12,126,32,14,48,14,2,14,14,27,25,73,131,6,33,13,20,73,127,7,33,253,204,142,
    190,73,122,10,84,7,10,73,114,10,36,168,42,74,100,57,73,103,13,33,91,91,71,51,7,137,187,35,66,2,38,1,133,187,73,97,8,35,1,0,26,36,130,186,82,135,16,137,183,46,87,18,1,121,9,1,106,9,
    1,103,5,119,5,2,1,65,118,8,32,14,83,76,6,131,183,73,73,5,144,183,66,179,10,34,53,51,21,95,42,7,73,60,10,136,183,73,49,12,144,185,72,253,5,32,225,73,43,13,131,185,33,0,1,80,239,14,35,
    100,0,0,3,136,45,73,43,18,32,2,77,113,7,39,120,64,20,56,56,26,26,37,134,69,82,116,16,34,93,93,53,130,110,76,67,8,58,25,0,65,64,42,121,20,1,104,20,1,101,16,117,16,2,88,7,1,86,3,1,12,
    126,32,10,48,85,49,12,34,25,3,12,85,49,5,66,131,19,65,0,7,33,53,51,24,75,255,8,35,17,51,1,12,66,131,10,40,162,91,92,161,120,69,168,1,187,65,187,7,32,91,74,99,8,37,3,223,0,255,255,0,
    72,247,9,32,228,72,247,11,131,197,33,24,25,65,11,14,72,247,9,32,221,70,107,15,78,103,18,34,182,36,36,81,139,21,71,121,6,32,221,70,107,13,37,1,100,26,26,23,23,80,145,15,71,233,6,32,
    223,165,111,80,253,9,71,233,18,66,167,6,71,233,7,131,111,82,107,17,32,4,70,219,5,132,111,72,247,19,32,2,67,152,5,146,238,81,17,19,71,233,20,130,135,70,219,8,33,7,1,67,55,5,32,32,81,
    251,5,34,7,7,37,134,150,80,7,21,65,13,6,32,225,65,13,37,65,125,28,66,251,6,65,13,11,82,59,14,35,10,0,0,4,72,247,5,132,111,72,247,19,37,2,0,46,56,0,1,81,21,23,83,9,29,71,233,6,130,135,
    72,247,19,135,89,34,100,26,26,82,102,18,82,35,5,65,13,6,32,228,65,13,37,84,135,21,66,139,6,32,228,66,139,17,66,27,24,132,111,72,247,19,131,179,33,24,25,65,13,23,83,208,15,72,247,20,
    130,135,72,247,17,135,87,85,28,5,66,25,28,138,155,72,247,13,82,33,22,130,148,66,202,19,34,9,36,36,132,174,82,33,27,63,2,30,5,154,0,25,0,23,64,9,2,0,13,15,2,21,8,14,3,0,63,220,221,204,
    1,47,205,222,205,49,48,1,68,147,12,105,103,7,72,85,10,72,222,8,35,3,170,49,49,72,76,7,72,215,10,82,135,5,139,101,37,12,14,13,20,7,1,141,101,32,51,119,205,8,70,15,10,139,101,35,14,26,
    35,21,72,178,15,41,49,49,19,35,27,16,18,29,34,16,69,145,11,55,5,0,35,178,0,126,2,184,255,192,64,13,9,12,72,2,2,7,4,2,18,3,145,0,130,112,40,237,63,1,47,18,57,47,43,237,130,215,48,17,
    35,17,33,53,3,217,168,253,51,5,154,250,102,4,246,164,71,201,13,32,249,71,201,16,37,26,36,5,0,37,2,71,15,6,113,137,6,32,1,70,7,8,60,5,0,38,64,22,32,0,48,0,2,0,0,7,2,126,15,4,31,4,2,
    4,2,145,5,3,3,18,0,63,90,76,9,130,120,33,21,33,130,122,43,3,217,253,51,168,5,154,164,251,10,5,154,80,231,12,133,119,71,133,10,33,6,16,130,118,33,1,21,90,13,14,36,3,217,5,154,0,131,
    233,32,4,140,233,37,1,3,3,2,145,0,130,113,138,233,46,51,53,33,17,51,17,100,2,205,168,164,4,246,250,102,80,55,12,32,251,71,57,15,36,6,16,3,4,37,138,109,143,45,71,57,20,88,37,6,41,120,
    64,20,36,36,6,6,37,1,0,132,69,35,2,41,5,38,134,73,80,101,10,65,49,15,49,4,48,4,2,4,4,7,3,126,15,0,31,0,2,0,2,3,3,136,191,40,93,237,18,57,47,93,49,48,51,130,190,42,33,21,100,168,2,205,
    5,154,251,10,164,70,237,13,32,254,70,237,13,35,6,16,1,2,142,191,70,237,9,32,247,68,97,12,87,107,6,35,16,16,5,5,130,49,90,37,13,79,209,5,32,247,68,77,15,35,6,6,0,0,140,43,137,91,32,
    249,140,91,35,40,64,16,1,130,2,47,48,16,1,32,16,1,16,16,1,0,16,1,184,255,156,182,139,107,70,217,18,65,251,6,69,203,9,149,111,36,7,66,0,39,1,132,111,70,217,21,34,26,36,5,130,165,145,
    126,33,64,11,132,127,65,151,5,79,209,27,130,135,70,217,21,133,89,32,100,132,150,140,70,77,183,15,32,251,65,13,12,65,105,8,33,1,1,65,61,12,34,100,0,0,69,183,6,66,135,6,139,249,33,4,
    4,140,43,70,197,9,132,91,70,197,16,38,35,183,2,0,26,36,3,130,52,34,184,255,156,131,233,130,107,143,162,67,225,6,70,177,6,130,115,70,177,21,133,70,130,229,130,130,143,66,70,177,12,32,
    254,140,229,65,243,24,137,245,65,243,18,32,254,66,99,17,146,249,72,205,5,132,111,70,177,21,36,26,36,1,2,37,66,114,18,65,9,19,70,177,20,130,135,65,13,8,79,169,8,32,2,93,120,5,32,1,65,
    11,39,79,169,19,32,2,92,77,17,37,64,21,36,36,1,1,148,174,34,9,16,16,130,25,35,2,41,1,21,79,169,20,32,1,79,169,8,40,5,0,18,182,4,0,2,3,5,110,47,5,55,1,47,205,205,49,48,19,51,17,33,21,
    33,100,124,1,62,254,70,5,154,253,175,124,0,70,125,12,8,38,59,64,37,119,21,1,119,15,1,152,8,1,138,8,1,136,7,152,7,2,136,3,152,3,2,154,2,1,137,2,1,12,25,5,18,24,0,12,11,89,15,8,32,220,
    93,174,10,89,17,8,79,95,10,33,51,21,89,17,9,33,84,84,108,254,7,32,84,89,14,9,33,124,38,89,14,5,33,38,124,138,135,131,185,130,181,34,2,0,4,130,104,33,221,205,133,185,49,1,33,53,33,17,
    51,2,30,254,70,1,62,124,2,205,124,2,81,138,49,60,34,0,49,64,25,217,21,233,21,249,21,3,15,19,16,16,10,24,34,19,19,0,34,19,15,18,29,0,5,130,68,36,205,205,220,205,50,130,71,42,50,47,16,
    222,205,50,47,17,57,49,48,125,239,9,41,20,14,2,7,33,21,33,53,62,3,98,39,8,32,100,80,17,7,8,49,40,74,102,61,1,21,254,70,102,124,69,23,12,25,37,24,23,36,24,13,4,188,46,81,60,35,35,60,
    81,46,72,105,84,73,37,124,124,65,92,79,80,55,19,36,27,16,17,28,35,18,79,123,10,8,53,36,0,120,64,80,71,33,87,33,2,138,22,1,153,21,1,150,12,1,117,7,1,102,7,1,105,3,121,3,2,136,2,1,105,
    2,121,2,2,145,15,1,130,15,1,118,15,1,53,15,101,15,2,15,18,130,189,46,126,32,26,48,26,2,26,26,0,38,18,18,36,126,15,130,82,39,19,15,145,18,18,0,31,145,131,204,35,253,204,63,237,130,204,
    41,93,237,51,47,17,18,57,47,93,237,131,208,32,93,130,0,108,163,5,109,108,16,133,220,24,71,63,8,33,21,100,79,176,7,45,106,176,229,123,2,122,252,139,135,212,159,111,69,31,77,75,7,88,
    111,5,57,69,120,162,92,144,235,198,171,79,164,164,89,152,134,124,124,130,74,60,101,73,41,42,73,101,59,120,165,8,51,3,117,0,18,0,35,0,79,64,49,87,23,1,104,15,1,106,14,1,101,97,108,7,
    46,103,5,119,5,2,102,4,1,19,126,17,7,125,30,17,130,1,49,37,1,18,2,145,16,19,1,19,25,145,12,18,0,63,253,222,93,24,81,242,12,137,187,32,19,110,210,9,86,255,18,32,100,89,14,26,33,2,205,
    88,90,28,120,99,8,37,129,2,38,2,22,0,90,27,6,63,253,219,0,37,64,23,3,2,0,56,66,16,16,37,3,2,95,71,1,47,71,1,31,71,1,15,71,1,71,0,47,93,130,0,122,85,9,36,100,0,0,5,254,134,247,8,62,
    91,64,61,90,14,106,14,2,104,13,120,13,2,89,13,1,120,9,1,105,9,1,88,9,1,90,8,106,8,2,100,4,1,85,4,1,87,3,103,3,119,3,3,0,126,34,17,34,17,37,24,125,11,17,145,0,16,35,1,35,29,145,6,65,
    3,5,40,196,237,1,47,237,18,57,57,47,24,64,141,13,24,65,186,13,89,65,14,34,17,3,217,87,227,27,33,2,205,91,64,28,65,5,8,32,29,65,5,8,44,78,1,174,253,219,0,17,64,9,2,0,36,46,87,145,11,
    88,241,5,136,241,8,35,79,64,49,87,31,1,101,14,1,102,13,118,13,2,119,9,1,102,9,1,101,8,1,107,4,1,104,3,1,34,126,0,11,125,24,0,130,1,43,37,18,6,145,16,29,1,29,35,0,145,17,131,229,34,
    196,220,93,130,229,24,83,220,8,65,233,9,32,37,126,54,12,99,86,10,38,34,14,2,21,17,2,137,91,233,27,32,168,89,5,28,70,199,7,33,254,5,131,229,40,24,0,0,1,7,1,78,3,211,137,229,85,235,14,
    35,100,0,0,5,136,43,33,0,39,130,43,34,30,253,219,131,51,33,4,134,130,51,48,33,183,3,136,56,56,46,46,37,2,184,255,187,64,9,46,46,130,61,33,3,71,85,42,8,33,53,43,90,27,7,65,41,8,55,95,
    64,63,102,15,118,15,2,87,15,1,101,14,1,86,14,1,107,10,1,90,10,1,122,66,34,7,8,39,89,5,105,5,121,5,3,105,4,1,90,4,1,17,126,19,0,19,0,37,30,125,7,12,145,16,25,1,25,18,19,145,2,18,0,63,
    253,196,222,93,66,29,21,35,93,93,37,21,93,67,27,33,5,254,91,67,26,32,168,92,108,28,66,29,12,32,27,65,55,5,66,29,11,88,97,9,98,67,8,38,232,3,117,0,39,2,20,71,233,15,86,101,18,34,182,
    46,46,88,113,9,96,191,13,130,67,32,38,130,67,35,0,1,7,1,121,109,10,35,36,36,7,7,89,175,15,32,6,133,111,32,27,165,111,137,63,72,89,12,37,254,3,117,2,38,2,134,223,33,4,151,90,143,8,34,
    46,17,0,145,111,33,5,29,130,223,132,111,74,77,12,44,153,253,219,0,54,64,23,3,0,56,66,12,12,88,3,20,34,9,46,46,130,127,66,41,10,81,37,14,66,113,5,130,133,131,87,131,133,90,29,5,130,
    87,34,30,64,18,135,87,33,100,36,131,148,66,110,13,65,165,15,132,151,32,252,66,177,5,33,2,100,66,177,19,130,161,141,66,32,0,87,71,5,8,38,3,49,4,135,0,18,0,35,0,28,64,11,7,30,19,1,17,
    19,12,25,19,18,2,0,47,221,196,220,205,1,47,221,204,16,222,205,49,48,1,66,110,13,85,72,10,37,20,30,2,51,3,49,87,35,26,33,3,73,87,93,22,63,29,37,23,9,0,1,0,115,1,82,3,37,4,51,0,21,0,
    13,180,7,15,11,218,0,0,47,237,204,50,48,49,1,69,251,7,8,47,22,51,50,54,53,17,51,17,20,14,2,1,204,78,128,90,49,130,115,100,100,115,130,49,90,128,1,82,50,93,132,83,1,123,254,143,118,
    128,128,118,1,113,254,133,83,132,93,50,145,215,42,11,24,35,17,1,35,6,29,0,35,17,131,215,32,222,138,215,86,208,8,130,93,95,150,15,34,21,1,119,88,23,11,39,254,16,1,240,16,34,28,19,88,
    0,7,33,3,73,88,195,26,32,0,72,5,5,53,5,254,3,117,0,25,0,66,64,42,89,23,1,90,17,1,116,10,1,102,10,1,94,27,5,71,32,6,41,14,7,125,20,20,27,0,2,145,16,130,50,34,15,145,13,70,22,5,38,237,
    1,47,18,57,47,237,24,79,64,9,71,20,10,34,35,53,51,117,253,7,71,12,10,86,179,9,71,5,10,86,35,8,69,15,9,36,129,2,38,2,41,70,251,13,54,2,1,0,46,56,23,23,37,2,1,95,61,1,47,61,1,31,61,1,
    15,61,1,61,70,251,13,35,1,0,100,0,70,251,5,39,25,0,68,64,42,107,21,1,85,210,5,60,121,16,1,104,16,1,105,15,1,84,8,1,86,2,1,25,25,18,27,12,5,125,18,23,145,16,25,1,25,103,220,5,34,253,
    222,93,130,207,34,253,204,17,130,210,69,252,9,32,1,99,53,7,33,51,21,24,87,202,8,35,33,21,2,31,85,190,19,33,2,205,85,232,10,98,22,6,67,3,12,133,209,70,199,8,35,1,0,26,36,85,97,11,65,
    141,12,8,44,68,64,43,102,21,1,102,20,118,20,2,118,16,1,103,16,1,118,15,1,101,15,1,90,8,1,89,2,1,18,125,11,5,5,27,25,13,145,16,11,1,11,0,145,24,65,142,11,32,206,107,191,10,32,37,65,
    130,7,33,35,53,90,232,8,35,33,53,4,67,137,189,89,0,9,98,193,8,34,168,70,120,86,165,6,70,159,13,32,43,70,159,13,131,189,33,13,13,86,31,12,70,159,9,130,43,70,159,18,33,2,136,106,6,5,
    38,184,255,187,64,9,36,36,130,61,33,2,61,82,210,8,33,53,43,87,143,7,66,143,6,39,62,64,38,86,23,1,84,17,88,156,6,47,105,5,121,5,2,106,4,1,0,0,7,27,13,20,125,7,130,252,35,15,1,15,0,70,
    132,5,65,187,18,70,124,10,75,234,10,70,116,12,88,134,8,70,109,9,66,139,18,36,29,2,38,2,46,70,99,13,131,251,84,143,17,69,243,6,32,39,69,243,15,83,209,18,32,182,77,56,5,106,167,5,70,
    99,18,32,39,133,111,103,35,8,103,154,5,106,215,8,78,189,5,134,111,32,41,165,111,103,147,23,34,3,117,0,67,107,7,70,211,8,32,1,104,240,5,144,111,36,5,29,0,39,2,132,111,70,99,19,32,2,
    67,160,5,146,238,32,64,102,226,23,24,64,199,9,32,29,133,133,104,159,12,32,174,70,99,5,135,87,133,148,66,50,13,67,95,7,36,3,117,2,38,2,66,161,6,33,0,235,71,41,5,37,1,135,26,26,25,25,
    66,161,15,33,7,66,132,241,134,43,65,97,11,65,33,9,32,53,66,249,16,132,239,130,87,73,161,8,34,30,64,18,131,151,34,13,13,37,134,102,147,151,34,7,66,5,131,215,67,13,6,131,107,147,63,130,
    215,103,255,13,81,133,8,65,225,6,32,46,65,225,37,106,63,9,66,81,11,32,5,72,69,5,66,193,6,47,4,151,251,251,0,19,185,0,1,255,121,182,36,36,0,0,65,29,12,36,10,0,0,6,232,130,177,33,39,
    2,132,113,65,227,23,33,12,12,87,83,20,32,9,73,7,5,67,209,9,72,71,20,130,135,72,71,16,37,33,183,2,0,46,56,131,86,35,184,255,121,64,130,71,130,151,65,78,13,121,169,8,56,4,135,0,25,0,
    21,183,13,7,0,20,12,14,0,2,0,47,221,222,205,1,47,206,205,204,71,254,12,67,240,10,71,246,11,86,103,9,71,239,10,40,124,14,26,35,21,29,37,23,9,136,99,8,36,5,154,0,12,0,95,64,54,11,5,1,
    6,9,3,12,8,8,14,2,12,9,0,12,6,2,3,3,4,1,8,7,164,7,180,7,196,7,130,23,8,50,7,7,8,12,3,5,10,20,11,36,11,52,11,3,0,11,1,2,11,5,4,3,0,63,205,220,95,93,93,205,18,23,57,47,47,47,93,17,18,
    57,18,57,47,205,57,16,205,57,1,47,196,130,10,130,23,43,196,198,49,48,19,37,37,53,37,21,5,5,131,2,43,37,100,1,237,254,19,2,205,253,231,2,25,130,3,48,24,253,52,3,230,77,71,165,123,143,
    62,73,163,77,58,141,115,71,167,8,132,153,39,156,64,101,10,5,1,9,6,134,153,8,59,154,12,1,105,12,121,12,137,12,3,72,12,1,12,133,0,149,0,2,119,0,1,0,9,155,8,1,89,8,137,8,2,8,148,7,1,135,
    7,1,7,1,149,3,1,134,3,1,117,3,1,102,3,1,25,3,1,8,3,1,3,81,47,5,52,104,2,120,2,2,2,6,9,1,6,6,1,9,3,4,10,126,11,18,5,126,131,200,34,237,63,237,24,77,72,7,35,51,93,93,93,131,3,130,6,32,
    17,130,7,130,2,134,6,34,93,1,24,143,214,32,1,134,214,43,1,100,4,103,251,153,5,154,251,66,4,190,130,3,50,188,250,104,1,246,215,201,211,1,49,185,224,217,185,225,216,182,1,35,70,241,5,
    33,3,133,130,215,8,33,16,0,67,64,43,86,12,102,12,2,20,12,1,7,12,1,87,11,103,11,2,121,4,1,121,3,1,15,126,1,1,18,8,126,105,183,5,36,1,145,15,15,8,104,197,5,33,57,47,113,219,7,70,242,
    8,36,33,17,46,3,53,88,121,6,60,51,17,2,221,164,239,155,75,168,64,135,207,143,84,2,44,13,122,190,246,137,170,170,111,199,150,87,253,51,68,55,5,48,4,5,7,166,2,38,2,67,0,0,1,7,1,79,2,
    93,0,114,145,7,39,37,47,8,9,37,2,1,52,114,145,16,134,171,59,65,64,41,116,13,1,88,5,104,5,2,88,4,104,4,2,28,4,1,8,4,1,9,126,7,7,18,15,95,237,6,38,15,145,1,1,8,16,18,130,171,34,63,63,
    18,145,170,37,51,17,51,50,62,2,132,171,61,14,2,7,17,100,84,143,207,135,64,168,75,155,239,164,2,205,87,150,199,111,170,170,137,246,190,122,13,253,212,133,169,35,3,161,7,66,136,169,34,
    78,2,193,130,169,38,19,64,11,1,0,17,27,130,168,33,1,32,88,85,15,65,81,5,8,54,77,64,51,120,13,136,13,2,86,5,1,55,5,71,5,2,100,4,1,54,4,70,4,86,4,3,20,4,1,7,4,1,1,126,15,15,18,8,126,
    15,10,31,10,2,10,1,145,15,15,8,16,3,8,18,0,148,175,40,93,93,1,17,35,34,14,2,21,97,95,5,35,55,17,3,133,137,178,35,5,154,253,51,134,180,37,245,191,122,13,2,44,69,63,6,134,181,34,69,0,
    0,114,13,8,134,181,34,16,0,37,138,181,140,227,130,45,34,0,39,1,132,227,131,53,33,1,90,130,235,55,50,64,10,2,32,47,1,16,47,1,0,47,1,184,255,121,64,20,47,47,17,17,37,1,130,255,130,73,
    35,2,52,5,38,134,77,88,161,7,33,93,53,66,91,12,8,47,73,64,48,90,12,106,12,2,57,12,73,12,2,27,12,1,8,12,1,56,11,72,11,2,134,3,1,119,3,1,6,126,8,8,18,1,126,15,15,31,15,2,15,15,145,1,
    1,8,96,238,5,65,181,17,38,93,93,1,17,30,3,21,76,90,6,35,35,17,1,12,66,97,9,39,5,154,253,212,13,122,191,245,66,99,6,33,2,205,95,205,6,38,133,7,66,2,38,2,72,88,223,13,33,17,27,65,1,16,
    42,10,0,0,4,111,5,154,0,39,2,65,69,239,12,36,23,179,1,16,27,130,244,45,156,182,27,27,7,7,37,1,32,0,47,53,1,43,130,224,65,53,5,33,4,201,130,51,32,38,130,51,38,0,1,7,1,78,3,233,70,251,
    6,36,100,17,17,15,15,135,44,33,53,0,72,49,5,134,95,32,67,140,95,43,40,64,16,1,64,27,1,48,27,1,32,27,131,105,32,0,135,108,33,1,1,136,108,113,185,10,134,111,67,51,5,140,111,33,9,9,144,
    111,34,139,7,66,130,207,132,111,87,233,12,32,171,102,153,5,36,2,0,37,47,8,130,53,145,126,35,64,11,27,27,130,127,65,217,5,88,227,20,33,4,201,130,91,33,38,2,130,135,131,89,131,247,66,
    99,8,34,32,64,20,135,89,34,100,17,17,130,150,140,70,71,93,5,41,72,0,0,3,133,5,154,2,38,2,66,157,6,35,0,72,253,97,71,25,5,37,156,182,17,27,9,8,140,203,34,100,0,0,134,247,134,45,65,103,
    11,33,0,0,140,43,131,89,67,173,5,66,201,6,131,89,136,155,34,35,183,2,130,244,32,16,130,50,36,184,255,156,64,11,132,105,143,158,67,61,6,134,225,134,69,147,225,131,70,130,225,34,0,0,
    37,143,66,65,239,12,32,72,65,239,37,33,16,16,65,239,26,130,213,66,95,15,66,140,9,66,95,9,36,7,66,0,39,2,132,111,88,223,21,33,37,47,131,179,65,239,21,130,127,140,198,88,223,13,65,13,
    6,130,135,65,239,8,88,223,10,133,87,34,100,17,17,130,148,140,68,73,75,8,66,251,6,138,155,95,145,15,36,47,1,48,47,1,68,101,10,36,156,64,21,47,47,130,148,67,29,18,35,64,9,27,27,132,174,
    34,1,32,0,113,179,24,8,42,2,30,5,154,0,16,0,77,64,52,199,4,1,166,4,182,4,2,135,4,151,4,2,118,4,1,165,3,181,3,197,3,3,148,3,1,133,3,1,116,3,1,54,121,160,5,49,39,3,3,7,8,16,1,15,1,1,
    8,16,3,0,63,204,57,47,24,68,164,17,32,19,68,127,12,8,86,224,82,120,78,38,124,41,66,85,44,82,5,154,254,215,7,55,88,117,68,85,85,60,81,49,21,1,165,0,0,1,0,100,1,82,2,136,4,71,0,46,0,
    27,64,13,14,38,11,38,35,3,0,30,218,23,6,218,0,0,47,237,220,237,18,23,57,17,51,48,49,1,34,38,39,53,22,51,50,62,2,53,52,38,35,35,34,38,80,112,5,8,94,22,23,21,38,38,39,38,14,2,21,20,22,
    51,51,50,22,21,20,14,2,1,91,57,119,50,88,126,40,67,48,27,65,80,20,120,132,40,77,109,69,60,117,51,45,107,47,34,68,56,35,65,60,32,124,136,49,83,109,1,82,22,21,129,53,12,26,43,31,42,43,
    107,105,52,85,60,32,23,21,126,24,24,2,2,7,24,44,35,43,45,107,103,62,88,55,26,132,159,105,175,6,8,33,16,0,51,64,31,13,24,18,21,72,139,13,1,122,13,1,13,8,9,12,72,12,16,16,21,72,1,15,
    10,8,1,15,15,9,65,16,14,36,43,43,93,93,43,70,145,14,60,2,30,82,44,85,66,41,124,38,78,120,82,5,154,254,91,21,49,81,60,85,85,68,117,88,55,7,1,41,24,68,103,8,8,38,5,90,0,19,0,24,64,9,
    19,16,10,0,3,0,19,9,11,0,47,221,206,50,1,47,51,204,205,50,49,48,19,6,6,21,20,30,2,51,33,82,75,6,42,54,55,242,8,10,18,29,34,16,1,240,82,62,5,50,24,23,5,10,11,28,16,29,37,23,9,124,35,
    60,80,46,38,69,29,105,235,10,8,51,28,0,40,0,49,64,27,0,29,19,5,24,21,24,72,153,5,169,5,185,5,3,5,35,14,11,24,32,5,38,14,11,13,3,0,63,221,205,220,57,222,205,1,47,221,220,50,93,43,205,
    222,205,130,116,41,52,62,2,51,46,3,35,35,17,51,24,90,90,8,8,79,46,2,55,20,22,51,50,54,53,52,38,35,34,6,231,24,40,53,29,16,44,52,56,27,82,124,82,120,78,38,25,42,57,31,35,58,41,22,96,
    27,18,19,27,28,18,18,27,3,92,28,50,38,22,23,29,17,6,1,105,237,7,58,86,104,52,41,64,45,23,23,40,51,25,19,26,25,20,20,26,27,83,73,6,36,2,30,5,154,0,131,165,44,54,181,0,29,35,18,15,24,
    184,255,232,64,21,130,170,48,150,24,166,24,182,24,3,24,10,35,5,38,24,32,15,18,16,138,169,37,205,51,93,43,220,205,83,99,5,32,20,24,104,211,9,44,55,53,51,17,35,34,14,2,7,50,30,2,7,132,
    166,32,21,132,178,44,1,155,22,41,58,35,32,56,42,25,38,78,120,130,175,40,27,56,52,44,16,29,53,40,24,130,173,35,18,28,27,19,131,173,8,43,29,51,40,23,23,45,64,41,52,104,86,58,7,237,254,
    151,6,17,29,23,22,38,50,32,19,27,26,20,20,25,26,0,0,3,0,91,1,218,2,40,6,141,0,11,132,175,8,63,100,185,0,16,255,240,64,22,17,21,72,119,16,1,181,15,197,15,2,166,15,1,117,15,133,15,149,
    15,3,15,184,255,248,64,30,9,12,72,6,6,28,13,29,29,19,21,35,35,21,13,0,0,13,32,38,20,27,13,13,20,28,9,3,28,130,206,37,222,205,16,204,57,47,131,199,41,1,47,51,47,16,204,50,47,16,205,
    133,3,35,49,48,43,93,130,0,45,43,19,20,6,35,34,38,53,52,54,51,50,22,7,67,104,12,32,1,138,25,41,234,43,29,29,42,42,29,29,43,10,67,125,9,33,1,196,135,20,37,6,70,30,42,42,30,130,32,32,
    201,67,143,14,33,252,136,134,24,39,255,255,0,91,0,0,8,28,130,219,8,47,39,1,140,2,90,0,0,1,6,2,95,0,0,0,43,64,21,3,2,1,80,26,1,64,26,1,48,26,1,32,26,1,16,26,1,0,26,1,184,255,156,180,
    26,26,6,6,37,1,132,166,37,93,93,53,53,53,0,140,69,34,169,2,30,136,69,34,47,64,24,130,69,51,96,45,1,80,45,1,64,45,1,48,45,1,32,45,1,16,45,1,0,45,132,72,35,45,45,18,18,136,72,138,73,
    33,5,247,132,143,32,198,141,73,54,4,3,2,96,55,1,80,55,1,64,55,1,48,55,1,32,55,1,16,55,1,0,55,132,73,35,55,55,16,16,154,73,32,228,141,73,157,147,33,24,24,141,73,32,0,83,201,5,8,41,182,
    5,154,0,43,0,95,64,64,120,40,1,117,36,1,151,29,1,134,29,1,138,25,154,25,2,137,24,153,24,2,119,18,1,121,14,1,136,7,152,7,2,69,75,5,60,134,2,150,2,2,0,126,43,10,21,126,22,32,126,10,22,
    16,145,27,3,0,38,145,5,18,0,63,253,204,130,2,38,1,47,237,222,237,16,220,76,47,8,32,93,132,0,24,75,161,9,126,22,7,32,35,108,66,8,105,210,7,56,5,182,51,98,143,91,92,142,97,51,25,53,81,
    57,57,81,51,24,168,51,98,142,92,91,143,137,16,33,1,187,116,18,7,33,2,36,83,97,8,117,20,6,33,253,220,107,172,8,139,217,8,45,93,64,63,137,41,153,41,2,138,40,154,40,2,134,36,150,36,2,
    118,29,1,120,25,1,134,19,150,19,2,149,18,1,132,18,1,137,14,153,14,2,121,7,1,119,3,1,137,216,40,43,5,145,38,18,21,27,145,16,109,12,5,33,253,204,130,216,32,220,144,216,127,189,9,135,
    206,24,92,52,9,67,199,7,35,1,12,24,51,131,207,35,53,25,51,97,131,206,34,98,51,168,137,16,131,240,33,98,51,106,172,9,33,2,36,84,47,7,135,205,33,253,220,135,243,74,141,5,40,5,182,7,66,
    2,38,2,101,0,74,141,5,32,199,76,167,7,38,44,54,16,16,37,1,59,76,167,12,8,57,255,235,7,89,3,106,0,67,0,55,64,27,35,10,51,58,125,50,0,25,43,43,69,17,10,125,18,25,40,145,18,17,63,19,51,
    50,7,145,30,0,47,237,51,51,63,51,51,237,1,47,51,237,50,18,57,47,18,57,130,7,52,17,57,48,49,1,46,5,35,34,6,21,20,30,2,23,22,23,7,38,39,46,24,167,25,8,42,23,30,3,51,50,54,53,52,46,2,
    39,24,175,103,14,8,37,3,167,28,59,63,70,79,88,51,109,113,19,31,39,21,48,63,99,84,65,28,53,42,25,60,105,143,83,102,161,132,110,50,43,91,105,123,76,130,28,32,32,130,28,34,62,99,83,131,
    28,32,26,133,28,8,38,133,110,1,104,38,81,78,71,53,31,141,123,45,77,65,54,21,50,34,123,47,69,29,74,90,105,61,103,157,106,54,70,111,135,66,57,124,103,67,149,28,38,109,136,0,255,255,0,
    100,131,243,37,5,29,2,38,2,103,65,33,6,32,110,91,237,5,40,1,0,68,78,25,58,37,1,83,126,185,10,65,31,6,8,37,69,0,55,64,27,0,25,52,45,125,53,35,10,60,60,71,18,25,125,17,10,63,145,18,17,
    40,53,52,28,145,5,19,0,63,237,51,51,47,65,31,20,24,136,186,9,45,55,54,55,23,6,7,14,3,21,20,22,51,50,62,24,151,170,8,8,53,20,14,2,7,6,7,39,54,55,62,3,53,52,38,35,34,14,4,4,22,50,109,
    133,161,102,83,143,105,60,25,42,53,28,65,84,99,63,48,21,39,31,19,113,109,51,88,78,69,64,59,29,50,110,132,133,30,32,26,131,30,34,83,99,62,130,30,32,32,132,30,8,39,79,70,63,59,1,104,
    66,136,109,70,54,106,157,103,61,105,90,74,29,69,47,123,34,50,21,54,65,77,45,123,141,31,52,70,78,82,38,66,135,111,150,30,35,53,71,78,81,65,35,13,32,105,65,35,15,38,70,80,10,45,37,1,
    85,101,227,9,42,10,0,0,6,160,5,154,0,39,2,100,74,227,12,45,19,185,0,1,255,156,182,54,54,22,22,37,1,59,102,19,12,33,6,250,130,47,32,38,130,47,38,0,1,7,1,78,6,26,77,47,7,35,44,44,0,0,
    140,43,137,91,32,101,140,91,32,28,132,91,44,64,13,54,54,43,43,37,1,79,59,1,15,59,131,98,36,93,93,53,1,43,84,207,6,134,99,67,5,6,139,99,33,21,21,145,99,33,7,66,130,191,132,99,75,51,
    11,44,4,177,0,0,0,43,183,2,0,64,74,16,16,125,227,5,32,17,132,114,35,2,79,5,38,140,118,24,73,145,13,33,6,250,130,79,33,38,2,130,123,131,77,131,123,131,231,33,3,199,130,77,34,32,64,20,
    135,78,34,100,44,44,130,138,132,74,32,59,24,70,241,12,38,10,255,235,8,67,3,106,130,145,32,103,140,245,65,81,6,35,78,78,25,25,66,165,9,66,209,5,36,8,157,3,106,0,66,209,8,33,7,189,65,
    81,7,34,68,68,58,66,209,10,130,43,132,91,33,5,29,135,91,140,237,36,88,253,219,0,33,130,237,34,88,98,25,130,52,36,184,255,156,64,9,132,107,35,2,103,1,83,75,98,6,34,53,43,53,66,31,6,
    33,8,157,130,69,132,113,131,67,131,113,132,227,32,110,86,47,7,133,68,32,100,132,128,141,64,135,133,132,225,32,105,147,225,35,80,80,10,10,66,99,12,34,100,255,235,134,225,66,143,6,137,
    225,34,70,70,45,66,143,14,130,43,32,67,132,225,132,91,148,225,34,90,100,10,130,52,132,225,132,107,35,2,105,1,85,151,225,130,113,149,225,133,68,32,100,132,128,142,64,8,34,1,0,100,3,
    143,3,49,5,154,0,49,0,24,64,10,17,0,0,33,8,42,25,25,8,3,0,63,51,47,205,16,204,50,47,205,92,184,6,35,52,54,55,51,74,239,6,33,50,54,92,100,9,35,6,7,35,54,130,14,8,32,46,2,35,34,6,21,
    20,14,2,1,62,59,83,52,24,19,17,132,25,19,7,19,35,28,38,46,35,57,72,38,59,82,53,138,18,61,45,35,57,73,3,143,54,89,113,60,51,108,48,48,98,54,31,71,58,39,67,84,62,93,62,31,49,81,107,59,
    133,19,42,32,63,52,32,74,86,63,90,58,28,0,81,155,10,48,8,0,73,64,43,8,1,0,1,126,7,8,20,7,7,8,88,130,27,8,39,0,2,126,16,4,32,4,2,4,4,10,32,7,48,7,2,7,7,1,145,5,5,4,0,8,3,4,18,0,63,63,
    51,18,57,47,237,51,1,47,93,130,7,8,57,93,237,50,47,93,135,16,43,135,125,196,49,48,1,1,33,17,35,17,33,53,1,3,49,254,100,1,240,168,253,135,2,71,5,59,253,146,253,51,2,41,10,3,103,255,
    255,0,39,0,0,3,133,7,166,2,38,2,124,130,132,34,6,1,79,130,16,45,23,64,13,2,1,0,29,39,8,0,37,2,1,44,83,241,21,8,37,8,0,67,64,39,0,7,8,7,126,1,0,20,1,0,63,1,79,1,2,1,1,6,10,71,8,87,8,
    2,8,8,3,126,6,7,145,1,3,130,158,38,8,0,3,0,63,50,63,24,101,0,9,32,17,131,164,32,135,135,159,32,21,132,160,54,1,1,62,2,71,253,135,168,1,240,254,100,5,154,252,153,10,253,215,2,205,2,
    110,71,65,5,130,159,32,66,134,159,36,7,1,78,0,139,71,65,7,33,9,19,130,160,33,1,24,71,65,12,131,205,39,5,154,0,8,0,71,64,42,136,157,51,1,0,88,8,1,8,8,6,126,16,3,32,3,2,3,3,10,32,1,48,
    130,173,133,160,35,3,8,0,18,137,160,65,65,16,60,33,1,53,33,17,51,17,33,1,2,171,253,185,2,121,168,254,16,1,156,3,103,10,2,41,253,51,253,146,83,165,12,32,126,83,211,15,36,9,19,4,5,37,
    138,159,143,45,83,211,21,36,39,1,16,39,1,130,26,43,184,255,121,64,20,39,39,9,9,37,1,0,132,73,35,2,44,5,38,134,77,83,211,20,36,8,0,69,64,40,66,55,8,51,8,63,7,79,7,2,7,7,2,10,71,0,87,
    0,2,0,0,5,126,2,66,52,7,34,18,4,3,66,52,10,35,237,51,47,93,65,147,11,33,55,1,132,240,53,21,1,184,1,156,254,16,168,2,121,253,185,95,2,110,2,205,253,215,10,252,153,83,193,12,32,129,83,
    193,13,35,9,19,3,4,142,239,83,193,9,32,122,68,89,12,58,40,64,16,1,64,19,1,48,19,1,32,19,1,16,19,1,0,19,1,184,255,156,182,19,19,7,7,130,65,124,135,16,82,105,6,32,122,65,97,5,82,105,
    9,35,9,9,2,2,135,47,83,209,14,32,124,165,111,33,6,6,154,111,32,124,145,111,33,1,1,145,111,36,7,66,0,39,2,132,111,68,221,11,40,1,117,0,0,0,56,64,23,2,67,72,5,146,238,35,64,11,19,19,
    130,127,65,215,5,83,209,27,130,135,81,225,8,67,1,7,34,32,64,20,135,89,34,100,9,9,130,150,140,70,81,227,15,32,126,65,13,37,65,77,9,91,47,11,65,125,6,66,223,6,65,125,11,33,5,5,65,13,
    16,84,223,5,132,111,84,223,21,34,29,39,4,130,53,65,252,17,35,64,11,19,19,130,127,140,198,91,47,13,82,239,6,130,135,65,13,11,36,2,193,0,0,0,65,13,6,131,89,34,100,9,9,130,150,140,70,
    65,13,15,32,129,65,13,66,32,129,66,27,17,66,75,9,83,253,14,132,111,66,27,12,124,135,6,38,2,0,29,39,3,4,37,66,27,22,65,13,26,34,72,0,0,65,13,6,130,135,83,253,19,133,87,34,100,9,9,66,
    96,15,65,11,20,108,221,21,37,39,1,48,39,1,32,68,143,9,38,156,64,21,39,39,3,3,148,174,34,9,19,19,130,25,35,2,44,1,24,83,253,30,8,79,8,0,88,181,229,8,245,8,2,8,184,255,240,64,47,19,22,
    72,250,1,1,235,1,1,218,1,1,171,1,187,1,203,1,3,136,1,152,1,2,1,0,15,7,1,7,5,8,0,0,3,5,7,1,5,5,0,0,8,1,8,3,3,0,63,204,93,50,57,47,205,51,1,47,205,50,47,51,16,205,93,24,117,246,8,8,61,
    43,93,19,19,35,17,51,21,33,21,1,144,181,225,124,1,62,254,221,3,11,1,40,1,103,235,5,254,35,0,0,1,0,99,2,205,3,48,5,154,0,12,0,147,64,100,72,12,1,23,9,39,9,55,9,119,9,135,9,151,9,6,133,
    116,36,16,13,16,72,24,130,51,8,67,56,1,3,9,1,1,160,0,176,0,192,0,3,0,12,12,8,11,234,9,1,169,9,185,9,201,9,3,9,96,10,1,10,8,229,6,1,6,7,5,175,2,191,2,207,2,3,2,3,3,7,4,111,5,1,5,1,7,
    8,9,6,1,3,0,10,5,7,2,130,56,47,63,196,220,196,196,18,23,57,1,47,221,57,222,113,205,17,130,184,41,93,17,18,57,93,16,222,113,57,93,125,118,5,48,93,49,48,93,93,43,93,93,93,1,19,19,51,
    19,35,3,3,131,2,58,19,1,125,77,71,165,122,142,62,73,163,77,58,142,116,5,154,254,19,1,237,253,51,2,25,253,231,130,3,34,232,2,204,130,205,8,32,90,1,102,3,39,4,51,0,11,0,19,183,1,9,218,
    4,7,7,6,0,0,47,205,57,47,51,237,50,48,49,1,17,33,70,182,5,47,21,33,17,1,130,254,216,1,40,124,1,41,254,215,1,102,134,8,34,124,254,216,130,67,38,100,1,104,2,180,4,53,130,67,8,39,70,64,
    40,10,2,9,6,11,7,8,3,6,11,4,8,3,5,0,1,2,9,5,0,5,6,6,3,2,11,0,9,8,8,0,2,8,6,5,9,11,131,213,36,47,196,196,196,221,130,3,38,1,47,206,50,47,205,16,130,1,35,51,47,205,15,130,0,8,40,49,48,
    1,39,7,35,19,3,51,23,55,51,3,19,2,34,150,150,145,222,223,145,151,151,145,224,223,1,104,241,241,1,101,1,104,243,243,254,152,254,155,93,189,8,62,3,223,0,27,0,63,64,39,57,22,73,22,2,54,
    18,70,18,2,90,9,1,85,5,1,1,126,25,27,25,27,29,123,110,7,48,14,1,145,26,20,145,7,18,0,63,237,220,237,196,1,47,93,107,121,12,33,1,33,117,97,8,33,17,51,117,198,7,8,51,53,17,33,5,254,254,
    58,70,128,181,110,104,179,132,76,168,46,85,118,72,72,119,85,47,2,110,3,55,254,182,114,182,128,69,69,129,182,113,1,242,254,14,80,122,82,41,41,82,122,80,1,242,100,109,8,131,149,50,77,
    64,49,91,22,1,84,18,1,57,9,73,9,2,54,5,70,5,2,130,149,60,14,126,12,48,25,64,25,2,32,12,1,25,12,25,12,29,15,27,31,27,2,27,26,145,13,1,7,145,20,132,159,33,196,237,130,159,41,18,57,57,
    47,47,93,93,16,237,16,81,87,6,32,19,24,141,181,9,131,163,84,49,6,57,17,33,100,2,110,47,85,119,72,72,118,85,46,168,76,132,179,104,110,181,128,70,254,58,3,223,139,150,43,254,14,113,182,
    129,69,69,128,182,114,1,74,65,57,13,130,163,8,33,54,22,70,22,2,57,18,73,18,2,85,9,1,91,5,1,25,126,1,12,126,14,48,1,64,1,2,32,14,1,1,14,1,14,134,163,38,20,145,7,14,1,145,26,131,163,
    33,196,220,148,163,33,55,33,107,214,8,24,71,219,10,130,163,33,1,198,65,70,16,36,253,146,168,1,74,65,69,7,35,254,14,1,242,65,69,7,33,254,14,101,223,8,36,229,2,38,2,154,130,176,8,36,
    7,1,79,1,123,254,63,0,45,64,29,2,1,0,48,58,20,20,37,2,1,159,63,1,95,63,1,79,63,1,63,63,1,31,63,1,15,130,8,34,0,47,93,132,0,109,181,7,38,1,0,100,0,0,5,254,66,35,7,55,85,22,1,90,18,1,
    54,9,70,9,2,57,5,73,5,2,26,126,2,27,2,27,29,13,92,164,6,38,7,145,20,26,145,13,1,130,223,33,196,237,132,223,66,35,12,32,33,130,219,36,46,2,35,34,14,24,180,23,12,37,17,33,5,254,253,146,
    65,128,16,33,1,198,139,207,33,1,242,65,126,7,33,254,182,136,219,32,29,136,219,44,78,1,223,253,219,0,29,64,18,1,0,28,38,130,218,42,1,159,43,1,95,43,1,31,43,1,43,132,208,33,53,1,101,
    197,8,8,55,4,67,5,154,0,27,0,70,64,43,85,23,1,87,22,1,84,17,1,72,10,1,72,4,1,20,126,48,7,1,7,7,29,26,126,13,15,1,31,1,2,1,25,145,1,1,14,27,18,12,145,14,3,0,63,237,95,44,7,32,196,130,
    210,32,47,24,82,140,8,37,51,17,33,50,62,2,24,94,76,15,34,33,17,100,151,188,66,106,20,133,209,35,4,67,7,66,130,209,34,156,0,0,24,68,231,8,34,19,64,11,131,209,36,14,20,37,1,43,75,97,
    14,137,199,8,39,72,23,1,72,17,1,84,10,1,87,5,1,85,4,1,7,126,48,20,1,20,20,29,14,1,126,15,26,31,26,2,26,26,145,2,2,0,14,145,12,94,60,5,131,190,39,237,1,47,93,237,196,18,57,137,199,32,
    1,130,199,34,30,2,21,105,200,14,35,33,17,1,12,66,97,23,33,5,154,67,216,20,130,188,62,0,99,0,0,4,66,5,154,0,27,0,66,64,40,70,23,1,70,17,1,91,10,1,92,4,1,14,26,126,48,1,130,0,48,29,20,
    126,15,7,31,7,2,7,1,145,25,25,0,15,145,13,97,56,5,136,154,131,153,33,253,196,104,237,5,34,33,17,33,24,76,44,19,34,17,3,154,68,65,23,67,41,20,34,0,255,255,133,151,37,7,66,2,38,2,159,
    130,166,39,7,1,78,2,24,0,0,0,65,99,6,33,7,13,65,99,14,140,197,60,91,23,1,90,17,1,70,10,1,71,4,1,13,1,126,48,26,1,26,26,29,7,126,15,20,31,20,2,20,131,197,38,15,27,3,13,145,15,18,66,
    40,10,131,205,32,93,135,197,32,1,130,197,33,14,2,89,194,12,38,62,2,51,33,17,4,66,68,100,23,33,5,154,67,20,20,131,199,40,10,0,0,5,44,7,66,0,39,130,199,32,234,130,193,33,39,1,24,103,
    53,8,34,3,2,0,74,239,6,33,48,58,131,209,62,64,38,1,48,38,1,32,38,1,16,38,1,0,38,1,184,255,156,64,11,38,38,7,7,37,2,63,5,38,1,43,74,239,17,130,245,33,5,135,130,91,32,38,130,91,46,0,
    0,39,1,78,4,167,251,251,1,7,1,78,2,25,130,89,36,32,64,20,2,0,133,89,36,100,28,28,26,26,141,70,34,53,43,53,116,85,6,49,76,5,154,0,27,0,33,64,15,14,26,1,7,20,1,26,26,0,130,24,40,15,13,
    3,0,63,205,204,93,57,92,62,5,38,221,196,49,48,1,53,35,65,220,7,101,27,10,8,45,51,17,1,208,85,67,104,71,37,37,71,104,67,209,211,26,54,44,29,28,45,54,26,211,2,205,229,33,64,90,55,52,
    90,66,38,124,14,29,42,36,36,45,27,11,254,159,72,85,13,54,111,64,67,10,6,3,11,3,4,12,12,14,7,1,4,6,10,9,11,5,4,3,12,130,36,8,42,116,4,132,4,148,4,3,4,5,137,5,153,5,169,5,3,169,5,185,
    5,201,5,3,9,5,9,5,8,2,20,1,36,1,52,1,3,0,1,1,2,1,7,8,101,44,8,42,17,57,57,47,47,93,113,16,205,93,17,130,181,34,57,17,18,101,47,5,39,198,196,18,57,47,18,23,57,131,193,35,5,53,37,37,
    131,2,45,5,21,5,5,3,48,253,52,2,24,253,231,2,25,130,3,50,205,254,19,1,237,3,64,115,141,58,77,163,73,62,143,123,165,71,77,70,219,7,8,51,5,154,0,12,0,133,64,20,133,12,149,12,2,118,12,
    1,154,10,1,137,10,1,104,10,120,10,2,9,184,255,240,64,59,15,18,72,149,5,1,135,5,1,154,4,1,89,4,137,4,2,0,16,130,18,34,10,3,6,136,216,39,12,0,3,5,4,11,10,9,130,235,32,6,130,21,43,3,1,
    7,126,8,3,2,126,1,18,0,63,101,30,11,34,51,17,51,131,2,34,1,24,47,138,179,34,0,43,93,130,0,132,4,34,93,1,1,134,191,32,1,130,191,8,41,5,254,250,104,4,188,251,66,4,190,251,66,5,154,251,
    153,4,103,1,35,254,221,182,216,225,185,217,224,185,254,207,211,201,215,0,2,0,100,0,0,5,48,130,193,8,136,25,0,45,0,171,64,59,9,24,1,8,21,1,6,15,38,15,2,40,5,1,9,5,25,5,2,72,0,1,213,
    20,1,198,20,1,145,20,1,101,20,117,20,133,20,3,84,20,1,20,26,25,26,24,1,24,21,36,125,16,12,64,12,80,12,3,12,184,255,192,64,51,16,20,72,12,12,26,136,22,1,3,22,125,27,21,1,23,21,21,23,
    47,26,125,2,146,21,1,2,112,21,128,21,2,84,21,100,21,2,21,17,25,24,18,20,17,145,31,41,145,23,7,3,0,63,196,253,222,237,50,63,196,18,57,93,93,95,93,116,3,5,49,25,47,24,47,93,233,95,93,
    17,57,47,43,93,237,17,51,93,51,131,29,117,215,10,33,19,38,82,74,9,42,14,2,35,34,34,39,19,1,51,1,35,24,103,50,15,8,45,129,29,53,93,123,71,70,124,92,53,53,92,124,70,9,17,9,206,2,27,178,
    253,61,20,254,173,28,49,65,37,37,64,48,27,27,48,64,37,37,65,49,28,3,187,64,76,135,39,46,71,124,92,54,1,254,98,4,69,250,102,4,70,38,65,131,36,33,66,38,133,7,32,65,69,221,5,8,33,5,48,
    7,166,2,38,2,169,0,0,1,7,1,79,1,144,0,0,0,23,64,13,3,2,0,67,77,0,1,37,3,2,82,5,24,111,129,12,65,101,7,8,81,26,0,46,0,127,64,78,38,10,1,146,5,1,116,5,132,5,2,102,5,1,5,27,23,1,1,1,0,
    4,37,125,13,136,3,1,3,3,125,2,20,13,148,13,2,13,2,4,2,13,4,3,48,27,125,23,42,145,5,157,4,1,2,127,4,143,4,2,107,4,1,4,0,144,8,1,8,8,0,32,145,2,18,70,6,5,37,196,237,17,57,47,93,65,64,
    5,48,51,237,1,47,237,18,23,57,25,47,24,47,47,93,16,237,95,130,3,34,17,51,51,132,29,43,93,49,48,93,1,51,1,35,1,3,54,50,24,72,148,12,35,52,54,55,23,24,100,131,14,43,2,89,20,2,195,178,
    253,229,206,9,17,9,65,24,8,37,123,93,53,15,14,133,65,59,15,44,5,154,250,102,4,69,254,98,1,54,92,125,70,135,43,45,38,74,28,139,39,65,48,27,27,48,65,38,38,66,132,7,65,59,8,32,66,65,59,
    8,53,78,1,244,0,0,0,19,64,11,2,0,47,57,0,1,37,2,62,5,38,0,43,71,225,5,66,157,5,8,65,254,4,204,0,26,0,46,0,166,64,105,154,21,1,153,20,1,153,16,1,154,15,1,149,11,1,151,10,1,87,26,1,22,
    20,1,8,11,1,25,10,1,10,10,1,3,42,125,5,155,4,1,140,4,1,2,127,4,1,107,4,1,4,1,128,8,1,131,1,8,36,48,32,125,3,18,13,125,31,37,175,37,2,37,37,3,137,5,1,122,5,1,5,4,27,125,23,1,43,0,1,
    0,55,4,71,4,2,32,130,47,8,32,4,23,3,125,2,18,0,63,237,204,57,25,47,93,93,51,93,51,24,16,237,17,57,93,93,17,57,47,93,237,1,47,196,73,3,5,32,93,65,107,5,39,93,51,237,49,48,0,95,93,131,
    0,32,1,132,5,41,93,1,21,1,53,1,37,22,20,21,24,135,151,14,32,7,111,227,7,72,97,6,33,5,254,65,51,37,24,97,129,7,65,134,11,33,70,124,65,134,20,32,0,65,95,6,39,254,6,116,2,38,2,171,0,72,
    121,5,34,71,255,50,118,207,5,38,47,57,23,23,37,2,62,24,112,13,9,35,100,0,0,5,65,93,7,8,59,154,64,96,153,16,1,10,16,1,153,15,1,149,11,1,150,10,1,149,6,1,149,5,1,101,23,1,87,0,1,32,125,
    18,21,18,42,125,23,8,112,22,128,22,144,22,3,22,26,32,18,1,15,18,1,18,8,18,8,48,26,65,86,8,63,27,23,125,24,123,21,1,21,22,27,125,3,43,26,1,26,25,71,22,1,32,22,48,22,2,22,22,3,24,18,
    0,63,65,83,6,33,51,93,65,83,5,33,16,237,65,84,6,75,219,6,46,18,57,93,16,196,237,17,51,16,237,49,48,0,93,93,65,80,6,35,93,1,54,54,66,171,13,38,52,55,5,1,21,1,53,113,53,8,65,81,6,36,
    4,31,28,74,38,67,194,35,38,4,175,14,15,53,93,124,66,169,8,43,9,17,9,206,253,229,178,2,195,20,1,83,66,218,15,65,83,13,32,173,65,83,5,33,4,59,65,83,9,33,3,3,65,83,9,46,255,255,0,10,0,
    0,6,232,6,116,0,39,2,173,0,71,205,6,37,5,37,255,50,1,7,71,213,5,8,33,0,54,64,16,3,64,77,1,48,77,1,32,77,1,16,77,1,0,77,1,184,255,156,64,16,77,77,25,25,37,2,0,47,57,130,79,35,3,82,2,
    62,87,171,8,111,183,13,130,89,32,38,130,89,71,203,5,130,133,131,87,45,6,98,251,251,0,30,64,18,3,100,67,67,8,8,148,68,8,86,53,0,2,0,100,2,205,3,49,5,171,0,23,0,35,0,67,64,35,23,11,33,
    27,22,2,21,27,250,21,1,21,24,2,1,22,22,6,224,23,240,23,2,23,0,64,19,24,72,0,16,30,24,6,4,0,63,221,222,221,220,43,205,93,18,57,25,47,51,51,18,57,93,1,24,47,221,205,57,16,222,221,196,
    49,48,1,1,53,65,153,14,35,39,7,1,3,24,128,34,11,60,49,253,51,1,221,14,37,19,35,62,46,27,27,46,62,35,32,55,44,30,5,161,1,241,170,20,27,27,20,130,3,40,2,205,1,157,10,1,40,7,8,131,28,
    131,36,49,21,36,46,26,105,254,229,1,220,28,20,20,26,26,20,20,28,0,70,161,6,8,137,94,5,154,0,39,0,56,0,174,64,122,37,38,101,38,2,22,38,1,86,37,134,37,150,37,3,117,32,133,32,2,6,32,102,
    32,2,118,31,134,31,2,101,31,1,102,27,118,27,134,27,3,133,26,1,116,26,1,101,26,1,137,22,1,122,22,1,107,22,1,121,21,1,104,21,1,137,15,1,123,15,1,133,11,1,116,11,1,105,2,1,42,2,1,25,2,
    1,29,126,46,7,126,5,32,5,1,46,5,46,5,58,35,40,126,15,18,31,18,2,18,34,145,40,40,0,51,145,6,24,3,0,145,13,18,0,63,237,63,196,24,83,230,8,32,50,66,180,5,78,143,9,32,93,145,0,36,37,50,
    62,2,53,24,94,212,13,34,52,62,2,66,204,8,35,35,30,3,1,101,15,5,77,71,6,8,49,2,225,132,180,110,47,168,71,153,242,171,172,242,153,70,67,112,145,78,79,146,111,67,67,111,146,79,230,11,
    58,109,167,254,163,236,43,84,66,40,40,65,84,44,46,85,66,39,164,87,150,24,95,6,15,63,1,64,85,147,109,62,62,108,148,85,87,147,107,61,98,169,126,72,2,121,36,63,85,50,52,87,62,34,34,62,
    87,52,65,79,14,8,43,176,64,123,102,38,1,21,38,37,38,2,139,29,1,124,29,1,132,25,1,115,25,1,102,19,118,19,2,134,18,1,117,18,1,100,18,1,139,14,1,121,14,1,106,130,5,8,81,13,137,13,2,106,
    13,1,105,9,121,9,137,9,3,122,8,138,8,2,107,8,1,9,8,1,154,3,1,89,3,137,3,2,42,2,106,2,2,25,2,1,50,126,11,21,126,56,5,56,32,56,1,11,56,11,56,58,34,126,15,32,31,32,2,32,6,145,56,56,45,
    0,145,27,18,45,145,34,16,3,0,63,196,24,133,48,10,65,79,5,33,17,51,65,81,32,32,55,74,101,8,36,50,30,2,21,17,24,96,49,11,33,17,20,24,198,141,13,41,51,51,2,225,120,167,109,58,11,230,65,
    71,7,8,50,78,145,112,67,70,153,242,172,171,242,153,71,168,47,110,180,2,89,39,66,85,46,44,84,65,40,40,66,84,43,236,164,72,126,169,98,61,107,147,87,85,148,108,62,62,109,147,85,254,192,
    145,24,96,99,10,39,253,45,111,199,150,87,3,99,65,78,7,35,50,85,63,36,66,163,15,8,88,166,64,113,88,53,1,89,47,1,86,43,1,106,38,1,25,38,1,134,29,1,115,29,1,137,25,1,124,25,1,121,19,1,
    106,19,1,137,18,1,122,18,1,105,18,1,134,14,1,117,14,1,102,14,1,115,13,1,101,13,1,100,9,116,9,2,117,8,1,6,8,102,8,2,150,3,1,102,2,1,21,2,1,11,126,50,32,126,34,50,130,1,55,58,5,56,126,
    15,21,31,21,2,21,56,145,6,6,45,0,145,27,3,45,145,34,16,18,65,73,13,66,154,5,65,71,28,34,93,93,1,24,135,221,7,70,168,8,24,96,163,12,34,17,52,46,24,147,48,12,33,35,35,65,73,31,33,253,
    167,65,73,12,43,4,246,72,126,170,97,61,107,147,87,86,147,65,74,5,34,1,64,145,24,96,214,10,39,2,211,111,199,150,87,252,157,65,74,7,35,49,86,63,36,66,157,14,8,72,162,64,111,88,53,1,86,
    49,1,87,43,1,107,38,1,26,38,1,153,37,1,122,32,1,107,32,1,9,32,1,120,31,1,105,31,1,121,27,1,104,27,1,122,26,138,26,2,105,26,1,101,22,117,22,133,22,3,102,21,118,21,2,133,15,1,116,15,
    1,138,11,1,123,11,65,71,6,39,46,126,29,35,19,126,40,29,130,1,33,58,5,78,179,6,45,41,145,35,35,0,51,145,6,24,18,0,145,13,3,67,226,13,39,18,57,57,47,47,16,237,51,82,113,7,67,224,17,33,
    93,1,80,254,7,40,18,54,54,51,50,22,22,18,21,24,128,104,12,35,51,46,3,1,24,172,65,12,67,225,31,33,1,93,67,225,12,37,4,246,87,150,199,111,24,98,17,13,33,254,192,67,226,5,47,147,86,87,
    147,107,61,97,170,126,72,253,135,36,63,86,49,67,226,8,72,5,6,38,94,7,66,2,38,2,180,70,177,5,33,2,113,73,101,7,38,57,67,27,27,37,2,72,73,101,8,140,45,32,181,145,45,33,13,13,139,45,65,
    161,6,32,254,69,143,6,8,95,194,64,134,89,52,1,86,48,1,87,42,1,89,38,1,90,37,1,11,37,1,118,30,1,101,29,1,22,29,1,117,25,1,100,25,1,21,24,1,122,19,1,107,19,1,121,18,1,106,18,1,120,14,
    1,105,14,1,137,13,153,13,2,123,13,1,106,13,1,119,9,1,98,9,1,7,9,1,118,8,1,155,4,1,141,4,1,157,3,1,139,3,1,89,3,1,11,130,5,42,2,137,2,2,5,126,55,27,126,0,55,130,1,59,58,45,126,34,15,
    16,31,16,2,16,11,145,50,50,21,34,145,32,18,5,56,145,21,3,0,63,237,50,24,90,112,8,73,163,6,67,0,30,65,205,6,36,1,52,46,2,39,73,172,12,48,33,50,4,22,18,21,20,2,6,4,35,33,53,33,50,62,
    2,72,104,12,34,53,5,90,66,216,49,37,2,205,130,194,134,76,68,133,13,34,84,176,254,24,99,19,10,33,2,182,67,61,12,65,149,6,38,254,7,66,2,38,2,184,93,5,13,37,2,0,57,67,56,5,65,149,26,8,
    77,148,64,101,88,48,104,48,2,102,44,1,87,44,1,86,38,1,85,37,1,54,37,70,37,2,121,32,1,121,31,1,116,27,1,118,26,1,117,22,1,117,21,1,26,16,106,16,2,105,15,121,15,2,25,11,121,11,2,107,
    10,1,85,3,1,4,3,1,86,2,1,40,126,34,6,24,126,51,34,130,1,60,58,0,126,15,13,31,13,2,13,29,145,46,46,6,35,40,145,19,3,6,145,8,18,0,63,237,63,237,50,114,118,9,37,57,47,47,16,253,196,67,
    45,22,36,1,20,30,2,51,24,98,184,13,24,85,21,11,36,53,14,3,1,21,76,129,11,33,1,8,70,221,49,33,2,205,24,98,234,14,71,65,13,35,76,134,194,1,71,65,13,24,99,9,12,33,2,186,65,103,17,33,35,
    40,67,43,14,40,10,0,0,6,232,7,66,0,39,130,45,81,173,15,112,111,6,35,3,0,77,87,131,55,62,64,67,1,48,67,1,32,67,1,16,67,1,0,67,1,184,255,156,64,11,67,67,13,13,37,3,92,5,38,2,72,81,173,
    17,32,100,24,96,53,7,130,91,36,0,0,39,1,78,119,203,7,91,143,7,135,89,36,100,57,57,24,24,141,70,33,53,43,73,227,9,73,43,5,8,38,43,64,21,38,16,51,40,26,5,32,26,0,38,21,46,64,16,20,72,
    46,27,40,11,3,0,63,221,50,222,43,221,222,205,1,47,222,205,16,221,73,203,5,81,183,8,65,147,14,32,21,65,181,5,65,154,14,24,95,217,8,32,138,24,86,15,11,43,34,56,41,20,36,63,88,47,1,103,
    254,194,126,25,7,36,17,26,33,15,2,24,95,240,8,126,67,11,44,82,12,45,57,68,37,49,85,65,36,124,2,81,24,86,103,8,35,27,39,22,10,70,5,10,8,53,25,0,29,0,80,64,52,88,8,1,105,7,121,7,2,102,
    3,118,3,2,85,3,1,7,3,23,3,2,25,126,31,23,79,23,2,23,26,12,126,16,10,64,10,2,10,28,126,26,29,29,5,25,11,27,24,104,109,9,43,196,18,57,47,1,47,237,222,93,237,16,220,85,229,8,32,1,66,102,
    7,73,153,13,33,51,1,130,15,24,104,114,19,33,1,129,24,104,117,26,37,253,51,2,205,253,51,72,3,11,131,179,43,84,64,55,135,20,1,87,8,1,117,7,1,24,104,66,7,52,89,3,105,3,121,3,3,11,126,
    31,13,79,13,2,13,28,24,126,16,0,64,130,235,36,26,126,28,26,26,130,182,36,28,18,5,145,18,70,129,5,134,182,130,178,138,182,33,93,1,106,55,7,32,17,24,104,78,13,32,1,130,15,24,104,82,19,
    34,254,127,168,24,103,132,9,70,67,13,130,13,130,181,33,2,205,24,70,191,8,34,5,154,0,131,183,8,41,110,64,72,4,21,1,118,20,1,101,20,1,6,20,22,20,2,118,16,1,21,16,101,16,2,6,16,1,5,15,
    1,89,8,1,89,7,1,89,3,1,152,2,126,32,5,33,5,29,130,1,34,31,25,11,89,179,5,43,29,145,27,27,10,25,145,23,18,10,145,12,132,200,86,2,7,32,196,130,208,34,57,47,47,68,25,16,32,37,87,109,11,
    69,116,10,37,1,33,53,33,3,55,24,105,16,23,32,2,132,186,55,168,64,135,207,143,145,207,134,63,168,86,176,254,241,184,186,254,241,176,84,168,1,209,168,70,161,10,131,209,50,106,64,70,86,
    23,150,23,2,87,22,1,87,18,1,86,17,1,12,10,125,234,6,53,9,9,25,9,2,121,5,1,10,5,26,5,106,5,3,9,4,1,14,0,28,29,130,1,34,31,20,125,86,207,5,43,26,145,28,28,2,15,145,13,3,0,145,2,68,236,
    5,35,17,57,47,237,91,43,8,33,16,196,86,212,6,70,73,6,32,37,24,103,149,13,24,99,10,8,37,19,33,21,33,5,254,24,106,172,13,74,72,5,36,87,150,199,111,6,131,206,55,168,168,84,176,1,15,186,
    184,1,15,176,86,168,63,134,207,145,143,207,135,64,2,121,168,65,159,6,8,153,8,53,4,31,0,17,0,35,0,223,64,137,34,33,30,31,35,132,32,148,32,2,101,32,117,32,2,32,29,126,20,25,24,21,64,
    22,1,22,26,122,23,138,23,154,23,3,105,23,1,23,20,20,2,37,16,15,12,13,17,132,14,148,14,2,101,14,117,14,2,14,11,126,2,7,6,3,64,4,1,4,8,122,5,138,5,154,5,3,105,5,1,5,2,33,34,34,22,21,
    21,26,29,23,32,35,20,6,28,19,1,24,25,25,31,30,30,28,10,13,12,12,6,7,7,8,11,5,14,17,2,6,32,10,48,10,128,10,144,10,4,10,1,15,16,16,4,3,3,1,18,0,63,51,47,51,130,2,39,16,205,93,23,57,50,
    47,51,130,2,33,16,196,130,4,131,18,33,196,17,132,18,130,10,45,1,47,51,93,93,196,221,93,50,198,50,16,253,50,131,10,37,50,196,50,17,18,57,147,22,52,49,48,33,35,17,1,39,1,1,55,1,17,51,
    17,1,23,1,1,7,1,1,144,17,57,2,142,168,254,246,120,1,101,254,156,119,1,10,168,1,21,120,254,156,1,100,119,254,234,4,26,149,23,33,1,57,131,46,32,100,131,35,41,246,1,67,254,177,1,22,120,
    254,157,131,60,34,22,254,186,153,27,67,209,13,8,72,51,0,112,64,73,119,42,135,42,2,153,33,1,150,29,1,137,20,153,20,2,124,20,1,26,20,1,133,16,149,16,2,115,16,1,21,16,1,106,7,1,9,7,1,
    6,3,102,3,2,12,126,10,49,126,51,51,36,53,23,126,25,38,126,36,44,145,31,31,5,50,25,11,38,68,154,10,32,196,71,56,5,35,237,222,237,17,131,9,32,222,110,46,16,34,30,2,51,78,63,17,33,51,
    1,82,87,7,24,121,232,12,68,183,19,51,3,30,50,88,121,70,70,120,88,50,166,18,39,61,44,44,62,39,18,166,24,109,62,25,37,254,153,74,131,97,56,24,122,22,7,41,39,70,53,32,32,53,70,39,1,103,
    69,141,12,8,83,51,0,112,64,73,150,48,1,153,44,1,119,35,135,35,2,102,22,1,5,22,1,9,18,105,18,2,151,9,1,134,9,1,20,9,116,9,2,137,5,153,5,2,124,5,1,26,5,1,12,126,14,27,126,51,51,41,53,
    2,126,0,39,126,41,33,145,46,46,20,26,14,0,39,18,20,145,7,3,0,63,237,63,196,196,65,7,30,32,33,68,236,13,32,17,69,4,7,32,1,24,122,109,11,93,101,8,53,1,12,168,71,153,242,171,171,242,153,
    71,168,49,111,179,130,128,179,112,50,3,30,130,255,35,62,44,44,61,130,255,39,50,88,120,70,70,121,88,50,24,108,171,23,35,253,45,1,103,135,251,33,254,153,24,122,57,9,68,85,12,52,51,0,
    118,64,77,136,47,1,137,41,1,118,34,150,34,2,118,28,150,28,2,69,55,8,38,118,16,1,100,16,1,4,69,45,10,43,89,2,153,2,2,44,126,31,18,126,5,31,130,1,46,53,38,25,11,50,38,145,36,0,145,24,
    18,49,145,51,69,44,5,35,253,222,237,63,130,3,34,1,47,196,69,44,8,69,46,16,32,93,69,47,24,71,181,7,74,175,5,38,53,52,46,2,35,33,53,69,65,25,137,215,131,227,137,239,69,83,20,42,3,139,
    56,98,130,75,74,131,97,56,166,24,124,67,7,32,166,70,55,13,8,35,51,0,124,64,80,153,49,1,152,43,1,134,36,1,135,30,1,151,23,1,85,23,1,86,22,1,86,18,1,86,17,1,27,10,1,10,69,117,7,51,121,
    5,1,108,4,1,27,4,1,10,4,1,33,126,46,27,14,0,39,46,130,1,51,53,20,126,7,28,145,26,15,145,13,3,39,145,41,25,145,1,18,0,63,65,18,8,85,114,5,34,16,196,196,74,91,21,69,119,23,32,1,24,104,
    141,11,92,118,8,69,137,25,35,2,211,254,153,66,247,9,67,15,9,69,156,20,44,3,139,166,23,48,73,49,49,72,48,23,166,56,24,124,74,7,66,37,6,8,40,138,5,154,0,21,0,38,0,146,64,97,55,35,1,55,
    29,1,56,25,1,69,14,85,14,2,103,13,1,86,13,1,68,13,1,104,9,1,90,9,1,73,130,2,8,40,8,89,8,2,90,4,1,72,4,1,91,3,1,74,3,1,150,20,1,20,17,126,38,154,18,1,89,18,1,18,19,151,19,1,0,19,0,19,
    40,32,125,126,152,5,60,19,18,18,38,145,1,1,151,17,1,17,21,18,27,145,11,3,0,63,237,63,51,93,57,47,237,51,47,51,70,153,8,40,93,17,51,93,93,196,237,50,93,24,150,116,16,32,1,24,75,43,13,
    33,1,23,67,70,10,24,104,114,20,36,1,58,119,254,79,24,69,181,8,24,104,119,19,38,253,14,1,56,118,254,81,24,104,124,14,37,2,0,100,0,0,5,65,9,7,47,152,64,101,55,35,1,56,31,1,56,25,1,84,
    18,1,69,130,2,8,67,17,85,17,2,70,13,86,13,2,103,12,1,84,12,1,69,12,1,104,8,1,75,8,91,8,2,88,7,1,74,7,1,22,21,126,4,169,1,1,154,1,1,1,4,15,125,28,64,4,80,4,2,4,28,4,28,40,147,3,1,86,
    3,1,3,15,2,31,2,130,0,43,3,3,22,145,20,20,0,33,145,10,3,4,106,116,5,33,237,17,65,13,8,43,51,93,93,18,57,57,47,47,93,16,237,17,130,11,33,16,253,71,176,13,39,93,93,93,33,35,1,55,1,68,
    61,8,95,48,14,43,34,14,2,21,2,189,168,254,79,119,1,58,24,104,149,27,38,1,175,118,254,200,2,242,114,181,7,24,104,154,16,76,101,7,38,138,7,66,2,38,2,201,76,101,5,33,3,95,79,145,7,38,
    39,49,10,10,37,2,54,88,247,15,65,59,7,35,140,64,93,56,130,165,8,59,31,1,55,25,1,105,18,1,75,18,91,18,2,90,17,1,73,17,1,74,13,90,13,2,105,12,1,74,12,90,12,2,86,8,102,8,2,69,8,1,86,7,
    1,69,7,1,89,3,1,149,1,1,1,4,126,22,157,3,1,3,130,198,43,21,2,40,28,125,15,15,31,15,2,15,2,65,51,9,37,18,4,0,3,0,63,79,57,6,66,65,11,33,17,51,66,63,22,34,51,1,7,24,79,144,28,39,3,49,
    168,1,177,119,254,198,24,79,149,11,24,104,215,17,41,254,81,118,1,56,253,14,92,161,120,24,106,219,9,24,104,220,11,65,49,13,32,203,24,88,119,13,37,2,0,39,49,0,1,65,49,26,8,36,150,64,
    101,56,35,1,57,29,1,55,25,1,74,14,90,14,2,91,13,107,13,2,74,13,1,103,9,1,69,9,85,9,2,85,8,1,70,130,2,8,49,4,86,4,2,102,3,1,69,3,85,3,2,0,38,126,16,171,20,1,154,20,1,20,16,6,125,32,
    64,16,80,16,2,16,32,16,32,40,146,18,1,85,18,1,18,15,19,31,19,2,19,67,126,6,43,17,21,3,27,145,11,18,0,63,237,63,51,66,108,41,32,1,24,104,203,13,34,1,39,1,101,90,9,38,52,46,2,35,33,2,
    189,24,79,200,13,36,254,198,119,1,177,67,123,12,24,104,211,15,38,2,242,254,200,118,1,175,24,104,216,13,80,59,6,66,109,5,32,205,65,59,5,33,1,249,66,109,9,33,20,21,65,59,15,35,2,205,
    2,218,68,179,6,8,60,84,183,9,32,38,245,21,1,21,0,184,255,192,64,38,24,27,72,0,1,20,3,38,244,0,1,0,21,21,38,4,4,1,27,0,14,1,14,42,20,1,255,20,1,155,20,1,138,20,1,20,1,3,0,63,51,93,93,
    93,113,220,103,127,5,53,51,47,51,93,1,47,196,221,50,205,43,50,93,16,222,205,49,48,19,55,51,17,89,194,12,33,17,7,92,109,12,35,35,100,188,124,24,76,65,12,33,100,224,24,101,6,13,35,223,
    187,254,237,24,101,9,12,34,64,99,221,24,101,9,13,38,1,0,100,0,0,4,154,130,197,58,11,0,59,64,33,11,126,2,32,9,1,9,9,5,13,8,3,126,15,5,31,5,2,5,3,145,47,131,17,42,4,10,7,3,1,4,18,0,63,
    196,63,99,51,5,32,1,24,108,161,7,44,93,196,237,49,48,33,35,17,33,17,35,17,51,130,5,53,51,4,154,168,253,26,168,168,2,230,168,2,142,253,114,5,154,253,139,2,117,0,97,19,6,32,180,132,105,
    34,71,64,41,103,227,24,40,9,8,8,11,0,2,6,5,9,130,5,35,3,8,9,3,130,113,44,221,196,196,196,16,196,196,1,47,222,205,51,47,103,228,36,33,2,205,103,228,11,65,217,5,47,6,37,7,166,0,39,1,
    193,2,76,0,0,0,38,2,164,130,146,36,7,1,79,3,150,130,13,8,43,60,64,27,4,3,0,84,94,7,7,37,2,80,49,1,64,49,1,48,49,1,32,49,1,16,49,1,0,49,1,184,255,156,64,10,49,49,2,2,37,4,3,99,5,94,
    160,6,76,188,5,35,53,43,53,53,135,93,130,217,134,93,33,1,6,131,93,35,0,39,64,19,149,77,32,180,132,76,122,47,8,67,181,6,130,159,32,66,144,159,34,78,3,250,130,159,38,56,64,26,3,0,64,
    74,153,158,32,9,132,81,33,3,79,132,157,136,88,32,43,141,155,32,195,131,249,162,155,34,12,12,37,144,155,33,65,7,131,155,132,65,65,59,8,34,78,5,97,137,155,33,0,1,65,58,23,130,155,130,
    81,158,155,32,198,166,155,33,17,17,145,155,32,37,132,155,132,65,137,155,33,2,148,137,155,33,17,18,154,155,130,81,145,155,8,51,0,3,0,100,2,205,4,106,5,154,0,27,0,46,0,63,0,67,64,35,
    34,57,63,28,44,7,20,14,1,26,44,1,223,26,239,26,255,26,3,26,26,29,63,63,13,52,27,0,39,1,39,46,99,217,5,56,196,220,93,196,205,17,57,47,205,51,47,93,205,1,47,222,205,196,222,205,16,221,
    196,220,205,99,231,25,24,104,231,29,34,1,206,83,100,5,8,32,209,100,5,7,33,209,224,67,247,12,32,124,67,246,12,35,2,205,223,39,100,33,7,43,10,29,46,36,36,47,29,13,254,165,1,186,68,9,
    12,24,105,19,19,35,0,0,3,217,68,207,6,8,107,131,64,86,86,17,1,86,16,102,16,118,16,3,123,12,1,106,12,1,91,12,1,122,11,1,88,11,104,11,2,105,7,1,123,6,1,105,6,1,90,6,1,89,5,105,5,2,153,
    4,1,4,3,1,2,19,126,0,38,38,9,40,2,125,3,3,32,125,80,9,1,15,9,31,9,2,9,4,0,145,37,37,27,2,21,18,27,145,14,3,0,63,237,63,196,18,57,47,237,50,1,47,93,93,237,51,47,237,17,130,13,37,196,
    237,18,57,18,57,73,118,14,36,1,33,1,35,1,122,54,9,32,21,106,10,10,73,115,7,42,243,254,242,178,1,29,62,105,76,42,70,24,113,235,23,40,253,219,2,66,24,81,108,129,71,72,103,7,32,252,24,
    113,240,15,34,2,0,99,130,249,32,216,134,249,8,32,121,64,80,150,17,1,87,16,1,85,15,101,15,117,15,3,119,14,1,85,10,101,10,117,10,3,102,9,118,9,2,85,9,132,241,8,47,121,5,3,89,4,1,20,19,
    17,21,18,125,19,19,12,125,28,28,1,40,22,21,126,80,1,1,15,1,31,1,2,1,17,21,145,23,23,0,33,145,7,3,19,0,18,0,63,196,87,100,5,132,243,33,253,196,131,241,132,249,141,243,24,113,226,13,
    32,7,130,252,76,151,10,38,34,14,2,21,1,11,168,73,79,7,43,42,76,105,62,1,29,178,254,242,254,243,1,24,113,234,22,40,71,129,108,81,24,253,190,2,37,24,113,239,12,75,107,5,65,231,8,57,123,
    64,79,153,17,1,123,15,1,108,15,1,91,15,1,89,10,105,10,121,10,3,123,9,1,106,130,231,43,9,1,119,5,1,102,5,1,85,5,1,86,132,236,54,18,21,2,126,22,22,12,40,19,125,18,18,28,125,80,12,1,15,
    12,31,12,2,12,136,236,35,18,19,0,3,141,236,132,230,36,18,57,47,237,196,65,224,17,24,107,83,13,36,55,1,51,1,33,76,122,9,24,113,231,14,131,239,38,254,227,178,1,14,1,13,24,113,236,25,
    132,241,35,2,66,253,219,143,241,65,223,11,38,147,64,97,72,35,1,57,130,2,8,37,29,73,29,2,54,25,70,25,2,89,17,1,121,16,1,104,16,1,90,16,1,118,12,1,103,12,1,86,12,1,84,11,1,86,7,1,86,
    6,65,3,6,57,87,5,1,148,4,1,4,38,3,125,1,38,2,2,9,125,32,32,19,40,0,38,126,80,19,1,72,226,5,44,37,145,4,0,0,27,2,21,3,27,145,14,18,66,228,7,37,51,237,1,47,93,93,65,240,8,34,18,57,237,
    112,35,8,91,20,12,33,1,33,130,252,33,30,3,89,78,8,80,163,10,45,52,46,2,35,33,1,11,1,13,1,14,178,254,227,66,233,5,37,162,91,92,161,120,69,72,225,16,40,2,37,253,190,24,81,108,129,71,
    74,31,7,32,3,24,113,186,15,32,1,76,93,8,8,37,28,0,89,64,55,90,26,1,87,22,1,103,21,1,86,21,1,106,17,1,89,17,1,74,4,1,149,28,1,28,25,126,158,26,1,26,27,0,130,1,33,30,11,88,195,6,40,13,
    6,145,19,3,27,26,25,0,130,221,37,50,205,50,63,253,204,77,55,8,34,17,51,93,77,52,11,108,224,9,33,21,35,102,109,8,36,17,1,23,1,3,24,107,240,17,36,1,58,119,254,79,24,107,244,19,77,30,
    6,32,0,104,45,6,8,51,138,5,154,0,28,0,95,64,59,70,24,1,88,20,1,102,11,1,105,7,1,27,126,3,202,0,1,187,0,1,154,0,170,0,2,0,3,14,126,16,64,3,1,3,16,3,16,30,147,2,1,2,67,137,5,42,1,2,3,
    28,18,15,22,145,9,3,0,130,179,35,63,51,205,50,130,183,32,51,76,224,12,32,93,80,50,7,32,33,76,213,11,131,188,96,96,6,34,17,2,21,76,205,12,24,82,136,8,76,194,14,32,91,24,82,180,8,33,
    252,33,141,185,49,83,64,51,73,24,1,87,20,1,106,11,1,102,7,1,150,0,182,131,176,33,126,157,130,166,35,1,80,27,1,131,1,40,30,16,126,15,14,31,14,2,14,131,177,33,3,16,130,177,32,18,130,
    177,32,206,134,177,79,204,5,32,93,65,106,11,32,1,76,83,11,34,53,51,21,87,159,8,33,3,217,76,75,12,136,173,33,5,154,76,64,15,136,175,33,3,223,141,175,41,107,64,69,105,21,1,101,17,1,89,
    75,240,6,8,38,250,28,1,219,28,235,28,2,202,28,1,185,28,1,170,28,1,155,28,1,28,1,126,25,13,126,11,64,25,1,25,11,25,11,30,147,26,1,26,86,209,5,49,12,6,145,19,18,27,26,25,0,3,0,63,50,
    205,50,63,253,206,65,115,12,34,16,237,51,66,254,5,69,240,5,32,1,113,41,8,131,196,55,14,2,35,34,46,2,53,17,1,39,1,2,189,44,75,100,57,57,100,74,42,168,70,120,66,244,5,38,254,198,119,
    1,177,5,154,75,185,9,24,107,162,9,75,212,6,73,77,6,47,7,234,7,166,0,39,1,223,4,17,0,0,0,38,2,233,130,222,36,7,1,79,5,92,130,13,45,58,64,25,4,3,0,104,114,7,7,37,2,1,80,24,67,205,16,
    41,64,10,26,26,2,2,37,4,3,119,73,235,12,32,53,73,235,9,35,7,234,5,154,135,91,33,1,6,131,91,35,0,37,64,17,147,75,32,180,132,74,73,77,6,33,53,53,137,155,32,66,144,155,34,78,5,191,130,
    155,34,54,64,24,74,136,7,147,154,32,9,132,79,74,133,6,136,86,73,231,8,32,72,132,151,34,225,2,111,130,237,133,151,43,13,183,2,1,0,55,66,13,14,37,1,43,136,127,34,6,100,7,131,127,132,
    39,65,27,8,34,78,5,132,130,127,34,30,64,19,131,127,34,0,1,37,135,55,136,108,32,53,117,175,5,34,100,0,0,134,255,34,228,4,17,136,103,153,255,24,68,60,9,143,255,132,63,137,127,33,4,89,
    137,255,33,24,25,65,154,21,130,255,130,79,145,255,101,167,5,33,4,17,130,255,8,55,40,0,57,0,51,64,24,17,52,42,28,0,40,10,30,28,41,11,29,29,35,47,22,0,35,0,5,1,5,22,3,0,63,220,93,221,
    204,16,205,17,57,47,205,196,1,47,221,205,222,205,16,221,222,205,49,48,86,32,8,32,53,81,235,12,32,21,67,87,9,33,37,53,85,68,7,37,20,30,2,51,4,17,73,79,7,33,254,234,91,206,11,32,181,
    73,139,7,33,254,13,24,113,232,11,33,3,170,135,36,32,54,24,114,42,12,32,178,77,143,7,32,178,91,204,8,36,29,37,23,9,0,74,129,5,33,6,71,130,209,8,37,54,0,71,0,88,0,73,64,35,0,54,11,43,
    41,55,66,17,83,73,27,29,31,66,72,42,29,11,55,55,49,61,36,78,22,22,36,0,49,131,222,32,36,134,222,35,17,51,47,205,134,226,32,196,131,228,33,50,222,130,226,32,16,131,4,156,233,108,27,
    9,152,245,32,33,65,3,12,33,6,71,135,203,33,252,180,65,3,11,33,146,22,135,23,65,13,22,33,254,43,65,27,35,33,46,51,135,84,32,97,65,38,22,65,51,14,59,1,0,100,0,0,4,11,5,154,0,51,0,120,
    64,80,117,41,1,86,35,102,35,2,90,31,1,103,22,97,59,5,8,70,155,12,1,138,12,1,86,8,1,102,7,1,87,7,1,102,3,1,87,3,1,0,0,38,5,125,46,25,125,27,16,46,48,46,64,46,160,46,176,46,5,46,27,46,
    27,53,38,125,15,27,18,10,145,43,51,145,0,33,145,20,3,0,63,253,222,253,222,237,63,1,47,69,141,6,34,16,237,16,130,9,32,47,68,198,5,68,210,5,34,93,93,1,94,11,11,24,70,170,8,87,118,9,36,
    20,30,2,51,50,127,194,5,8,135,1,204,67,106,72,36,34,69,106,72,80,132,95,53,72,126,171,99,99,171,125,72,168,47,82,109,63,65,109,79,45,31,53,70,39,54,58,18,31,41,23,4,55,47,74,93,46,
    53,97,75,45,61,109,149,88,101,175,129,73,69,123,170,101,252,53,3,203,67,110,76,42,47,83,114,66,56,89,61,33,58,44,17,33,26,16,0,1,0,99,0,0,4,10,5,154,0,51,0,94,64,60,89,48,105,48,2,
    89,44,105,44,2,133,39,149,39,2,134,38,1,105,29,1,85,20,1,89,16,105,16,2,121,10,1,51,51,13,130,253,55,46,23,36,125,13,13,53,23,126,25,41,145,8,0,145,51,18,145,31,3,24,18,0,63,133,242,
    132,241,131,245,35,47,237,17,57,138,239,97,226,5,32,22,103,141,12,32,17,24,124,169,13,95,34,7,54,2,162,23,41,31,18,58,54,39,70,53,31,45,79,110,64,63,109,82,47,168,72,125,131,242,8,
    33,126,72,53,95,132,80,72,106,69,34,36,72,106,67,3,143,16,26,33,17,44,58,33,61,89,56,66,114,83,47,42,76,110,67,131,233,51,101,170,123,69,73,129,175,101,88,149,109,61,45,75,97,53,46,
    93,74,47,72,29,5,65,241,6,8,41,118,64,79,102,49,1,86,48,1,85,44,101,44,2,155,39,1,124,39,140,39,2,156,38,1,136,38,1,122,38,1,101,29,1,90,20,1,86,16,102,16,2,130,242,56,46,125,5,25,
    126,23,16,5,48,5,64,5,160,5,176,5,5,5,23,5,23,53,13,125,36,136,254,34,18,24,3,140,254,85,16,5,65,240,17,38,1,50,62,2,53,52,38,100,175,12,119,89,10,104,185,11,33,1,204,65,3,11,33,109,
    65,65,3,24,33,2,11,65,3,15,37,109,68,3,203,252,53,65,3,22,65,239,9,8,35,114,64,74,105,35,1,90,35,1,85,31,1,106,22,1,149,13,1,134,13,1,116,13,1,132,12,148,12,2,115,12,1,106,7,1,89,130,
    2,59,2,105,2,2,0,0,38,46,125,5,15,125,38,15,5,47,5,2,5,38,5,38,53,28,126,25,27,3,66,239,8,32,18,66,239,20,65,255,12,32,93,130,0,24,103,180,8,24,119,173,26,40,34,6,21,20,30,2,51,2,162,
    66,239,24,33,64,110,66,239,11,33,1,99,66,239,19,37,3,203,252,53,68,109,66,239,16,8,55,3,0,100,0,0,5,254,4,245,0,18,0,35,0,39,0,88,64,54,71,23,1,105,15,1,90,14,106,14,2,84,10,100,10,
    2,102,9,1,102,5,1,102,4,1,85,4,1,19,126,17,39,39,7,125,30,17,130,1,44,41,36,1,19,18,145,1,39,145,37,25,145,12,130,239,39,237,47,253,222,253,196,1,47,95,120,7,32,51,95,124,12,32,19,
    24,102,86,28,35,1,53,33,21,24,102,90,27,35,252,33,4,242,24,95,88,10,35,70,120,161,92,85,225,15,37,1,128,168,168,0,0,143,207,42,98,64,60,73,31,1,71,21,1,105,14,130,210,59,1,105,13,1,
    105,9,1,106,8,1,88,8,1,101,4,1,86,4,1,102,3,1,1,126,35,39,17,35,130,1,42,11,41,36,36,24,125,11,0,35,145,16,130,213,34,29,145,6,138,213,41,237,50,47,17,18,57,57,47,47,16,90,17,15,86,
    155,28,32,17,131,217,24,102,48,29,33,253,219,131,219,24,102,52,28,136,219,33,255,255,65,171,10,8,40,94,64,57,71,31,1,102,14,1,85,14,1,102,13,1,102,9,1,102,8,1,84,8,1,91,4,107,4,2,105,
    3,1,34,126,0,39,39,11,125,24,0,130,1,32,18,130,218,32,18,130,209,130,215,24,102,23,8,38,47,253,222,237,1,47,51,135,215,32,237,65,176,13,32,93,24,102,30,29,131,215,24,102,34,29,36,253,
    51,4,242,167,86,93,13,77,118,15,34,166,168,168,65,177,8,32,246,66,129,6,8,41,98,64,60,71,33,1,72,23,1,103,15,1,101,14,1,87,14,1,106,10,1,89,10,1,105,9,1,105,5,1,90,4,106,4,2,17,126,
    19,39,39,0,19,130,1,32,7,130,214,37,30,125,7,25,145,12,130,216,24,101,184,8,133,216,65,177,8,32,17,142,216,24,101,190,30,131,217,24,95,77,12,34,161,120,70,79,20,12,35,254,237,4,242,
    24,95,88,9,35,70,120,162,91,24,130,51,16,131,216,72,133,5,35,3,49,5,107,134,217,8,38,40,64,17,38,38,7,30,19,37,37,0,17,19,18,2,19,25,12,36,38,0,47,205,222,221,222,205,196,1,47,221,
    206,50,47,16,222,205,51,47,24,91,240,12,34,50,30,2,24,87,148,7,113,204,7,35,1,21,33,53,24,91,252,11,36,46,80,60,35,124,72,18,11,35,1,240,253,175,24,92,7,10,35,35,60,80,46,24,123,85,
    9,39,29,37,23,9,2,34,124,124,69,253,5,50,5,254,5,38,0,28,0,94,64,60,90,22,106,22,2,106,21,1,89,130,2,8,44,17,105,17,2,89,16,1,71,3,1,148,27,1,27,26,188,25,1,170,25,1,75,25,155,25,2,
    58,25,1,25,0,0,19,30,13,6,125,19,26,27,28,1,145,25,12,79,178,5,40,47,237,51,205,50,1,47,253,204,97,103,6,34,93,93,205,92,10,9,32,1,98,151,8,24,98,108,11,36,1,55,1,5,254,76,140,26,24,
    98,117,18,36,1,58,119,254,79,104,167,12,40,244,0,0,1,7,1,78,3,164,88,107,5,40,1,0,29,39,26,27,37,1,44,130,214,115,209,6,40,1,0,100,254,79,5,254,3,117,130,231,49,96,64,62,101,22,1,84,
    22,1,86,21,102,21,2,101,17,1,86,130,2,51,16,1,73,3,1,19,125,12,6,6,30,219,27,1,170,27,186,27,2,155,131,246,37,68,25,148,25,2,53,131,240,133,233,39,18,11,145,13,0,47,237,63,133,233,
    32,51,132,225,36,93,93,18,57,47,24,98,152,10,35,55,33,50,62,123,204,5,24,98,152,11,130,161,32,100,79,167,26,67,21,8,77,169,13,131,231,32,254,130,185,37,5,29,2,38,2,246,134,231,38,211,
    253,219,0,17,64,9,131,231,33,14,14,130,231,24,77,85,10,139,229,8,68,71,25,1,90,12,1,105,11,1,88,11,1,89,7,105,7,2,105,6,1,91,6,1,180,1,212,1,2,165,1,1,148,1,1,1,2,59,3,75,3,155,3,3,
    3,28,28,9,30,15,22,125,9,17,145,15,2,1,28,145,0,4,18,0,63,51,237,206,50,47,24,100,53,8,35,51,93,205,50,119,7,10,35,93,33,1,39,93,212,9,121,118,10,34,33,5,254,79,36,26,80,5,21,140,229,
    32,248,24,98,141,15,33,29,39,143,229,8,59,2,205,3,49,5,67,0,28,0,58,64,33,243,13,1,13,14,155,15,251,15,2,138,15,1,15,12,27,21,5,26,28,10,14,0,13,16,13,32,13,3,13,12,15,10,0,47,221,
    50,205,113,50,16,222,205,1,47,205,204,220,130,196,130,200,33,49,48,73,121,5,39,30,2,51,33,21,7,39,55,76,178,8,50,51,21,1,65,19,35,27,16,18,29,34,16,1,240,187,88,99,254,192,75,67,7,
    34,49,4,199,24,95,77,7,39,124,188,88,100,35,60,80,46,131,26,67,69,6,8,70,4,2,5,154,0,16,0,82,64,45,6,8,7,8,126,5,6,20,5,5,6,88,7,1,7,7,13,12,10,126,2,1,16,16,18,31,5,1,5,13,16,145,
    2,10,8,145,5,3,3,6,15,18,7,6,3,0,63,51,63,18,57,47,51,237,221,196,253,196,1,47,93,130,11,37,205,196,253,205,196,51,24,73,248,10,33,53,33,130,1,8,43,1,23,1,33,17,51,21,35,17,35,17,1,
    99,1,122,253,135,2,71,134,254,100,1,240,125,125,168,1,9,164,124,10,3,103,95,253,146,254,224,164,254,247,1,9,67,215,5,134,145,59,80,64,43,10,8,9,8,126,11,10,20,11,10,11,11,3,18,16,0,
    14,126,3,87,9,1,9,9,6,130,134,45,0,145,14,6,11,8,145,13,13,2,9,10,3,2,24,74,129,9,133,143,132,136,36,16,253,196,205,17,130,152,39,135,43,135,125,196,49,48,1,130,130,41,35,53,51,17,
    33,1,55,1,21,33,130,1,49,1,137,168,125,125,1,240,254,100,134,2,71,253,135,1,122,1,9,131,132,42,164,1,32,2,110,95,252,153,10,124,164,67,173,5,39,4,2,7,66,2,38,2,252,65,223,6,32,8,67,
    173,7,37,17,27,9,10,37,1,24,93,229,15,65,79,9,136,189,34,11,10,88,134,181,32,126,130,195,38,14,18,31,11,1,11,3,136,191,36,1,9,10,18,1,24,73,12,9,65,79,9,33,196,205,65,79,14,37,17,51,
    17,51,21,35,130,191,35,7,1,53,33,130,1,33,2,221,130,191,44,254,16,1,156,134,253,185,2,121,254,134,4,145,131,193,39,164,254,224,253,146,95,3,103,143,191,32,254,107,173,13,37,1,0,17,
    27,1,2,152,191,34,80,64,43,66,15,8,42,6,5,5,10,18,2,1,16,126,10,87,66,23,6,66,13,11,35,3,7,6,18,66,13,14,65,125,6,33,205,196,65,125,11,65,114,5,37,39,1,33,17,35,53,130,200,61,17,3,
    3,254,134,2,121,253,185,134,1,156,254,16,125,125,168,4,145,164,124,10,252,153,95,2,110,1,32,164,131,200,65,125,11,33,3,0,133,189,33,0,197,65,125,9,33,14,15,143,189,8,58,2,205,2,130,
    5,154,0,16,0,75,64,15,253,13,1,13,32,20,23,72,13,15,10,1,10,7,11,184,255,224,64,23,20,24,72,11,12,12,16,15,1,5,4,7,12,11,13,10,8,7,15,1,4,2,3,0,63,221,196,130,1,62,50,221,205,50,1,
    47,196,205,221,196,205,51,47,51,43,16,204,93,50,43,93,49,48,19,51,53,51,21,51,21,35,130,191,8,115,1,39,19,35,53,35,100,100,124,200,200,1,62,254,221,107,181,225,100,5,98,56,56,124,55,
    5,254,35,62,1,40,179,0,2,0,100,255,232,4,18,5,236,0,20,0,41,0,96,64,60,73,28,1,102,17,1,85,17,1,86,12,1,117,11,1,99,11,1,84,11,1,14,125,16,31,32,31,2,31,31,3,43,7,5,5,0,2,2,41,126,
    15,3,31,3,2,3,26,145,0,19,19,36,145,7,9,16,5,0,3,18,0,63,63,63,51,237,130,2,38,1,47,93,237,50,47,51,130,2,34,17,18,57,130,11,78,118,8,39,37,35,21,35,17,51,17,51,119,239,10,32,3,122,
    149,14,8,68,21,1,12,4,164,164,4,121,233,108,158,103,51,56,115,176,120,204,107,43,78,107,64,88,121,75,34,34,71,111,77,72,118,82,45,148,148,5,236,253,96,204,81,138,184,103,114,206,155,
    91,1,186,63,111,82,48,72,120,156,83,74,132,100,59,56,95,127,71,0,67,151,5,35,2,226,5,154,130,215,42,45,64,22,11,11,0,22,3,19,126,15,130,23,52,20,18,19,16,6,3,6,1,1,6,3,0,63,51,47,17,
    51,16,205,50,63,94,34,8,33,49,48,130,157,32,21,120,143,11,55,38,39,17,100,168,38,99,57,57,100,75,44,44,75,100,57,57,99,38,5,154,80,37,43,132,11,38,101,75,43,42,38,252,62,141,111,53,
    37,64,19,3,1,126,19,19,22,15,11,31,11,2,11,20,3,19,16,3,6,18,130,108,133,104,35,18,57,47,237,67,220,5,34,53,6,6,116,233,9,36,22,23,17,2,226,132,104,130,84,34,43,75,101,133,104,33,250,
    102,130,106,132,13,130,126,35,42,38,3,194,140,217,54,43,64,21,9,9,19,22,17,1,126,15,19,1,19,1,4,14,19,19,17,14,18,0,130,216,35,63,51,51,47,130,216,138,215,33,1,17,141,213,36,21,35,
    17,1,12,141,215,38,168,5,154,252,62,38,42,135,217,36,43,37,80,5,154,140,111,53,51,64,29,19,126,17,32,1,48,1,64,1,3,1,1,22,15,9,31,9,2,9,134,119,33,3,0,130,227,137,119,130,230,38,93,
    196,237,49,48,33,17,141,229,36,53,51,17,2,58,131,119,137,230,36,168,3,194,38,42,135,228,130,117,33,250,102,133,117,33,3,217,130,127,61,9,0,49,64,26,1,1,5,5,8,11,6,3,126,15,8,31,8,2,
    8,3,145,5,5,7,2,145,9,3,7,24,140,134,17,73,210,6,40,17,33,21,33,17,35,17,3,217,109,191,5,130,218,37,164,254,22,164,253,152,132,209,89,101,7,45,9,0,47,64,25,8,126,6,3,3,1,11,4,4,87,
    133,5,37,6,145,4,4,7,3,24,123,54,7,108,14,5,33,51,47,91,250,5,38,49,48,51,53,33,17,33,130,3,35,51,17,99,2,108,165,5,37,164,1,234,164,2,104,130,178,130,179,36,98,0,0,3,215,132,179,131,
    87,54,8,5,5,0,11,3,6,126,15,0,31,0,2,0,4,145,6,6,2,3,8,145,0,130,178,33,237,63,133,87,89,111,5,131,177,34,51,17,51,132,177,35,33,21,98,168,133,88,35,5,154,253,152,131,178,32,0,81,37,
    5,33,3,216,135,87,40,3,0,126,6,6,5,11,8,8,96,245,5,131,175,37,2,18,7,145,0,3,24,139,99,9,133,175,66,101,6,132,175,36,33,53,3,216,168,65,10,5,37,5,154,250,102,2,104,131,182,65,101,5,
    33,4,61,130,89,8,42,29,0,63,64,39,138,7,154,7,2,40,7,1,133,3,149,3,2,134,2,150,2,2,29,126,27,27,31,16,126,10,12,126,15,5,145,22,28,11,145,16,3,13,130,191,42,63,237,196,222,237,1,47,
    253,222,237,18,130,200,67,221,5,84,149,9,35,35,17,35,17,24,71,186,11,33,4,61,104,95,7,35,100,168,1,178,104,98,8,32,4,24,164,166,8,36,195,251,10,5,154,103,154,11,134,243,33,4,60,132,
    153,41,81,64,52,149,8,1,134,8,1,150,130,154,55,7,1,39,7,1,155,3,1,138,3,1,168,2,184,2,2,137,2,153,2,2,15,126,12,130,164,36,10,31,27,126,29,131,166,39,12,145,16,18,13,3,0,63,134,166,
    132,164,107,125,12,24,162,95,8,35,21,51,17,51,130,171,85,45,7,34,17,35,99,137,170,33,254,78,136,170,105,78,9,36,195,4,246,250,102,106,65,11,65,245,5,33,4,59,134,169,42,51,182,27,1,
    167,27,1,149,27,1,134,130,5,8,44,26,1,132,26,1,154,22,1,139,22,1,40,22,1,153,21,1,29,126,1,1,31,13,126,19,17,126,14,16,3,24,145,7,17,145,1,13,18,0,63,196,237,222,237,63,65,79,14,83,
    164,5,32,33,83,141,9,130,167,130,173,90,166,8,33,4,59,104,159,8,35,254,78,168,100,104,162,7,139,155,37,5,154,251,10,195,74,107,244,6,65,83,13,8,33,63,64,39,27,8,17,20,72,138,26,154,
    26,2,133,22,149,22,2,39,22,1,14,126,17,13,126,18,18,31,1,126,29,16,18,130,157,32,18,130,157,32,3,136,157,65,70,11,34,43,19,51,89,10,8,130,151,34,35,17,35,92,17,8,32,99,136,150,33,1,
    178,137,150,65,223,13,35,250,102,4,246,137,152,85,11,5,55,66,5,154,0,9,0,103,64,67,7,23,6,39,6,55,6,3,6,24,3,40,3,56,3,130,0,34,8,5,125,104,29,5,8,38,4,11,2,8,126,31,0,1,0,9,18,146,
    3,1,116,3,132,3,2,98,3,1,3,6,157,8,1,139,8,1,122,8,1,107,8,1,8,4,2,131,179,33,51,93,130,0,33,205,50,130,4,39,63,1,47,93,237,50,18,57,130,5,63,18,57,25,47,93,51,93,51,49,48,51,17,51,
    1,1,51,1,35,1,17,100,168,1,53,1,49,208,254,9,20,254,213,130,165,41,70,1,186,253,51,1,164,251,143,0,67,29,6,133,151,43,105,64,66,55,7,1,7,40,6,1,6,39,130,113,61,3,4,2,0,126,48,8,64,
    8,80,8,3,8,8,11,4,125,5,9,3,159,3,1,142,3,1,124,3,1,109,130,32,55,6,181,8,1,3,147,8,1,2,128,8,1,100,8,116,8,2,8,2,4,18,0,63,196,130,144,34,95,93,95,75,225,5,32,93,85,158,5,130,151,
    32,51,135,152,38,93,49,48,1,17,35,1,130,151,61,51,1,17,4,66,168,254,203,254,207,208,1,247,20,1,43,5,154,250,102,1,186,254,70,2,205,254,92,4,113,140,155,8,50,104,64,68,23,3,39,3,55,
    3,3,3,2,6,4,125,64,6,80,6,2,24,6,40,6,56,6,3,64,5,80,5,2,5,6,6,5,11,7,1,126,8,157,6,1,139,6,1,124,6,1,109,130,198,45,3,5,147,1,1,100,1,116,1,132,1,3,1,7,70,52,6,35,93,93,196,205,130,
    162,51,93,93,1,47,237,50,18,57,57,25,47,24,47,93,93,93,237,17,51,51,132,154,33,1,51,130,154,55,1,35,17,1,12,1,43,20,1,247,208,254,207,254,203,168,5,154,251,143,1,164,253,51,131,158,
    33,5,154,65,55,13,43,100,64,63,3,54,2,1,2,6,8,126,7,132,152,40,40,6,1,3,52,1,68,1,84,130,117,60,6,6,1,11,5,125,4,105,1,121,1,137,1,169,1,4,1,7,148,6,1,131,6,1,2,116,6,1,99,132,162,
    70,83,5,131,146,32,95,130,154,32,50,131,150,135,149,130,16,32,51,130,151,37,93,51,49,48,33,17,130,149,59,51,1,1,51,17,3,154,254,213,20,254,9,208,1,49,1,53,168,4,113,254,92,2,205,254,
    70,1,186,69,157,5,8,38,95,0,0,5,97,5,178,0,30,0,25,64,12,26,13,13,0,28,3,5,238,21,4,0,18,0,63,63,237,63,18,57,47,57,48,49,33,10,2,85,31,6,34,22,23,35,88,101,6,8,62,22,18,18,19,51,17,
    51,17,4,156,88,163,166,179,104,57,80,51,23,33,29,175,61,41,90,143,102,140,209,168,141,73,7,168,1,40,1,222,1,81,182,41,71,97,56,71,154,76,135,152,87,160,123,74,140,254,238,254,105,254,
    246,4,39,130,124,34,1,0,200,130,123,32,202,132,123,42,23,64,11,25,238,9,4,3,16,16,2,133,221,130,119,35,63,237,48,49,131,97,34,51,26,2,73,25,6,24,87,59,10,8,65,2,2,3,200,168,7,73,141,
    168,209,140,102,143,90,41,61,175,29,33,23,51,80,57,104,179,166,163,88,5,154,251,217,1,10,1,151,1,18,140,74,123,160,87,152,135,76,154,71,56,97,71,41,182,254,175,254,34,254,216,0,0,1,
    0,95,255,232,130,245,32,154,133,245,44,25,8,8,22,24,18,22,3,16,238,0,19,0,130,244,134,245,40,5,34,46,2,53,52,55,51,6,84,252,5,33,50,54,132,241,39,35,17,35,10,2,6,1,215,144,117,32,197,
    133,142,32,24,141,114,48,1,81,1,222,1,40,250,102,4,39,254,246,254,106,254,237,140,131,125,32,200,130,125,33,202,5,131,125,44,27,64,13,5,9,23,23,7,9,3,7,18,15,138,126,38,18,57,48,49,
    5,34,38,130,229,130,111,35,17,51,26,2,87,163,6,41,38,39,51,22,21,20,14,2,4,82,65,100,6,32,197,65,125,15,45,24,140,1,19,1,150,1,10,251,217,5,154,254,216,130,239,32,175,65,129,13,40,
    0,0,2,0,40,255,246,5,194,130,127,47,7,0,10,0,26,64,12,3,238,8,9,9,1,5,5,1,65,122,5,38,51,47,17,57,47,57,237,130,128,8,38,1,51,19,33,19,51,1,3,19,33,2,235,253,61,178,228,2,110,228,178,
    253,61,10,233,254,46,10,5,164,254,44,1,212,250,92,1,79,1,223,131,81,38,0,0,5,194,5,164,0,131,81,51,25,64,12,5,238,10,8,8,2,7,18,4,18,2,3,0,63,63,63,18,133,80,60,51,1,51,1,35,3,33,3,
    1,33,3,40,2,195,20,2,195,178,227,253,145,228,1,50,1,210,233,5,164,130,75,38,212,254,44,2,118,1,224,130,79,36,155,255,246,6,53,134,79,8,63,23,64,12,2,8,9,10,3,5,4,5,3,1,0,18,0,63,50,
    63,51,23,57,48,49,23,53,37,17,37,53,1,21,5,37,37,155,1,209,254,47,5,154,252,223,1,243,254,13,10,170,238,2,126,238,170,253,51,20,233,243,242,0,3,0,139,75,40,14,0,33,64,16,14,3,10,5,
    130,81,34,3,12,12,138,81,39,57,47,23,57,18,57,57,205,140,87,35,1,17,51,17,140,91,34,253,135,160,140,94,42,254,62,1,160,254,96,0,255,255,0,120,133,177,42,16,34,3,26,0,0,16,2,1,123,20,
    65,105,9,65,23,5,130,199,41,6,9,8,10,5,5,4,3,3,7,138,199,33,5,1,131,196,8,34,17,5,1,5,17,5,194,250,102,5,154,254,47,1,209,251,148,1,243,10,2,205,20,2,205,170,238,253,130,238,2,45,243,
    1,229,65,185,14,41,23,64,11,5,3,9,238,8,3,3,65,184,6,48,57,47,57,237,63,48,49,5,1,51,23,33,55,51,1,3,1,65,182,5,54,81,3,148,81,178,253,61,10,1,123,253,11,10,5,164,164,164,250,92,1,
    79,3,13,65,181,17,33,7,18,65,183,5,65,181,6,32,18,134,79,42,51,1,51,1,35,39,33,7,19,33,1,65,181,6,41,81,252,108,81,161,2,245,254,133,5,130,75,35,164,164,1,72,132,77,65,179,37,34,55,
    17,39,130,238,60,9,2,155,162,162,5,154,251,176,3,34,252,222,10,170,83,3,180,83,170,253,51,20,254,132,1,134,1,134,65,179,52,135,87,35,19,17,51,17,138,91,33,108,160,143,93,37,253,170,
    1,162,254,94,142,99,37,30,0,33,64,16,11,66,23,6,33,21,21,66,23,22,136,99,34,34,46,2,72,114,8,33,20,14,139,203,37,233,23,41,30,18,18,24,186,251,9,143,125,33,254,10,138,28,32,23,131,
    44,65,213,8,66,37,34,36,7,17,23,1,1,66,37,6,37,162,162,251,148,3,34,66,35,6,44,83,252,76,83,2,45,254,122,3,12,0,0,1,67,223,8,42,9,0,21,64,10,7,3,6,3,4,3,69,86,6,34,63,57,57,66,33,5,
    58,1,17,51,17,1,51,1,2,235,253,61,178,1,199,168,1,199,178,253,61,10,5,164,252,52,3,204,131,3,33,250,92,132,71,32,0,67,213,5,46,9,0,23,64,11,9,18,5,8,1,7,18,4,18,1,67,210,6,131,73,32,
    51,130,67,36,35,1,17,35,17,66,26,7,40,254,57,168,254,57,5,164,250,92,133,71,33,252,52,131,71,66,25,7,44,9,0,26,64,12,9,8,2,238,4,4,6,7,67,203,8,53,57,47,237,57,57,48,49,23,53,1,33,
    53,33,1,53,1,21,155,3,197,252,59,131,3,44,5,154,10,170,1,219,164,1,219,170,253,51,20,130,215,135,71,49,17,0,36,64,17,16,17,8,4,3,6,238,12,11,8,8,14,15,138,76,37,204,51,253,50,205,18,
    135,81,40,21,35,53,35,53,51,53,51,21,135,89,34,253,123,160,130,0,35,2,133,252,59,133,93,36,166,166,164,166,166,144,97,32,23,131,97,44,22,23,11,6,3,10,238,18,15,11,11,20,21,142,97,32,
    204,136,97,37,6,6,35,34,38,39,130,100,37,54,54,51,50,22,23,135,103,41,254,41,21,77,48,48,77,21,202,202,133,7,33,1,215,135,113,36,39,48,48,39,164,131,4,136,117,69,143,5,48,164,0,9,0,
    26,64,12,2,1,7,238,6,6,4,3,3,9,68,35,6,65,31,6,41,5,1,53,1,21,1,33,21,33,1,68,37,5,65,34,5,33,3,197,66,1,6,38,254,37,164,254,37,0,3,65,251,8,51,11,0,14,0,17,0,32,64,16,9,3,14,16,238,
    7,15,12,3,3,5,66,5,9,37,47,57,57,51,237,50,66,10,5,34,19,51,17,130,1,8,33,19,51,1,19,19,35,3,17,35,2,235,253,61,178,210,245,168,245,210,178,253,61,74,172,172,168,173,10,5,164,254,64,
    1,192,131,3,42,250,92,1,214,1,113,254,143,1,113,0,131,107,66,31,6,133,107,46,34,64,17,11,18,6,9,238,15,14,17,12,12,1,8,66,41,10,136,109,39,51,1,51,1,35,3,35,17,130,1,37,3,1,51,17,19,
    51,70,7,7,132,111,39,1,26,173,168,172,172,5,164,130,100,132,108,35,254,64,2,93,134,108,69,197,9,133,107,51,43,64,21,10,11,17,238,12,12,3,238,7,14,9,15,2,5,5,8,9,65,161,10,38,57,57,
    18,57,57,237,51,66,88,7,48,37,39,33,53,33,39,37,53,1,21,37,33,37,17,37,33,155,130,196,46,254,43,1,213,1,254,44,5,154,252,222,1,84,254,172,131,3,47,10,170,232,243,164,244,231,170,253,
    51,20,89,171,254,12,171,68,35,10,60,19,0,22,0,25,0,50,64,24,18,19,9,25,6,4,7,238,20,14,11,15,22,17,23,2,9,9,16,17,143,120,38,206,51,51,253,50,205,51,66,33,6,34,37,39,35,66,132,8,32,
    51,142,132,33,149,160,130,0,32,149,146,134,36,166,166,164,166,166,139,138,130,139,32,132,70,97,5,62,0,26,0,29,0,32,0,51,64,25,25,26,32,238,27,27,21,7,3,238,17,2,30,0,29,22,24,21,21,
    23,24,138,140,130,128,130,2,35,204,253,204,17,65,9,10,32,35,80,120,13,143,145,47,179,21,77,48,35,61,45,26,26,45,61,35,48,77,21,179,146,155,43,39,48,27,46,61,35,35,61,46,27,48,39,65,
    45,13,66,199,7,65,163,8,50,2,1,17,238,13,13,9,238,5,14,3,16,10,6,6,4,3,3,11,66,212,8,65,163,12,49,5,1,53,1,21,5,7,33,21,33,7,5,1,33,53,1,5,53,66,228,5,43,254,44,1,1,213,254,43,1,1,
    214,252,52,65,166,5,66,238,6,8,33,231,244,164,243,232,2,124,171,254,183,171,171,0,2,0,117,255,232,6,2,5,154,0,20,0,40,0,16,183,32,238,10,3,21,73,77,6,8,108,237,48,49,5,34,46,3,2,53,
    52,54,55,33,22,22,21,20,2,14,3,39,50,62,4,53,52,38,39,33,6,21,20,30,4,3,59,121,199,158,117,77,38,37,28,5,11,28,37,38,77,118,158,199,120,92,151,120,88,58,29,15,16,252,9,31,29,58,89,
    120,151,24,72,130,183,223,1,1,139,126,225,103,103,225,126,139,254,255,223,183,130,72,166,63,112,156,187,210,111,80,140,69,131,158,111,210,187,156,112,63,131,137,37,0,0,5,237,5,178,
    134,137,37,31,238,10,4,40,238,80,221,5,130,137,63,51,38,38,53,52,18,62,3,51,50,30,3,18,21,20,6,7,39,54,54,53,52,46,4,35,34,14,4,21,20,23,182,130,126,40,76,116,156,196,118,118,196,155,
    116,131,144,47,138,16,15,28,58,87,117,148,89,89,148,118,87,58,29,31,131,123,33,1,1,131,123,131,143,33,254,255,131,143,35,164,69,140,80,133,125,133,142,33,158,131,130,133,36,200,255,
    232,5,249,132,133,39,41,0,16,183,31,238,11,4,65,15,13,34,2,39,17,131,133,126,124,7,34,39,50,36,131,136,34,38,38,36,130,137,8,89,2,7,17,30,3,2,127,69,116,104,99,51,51,99,104,116,69,
    213,1,75,228,118,118,228,254,181,213,183,1,13,176,86,86,176,254,243,183,46,72,62,58,33,33,58,62,72,24,10,17,24,14,5,72,14,24,17,10,104,193,254,238,170,170,254,238,193,104,164,83,152,
    212,130,130,213,151,83,4,7,12,8,251,188,8,12,7,4,0,255,255,0,134,149,44,16,38,3,50,0,0,16,7,4,234,2,231,0,146,23,41,1,123,2,95,0,0,0,2,0,95,130,197,32,144,136,197,34,32,238,10,141,
    197,33,36,38,24,207,187,7,60,30,2,23,17,14,3,39,50,62,2,55,17,46,3,35,34,4,6,6,21,20,22,22,4,3,217,213,254,181,131,187,34,1,75,213,137,207,33,47,71,134,187,35,46,183,254,243,131,207,
    44,1,13,24,104,193,1,18,170,170,1,18,193,104,131,207,33,250,184,131,207,32,164,131,189,33,4,68,131,189,39,83,151,213,130,130,212,152,83,131,149,66,107,7,50,21,0,43,0,31,64,15,33,238,
    11,11,0,32,12,12,34,10,3,22,66,115,6,38,51,51,47,51,18,57,47,66,122,11,33,5,37,66,123,14,33,5,37,131,163,66,125,11,57,2,133,2,133,28,38,38,78,117,158,199,120,92,151,119,89,59,28,12,
    16,254,1,254,1,16,11,66,130,15,33,170,170,66,132,12,47,153,180,200,102,79,133,69,140,140,67,136,78,102,200,180,153,66,135,7,36,6,2,5,178,0,131,165,47,30,64,15,22,238,21,21,10,23,20,
    18,33,238,10,4,43,67,131,5,33,237,63,134,164,66,149,16,34,37,53,5,66,151,11,38,22,23,182,28,37,38,77,130,151,60,121,121,199,158,117,78,38,38,28,253,123,1,255,16,12,28,59,89,119,152,
    91,92,151,120,89,58,29,11,16,66,156,19,37,170,171,140,69,133,79,133,148,33,63,112,131,168,42,78,136,67,0,2,0,100,255,232,5,149,132,157,43,39,0,23,64,11,6,36,0,32,238,12,4,65,63,7,44,
    237,18,57,57,48,49,5,34,46,2,39,19,3,66,167,23,46,6,7,19,3,22,22,2,27,69,116,104,99,51,164,164,66,169,24,47,63,117,54,132,132,57,114,24,10,17,24,14,2,164,2,164,66,169,22,42,14,17,253,
    222,253,222,17,14,0,255,255,135,157,35,16,38,3,56,66,167,6,33,151,0,146,23,35,1,123,2,44,66,167,12,33,21,0,132,205,37,26,16,0,30,238,10,144,205,66,174,12,43,3,19,14,3,39,50,54,55,3,
    19,38,38,66,175,26,134,215,39,63,114,57,132,132,54,117,63,66,175,23,35,253,92,253,92,131,215,40,164,14,17,2,34,2,34,17,14,66,175,19,48,32,0,51,0,26,64,13,20,3,33,238,16,16,0,12,3,42,
    66,173,6,36,18,57,47,237,63,69,37,8,63,62,2,55,51,6,6,7,33,38,38,39,51,30,3,21,20,2,14,3,1,6,21,20,30,4,51,50,62,4,53,52,39,69,45,7,8,38,10,17,24,14,169,10,22,9,4,11,9,22,10,168,14,
    24,18,10,38,78,117,158,199,253,112,3,29,58,89,120,151,92,92,151,119,89,58,29,3,69,56,7,47,67,120,111,105,51,41,110,63,63,110,41,51,105,111,120,67,69,66,6,35,4,56,39,36,68,188,11,34,
    36,39,0,69,67,5,39,6,2,5,178,0,28,0,47,131,177,45,28,18,24,238,29,29,10,21,18,38,238,10,4,0,80,149,5,131,177,66,183,16,40,35,54,54,55,33,22,22,23,1,69,84,15,66,187,12,32,168,130,181,
    33,251,245,130,181,33,3,244,131,171,32,119,131,171,32,120,131,171,35,102,219,133,139,69,93,12,34,133,219,102,133,178,33,1,122,144,166,38,2,0,120,255,232,5,219,132,165,55,45,0,43,64,
    22,45,10,19,13,238,14,14,42,238,19,4,29,9,0,6,238,5,5,32,65,96,5,39,51,47,237,17,57,57,63,237,133,7,32,48,66,215,5,32,53,130,166,36,17,6,6,7,53,66,222,11,35,37,22,22,51,69,137,10,8,
    37,6,7,2,97,76,134,119,109,51,51,94,50,50,95,50,51,109,119,134,76,213,1,75,228,118,119,228,254,181,254,174,27,57,31,187,1,17,179,69,143,5,53,34,63,29,24,12,20,27,15,165,14,25,9,4,76,
    10,24,14,165,15,27,20,12,69,145,9,47,168,2,2,83,152,213,129,129,213,152,83,2,2,0,255,255,135,189,35,16,38,3,62,66,227,6,33,236,0,146,23,35,1,123,2,99,66,227,8,32,194,137,237,49,29,
    19,10,16,238,15,15,32,238,10,4,20,45,0,23,238,24,24,66,78,6,145,237,66,247,12,41,21,38,38,39,17,54,54,55,21,14,66,249,13,49,51,50,54,55,3,217,212,254,181,228,119,118,228,1,75,213,77,
    133,130,247,130,244,34,50,94,51,130,247,36,133,49,29,63,34,66,252,5,38,179,1,17,187,31,57,27,69,176,10,133,246,37,24,10,251,180,9,25,133,246,33,5,34,140,237,8,49,0,1,0,60,1,102,2,214,
    4,51,0,7,0,13,180,5,2,218,3,0,0,47,204,237,50,48,49,19,53,33,17,51,17,33,21,60,1,12,130,1,12,1,102,122,2,83,253,173,122,0,130,49,57,117,255,232,6,203,5,154,0,65,0,31,64,16,48,3,64,
    61,29,3,11,3,36,238,61,61,23,65,213,8,51,63,63,18,57,63,48,49,5,34,38,38,2,53,52,62,2,55,51,14,3,67,41,6,33,2,53,114,3,8,36,4,53,52,46,2,67,75,5,8,48,14,4,35,34,38,39,6,2,91,132,185,
    116,53,8,16,25,16,171,16,24,16,8,15,31,51,73,95,61,70,89,51,19,168,17,50,89,73,60,95,73,51,32,15,9,16,24,15,171,17,130,29,8,68,25,51,78,108,139,85,122,156,47,94,24,138,233,1,49,168,
    77,168,163,148,58,58,151,163,167,75,85,167,151,128,94,53,69,121,168,99,3,67,252,189,99,168,121,69,53,95,128,151,166,85,75,166,164,151,58,60,151,163,165,75,112,213,188,156,112,63,102,
    100,202,131,207,37,0,0,6,203,5,178,130,207,47,30,64,16,65,18,12,10,47,18,28,18,40,238,15,4,53,67,84,6,32,237,134,206,42,51,46,3,53,52,18,54,54,51,50,23,131,4,40,30,4,21,20,14,2,7,35,
    62,130,21,32,46,130,191,33,14,2,117,200,9,130,26,35,30,2,23,182,130,200,58,8,53,116,185,132,231,94,47,156,122,85,139,108,78,51,25,8,16,24,17,171,15,24,16,9,15,32,130,220,42,60,73,89,
    50,17,168,19,51,89,70,61,130,220,33,31,15,130,29,35,16,58,148,163,130,203,59,1,49,233,138,202,100,102,63,112,156,188,212,113,75,165,163,151,60,59,150,164,166,75,85,166,151,128,95,132,
    218,35,252,189,3,67,132,218,32,94,130,218,38,86,75,167,163,151,58,0,130,205,36,145,255,232,5,4,130,205,47,56,0,28,64,14,49,22,238,23,23,0,32,238,41,4,11,65,155,5,37,63,237,18,57,47,
    237,70,66,7,34,53,30,3,106,224,8,34,33,53,33,107,187,6,32,34,130,212,32,53,130,224,32,32,133,223,8,102,30,3,21,20,4,1,211,34,80,85,85,38,30,77,84,89,42,153,238,164,86,57,109,160,103,
    253,234,2,22,206,223,86,164,238,153,40,87,85,78,32,78,166,78,1,148,1,157,37,68,95,58,59,95,68,36,254,99,24,3,6,9,5,174,5,11,10,7,31,65,103,72,61,87,55,25,164,113,113,73,101,63,28,6,
    10,11,6,174,11,12,199,215,68,106,79,55,16,15,51,74,99,65,215,225,139,187,52,64,0,41,64,21,57,22,238,31,31,27,24,26,238,29,27,27,0,40,238,49,140,194,36,206,253,206,17,51,149,200,36,
    35,21,35,53,33,130,1,33,51,21,108,140,7,167,208,39,68,156,254,202,1,54,156,68,173,212,36,170,170,164,162,162,155,216,33,1,0,65,147,7,50,70,0,38,64,19,63,30,25,21,29,238,37,34,30,30,
    0,46,238,55,140,213,36,204,51,253,50,204,84,224,5,34,46,2,39,65,157,11,32,35,79,75,14,175,218,47,3,17,67,52,50,73,19,253,255,19,71,50,46,68,18,7,173,226,40,39,48,48,39,164,38,45,45,
    38,155,230,58,0,1,0,100,255,232,4,215,5,178,0,58,0,28,64,14,8,37,238,36,36,0,27,238,18,4,48,66,123,15,52,32,36,53,52,62,2,55,46,3,53,52,54,54,36,51,50,22,23,21,46,3,108,102,5,33,22,
    51,119,42,13,8,108,55,21,14,3,3,149,254,109,254,98,36,68,95,59,58,95,68,37,104,206,1,49,202,78,166,78,31,79,85,87,40,153,238,164,86,223,206,2,22,253,234,103,160,109,57,86,164,238,153,
    42,89,84,77,30,38,85,85,80,24,225,215,65,99,74,51,15,16,55,79,106,68,108,156,101,49,12,11,174,6,11,10,6,28,63,101,73,113,113,164,25,55,87,61,72,103,65,31,7,10,11,5,174,5,9,6,3,0,3,
    0,68,217,7,52,32,0,51,0,70,0,29,64,15,58,238,23,19,52,39,238,28,0,10,3,33,72,19,8,35,237,50,63,237,68,219,11,43,33,30,3,21,20,14,4,35,34,46,2,39,75,106,5,34,53,17,33,68,239,5,32,1,
    90,245,6,68,233,5,8,62,2,102,132,189,120,56,8,16,25,16,5,212,17,24,16,8,25,52,80,110,142,88,57,95,78,61,23,23,61,78,95,56,70,89,51,19,253,247,4,12,11,7,15,31,51,73,95,1,202,17,50,89,
    73,61,96,72,51,32,14,7,11,11,5,68,229,5,41,76,165,161,151,61,61,151,161,165,76,68,195,5,50,29,53,74,46,46,74,53,29,166,71,124,167,95,2,159,27,92,118,138,68,255,6,39,4,104,253,97,95,
    167,124,71,68,251,6,35,138,118,92,27,131,227,68,237,6,36,30,0,49,0,68,131,227,43,63,238,18,4,15,0,37,238,10,4,50,31,77,143,5,130,223,33,18,57,131,227,68,240,9,33,30,2,68,242,11,36,
    37,17,52,46,2,77,153,5,130,24,33,33,33,69,1,10,32,182,130,221,36,8,56,120,189,132,132,214,46,47,153,114,88,142,110,80,52,25,8,16,24,17,252,194,68,231,9,55,7,11,12,4,2,177,2,9,5,11,
    11,7,14,32,51,72,96,61,73,89,50,17,62,150,130,214,36,168,1,49,233,138,131,213,33,92,110,68,252,5,130,241,36,150,62,164,2,159,132,203,68,236,5,131,203,133,233,36,166,151,128,95,53,131,
    250,32,0,130,219,62,200,255,232,5,59,5,178,0,26,0,39,0,54,0,32,64,17,19,51,238,255,27,1,27,27,0,36,238,11,4,40,66,134,10,32,93,69,3,8,35,17,62,3,51,68,233,12,69,6,8,34,6,7,19,104,47,
    7,47,33,17,22,22,2,10,33,80,85,86,38,38,86,85,80,33,68,232,13,35,253,210,1,110,67,80,5,35,45,76,33,154,69,30,7,46,254,146,33,76,24,3,6,9,5,5,156,5,9,6,3,68,237,7,61,52,74,100,63,215,
    225,3,59,112,109,73,103,64,30,4,4,251,134,31,65,103,72,62,86,55,25,254,20,3,4,141,197,46,42,0,58,0,42,64,22,19,27,57,43,45,238,31,29,133,202,32,39,130,202,32,49,139,202,36,206,51,253,
    206,51,68,53,8,145,207,35,53,51,21,54,113,149,5,36,34,6,7,1,53,131,203,69,245,5,34,38,39,21,156,212,39,6,160,181,192,86,164,238,153,130,213,38,1,6,254,250,33,76,45,131,219,33,193,180,
    154,213,36,162,161,8,109,103,133,216,34,252,207,170,131,209,131,221,35,115,106,6,169,141,217,34,45,0,64,133,217,37,62,46,50,238,34,31,133,217,32,42,130,217,32,54,139,217,35,204,51,
    253,204,154,217,32,51,84,87,5,138,220,37,34,38,39,35,17,22,90,17,8,33,6,6,155,223,40,192,18,72,51,48,67,17,161,169,135,227,36,77,51,72,19,191,134,229,35,170,159,17,65,154,231,38,37,
    46,44,35,12,109,96,133,233,35,253,34,48,39,135,234,36,107,107,11,38,46,131,235,69,11,7,54,28,0,41,0,56,0,32,64,17,8,46,238,255,29,1,29,29,0,33,238,18,4,42,66,139,16,69,19,15,79,184,
    5,34,19,17,38,92,95,7,36,51,19,50,54,55,24,100,103,8,69,19,18,66,158,8,32,121,134,195,39,223,206,212,45,76,33,254,146,69,26,6,38,24,225,215,63,100,74,52,69,16,9,8,35,3,6,9,5,250,100,
    5,9,6,3,3,59,1,227,4,4,30,64,103,73,109,112,253,105,4,3,1,236,25,55,86,62,72,103,65,31,69,21,10,63,34,0,51,0,68,0,48,64,26,58,238,27,19,68,15,3,13,3,52,238,14,14,51,238,32,11,11,0,
    35,10,3,45,134,210,43,51,18,57,47,57,237,51,47,237,63,63,51,69,40,13,36,5,53,51,21,37,69,44,10,34,6,6,1,79,233,5,24,76,214,8,33,30,2,77,75,5,33,38,39,69,43,9,36,2,150,168,2,150,69,
    46,14,37,46,154,254,103,11,19,74,30,14,69,41,5,33,19,11,74,13,10,32,151,130,0,73,239,10,42,29,52,75,46,92,110,4,235,78,200,137,74,34,5,53,71,124,167,95,2,7,253,249,95,167,125,72,54,
    94,129,152,166,85,137,199,78,0,69,37,10,133,243,59,50,64,27,32,18,36,30,18,46,238,18,4,35,238,31,31,52,238,13,10,34,34,0,58,238,10,4,68,80,49,6,130,244,32,18,133,245,32,237,130,246,
    33,48,49,69,58,9,37,22,23,62,3,51,50,74,44,6,36,37,21,35,53,55,80,66,9,34,2,21,3,69,74,9,32,22,74,47,5,43,56,120,189,132,114,154,46,23,61,78,95,57,69,60,9,39,253,106,168,168,2,13,11,
    19,69,44,9,74,53,10,33,19,11,74,51,9,37,110,92,46,75,52,29,74,54,10,53,151,151,151,165,116,78,199,137,85,166,152,129,94,54,72,124,168,95,253,249,2,7,69,76,9,33,137,200,132,241,47,165,
    255,232,5,44,5,178,0,30,0,45,0,62,0,41,64,130,182,48,238,31,6,238,9,9,255,31,1,31,31,0,40,238,15,4,51,66,180,11,35,51,47,237,16,69,68,8,36,19,35,53,51,3,69,72,15,72,163,11,32,17,74,
    105,10,47,35,1,251,33,86,93,92,38,153,153,151,151,38,92,93,86,69,77,8,40,58,95,68,37,254,99,254,84,236,69,76,5,41,16,42,39,32,7,7,29,38,42,20,69,82,7,8,57,234,24,3,6,9,5,2,128,164,
    2,120,5,9,6,3,206,208,68,107,79,54,15,15,52,77,103,66,207,225,3,59,112,109,75,103,64,28,1,2,2,2,251,143,1,4,3,2,31,66,102,72,62,86,55,25,0,3,0,137,221,56,48,0,66,0,47,64,25,65,51,238,
    35,31,31,9,66,6,238,34,255,9,1,9,9,0,43,130,224,32,57,68,124,12,40,253,204,17,51,47,51,237,50,48,78,151,5,149,227,69,92,10,38,14,2,7,1,53,35,3,135,233,34,38,39,21,159,232,39,172,160,
    162,171,86,164,238,153,132,233,35,1,28,170,114,136,237,33,172,161,149,234,35,55,78,102,63,131,234,36,162,159,13,108,97,135,237,36,252,206,170,254,23,132,240,39,65,103,72,108,107,10,
    167,0,140,241,45,51,0,72,0,49,64,26,23,31,70,52,56,238,38,130,245,40,7,238,255,8,1,8,8,0,46,130,242,32,62,68,136,12,33,51,47,69,114,13,149,243,69,118,13,131,246,34,34,38,39,139,248,
    33,6,6,159,249,40,131,18,72,51,46,64,19,129,135,137,254,36,128,51,72,19,128,65,1,9,33,135,128,69,128,6,65,238,16,65,3,7,38,37,46,40,32,20,104,86,65,5,7,35,253,33,48,39,65,6,9,36,96,
    104,18,35,42,69,135,7,62,235,5,178,0,32,0,47,0,64,0,40,64,21,10,27,49,238,47,24,24,255,47,1,47,47,0,39,238,18,4,59,66,214,13,42,16,237,50,57,48,49,5,34,36,38,38,74,165,8,46,36,33,50,
    30,2,23,3,51,21,35,19,14,3,3,19,74,172,9,32,23,74,181,5,8,45,30,2,51,50,62,2,55,3,149,202,254,207,206,104,37,68,95,58,58,95,68,37,1,158,1,147,33,86,93,92,38,151,151,153,153,38,92,93,
    86,9,112,7,32,39,42,16,74,180,5,32,234,74,178,8,62,20,42,38,29,7,24,57,111,161,103,66,103,77,52,15,15,54,79,107,68,208,206,3,6,9,5,253,136,164,253,128,69,160,6,8,77,228,2,2,2,1,28,
    64,103,75,109,112,164,25,55,86,62,72,102,66,31,2,3,4,1,0,1,0,50,1,102,3,36,4,51,0,6,0,12,179,2,4,3,0,0,47,50,205,50,48,49,1,1,51,19,19,51,1,1,136,254,170,135,242,242,135,254,170,1,
    102,2,205,253,248,2,8,253,51,0,130,53,50,150,255,232,5,124,5,154,0,25,0,20,64,9,19,19,0,6,3,13,74,227,8,32,47,74,224,7,79,172,8,8,52,2,53,53,51,21,20,2,6,6,3,9,155,235,157,80,168,57,
    115,172,115,117,173,113,56,168,80,158,235,24,102,191,1,16,170,2,211,253,45,133,212,147,79,79,147,212,133,35,35,171,254,240,191,101,130,101,38,120,0,0,5,94,5,178,133,101,46,12,12,0,
    6,238,19,4,0,18,0,63,63,237,18,57,130,101,24,65,82,13,24,93,52,9,33,4,182,139,96,32,154,131,114,33,2,211,138,93,42,1,16,191,101,102,191,254,240,170,253,45,131,203,32,85,130,101,32,
    219,132,203,50,16,183,2,238,25,18,12,238,14,3,0,63,237,63,237,48,49,33,53,78,227,12,87,216,7,8,35,35,2,236,40,128,199,137,71,73,137,199,126,253,65,2,191,165,1,8,184,98,99,185,254,249,
    164,164,76,142,206,129,129,206,142,76,164,130,14,42,246,167,167,254,246,185,99,0,255,255,0,134,99,35,16,38,3,88,82,23,6,33,106,0,139,23,36,39,1,123,1,221,130,25,32,6,131,31,130,249,
    32,95,130,147,32,229,137,147,35,3,13,238,14,130,247,132,147,33,1,21,66,55,8,32,33,24,86,36,11,33,3,78,136,147,38,2,191,253,65,165,254,248,131,147,36,1,7,164,5,154,139,149,37,1,10,167,
    167,1,10,130,149,81,141,6,32,200,130,101,49,53,0,32,64,16,38,238,49,19,43,43,52,31,31,0,12,3,25,65,204,9,34,57,51,47,71,137,5,33,46,4,81,141,16,24,65,94,9,35,55,51,6,2,78,102,5,40,
    6,2,91,88,141,108,77,49,23,81,137,23,53,82,117,77,41,6,172,8,65,118,174,118,115,163,47,94,24,63,113,157,187,212,112,81,128,19,46,119,119,99,168,121,69,96,160,207,112,153,254,242,201,
    117,81,113,9,54,200,5,178,0,53,0,34,64,17,30,30,39,41,19,19,0,12,238,41,4,25,238,36,66,27,6,37,63,237,18,57,47,18,130,183,33,48,49,76,93,11,24,66,185,9,34,7,35,54,66,44,5,33,23,54,
    71,92,8,33,5,220,82,52,15,34,49,90,73,142,170,32,231,135,216,40,24,17,58,151,163,167,75,86,166,82,42,7,138,169,33,1,14,132,169,138,213,40,0,0,1,0,80,255,232,4,195,130,181,53,52,0,28,
    64,14,40,13,238,14,14,32,0,238,52,19,23,238,32,4,0,63,237,81,89,8,41,37,62,5,53,52,46,2,35,35,53,79,164,26,48,14,4,7,1,236,58,126,123,111,83,50,57,109,160,103,122,122,79,144,25,45,
    61,106,142,161,173,84,140,3,11,23,37,58,83,56,81,70,29,38,85,130,96,65,41,20,3,139,173,52,56,0,34,64,17,44,19,15,12,238,16,19,19,36,0,238,56,19,27,238,36,136,176,38,204,253,204,18,
    57,48,49,136,179,34,21,35,17,81,55,28,142,183,35,42,160,160,42,171,185,34,170,1,240,81,42,25,135,188,65,105,9,32,67,131,187,47,39,14,0,64,238,10,14,14,31,52,238,51,19,22,238,31,143,
    187,123,165,9,33,22,23,65,113,29,32,53,65,153,7,35,6,6,1,198,93,202,7,35,47,75,22,16,159,194,65,168,9,37,12,20,79,2,36,27,93,228,6,81,15,26,134,203,32,164,65,181,10,33,41,50,81,33,
    10,50,54,0,28,64,14,42,14,238,13,13,32,0,238,54,4,23,238,32,19,66,67,11,43,1,14,5,21,20,30,2,51,51,21,35,34,103,99,6,38,62,2,55,21,6,6,35,70,150,12,36,62,4,55,3,59,66,69,20,36,79,31,
    78,166,78,70,140,8,35,59,95,68,36,66,70,5,33,5,14,66,71,24,45,49,101,156,108,68,106,80,54,16,15,51,74,100,64,66,73,10,50,150,255,232,5,239,5,178,0,53,0,26,64,13,23,238,44,4,34,34,70,
    43,13,68,91,5,33,36,38,85,218,10,119,102,5,121,20,6,35,23,35,46,3,100,20,9,8,101,2,6,4,3,61,176,255,0,167,80,168,59,125,193,134,140,196,123,55,26,66,113,86,57,88,60,31,14,32,49,36,
    198,21,39,30,19,55,105,153,99,122,177,114,54,81,170,254,252,24,103,190,1,17,169,2,211,253,45,135,212,147,77,103,177,237,133,104,183,137,80,39,71,100,60,39,76,77,82,45,28,66,79,93,56,
    92,158,116,65,91,173,247,156,172,254,215,220,126,0,0,1,0,110,130,179,32,199,135,179,40,44,44,10,17,18,23,238,10,4,81,195,7,36,237,63,18,57,47,73,214,5,41,53,52,18,54,36,51,50,4,22,
    18,85,188,9,32,2,24,70,112,8,86,159,9,34,2,2,65,133,144,36,1,4,179,176,1,156,188,35,54,105,154,24,132,140,44,1,41,220,126,103,190,254,240,170,253,45,2,211,133,188,33,236,134,147,188,
    33,115,66,131,179,32,160,130,179,50,229,5,154,0,49,0,26,64,13,17,238,10,10,0,38,238,39,3,27,72,157,10,32,237,135,179,38,62,2,51,50,22,23,21,78,27,7,72,126,5,32,53,130,173,37,35,33,
    53,33,50,22,130,204,8,73,20,2,6,4,3,4,144,227,158,83,68,115,149,81,73,120,52,57,121,66,51,87,64,36,39,99,168,130,146,211,138,66,73,134,188,115,253,115,2,141,154,253,180,99,96,187,254,
    237,24,58,111,165,106,100,147,96,46,26,25,175,36,26,27,55,85,58,49,99,79,49,76,146,216,139,71,9,9,36,176,254,236,190,99,139,169,40,52,0,30,64,15,16,20,238,14,130,171,36,41,238,42,3,
    30,138,171,34,204,253,206,139,173,37,23,53,51,17,35,53,158,176,36,2,252,141,224,156,132,176,42,88,69,164,164,38,77,41,53,88,63,34,148,177,34,188,254,233,136,177,38,19,99,254,95,152,
    12,9,166,179,32,63,131,179,35,25,31,238,15,130,179,36,52,238,53,3,41,140,179,32,204,140,179,115,1,13,33,38,34,71,180,8,82,23,8,65,109,10,137,188,50,34,33,22,65,40,36,60,44,25,25,44,
    60,36,57,85,15,9,18,9,164,200,45,3,27,31,25,45,61,36,37,61,44,25,63,51,1,158,207,38,130,0,0,5,199,5,178,66,45,13,35,18,27,238,0,69,31,8,34,237,48,49,24,67,177,9,34,38,39,53,81,23,7,
    33,46,2,72,124,12,32,38,96,43,6,33,3,99,66,45,21,33,145,212,66,45,5,35,2,141,253,115,66,45,5,39,1,19,5,178,58,111,164,107,66,46,9,42,56,84,58,50,98,79,49,76,146,215,140,66,46,6,8,37,
    1,10,167,176,1,20,190,99,0,1,0,130,255,232,5,117,5,154,0,67,0,34,64,17,50,50,0,41,238,58,58,0,19,16,238,17,3,31,65,123,7,35,50,18,57,47,74,15,5,108,90,5,43,62,6,55,53,33,53,33,21,6,
    4,14,3,67,140,11,133,184,34,22,23,35,75,155,5,8,122,51,50,30,2,21,20,14,2,2,255,138,234,169,96,50,87,117,132,140,136,123,48,252,191,4,243,122,254,255,242,215,161,94,77,129,168,91,106,
    167,116,61,26,55,84,58,49,71,47,22,21,12,177,9,23,56,98,132,75,91,148,106,58,94,168,231,24,65,132,198,133,89,151,127,106,88,71,59,48,20,7,164,164,39,86,100,118,140,168,100,112,146,
    86,35,54,93,124,70,50,87,63,36,25,44,60,34,48,60,24,17,69,57,78,119,79,40,50,99,147,97,115,189,134,74,140,215,49,36,64,18,49,52,238,51,18,18,0,27,238,10,10,0,51,3,37,73,123,9,131,214,
    32,16,78,68,7,24,132,237,20,32,14,68,111,11,34,3,36,39,130,247,35,33,21,30,7,132,216,8,114,248,136,231,168,95,58,106,148,91,75,132,98,56,23,9,177,12,21,22,47,71,49,58,84,55,26,61,116,
    167,106,91,168,129,77,94,161,215,242,254,255,122,4,243,252,191,48,123,136,140,132,117,87,50,96,169,234,24,74,134,189,115,97,147,99,50,40,79,119,78,57,69,17,24,60,48,34,60,44,25,36,
    63,87,50,70,124,93,54,35,86,146,112,100,168,140,118,100,86,39,164,164,7,20,48,59,71,88,106,127,151,89,133,198,132,65,0,131,217,40,0,0,5,117,5,178,0,67,0,130,217,39,1,66,238,67,23,238,
    40,32,130,1,49,50,67,18,13,238,50,4,0,63,237,63,18,57,57,47,47,16,237,132,217,37,51,53,54,36,62,3,24,71,63,12,41,50,62,2,53,52,38,39,51,22,22,131,198,118,181,9,32,30,130,235,41,14,
    6,7,21,33,21,130,122,1,1,65,162,21,33,178,8,65,162,8,35,95,168,231,136,65,218,11,33,3,65,65,161,15,39,45,81,61,36,25,42,54,29,65,161,5,43,70,113,78,42,53,99,140,88,115,189,134,74,65,
    218,14,131,215,36,200,255,232,6,209,132,215,48,34,64,18,17,238,50,10,10,0,37,238,58,4,49,3,46,18,75,45,7,76,151,5,32,57,69,105,19,71,136,7,32,4,134,233,41,6,7,35,17,51,17,51,62,5,51,
    69,113,7,8,119,6,4,177,113,172,115,58,48,92,135,86,62,106,36,35,101,55,94,104,39,74,108,68,71,109,80,54,33,14,61,111,157,96,102,172,142,114,89,68,48,32,9,168,168,7,22,68,96,124,157,
    190,112,146,224,153,78,64,134,205,24,63,111,150,87,79,139,104,60,29,22,175,26,36,112,95,57,95,68,38,41,73,101,121,136,72,139,226,159,86,90,152,200,218,224,200,162,48,5,154,252,228,
    76,180,182,171,132,79,110,203,254,226,176,160,254,251,185,101,0,1,0,135,215,41,69,0,38,64,20,15,19,238,14,52,130,217,39,39,238,60,4,51,3,48,18,70,64,7,134,217,33,204,253,68,227,13,
    69,151,5,72,102,9,155,221,35,186,117,175,117,132,221,37,44,34,164,164,30,35,130,221,37,73,105,66,73,111,82,157,221,34,66,132,201,136,221,37,6,150,254,16,182,6,172,222,138,223,32,76,
    131,223,36,23,26,238,13,59,130,223,40,46,238,67,4,58,3,55,18,34,67,107,6,134,223,34,205,253,205,69,195,9,32,55,69,193,13,32,6,65,194,35,133,228,37,91,133,85,21,74,47,69,196,7,37,51,
    77,20,83,92,39,172,236,37,138,104,60,1,36,43,69,207,7,36,49,41,7,110,89,65,211,52,51,67,0,36,64,19,41,238,7,58,48,48,0,33,238,58,4,12,3,9,18,102,177,7,133,242,32,18,66,173,6,40,4,39,
    35,17,35,17,51,30,7,78,239,6,94,146,5,36,20,22,51,50,54,74,58,5,109,154,7,72,31,6,8,116,6,4,120,112,190,157,124,96,69,21,7,168,168,9,32,48,68,89,114,142,172,102,96,157,111,61,14,33,
    54,80,109,71,68,108,74,39,104,94,55,101,35,36,106,62,86,135,92,48,58,115,172,113,141,205,134,64,78,153,224,24,79,132,171,182,180,76,252,228,5,154,48,162,200,224,218,200,152,90,86,159,
    226,139,72,136,121,101,73,41,38,68,95,57,95,112,36,26,175,22,29,60,104,139,79,87,150,111,63,101,185,254,251,160,176,254,226,203,110,69,55,5,34,6,139,5,68,93,5,48,19,59,18,57,3,27,238,
    60,10,20,20,0,35,238,10,4,47,71,141,10,130,215,32,63,96,68,9,32,18,65,202,13,33,53,22,131,215,67,137,6,32,4,70,35,6,50,6,55,51,17,35,17,35,14,5,2,219,145,225,153,78,64,134,205,141,
    67,143,41,48,21,69,96,124,157,189,24,110,203,1,30,176,160,1,5,185,101,67,145,36,35,250,102,3,28,67,145,5,78,33,5,33,5,115,130,217,56,63,0,45,64,23,39,238,45,46,11,8,14,54,46,54,46,
    0,17,238,14,4,9,10,4,29,134,221,39,51,63,237,18,57,57,47,47,130,4,33,17,51,68,106,8,43,18,55,37,55,5,54,36,51,51,21,35,34,70,247,16,37,6,21,20,22,23,7,70,246,13,8,109,3,6,141,238,173,
    96,109,107,254,250,119,1,22,145,1,139,252,95,115,157,254,249,210,158,106,53,71,127,175,103,107,164,112,58,22,52,83,62,99,94,16,5,162,9,26,58,101,134,75,82,144,108,62,92,164,228,24,
    82,159,231,148,143,1,6,109,229,119,235,98,113,164,53,93,128,149,166,84,115,172,114,56,54,93,125,70,48,85,64,38,90,72,31,43,11,48,17,69,57,75,118,81,42,50,99,146,97,115,189,135,74,75,
    85,6,32,145,132,223,60,43,64,22,31,31,49,24,238,60,57,63,39,39,49,2,238,63,18,58,59,19,14,238,49,4,0,63,237,63,132,222,131,220,71,210,5,35,51,53,51,50,70,31,16,32,54,70,30,24,44,2,
    7,5,7,37,6,4,35,141,115,157,1,7,141,204,40,84,61,99,94,19,14,178,8,23,131,204,39,89,146,104,57,92,164,229,136,65,3,5,40,1,6,119,254,234,145,254,117,252,148,206,34,48,57,28,131,205,
    38,117,81,43,56,102,145,89,131,205,38,82,159,230,149,143,254,250,65,2,5,32,0,130,219,36,70,255,232,6,163,130,219,52,61,0,40,64,21,17,238,49,46,52,10,10,0,35,238,52,4,47,48,4,41,70,
    30,9,137,219,75,139,18,77,170,8,66,151,5,46,6,2,2,17,21,35,53,16,18,55,1,55,1,54,36,70,38,6,35,14,2,4,131,66,149,19,62,98,137,87,39,62,123,181,119,140,235,170,95,168,99,94,254,159,
    119,1,70,103,1,9,162,167,246,162,79,71,137,203,70,32,19,8,37,80,139,187,108,138,235,171,96,167,254,206,254,79,254,246,122,108,1,25,1,214,177,1,47,119,254,235,134,143,126,217,254,218,
    168,160,252,174,91,139,221,43,64,0,44,64,23,17,20,238,15,52,49,55,130,223,41,38,238,55,4,50,51,4,44,18,28,66,153,6,65,187,9,75,191,14,32,22,75,192,6,70,40,9,77,226,6,161,228,38,15,
    28,14,164,164,21,23,71,11,5,140,229,35,98,94,254,160,130,229,34,102,1,10,144,229,44,2,2,174,254,16,159,3,112,95,59,96,67,36,147,231,32,213,134,231,32,144,148,231,32,75,131,231,38,28,
    31,238,13,63,60,66,130,231,41,49,238,66,4,61,62,4,55,18,39,144,231,70,47,21,37,6,7,21,38,38,39,83,13,6,65,214,44,59,43,84,122,78,17,83,55,36,60,44,25,44,38,4,7,4,16,33,19,42,67,23,
    82,92,39,74,108,68,140,252,65,226,22,56,75,133,103,64,6,48,58,25,45,61,36,50,76,21,17,2,6,2,6,7,35,29,8,109,89,65,8,23,65,240,18,38,1,0,130,255,232,6,223,66,205,9,33,46,49,66,205,6,
    38,19,47,48,19,41,3,25,76,37,6,65,237,9,76,43,19,94,51,6,74,158,7,47,54,18,18,17,53,51,21,16,2,7,1,7,1,6,4,35,101,219,7,33,2,162,66,205,34,49,1,97,119,254,186,103,254,247,162,166,247,
    162,79,71,137,203,5,178,69,93,18,8,32,80,139,187,108,138,234,171,97,167,1,50,1,177,1,10,122,108,254,231,254,42,177,254,209,119,1,21,134,143,126,217,1,38,66,206,5,52,1,0,120,255,232,
    5,187,5,154,0,35,0,22,64,11,29,3,15,12,238,13,84,238,8,34,237,50,63,67,153,9,76,69,5,76,67,7,8,81,18,17,17,51,17,20,2,6,6,3,55,158,250,174,93,47,82,111,63,254,181,2,18,65,118,89,53,
    57,122,190,132,237,236,168,81,162,241,24,97,172,239,141,103,188,165,136,50,3,164,154,51,132,158,183,103,102,186,141,84,1,25,1,10,2,235,253,21,162,254,249,185,101,0,1,0,150,0,0,5,217,
    5,178,133,129,46,35,18,21,16,238,20,18,31,238,6,4,0,63,237,63,132,129,33,51,17,102,41,5,40,30,2,21,20,14,2,7,21,33,24,181,67,10,35,2,17,17,150,130,105,32,160,135,132,35,1,75,253,238,
    137,132,38,2,235,162,1,7,185,101,138,134,34,157,50,130,134,134,37,254,231,254,246,253,21,130,127,36,160,0,0,6,62,130,127,52,34,0,24,64,12,2,238,34,18,12,238,24,4,21,18,19,3,0,63,205,
    57,131,131,35,48,49,51,53,24,72,238,8,32,34,130,129,32,7,130,251,80,233,7,8,69,20,2,6,4,35,160,2,211,128,200,138,73,75,140,200,125,103,168,136,108,42,165,168,98,1,27,186,155,1,3,186,
    103,101,186,254,248,164,164,74,145,212,138,124,206,149,82,60,99,130,70,1,1,244,254,227,149,160,108,192,254,246,159,176,254,239,187,97,255,255,0,134,131,35,16,38,3,120,87,15,5,35,3,
    25,0,0,138,23,32,34,132,23,46,3,1,123,2,139,0,0,0,1,0,95,0,0,5,253,135,179,40,14,17,16,3,23,238,11,4,33,98,114,5,130,178,38,205,57,48,49,33,33,34,105,19,9,33,4,23,130,178,32,39,25,
    3,22,10,8,64,33,5,253,253,45,164,254,248,186,101,103,186,1,3,155,186,1,27,98,168,165,42,108,136,168,103,125,200,140,75,73,138,200,128,2,211,97,187,1,17,176,159,1,10,192,108,160,149,
    1,29,254,12,1,70,130,99,60,82,149,206,124,138,212,145,74,131,133,54,150,255,232,6,57,6,31,0,38,0,26,64,13,26,21,28,238,25,22,3,7,3,11,68,23,7,35,204,253,50,205,88,251,10,33,16,18,73,
    76,6,40,2,39,53,33,53,51,17,35,53,99,237,5,8,67,2,3,30,162,243,162,81,168,236,244,124,186,123,61,53,89,118,65,1,206,164,164,254,238,66,114,85,49,90,171,250,24,101,185,1,7,162,2,235,
    253,21,254,246,254,231,84,142,186,103,102,185,159,130,49,154,133,254,82,133,49,139,166,185,96,146,242,174,97,133,141,36,123,6,57,5,178,133,141,41,38,18,24,19,17,238,21,23,18,34,66,
    73,6,38,206,253,205,51,63,48,49,66,75,14,134,135,32,53,77,114,7,66,78,5,43,243,162,160,250,171,90,49,85,114,66,1,18,130,137,42,50,65,118,89,53,61,123,186,124,244,236,66,80,7,39,174,
    242,146,96,185,166,138,50,131,132,41,154,49,130,159,185,102,103,186,142,84,66,82,6,41,0,1,0,55,0,0,6,82,6,31,130,139,36,28,64,14,2,238,130,141,46,20,238,21,25,17,12,238,28,4,0,63,237,
    204,57,220,100,248,5,66,87,16,37,35,53,33,21,35,17,66,91,12,32,180,66,91,11,43,138,110,45,158,125,1,162,125,96,1,36,179,66,94,24,38,217,160,160,254,251,154,158,66,96,9,34,0,255,255,
    135,145,35,16,38,3,126,66,97,6,33,45,0,146,23,35,1,123,2,141,66,97,7,33,6,122,132,193,46,30,64,15,14,21,11,20,15,238,17,27,238,11,4,38,66,100,7,35,222,237,50,16,66,103,19,133,194,66,
    107,26,35,179,1,36,96,131,195,35,158,45,110,138,66,110,21,35,158,154,1,5,130,195,32,39,66,112,13,54,1,0,100,255,232,5,249,5,154,0,43,0,27,64,14,32,35,238,34,3,12,9,238,113,16,9,34,
    237,50,63,80,28,5,81,134,5,36,55,33,53,33,21,80,243,13,32,39,130,17,32,33,83,209,7,8,88,40,157,252,176,95,131,118,254,235,1,214,60,98,70,38,70,131,188,118,118,190,134,72,36,69,98,63,
    1,214,254,233,119,132,97,181,255,0,24,100,183,1,3,158,189,1,47,102,164,154,41,127,157,179,93,126,201,141,75,76,142,201,126,89,169,154,132,51,154,164,103,254,209,186,157,254,254,185,
    102,0,0,1,0,100,0,0,5,249,5,178,133,159,42,42,2,238,43,18,22,18,238,21,18,32,106,251,7,133,159,34,51,53,33,82,38,5,32,54,85,43,5,34,20,2,7,68,209,11,70,35,5,8,77,23,21,100,1,21,118,
    131,95,177,252,157,159,1,0,180,97,132,119,1,23,254,42,63,98,69,36,72,134,191,118,117,188,131,70,36,69,99,62,164,102,1,51,189,159,1,1,181,99,101,183,254,255,156,186,254,204,103,164,
    154,51,132,154,170,88,126,201,142,76,75,141,201,125,90,172,154,132,50,154,130,155,36,200,255,242,6,103,130,155,49,49,0,31,64,16,24,238,38,4,33,30,31,3,5,8,7,18,14,67,174,6,34,205,57,
    63,130,2,75,65,6,36,39,21,35,17,51,95,125,7,82,228,5,38,7,7,17,51,21,62,3,134,179,8,88,14,4,3,162,82,162,147,126,45,168,158,41,119,144,166,88,127,203,141,76,76,141,203,127,88,166,144,
    119,41,158,168,45,125,146,162,84,157,1,5,187,104,47,88,126,157,186,14,33,65,98,66,248,1,190,66,109,78,43,86,152,210,124,128,211,150,83,43,79,113,70,1,1,190,248,69,103,67,33,108,195,
    254,242,163,107,192,164,131,92,50,115,169,5,132,171,35,16,38,3,132,66,171,6,33,32,0,146,23,35,1,123,2,162,66,171,5,35,255,242,5,254,130,219,32,47,131,219,43,43,40,41,18,15,18,17,3,
    24,238,10,4,78,207,7,32,237,131,218,32,205,94,138,6,115,206,10,34,53,51,17,66,168,11,57,50,62,2,55,51,17,35,53,14,3,3,36,157,254,252,188,104,104,187,1,5,157,84,162,146,125,133,230,
    33,165,89,135,230,33,89,165,133,230,8,44,126,147,162,14,110,195,1,15,160,163,1,14,195,108,33,67,103,69,248,254,66,1,70,113,79,43,83,150,211,128,124,210,152,86,43,78,109,66,254,66,248,
    66,98,65,33,88,179,5,37,6,113,6,38,0,49,131,169,36,37,32,39,238,36,66,195,17,32,205,69,54,6,69,189,5,66,197,21,32,53,131,166,8,91,35,30,3,21,20,14,2,3,52,159,255,0,180,97,131,120,254,
    233,1,214,63,99,68,36,72,134,190,118,118,188,131,70,36,68,99,63,1,174,160,160,237,59,92,64,34,95,177,252,24,99,182,1,0,157,186,1,55,103,164,164,52,130,150,167,87,125,202,141,76,76,
    141,200,125,89,168,150,130,51,164,140,254,68,140,51,130,154,174,94,159,255,180,97,130,171,36,70,255,116,6,83,66,49,7,46,46,1,3,238,49,47,18,26,22,238,25,18,36,238,14,71,156,7,42,206,
    253,205,51,48,49,23,17,51,21,51,88,151,7,35,4,22,18,21,66,210,22,34,33,21,70,136,140,66,214,5,37,131,120,1,23,254,42,143,180,36,254,82,140,1,188,137,139,34,99,182,255,130,182,33,254,
    201,150,182,75,147,5,52,242,6,103,6,31,0,53,0,35,64,18,36,32,238,33,37,29,24,238,42,4,66,223,14,69,120,5,72,210,5,66,225,21,36,35,53,33,21,35,66,229,39,35,130,1,162,120,66,232,40,35,
    167,156,156,225,66,234,14,36,255,255,0,70,255,132,183,35,16,38,3,138,66,233,12,142,23,66,233,11,33,6,128,130,231,52,51,0,37,64,19,47,44,45,18,15,22,10,21,16,238,18,28,238,10,4,38,67,
    200,6,32,237,69,153,5,32,63,66,239,19,69,156,17,66,243,26,39,120,1,162,130,158,41,119,144,66,246,35,36,225,156,156,254,89,66,248,24,8,36,0,1,0,80,255,236,6,251,6,38,0,72,0,45,64,24,
    37,238,67,19,52,47,54,238,51,49,3,70,31,31,0,16,13,238,14,3,26,86,192,11,37,57,63,205,253,50,205,91,12,5,37,46,2,53,52,62,4,67,7,15,33,51,6,89,169,9,72,79,9,32,5,85,34,5,8,127,38,39,
    6,6,2,8,107,158,103,51,34,57,75,81,83,37,254,124,2,70,66,137,111,71,29,63,99,69,62,100,70,38,168,1,38,71,101,61,69,99,63,29,71,111,137,66,1,251,160,160,254,199,37,83,81,75,57,34,50,
    104,158,107,134,190,47,47,188,20,83,142,189,105,97,174,153,131,107,82,27,164,164,52,141,186,238,150,79,131,95,52,51,111,176,125,125,176,111,51,52,95,131,79,150,238,186,141,52,164,140,
    254,68,140,27,82,107,131,153,175,96,105,189,142,83,110,92,92,110,0,132,239,36,116,6,251,5,174,100,47,5,54,71,2,238,72,18,17,14,56,56,20,39,40,35,33,238,37,39,18,50,238,20,4,61,67,71,
    7,52,63,206,253,205,51,17,18,57,47,18,57,63,237,50,48,49,51,53,33,46,5,86,223,5,33,22,23,81,171,8,33,4,7,72,173,15,36,14,2,23,35,52,69,157,5,40,21,20,30,2,23,21,80,1,132,133,198,45,
    51,103,158,107,136,188,47,47,190,134,107,158,104,50,65,4,5,33,1,57,130,230,32,5,65,6,7,41,61,101,71,38,1,168,38,70,100,62,65,6,7,32,164,142,197,42,83,142,189,105,96,175,153,131,107,
    82,27,131,227,34,164,53,140,65,3,21,34,140,53,164,130,239,8,34,70,255,232,6,73,6,38,0,76,0,48,64,25,49,45,238,46,50,43,57,67,26,238,27,27,0,35,238,57,4,7,10,9,18,18,70,104,9,32,237,
    130,235,37,237,57,16,205,57,220,71,176,5,40,46,4,39,17,35,17,51,30,5,94,116,7,32,53,24,91,46,8,35,4,7,35,17,66,181,5,32,62,130,29,76,2,6,68,248,5,8,52,4,49,98,169,143,117,93,68,22,
    168,168,27,70,90,111,135,163,96,183,195,61,107,143,82,82,143,107,61,195,183,98,164,136,110,88,69,27,168,125,1,162,125,20,66,92,118,143,168,96,128,202,140,73,113,83,7,8,71,73,138,199,
    24,34,56,71,73,69,25,254,208,1,248,34,80,81,76,58,35,130,132,69,91,54,23,164,24,57,95,70,117,128,35,58,76,81,80,34,1,228,160,160,254,228,24,67,73,72,57,35,59,108,152,93,63,105,83,59,
    16,15,59,82,105,61,97,156,110,59,255,255,0,134,247,35,16,38,3,144,67,193,5,35,2,172,0,0,138,23,36,39,1,123,2,26,130,25,32,6,131,31,70,1,6,32,103,65,39,7,51,70,67,68,18,10,51,238,50,
    50,0,27,34,20,33,28,238,30,42,238,20,101,67,9,67,199,5,36,18,57,47,237,57,67,204,6,33,46,2,101,70,8,38,62,2,51,50,30,4,23,65,27,6,32,35,24,163,71,8,56,51,21,34,14,2,21,20,22,51,50,
    62,4,55,51,17,35,17,14,5,2,124,126,199,138,73,111,243,7,54,74,139,202,128,96,168,143,118,92,66,20,125,1,162,125,168,27,69,88,110,136,164,98,65,58,11,8,45,96,163,135,111,90,70,27,168,
    168,22,68,93,117,143,169,24,59,110,156,97,61,105,82,59,15,16,59,83,105,63,93,152,108,59,35,57,72,73,67,24,1,28,160,160,254,28,65,59,5,44,128,117,70,95,57,24,164,23,54,91,69,132,130,
    65,59,5,41,254,8,1,48,25,69,73,71,56,34,133,247,54,5,229,5,154,0,61,0,32,64,18,47,238,46,3,36,26,12,50,4,0,15,238,16,90,186,9,34,18,23,57,106,229,11,44,4,53,52,38,39,53,22,22,21,20,
    14,4,23,72,140,5,34,55,54,46,130,21,40,54,55,21,6,6,21,20,30,4,101,68,5,63,36,151,249,178,98,15,22,26,22,15,58,70,156,145,15,22,25,22,14,1,1,70,129,185,115,116,184,129,70,1,1,14,130,
    16,36,15,146,156,70,59,132,34,8,64,96,177,250,24,97,180,1,1,159,59,96,81,71,68,68,38,54,62,4,164,7,144,131,42,70,68,69,81,97,62,118,193,137,75,75,137,193,118,62,97,81,69,68,70,42,131,
    144,7,164,4,63,53,38,68,68,71,81,96,59,159,254,255,180,97,0,73,231,6,36,229,5,178,0,57,131,203,48,23,3,47,37,4,13,0,238,57,18,26,238,27,18,42,238,13,71,27,5,33,63,237,130,205,41,48,
    49,55,54,54,53,52,46,2,53,24,228,162,9,46,20,14,2,21,20,22,23,21,38,38,53,52,62,4,53,77,147,5,32,7,132,204,47,6,7,100,70,58,31,38,31,98,178,249,151,153,250,177,96,130,10,35,59,70,156,
    146,131,209,32,13,130,208,35,184,116,115,185,130,208,32,13,131,207,59,145,156,164,4,62,54,57,99,107,130,88,159,1,1,180,97,97,180,254,255,159,88,130,107,99,57,53,63,157,208,133,191,
    8,41,6,7,5,154,0,59,0,49,64,28,59,18,25,238,34,5,238,56,0,34,16,34,2,34,56,34,56,39,10,238,51,18,20,238,39,3,30,3,0,63,63,237,85,95,6,44,93,16,237,16,237,63,48,49,51,62,3,51,50,69,
    122,9,41,35,34,14,2,35,34,46,2,39,51,93,18,5,74,30,8,131,20,33,35,34,130,209,8,70,3,43,74,103,63,63,98,98,112,78,116,195,142,79,79,141,196,116,75,112,99,100,63,65,103,73,42,3,168,4,
    63,53,57,97,102,118,77,154,1,3,187,104,48,88,126,157,183,102,77,119,103,97,55,54,63,3,59,97,70,39,32,37,32,86,151,204,117,116,201,147,84,130,10,37,39,70,98,58,50,51,130,8,42,108,190,
    254,251,153,102,187,161,130,92,50,130,13,41,54,47,0,255,255,0,100,0,0,6,130,211,35,16,38,3,150,67,135,6,33,221,0,146,23,35,1,123,2,96,76,223,8,32,2,65,3,7,57,29,3,34,238,25,54,238,
    3,0,25,16,25,2,25,3,25,3,8,39,238,20,3,49,238,8,18,103,94,5,65,3,14,32,33,95,123,5,130,226,33,4,53,96,243,5,40,30,2,51,50,54,55,51,14,3,133,246,92,134,9,8,78,51,50,30,2,23,5,90,3,63,
    54,55,97,103,119,77,102,183,157,126,88,48,104,187,1,3,154,77,118,102,97,57,53,63,4,168,3,42,73,103,65,63,100,99,112,75,116,196,141,79,79,142,195,116,78,112,98,98,63,63,103,74,43,3,
    47,54,32,37,32,50,92,130,161,187,102,153,1,5,190,108,130,13,37,51,50,58,98,70,39,130,8,39,84,147,201,116,117,204,151,86,130,10,8,69,39,70,97,59,0,1,0,120,1,102,2,150,4,51,0,9,0,21,
    64,9,1,8,218,0,6,3,218,4,0,0,47,222,237,50,16,237,50,48,49,19,53,1,33,53,33,21,1,33,21,120,1,125,254,131,2,30,254,131,1,125,1,102,106,1,233,122,106,254,23,122,131,65,33,0,178,130,65,
    50,231,0,17,0,29,64,13,1,11,218,0,10,3,218,8,7,4,16,14,130,69,36,50,206,222,204,51,130,73,33,253,205,134,73,36,51,53,51,21,51,131,77,35,35,21,35,53,132,81,34,210,124,208,131,82,33,
    208,124,133,84,33,180,180,131,86,35,180,180,0,255,124,237,5,46,229,5,154,16,38,3,148,0,0,16,7,4,235,1,229,65,159,7,35,5,229,5,178,130,23,32,149,132,23,32,236,137,23,65,207,10,35,6,
    4,237,100,65,205,17,32,38,131,21,37,16,7,4,234,2,242,65,235,18,134,29,45,1,78,2,207,251,251,255,255,0,95,0,0,6,2,132,129,32,153,132,105,36,238,6,2,0,0,68,153,5,56,6,186,5,154,0,76,
    0,36,64,20,72,33,40,238,67,19,54,238,53,3,33,3,13,238,14,98,50,9,67,9,6,74,60,5,33,53,52,93,147,5,68,156,5,94,115,9,105,225,10,36,52,46,2,53,52,68,168,7,130,33,67,38,6,8,34,14,3,2,
    39,102,159,108,56,27,31,27,54,57,140,140,26,31,26,32,65,100,69,75,101,61,27,168,27,61,101,75,69,100,65,32,130,19,35,140,140,57,54,130,33,54,56,108,159,102,71,113,88,65,23,23,65,88,
    113,24,80,141,193,112,99,182,163,143,61,68,178,6,61,68,133,149,175,110,78,137,102,58,60,115,171,111,3,67,252,189,111,171,115,60,58,102,137,78,110,175,149,133,68,68,186,6,41,61,140,
    161,183,103,112,193,141,80,29,116,219,6,132,241,37,0,0,6,186,5,178,133,241,48,0,238,76,18,18,23,56,18,36,238,37,18,50,238,23,4,63,68,193,9,36,63,18,57,63,237,68,195,10,37,62,2,51,50,
    30,2,111,131,5,132,210,68,200,9,100,164,9,33,17,35,111,134,6,130,33,32,30,130,3,34,6,7,100,145,204,32,71,65,3,35,42,164,4,62,54,61,143,163,182,99,113,192,137,191,41,80,141,192,113,
    103,183,161,140,61,53,68,231,5,65,5,12,35,252,189,3,67,65,5,15,84,97,6,8,43,38,5,154,0,74,0,50,64,26,3,238,71,74,56,18,238,21,21,46,8,238,66,66,74,18,29,238,46,46,38,34,238,41,38,3,
    0,63,222,237,17,51,47,237,63,130,3,38,18,57,47,237,57,16,222,130,253,32,51,73,128,5,34,51,50,62,133,227,121,220,12,34,35,34,38,68,249,9,72,156,12,68,8,5,46,6,7,150,8,140,134,58,98,
    98,103,63,66,108,75,41,121,234,9,56,41,75,108,66,68,105,97,100,62,125,137,8,168,4,62,50,51,102,104,106,55,107,172,121,66,72,159,7,51,66,121,172,107,55,107,103,103,52,48,62,4,121,124,
    16,19,16,26,53,82,104,177,7,35,47,78,55,31,130,17,35,124,121,42,39,130,6,49,50,93,134,84,60,104,83,60,16,15,57,81,105,65,94,139,90,44,130,20,33,42,39,138,247,8,36,82,0,64,64,33,3,238,
    79,82,64,28,19,28,25,19,20,22,238,27,25,25,54,8,238,74,74,82,18,37,238,54,54,46,42,238,49,46,143,254,39,205,253,205,51,17,51,17,18,65,5,21,32,35,122,25,19,65,13,50,41,35,160,254,173,
    1,83,160,35,206,223,65,17,57,122,53,6,65,21,47,8,49,84,0,64,64,33,66,31,16,31,24,16,19,23,238,28,24,24,76,39,238,56,56,47,44,238,51,47,3,3,238,81,0,8,238,76,76,0,18,0,63,50,47,237,
    16,220,237,63,220,237,17,66,23,5,34,204,253,204,65,24,5,33,48,49,66,27,12,33,38,39,88,227,5,40,33,53,33,54,54,51,50,22,23,71,250,5,71,33,5,66,38,44,51,194,181,20,81,50,50,81,20,254,
    226,1,36,22,77,46,46,75,23,185,197,65,30,53,46,114,107,6,45,53,54,45,164,40,46,46,39,6,112,107,65,34,37,72,33,5,33,4,244,67,49,7,8,32,18,55,238,54,54,28,66,238,8,8,74,71,238,3,74,18,
    40,238,33,36,45,238,28,28,36,3,0,63,51,47,237,16,222,65,28,10,35,237,57,48,49,71,30,9,74,174,14,71,36,5,33,6,6,71,36,9,122,162,15,49,51,50,22,23,4,76,4,62,48,52,103,103,106,56,106,
    173,121,66,74,182,7,57,66,121,173,106,56,105,104,102,51,50,62,4,168,8,137,125,62,100,97,105,68,66,108,75,41,223,122,178,8,8,34,41,75,108,66,63,103,98,98,58,134,140,8,39,42,16,19,16,
    44,90,139,94,65,105,81,57,15,16,60,83,104,60,84,134,93,50,130,20,35,39,42,121,124,130,6,35,31,55,78,47,122,186,6,35,56,82,53,26,130,17,42,124,121,0,0,3,0,100,255,232,6,186,130,249,
    8,33,48,0,65,0,82,0,50,64,28,55,238,39,19,26,238,25,3,49,238,21,21,66,238,44,17,17,0,19,3,13,238,14,3,76,76,206,6,33,237,63,117,164,8,32,237,78,176,10,130,250,43,38,39,53,22,22,7,33,
    17,51,17,33,38,70,9,19,122,209,7,130,42,34,46,2,39,122,230,5,32,2,130,223,34,2,53,17,70,36,10,42,135,146,1,1,191,168,1,191,1,146,135,69,71,17,51,1,117,25,60,102,77,69,100,65,32,13,
    19,23,9,251,180,9,23,19,13,70,80,7,70,48,14,44,141,137,1,29,254,227,137,141,7,164,4,63,53,70,22,16,35,3,241,254,126,69,57,7,41,76,130,114,101,47,47,101,114,130,76,70,93,7,37,1,130,
    0,3,0,100,70,53,6,65,25,8,56,0,238,48,18,44,238,66,66,41,238,18,49,49,23,42,18,36,238,37,18,60,238,23,4,72,70,65,12,118,192,6,70,71,35,41,55,33,17,35,17,33,22,6,7,1,110,168,5,35,2,
    35,34,14,117,214,9,37,2,21,20,30,2,23,70,76,28,46,135,146,1,254,65,168,254,65,1,146,135,3,127,1,210,65,13,7,35,77,102,60,25,71,94,8,35,13,19,23,9,70,89,34,42,141,137,254,227,1,29,137,
    141,7,1,193,65,10,12,35,254,126,1,130,65,40,12,8,56,0,3,0,150,0,0,5,122,5,154,0,50,0,65,0,82,0,61,64,32,3,66,238,45,50,30,82,238,51,51,4,238,7,7,20,71,238,40,40,50,18,60,238,20,20,
    11,8,65,238,15,11,3,0,63,220,237,50,70,106,11,32,51,70,109,5,44,50,48,49,51,54,54,55,17,35,53,51,17,38,68,50,29,35,14,2,7,19,109,162,11,32,17,77,32,5,58,53,52,46,2,35,35,150,6,128,
    112,246,246,123,116,7,168,4,76,90,51,103,107,114,63,110,177,123,70,90,12,8,47,67,118,109,104,52,48,63,38,17,2,246,233,206,223,41,78,111,70,60,97,84,78,43,43,81,87,99,60,67,108,76,41,
    57,109,160,103,233,102,120,17,1,144,164,1,134,14,122,105,70,105,25,53,11,20,30,20,3,35,113,113,47,78,55,31,11,15,15,5,252,71,5,16,15,11,70,173,7,65,29,12,8,48,68,0,86,0,75,64,39,3,
    72,238,45,50,30,55,85,85,71,238,55,51,51,7,69,4,238,53,7,7,20,77,238,40,40,50,18,63,238,20,20,12,8,68,238,15,12,3,0,63,222,65,36,12,33,204,253,118,68,6,70,147,5,65,43,47,118,93,13,
    34,19,53,35,65,49,8,34,38,39,21,65,48,44,35,172,160,161,169,65,49,8,33,172,172,65,51,8,33,170,160,65,48,42,36,162,158,12,110,100,65,51,7,36,253,43,170,254,114,65,54,7,36,107,107,11,
    167,0,65,55,12,52,71,0,92,0,73,64,38,3,77,238,45,50,30,58,90,90,72,76,238,58,55,66,91,6,44,82,238,40,40,50,18,66,238,20,20,12,8,71,65,54,19,32,237,118,136,6,65,53,53,70,201,15,36,7,
    1,34,38,39,65,58,11,33,6,6,65,59,44,40,123,23,78,45,42,73,23,125,130,65,64,8,37,1,13,50,81,20,118,65,68,8,35,126,121,22,77,65,70,42,38,38,48,40,34,21,106,87,65,72,8,34,114,54,45,65,
    73,9,44,93,103,20,41,46,0,3,0,100,0,0,5,72,67,159,11,63,52,42,238,35,38,20,66,238,51,51,46,238,43,43,10,57,238,30,30,38,3,77,238,10,10,0,47,82,238,5,0,18,66,122,15,32,237,67,159,9,
    32,33,79,176,5,105,134,7,32,55,111,142,9,49,51,50,54,55,51,6,6,7,17,51,21,35,17,22,22,23,1,17,118,219,23,43,4,160,2,17,38,63,48,52,104,109,118,67,70,227,12,8,51,123,177,110,63,114,
    107,103,51,90,76,4,168,7,116,123,246,246,112,128,6,254,98,43,78,84,97,60,70,111,78,41,223,206,233,233,103,160,109,57,41,76,108,67,60,99,87,81,43,20,30,20,11,70,231,25,50,105,122,14,
    254,122,164,254,112,17,120,102,3,35,1,135,5,15,15,11,70,245,14,8,56,11,15,16,5,0,1,0,100,0,178,2,136,4,231,0,50,0,33,64,15,37,12,1,32,217,29,23,22,25,50,49,7,217,5,1,0,47,51,237,51,
    205,220,50,205,51,237,18,57,57,48,49,37,53,38,38,39,53,126,22,7,42,35,35,34,38,53,52,54,55,53,51,21,130,241,32,21,24,188,108,16,8,46,7,21,1,60,51,101,43,88,126,40,67,48,27,65,80,20,
    120,132,113,103,124,48,94,41,45,107,47,34,68,56,35,65,60,32,124,136,32,56,76,44,178,161,2,22,18,129,53,24,188,111,7,39,89,117,17,166,162,3,22,17,24,188,113,12,37,49,76,55,34,9,168,
    71,163,11,8,37,52,0,69,0,86,0,57,64,32,59,238,43,19,30,238,29,3,24,19,238,21,53,238,25,25,70,238,48,17,17,0,21,3,13,238,14,3,80,71,167,16,35,16,237,50,63,71,170,21,38,53,35,53,33,21,
    35,21,71,174,64,35,160,1,232,160,71,177,66,37,138,130,156,156,130,138,71,177,23,34,240,254,127,70,136,8,32,129,71,177,5,32,129,71,177,9,32,129,71,177,10,65,39,8,60,0,238,52,18,42,47,
    238,45,41,238,53,53,49,238,18,86,86,13,45,18,36,238,37,18,64,238,23,4,76,71,181,18,34,16,237,50,78,0,36,40,55,33,21,51,21,33,53,51,53,71,188,64,35,160,254,24,160,71,191,69,65,60,7,
    36,1,194,47,101,114,65,24,9,35,254,127,1,129,65,54,12,8,53,0,3,0,110,0,0,5,136,5,154,0,54,0,69,0,86,0,67,64,35,3,70,238,49,54,34,86,238,55,55,11,7,4,238,8,11,11,24,75,238,44,44,54,
    18,64,238,24,24,16,12,69,238,19,16,70,157,22,36,237,57,16,222,237,70,153,8,36,21,35,17,51,21,71,201,61,43,165,6,128,111,140,160,160,140,122,116,7,167,71,203,8,33,176,124,71,203,15,
    36,103,52,48,63,39,71,203,6,44,80,114,74,58,92,83,77,43,44,83,88,97,58,71,203,8,45,101,120,17,1,145,170,1,240,162,1,134,14,123,104,71,206,43,34,4,16,16,71,206,9,65,45,12,8,43,72,0,
    90,0,79,64,41,3,76,238,49,54,34,59,89,89,75,238,59,55,55,57,8,10,73,7,5,238,10,10,24,81,238,44,44,54,18,67,238,24,24,16,12,72,65,51,19,42,253,205,50,16,205,50,51,47,51,237,50,70,157,
    14,65,57,42,71,215,28,65,62,46,36,172,160,161,169,41,65,63,7,33,172,172,65,65,8,33,170,160,65,62,45,71,220,17,65,68,7,35,107,107,11,167,65,67,12,56,75,0,96,0,79,64,41,3,81,238,49,54,
    34,62,94,94,76,80,238,62,59,55,55,11,6,66,119,5,47,86,238,44,44,54,18,70,238,24,24,15,12,75,238,19,15,74,58,16,35,204,253,206,17,71,225,21,65,67,42,71,229,16,32,19,71,229,16,65,73,
    46,40,70,23,78,45,44,75,23,148,156,65,78,8,36,216,50,81,20,65,65,81,8,35,153,144,21,80,65,83,45,38,38,48,44,36,16,109,95,71,233,13,65,86,7,36,101,106,15,43,50,68,241,5,33,5,126,67,
    201,11,62,56,42,238,35,38,20,70,238,55,48,50,238,45,43,43,55,55,10,61,238,30,30,38,3,81,238,10,10,0,86,51,71,236,19,37,51,47,206,253,206,16,67,201,7,71,239,35,95,141,5,71,243,33,36,
    39,63,48,52,103,71,243,15,33,124,176,71,243,8,52,167,7,116,122,140,160,160,140,111,128,6,254,99,43,77,83,92,58,73,115,80,71,245,12,35,58,97,88,84,71,245,30,45,104,123,14,254,122,162,
    254,16,170,254,111,17,120,101,71,248,23,34,16,16,4,84,251,11,50,80,0,41,64,23,44,238,71,19,58,238,57,3,76,0,37,34,238,35,84,254,14,34,237,50,18,103,12,6,116,86,6,78,224,7,33,21,20,
    86,237,9,32,53,90,142,6,32,20,88,32,9,85,4,45,35,160,1,232,160,78,209,7,85,7,52,37,2,167,156,156,253,89,84,3,15,78,242,20,132,255,78,211,6,132,255,41,0,238,80,18,56,61,238,18,13,59,
    85,12,8,32,67,71,20,12,71,13,38,86,29,5,93,151,6,37,17,51,21,33,53,51,85,18,53,39,77,102,60,25,160,254,24,160,86,25,12,78,187,34,33,144,131,85,21,12,37,253,89,156,156,2,167,65,19,15,
    8,43,0,1,0,110,0,0,4,254,5,154,0,78,0,56,64,29,60,25,21,18,238,22,25,25,70,33,238,50,50,41,38,238,45,41,3,3,238,75,0,8,238,70,70,0,83,5,21,33,18,57,83,1,14,130,242,34,33,21,35,131,
    241,24,78,254,9,84,19,33,32,110,85,33,15,37,254,138,160,160,1,118,84,17,59,35,150,1,208,150,78,145,5,85,38,33,65,7,10,60,84,0,67,64,35,66,18,238,31,31,29,25,27,20,24,22,238,27,27,76,
    39,238,56,56,48,44,238,51,48,84,19,19,32,222,84,19,7,70,201,7,32,237,65,18,17,35,39,21,35,53,65,21,6,70,170,12,65,24,46,45,51,97,143,93,160,254,252,160,160,1,4,160,186,198,84,13,53,
    47,58,83,55,28,3,169,170,170,1,240,162,162,161,6,111,108,84,14,37,66,39,9,8,37,88,0,68,64,35,70,35,16,35,28,16,25,19,23,238,32,26,28,28,80,43,238,60,60,51,48,238,55,51,3,3,238,85,0,
    8,238,80,80,66,45,19,35,205,204,253,204,86,78,6,66,51,14,85,53,7,37,35,21,35,17,51,21,24,74,134,14,33,14,2,65,37,46,51,162,152,21,80,49,50,81,20,189,160,160,194,23,78,45,44,76,23,156,
    165,65,43,53,49,105,105,14,44,51,54,45,170,1,240,162,38,48,45,37,14,110,98,65,45,39,36,100,0,0,4,244,67,85,7,44,40,238,33,36,18,54,58,59,238,55,54,54,8,85,52,5,39,70,238,8,8,0,75,238,
    3,78,102,5,82,5,10,34,205,253,205,87,109,6,85,65,45,36,53,51,17,35,53,24,118,121,8,34,50,62,2,85,69,52,41,1,118,160,160,254,138,103,160,109,57,85,71,50,39,150,254,48,150,25,55,87,61,
    85,74,9,85,73,9,8,43,56,0,74,0,92,0,69,64,38,81,238,47,19,28,92,75,34,238,33,3,75,238,27,19,58,14,57,238,20,20,52,27,27,0,26,21,238,23,3,13,238,14,3,69,77,171,8,44,237,50,18,57,47,
    57,51,47,237,17,57,57,16,94,131,5,85,92,16,35,30,3,23,5,77,177,6,34,37,62,3,96,19,6,91,106,11,33,19,37,70,156,12,33,19,17,70,151,11,32,53,85,99,10,51,35,75,80,83,42,1,156,160,1,232,
    160,1,156,42,83,80,76,34,50,61,91,127,15,34,205,254,65,91,173,11,70,165,10,85,104,13,47,3,22,33,40,20,196,158,156,156,158,196,20,40,33,22,3,70,123,20,35,3,209,211,12,69,163,12,35,1,
    98,254,158,69,161,12,32,12,77,191,10,65,65,5,8,38,71,64,39,45,50,238,48,43,58,38,57,238,44,92,52,0,75,238,51,51,19,44,44,14,48,18,37,238,38,18,69,238,24,4,81,238,14,4,1,110,4,8,33,
    237,63,86,154,5,65,68,7,41,17,57,57,16,237,50,48,49,51,53,88,152,5,100,96,5,91,207,17,35,46,3,39,37,77,206,6,36,5,14,3,1,5,70,206,12,32,3,70,201,12,32,21,85,141,26,54,61,50,34,76,80,
    83,42,254,100,160,254,24,160,254,100,42,83,80,75,3,92,1,191,65,67,6,85,146,12,130,19,70,213,33,65,87,14,33,1,222,65,65,14,35,254,158,1,98,65,65,16,36,110,0,0,5,81,74,15,8,43,69,64,
    36,3,70,238,49,54,75,238,44,44,77,222,5,39,10,6,4,238,8,10,10,16,77,218,22,42,18,57,47,205,253,206,17,51,47,237,57,131,4,77,219,9,32,19,68,174,6,32,3,85,165,33,32,1,85,165,11,32,19,
    85,165,11,53,110,5,100,88,123,156,160,160,153,118,97,92,6,164,4,80,90,51,105,109,118,64,92,4,15,44,64,118,110,105,52,48,64,40,18,2,1,61,165,71,20,5,42,65,107,98,95,54,1,52,94,98,108,
    66,92,85,7,46,162,89,114,24,1,156,170,1,240,162,1,143,22,116,94,77,223,14,35,58,83,105,62,92,24,6,85,174,11,41,14,17,16,3,252,78,2,16,18,14,85,174,8,32,0,79,13,5,65,51,5,49,72,0,90,
    0,77,64,40,3,76,238,49,54,81,238,44,44,54,34,77,226,14,32,16,77,222,22,34,18,57,47,77,219,10,65,59,59,71,50,12,33,7,19,24,76,19,13,65,64,49,47,110,160,158,166,41,75,108,66,65,107,98,
    95,54,225,107,117,65,67,8,33,166,158,65,64,45,49,162,158,14,109,99,47,78,55,31,14,17,16,3,253,47,170,254,117,65,70,7,35,106,107,11,166,65,69,12,49,75,0,96,0,77,64,40,3,81,238,49,54,
    86,238,44,44,54,34,77,228,7,34,10,7,5,66,125,6,38,70,238,24,24,16,12,75,79,36,13,45,18,57,47,205,253,205,17,51,47,51,51,237,50,50,65,69,59,77,223,16,32,1,24,76,101,16,65,75,49,40,79,
    23,78,45,41,71,23,115,119,65,80,8,37,1,84,50,81,20,71,65,84,9,35,115,110,23,76,65,86,45,38,38,48,38,32,23,104,83,65,88,8,34,118,54,45,65,89,9,37,88,102,22,38,45,0,69,23,5,33,5,71,67,
    213,11,37,20,70,238,55,55,43,77,226,5,38,38,0,42,56,238,35,38,77,230,10,33,51,86,77,230,16,39,16,222,237,50,17,18,57,47,24,88,251,9,77,231,33,45,3,51,53,51,17,35,53,35,19,22,22,23,
    1,19,85,219,24,42,163,2,18,40,64,48,52,105,110,117,65,85,219,12,8,39,121,173,106,65,117,109,105,51,90,80,4,164,6,92,97,118,153,160,160,156,123,88,100,5,254,31,115,54,95,98,107,65,66,
    108,75,41,223,206,162,162,71,129,7,36,66,108,98,94,52,85,224,10,35,62,105,83,58,92,200,14,53,94,116,22,254,113,162,254,16,170,254,100,24,114,89,3,35,1,131,3,16,17,14,85,227,14,62,14,
    18,16,2,0,1,0,100,0,0,4,222,5,154,0,11,0,22,64,11,8,11,5,2,4,7,10,18,3,0,3,24,108,232,8,35,19,51,1,1,130,2,8,40,35,1,1,35,1,100,193,1,124,1,124,193,254,39,1,217,206,254,145,254,145,
    206,1,217,5,154,253,196,2,60,253,46,253,56,2,47,253,209,2,200,0,133,83,35,2,180,2,205,130,83,61,23,64,11,1,10,4,7,4,2,8,6,0,2,18,0,63,51,47,51,18,23,57,48,49,33,39,7,35,19,3,24,189,
    96,18,34,241,241,1,24,189,94,8,101,189,6,32,37,130,157,36,39,1,191,2,76,130,90,24,149,27,33,32,6,24,207,75,10,100,25,6,33,7,234,132,65,34,221,4,17,132,65,41,233,0,0,0,37,64,17,2,1,
    80,24,139,189,16,37,180,26,26,12,12,37,24,139,26,8,134,63,33,10,32,132,63,34,223,6,71,132,63,32,234,130,63,41,43,64,21,3,2,1,128,26,1,64,147,67,33,2,2,135,67,33,93,53,24,138,183,8,
    38,10,32,7,66,0,39,1,132,69,34,0,38,2,130,69,37,1,7,1,78,7,245,130,77,41,60,64,28,4,0,115,125,7,7,37,151,84,35,64,9,26,26,130,85,37,4,130,5,38,0,43,25,9,224,7,131,92,24,139,189,7,33,
    8,126,132,163,34,225,4,165,136,163,34,16,64,9,130,78,36,0,69,80,13,14,130,142,130,136,65,79,5,33,8,158,132,135,132,41,138,135,32,190,130,135,34,32,64,20,131,135,33,0,1,131,135,133,
    56,136,113,130,63,135,107,65,15,6,32,228,65,15,40,33,24,24,65,15,25,132,69,137,135,33,6,139,65,15,9,33,24,25,65,15,28,130,85,65,15,25,35,4,201,5,154,130,85,35,251,0,0,1,24,217,59,14,
    24,216,203,14,36,100,0,0,5,70,132,43,32,252,133,43,50,4,102,251,251,0,17,64,9,1,100,17,17,12,12,37,1,32,0,47,25,202,95,7,132,43,33,7,66,130,87,130,43,35,0,39,1,78,131,43,32,1,130,95,
    33,1,8,65,61,5,38,2,0,37,47,9,10,37,134,58,24,214,173,18,24,216,109,9,32,254,145,153,33,7,7,143,109,33,4,201,132,109,130,43,24,195,27,8,130,109,33,2,193,137,109,33,1,2,132,109,130,
    58,149,109,133,219,32,3,130,48,32,1,130,57,139,219,33,4,4,143,109,33,5,70,131,109,131,43,139,219,33,0,197,137,109,33,14,15,132,109,130,58,143,109,25,8,203,10,57,19,0,49,64,22,10,13,
    15,8,11,1,0,3,18,5,1,16,15,18,1,12,11,2,5,8,6,24,125,36,7,43,196,221,196,205,1,47,196,196,222,196,16,221,131,5,36,49,48,19,33,53,131,1,34,51,21,33,134,1,46,35,53,33,100,1,40,254,216,
    1,40,124,1,41,254,215,131,3,8,43,124,254,216,3,245,124,124,173,173,124,124,124,172,172,0,0,2,0,100,3,121,3,49,4,237,0,3,0,7,0,21,183,5,2,3,4,7,5,1,3,0,47,205,222,131,94,33,222,196,
    131,87,133,79,36,100,2,205,253,51,131,3,33,4,237,130,61,67,215,6,47,5,254,7,166,2,38,1,89,0,0,1,7,1,79,2,93,130,231,45,23,64,13,3,2,0,27,37,1,2,37,3,2,42,24,184,79,10,25,24,239,12,
    32,94,134,49,32,33,171,49,37,0,39,1,79,0,215,132,57,34,78,3,187,130,57,39,38,64,24,4,0,47,57,37,131,106,41,175,27,27,1,1,37,4,62,5,38,136,118,36,43,53,1,43,53,24,196,229,8,35,6,232,
    7,166,130,65,34,94,1,38,130,57,39,39,1,78,0,10,251,251,1,130,181,33,3,77,130,15,35,68,64,30,4,130,246,32,57,130,181,33,2,96,25,35,23,22,35,64,12,17,17,131,98,32,3,130,99,35,2,22,0,
    47,130,89,68,71,8,32,93,131,12,68,29,5,65,19,6,32,135,65,19,13,40,2,1,0,27,37,6,0,37,2,25,25,209,12,65,19,12,32,140,166,49,34,10,0,0,134,203,33,135,0,24,171,203,13,34,79,3,71,130,203,
    39,40,64,9,3,2,0,47,57,130,109,35,1,184,255,156,131,182,36,6,6,37,3,2,130,182,32,1,136,182,144,175,32,169,144,125,35,46,56,23,23,24,255,213,14,133,225,39,3,217,7,166,2,38,1,198,133,
    49,33,1,74,65,195,8,35,56,66,1,1,130,104,32,71,65,245,13,130,175,37,4,195,7,166,0,39,130,49,142,175,33,2,52,130,59,39,60,64,24,4,3,0,76,86,130,59,33,2,64,25,6,145,16,45,64,12,46,46,
    17,17,37,4,3,91,5,38,2,51,65,117,12,133,195,35,255,230,0,0,134,145,32,254,130,145,34,6,1,79,130,16,41,23,64,13,2,1,0,26,36,1,2,130,193,32,41,141,143,42,100,0,0,5,254,5,129,2,38,2,24,
    133,193,46,3,106,253,219,0,37,64,23,3,2,0,56,66,6,6,130,193,33,95,71,24,240,65,23,65,1,5,134,63,32,27,133,63,33,1,78,138,63,33,12,12,169,63,32,46,141,63,130,177,33,46,56,130,63,35,
    2,1,95,61,24,233,197,23,133,127,35,3,133,7,166,130,191,32,69,133,63,33,0,250,24,225,155,11,32,16,130,241,32,52,144,241,134,49,32,72,146,49,33,0,8,146,49,37,255,235,7,89,5,129,130,99,
    32,103,133,49,33,3,11,130,227,34,21,64,11,130,163,35,88,98,25,58,130,49,38,103,0,47,53,53,1,43,71,157,6,136,47,32,105,144,47,35,90,100,10,45,130,47,32,105,139,47,42,10,255,235,8,67,
    5,129,0,39,2,103,66,195,16,32,244,130,105,39,38,64,9,3,2,0,108,118,130,105,47,1,184,255,156,64,10,78,78,25,25,37,3,2,123,1,83,130,70,133,120,130,122,65,13,12,32,126,133,121,33,0,248,
    65,13,8,35,29,39,2,4,130,121,32,44,65,13,23,32,129,145,49,33,29,4,146,49,42,99,0,0,4,66,7,166,2,38,2,159,133,49,33,1,182,136,99,35,48,58,12,12,130,99,32,63,141,99,38,10,0,0,6,232,3,
    117,130,223,32,22,131,223,32,1,24,185,71,7,34,40,64,16,66,227,18,39,182,46,46,11,11,37,2,51,130,218,73,57,7,133,217,33,7,66,130,67,34,38,2,22,132,117,34,78,6,98,24,238,229,8,34,36,
    17,17,135,47,24,239,197,9,33,5,29,135,111,32,0,68,223,9,47,78,2,153,253,219,0,54,64,23,3,0,56,66,16,16,37,146,126,33,64,9,132,127,33,3,71,132,129,32,47,68,212,7,32,53,72,173,7,33,7,
    66,130,89,132,133,131,87,131,133,32,1,130,207,33,1,174,130,87,34,30,64,18,135,87,33,100,36,131,148,138,68,24,231,169,15,32,24,65,9,37,33,18,18,65,9,26,67,177,5,65,9,12,65,57,9,65,9,
    14,132,111,65,9,11,33,4,189,65,9,9,33,6,6,65,9,23,130,127,65,9,31,130,133,65,9,11,33,3,211,65,9,9,131,87,34,100,36,36,130,148,65,9,22,36,251,0,39,2,151,65,9,9,38,252,185,0,40,64,16,
    1,24,195,34,17,39,182,38,38,0,0,37,1,43,66,19,13,66,87,5,131,67,32,154,165,67,33,15,15,149,67,33,5,29,130,135,132,67,65,33,5,39,252,185,1,7,1,78,2,201,130,201,41,66,64,23,2,0,48,58,
    20,20,37,146,150,35,64,18,38,38,130,83,42,2,159,63,1,95,63,1,31,63,1,63,132,162,35,47,93,93,93,66,55,13,130,169,35,5,45,5,154,130,101,32,156,65,247,15,146,86,130,237,33,1,1,147,169,
    35,5,45,7,66,135,67,66,213,12,36,206,0,0,0,56,133,169,130,143,146,82,35,64,11,38,38,130,83,32,2,24,196,99,22,131,159,32,44,132,159,32,159,165,159,33,7,7,141,159,73,175,10,8,84,6,0,
    55,64,29,7,5,1,0,6,6,5,3,4,4,2,1,5,220,5,1,169,5,185,5,201,5,3,5,1,4,6,1,3,0,63,221,196,18,57,93,93,1,25,47,51,51,51,24,47,205,17,51,47,205,49,48,93,19,1,51,1,35,3,3,100,1,97,10,1,
    98,135,224,223,2,205,2,205,253,51,1,191,254,65,0,138,95,8,33,25,0,49,64,30,122,20,138,20,154,20,3,148,16,1,117,16,133,16,2,168,7,1,167,3,1,0,24,11,13,18,5,24,11,130,96,38,196,220,205,
    1,47,205,222,131,85,32,93,130,0,32,1,24,173,112,20,33,3,49,24,130,10,7,42,126,36,63,84,49,49,85,63,36,126,4,24,130,178,8,50,1,103,254,153,47,88,67,40,40,67,88,47,1,103,0,0,2,0,100,
    25,10,51,13,46,12,25,18,2,19,18,20,20,1,30,0,1,1,1,7,130,117,37,204,93,205,17,57,47,131,121,32,196,131,122,34,19,35,17,24,188,42,12,24,90,213,9,37,34,14,2,21,224,124,24,138,245,7,24,
    142,80,18,38,205,1,240,46,80,60,35,135,33,32,124,24,142,142,7,35,19,28,34,16,65,11,5,49,2,30,5,154,0,23,0,23,64,9,23,0,13,11,23,5,12,18,131,128,33,221,204,134,246,32,1,24,130,240,9,
    136,132,33,1,162,24,139,115,8,135,88,37,4,189,19,35,27,16,131,85,33,254,16,137,114,24,250,187,14,37,1,0,3,4,2,0,130,94,32,205,131,218,32,205,75,6,5,44,17,35,100,1,186,254,194,124,5,
    154,124,253,175,65,29,6,48,3,49,4,135,0,18,0,35,0,28,64,11,11,24,34,17,0,130,172,39,35,6,29,0,47,205,222,196,130,58,35,221,205,16,222,130,61,33,1,21,24,193,86,11,33,33,21,88,82,11,
    35,53,53,2,30,65,22,11,35,1,240,254,16,24,166,36,11,33,4,11,24,162,107,15,37,38,28,21,35,26,14,131,216,32,98,66,37,6,8,40,2,30,5,154,0,16,0,53,64,32,229,13,245,13,2,246,12,1,231,12,
    1,186,5,1,203,4,219,4,2,170,4,1,9,7,0,15,15,2,2,0,9,24,227,57,19,32,19,24,233,109,14,58,82,44,85,66,41,124,38,78,120,82,2,205,1,165,21,49,81,60,85,85,68,117,88,55,7,254,215,65,37,5,
    33,3,92,130,109,45,39,0,24,64,10,26,7,26,7,20,11,0,31,20,130,87,55,205,220,205,18,57,57,47,47,48,49,1,34,46,2,53,53,51,21,20,22,51,50,54,53,65,139,8,59,21,35,53,52,38,35,34,6,21,17,
    20,14,2,1,65,53,82,57,29,124,54,42,53,45,29,57,82,53,139,12,49,2,205,35,60,81,45,49,49,50,57,57,50,1,19,45,81,60,35,137,15,46,254,247,45,84,64,38,0,0,3,0,10,2,205,3,183,130,135,48,
    3,0,23,0,26,0,22,64,9,4,14,14,2,24,0,26,2,130,138,33,51,221,130,138,40,47,205,48,49,19,1,51,1,37,131,141,66,148,10,42,5,33,3,234,1,97,10,1,98,252,195,24,123,235,14,36,1,32,1,62,159,
    67,129,5,36,246,18,30,41,23,139,30,34,122,1,66,131,117,36,100,2,205,4,17,163,117,32,55,145,117,67,228,5,32,112,142,116,33,253,115,157,116,38,2,0,100,3,242,2,12,130,115,48,19,0,39,0,
    25,64,11,30,10,175,20,0,35,5,175,15,25,130,233,42,221,254,205,1,47,205,254,205,49,48,19,89,129,10,36,35,34,14,2,7,138,242,32,35,130,140,32,200,143,209,33,100,34,25,55,102,14,33,4,198,
    142,154,33,20,43,25,55,102,14,38,1,0,100,4,186,1,68,132,139,39,16,182,10,175,0,15,175,5,130,132,35,237,1,47,237,130,128,142,112,32,100,143,112,33,5,42,142,95,72,227,8,46,251,0,38,2,
    151,0,0,1,7,1,78,6,98,252,185,80,59,5,35,28,28,14,14,69,113,7,33,53,0,130,43,42,70,0,0,4,67,7,66,2,38,2,158,130,43,34,6,1,78,130,16,40,19,64,11,1,0,28,38,27,0,24,203,55,11,130,43,32,
    99,130,43,32,97,132,43,32,161,130,43,130,87,36,3,129,0,0,0,151,45,36,100,0,0,6,162,130,213,34,38,2,178,133,45,33,5,194,73,105,7,38,57,57,7,7,37,2,72,80,193,12,33,7,66,132,43,32,186,
    73,149,15,35,57,57,24,24,143,43,134,87,32,191,143,87,38,30,30,12,12,37,2,45,140,87,33,5,254,132,177,32,193,133,43,33,2,199,133,177,37,2,0,30,40,13,13,130,43,24,188,251,15,133,45,32,
    194,134,45,32,187,137,45,132,89,143,45,32,29,132,179,32,218,133,45,33,4,61,135,223,38,39,39,19,19,37,2,54,140,135,33,3,217,132,135,32,220,133,43,32,1,130,143,41,0,19,64,11,2,0,39,49,
    18,1,24,171,131,11,65,191,5,33,5,79,132,89,32,235,133,45,33,4,111,133,89,40,1,100,52,52,25,25,37,1,67,140,89,33,4,41,132,89,32,237,133,43,33,3,73,133,179,36,1,0,52,62,24,131,43,139,
    179,62,95,255,232,5,128,7,66,16,38,3,22,0,0,16,7,1,78,4,160,0,0,255,255,0,117,255,232,8,13,5,154,130,23,32,67,132,23,34,123,6,201,132,23,32,145,130,47,32,4,132,47,32,69,132,23,34,78,
    2,24,132,23,36,100,255,232,4,215,132,23,32,72,134,23,32,127,135,23,35,6,22,5,178,130,71,133,23,34,123,4,210,132,23,32,130,130,71,32,117,132,47,32,105,134,47,32,88,135,23,33,7,207,132,
    47,32,111,132,23,34,123,6,139,130,189,8,37,1,0,100,2,205,2,5,6,198,0,9,0,30,64,12,8,4,6,2,0,6,6,1,4,7,0,4,0,47,220,205,16,221,196,1,47,221,206,130,6,34,49,48,1,98,175,6,49,35,53,1,
    114,147,254,95,146,146,6,198,252,131,124,124,3,1,124,69,85,5,8,36,2,91,5,164,0,25,0,31,64,13,16,14,0,12,0,5,25,17,20,20,13,15,3,0,63,205,51,47,51,51,205,50,1,47,206,205,196,130,70,
    33,38,39,123,35,5,36,7,6,7,17,35,130,83,88,129,5,8,49,22,23,2,91,21,26,22,58,33,39,61,45,32,11,26,5,124,124,34,103,75,38,61,22,26,20,5,6,9,8,6,11,15,26,32,17,40,49,254,88,2,205,100,
    45,65,8,5,6,7,0,131,117,8,46,1,102,2,186,4,51,0,35,0,29,64,12,9,23,18,5,0,26,26,18,1,0,19,18,0,47,205,220,205,18,57,47,18,57,18,57,51,48,49,19,53,33,50,54,53,52,38,108,147,5,8,90,62,
    2,51,33,21,33,34,6,21,20,22,51,51,50,22,21,20,14,2,35,121,1,29,79,82,70,75,70,121,131,38,69,97,59,1,44,254,238,75,83,64,61,82,124,136,39,72,101,63,1,102,119,48,47,42,40,102,95,50,83,
    59,32,120,50,45,40,41,105,95,50,82,58,31,255,255,0,112,0,178,1,80,4,222,2,7,0,29,0,0,0,200,130,147,8,48,80,0,0,4,192,5,178,0,33,0,28,64,14,16,145,14,18,18,13,0,145,33,4,13,145,21,18,
    0,63,237,63,237,17,57,47,51,237,48,49,1,34,14,4,21,20,30,2,51,51,94,167,5,33,17,33,69,112,5,8,103,4,51,3,140,80,159,145,124,92,52,40,94,158,119,241,140,1,192,140,254,103,151,221,145,
    70,61,109,153,184,210,111,5,27,55,103,147,183,215,120,62,119,93,58,244,144,144,254,116,71,127,178,108,135,250,217,177,126,69,0,2,0,56,254,183,4,12,4,24,0,46,0,74,0,54,64,28,37,36,46,
    40,30,3,27,0,20,19,57,149,55,72,4,0,55,0,55,64,47,149,27,22,64,149,12,16,134,139,43,57,47,47,57,57,16,237,57,57,17,18,23,130,152,50,48,49,19,23,22,22,23,17,52,62,4,51,50,30,2,21,20,
    7,21,65,31,5,46,34,38,39,21,20,14,2,7,39,54,53,53,38,38,39,130,2,43,5,50,62,2,53,52,33,35,53,51,50,53,72,184,5,8,167,4,21,17,22,22,167,21,11,31,23,14,35,59,89,122,82,68,127,98,59,180,
    210,75,121,153,77,54,97,43,9,18,27,18,141,50,51,72,25,14,25,10,2,50,44,89,72,45,254,246,72,68,244,32,54,70,38,55,77,51,30,15,4,42,100,1,89,30,14,35,19,1,96,54,109,100,86,64,36,32,68,
    105,73,163,67,4,48,202,83,117,75,35,13,11,33,41,83,78,69,25,65,75,152,102,28,59,24,14,27,14,117,18,40,65,47,172,142,160,41,58,36,17,31,50,65,69,68,28,254,62,16,19,0,1,0,90,255,232,
    4,209,4,0,0,61,0,42,64,21,31,37,25,32,32,25,21,5,5,11,49,44,149,46,15,25,21,11,149,0,22,131,250,38,63,237,50,17,57,47,57,130,3,34,18,57,51,123,185,6,35,51,20,30,2,93,145,5,49,4,39,
    6,6,7,1,35,46,3,39,55,30,3,23,1,62,3,53,65,164,5,8,118,21,20,30,4,21,20,14,2,3,152,53,95,70,41,153,15,27,39,25,79,66,35,55,65,62,50,11,15,47,34,254,201,168,3,25,35,41,18,137,10,24,
    25,21,7,1,28,14,33,29,19,170,1,248,170,56,83,98,83,56,56,89,112,24,34,62,88,54,18,35,29,18,92,99,42,71,61,52,48,45,22,50,86,56,254,58,33,97,108,105,40,49,23,64,69,71,31,1,161,22,55,
    61,67,34,54,140,140,63,34,63,67,75,92,115,72,99,126,74,28,0,130,213,8,36,166,0,0,3,218,4,24,0,39,0,31,64,15,37,34,149,20,23,23,30,38,21,9,14,149,8,5,16,0,63,51,237,50,63,51,57,47,130,
    6,35,48,49,19,52,97,229,5,32,21,90,134,6,8,83,21,54,54,51,50,30,2,21,17,35,17,52,38,35,34,6,7,17,35,166,50,108,171,122,67,151,66,31,74,75,73,30,82,111,67,28,86,167,96,71,114,79,43,
    163,84,96,75,153,85,163,2,156,76,138,105,61,27,28,152,16,25,18,9,35,63,88,52,139,40,51,38,75,111,73,254,186,1,41,100,91,40,38,254,102,131,145,36,96,255,232,5,107,130,145,50,54,0,53,
    64,28,46,38,41,3,47,42,39,15,45,47,13,149,16,47,130,1,42,10,0,149,32,16,12,10,149,17,22,22,133,158,42,237,17,57,57,47,47,16,237,17,51,63,89,255,5,24,150,128,8,40,50,55,53,35,53,33,
    17,6,7,119,253,5,96,123,5,38,22,23,22,22,23,55,51,130,2,8,108,3,35,39,7,35,38,39,46,3,2,61,78,117,76,38,48,93,135,88,107,78,223,1,131,54,60,51,131,67,119,191,135,73,67,124,178,111,
    90,121,38,22,35,13,66,155,69,70,132,126,153,75,74,148,19,32,14,36,46,57,3,142,63,109,147,85,87,145,103,57,38,190,139,254,75,19,15,13,21,73,136,192,119,119,202,148,83,46,27,17,36,20,
    122,161,161,254,240,168,168,45,34,15,28,22,14,0,0,3,0,166,130,4,58,209,4,0,0,12,0,19,0,26,0,28,64,14,7,15,149,20,20,0,19,149,1,15,22,149,0,21,67,51,6,44,47,237,57,48,49,51,17,33,50,
    22,21,20,7,130,3,39,6,35,1,17,51,50,53,52,130,6,32,33,131,6,8,52,166,1,190,151,175,196,235,193,143,254,201,246,198,178,254,246,1,28,199,223,4,0,134,116,199,43,30,207,125,170,3,116,
    254,224,154,134,254,84,254,196,161,155,0,0,1,0,90,0,0,4,16,4,24,68,47,6,43,149,14,18,18,13,0,149,33,16,13,149,21,136,111,68,47,29,8,57,2,220,21,86,103,109,90,57,23,61,107,84,191,140,
    1,192,140,254,153,96,165,121,69,53,92,125,142,154,76,3,129,21,49,77,112,149,95,40,86,70,46,244,144,144,254,116,47,97,148,102,95,171,145,117,82,44,0,2,131,239,43,4,6,4,0,0,7,0,14,0,
    16,183,14,130,230,32,10,135,230,132,225,8,41,32,17,20,0,35,3,17,51,32,17,16,33,166,1,64,2,32,254,213,252,151,143,1,137,254,123,4,0,254,10,235,254,225,3,116,253,24,1,122,1,110,0,130,
    75,35,255,242,4,67,130,75,55,41,0,49,0,35,64,18,44,149,18,17,3,3,49,31,25,149,36,22,49,149,6,15,4,130,205,40,63,237,63,253,204,18,57,47,51,132,208,39,38,35,35,17,35,17,33,50,125,27,
    6,37,21,22,22,23,23,22,114,61,5,32,51,76,109,6,8,111,39,1,17,51,50,54,53,52,35,2,46,62,120,48,162,1,92,82,130,89,47,38,69,97,60,33,58,29,61,10,33,36,22,27,15,4,165,23,55,90,66,58,81,
    55,34,11,254,222,156,103,111,196,1,20,146,254,90,4,0,39,74,105,66,59,96,71,46,10,4,13,66,63,135,19,34,14,22,27,13,38,77,61,38,28,43,53,25,2,237,254,189,83,85,155,0,0,1,0,30,0,0,3,41,
    4,0,0,7,0,15,182,1,4,130,167,32,3,132,167,36,50,48,49,1,33,131,157,50,53,33,3,41,254,205,164,254,204,3,11,3,116,252,140,3,116,140,0,130,235,36,80,255,232,4,112,130,53,8,42,53,0,73,
    0,40,64,20,54,28,46,46,49,25,149,2,1,50,149,52,15,64,36,36,15,149,14,22,0,63,237,51,47,205,63,253,50,221,237,51,51,47,51,205,130,80,42,35,21,30,3,21,20,14,4,35,53,50,126,48,6,34,34,
    6,7,24,93,186,14,38,54,54,55,53,35,53,33,24,196,250,12,8,126,52,46,2,3,162,170,91,141,95,49,33,57,75,84,88,44,42,77,59,35,44,72,93,49,67,95,33,58,51,30,66,103,73,73,103,66,30,29,65,
    102,74,46,127,73,170,1,248,253,190,36,51,32,15,15,32,51,36,36,55,37,18,18,37,55,3,116,108,8,68,108,142,82,69,112,87,63,41,20,138,34,64,92,59,70,105,68,34,37,35,45,144,88,63,115,86,
    51,51,86,115,63,66,118,90,53,67,71,13,115,140,253,245,31,55,75,43,44,74,55,31,31,55,74,44,44,75,54,31,130,239,54,96,255,232,5,126,4,169,0,40,0,54,0,31,64,16,49,149,33,22,26,23,149,
    6,130,231,36,14,41,149,0,16,132,235,33,237,222,107,3,5,34,1,50,22,65,198,7,33,52,38,130,234,70,63,8,131,233,8,111,7,6,35,34,39,38,53,16,55,54,23,34,7,6,21,20,23,22,51,50,54,53,52,38,
    2,106,118,179,59,36,79,46,22,44,35,22,71,58,133,144,39,72,100,60,14,24,12,9,10,139,138,233,226,137,135,141,141,228,159,91,92,93,93,156,160,170,170,4,24,72,68,42,46,12,28,46,35,57,57,
    138,131,121,59,95,69,37,2,2,40,84,45,245,148,147,143,144,237,1,1,146,145,138,108,107,189,182,105,105,205,191,193,207,130,187,55,48,0,0,1,240,4,24,0,7,0,27,0,20,64,9,7,8,18,16,6,1,149,
    4,21,130,180,37,50,63,221,206,48,49,72,215,7,76,241,15,62,1,100,140,254,64,140,86,22,39,28,17,17,28,39,22,22,40,29,17,17,29,40,2,162,253,238,144,144,2,18,161,25,157,63,19,36,20,255,
    232,3,81,130,107,55,48,0,38,64,19,2,25,149,28,48,48,15,39,41,149,38,36,22,13,15,149,12,10,16,69,10,5,35,51,237,50,18,69,172,7,35,53,51,38,38,68,241,5,36,23,21,38,35,34,109,47,5,35,
    23,33,21,35,66,23,7,34,39,53,22,71,96,5,8,86,39,20,141,28,27,66,110,141,75,134,104,113,147,46,74,53,29,26,48,71,45,1,129,130,31,33,67,112,147,79,159,115,133,162,216,33,57,78,45,1,190,
    140,32,80,54,72,111,75,38,46,166,74,21,38,53,32,37,52,41,32,18,140,33,84,55,76,112,74,36,61,176,99,144,39,55,43,34,17,0,0,2,0,70,255,232,4,48,132,173,53,68,0,36,64,18,15,54,18,64,149,
    30,28,5,5,28,18,149,44,22,28,149,34,72,35,7,38,47,18,57,237,17,57,57,70,90,7,67,175,6,67,170,6,38,52,46,2,35,34,7,53,70,100,6,67,183,6,36,55,20,30,2,23,24,96,171,9,8,143,70,32,67,104,
    72,62,94,63,32,46,67,78,32,36,98,68,84,124,81,40,40,80,120,80,122,110,48,123,69,123,181,118,58,66,128,185,118,119,187,130,69,152,13,20,24,11,24,59,52,35,14,28,42,28,33,48,31,14,1,200,
    54,110,89,56,43,74,98,55,68,104,78,55,18,28,30,57,103,147,90,90,146,105,57,61,158,20,22,85,145,193,108,116,198,145,82,76,130,175,101,36,61,51,40,15,12,38,58,78,52,30,55,44,26,34,54,
    68,0,1,0,76,0,0,3,117,4,0,0,57,0,51,64,26,48,150,13,10,13,30,30,13,24,150,35,13,130,1,39,3,9,4,149,6,15,56,3,68,238,5,35,50,63,237,50,130,225,34,47,47,16,130,232,48,47,17,51,16,237,
    48,49,33,33,53,51,17,35,53,33,21,35,71,65,7,35,20,6,21,20,68,159,5,32,55,81,13,10,8,98,53,52,38,35,34,14,4,21,17,51,2,68,254,8,170,170,1,248,170,32,61,14,44,57,32,12,20,17,23,19,27,
    16,8,133,29,57,85,56,47,59,35,13,7,8,7,15,20,19,27,18,10,5,2,170,140,2,232,140,140,153,20,11,30,46,56,26,40,72,32,27,33,24,37,48,24,1,54,93,67,39,28,45,56,28,20,40,39,35,15,24,29,32,
    49,59,55,41,8,255,0,130,203,54,33,255,232,3,223,4,0,0,26,0,24,64,11,24,18,26,20,15,10,13,149,9,6,71,92,6,45,51,57,57,48,49,1,1,14,3,35,34,38,39,53,65,153,5,8,51,55,1,51,1,22,22,23,
    51,1,3,223,254,145,39,85,99,118,72,44,73,31,35,72,38,31,63,62,57,25,254,99,182,1,24,8,17,6,1,1,35,4,0,253,18,80,113,72,33,13,11,152,19,130,143,51,56,45,3,1,253,220,14,38,18,2,106,0,
    0,2,0,10,0,0,3,182,69,31,5,131,121,8,60,15,149,2,2,4,12,6,15,1,4,21,0,63,51,63,51,18,57,47,237,48,49,33,35,3,33,3,35,1,51,19,3,38,39,35,6,7,3,3,182,180,88,254,102,86,176,1,117,195,
    59,135,2,18,7,5,16,136,1,2,254,254,130,105,41,142,1,140,6,78,26,58,254,116,0,130,211,50,70,255,232,2,86,4,0,0,12,0,17,183,12,15,6,8,149,5,3,134,207,130,204,35,20,2,35,34,130,202,58,
    51,50,17,17,51,2,86,203,189,70,66,74,66,224,164,1,245,252,254,239,32,153,45,1,102,2,38,73,21,5,33,2,240,130,65,48,11,0,26,64,13,7,149,9,9,1,5,149,2,15,10,149,1,71,58,8,131,157,35,33,
    17,33,21,134,3,55,2,240,253,182,2,51,254,111,1,116,254,140,1,168,4,0,140,254,212,140,254,208,0,3,131,231,33,5,22,130,73,8,56,33,0,36,0,56,0,53,64,28,37,149,26,32,5,149,11,34,7,11,47,
    149,16,26,11,16,16,11,26,3,35,9,21,4,33,149,1,15,0,63,237,50,63,51,23,57,47,47,47,16,237,17,51,51,16,237,50,16,130,104,36,1,53,33,21,35,130,107,37,1,35,17,35,14,3,122,6,7,41,51,50,
    30,2,23,51,17,19,17,19,69,212,15,8,42,2,24,1,186,140,1,208,254,84,198,113,8,41,69,98,65,73,103,66,30,33,70,107,74,60,93,66,41,8,114,162,242,252,232,36,53,35,16,17,34,53,36,36,130,7,
    8,39,16,35,53,3,116,140,140,254,191,107,254,56,1,174,56,95,70,40,51,86,115,63,66,119,90,52,39,67,91,51,1,65,254,58,254,253,1,3,1,8,69,190,16,8,35,0,1,0,110,0,0,3,152,4,24,0,48,0,41,
    64,21,15,149,33,33,20,27,149,30,30,43,21,26,149,23,21,48,43,149,0,5,130,210,32,51,130,211,40,237,50,17,57,47,237,50,51,47,130,205,34,19,54,55,67,212,11,35,38,39,38,39,78,0,8,32,33,
    67,252,11,8,93,6,7,6,7,200,32,44,38,111,72,100,157,109,57,48,84,116,68,34,57,22,26,21,170,254,8,170,170,1,78,47,67,33,34,58,44,25,47,77,99,52,64,102,36,42,33,3,224,16,12,11,17,55,95,
    129,74,82,121,79,39,14,9,10,13,254,232,140,140,1,31,140,35,20,16,41,71,56,54,81,53,26,20,12,14,17,0,0,2,0,10,255,232,4,100,130,181,50,42,0,53,0,30,64,15,49,149,35,35,43,2,29,149,40,
    24,22,43,76,192,6,69,73,5,130,172,41,55,22,51,50,62,2,55,62,3,51,72,78,5,32,4,66,60,6,130,19,34,33,14,3,130,12,8,104,1,34,14,2,7,33,46,3,10,42,46,64,80,51,33,17,16,62,94,125,80,86,
    148,108,62,12,32,56,86,121,81,61,45,40,47,49,82,60,38,6,254,37,20,49,80,120,92,60,46,2,200,50,73,53,34,10,1,194,6,38,60,79,134,21,84,141,184,101,94,156,113,62,69,135,200,131,43,113,
    119,115,91,56,15,143,21,47,86,123,75,106,173,122,67,15,3,152,58,93,117,59,81,122,83,41,0,66,149,6,52,188,4,0,0,7,0,20,64,9,3,149,5,5,1,7,21,1,15,0,63,63,67,45,5,52,51,17,51,17,33,21,
    33,17,166,162,1,116,254,140,4,0,254,72,140,254,68,134,55,46,3,4,4,0,0,5,0,13,181,1,149,4,15,2,21,130,51,130,220,72,137,5,41,3,4,254,68,162,2,94,3,116,252,130,51,65,29,5,45,5,17,4,24,
    0,56,0,76,0,42,64,21,56,21,130,50,53,4,51,57,149,38,16,67,48,28,26,51,149,15,12,9,22,0,63,51,51,237,50,130,0,51,63,237,17,57,47,237,51,51,48,49,1,53,33,21,35,20,14,2,35,34,111,94,5,
    34,46,2,53,76,247,5,124,174,10,32,21,130,31,32,7,65,226,6,66,180,7,69,206,6,8,156,3,212,1,61,78,38,83,133,94,74,128,57,56,126,71,97,133,83,36,160,30,52,67,38,63,54,51,77,53,26,44,87,
    130,87,88,131,87,42,28,54,79,51,29,60,33,39,69,50,29,254,67,53,70,42,17,24,46,67,44,44,68,47,24,17,41,70,1,64,140,140,59,123,99,63,30,27,27,30,63,99,123,59,47,76,54,30,16,49,119,134,
    143,72,74,144,114,70,70,114,144,74,72,143,133,119,49,8,9,30,54,76,47,2,78,52,77,90,38,67,128,116,99,37,37,99,116,128,67,38,90,77,52,0,1,0,24,0,0,5,176,4,0,0,27,0,23,64,10,27,12,6,19,
    15,23,15,2,9,21,131,238,32,51,130,242,131,230,33,1,35,68,140,6,41,35,1,51,19,22,23,51,54,55,19,135,7,61,5,176,254,205,170,211,12,4,4,3,18,229,164,254,202,172,212,10,4,8,3,15,236,150,
    212,10,5,8,2,15,25,245,144,31,8,35,2,0,86,255,232,3,219,4,24,0,44,0,64,0,32,64,16,32,37,149,26,50,149,18,15,15,31,26,16,60,149,5,22,0,63,237,77,140,5,36,16,237,50,48,49,88,198,8,77,
    150,6,71,238,7,49,22,23,22,23,21,38,39,38,38,35,34,6,21,20,30,2,7,52,24,81,173,9,8,107,51,50,62,2,3,93,57,102,142,86,85,142,103,58,57,103,142,86,38,68,22,6,15,39,69,94,56,26,53,21,
    25,23,20,23,20,50,27,50,55,37,45,37,167,34,59,81,47,51,81,57,31,31,58,81,50,49,81,59,32,1,90,82,136,98,54,54,98,136,82,81,136,98,55,10,10,25,69,31,58,86,56,27,10,5,6,8,161,15,11,10,
    16,51,47,64,110,108,115,69,52,85,61,33,33,61,85,52,52,86,131,7,41,86,0,0,1,0,143,0,0,3,126,130,213,49,31,0,23,64,11,31,149,0,0,14,10,149,21,15,14,21,0,63,66,60,5,130,202,74,182,9,33,
    21,17,89,39,13,8,52,2,6,46,78,57,32,32,57,78,46,102,111,162,52,97,139,87,87,139,98,52,56,99,139,82,1,233,24,51,81,56,49,78,53,28,132,128,253,119,2,137,92,148,103,56,53,94,127,73,92,
    133,85,41,131,117,36,96,255,232,4,20,130,117,52,51,0,34,64,17,13,15,149,12,10,10,0,40,42,149,39,37,16,0,149,27,65,74,5,42,237,50,17,57,47,205,253,205,48,49,37,135,128,36,21,35,17,51,
    21,67,233,8,33,46,2,73,65,16,8,82,2,113,47,95,76,48,15,34,56,41,144,144,58,109,82,50,37,63,83,93,96,45,122,196,138,75,80,147,205,126,153,131,131,162,87,141,99,54,50,93,134,114,19,42,
    68,50,28,52,41,25,105,1,102,115,32,68,106,73,64,97,71,48,30,12,74,136,192,118,121,203,146,82,48,156,66,62,108,148,86,87,145,103,57,131,173,53,166,0,0,4,248,4,0,0,19,0,23,64,10,13,9,
    2,18,15,16,10,1,6,66,119,11,42,33,35,17,6,7,1,35,1,38,39,17,130,163,8,83,1,22,23,55,1,51,4,248,162,22,46,255,0,147,254,248,21,47,141,209,1,14,29,45,73,1,6,218,3,76,64,124,253,112,2,
    144,53,135,252,180,4,0,253,100,73,129,198,2,160,0,1,0,120,0,0,3,19,4,24,0,51,0,42,64,21,25,13,16,149,40,37,24,21,149,29,28,32,32,12,21,51,46,149,0,79,239,7,44,57,47,206,51,237,50,220,
    50,253,50,206,48,49,69,172,9,130,118,42,6,6,35,34,46,2,35,34,6,7,21,83,95,7,37,30,2,51,50,54,55,81,167,5,8,74,6,7,6,7,187,24,32,27,75,46,90,146,104,56,161,21,42,19,32,48,38,31,15,24,
    60,32,144,144,29,64,32,29,39,34,35,26,17,38,19,33,62,86,54,45,74,27,32,26,3,250,8,7,5,10,55,106,154,98,253,133,1,75,9,13,33,39,33,32,18,130,1,152,130,16,27,130,10,8,62,11,10,164,69,
    103,69,34,12,7,9,10,0,0,2,0,20,255,232,5,26,4,0,0,69,0,87,0,59,64,31,75,149,49,22,59,57,85,149,60,39,63,63,36,149,68,21,26,149,15,31,149,10,20,15,15,68,4,69,149,1,15,0,63,130,189,39,
    50,47,51,220,237,16,237,50,130,2,33,47,51,130,210,33,50,63,71,75,7,33,21,20,120,44,5,33,51,50,67,45,10,33,14,2,131,221,32,39,130,221,33,30,3,90,149,9,44,54,55,6,7,39,54,54,55,62,3,
    51,53,1,88,73,11,8,100,6,6,1,217,1,208,150,10,22,36,26,30,60,68,80,48,29,50,19,22,19,18,21,18,49,28,29,55,66,84,57,57,85,60,35,6,47,90,39,54,95,71,41,44,74,97,53,68,110,77,42,8,8,51,
    30,73,43,123,69,33,84,96,104,53,254,211,15,30,48,33,27,42,29,14,34,57,74,41,15,17,3,116,140,140,78,53,87,64,35,43,52,43,14,8,10,11,148,15,11,10,17,130,11,8,84,38,61,77,39,33,31,11,
    47,77,111,73,71,106,72,36,40,77,110,69,34,65,32,22,25,120,37,53,13,47,76,53,29,100,253,170,35,62,47,28,27,42,53,26,51,70,42,17,35,78,0,0,2,0,99,255,232,5,219,4,24,0,23,0,31,0,32,64,
    17,22,21,19,15,18,149,0,0,4,24,149,12,16,28,149,4,22,80,112,8,37,63,63,48,49,1,6,77,61,9,47,51,50,30,2,23,33,17,51,17,35,17,1,32,17,16,33,130,3,8,60,4,21,11,119,119,213,226,129,129,
    120,121,219,115,172,118,68,11,1,36,164,164,253,6,254,205,1,47,1,54,1,209,233,128,128,145,145,250,1,0,138,138,63,115,164,102,1,164,252,0,1,209,1,189,254,112,254,116,1,140,1,144,70,181,
    5,48,3,248,4,0,0,11,0,24,64,11,8,149,2,2,4,10,7,73,231,16,33,17,33,130,113,32,51,130,5,52,51,3,248,164,253,246,164,164,2,10,164,1,209,254,47,4,0,254,92,1,164,130,69,36,90,255,234,3,
    101,130,69,8,36,50,0,36,64,18,21,21,39,29,149,16,22,44,39,149,45,6,50,0,5,149,2,15,0,63,253,50,221,50,50,237,50,63,237,17,57,47,130,211,75,96,5,65,201,10,36,51,20,30,4,51,68,104,8,
    8,91,14,2,7,53,62,3,51,1,74,140,1,192,140,86,138,96,51,45,91,140,95,96,130,79,34,171,4,13,23,37,53,37,55,78,49,22,44,82,115,71,47,90,79,63,20,15,56,67,71,31,3,112,144,144,96,15,69,
    103,137,83,87,146,106,60,65,99,120,54,12,39,44,45,37,23,40,70,98,57,64,101,71,38,15,22,27,12,157,7,19,16,12,0,130,173,40,38,255,232,4,160,4,0,0,38,131,173,48,36,30,38,27,20,149,17,
    22,27,149,10,22,4,38,149,32,1,73,20,6,39,253,222,50,237,17,18,57,57,71,59,6,34,1,14,3,25,146,220,14,32,54,75,106,7,63,19,2,248,1,168,122,254,206,32,70,83,102,63,66,93,62,36,19,5,87,
    1,88,95,12,27,46,34,57,93,32,254,97,75,115,6,8,38,225,3,116,140,140,253,142,66,105,72,39,33,52,62,58,48,10,140,140,23,43,34,21,67,69,3,2,253,220,14,38,18,1,222,0,0,3,0,96,130,155,54,
    80,4,24,0,11,0,17,0,23,0,26,64,13,12,149,19,19,0,14,149,6,16,22,84,195,6,8,90,237,17,57,47,237,48,49,5,34,0,53,16,0,51,50,0,21,20,0,19,2,33,34,6,7,5,33,22,22,51,32,2,82,228,254,242,
    1,26,240,229,1,1,254,235,103,41,254,229,136,177,23,2,152,253,100,10,183,147,1,51,24,1,32,236,1,2,1,34,254,230,250,245,254,217,2,116,1,50,160,146,139,163,188,0,1,0,30,254,90,4,48,130,
    119,63,50,0,42,64,22,34,31,149,28,37,49,3,149,0,0,32,33,16,43,149,19,22,22,12,149,11,28,0,63,237,63,51,130,2,130,126,41,50,222,50,237,50,48,49,1,33,21,25,146,4,41,60,116,1,188,140,
    34,55,70,37,86,110,36,50,59,50,33,45,115,74,75,121,86,46,176,176,164,1,2,254,254,25,146,4,34,50,1,71,140,220,53,254,239,140,254,214,51,82,57,31,31,57,82,51,39,130,183,36,96,255,232,
    3,205,130,183,52,32,0,34,64,17,31,149,1,1,28,18,20,149,17,15,16,30,28,149,2,7,76,87,6,34,51,237,50,65,51,5,32,1,84,111,7,43,39,38,53,52,55,54,51,50,23,21,38,35,80,145,8,8,61,55,17,
    35,2,78,1,127,44,55,48,131,81,234,143,141,156,156,246,153,131,131,162,169,106,106,98,99,167,107,78,223,2,39,254,28,26,20,17,28,140,139,241,241,155,156,48,156,66,112,112,180,184,104,
    104,38,1,4,0,0,1,0,20,130,135,32,248,130,135,47,33,0,24,64,12,33,21,5,149,28,16,17,19,149,16,14,134,130,35,237,63,48,49,95,232,6,32,6,74,164,7,8,53,6,51,50,22,21,17,35,3,84,11,24,37,
    25,58,83,65,53,53,60,79,106,72,60,46,42,46,48,67,53,47,53,66,94,129,88,130,133,164,3,3,29,51,38,22,80,132,168,175,168,132,80,15,143,21,134,9,35,140,152,253,12,131,119,40,166,0,0,3,
    248,4,0,0,28,130,119,55,11,17,7,149,23,23,13,16,15,1,13,21,0,63,51,63,18,57,47,237,51,48,49,33,24,81,127,9,37,17,35,17,51,17,51,75,47,6,8,47,3,248,164,30,59,89,60,69,106,72,37,164,
    164,4,26,76,89,97,48,82,129,88,47,1,56,86,121,77,36,60,99,126,65,254,230,4,0,254,74,43,69,47,25,56,107,155,99,0,130,229,32,33,130,109,49,112,4,0,0,9,0,21,64,10,0,6,149,8,15,5,1,149,
    3,81,84,5,33,237,50,130,226,63,1,33,21,33,53,1,33,53,33,3,112,253,162,2,88,252,183,2,94,253,219,3,22,3,209,252,187,140,51,3,65,140,130,67,32,96,130,67,8,37,152,4,24,0,43,0,33,64,17,
    15,21,18,149,34,23,28,149,25,1,149,43,4,40,149,9,16,0,63,253,220,50,253,222,253,50,222,237,63,130,79,130,77,36,51,52,62,2,51,86,202,5,36,53,35,34,46,2,87,147,5,35,20,30,2,51,125,68,
    6,8,77,6,21,1,224,254,128,114,43,82,118,74,97,147,99,50,162,231,68,116,85,48,114,1,128,108,26,43,56,31,230,34,61,85,50,74,82,3,41,140,140,54,89,62,34,56,103,148,92,253,119,188,25,56,
    91,67,140,140,34,39,21,6,1,66,63,97,66,34,55,45,0,0,2,0,96,255,232,6,13,130,153,60,65,0,79,0,47,64,25,41,150,30,12,17,150,54,59,54,30,35,54,54,35,30,3,74,66,149,8,16,74,67,133,8,37,
    23,57,47,47,47,17,78,5,7,32,5,82,224,6,35,51,50,23,22,24,83,15,13,36,51,50,62,2,53,80,58,17,33,2,7,83,25,5,32,3,83,17,13,54,82,226,137,135,141,141,240,229,128,60,31,18,39,34,27,7,44,
    57,32,12,9,12,9,80,83,14,46,10,12,10,15,20,14,22,17,12,3,4,3,139,138,221,83,37,8,32,24,83,11,6,47,141,68,85,15,19,11,3,30,46,56,26,20,66,70,62,16,80,104,15,49,68,72,64,15,24,29,12,
    18,19,7,26,53,29,245,148,147,3,166,83,60,10,56,0,1,0,30,0,0,3,42,4,0,0,11,0,21,64,10,1,8,149,11,21,7,2,149,4,79,1,5,42,237,50,48,49,55,33,17,33,53,33,21,130,5,54,21,33,31,1,51,254,
    204,3,11,254,205,1,52,252,245,140,2,232,140,140,253,24,140,130,67,8,40,80,255,232,6,167,4,24,0,76,0,49,64,25,60,59,22,21,0,59,21,21,59,0,3,14,75,70,1,4,149,49,44,39,22,14,149,29,16,
    0,63,237,76,173,6,32,17,65,80,6,39,17,51,48,49,1,17,22,22,70,14,12,35,6,7,39,54,77,231,9,72,11,5,46,14,3,35,34,46,2,53,52,54,55,54,55,23,6,7,24,112,127,8,8,161,2,55,17,3,182,54,137,
    80,82,119,77,36,61,115,164,103,50,89,76,64,26,61,44,121,55,77,33,83,99,116,67,153,229,153,77,70,126,176,106,58,103,91,80,35,38,89,97,105,54,105,150,96,44,54,33,39,49,117,35,27,23,39,
    30,60,91,61,51,89,74,58,20,2,21,254,253,69,78,51,93,131,80,82,155,119,72,17,28,36,18,44,55,107,64,50,21,42,32,20,93,159,211,117,113,182,128,69,24,42,57,33,33,57,42,24,61,102,136,75,
    79,122,42,49,36,115,22,32,27,80,54,47,81,59,34,28,43,52,24,1,3,0,0,2,0,10,255,232,5,79,4,24,0,59,0,79,0,44,64,22,5,149,6,41,130,1,46,25,70,36,33,17,14,149,50,47,55,16,60,149,25,22,
    65,6,9,39,50,17,57,57,47,47,16,237,76,67,6,34,53,50,62,130,226,38,38,35,34,6,7,22,18,72,248,9,34,18,55,38,130,19,36,14,2,21,35,52,89,247,5,67,211,5,32,1,132,45,8,95,46,2,39,14,3,21,
    20,30,2,5,79,51,86,110,58,41,56,34,15,140,147,48,75,31,103,101,42,87,131,88,87,130,87,44,112,103,27,67,31,38,67,52,30,160,36,83,133,97,71,130,60,26,64,75,82,44,126,171,105,46,253,24,
    54,70,41,17,22,43,66,44,44,70,49,26,17,42,70,2,192,74,107,69,34,138,25,41,52,28,94,113,16,11,98,254,249,145,77,177,7,8,37,146,1,7,98,15,11,30,54,76,47,59,123,99,63,35,32,12,23,20,12,
    63,99,123,253,119,52,77,90,38,67,125,112,95,38,38,95,112,125,77,189,5,8,62,2,0,10,0,0,7,210,4,24,0,97,0,114,0,73,64,39,76,75,62,90,15,65,149,84,0,59,149,114,16,114,38,33,149,44,39,
    44,84,114,44,44,114,84,3,93,61,21,104,149,10,52,149,23,23,10,16,0,63,51,47,237,16,237,81,119,6,36,17,51,16,237,50,132,4,131,18,38,51,51,48,49,1,46,3,76,6,5,36,22,21,20,6,7,24,94,17,
    9,40,35,34,38,39,38,39,55,22,23,24,112,202,12,35,7,3,35,1,75,39,5,66,54,12,37,22,51,50,62,2,55,78,40,5,36,54,55,19,54,54,83,221,5,8,140,2,21,20,30,2,23,4,45,44,89,71,45,34,63,89,54,
    113,117,14,8,39,72,69,71,78,88,53,60,98,69,37,36,66,95,58,43,75,28,33,27,46,21,24,20,49,25,58,70,20,32,42,22,40,71,70,75,90,108,68,209,161,254,226,54,150,98,53,90,67,38,24,15,17,22,
    122,16,11,11,16,58,49,44,69,55,41,17,173,233,29,10,4,10,21,11,149,7,12,53,38,20,31,22,11,25,41,52,28,2,21,11,44,72,101,68,45,80,59,35,134,126,33,57,32,19,63,71,72,58,37,39,70,98,58,
    60,97,67,36,21,14,15,20,132,66,58,60,57,35,50,31,15,43,67,80,72,55,9,253,247,3,50,126,109,29,59,88,59,42,73,29,33,29,131,132,49,50,24,54,64,45,81,111,66,253,24,83,60,38,70,31,1,160,
    22,25,141,209,16,49,80,255,232,5,246,4,24,0,74,0,46,64,23,51,150,30,29,50,130,1,45,71,43,40,61,149,19,16,13,22,74,71,149,0,3,86,175,7,34,51,237,50,66,140,5,37,17,51,237,48,49,1,82,
    22,13,65,69,18,37,30,2,51,50,54,55,86,215,5,33,21,34,86,211,5,67,196,9,38,6,7,2,86,62,157,97,67,125,7,37,89,137,50,66,151,80,67,121,16,62,57,98,40,23,42,88,135,93,49,71,46,22,32,56,
    78,45,82,120,76,37,61,115,164,103,75,122,48,3,211,29,40,67,120,7,35,50,42,42,50,67,116,18,48,35,26,58,63,65,120,91,55,147,26,48,66,40,42,69,49,26,67,189,7,8,37,36,26,0,0,1,0,87,255,
    231,3,121,4,25,0,55,0,40,64,20,29,30,48,149,50,8,9,50,50,0,39,41,149,38,36,16,0,149,19,78,120,10,49,222,205,16,237,57,57,48,49,37,50,54,53,52,46,2,35,53,50,86,255,5,68,113,5,36,62,
    2,55,53,38,130,232,72,65,7,40,6,21,20,22,51,51,21,35,34,131,8,8,110,2,9,97,112,12,27,44,33,51,99,77,48,41,91,144,102,80,152,119,73,39,67,92,54,100,111,186,188,141,171,150,149,105,119,
    130,131,96,101,149,148,134,115,47,44,11,21,17,10,138,19,45,76,57,48,84,63,36,36,77,120,83,54,92,69,41,3,5,21,118,90,128,137,62,153,75,72,69,81,71,139,95,85,87,95,0,0,2,0,145,255,232,
    3,245,4,0,0,16,0,29,0,23,64,11,13,149,23,23,15,12,15,17,80,122,9,68,50,7,8,73,34,46,2,53,17,51,17,33,17,51,1,50,62,2,53,53,33,21,20,30,2,3,245,61,115,165,103,99,157,109,59,161,2,33,
    162,254,77,60,101,72,40,253,223,40,72,100,1,166,111,168,111,56,54,109,163,108,2,102,254,92,1,164,252,114,37,75,111,75,53,47,76,114,76,38,130,117,8,42,70,255,232,7,107,4,24,0,19,0,89,
    0,55,64,29,15,150,72,72,61,66,150,87,20,5,150,82,82,22,150,24,20,20,40,30,149,61,22,45,40,149,46,51,66,55,6,46,237,17,57,47,51,237,50,47,237,16,222,237,17,51,47,131,149,80,228,14,32,
    37,130,146,32,35,131,148,69,226,12,32,39,83,198,8,69,222,5,32,33,69,223,7,71,254,5,46,23,51,53,2,90,23,46,69,46,46,69,47,23,23,47,131,7,8,87,46,23,1,16,1,192,140,37,64,85,48,79,119,
    78,39,62,119,171,108,89,128,93,64,24,71,31,79,111,148,100,162,237,155,74,70,126,176,106,81,137,104,69,13,255,0,5,50,87,123,79,88,130,86,43,43,86,130,88,77,122,87,51,6,254,1,67,42,75,
    57,34,33,56,74,42,43,76,56,33,35,58,74,244,144,144,193,53,78,52,66,54,8,40,22,32,35,13,129,14,38,35,24,66,119,7,8,52,37,71,105,69,59,103,76,44,60,98,127,66,67,125,96,58,43,74,99,55,
    127,0,2,0,56,0,0,3,28,4,0,0,10,0,13,0,26,64,12,6,3,11,149,1,9,9,12,5,15,0,21,0,63,82,22,5,8,45,50,50,48,49,33,17,33,53,1,51,17,51,21,35,17,3,17,1,2,8,254,48,1,172,198,114,114,162,254,
    210,1,1,107,2,148,253,134,133,254,255,1,134,1,208,254,48,130,83,32,166,130,83,32,180,132,83,43,17,0,21,64,10,4,149,11,11,1,15,13,88,185,5,37,63,57,47,237,48,49,130,73,94,142,5,94,138,
    8,8,69,166,164,1,20,157,185,186,142,254,222,246,204,206,4,0,254,119,164,147,148,172,1,234,254,162,180,170,0,1,255,236,0,0,2,164,4,0,0,53,0,55,64,28,10,7,149,46,43,28,28,46,33,17,0,
    0,19,149,36,33,33,11,37,42,149,39,15,16,11,149,14,73,207,7,65,200,6,39,50,16,204,50,47,50,16,237,130,192,42,1,35,54,54,53,52,38,7,34,6,7,86,21,6,32,6,67,17,5,46,54,55,51,6,6,21,20,
    51,50,54,55,17,35,53,33,130,224,68,46,7,54,6,2,144,118,13,12,42,46,17,31,17,170,254,8,170,43,38,51,71,43,20,11,9,130,19,8,55,88,14,33,18,170,1,248,170,20,40,19,55,72,43,17,12,1,96,
    21,41,28,39,43,1,9,7,254,145,140,140,1,23,19,31,52,69,39,28,54,22,20,43,28,80,9,8,1,87,140,140,254,255,8,11,30,53,130,20,8,43,53,0,2,0,86,255,232,5,58,4,24,0,82,0,98,0,50,64,26,96,
    70,149,12,31,149,32,52,15,12,12,23,86,149,62,0,5,149,77,77,62,16,23,149,42,67,213,5,36,47,253,206,16,237,131,208,34,51,222,237,133,202,44,46,3,35,34,14,4,35,34,38,39,14,3,21,105,243,
    5,67,233,20,70,96,10,35,14,2,7,50,99,34,5,34,23,37,52,91,183,9,8,74,54,54,4,166,5,13,27,45,36,37,63,62,68,83,104,68,24,61,23,19,74,74,55,75,62,62,59,21,31,35,15,43,87,70,45,41,72,96,
    55,59,108,83,50,54,75,77,23,40,75,59,35,34,63,89,54,113,132,6,10,12,5,60,89,73,65,73,87,59,73,100,63,32,6,252,120,70,17,5,8,135,33,47,51,19,9,16,2,174,27,50,40,24,41,62,72,62,41,4,
    4,23,62,77,92,52,67,75,65,47,32,44,27,12,128,26,58,93,67,56,91,63,34,32,67,105,72,65,104,83,63,23,11,38,58,79,53,45,80,59,35,124,117,14,37,36,31,8,43,63,75,63,43,49,80,101,52,110,54,
    63,15,24,31,16,35,46,28,15,3,14,44,0,0,1,0,164,0,0,3,200,4,0,0,20,0,27,64,13,17,0,149,20,20,7,6,149,9,15,19,7,21,0,63,51,63,237,17,57,47,237,51,48,49,1,50,54,53,52,95,170,6,8,46,23,
    22,21,20,6,7,1,35,1,1,244,117,112,207,194,164,1,102,178,98,99,125,121,1,61,190,254,234,2,49,82,84,157,252,140,4,0,80,80,135,109,143,16,254,51,1,185,130,99,36,143,255,232,3,125,130,
    99,47,21,0,15,182,10,4,149,15,22,0,15,0,63,63,253,204,130,87,32,17,91,76,6,32,51,109,232,8,39,1,49,112,101,46,78,57,32,84,65,6,54,97,52,4,0,253,119,127,133,34,66,97,63,92,148,103,56,
    56,103,148,92,2,137,0,130,85,59,20,0,0,5,145,4,0,0,27,0,30,64,16,17,20,14,3,23,11,6,2,27,18,7,15,15,4,2,132,188,33,51,51,98,100,6,44,1,35,3,3,35,1,51,1,22,23,51,54,55,124,110,7,132,
    12,58,1,5,145,254,96,161,136,143,161,254,124,180,1,4,29,7,4,10,22,106,178,180,85,87,173,187,102,132,12,41,1,24,4,0,252,0,1,101,254,155,130,133,46,24,82,61,77,62,1,22,1,214,243,243,
    254,58,254,222,131,13,34,2,236,0,130,137,32,104,130,223,59,134,4,24,0,57,0,39,64,20,19,1,149,57,40,35,149,38,38,13,45,149,30,16,11,13,149,10,8,98,253,9,35,47,237,50,222,67,57,5,95,
    17,6,93,217,9,66,88,7,43,30,2,21,51,21,33,53,51,52,46,2,35,86,21,5,41,23,22,22,23,33,3,134,148,29,67,93,227,7,8,39,37,63,85,49,68,103,69,35,55,96,130,75,83,128,87,45,97,254,155,98,
    21,44,68,47,98,91,29,56,80,51,26,47,23,1,60,1,141,53,70,76,93,236,6,8,33,41,57,44,35,19,27,55,69,88,59,72,111,75,38,48,75,92,44,140,140,19,43,36,23,80,64,40,54,42,34,20,10,19,11,131,
    197,32,10,130,197,53,221,4,0,0,37,0,28,64,14,17,149,18,18,7,34,1,149,36,15,7,149,28,130,191,33,237,63,79,53,10,83,90,11,71,10,12,8,75,17,33,53,33,3,21,254,204,19,40,65,46,46,64,40,
    18,18,40,64,46,89,127,82,40,40,82,127,88,88,126,81,38,254,203,3,11,3,116,253,190,38,67,49,29,29,49,67,38,40,68,50,29,147,55,91,120,65,66,120,92,55,54,92,120,65,2,65,140,0,1,0,12,255,
    238,2,95,130,137,56,28,0,21,64,10,2,8,28,15,19,149,8,22,1,21,0,63,63,237,63,18,57,48,49,33,91,91,6,36,38,39,38,39,53,73,211,5,8,53,62,4,53,53,51,2,95,164,4,21,67,85,101,55,22,35,13,
    15,13,12,16,14,35,22,37,78,74,65,49,29,164,1,57,54,117,97,63,4,3,3,4,170,10,7,6,10,62,104,136,146,148,64,231,0,130,107,32,60,130,107,53,143,4,0,0,30,0,24,64,12,4,29,21,149,10,22,3,
    21,29,149,0,15,0,130,108,33,63,237,131,110,34,19,33,17,149,112,35,33,60,2,83,150,113,37,254,81,4,0,252,0,148,118,32,91,130,117,36,14,0,0,3,203,130,117,39,11,0,14,181,11,4,15,7,66,176,
    6,35,48,49,1,1,66,168,8,36,1,3,203,254,104,66,152,10,33,1,16,131,82,66,135,7,66,121,9,53,15,4,24,0,46,0,27,64,13,10,25,45,35,20,3,149,45,22,25,149,20,16,84,53,6,32,17,131,192,34,55,
    53,22,96,62,6,66,100,7,32,23,96,103,10,83,119,7,35,104,134,161,216,66,88,7,37,66,110,141,75,133,105,96,98,5,48,29,56,80,51,68,108,76,41,67,112,146,80,158,37,176,99,144,66,78,13,96,
    97,6,54,40,54,42,34,20,26,55,69,90,62,76,112,74,36,0,0,2,0,96,255,232,5,137,130,155,43,67,0,87,0,43,64,21,85,23,0,67,23,130,1,43,73,15,10,20,149,57,52,47,16,73,149,35,76,232,9,46,17,
    57,57,47,47,17,51,17,51,48,49,1,54,55,54,24,119,15,8,40,7,46,3,35,34,6,7,30,5,76,236,9,69,114,6,72,31,7,36,6,7,6,7,5,66,145,9,8,193,4,39,6,6,4,118,30,25,20,35,25,43,57,31,39,62,52,
    48,24,24,53,58,65,37,51,95,42,15,66,80,84,69,45,50,92,128,79,77,132,96,55,51,84,111,119,121,52,47,71,55,44,21,21,46,54,66,41,69,116,84,48,52,31,36,46,252,44,21,45,70,48,43,64,44,22,
    35,56,67,63,52,12,34,38,2,94,20,25,21,57,33,36,51,33,15,20,33,46,25,25,46,33,20,35,33,6,31,52,77,103,133,83,84,136,96,52,56,108,157,102,117,187,144,104,66,31,13,22,27,15,15,27,22,13,
    39,75,108,68,58,97,34,40,33,97,52,98,75,46,37,60,78,41,66,103,78,55,37,20,4,60,154,0,0,2,0,46,255,232,4,120,4,24,0,11,0,99,0,34,64,18,9,12,28,64,4,3,83,45,77,51,149,88,40,22,3,65,188,
    6,43,51,253,50,204,50,17,23,57,48,49,1,52,90,31,5,37,22,23,54,54,7,38,75,81,5,36,50,30,2,21,20,65,23,12,91,192,5,43,54,53,52,46,2,39,46,3,39,14,3,7,70,200,6,34,62,2,53,69,93,8,8,75,
    52,62,4,2,170,45,42,43,44,38,49,48,39,217,55,47,36,62,85,49,49,85,62,36,48,54,39,93,93,87,67,40,31,62,94,62,51,89,67,38,142,15,27,36,22,45,59,20,34,43,24,24,76,81,75,24,24,75,81,76,
    24,24,43,34,20,59,45,22,36,27,15,142,38,67,89,51,130,42,8,62,31,40,67,87,93,93,3,74,32,43,47,28,38,53,30,30,53,147,38,93,60,45,73,53,29,29,53,73,45,60,93,38,22,47,55,65,80,98,60,54,
    93,68,39,32,63,91,60,22,42,33,20,62,63,36,60,48,37,15,13,43,47,43,13,132,4,57,15,38,48,59,36,63,62,21,34,42,20,60,91,63,32,39,68,93,54,60,98,80,65,55,47,0,90,143,6,48,125,4,24,0,23,
    0,23,64,11,12,12,22,18,149,7,16,22,95,217,10,35,48,49,33,33,113,154,8,32,35,69,50,6,37,17,33,3,125,253,18,90,128,5,44,97,52,162,32,57,78,46,101,112,2,76,2,137,70,63,7,39,63,97,66,34,
    133,127,254,3,83,49,6,43,2,223,4,0,0,5,0,13,181,3,15,4,134,91,132,87,45,51,17,33,2,223,253,199,164,1,149,4,0,252,140,130,141,36,96,255,232,3,98,130,141,58,21,0,16,183,13,149,8,16,19,
    149,2,22,0,63,237,63,237,48,49,37,6,35,34,0,53,52,0,84,68,6,8,50,6,21,20,22,51,50,55,3,96,118,162,219,254,243,1,34,242,135,103,114,130,157,201,189,159,134,118,47,71,1,29,227,253,1,
    51,50,168,80,225,183,180,208,89,0,3,0,20,255,232,5,104,130,131,52,33,0,44,0,61,0,44,64,22,15,5,44,149,32,45,2,32,26,149,51,32,130,1,38,1,39,149,10,22,1,15,91,132,5,35,57,47,47,16,96,
    111,6,34,50,48,49,74,63,5,71,60,7,96,110,11,34,21,21,33,67,22,8,33,37,53,75,115,11,8,105,4,5,165,190,190,50,96,139,89,92,138,93,47,118,74,115,81,42,60,90,105,46,77,110,70,33,1,162,
    254,94,20,48,81,60,58,79,50,22,253,189,21,35,47,26,28,49,37,21,16,34,56,40,4,0,253,248,152,82,138,100,56,56,100,138,82,39,73,106,67,76,105,66,30,53,85,109,55,108,152,41,84,69,44,44,
    69,84,41,152,113,38,55,36,17,18,33,47,28,27,48,37,21,0,0,2,0,11,130,215,33,18,4,94,157,8,42,21,4,2,149,0,0,51,57,149,38,22,94,157,8,32,16,94,157,19,83,230,8,24,110,221,8,65,218,5,32,
    7,68,170,7,91,236,5,32,55,80,244,6,80,230,14,40,1,72,254,195,78,38,83,132,95,94,157,12,33,68,37,94,157,9,33,89,130,94,157,12,33,1,189,94,157,8,39,69,46,24,17,41,70,2,192,94,157,42,
    45,253,178,52,77,90,38,67,128,115,99,38,38,99,115,94,157,6,33,2,0,75,175,11,44,23,64,11,17,149,6,6,8,16,149,10,15,8,93,75,12,8,46,22,21,20,6,35,33,17,35,17,1,50,53,52,35,35,17,2,108,
    142,186,185,157,254,236,164,1,152,206,204,246,4,0,172,148,147,164,254,119,4,0,254,22,170,180,254,162,0,66,131,5,54,4,21,4,24,0,47,0,28,64,14,47,149,45,2,7,149,38,16,22,17,149,23,28,
    72,101,6,40,253,222,50,237,48,49,1,53,51,94,87,5,79,120,7,35,54,55,54,55,25,123,193,8,81,35,5,8,87,30,2,23,22,23,51,21,2,176,93,11,26,22,77,63,78,119,80,41,45,88,130,85,67,117,44,51,
    43,39,50,43,120,73,127,192,129,65,67,127,183,115,60,94,74,54,20,45,18,92,2,147,140,30,25,21,35,65,111,149,83,84,142,103,59,27,17,19,26,156,20,16,14,21,77,137,187,111,122,206,149,83,
    22,35,45,23,55,69,140,130,165,47,31,0,0,4,251,4,0,0,8,0,17,183,8,3,149,6,70,225,7,32,237,95,149,5,59,17,33,53,33,17,1,4,251,253,130,162,254,68,2,94,1,199,4,0,252,0,3,116,140,253,15,
    2,241,130,61,32,166,130,61,32,237,130,61,50,23,0,30,64,14,18,17,9,14,149,23,3,3,19,22,15,16,19,21,86,200,6,35,51,253,206,51,131,74,76,86,6,40,35,52,46,2,35,17,35,17,5,130,3,8,43,51,
    17,2,171,57,124,72,75,120,85,45,146,15,37,62,47,161,254,46,161,161,3,5,27,38,47,87,126,79,31,67,54,35,253,82,2,134,232,254,98,4,0,254,85,133,107,33,3,248,130,107,40,12,0,23,64,11,9,
    12,2,3,90,241,12,38,23,57,48,49,33,35,1,131,91,55,51,17,51,1,51,1,3,248,230,254,60,4,164,164,4,1,174,215,254,37,1,236,254,20,130,71,36,43,1,213,254,18,76,131,5,33,3,73,130,75,47,20,
    0,40,0,25,64,12,26,149,20,17,17,1,36,149,7,67,187,8,39,47,51,237,48,49,1,51,17,24,248,158,14,25,154,86,16,8,68,166,163,56,103,143,87,85,135,95,51,51,95,135,85,69,112,45,22,27,52,77,
    50,50,75,52,26,26,52,75,50,50,77,52,27,4,0,253,95,79,137,101,58,55,100,137,83,83,138,100,55,36,34,254,206,48,86,65,38,38,65,86,48,47,86,65,39,39,65,86,66,45,6,59,6,33,4,24,0,67,0,61,
    64,32,48,51,43,3,41,47,49,58,44,41,15,7,10,3,9,14,17,41,130,1,48,24,34,149,58,22,24,149,11,9,0,16,0,63,204,50,237,63,109,163,5,39,17,51,18,23,57,17,51,16,70,130,7,36,50,30,2,23,22,
    109,139,14,32,35,81,213,10,35,55,54,55,51,130,30,39,19,35,39,7,35,39,6,7,79,172,9,8,73,2,116,54,94,79,64,25,59,39,61,170,69,70,157,146,163,75,74,167,22,41,18,47,63,77,48,86,135,93,
    50,46,90,132,85,48,77,63,47,18,41,22,167,74,75,163,146,157,70,69,170,61,39,59,25,64,79,94,54,124,194,133,70,73,138,197,4,24,21,33,44,22,53,67,131,109,167,5,37,67,54,23,44,34,21,66,
    130,7,49,21,34,43,23,54,68,168,168,254,240,161,161,131,67,53,23,43,33,66,140,8,39,0,1,0,96,255,230,5,85,130,253,63,60,0,46,64,23,11,8,149,6,6,0,50,52,149,49,47,16,22,17,12,5,0,149,
    33,28,23,38,22,0,63,51,51,99,30,6,130,6,76,238,6,34,37,50,54,130,208,88,90,5,32,21,74,96,5,67,63,10,34,38,39,38,130,226,37,6,6,35,6,46,2,90,56,14,8,41,30,2,2,106,49,89,35,40,35,140,
    1,187,140,20,26,22,58,35,38,64,23,27,23,23,28,24,64,37,63,94,32,37,26,45,54,46,121,67,119,188,131,70,90,79,9,8,37,45,89,132,114,20,13,15,19,185,141,141,185,19,15,13,20,15,9,11,14,143,
    12,10,8,14,24,14,17,21,21,17,14,24,2,74,137,193,118,90,100,8,35,87,145,103,57,70,51,5,33,6,62,130,219,46,15,0,85,0,101,0,44,64,22,99,149,21,21,62,91,130,213,43,73,16,81,65,56,16,8,
    62,149,48,43,51,132,222,38,253,50,50,204,51,51,63,90,247,9,36,34,14,2,21,20,24,104,127,7,38,1,50,62,2,55,46,5,68,18,7,73,103,7,32,39,83,210,6,72,86,6,32,55,72,119,14,32,3,90,190,5,
    8,52,6,21,20,30,2,23,54,54,2,113,53,70,42,17,104,88,89,83,17,41,70,1,43,88,146,111,71,12,12,60,78,84,71,45,36,65,91,55,85,113,68,29,73,153,234,160,70,106,81,62,26,60,133,100,48,9,37,
    31,70,27,103,115,44,100,47,6,8,68,98,103,15,37,48,64,1,240,18,34,49,31,34,50,41,63,75,33,2,2,3,142,52,77,90,38,134,218,75,75,218,134,38,90,77,52,252,227,42,73,100,59,1,13,30,53,85,
    121,83,61,101,73,40,77,130,171,94,130,220,160,90,15,23,27,12,32,45,63,99,100,74,5,36,21,15,98,253,146,86,151,7,53,145,253,98,6,13,11,7,1,206,78,124,87,46,70,61,76,101,62,30,6,18,35,
    93,93,11,47,15,0,33,0,51,0,37,64,19,20,149,49,41,149,28,49,130,1,37,38,16,149,8,16,38,89,221,8,87,132,5,35,16,237,48,49,89,217,11,8,35,21,20,7,6,3,34,7,6,7,51,50,30,4,51,51,54,53,52,
    38,1,20,23,22,51,50,54,55,35,34,46,4,35,35,6,2,82,89,192,7,8,36,129,139,138,221,159,91,23,19,69,74,94,62,43,44,59,48,120,9,170,254,10,93,93,156,97,138,41,101,70,90,61,44,47,62,48,89,
    6,89,176,8,57,141,250,245,148,147,3,166,108,28,34,50,76,87,76,50,49,60,193,207,254,108,182,105,105,76,73,132,15,33,45,0,75,95,6,57,97,4,24,0,15,0,73,0,33,64,16,3,59,59,64,21,16,11,
    149,27,22,47,16,64,149,37,75,90,11,41,47,51,48,49,1,20,22,23,62,3,130,161,38,35,34,14,2,1,34,6,130,181,32,53,103,220,13,71,84,5,130,202,37,2,21,20,14,4,7,24,80,137,9,8,128,1,6,2,2,
    33,91,81,57,48,46,31,62,49,30,2,231,39,68,25,30,24,21,28,24,71,46,98,140,89,41,98,176,243,144,161,233,153,73,45,88,129,85,58,91,63,33,53,81,97,89,68,12,12,71,111,146,88,113,183,129,
    70,24,51,78,2,63,18,35,18,6,30,62,101,76,61,70,46,87,124,1,1,17,10,12,15,158,9,8,6,11,64,112,150,86,148,243,174,95,90,160,220,130,94,171,130,77,40,73,101,61,83,121,85,53,30,13,1,59,
    100,73,42,75,139,196,120,52,97,74,44,131,237,36,40,255,232,3,225,130,237,42,32,0,52,0,23,64,11,43,7,0,5,130,234,39,33,149,17,16,0,63,237,63,70,14,5,122,61,5,102,195,14,33,35,34,130,
    216,102,192,14,32,200,102,173,17,41,84,156,224,140,97,133,83,36,2,93,72,12,8,102,170,7,33,47,76,102,156,15,39,128,241,188,113,63,99,123,59,102,153,20,34,2,0,106,130,171,32,109,130,
    171,51,43,0,63,0,32,64,16,49,149,33,39,39,59,21,26,149,20,15,16,59,85,160,7,35,237,50,17,57,70,14,5,70,12,14,102,17,8,35,14,4,21,51,65,161,7,102,24,16,8,107,109,53,97,136,82,89,149,
    106,59,69,127,181,112,42,72,27,32,26,27,32,27,71,39,49,82,67,52,35,18,2,24,35,30,93,65,83,125,83,42,160,30,55,76,45,43,77,57,33,32,56,78,46,46,75,54,29,1,71,77,129,93,52,60,123,184,
    125,137,215,149,79,10,6,7,8,149,11,9,8,13,31,51,68,73,74,32,25,20,17,29,59,96,124,64,49,77,51,27,27,52,77,50,47,78,56,31,31,56,79,91,189,10,8,40,102,0,73,64,37,81,80,80,70,65,64,64,
    89,56,59,86,149,75,59,149,70,40,39,39,70,70,0,90,55,50,93,149,29,24,19,22,3,0,149,4,9,85,253,6,69,90,5,131,233,53,47,51,16,237,220,237,17,57,57,50,47,51,17,51,47,51,48,49,1,34,6,7,
    85,229,15,91,196,25,35,53,38,38,35,130,48,130,50,65,19,7,33,51,50,91,234,8,36,35,34,38,39,21,92,35,9,46,4,23,91,138,48,72,31,78,90,99,50,153,244,169,91,91,222,35,51,7,11,5,27,52,20,
    24,21,84,33,38,32,82,43,33,74,73,69,27,26,130,15,41,22,84,33,37,32,81,45,20,43,22,92,65,6,43,75,132,178,3,142,36,26,131,14,25,19,11,91,247,38,46,248,2,2,30,18,21,27,100,37,30,26,42,
    30,36,30,132,11,38,38,30,26,41,10,8,174,92,80,9,8,71,0,1,0,18,0,0,4,230,5,154,0,11,0,14,181,3,9,3,0,7,18,0,63,51,63,51,48,49,33,1,38,39,35,6,7,1,35,1,51,1,4,49,254,107,25,8,4,10,19,
    254,115,187,2,8,185,2,19,4,111,69,60,72,55,251,143,5,154,250,102,0,0,2,138,69,39,15,0,23,64,10,14,14,7,130,75,32,12,133,76,130,77,34,18,57,47,141,80,35,33,17,51,17,144,84,34,253,86,
    124,139,87,35,1,222,254,34,141,91,32,31,131,91,34,17,27,27,131,92,133,91,33,63,51,130,90,32,205,141,91,32,1,125,76,14,145,103,32,38,125,94,15,140,118,32,185,125,106,14,32,0,65,37,6,
    57,96,5,179,0,7,0,16,182,6,3,4,18,2,18,0,0,47,63,63,57,57,48,49,1,1,35,132,2,8,70,1,96,3,0,178,254,136,254,155,191,1,198,255,0,5,179,250,77,2,202,253,54,3,126,1,233,0,2,0,166,254,129,
    4,249,5,154,0,34,0,55,0,33,64,16,0,30,25,51,145,5,34,3,15,23,41,145,25,20,31,18,0,63,206,222,237,57,57,63,222,237,18,132,82,80,101,7,53,14,2,7,22,22,23,19,35,3,38,39,6,35,34,46,2,39,
    17,35,17,51,17,24,106,34,8,8,118,46,2,35,34,14,2,1,74,30,75,83,89,45,133,193,125,60,28,60,92,64,29,51,23,251,200,214,34,39,62,72,45,89,83,75,30,164,164,24,62,78,95,58,95,133,83,37,
    40,84,132,92,58,95,78,62,4,148,18,32,24,14,83,141,186,103,75,140,121,97,33,17,56,49,253,226,1,252,78,33,12,14,24,32,18,254,200,5,154,254,73,254,6,20,41,32,20,68,107,133,66,73,131,97,
    57,20,32,41,0,1,0,94,0,0,4,214,5,178,0,62,130,193,48,17,9,145,53,53,19,32,34,145,31,29,4,44,145,19,18,0,130,194,34,63,237,63,109,20,8,33,33,19,81,76,9,33,21,20,131,194,130,189,36,53,
    52,18,54,36,78,170,6,76,32,5,33,23,39,71,224,13,8,45,3,3,231,63,5,3,14,40,74,61,61,74,40,13,3,5,63,82,128,216,156,88,99,189,1,15,173,164,120,134,152,131,206,142,75,38,75,111,73,31,
    8,6,46,90,134,89,130,2,8,60,44,5,6,59,1,55,21,39,18,46,82,60,35,35,61,82,47,17,38,21,254,201,100,181,254,155,164,1,25,206,117,38,174,60,93,162,222,129,96,169,138,102,28,124,32,58,27,
    82,133,93,50,51,94,137,85,25,54,29,254,221,0,133,211,33,5,87,130,211,58,64,0,45,64,23,29,34,26,36,36,33,42,145,21,21,51,0,2,145,64,62,4,12,145,51,18,33,140,217,40,18,57,47,51,205,50,
    48,49,1,87,120,9,138,194,46,23,51,21,35,6,21,3,35,19,35,53,51,46,3,35,72,240,6,33,19,35,138,251,33,4,86,144,198,49,86,129,89,48,4,130,139,1,59,169,60,176,185,2,19,42,70,54,134,252,
    36,92,126,213,153,86,134,252,33,4,222,145,201,47,46,87,125,78,134,2,3,254,221,1,40,134,39,67,48,28,136,255,35,101,182,254,153,133,255,40,0,0,2,0,114,255,233,6,9,130,227,55,33,0,55,
    0,34,64,18,11,4,34,31,4,8,0,0,39,145,26,4,8,8,49,145,16,130,224,35,237,51,47,63,130,3,34,18,23,57,130,218,43,23,6,2,7,22,18,23,7,38,38,39,14,130,196,37,38,38,2,53,52,18,76,242,5,35,
    23,54,54,3,135,216,8,115,30,2,51,50,62,4,5,68,197,11,54,47,47,54,11,197,2,19,18,51,130,154,178,100,124,213,156,89,97,164,214,117,100,175,150,127,51,18,19,110,39,104,131,157,92,77,151,
    120,75,66,112,146,80,79,132,108,86,68,50,5,175,26,194,254,154,160,159,254,154,194,26,103,213,106,98,158,110,59,88,183,1,28,195,190,1,20,179,86,59,111,158,98,107,213,253,133,126,218,
    161,92,67,142,222,156,156,228,149,73,56,93,122,132,134,0,0,3,140,203,45,75,0,43,64,22,71,31,0,11,34,4,3,61,61,150,209,38,57,47,23,57,18,57,51,175,214,32,37,68,67,14,180,230,33,253,
    175,68,103,15,181,248,32,69,68,144,15,59,1,0,49,255,232,5,57,5,193,0,43,0,21,64,10,43,17,6,145,35,18,24,145,17,3,0,63,237,86,60,5,33,1,6,82,11,5,50,62,2,55,19,62,3,51,50,22,23,7,38,
    38,35,34,14,2,7,3,70,236,9,8,80,1,58,44,53,77,75,63,96,72,50,17,223,38,67,75,90,61,26,57,33,47,20,35,15,35,50,45,47,32,168,36,82,108,146,100,75,119,84,44,63,48,2,46,61,137,73,84,76,
    64,97,112,48,2,111,106,152,98,46,10,9,156,7,2,57,103,144,87,254,54,100,175,131,76,40,79,117,77,91,160,71,0,133,151,41,121,6,45,0,46,0,25,64,12,46,130,151,37,38,18,22,27,145,21,131,
    153,34,204,253,204,149,155,39,23,53,51,17,35,53,38,34,160,158,38,16,20,144,144,11,18,8,161,158,36,3,111,254,45,191,148,159,61,147,0,0,4,237,5,178,0,25,0,15,182,7,145,20,4,1,13,18,0,
    63,51,63,237,48,49,33,35,19,54,69,79,5,8,94,23,19,35,3,38,62,2,51,50,30,2,7,4,73,168,136,16,39,97,145,90,93,151,100,41,17,136,168,140,23,60,142,212,131,136,222,149,62,24,3,113,104,
    159,108,55,57,112,164,108,252,158,3,109,142,217,147,75,79,153,225,147,0,1,0,188,254,30,4,164,5,154,0,50,0,38,64,19,0,5,145,50,45,28,20,15,146,32,25,35,35,21,30,24,3,21,130,112,33,63,
    51,122,137,5,39,50,63,51,237,50,48,49,5,120,181,9,133,129,43,7,17,35,17,51,17,51,54,54,55,1,51,96,242,14,8,93,38,39,1,154,29,39,33,96,60,77,127,91,50,49,91,131,81,60,100,77,54,13,168,
    168,4,7,19,12,2,24,209,253,194,32,66,35,105,178,130,73,71,134,194,122,60,96,33,39,29,255,21,17,14,24,69,132,191,122,129,190,125,61,28,39,40,13,253,200,5,154,253,94,11,24,14,2,113,253,
    132,11,12,77,157,241,164,155,246,171,92,20,12,14,18,0,130,189,32,18,134,189,61,58,0,47,64,24,49,44,146,10,3,13,13,50,2,52,146,55,58,8,56,3,50,18,29,34,145,28,23,28,0,131,184,36,63,
    63,51,221,50,74,80,5,132,198,33,1,21,130,181,149,179,32,53,147,225,40,35,53,51,53,51,21,2,10,166,152,187,146,232,39,170,170,168,5,26,135,254,101,148,191,32,163,145,235,41,4,147,135,
    128,128,0,0,2,0,188,130,215,8,39,201,5,154,0,57,0,73,0,57,64,30,31,26,146,43,36,46,46,33,41,35,3,19,54,3,3,6,16,146,71,58,58,33,18,68,146,6,0,0,6,131,223,42,47,16,237,63,57,47,57,237,
    17,23,57,65,174,8,35,48,49,1,52,114,186,8,39,52,62,2,51,50,22,23,54,84,228,7,65,180,20,36,6,7,22,22,21,117,124,9,54,54,55,38,38,4,53,30,25,72,185,103,98,127,74,28,36,84,139,104,72,
    139,62,41,65,199,26,8,41,44,39,55,65,253,245,63,83,49,19,16,39,63,47,83,128,45,54,111,254,41,50,81,33,85,90,48,68,76,28,41,84,68,43,31,31,121,162,129,186,121,57,65,219,18,8,54,72,152,
    237,164,111,190,78,56,144,88,1,57,19,30,38,19,14,28,23,15,67,63,29,27,0,0,1,0,110,255,233,4,213,5,167,0,29,0,24,64,11,19,29,29,5,15,14,4,24,145,5,18,0,63,237,132,239,32,57,130,236,
    68,42,10,36,23,1,6,6,21,88,239,6,8,95,55,4,213,12,102,155,193,102,140,211,141,71,85,86,1,184,133,254,75,63,65,45,91,139,94,91,143,103,62,10,1,212,134,187,117,53,72,125,169,96,106,227,
    111,2,52,95,253,185,84,172,79,65,111,83,47,49,90,125,76,0,0,2,0,80,0,0,6,74,5,178,0,57,0,72,0,40,64,20,61,146,19,68,9,37,37,0,9,12,24,145,15,15,50,145,9,3,72,76,5,52,51,47,237,57,17,
    18,57,47,18,57,57,237,48,49,51,35,19,62,5,51,50,85,154,5,33,22,23,89,160,6,37,22,22,21,20,14,4,71,95,5,42,62,2,55,34,38,35,34,14,4,7,37,93,221,6,8,155,52,39,14,3,248,168,114,11,42,
    66,91,122,155,96,49,88,39,49,99,48,164,231,66,113,33,81,91,96,48,14,28,15,42,46,14,31,53,77,106,69,62,95,64,33,29,53,72,44,6,13,6,71,111,83,59,38,20,3,1,121,52,62,40,62,40,21,84,40,
    70,53,30,3,31,80,161,146,126,93,53,20,19,20,19,131,133,124,63,89,57,27,2,2,60,152,91,51,110,106,95,72,42,47,80,106,59,82,143,123,101,41,1,61,95,116,110,91,22,18,78,92,60,91,107,47,
    195,83,26,75,101,130,0,1,255,236,255,233,6,45,5,167,0,28,0,27,64,13,10,28,28,11,14,13,4,11,18,23,65,105,6,65,106,14,35,39,1,35,1,65,105,12,53,6,45,12,102,155,193,102,125,194,139,84,
    14,254,251,220,4,61,133,254,28,47,34,65,107,14,45,57,101,139,81,254,157,5,167,95,253,115,63,140,62,65,106,8,56,2,0,94,255,232,5,170,5,178,0,11,0,23,0,16,183,12,145,6,4,18,145,0,19,
    0,88,59,5,38,5,32,0,17,16,0,33,132,5,33,1,34,131,11,33,51,50,131,5,63,2,254,254,207,254,145,1,118,1,66,1,41,1,107,254,140,254,212,226,254,230,1,19,221,236,1,16,254,248,24,1,146,130,
    23,48,90,1,156,254,112,254,189,254,161,254,104,5,50,254,186,254,247,130,1,42,189,1,52,1,21,1,28,1,54,0,3,140,119,40,43,0,25,64,12,29,39,39,0,140,126,37,17,57,47,205,48,49,151,130,75,
    193,15,156,146,33,254,156,71,101,15,162,164,33,253,179,71,83,15,57,2,0,166,0,0,4,139,5,154,0,24,0,45,0,35,64,17,0,41,145,5,20,31,145,15,5,130,1,40,22,23,3,22,18,0,63,63,18,81,145,5,
    98,135,6,75,165,9,75,155,35,35,57,123,194,137,75,145,33,35,107,190,143,83,75,134,27,35,0,2,0,20,134,167,47,28,0,53,0,45,64,23,24,38,145,19,4,48,145,9,19,130,1,47,2,26,18,2,3,32,28,
    146,29,0,3,0,63,50,237,50,142,177,36,19,51,17,51,17,145,181,37,35,37,51,21,35,21,81,121,9,39,35,34,14,2,7,20,146,164,76,86,8,137,186,36,146,1,54,215,215,76,80,16,38,24,3,20,2,134,254,
    250,76,85,7,137,195,36,2,136,140,140,159,76,73,14,130,190,8,39,1,0,90,255,244,4,210,5,178,0,31,0,26,64,12,10,26,0,0,15,5,145,26,4,14,15,19,0,63,51,63,237,18,57,47,18,57,48,49,1,74,
    106,7,35,22,23,1,7,105,99,8,8,73,30,2,23,4,40,18,75,112,148,91,93,134,86,41,80,74,2,25,100,253,222,54,83,55,29,64,130,200,136,102,199,166,122,25,3,199,76,125,90,49,49,84,111,62,91,
    184,75,253,227,122,2,17,52,119,122,121,56,98,172,127,74,53,117,187,134,0,0,2,0,95,254,213,5,134,130,127,60,76,0,97,0,46,64,23,66,45,10,88,88,10,13,146,37,42,42,56,0,2,145,76,74,4,77,
    145,28,56,130,140,32,206,76,211,7,36,57,237,50,51,17,77,156,5,32,38,75,216,7,75,19,5,34,21,21,20,84,216,5,93,36,6,32,53,70,16,5,34,6,7,23,90,55,16,84,237,5,35,23,1,50,54,131,35,33,
    39,38,99,245,5,9,8,30,2,3,234,127,199,56,109,86,54,50,51,69,157,93,148,226,153,78,16,19,16,28,41,122,59,51,16,18,16,50,106,167,118,42,76,37,46,77,138,104,61,84,137,173,90,123,182,119,
    59,28,55,82,54,56,63,86,140,179,93,211,98,254,88,157,151,51,90,122,72,29,56,26,44,66,45,22,39,78,118,4,194,88,23,52,82,59,63,89,39,28,31,68,122,170,102,81,59,110,103,96,44,63,105,37,
    97,43,143,95,59,104,105,111,66,27,81,135,97,53,9,8,23,38,84,102,125,79,106,144,88,38,67,114,148,81,56,114,109,101,42,52,128,87,98,144,95,46,51,251,0,104,109,58,86,72,64,36,14,31,15,
    32,76,82,86,42,54,99,74,44,0,2,0,40,0,0,5,151,5,154,0,10,0,19,0,19,64,9,11,1,145,4,3,12,145,0,18,0,63,237,63,237,50,48,49,51,1,33,53,33,32,17,16,7,6,33,19,3,51,32,55,54,17,16,33,217,
    1,14,254,65,2,121,2,246,211,210,254,159,3,240,239,1,26,157,157,253,182,5,2,152,253,69,254,180,202,201,5,2,251,150,151,151,1,21,2,39,138,95,51,14,0,27,0,32,64,16,4,27,146,17,1,1,0,26,
    5,145,8,3,18,136,102,35,17,57,47,51,132,108,36,19,35,53,51,19,137,112,33,21,35,135,114,38,35,3,217,136,160,190,104,137,118,34,111,243,105,134,119,38,9,105,2,136,140,1,238,134,124,36,
    3,20,140,254,16,133,125,33,254,18,130,127,36,80,255,233,4,155,130,223,34,43,0,54,130,127,49,17,49,16,145,33,15,50,44,4,5,18,3,43,38,145,0,5,19,72,248,5,34,18,23,57,131,127,32,37,88,
    111,9,32,55,131,134,37,50,30,2,21,20,14,71,38,9,8,107,19,52,46,2,35,3,62,3,3,212,33,98,113,121,56,143,186,108,42,50,85,115,65,143,254,185,1,231,102,175,130,74,79,130,166,173,166,130,
    79,29,70,117,89,55,118,111,98,37,23,36,72,110,74,132,82,153,118,71,64,17,32,24,14,55,91,120,65,78,117,86,62,24,2,95,152,46,86,123,76,115,158,110,70,55,48,66,93,71,38,69,52,30,19,33,
    46,26,3,62,42,73,54,31,253,220,24,54,78,111,138,187,58,47,0,62,0,45,64,24,53,20,145,22,57,16,146,54,37,15,58,48,4,19,19,5,22,3,47,42,138,194,37,57,47,23,57,51,237,68,201,5,140,200,
    38,35,53,51,55,33,53,33,148,204,36,7,51,21,35,7,144,208,35,69,131,164,41,156,211,35,42,169,203,56,145,214,35,1,37,140,174,155,216,34,174,140,234,133,217,33,255,216,65,149,6,58,49,0,
    60,0,46,64,24,55,19,145,21,39,10,146,36,50,56,18,3,21,12,12,5,21,3,49,44,140,217,32,18,144,218,32,39,130,213,33,62,3,65,165,11,131,202,68,242,5,32,55,65,167,14,43,126,174,111,55,9,
    124,127,12,57,81,102,56,65,171,8,46,69,115,150,160,160,135,100,21,216,216,11,44,70,99,68,65,175,19,41,43,73,98,56,140,58,89,68,52,21,65,176,6,45,107,152,107,72,54,45,54,72,53,140,26,
    44,31,18,65,177,16,8,49,1,0,41,0,0,5,28,5,154,0,23,0,26,64,13,17,4,146,20,16,16,11,145,13,3,1,9,18,0,63,51,63,237,50,63,237,51,48,49,33,35,17,16,35,34,7,6,7,17,35,112,239,6,8,51,51,
    54,51,50,22,21,5,28,164,238,123,81,76,4,168,254,99,3,227,254,98,4,116,220,168,178,2,72,1,70,93,88,131,253,170,5,2,152,152,254,84,194,217,205,0,1,0,170,255,232,4,213,130,101,46,13,0,
    15,182,12,6,3,9,145,2,19,0,63,237,63,130,90,39,1,16,33,32,17,17,51,17,133,6,60,4,213,253,223,253,246,168,1,116,1,103,168,2,68,253,164,2,69,3,109,252,158,254,71,1,170,3,113,0,130,71,
    36,72,0,0,3,181,130,71,44,7,0,20,64,9,0,145,4,4,1,6,3,1,71,21,5,8,33,47,237,48,49,1,1,35,19,33,1,51,3,3,181,254,219,180,242,253,122,1,28,180,233,3,37,252,219,2,142,3,12,253,139,130,
    63,32,136,130,237,32,174,130,63,36,25,0,25,64,12,130,229,39,24,145,14,11,16,20,5,0,130,66,35,50,50,63,51,130,238,130,236,33,17,35,130,145,37,35,17,52,0,51,51,130,152,35,51,50,0,21,
    130,246,8,40,16,33,35,17,2,205,57,254,150,162,1,28,240,57,155,57,239,1,30,163,254,150,57,3,128,254,146,253,238,2,18,227,1,31,1,134,254,122,254,225,227,131,13,35,1,110,252,128,140,109,
    46,28,64,13,24,1,145,11,14,14,12,20,5,25,3,12,130,110,35,63,51,51,18,68,67,6,34,1,17,51,130,112,37,51,17,20,0,35,35,130,105,35,35,34,0,53,130,119,44,16,33,51,17,3,104,57,1,106,163,
    254,226,239,130,112,58,240,254,228,162,1,106,57,5,154,252,128,1,110,2,18,253,238,227,254,225,254,122,1,134,1,31,227,131,13,36,254,146,3,128,0,130,115,36,60,0,0,4,160,130,225,39,6,0,
    15,182,3,145,0,5,65,30,5,32,51,65,28,5,53,1,33,53,33,4,160,252,83,183,3,98,253,2,4,0,5,114,250,142,5,2,152,130,53,36,33,255,6,4,252,130,53,50,26,0,31,64,15,0,23,145,25,15,146,1,4,4,
    20,25,3,10,20,130,171,33,206,63,132,170,106,46,5,78,12,6,43,23,35,46,3,35,34,7,6,6,7,35,53,132,86,56,100,253,75,48,102,57,137,219,160,102,20,179,17,75,117,160,102,226,141,20,33,14,
    159,3,65,133,105,51,251,255,16,18,86,166,243,158,136,194,123,58,163,23,49,26,47,4,211,152,131,175,102,15,11,37,3,9,15,0,7,21,85,251,13,8,35,3,35,1,51,1,3,30,254,230,22,10,4,7,29,250,
    180,1,132,161,1,152,2,236,62,77,61,82,253,24,4,0,252,0,0,2,0,137,67,85,249,9,33,15,12,133,74,85,249,14,132,78,35,33,17,51,17,143,82,34,253,217,124,139,85,35,1,82,254,174,140,89,85,
    247,10,136,164,85,247,12,132,89,74,53,15,144,101,32,178,74,40,15,140,116,32,85,74,17,15,57,1,0,20,0,0,3,59,4,51,0,7,0,16,183,6,3,2,3,4,21,0,16,0,63,63,23,85,245,5,32,3,130,117,56,3,
    1,94,1,221,170,229,226,182,1,69,136,4,51,251,205,2,4,253,252,2,191,1,52,0,130,191,57,166,254,214,4,1,4,0,0,33,0,54,0,41,64,20,50,149,30,33,19,30,4,9,149,39,30,130,1,42,3,15,24,24,0,
    21,0,63,50,47,63,74,83,6,43,17,51,51,16,237,48,49,33,35,17,51,21,88,108,7,33,21,20,86,1,8,38,46,3,35,34,38,39,1,99,81,6,32,7,77,139,7,8,97,1,73,163,163,23,32,27,79,52,82,132,93,50,
    26,47,64,38,42,75,35,149,190,122,20,40,43,49,30,64,99,39,1,153,27,53,75,48,49,75,53,28,1,27,52,77,50,48,75,53,27,4,0,202,21,17,14,24,55,100,138,82,57,101,84,63,19,14,91,64,254,184,
    1,38,42,58,36,16,46,33,1,40,49,86,63,37,36,63,83,46,7,48,86,65,38,36,64,87,0,119,45,6,51,248,4,24,0,59,0,33,64,17,23,236,59,21,50,149,32,32,10,42,21,24,72,89,11,36,18,57,47,237,63,
    131,191,32,34,127,19,16,32,22,85,230,16,34,35,19,54,100,27,9,8,102,22,23,19,2,68,112,179,126,67,73,138,197,124,132,106,116,128,86,133,91,48,100,100,20,5,3,55,84,103,48,49,103,84,54,
    3,5,47,151,50,6,23,39,51,27,27,50,39,23,2,3,48,70,127,180,111,122,206,149,83,50,168,80,65,111,149,83,129,183,40,115,25,44,20,75,99,60,25,25,61,101,75,20,42,24,254,247,1,27,34,25,40,
    52,30,11,12,30,52,40,13,28,17,254,230,133,199,33,4,109,95,153,6,40,24,40,43,150,37,46,46,42,51,131,202,35,23,236,60,21,141,206,33,63,237,131,208,130,3,37,51,237,50,48,49,33,158,212,
    38,23,51,21,35,7,35,55,85,246,13,154,212,48,48,100,83,55,3,118,131,41,151,41,122,135,4,25,37,47,25,160,215,43,24,57,95,70,134,233,233,134,32,42,24,10,137,210,8,39,0,2,0,96,255,251,
    4,128,4,40,0,35,0,57,0,32,64,18,32,3,36,12,4,9,35,15,41,149,27,16,53,149,17,21,9,21,0,63,63,237,130,1,85,219,5,36,6,2,7,30,3,85,218,8,65,169,7,37,30,2,23,54,54,55,85,217,9,8,114,4,
    51,50,62,2,4,128,12,38,35,18,26,20,15,6,162,6,17,11,34,86,107,130,78,104,159,107,55,55,107,159,104,78,130,107,86,34,11,17,6,95,25,68,88,112,69,72,102,65,30,12,27,42,60,78,50,69,112,
    88,68,4,18,117,254,246,130,65,132,131,126,58,22,61,132,69,58,97,69,38,93,151,191,99,99,191,152,93,38,69,97,58,68,132,62,253,233,82,142,106,61,72,113,139,66,41,89,87,79,60,35,61,105,
    142,0,3,140,199,46,77,0,43,64,23,73,36,3,12,3,9,32,35,63,63,148,206,35,57,47,18,57,130,211,35,51,48,49,1,170,212,85,215,15,182,228,33,254,88,68,74,15,180,246,32,93,68,114,15,57,0,1,
    0,90,255,232,4,54,4,22,0,44,0,21,64,10,44,18,7,149,36,22,25,149,18,16,85,217,10,81,134,8,85,217,14,98,254,8,8,83,54,55,1,9,21,16,27,35,19,49,69,51,38,17,102,37,62,67,82,56,24,53,30,
    43,19,32,14,32,41,36,41,31,62,35,60,80,117,92,56,93,67,37,21,19,1,106,59,62,38,49,28,11,59,89,103,44,1,11,96,140,90,43,9,8,144,6,2,39,78,119,79,159,91,161,121,70,30,64,102,71,51,79,
    44,0,134,153,35,91,4,109,0,85,219,5,32,25,130,154,36,38,22,27,149,23,131,155,34,204,237,63,90,185,6,74,253,10,36,62,3,51,50,50,85,220,5,36,35,34,14,2,7,156,159,37,5,10,5,124,124,21,
    162,158,36,1,88,254,125,147,144,158,40,1,0,101,0,0,3,207,4,24,85,217,5,34,149,20,16,124,126,6,85,217,24,8,81,3,113,162,80,9,30,69,105,66,65,105,71,31,10,80,161,80,14,50,112,169,106,
    105,169,113,50,14,2,120,73,106,67,32,32,68,108,76,253,142,2,122,106,156,102,50,51,105,160,110,0,1,0,166,254,102,3,228,5,236,0,38,0,35,64,18,36,1,150,30,30,32,38,15,35,0,32,21,17,22,
    149,16,11,85,21,7,41,63,18,57,47,237,50,48,49,1,1,78,0,7,110,49,10,68,247,5,8,81,35,17,35,17,51,17,51,1,3,174,254,76,103,179,132,76,62,119,171,108,42,75,28,33,28,31,34,29,74,40,144,
    148,68,119,162,93,58,164,164,2,1,135,4,0,254,108,15,84,134,184,115,112,184,130,72,11,8,8,11,164,16,12,11,17,181,169,101,150,100,49,254,26,5,236,252,136,1,140,0,1,0,39,134,153,8,36,
    46,0,44,64,23,9,13,150,42,42,11,44,21,29,34,149,28,23,28,11,15,8,46,150,2,5,3,0,0,63,221,50,237,50,63,63,51,130,4,134,162,40,19,53,51,53,51,21,33,21,33,130,139,32,51,157,174,38,39,
    127,164,1,84,254,172,130,146,32,219,155,179,41,4,178,134,180,180,134,253,194,1,140,155,183,8,36,4,178,0,2,0,166,254,30,3,253,5,236,0,50,0,66,0,56,64,30,46,23,30,3,33,43,150,64,51,51,
    8,61,150,33,27,27,33,130,185,44,150,5,5,8,13,15,10,0,8,21,0,63,63,65,87,5,37,63,51,47,16,237,17,85,153,6,38,48,49,37,52,46,2,35,65,78,7,133,194,37,6,7,22,22,21,35,85,168,16,35,62,3,
    7,34,104,150,9,38,38,38,3,70,71,122,164,65,101,6,134,210,8,39,33,30,45,43,122,18,21,59,166,85,80,104,61,25,31,72,117,86,60,119,51,7,9,5,1,248,43,60,37,17,14,30,45,32,69,105,37,44,96,
    86,65,124,11,130,230,8,64,85,135,184,115,75,138,59,56,141,88,50,76,33,84,86,45,67,77,31,43,85,67,41,34,31,20,37,40,48,212,17,28,37,21,16,30,24,15,66,63,29,30,0,1,0,106,255,232,3,186,
    4,42,0,27,0,22,64,10,19,27,5,15,14,16,24,97,157,7,80,95,5,85,117,20,8,108,54,55,3,186,11,72,113,148,86,113,159,100,46,87,80,1,32,120,254,208,50,62,28,62,99,71,118,140,9,1,76,101,137,
    83,35,53,91,123,69,103,182,101,1,112,83,254,118,64,141,68,44,74,54,30,115,103,0,2,0,100,0,0,5,24,4,20,0,52,0,68,0,40,64,20,44,5,149,47,47,64,26,149,41,56,149,0,41,16,16,32,41,15,32,
    21,0,63,63,18,57,47,18,57,237,16,237,50,51,47,237,81,123,7,34,6,35,22,114,48,6,70,41,5,36,55,14,3,7,3,85,136,11,35,30,2,23,5,85,108,7,8,153,38,39,14,3,4,170,20,51,64,78,47,7,12,7,32,
    40,32,66,99,67,57,86,57,28,15,30,47,32,77,103,66,34,8,60,164,60,5,21,40,65,98,138,93,32,76,39,38,87,51,60,108,92,75,26,253,106,46,33,22,37,27,14,36,26,34,45,27,11,2,226,31,60,46,29,
    1,43,118,78,78,126,90,48,43,75,100,58,33,77,80,80,35,3,70,111,140,71,254,4,1,229,44,117,124,120,94,58,14,17,16,17,32,55,75,43,238,79,64,31,58,81,50,63,81,26,26,67,69,63,0,1,255,236,
    255,232,4,124,4,42,0,26,0,27,64,13,10,26,26,11,14,13,16,11,21,23,65,94,6,85,107,15,32,7,85,107,11,54,54,55,4,124,11,72,113,148,86,93,140,99,61,13,149,183,3,7,120,254,186,40,50,65,96,
    12,44,36,64,87,52,215,4,42,83,254,74,54,118,57,65,94,6,34,0,2,0,24,67,45,9,40,23,0,16,183,12,149,6,16,18,101,193,7,32,48,24,67,33,12,25,31,41,11,56,2,82,227,254,241,1,26,240,229,1,
    1,254,235,221,158,184,186,156,159,171,171,24,1,31,237,24,67,25,9,41,3,166,215,189,182,210,206,190,192,208,102,53,11,35,11,0,23,0,85,83,9,139,108,32,17,85,83,5,32,34,24,67,146,9,139,
    112,75,29,15,148,128,33,254,241,70,215,15,151,146,32,254,24,95,139,15,36,0,0,2,0,166,130,168,52,131,4,0,0,20,0,41,0,35,64,17,37,149,17,20,17,4,7,149,26,17,130,1,32,3,120,134,5,74,236,
    8,74,235,9,33,54,51,80,107,5,32,2,74,224,24,45,37,108,68,85,132,92,48,58,99,132,75,75,105,26,74,211,21,45,34,42,55,100,138,82,91,140,95,49,44,25,1,50,74,197,17,32,2,130,149,33,0,3,
    131,157,61,24,0,47,0,50,64,26,43,149,21,24,21,5,35,149,2,38,2,8,11,149,30,21,2,30,30,2,21,3,7,133,166,41,23,57,47,47,47,16,237,50,17,51,134,4,41,48,49,33,35,17,35,53,51,17,51,24,81,
    146,8,32,14,141,176,34,51,21,35,80,79,5,50,1,73,163,146,146,163,37,104,74,85,132,91,47,57,98,128,72,64,112,75,135,6,53,40,65,49,34,9,182,185,8,33,51,67,41,48,75,53,27,1,202,140,1,170,
    202,137,185,33,36,33,133,185,40,24,43,59,34,140,37,63,46,26,75,127,6,56,80,255,193,3,190,4,24,0,31,0,26,64,12,5,23,29,29,12,0,149,23,16,11,12,22,85,45,12,68,221,6,85,43,14,8,72,35,
    38,38,1,249,71,98,61,27,26,43,58,32,1,111,98,254,156,59,89,61,31,49,104,158,110,82,146,120,87,22,162,20,153,3,142,33,54,70,36,49,87,77,68,31,254,158,114,1,73,54,105,109,116,64,70,122,
    90,52,35,83,137,101,103,115,0,2,0,90,255,7,4,110,130,127,56,74,0,90,0,38,64,19,9,43,88,88,41,149,63,12,12,53,2,149,71,16,80,149,26,53,130,136,35,206,237,63,237,75,46,5,85,37,12,32,
    23,85,36,6,85,35,7,116,10,5,75,252,5,111,88,17,85,32,5,34,22,23,1,24,91,14,8,8,186,2,39,6,6,2,243,83,98,46,74,53,29,27,48,110,63,118,179,120,60,9,10,9,27,38,127,31,40,23,9,8,9,8,40,
    77,113,72,65,54,40,86,70,45,51,88,119,69,80,123,85,44,19,41,61,42,23,26,61,101,131,69,68,124,48,253,220,22,44,62,41,68,86,43,67,82,40,47,44,3,109,33,21,38,53,32,47,44,18,20,69,122,
    165,97,29,61,65,65,32,46,86,35,90,28,59,59,60,31,33,68,69,69,35,73,116,81,43,14,34,70,79,91,56,62,95,65,33,47,79,105,59,37,85,85,82,35,38,89,54,72,105,68,32,18,17,253,38,35,61,46,27,
    55,64,32,57,57,62,36,44,104,0,0,2,0,70,0,0,4,61,4,0,0,14,0,24,0,19,64,9,16,1,149,3,15,17,149,0,21,85,19,8,32,19,83,78,8,36,2,35,19,35,3,124,112,5,8,41,194,186,254,202,2,48,114,171,
    113,57,95,174,247,151,171,47,157,43,111,179,127,69,3,116,140,57,104,144,88,144,234,164,89,3,116,253,24,68,128,180,113,255,139,99,51,18,0,32,0,32,64,16,4,21,149,24,1,1,0,20,5,149,7,
    15,25,136,106,85,23,16,67,87,7,131,116,32,21,136,120,35,96,147,177,60,141,123,35,61,187,216,67,132,126,36,1,202,140,1,30,138,129,36,254,226,140,254,194,133,132,38,2,0,90,255,233,3,
    162,130,231,50,41,0,50,0,29,64,15,45,16,149,46,15,33,3,5,18,15,41,36,69,97,5,33,50,63,85,24,19,32,35,84,75,10,32,22,131,251,8,101,55,19,52,38,39,3,62,3,2,238,34,74,73,68,27,83,141,
    102,58,33,58,77,45,96,192,1,92,93,140,95,47,58,96,121,128,121,96,58,112,107,21,69,78,79,32,14,108,100,85,57,106,81,49,27,12,19,12,7,20,54,94,74,51,78,58,42,17,1,163,140,36,68,95,58,
    80,114,81,54,41,35,41,56,41,54,57,9,18,25,17,2,66,72,54,2,254,141,17,38,55,77,0,0,137,175,54,45,0,58,0,37,64,19,53,16,50,37,15,54,3,18,18,5,20,149,22,15,45,40,135,179,42,237,18,57,
    47,23,57,51,205,50,48,49,85,7,11,35,55,35,53,51,130,3,85,7,9,137,187,85,5,6,142,191,35,47,120,148,21,151,194,35,22,111,139,35,145,197,34,207,120,92,152,198,34,96,120,155,134,199,33,
    255,250,65,119,6,48,46,0,55,0,43,64,22,50,19,149,21,39,11,36,18,51,47,84,252,5,34,15,46,41,65,126,8,34,57,47,18,132,202,32,16,86,158,10,84,250,7,65,135,9,131,191,65,137,19,43,75,129,
    99,66,10,101,104,9,39,54,66,37,65,141,7,43,47,79,103,113,114,100,79,22,136,133,49,146,65,144,17,41,16,42,72,57,120,35,55,44,33,14,65,145,6,42,72,106,78,55,41,33,33,38,27,120,51,65,
    145,16,8,35,1,0,30,0,0,4,154,4,0,0,19,0,27,64,13,11,9,150,0,2,19,14,149,16,15,6,12,21,0,63,51,63,253,50,220,50,131,183,34,1,54,51,84,3,5,33,35,34,84,243,8,8,45,1,246,180,152,1,88,164,
    194,148,170,164,254,204,3,11,254,205,2,100,98,254,148,254,166,1,82,225,86,254,35,3,116,140,140,0,0,1,0,144,255,232,3,182,4,0,0,84,233,5,36,15,9,149,2,22,84,233,15,63,51,50,17,17,51,
    3,182,254,100,254,118,161,242,241,162,1,166,254,66,1,178,2,102,253,162,254,208,1,42,2,100,0,130,69,52,70,0,0,3,28,4,0,0,7,0,20,64,9,7,149,4,4,1,6,15,1,130,161,8,34,63,18,57,47,237,
    48,49,1,3,35,19,33,19,51,3,3,28,226,164,172,254,4,217,165,162,2,86,253,170,1,201,2,55,254,86,130,61,32,152,130,227,32,160,130,61,52,29,0,28,64,13,27,0,236,16,13,13,14,21,6,29,21,14,
    15,0,63,63,84,119,10,46,14,3,21,21,35,53,52,62,2,55,17,51,17,22,18,132,12,8,56,46,2,39,17,35,2,82,82,107,62,25,166,54,110,166,112,148,224,218,166,24,63,107,82,148,2,30,5,62,101,132,
    74,168,168,117,186,132,76,6,1,83,254,173,13,254,243,235,168,168,74,132,101,62,5,253,226,0,140,119,51,26,64,12,2,29,236,13,16,16,15,21,23,8,0,15,0,63,50,50,63,84,237,7,46,51,17,62,3,
    53,53,51,21,20,2,7,17,35,17,46,133,11,60,30,2,23,2,82,148,82,107,63,24,166,218,224,148,112,166,110,54,166,25,62,107,82,4,0,253,226,4,63,132,118,44,235,254,243,13,254,173,1,83,6,75,
    132,187,117,132,118,33,63,4,130,115,47,50,0,0,3,167,4,0,0,6,0,15,182,3,149,0,5,65,38,5,84,239,10,49,3,167,253,60,177,2,132,253,219,3,22,3,209,252,47,3,116,140,130,53,36,33,255,136,
    3,212,130,53,55,24,0,27,64,13,5,150,19,19,8,15,11,149,13,15,0,8,21,0,63,206,63,237,50,65,102,5,36,5,46,3,35,34,84,224,7,32,21,84,248,7,44,3,41,11,61,87,110,60,111,146,38,152,2,94,131,
    86,8,65,254,28,24,50,27,96,164,126,83,14,120,81,117,75,35,103,85,51,3,65,140,47,253,99,5,6,57,111,164,107,0,1,255,128,4,194,0,220,5,184,0,3,0,10,178,1,128,0,0,47,26,205,48,49,3,55,
    51,7,128,180,168,220,4,194,246,246,130,35,32,2,130,35,34,254,5,47,130,35,37,8,177,3,1,0,47,130,33,52,19,33,53,33,254,254,4,1,252,4,194,109,0,2,254,221,4,194,1,37,6,130,238,47,0,7,0,
    14,180,0,4,128,2,6,0,47,51,26,205,50,66,15,5,50,35,3,35,19,1,37,246,96,209,115,239,97,207,6,0,254,194,1,62,131,3,32,0,130,91,32,158,130,91,40,99,5,133,0,11,0,8,177,6,130,126,131,91,
    8,61,34,38,53,52,54,51,50,22,21,20,6,1,40,59,58,41,41,57,57,4,194,57,42,40,56,55,41,41,58,0,2,0,86,255,232,5,102,5,178,0,17,0,24,0,30,64,15,1,145,19,19,4,22,145,15,19,6,4,145,7,9,4,
    90,131,5,32,237,65,38,5,42,19,33,38,0,35,34,7,53,54,51,32,95,206,6,8,63,37,33,22,18,51,50,0,86,4,94,17,254,229,218,216,156,158,226,1,63,1,109,254,148,254,211,254,209,254,184,4,94,252,
    86,4,242,207,207,1,2,3,26,237,1,19,96,164,84,254,118,254,165,254,190,254,93,1,159,252,235,254,231,1,24,130,127,42,82,255,232,3,207,4,24,0,18,0,25,132,127,45,149,20,20,4,23,149,15,22,
    6,4,149,7,9,16,143,127,32,38,133,127,35,50,18,21,20,130,138,8,88,2,53,5,33,22,22,51,50,54,82,2,211,4,176,154,174,144,134,223,217,248,254,241,200,203,219,2,209,253,215,2,132,122,116,
    163,2,41,171,186,114,154,98,254,234,254,242,254,214,1,6,229,53,141,159,166,255,255,0,22,254,112,5,18,5,154,2,38,0,36,0,0,0,7,0,223,3,121,0,0,255,255,0,90,254,112,3,131,4,24,130,23,
    32,68,133,23,33,2,0,132,23,36,48,254,112,1,240,132,47,32,44,130,16,40,6,0,223,18,0,255,255,0,55,130,21,34,102,5,217,130,45,32,76,133,21,33,249,0,130,209,32,94,130,91,34,170,5,178,130,
    207,8,46,37,0,28,64,14,26,145,20,4,32,145,0,14,19,6,4,150,7,9,0,47,51,237,50,63,51,237,63,237,48,49,5,6,21,20,51,50,55,21,6,35,34,38,53,52,55,36,97,35,21,46,3,72,128,68,30,26,30,52,
    76,92,131,254,229,254,172,97,44,8,34,185,254,167,97,44,10,50,20,126,77,70,13,107,13,80,70,111,117,17,1,141,1,52,1,90,1,97,55,5,36,186,254,107,5,22,97,55,16,40,2,0,96,254,112,4,80,4,
    24,136,165,39,149,20,16,32,149,0,14,22,159,165,33,38,2,24,79,133,7,34,2,1,34,130,192,42,22,51,50,54,53,52,38,2,162,129,67,134,165,33,208,245,76,107,5,34,230,254,244,76,107,6,34,20,
    128,75,135,156,35,14,1,28,224,76,118,6,36,222,254,223,3,137,76,118,7,8,33,255,255,0,94,254,112,5,170,6,139,2,38,4,197,0,0,1,7,0,217,1,206,1,92,0,8,179,2,39,5,38,0,43,53,130,33,132,
    179,33,5,47,130,33,36,198,0,0,0,7,130,33,48,38,0,0,255,255,0,188,254,112,3,180,5,154,2,38,0,40,132,23,34,223,2,15,132,23,130,47,35,3,221,4,24,130,23,32,72,133,23,33,1,161,132,23,32,
    170,130,251,32,213,132,47,32,56,133,23,33,2,16,132,23,32,144,130,71,34,226,4,0,130,47,32,88,134,23,32,144,132,23,36,55,254,112,1,75,132,23,32,214,65,231,8,38,1,255,67,254,112,0,87,
    130,15,62,14,0,27,64,13,123,11,1,0,6,2,194,12,4,226,9,14,18,0,63,47,237,1,47,241,214,198,48,49,93,51,65,228,13,33,87,150,65,204,6,34,157,141,82,65,19,5,8,41,122,128,0,1,255,236,0,0,
    3,133,5,154,0,24,0,43,64,23,5,145,19,4,20,145,22,1,22,63,19,79,19,2,19,22,19,22,12,24,18,12,3,0,97,97,6,46,93,17,51,16,237,50,16,237,48,49,51,17,35,53,51,24,110,21,12,8,44,21,33,21,
    33,17,100,120,120,84,129,204,141,75,168,85,162,235,151,1,122,254,134,1,9,164,1,32,76,143,202,126,170,170,151,251,183,111,12,127,164,254,247,0,255,255,132,117,46,161,7,66,16,38,4,207,
    0,0,17,7,1,78,2,193,130,207,48,8,179,1,40,5,38,0,43,53,0,1,0,100,0,0,4,2,135,151,53,16,7,145,9,13,9,17,145,6,63,9,79,9,2,9,6,9,6,11,24,18,11,137,151,33,16,237,132,153,130,151,41,53,
    52,62,2,55,53,33,53,33,17,130,158,52,21,35,17,35,34,14,2,21,21,100,85,162,236,150,254,139,1,117,168,125,125,132,160,36,170,151,250,185,110,130,141,40,1,9,254,247,164,254,224,77,142,
    130,161,130,151,133,117,132,151,32,209,149,151,33,255,231,65,47,8,53,41,64,22,8,16,145,14,11,14,6,145,18,63,14,79,14,2,14,18,14,18,13,101,215,5,36,57,57,47,47,93,136,149,33,33,53,81,
    191,6,32,53,130,148,8,34,17,33,21,33,21,30,3,21,21,2,221,75,142,204,128,84,125,125,168,1,117,254,139,152,235,162,84,170,127,202,142,76,1,32,164,132,149,38,127,12,111,185,250,150,170,
    130,149,133,115,132,149,32,211,130,149,37,6,1,78,72,0,0,65,43,14,47,5,54,5,154,0,18,0,32,64,15,10,11,17,145,1,11,130,1,42,8,18,18,12,8,3,0,63,51,63,18,78,143,5,35,17,51,48,49,65,180,
    8,8,50,1,7,1,14,3,7,17,100,84,129,204,141,75,168,1,177,119,254,196,9,93,160,226,143,2,205,76,143,202,126,170,254,81,118,1,58,138,227,166,101,11,253,212,255,255,0,100,0,0,5,54,132,129,
    32,213,65,23,13,32,34,65,175,10,134,131,40,30,64,14,1,2,11,145,8,2,130,1,34,9,3,3,130,149,34,63,50,63,137,129,40,33,1,55,1,62,3,55,17,51,65,155,6,44,2,21,254,79,119,1,60,9,93,161,226,
    142,168,132,141,45,1,175,118,254,198,138,227,167,100,11,2,44,253,51,132,142,65,155,6,33,5,82,132,131,32,215,133,131,33,4,114,66,51,5,146,131,39,28,64,13,7,145,17,16,10,130,1,34,8,3,
    15,137,130,36,17,51,237,48,49,65,142,7,39,51,17,30,3,23,1,23,1,65,136,6,61,168,143,227,160,92,9,1,60,119,254,79,170,127,202,142,76,2,205,253,212,11,101,167,227,137,1,58,118,254,81,
    131,129,32,72,65,5,8,32,217,65,135,11,139,127,55,4,167,5,154,0,9,0,21,64,10,1,7,145,9,18,6,2,145,4,3,0,63,237,50,98,55,5,59,53,1,33,53,33,21,1,33,21,100,3,45,253,22,4,0,252,204,3,32,
    47,4,199,164,40,251,50,164,66,243,6,136,63,33,8,2,130,63,35,3,6,145,5,139,63,33,33,1,130,63,53,33,1,21,120,3,32,252,204,4,0,253,22,3,45,164,4,206,40,164,251,57,47,130,63,32,200,130,
    159,32,76,130,127,32,23,131,127,40,23,18,6,14,18,12,3,18,1,130,63,8,75,51,63,63,51,63,48,49,51,17,51,1,22,22,23,51,38,38,53,17,51,17,35,1,38,38,39,35,22,22,21,17,200,218,2,206,23,28,
    7,4,5,5,168,207,253,30,14,22,9,6,4,4,5,154,251,162,35,48,13,31,105,75,3,235,250,102,4,112,22,45,23,23,98,76,251,251,0,143,103,40,6,23,18,14,18,18,11,3,2,130,103,130,101,32,63,66,100,
    5,35,17,20,6,7,107,218,5,8,42,17,35,17,52,54,55,35,6,6,7,1,200,168,5,5,4,7,28,23,2,206,218,168,4,4,6,9,22,14,253,30,5,154,252,21,75,105,31,13,48,35,4,94,130,103,40,5,76,98,23,23,45,
    22,251,144,131,103,49,95,1,102,2,129,4,51,0,25,0,19,183,3,19,76,6,6,13,130,124,35,47,205,50,57,83,188,5,130,89,121,72,6,8,53,53,51,21,20,30,2,51,50,62,2,53,53,2,129,130,20,83,60,64,
    96,62,31,130,22,38,52,29,31,53,39,22,4,51,253,51,1,38,17,33,35,64,90,55,229,226,35,50,32,15,15,32,50,35,226,130,76,32,75,130,97,32,57,130,97,35,3,0,8,177,130,192,45,47,205,48,49,19,
    1,51,1,75,1,100,138,254,156,130,27,34,205,253,51,143,37,34,0,0,47,130,37,32,1,130,37,38,1,175,254,156,138,1,100,134,38,130,39,32,120,131,10,130,77,40,23,0,14,181,10,218,12,2,218,130,
    43,34,237,222,237,130,83,34,53,51,50,87,19,6,130,9,8,46,30,2,21,20,14,2,35,120,232,117,114,29,57,87,58,232,242,85,132,91,47,47,91,132,85,1,102,122,125,112,48,86,65,37,122,56,96,131,
    75,74,131,97,57,0,0,2,0,130,163,33,3,87,132,163,38,7,0,12,179,1,6,7,130,85,36,50,206,50,48,49,131,131,32,33,131,135,32,105,132,174,33,254,88,138,181,32,2,131,185,34,2,0,140,130,145,
    32,28,136,59,33,6,1,132,59,32,205,131,59,35,17,51,17,33,130,3,37,1,154,130,254,112,130,133,194,132,51,38,1,0,90,1,102,3,40,130,51,44,11,0,23,64,10,11,10,218,7,1,218,4,4,130,117,38,
    47,205,51,47,237,16,253,131,252,32,17,69,96,5,44,33,21,33,17,1,128,254,218,1,38,130,1,38,130,6,42,102,1,42,122,1,41,254,215,122,254,214,24,225,145,14,35,6,1,218,3,130,182,33,220,237,
    132,122,130,61,131,57,38,72,254,244,2,154,254,244,130,124,37,83,122,122,253,173,0,130,51,130,175,33,1,14,132,175,34,8,177,2,130,48,130,108,32,19,130,165,32,140,134,161,130,33,36,130,
    1,82,2,112,130,33,40,17,0,15,181,13,13,4,9,217,130,37,41,237,204,57,47,48,49,1,34,38,53,130,43,8,37,20,22,51,50,54,53,53,51,21,20,6,1,121,118,129,130,51,67,57,59,130,129,1,82,125,106,
    1,250,254,18,60,64,64,60,80,92,106,125,130,73,32,95,130,120,32,180,65,173,6,36,12,218,10,22,218,131,73,33,220,237,131,72,86,8,5,39,51,51,21,35,34,14,2,21,130,81,35,51,21,1,194,65,165,
    7,58,242,232,58,87,57,29,114,117,232,1,102,57,97,131,74,75,131,96,56,122,37,65,86,48,112,125,122,130,85,39,0,1,253,0,160,3,157,0,66,81,10,32,17,130,149,38,160,1,253,1,160,254,96,131,
    227,8,45,51,3,189,2,75,5,154,0,7,0,14,181,4,7,146,5,1,3,0,63,204,237,50,48,49,19,53,33,21,35,17,35,17,51,2,24,188,160,5,6,148,148,254,183,1,73,132,49,37,0,0,2,75,1,221,132,49,37,1,
    4,146,3,6,18,134,49,33,55,51,130,90,46,51,21,33,51,188,160,188,253,232,148,1,73,254,183,148,130,131,38,10,1,193,1,231,3,217,130,47,43,13,180,7,5,146,2,4,0,47,206,253,206,130,96,32,
    17,130,43,55,21,33,21,10,160,1,61,254,195,1,193,2,24,194,148,194,0,1,254,25,1,193,255,246,134,47,54,3,4,146,1,6,0,47,204,253,205,48,49,3,51,17,35,53,33,53,33,170,160,160,130,46,39,
    61,3,217,253,232,194,148,0,131,145,50,0,32,186,0,1,5,114,24,0,0,10,8,172,0,5,0,85,255,205,130,5,34,86,255,190,130,5,32,228,130,5,32,10,132,17,33,10,0,131,17,33,10,0,131,17,42,11,0,
    77,0,233,0,13,0,36,255,90,130,5,34,45,255,102,130,5,34,70,255,154,130,5,32,71,132,5,32,72,132,5,32,74,132,5,32,82,132,5,32,84,130,5,40,15,0,180,255,49,0,15,0,181,132,5,32,182,132,5,
    32,183,130,5,32,17,132,23,32,17,130,23,35,61,0,17,0,131,23,42,17,0,183,255,61,0,36,0,13,255,127,130,5,34,15,0,68,130,5,32,30,132,5,34,38,255,229,130,11,32,42,132,5,34,45,0,94,130,11,
    32,50,132,11,34,55,255,109,130,11,32,56,132,11,34,57,255,139,130,11,34,58,255,182,130,5,34,60,255,100,130,5,34,61,0,59,130,5,32,87,132,29,34,89,255,213,130,11,32,90,132,11,34,92,255,
    219,130,11,32,141,130,155,32,36,130,131,32,102,130,11,34,181,255,63,130,5,32,182,130,209,32,36,130,131,131,11,34,186,255,193,130,17,32,187,132,71,32,229,132,71,32,234,132,11,32,235,
    130,59,42,37,0,55,255,164,0,37,0,60,255,190,130,5,32,187,132,5,34,196,255,178,130,11,32,197,132,5,32,234,130,17,42,38,0,34,0,2,0,38,0,38,255,201,130,5,32,42,132,5,32,50,130,125,34,
    38,0,52,132,11,32,100,132,5,32,103,132,5,34,119,0,29,130,35,32,146,132,11,34,170,255,207,130,11,32,175,132,11,32,176,132,5,32,190,132,17,32,208,132,11,32,209,132,5,32,210,130,5,40,
    39,0,15,255,127,0,39,0,17,132,5,34,36,255,223,130,11,131,149,36,39,0,59,255,203,130,11,32,61,130,53,34,39,0,98,132,23,32,99,132,5,34,145,255,182,130,23,32,172,132,47,32,173,132,17,
    32,174,132,5,34,187,255,246,130,23,34,196,255,139,130,5,32,197,132,5,32,199,132,23,32,201,132,5,32,229,130,71,42,40,0,36,0,10,0,40,0,45,0,68,130,5,34,55,0,4,130,5,32,58,130,179,36,
    40,0,59,0,8,130,11,32,98,132,29,32,99,132,5,34,119,0,18,130,17,32,173,132,11,32,174,132,5,32,199,132,5,32,201,130,5,32,41,130,179,34,102,0,41,130,179,131,5,34,36,255,123,130,11,34,
    45,255,190,130,5,34,54,255,229,130,5,34,55,0,14,130,5,34,68,255,180,130,5,32,73,132,47,32,98,132,35,32,99,132,5,34,119,0,39,130,23,34,145,255,76,130,5,33,172,255,131,65,32,173,132,
    23,32,174,132,5,34,196,255,74,130,23,32,197,132,5,32,199,132,17,32,201,132,5,32,227,130,89,34,42,0,55,130,197,34,42,0,57,132,11,32,92,132,5,32,186,132,5,32,235,130,5,32,45,130,149,
    32,154,130,219,33,17,255,131,5,34,36,255,219,130,11,131,149,130,237,132,29,32,98,132,17,32,99,132,5,32,105,132,17,32,106,132,5,32,107,132,5,32,108,132,5,32,109,132,5,32,110,132,5,34,
    145,255,152,130,65,32,160,132,11,32,172,132,83,32,173,132,59,32,174,132,5,32,196,130,185,34,45,0,197,132,23,32,199,132,17,32,201,130,5,34,46,0,15,130,221,34,46,0,30,132,5,38,38,255,
    166,0,46,0,42,132,5,34,45,0,90,130,11,32,50,132,11,32,52,132,5,34,59,0,37,130,17,32,61,132,41,32,70,130,101,34,46,0,71,132,5,32,72,132,5,32,74,132,5,32,82,132,5,32,84,132,5,34,87,255,
    209,130,47,34,89,255,182,130,5,34,90,255,203,130,5,34,92,255,164,130,5,32,100,130,131,34,46,0,103,132,83,32,111,132,41,32,112,132,5,32,113,132,5,32,114,132,5,32,115,132,5,34,116,255,
    207,130,47,32,119,132,113,32,121,132,17,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,146,132,77,32,175,132,5,32,176,132,5,32,177,132,23,32,186,132,113,34,196,0,68,130,71,
    32,197,132,5,32,208,132,29,32,209,132,5,32,210,132,5,32,229,132,95,32,235,130,41,40,47,0,13,255,49,0,47,0,34,130,161,36,47,0,36,0,59,130,11,34,38,255,190,130,5,32,42,132,5,34,45,0,
    100,130,11,34,50,255,186,130,5,32,52,132,5,34,55,255,143,130,11,34,56,255,227,130,5,34,57,255,139,130,5,32,58,130,179,36,47,0,60,255,127,130,11,32,61,132,65,32,87,130,137,34,47,0,89,
    132,83,32,90,132,71,34,92,255,180,130,29,32,100,132,11,32,103,132,71,32,104,132,65,32,141,132,35,32,146,132,17,32,175,132,5,32,176,132,5,34,180,255,115,130,47,32,181,132,95,32,182,
    132,11,32,183,132,95,32,186,132,71,32,187,132,53,131,227,33,47,0,131,227,34,47,0,208,132,53,32,209,132,5,32,210,132,5,32,211,132,95,32,212,132,5,32,213,132,5,32,229,132,155,32,234,
    132,71,32,235,130,71,40,50,0,15,255,164,0,50,0,17,132,5,32,36,130,179,36,50,0,45,255,246,130,17,32,55,132,17,34,59,255,219,130,11,34,60,255,231,130,5,32,61,130,227,34,50,0,98,132,35,
    32,99,132,5,32,172,132,35,32,173,132,11,32,174,132,5,32,187,132,59,34,196,255,76,130,47,34,197,255,152,130,5,32,199,132,23,32,201,132,5,32,229,132,65,32,234,130,77,40,51,0,15,254,186,
    0,51,0,17,132,5,34,36,255,98,130,11,32,42,130,59,32,51,130,125,32,127,130,11,34,58,0,39,130,5,34,59,255,195,130,5,34,68,255,190,130,5,32,70,130,173,34,51,0,71,132,5,32,72,132,5,32,
    74,132,5,32,82,132,5,34,84,255,182,130,35,32,98,132,71,32,99,132,5,32,105,132,53,32,106,132,5,32,107,132,5,32,108,132,5,32,109,132,5,32,110,132,5,32,111,132,59,32,112,132,5,32,113,
    132,5,32,114,132,5,32,115,132,5,32,121,132,5,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,34,145,254,233,130,113,32,160,132,71,32,172,132,197,32,173,132,119,32,174,132,5,32,
    177,132,35,32,196,132,23,34,197,254,174,130,41,32,199,132,23,32,201,130,5,42,52,0,15,255,164,0,52,0,17,255,127,130,5,34,36,255,229,130,5,32,55,132,17,34,59,255,219,130,11,34,60,255,
    246,130,5,34,61,255,207,130,5,32,98,132,29,32,99,132,5,32,172,132,47,32,173,132,11,32,174,132,5,32,187,132,41,34,196,255,152,130,41,32,197,132,5,32,199,132,23,32,201,132,5,32,229,132,
    65,32,234,130,35,42,53,0,30,0,82,0,53,0,38,255,227,130,5,32,42,132,5,34,45,0,57,130,11,34,50,255,236,130,5,32,52,132,5,34,55,255,203,130,11,34,60,255,217,130,5,32,70,132,11,32,71,132,
    5,34,72,255,199,130,17,32,74,132,5,34,82,255,197,130,11,32,84,132,23,32,100,130,101,34,53,0,103,132,59,34,111,255,190,130,23,32,112,132,5,32,113,132,5,32,114,132,5,32,115,132,5,32,
    121,132,53,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,146,132,65,32,175,132,5,32,176,132,5,32,177,132,23,32,187,132,95,32,208,132,17,32,209,132,5,32,210,132,5,32,234,130,
    161,34,54,0,87,130,89,34,54,0,89,130,227,34,54,0,90,130,41,40,54,0,92,255,209,0,54,0,186,132,23,34,196,255,178,130,11,32,197,132,5,32,235,130,23,42,55,0,15,255,127,0,55,0,17,255,76,
    130,5,34,29,255,233,130,5,32,30,132,5,34,36,255,102,130,11,34,38,255,164,130,5,32,42,132,5,34,45,255,143,130,11,32,50,132,11,32,52,132,5,34,55,0,39,130,17,34,57,0,43,130,5,32,58,132,
    11,34,59,255,250,130,11,34,60,0,29,130,5,33,68,255,131,29,34,70,255,45,130,11,32,71,132,5,32,72,132,5,34,73,255,160,130,17,32,74,132,11,34,80,255,78,130,11,32,81,132,5,32,82,132,17,
    32,83,132,11,32,84,132,11,32,85,132,11,32,86,132,137,32,88,132,11,34,89,255,154,130,47,32,90,132,137,32,91,132,179,32,92,132,11,32,93,132,197,32,98,132,41,32,99,132,5,32,100,132,161,
    32,103,132,5,32,105,132,137,32,106,132,5,32,107,132,5,32,108,132,5,34,109,255,63,130,77,32,110,132,11,34,111,255,27,130,11,32,112,132,119,32,113,132,5,32,114,132,5,32,115,132,5,34,
    116,255,233,130,29,34,118,0,49,130,5,34,119,0,111,130,5,32,120,132,143,32,121,132,29,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,126,132,35,32,127,132,5,32,128,132,5,32,
    129,132,5,34,141,0,82,130,65,34,145,255,12,130,5,32,146,132,161,32,160,132,131,32,170,132,221,34,171,255,207,130,23,32,172,132,221,32,173,132,203,32,174,132,5,32,175,132,41,32,176,
    132,5,32,177,132,95,34,181,0,39,130,41,32,183,132,5,34,186,255,143,130,11,32,190,132,65,32,191,132,65,34,196,255,23,130,17,32,197,132,5,32,199,132,65,32,201,132,5,32,208,132,65,32,
    209,132,5,32,210,132,5,32,228,132,23,34,230,255,127,130,47,34,234,0,29,130,5,32,235,130,77,42,56,0,36,255,215,0,56,0,145,255,139,130,5,34,196,255,166,130,5,32,197,130,5,42,57,0,15,
    255,51,0,57,0,17,255,27,130,5,32,36,130,29,36,57,0,38,255,213,130,11,32,42,132,5,34,45,255,186,130,11,34,50,255,244,130,5,32,52,132,17,34,54,255,229,130,11,32,55,130,167,36,57,0,68,
    255,109,130,11,32,70,130,107,34,57,0,71,132,5,32,72,132,5,32,74,132,5,34,80,255,180,130,29,32,81,132,5,32,82,132,17,32,83,132,11,32,84,132,11,32,85,132,11,34,86,255,190,130,35,32,88,
    132,11,32,98,132,125,32,99,132,5,34,100,255,225,130,23,32,103,132,119,32,105,132,101,32,106,132,5,32,107,132,5,32,108,132,5,32,109,132,5,32,110,132,5,32,111,132,83,32,112,132,5,32,
    113,132,5,32,114,132,5,32,115,132,5,34,118,0,29,130,77,34,119,0,90,130,5,32,120,132,107,32,121,132,23,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,126,132,35,32,127,132,5,
    32,128,132,5,32,129,132,5,34,141,0,82,130,65,34,145,255,68,130,5,32,146,132,155,32,160,132,125,34,162,255,117,130,17,34,170,255,207,130,5,34,172,255,27,130,5,32,173,132,197,32,174,
    132,5,32,175,132,41,32,176,132,5,32,177,132,95,32,190,132,41,34,196,255,115,130,41,32,197,132,17,32,199,132,41,32,201,132,5,32,208,132,41,32,209,132,5,32,210,132,5,34,227,255,229,130,
    41,38,228,255,190,0,58,0,15,130,35,34,58,0,17,130,53,36,58,0,36,255,182,130,17,34,55,0,39,130,5,32,68,130,161,34,58,0,70,130,89,34,58,0,71,132,5,32,72,132,5,32,74,132,5,32,82,132,5,
    32,84,132,5,32,98,132,53,32,99,132,5,32,105,132,53,32,106,132,5,32,107,132,5,32,108,132,5,32,109,132,5,32,110,132,5,32,111,132,53,32,112,132,5,32,113,132,5,32,114,132,5,32,115,132,
    5,34,118,0,49,130,125,34,119,0,100,130,5,32,121,132,17,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,34,145,255,154,130,35,32,160,132,83,34,162,255,164,130,11,32,170,132,23,32,
    171,132,191,32,172,132,209,32,173,132,149,32,174,132,5,32,177,132,29,32,190,132,5,32,191,132,35,32,196,132,35,34,197,255,139,130,59,32,199,132,35,32,201,130,5,42,59,0,15,0,68,0,59,
    0,17,0,57,130,5,34,30,0,82,130,5,34,38,255,233,130,5,32,42,132,5,34,45,0,96,130,11,32,50,132,11,32,52,132,5,34,55,0,33,130,17,32,100,132,11,32,103,132,5,32,119,132,53,32,146,132,11,
    32,172,132,71,32,175,132,11,32,176,132,5,32,196,132,95,32,197,132,35,32,208,132,17,32,209,132,5,32,210,130,5,42,60,0,15,255,80,0,60,0,17,255,61,130,5,34,36,255,98,130,5,34,38,255,211,
    130,5,32,42,132,5,34,45,255,190,130,11,32,50,132,11,32,52,132,5,34,54,255,229,130,17,32,55,130,209,36,60,0,68,255,57,130,11,34,70,255,76,130,5,32,71,132,5,32,72,132,5,32,73,132,35,
    32,74,132,11,34,80,255,115,130,29,32,81,132,5,32,82,132,17,32,83,132,11,32,84,132,11,32,85,132,11,34,86,255,123,130,35,32,88,132,11,32,98,132,131,32,99,132,5,34,100,255,205,130,23,
    32,103,132,119,32,105,132,107,32,106,132,5,32,107,132,5,34,108,255,127,130,29,34,109,255,102,130,5,32,110,132,17,32,111,132,83,32,112,132,5,32,113,132,5,32,114,132,5,32,115,132,5,34,
    119,0,90,130,41,32,120,132,101,32,121,132,17,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,126,132,35,32,127,132,5,32,128,132,5,32,129,132,5,34,145,255,63,130,65,32,146,132,
    143,32,160,132,113,34,162,255,27,130,17,34,170,255,207,130,5,34,172,255,61,130,5,32,173,132,185,32,174,132,5,32,175,132,41,32,176,132,5,32,177,132,89,34,190,255,154,130,35,34,196,255,
    8,130,5,32,197,132,5,32,199,132,41,32,201,132,5,32,208,132,41,32,209,132,5,32,210,132,5,34,227,255,229,130,41,40,228,255,123,0,61,0,45,0,82,130,5,34,55,0,39,130,5,34,92,255,203,130,
    5,34,119,0,70,130,5,32,171,132,17,34,186,255,190,130,11,32,191,132,35,32,235,130,29,40,62,0,77,0,233,0,69,0,68,130,65,36,69,0,73,255,246,130,11,34,91,255,231,130,5,32,105,132,17,32,
    106,132,5,32,107,132,5,32,108,132,5,32,109,132,5,32,110,132,5,32,160,130,5,32,70,130,113,32,70,130,97,32,55,130,179,36,70,0,60,255,180,130,11,32,234,130,5,40,72,0,5,255,152,0,72,0,
    10,130,5,42,73,0,12,0,141,0,73,0,15,255,127,130,5,32,16,130,41,34,73,0,17,132,11,32,29,130,137,34,73,0,30,132,5,34,34,0,66,130,29,32,64,132,41,34,69,0,18,130,11,32,75,132,5,34,87,0,
    37,130,11,32,89,130,191,34,73,0,90,132,5,32,91,132,23,34,92,0,33,130,23,32,96,132,59,32,117,130,133,34,73,0,118,132,23,34,119,0,121,130,23,34,141,0,147,130,5,32,171,132,47,32,172,132,
    107,34,180,0,94,130,17,32,181,132,47,32,182,132,11,32,183,132,11,32,191,132,35,34,196,255,176,130,29,32,197,132,5,32,235,132,89,32,237,130,77,36,74,0,77,0,47,130,3,38,77,0,35,0,78,
    0,15,130,47,36,78,0,16,255,117,130,11,32,17,132,11,131,191,32,78,132,191,36,78,0,70,255,215,130,23,34,71,255,229,130,5,32,72,132,11,32,74,132,5,32,82,132,5,32,84,132,23,34,87,255,240,
    130,29,32,111,132,11,32,112,132,23,32,113,132,5,32,114,132,5,32,115,132,5,32,121,132,5,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,172,132,119,32,177,130,11,40,81,0,5,255,
    152,0,81,0,10,130,5,32,82,130,11,32,111,130,25,33,10,255,131,5,32,68,130,95,36,82,0,73,255,219,130,17,34,91,255,231,130,5,32,105,132,17,32,106,132,5,32,107,132,5,32,108,132,5,32,109,
    132,5,32,110,132,5,32,160,132,5,34,180,255,178,130,47,34,181,255,125,130,5,34,182,255,190,130,5,32,183,130,11,33,83,0,131,83,32,83,132,83,33,83,0,131,83,33,83,0,131,83,34,83,0,106,
    130,53,34,83,0,107,132,5,32,108,132,5,32,109,132,5,32,110,132,5,131,83,42,83,0,180,255,203,0,83,0,181,255,137,130,5,32,182,132,5,32,183,130,5,42,84,0,77,0,102,0,85,0,15,255,98,130,
    5,34,16,255,127,130,5,34,17,255,86,130,5,32,29,130,117,34,85,0,30,132,5,32,70,130,71,34,85,0,71,132,5,32,72,132,5,34,73,0,39,130,35,32,74,132,11,34,80,255,252,130,11,32,81,132,5,32,
    82,132,17,32,84,132,5,34,86,0,14,130,23,34,87,0,59,130,5,32,89,132,71,32,90,132,5,32,91,132,17,32,92,132,11,32,93,132,71,32,111,132,47,32,112,132,5,32,113,132,5,32,114,132,5,32,115,
    132,5,32,121,132,5,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,172,132,173,32,177,132,11,34,180,0,164,130,107,34,181,0,121,130,5,32,182,132,11,32,183,132,11,32,196,132,221,
    32,197,132,5,32,228,132,149,32,230,132,119,32,235,130,131,42,87,0,16,255,143,0,87,0,34,255,203,130,5,32,70,130,71,32,87,132,233,36,87,0,72,255,240,130,17,32,74,132,5,32,82,132,5,32,
    84,132,5,34,91,0,29,130,23,32,111,132,41,32,112,132,17,32,113,132,5,32,114,132,5,32,115,132,5,32,121,132,5,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,171,130,125,34,87,
    0,177,132,11,32,191,130,11,40,88,0,5,255,190,0,88,0,10,130,5,42,89,0,15,255,139,0,89,0,17,255,127,130,5,34,68,255,219,130,5,34,70,255,244,130,5,32,71,130,47,32,89,130,149,131,11,32,
    74,132,17,32,82,132,5,131,149,34,89,0,105,132,41,32,106,132,5,32,107,132,5,32,108,132,5,32,109,132,5,32,110,132,5,32,111,132,47,32,112,132,5,32,113,132,5,32,114,132,5,32,115,132,5,
    32,121,132,5,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,160,132,65,32,172,132,149,32,177,132,17,32,196,132,167,32,197,130,17,32,90,130,179,34,166,0,90,130,179,32,154,130,
    5,34,70,255,250,130,5,34,71,255,246,130,5,32,72,132,5,32,74,132,17,32,82,132,5,32,84,132,17,32,111,132,11,32,112,132,11,32,113,132,5,32,114,132,5,32,115,132,5,32,121,132,29,32,122,
    132,5,32,123,132,5,32,124,132,5,32,125,132,5,33,172,255,131,101,32,177,132,11,34,196,255,180,130,101,32,197,130,5,40,91,0,70,255,240,0,91,0,71,132,5,32,72,132,5,32,74,132,5,32,82,132,
    5,32,84,132,5,32,111,132,5,32,112,132,5,32,113,132,5,32,114,132,5,32,115,132,5,32,121,132,5,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,177,130,5,40,92,0,5,0,29,0,92,0,10,
    132,5,32,15,130,137,32,92,130,245,32,129,130,17,32,34,130,131,32,92,130,131,32,246,130,11,131,251,33,92,0,131,251,36,92,0,73,0,4,130,17,32,74,130,215,34,92,0,82,132,5,32,84,132,5,34,
    87,0,6,130,23,32,111,132,11,32,112,132,5,32,113,132,5,32,114,132,5,32,115,132,5,32,121,132,5,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,33,172,255,131,119,32,177,132,11,34,
    191,0,12,130,77,34,196,255,141,130,5,32,197,130,5,42,94,0,77,0,203,0,98,0,45,0,94,130,5,34,55,255,109,130,5,34,57,255,139,130,5,34,58,255,176,130,5,34,60,255,100,130,5,32,234,130,5,
    32,99,132,35,33,99,0,131,35,33,99,0,131,35,39,99,0,58,255,182,0,99,0,131,35,33,99,0,131,35,40,100,0,38,255,201,0,100,0,42,132,5,32,50,132,5,32,52,130,5,32,101,130,59,38,61,0,103,0,
    36,255,229,130,5,34,55,255,164,130,5,34,59,255,219,130,5,34,61,255,207,130,5,32,229,130,5,34,121,0,68,130,29,40,121,0,73,255,231,0,121,0,91,130,11,32,122,132,17,32,122,132,17,32,122,
    132,17,32,123,132,17,32,123,132,17,32,123,132,17,32,124,132,17,32,124,132,17,32,124,132,17,32,125,132,17,32,125,132,17,32,125,132,17,38,145,0,45,0,14,0,146,132,125,32,146,130,191,32,
    164,130,11,131,125,33,146,0,131,125,33,146,0,131,125,32,161,132,53,32,161,132,53,32,161,132,53,32,162,130,215,38,154,0,162,0,77,0,188,130,5,36,234,255,154,0,164,130,11,34,160,0,170,
    130,77,32,82,130,5,32,55,130,179,35,170,0,58,0,131,11,32,61,132,5,34,92,0,12,130,23,32,229,132,11,32,235,130,11,32,171,130,107,36,207,0,171,0,57,130,41,34,171,0,58,132,5,32,60,130,
    71,33,171,0,131,77,42,172,0,180,255,49,0,172,0,181,255,61,130,5,32,182,132,11,32,183,130,11,32,173,130,95,34,94,0,173,130,59,32,109,130,5,34,57,255,139,130,5,34,58,255,182,130,5,34,
    60,255,100,130,5,32,234,130,5,32,174,132,35,32,174,132,35,32,174,130,95,34,139,0,174,130,95,32,182,130,5,131,35,32,174,130,95,34,100,0,175,132,239,32,175,132,239,33,175,0,131,239,32,
    175,132,239,32,175,132,239,38,176,0,36,0,29,0,176,130,71,32,68,130,5,34,55,0,4,130,5,40,59,0,8,0,180,0,15,255,152,130,5,32,17,132,5,34,36,255,37,130,11,34,45,255,98,130,5,32,55,130,
    215,36,180,0,70,255,127,130,11,32,71,132,5,32,72,132,5,34,74,255,115,130,17,34,86,255,178,130,5,34,145,254,135,130,5,32,172,132,59,32,228,130,17,34,181,0,15,130,239,34,181,0,17,132,
    5,37,36,255,102,0,181,0,131,71,32,181,130,71,32,221,130,11,32,71,130,89,34,181,0,72,132,5,32,74,132,5,32,82,132,5,32,86,130,89,36,181,0,145,255,10,130,35,32,172,132,59,32,228,130,17,
    32,182,130,209,38,37,0,182,0,38,255,190,130,5,34,45,255,111,130,5,131,77,32,182,130,77,32,127,130,11,131,77,32,182,130,77,131,11,32,74,132,47,32,82,132,5,32,86,130,137,32,182,132,77,
    36,182,0,182,255,84,130,41,131,155,32,183,132,155,32,183,132,155,32,183,130,89,35,90,0,183,0,131,233,32,183,132,161,34,183,0,68,130,197,32,183,130,89,32,61,130,23,131,89,32,183,132,
    167,33,183,0,131,167,33,183,0,131,167,34,183,0,84,130,131,36,183,0,86,255,139,130,35,34,145,255,8,130,5,131,173,34,183,0,183,130,101,34,183,0,196,132,65,32,228,130,29,32,186,130,71,
    37,246,0,186,0,71,255,131,5,32,72,132,5,32,74,132,5,32,82,132,5,32,84,130,5,32,187,130,131,34,98,0,187,130,221,32,225,130,5,33,42,255,131,5,32,50,132,5,34,52,255,205,130,17,34,54,255,
    229,130,5,32,227,130,5,42,190,0,45,0,82,0,190,0,55,255,207,130,5,34,58,0,39,130,5,32,61,132,17,34,92,0,12,130,11,32,229,132,11,32,235,130,11,32,191,130,35,36,154,0,191,0,57,130,41,
    34,191,0,58,132,5,33,60,255,131,17,32,234,130,5,38,196,0,42,255,178,0,196,130,77,32,109,130,5,34,50,255,166,130,5,34,55,255,49,130,5,32,56,130,143,32,196,130,53,32,115,130,11,34,58,
    255,127,130,5,34,59,0,57,130,5,32,60,130,227,32,196,130,93,32,25,130,11,32,89,130,215,34,196,0,90,132,29,32,234,130,23,32,197,130,77,34,166,0,197,132,77,34,197,0,50,130,89,32,197,130,
    125,32,49,130,17,131,77,32,197,130,77,32,127,130,11,32,58,130,53,34,197,0,59,130,179,36,197,0,60,255,61,130,17,131,71,32,197,132,71,34,197,0,234,130,17,32,199,130,65,34,94,0,199,130,
    59,32,109,130,5,32,57,130,47,32,199,130,185,32,182,130,11,34,60,255,100,130,5,32,234,130,5,32,200,130,35,34,61,0,201,132,41,32,201,132,41,32,201,130,95,32,139,130,17,34,58,255,182,
    130,5,131,41,32,201,130,77,34,100,0,202,132,41,32,203,132,5,38,208,0,36,255,229,0,208,130,47,32,164,130,5,34,59,255,219,130,5,34,61,255,207,130,5,32,229,130,5,32,209,132,29,32,209,
    132,29,33,209,0,131,29,33,209,0,131,29,33,209,0,131,29,32,210,132,29,32,210,132,29,32,210,132,29,32,210,132,29,32,210,132,29,42,225,0,13,255,49,0,225,0,34,255,154,130,5,32,36,130,223,
    36,225,0,38,255,190,130,11,32,42,132,5,34,45,0,100,130,11,34,50,255,186,130,5,32,52,132,5,34,55,255,143,130,11,34,56,255,227,130,5,131,227,32,225,130,227,32,207,130,11,34,60,255,127,
    130,5,32,61,132,65,32,87,130,173,34,225,0,89,132,83,32,90,132,71,34,92,255,180,130,29,32,100,132,11,32,103,132,71,32,104,132,65,32,141,132,35,32,146,132,17,32,175,132,5,32,176,132,
    5,34,180,255,115,130,47,34,181,255,139,130,5,32,182,132,11,32,183,132,95,32,186,132,71,32,187,132,53,34,196,0,68,130,29,32,197,132,5,32,208,132,53,32,209,132,5,32,210,132,5,32,211,
    132,95,32,212,132,5,32,213,132,5,32,229,132,155,32,234,132,71,32,235,130,71,34,227,0,87,130,143,32,227,130,167,36,207,0,227,0,90,130,179,36,227,0,92,255,209,130,11,32,186,132,23,34,
    196,255,178,130,11,32,197,132,5,32,235,130,23,42,229,0,45,0,82,0,229,0,55,0,39,130,5,34,92,255,203,130,5,34,119,0,70,130,5,32,171,132,17,131,53,34,229,0,191,132,35,32,235,130,29,34,
    232,0,15,130,107,34,232,0,17,132,5,40,36,255,223,0,232,0,55,255,164,130,5,32,59,132,29,34,61,255,207,130,11,32,98,132,23,32,99,132,5,34,145,255,182,130,17,32,172,132,47,32,173,132,
    17,32,174,132,5,34,187,255,246,130,23,34,196,255,139,130,5,32,197,132,5,32,199,132,23,32,201,132,5,32,229,130,71,32,234,130,107,34,80,0,234,130,107,32,61,130,5,34,36,255,98,130,5,34,
    38,255,211,130,5,32,42,132,5,32,45,130,209,34,234,0,50,132,11,32,52,132,5,32,54,130,239,32,234,132,203,36,234,0,68,255,57,130,41,34,70,255,76,130,5,32,71,132,5,32,72,132,5,32,73,132,
    35,32,74,132,11,34,80,255,115,130,29,32,81,132,5,32,82,132,17,32,83,132,11,32,84,132,11,32,85,132,11,34,86,255,123,130,35,32,88,132,11,32,98,132,131,32,99,132,5,34,100,255,205,130,
    23,32,103,132,119,32,105,132,107,32,106,132,5,32,107,132,5,32,108,130,239,36,234,0,109,255,102,130,35,32,110,132,17,32,111,132,83,32,112,132,5,32,113,132,5,32,114,132,5,32,115,132,
    5,34,119,0,90,130,41,32,120,132,101,32,121,132,17,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,126,132,35,32,127,132,5,32,128,132,5,32,129,132,5,34,145,255,63,130,65,32,146,
    132,143,32,160,132,113,34,162,255,27,130,17,34,170,255,207,130,5,34,172,255,61,130,5,32,173,132,185,32,174,132,5,32,175,132,41,32,176,132,5,32,177,132,89,34,190,255,154,130,35,34,196,
    255,8,130,5,32,197,132,5,32,199,132,41,32,201,132,5,32,208,132,41,32,209,132,5,32,210,132,5,34,227,255,229,130,41,40,228,255,123,0,235,0,5,0,29,130,5,32,10,132,5,32,15,130,71,36,235,
    0,17,255,129,130,17,34,34,255,180,130,5,34,70,255,246,130,5,32,71,132,5,32,72,132,5,34,73,0,4,130,17,32,74,132,11,32,82,132,5,32,84,132,5,34,87,0,6,130,23,32,111,132,11,32,112,132,
    5,32,113,132,5,32,114,132,5,32,115,132,5,32,121,132,5,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,32,172,132,119,32,177,132,11,34,191,0,12,130,77,34,196,255,141,130,5,32,197,
    130,5,40,236,0,15,254,186,0,236,0,17,132,5,32,36,130,215,34,236,0,42,130,41,36,236,0,45,255,127,130,23,34,58,0,39,130,5,34,59,255,195,130,5,34,68,255,190,130,5,32,70,130,191,34,236,
    0,71,132,5,32,72,132,5,32,74,132,5,32,82,132,5,34,84,255,182,130,35,32,98,132,71,32,99,132,5,32,105,132,53,32,106,132,5,32,107,132,5,32,108,132,5,32,109,132,5,32,110,132,5,32,111,132,
    59,32,112,132,5,32,113,132,5,32,114,132,5,32,115,132,5,32,121,132,5,32,122,132,5,32,123,132,5,32,124,132,5,32,125,132,5,34,145,254,233,130,113,32,160,132,71,32,172,132,197,32,173,132,
    119,32,174,132,5,32,177,132,35,32,196,132,23,34,197,254,174,130,41,32,199,132,23,32,201,130,5,42,237,0,68,255,229,0,237,0,73,255,246,130,5,34,91,255,231,130,5,32,105,132,17,32,106,
    132,5,32,107,132,5,32,108,132,5,32,109,132,5,32,110,132,5,32,160,130,5,32,0,130,0,36,48,2,70,0,1,130,7,131,2,32,50,130,4,133,11,36,1,0,6,0,50,134,23,36,2,0,7,0,56,134,11,36,3,0,14,
    0,63,134,11,32,4,138,35,36,5,0,12,0,77,134,23,32,6,138,23,36,7,0,58,0,89,134,23,34,8,0,21,130,73,133,95,36,11,0,43,0,147,134,23,36,13,1,196,0,190,134,11,34,14,0,37,130,23,43,3,0,1,
    4,3,0,2,0,12,2,130,0,131,11,32,5,130,11,34,16,2,142,132,11,32,6,132,23,32,158,132,11,32,7,132,23,32,170,132,11,32,8,132,11,32,186,132,11,32,9,130,199,34,100,2,202,134,11,36,1,0,12,
    3,46,134,11,36,2,0,14,3,58,134,11,36,3,0,28,3,72,134,11,32,4,138,35,36,5,0,24,3,100,134,23,32,6,138,23,36,7,0,116,3,124,134,23,36,8,0,42,2,216,134,11,36,11,0,86,3,240,134,11,36,13,
    3,136,4,70,134,11,34,14,0,74,134,23,32,10,138,203,32,11,130,11,34,16,7,206,132,35,32,12,138,23,32,14,131,11,33,7,222,132,23,32,16,130,11,34,14,7,234,132,11,32,19,130,11,34,18,7,248,
    132,11,32,20,138,47,32,21,130,11,34,16,8,10,132,23,32,22,138,23,32,25,130,11,34,14,8,26,132,23,32,27,132,35,32,40,132,11,32,29,138,35,32,31,138,11,32,36,132,47,32,56,132,35,32,45,132,
    11,132,191,130,89,65,127,8,32,8,138,95,32,12,138,203,32,12,134,191,8,62,169,32,50,48,49,56,32,77,105,99,114,111,115,111,102,116,32,67,111,114,112,111,114,97,116,105,111,110,46,32,65,
    108,108,32,82,105,103,104,116,115,32,82,101,115,101,114,118,101,100,46,71,97,100,117,103,105,82,101,103,117,108,97,114,133,12,130,28,132,13,35,86,101,114,115,130,55,36,32,49,46,49,
    51,134,25,52,105,115,32,97,32,116,114,97,100,101,109,97,114,107,32,111,102,32,116,104,101,138,110,36,103,114,111,117,112,131,22,55,99,111,109,112,97,110,105,101,115,46,104,116,116,
    112,115,58,47,47,100,111,99,115,46,109,135,152,32,46,130,32,48,47,116,121,112,111,103,114,97,112,104,121,47,97,98,111,117,116,137,182,57,115,117,112,112,108,105,101,100,32,102,111,
    110,116,46,32,89,111,117,32,109,97,121,32,117,115,101,130,112,130,132,131,22,63,32,116,111,32,99,114,101,97,116,101,44,32,100,105,115,112,108,97,121,44,32,97,110,100,32,112,114,105,
    110,116,32,99,130,57,32,101,130,7,41,97,115,32,112,101,114,109,105,116,116,130,78,33,98,121,132,176,36,108,105,99,101,110,131,75,130,24,37,115,32,111,114,32,116,133,8,32,102,131,97,
    32,44,145,214,44,112,114,111,100,117,99,116,44,32,115,101,114,118,130,59,130,34,32,114,136,96,39,105,110,32,119,104,105,99,104,138,153,32,119,130,116,39,105,110,99,108,117,100,101,
    100,137,189,43,111,110,108,121,32,40,105,41,32,101,109,98,130,139,33,116,104,135,200,130,65,155,173,132,46,49,100,105,110,103,32,114,101,115,116,114,105,99,116,105,111,110,115,32,135,
    96,130,8,137,122,36,59,32,97,110,100,130,100,130,101,40,116,101,109,112,111,114,97,114,105,130,117,39,100,111,119,110,108,111,97,100,65,61,13,32,97,65,42,5,35,101,114,32,111,130,2,
    33,116,104,131,8,39,117,116,112,117,116,32,100,101,131,230,39,32,116,111,32,104,101,108,112,65,81,13,38,46,32,65,110,121,32,111,132,46,35,117,115,101,32,130,197,39,112,114,111,104,
    105,98,105,116,130,237,43,0,78,0,111,0,114,0,109,0,97,0,108,130,9,44,98,0,121,1,13,0,101,0,106,0,110,0,233,130,3,137,27,34,83,0,116,130,35,34,110,0,100,130,5,42,114,0,100,3,154,3,177,
    3,189,3,191,130,3,48,185,3,186,3,172,0,169,0,32,0,50,0,48,0,49,0,56,130,9,36,77,0,105,0,99,130,87,34,111,0,115,130,85,32,102,130,59,34,32,0,67,132,105,32,112,132,5,32,97,130,17,32,
    105,130,9,34,110,0,46,130,45,32,65,130,123,32,108,130,7,32,82,130,53,34,103,0,104,130,27,32,115,132,13,32,101,130,61,32,101,130,69,32,118,130,145,32,100,130,41,34,71,0,97,130,127,34,
    117,0,103,130,39,32,82,130,19,34,103,0,117,130,57,32,97,130,33,139,25,33,32,0,141,27,32,86,130,41,32,114,130,73,133,111,32,32,130,161,32,46,130,3,33,51,0,141,51,32,105,130,33,34,32,
    0,97,130,119,32,116,130,79,131,103,34,101,0,109,132,235,32,107,130,19,34,111,0,102,132,25,32,104,130,75,33,32,0,147,221,32,103,132,237,34,117,0,112,136,45,32,99,130,223,32,109,130,
    15,34,97,0,110,130,175,131,203,32,46,132,219,32,116,130,19,36,115,0,58,0,47,130,1,32,100,130,35,32,99,130,121,32,46,130,107,65,49,15,33,46,0,133,65,36,47,0,116,0,121,130,51,32,111,
    130,247,32,114,130,145,32,112,130,69,32,121,130,59,34,97,0,98,130,61,32,117,130,29,147,143,33,115,0,131,139,34,112,0,108,132,123,32,100,130,149,32,102,130,45,32,110,130,45,32,46,130,
    11,32,89,132,57,32,32,130,117,32,97,130,91,32,32,130,43,32,115,132,217,32,116,130,91,65,9,5,135,45,32,32,130,49,32,111,130,49,32,99,130,223,34,101,0,97,130,13,34,101,0,44,130,15,32,
    100,130,87,32,115,130,149,32,108,130,19,32,121,132,17,131,231,131,103,32,112,130,41,33,105,0,131,105,32,32,132,193,131,9,32,101,134,15,32,97,130,235,32,32,130,53,32,101,130,35,32,109,
    130,65,32,116,132,79,131,53,32,98,132,135,131,127,32,101,130,93,131,181,32,99,130,145,32,110,130,47,131,15,32,116,130,11,32,114,130,173,32,115,130,27,32,111,130,59,32,32,132,55,137,
    17,32,102,130,23,32,117,132,43,131,155,65,173,33,32,112,130,67,34,111,0,100,130,247,32,99,130,73,131,51,32,115,132,97,32,118,130,147,32,99,130,9,133,69,32,114,132,247,32,111,132,187,
    135,193,131,217,36,32,0,119,0,104,132,41,32,104,66,21,6,65,51,15,34,119,0,97,132,233,131,45,34,99,0,108,130,103,32,100,130,85,33,100,0,65,123,19,131,95,32,108,132,253,32,40,130,83,
    32,41,130,81,36,101,0,109,0,98,132,47,131,231,32,104,130,21,131,247,65,145,9,133,131,32,99,65,207,6,65,91,47,137,93,32,100,130,89,34,110,0,103,130,113,32,114,130,109,36,115,0,116,0,
    114,130,17,32,99,67,155,8,131,117,143,193,32,32,132,51,147,151,32,59,130,71,66,3,7,131,201,33,105,0,131,203,32,116,130,89,33,109,0,67,245,7,131,95,133,235,32,100,130,189,36,119,0,110,
    0,108,130,7,34,97,0,100,65,71,22,32,116,130,29,67,149,5,66,85,9,32,101,130,165,32,32,130,23,32,114,130,51,32,111,130,171,32,104,130,97,133,11,32,117,130,13,33,112,0,131,5,32,32,130,
    81,33,101,0,65,205,7,131,173,32,111,130,45,131,41,34,108,0,112,130,9,66,163,25,68,143,5,34,110,0,121,142,93,66,93,5,131,253,32,115,134,63,38,111,0,104,0,105,0,98,130,3,32,116,130,129,
    34,100,0,46,69,75,10,32,97,130,215,32,105,136,15,32,225,130,13,69,103,11,33,101,0,69,77,11,32,97,130,207,32,100,69,135,12,130,125,41,4,30,4,49,4,75,4,71,4,61,130,5,32,57,140,73,32,
    110,130,109,34,78,0,97,130,221,32,97,130,229,36,110,0,111,0,65,130,65,32,114,130,247,32,110,130,255,36,97,0,0,0,3,130,3,130,2,35,255,78,0,119,130,6,145,2,32,1,130,33,44,8,0,10,0,19,
    0,7,255,255,0,15,0,1,130,34,32,12,130,3,32,46,130,3,50,2,0,5,0,0,4,186,0,1,4,187,4,190,0,3,4,191,4,205,130,11,34,206,4,206,130,11,34,207,4,238,130,43,32,4,130,37,131,49,34,0,0,1,130,
    9,58,10,0,88,0,196,0,3,68,70,76,84,0,20,79,115,103,101,0,36,99,97,110,115,0,52,0,4,130,29,32,0,130,95,32,3,130,109,34,5,0,7,138,15,130,96,44,0,6,0,10,0,1,67,82,82,32,0,18,0,131,37,
    34,1,0,2,134,7,40,1,0,8,107,101,114,110,0,50,132,5,32,58,132,5,32,64,132,5,38,70,109,97,114,107,0,80,132,5,38,88,109,107,109,107,0,96,132,5,38,102,0,0,0,2,0,0,132,143,132,71,130,9,
    36,2,0,0,0,3,132,19,33,2,0,131,29,32,3,131,215,33,0,2,130,19,32,5,132,21,32,4,134,5,50,8,0,18,0,34,0,50,0,76,0,92,0,108,0,124,0,140,0,9,132,25,36,8,0,1,0,8,130,9,32,130,142,15,32,164,
    130,15,34,8,0,2,130,189,32,18,130,33,36,2,0,0,0,194,133,7,33,19,76,138,41,130,93,33,27,8,138,15,36,6,0,0,29,60,141,31,33,29,98,138,15,36,1,0,0,30,174,142,15,32,178,134,191,32,20,130,
    109,34,44,0,32,132,221,32,6,130,5,40,4,4,126,4,133,4,134,4,145,132,11,38,128,4,139,4,140,4,144,130,11,34,1,4,190,138,49,32,42,134,49,32,7,132,37,38,162,4,169,4,170,4,181,130,11,38,
    3,4,175,4,176,4,180,134,47,36,1,1,44,0,4,130,195,50,145,1,246,2,216,3,50,3,200,4,50,4,112,4,194,5,0,5,78,132,1,32,156,137,11,33,6,74,137,11,137,9,37,6,188,6,198,6,204,132,1,137,9,32,
    222,138,21,40,240,7,2,7,228,7,238,7,248,132,1,146,9,35,8,22,8,164,132,63,36,204,8,174,8,164,135,41,33,9,236,133,19,39,6,204,11,42,11,116,11,138,136,13,32,180,133,9,33,11,222,133,7,
    35,12,96,12,114,134,9,32,168,134,7,32,186,135,7,36,12,204,12,222,12,134,13,37,13,20,13,202,13,202,133,25,132,137,133,143,34,248,13,212,139,13,135,11,8,221,14,62,14,152,15,58,15,168,
    16,30,16,172,3,200,17,22,16,172,18,16,0,2,0,33,1,159,1,159,0,0,2,37,2,37,0,1,2,90,2,90,0,2,2,148,2,148,0,3,3,21,3,21,0,4,3,23,3,40,0,5,3,42,3,46,0,23,3,48,3,52,0,28,3,54,3,71,0,33,
    3,73,3,77,0,51,3,79,3,83,0,56,3,85,3,86,0,61,3,88,3,98,0,63,3,100,3,104,0,74,3,106,3,110,0,79,3,113,3,117,0,84,3,120,3,123,0,89,3,125,3,128,0,93,3,130,3,130,0,97,3,132,3,135,0,98,3,
    138,3,143,0,102,3,147,3,147,0,108,3,150,3,155,0,109,3,158,3,160,0,115,3,164,3,166,0,118,3,170,3,172,0,121,3,174,3,174,0,124,3,177,3,179,0,125,3,183,3,185,0,128,3,189,3,191,0,131,4,
    18,4,18,0,134,4,25,4,25,0,135,4,224,4,232,0,136,130,133,44,24,255,156,3,26,255,206,3,29,0,20,3,30,130,11,36,32,255,236,3,33,130,3,32,34,130,3,32,35,130,19,32,36,130,19,32,41,130,7,
    32,42,130,7,32,47,130,7,32,49,130,3,32,53,130,3,32,55,130,3,32,59,130,3,32,61,130,3,32,65,130,3,36,67,0,10,3,72,130,7,32,73,130,7,32,78,130,7,32,79,130,7,32,84,130,7,36,88,255,146,
    3,89,130,3,32,90,130,3,32,91,130,15,32,92,130,23,32,94,130,75,32,95,130,3,32,96,130,3,32,97,130,19,32,99,130,3,32,103,130,3,32,111,130,3,32,117,130,3,32,123,130,3,32,129,130,3,32,135,
    130,3,32,141,130,3,32,147,130,3,36,150,255,226,3,151,130,3,32,152,130,3,32,153,130,15,32,161,130,3,32,167,130,3,32,173,130,3,32,180,130,3,32,186,130,3,40,192,0,20,4,17,0,10,4,19,130,
    7,32,20,130,3,40,22,0,20,0,22,3,25,255,216,136,225,32,31,130,11,143,225,32,37,130,19,131,225,32,43,130,7,131,225,34,88,255,206,130,177,34,206,3,90,130,7,32,94,130,3,32,95,130,3,32,
    96,130,3,32,97,141,133,35,0,37,3,24,130,139,32,25,130,3,65,63,9,34,226,3,31,130,15,143,97,32,36,130,19,32,37,130,3,65,71,5,34,226,3,43,130,11,131,105,32,72,130,81,32,78,130,3,65,39,
    5,153,117,143,255,147,247,134,243,33,0,26,130,149,32,186,130,243,32,186,136,243,34,30,255,186,130,149,33,186,3,145,149,34,186,3,37,130,27,133,149,32,186,130,149,33,186,3,161,255,42,
    196,3,151,255,196,3,152,255,196,0,15,130,101,32,136,130,89,32,136,130,69,32,136,130,61,38,136,3,49,255,236,3,55,130,3,32,61,130,3,32,85,130,215,34,150,255,216,130,45,32,216,130,45,
    42,216,4,224,255,216,4,225,0,30,4,227,130,7,34,230,0,20,130,29,32,48,130,37,32,54,130,3,32,60,130,3,32,66,130,15,40,85,255,226,3,88,255,56,3,89,130,3,32,90,130,3,36,94,255,106,3,95,
    130,3,32,96,130,3,32,114,130,27,32,115,130,3,32,116,130,3,140,85,130,81,32,225,130,81,46,227,0,30,0,15,1,159,255,236,2,90,255,226,2,148,130,213,36,66,255,156,3,106,130,89,32,131,130,
    49,38,174,255,236,4,25,255,226,130,131,36,176,4,225,0,20,130,131,34,176,4,228,130,19,36,229,255,186,4,231,130,7,36,233,255,186,0,19,132,61,34,37,255,216,136,65,32,85,130,65,32,104,
    130,65,32,105,130,3,35,112,255,206,3,131,69,32,223,130,49,32,224,130,65,36,225,255,176,4,227,130,7,135,69,34,230,255,156,132,73,32,232,137,77,35,0,20,2,37,130,3,32,90,130,3,32,148,
    130,221,32,26,130,209,132,229,130,11,32,174,130,57,32,25,130,3,32,223,130,3,132,77,130,7,132,77,130,7,32,229,130,3,32,230,130,3,32,231,130,3,32,232,130,3,36,233,0,20,0,43,130,155,36,
    206,2,37,255,206,130,155,32,206,130,221,38,206,3,20,255,216,3,29,130,81,32,35,130,3,32,41,130,3,32,47,130,3,32,53,130,163,32,59,130,3,32,65,130,3,32,66,130,3,32,85,130,3,32,87,130,
    39,32,91,130,7,32,93,130,7,32,99,130,7,32,103,130,3,32,111,130,3,32,117,130,3,32,123,130,3,32,129,130,3,32,135,130,3,32,141,130,3,32,153,130,3,32,154,130,3,32,155,130,3,32,161,132,
    239,38,206,4,22,255,206,4,25,130,3,32,223,130,3,32,224,130,3,32,225,130,3,32,226,130,3,32,227,130,3,32,228,130,3,32,229,130,3,32,230,130,3,32,231,130,3,32,232,130,3,36,233,255,206,
    0,28,65,73,6,32,236,130,173,38,236,2,148,255,236,3,20,130,3,36,29,255,156,3,35,130,3,32,41,130,3,32,47,130,3,32,66,130,19,32,85,130,3,32,87,130,3,32,93,130,3,32,154,130,3,32,155,130,
    3,65,171,5,38,236,4,223,255,236,4,224,130,3,32,225,130,3,32,226,130,3,32,227,130,3,65,109,5,34,236,4,230,130,11,32,231,130,3,65,109,5,46,236,0,2,4,223,0,20,4,230,0,40,0,1,3,66,130,
    5,34,4,3,26,130,191,34,66,0,30,134,23,38,30,0,4,3,29,255,216,130,127,34,216,3,41,130,7,34,47,255,216,132,17,32,166,130,17,32,166,130,17,43,166,3,47,255,166,0,56,3,22,0,20,3,69,15,49,
    42,40,3,53,0,30,3,55,0,40,3,59,130,7,32,61,130,7,32,65,130,7,32,68,130,75,32,72,130,7,32,74,130,7,32,78,130,7,32,80,130,7,32,84,130,7,68,93,11,32,91,130,15,68,97,13,34,30,3,99,130,
    19,32,103,130,3,32,111,130,3,32,117,130,3,32,123,130,3,32,129,130,3,32,135,130,3,32,141,130,3,32,147,130,3,69,11,13,34,30,3,161,130,19,32,167,130,3,32,173,130,3,32,180,130,3,32,186,
    130,3,32,192,130,253,36,16,0,20,4,19,130,7,32,20,130,3,38,22,0,30,0,2,3,85,130,17,34,230,0,20,130,9,32,66,66,123,5,35,0,7,2,90,130,153,65,43,7,66,141,11,32,230,130,49,34,35,3,20,130,
    29,36,22,255,226,3,25,70,39,10,32,31,70,39,18,32,37,70,39,6,32,43,70,39,6,32,67,130,55,32,73,130,3,32,79,130,3,32,87,130,3,139,247,32,92,130,15,32,93,130,3,141,251,38,20,3,100,255,
    236,3,101,130,3,32,102,130,3,138,227,37,4,16,255,226,4,17,132,181,40,26,255,216,4,224,0,10,0,79,67,17,18,42,106,3,24,255,56,3,29,255,106,3,30,130,7,32,35,130,7,32,36,130,7,32,41,130,
    7,32,42,130,7,32,47,130,7,36,48,255,176,3,53,130,3,32,54,130,3,32,59,130,3,32,60,130,3,32,65,130,3,40,66,255,206,3,67,255,216,3,73,130,3,32,79,130,3,32,85,130,15,32,86,130,7,32,87,
    130,51,68,145,11,32,91,130,43,32,92,130,23,32,93,130,23,68,157,11,32,98,130,19,32,99,130,27,34,100,255,226,130,191,34,226,3,102,130,7,32,103,130,15,36,104,255,246,3,105,130,203,32,
    111,130,11,35,112,255,196,3,68,197,11,32,117,130,19,32,118,130,19,32,123,130,7,32,124,130,63,32,129,130,7,32,130,130,15,32,135,130,7,32,136,130,7,32,141,130,7,32,148,130,7,32,153,130,
    7,67,149,7,32,156,130,15,32,161,130,15,46,174,255,206,4,17,255,216,4,21,255,236,4,22,255,176,67,161,48,65,61,107,38,106,3,89,255,106,3,90,130,3,65,61,65,38,236,3,115,255,236,3,116,
    130,3,65,61,5,33,216,3,65,61,13,33,216,3,65,61,5,33,216,3,65,61,91,38,18,3,20,255,226,3,24,130,3,36,29,255,196,3,30,130,7,32,35,130,7,32,36,130,7,32,41,130,7,32,42,130,7,32,47,130,
    7,32,66,130,7,32,87,130,3,32,88,130,3,32,89,130,3,32,90,130,3,32,93,130,3,32,94,130,189,32,95,130,3,38,96,255,236,0,5,3,25,130,9,32,31,130,3,32,37,130,3,32,43,68,237,5,33,0,10,132,
    91,131,87,131,83,131,79,72,145,13,34,226,3,95,130,71,34,96,255,226,132,41,32,206,130,129,38,206,3,36,255,206,3,42,130,3,36,88,255,176,3,89,130,3,32,90,130,3,32,94,130,133,32,95,130,
    3,36,96,255,196,0,32,130,179,32,216,132,87,32,29,130,25,133,179,32,176,130,53,34,226,3,41,130,15,133,179,34,176,3,53,130,133,32,59,130,3,32,65,130,3,35,87,255,216,3,139,187,32,91,130,
    19,32,93,130,19,138,191,33,3,99,130,19,32,103,130,3,32,111,130,3,32,117,130,3,32,123,130,3,32,129,130,3,32,135,130,3,32,141,130,3,32,153,130,3,32,161,130,213,32,22,130,235,34,4,3,29,
    130,159,32,35,130,3,32,41,130,3,36,47,255,206,0,13,130,147,32,206,130,147,32,146,130,193,32,146,130,139,32,146,130,193,34,146,3,87,132,197,32,156,130,197,38,156,3,90,255,156,3,93,130,
    15,32,94,130,163,32,95,130,3,38,96,255,176,0,4,3,25,130,253,32,31,130,3,32,37,130,3,34,43,255,226,130,17,131,211,32,35,130,31,131,203,32,47,134,35,33,236,3,65,87,10,132,107,32,236,
    130,107,34,216,3,30,130,195,32,36,130,3,32,42,130,3,32,87,130,159,34,88,255,196,130,107,32,196,130,107,32,196,130,107,33,236,3,65,53,11,37,45,1,159,255,226,2,130,103,72,19,6,32,226,
    130,177,34,216,3,22,130,57,36,29,255,86,3,35,130,3,32,41,130,3,32,47,130,3,71,123,13,34,226,3,85,130,155,71,123,49,34,226,3,155,130,55,71,123,5,36,226,4,16,255,216,71,127,6,38,226,
    4,223,255,226,4,224,130,3,32,225,130,3,32,226,130,3,32,227,130,3,32,228,130,3,32,229,130,3,32,230,130,3,32,231,130,3,32,232,130,3,46,233,255,226,0,2,3,26,255,206,3,97,0,20,0,26,130,
    241,34,236,3,25,130,229,74,161,9,65,23,5,36,32,255,246,3,33,130,3,32,34,130,3,75,229,5,34,236,3,37,130,39,74,11,5,34,236,3,43,130,11,74,11,33,75,145,8,33,0,22,130,101,32,226,130,119,
    36,216,3,29,0,20,75,3,20,74,255,7,74,251,7,66,211,13,33,206,3,75,101,23,42,40,3,20,0,30,3,22,255,216,3,24,130,89,34,25,255,176,74,215,9,132,105,32,176,74,215,17,130,23,32,37,130,39,
    132,203,130,11,32,43,130,11,131,203,70,145,13,33,30,3,70,145,17,33,30,3,70,145,27,32,105,70,149,16,38,216,4,17,0,20,0,27,66,21,5,130,165,32,25,130,109,65,105,7,32,31,130,11,75,113,
    15,32,37,130,19,131,149,32,43,130,7,131,145,67,99,5,66,235,11,32,216,130,129,33,156,3,77,27,11,71,7,12,35,0,30,0,29,75,223,66,68,35,11,32,176,130,109,38,176,3,96,255,176,3,97,130,149,
    71,129,13,34,176,3,151,130,23,36,152,255,176,0,35,65,133,72,32,87,73,3,14,32,93,73,3,16,72,7,13,65,117,18,66,215,28,78,189,15,66,215,61,32,62,130,247,32,20,72,251,48,33,48,0,74,19,
    5,32,54,74,19,6,32,60,74,19,6,36,67,0,20,3,72,130,239,32,73,74,19,6,32,79,74,19,6,73,31,15,32,91,130,35,73,35,21,74,31,5,72,103,12,74,43,70,32,17,74,43,14,40,32,3,20,255,236,3,24,255,
    156,67,217,6,80,33,9,78,225,5,38,226,3,33,255,226,3,34,130,3,68,63,5,33,156,3,67,225,7,32,42,130,51,67,229,7,69,97,5,80,1,9,32,93,130,83,36,94,255,146,3,95,130,3,32,96,130,3,66,115,
    5,73,53,9,36,150,255,186,3,151,130,3,51,152,255,186,0,2,3,166,0,4,0,0,4,100,5,250,0,17,0,27,0,183,0,130,199,33,255,206,130,1,32,226,171,64,33,255,166,135,45,37,255,226,255,66,255,176,
    130,63,32,236,135,17,153,7,131,53,39,255,36,255,56,0,0,255,136,131,37,43,255,206,0,0,255,66,255,196,255,236,255,216,130,71,32,226,130,5,148,69,39,255,156,255,56,255,106,255,86,130,
    45,134,119,135,127,130,59,32,176,131,73,33,0,20,130,59,131,133,33,176,255,133,15,132,20,135,4,33,255,196,135,9,153,7,32,255,136,121,130,205,130,67,139,15,153,53,137,25,32,255,130,121,
    132,207,130,211,131,21,131,11,131,7,33,0,0,137,73,133,15,147,5,163,35,137,55,35,255,216,255,216,137,13,165,9,37,255,236,255,86,255,156,132,161,65,233,6,33,255,136,133,117,33,255,226,
    155,77,33,255,206,141,29,141,13,147,123,33,255,106,131,21,131,5,134,9,38,176,0,0,255,56,255,176,131,181,32,255,132,123,33,255,196,65,29,9,132,43,65,123,6,130,23,140,215,133,45,131,
    33,163,3,33,255,186,163,37,135,35,130,143,66,21,12,65,129,34,36,80,0,0,255,206,130,197,32,206,130,7,140,167,65,227,26,34,2,0,31,85,11,24,8,164,24,3,28,0,4,3,30,3,34,0,9,3,36,3,40,0,
    14,3,42,3,46,0,19,3,48,3,52,0,24,3,54,3,58,0,29,3,60,3,64,0,34,3,66,3,68,0,39,3,73,3,74,0,42,3,79,3,80,0,44,3,85,3,93,0,46,3,97,3,102,0,55,3,107,3,110,0,61,3,114,3,116,0,65,3,119,3,
    122,0,68,3,124,3,124,0,72,3,126,3,128,0,73,3,131,3,134,0,76,3,136,3,140,0,80,3,148,3,152,0,85,3,154,3,160,0,90,3,174,3,174,0,97,4,17,4,17,0,98,4,20,4,20,0,99,4,22,4,22,0,100,4,25,4,
    25,0,101,4,223,4,233,0,102,0,2,0,67,132,189,32,1,85,201,10,52,1,2,148,2,148,0,1,3,24,3,24,0,2,3,25,3,25,0,3,3,26,134,201,32,30,130,17,34,31,3,31,130,17,32,32,130,213,32,4,130,213,32,
    36,130,17,34,37,3,37,130,17,32,38,130,225,32,4,130,225,32,42,130,17,34,43,3,43,130,17,32,44,130,237,32,4,130,237,42,48,0,5,3,49,3,49,0,6,3,50,130,249,32,7,130,249,32,54,130,17,34,55,
    3,55,130,17,40,56,3,58,0,7,3,60,3,60,130,17,34,61,3,61,130,17,34,62,3,64,130,17,34,66,3,66,130,131,46,67,3,67,0,8,3,68,3,68,0,9,3,73,3,73,130,11,34,74,3,74,130,11,34,79,3,79,130,11,
    34,80,3,80,130,11,34,85,3,85,130,41,46,86,3,86,0,10,3,87,3,87,0,11,3,88,3,90,130,65,40,91,3,91,0,12,3,92,3,92,130,23,34,93,3,93,130,41,34,97,3,97,130,17,34,98,3,98,130,29,34,99,3,99,
    130,41,34,100,3,102,130,11,34,107,3,110,130,5,34,114,3,116,130,5,40,119,3,119,0,13,3,120,3,122,130,11,40,124,3,124,0,14,3,126,3,128,130,11,34,131,3,131,130,23,34,132,3,134,130,11,34,
    136,3,136,130,23,34,137,3,137,130,17,34,138,3,140,130,17,34,148,3,148,130,143,40,149,3,149,0,15,3,150,3,152,130,17,34,154,3,155,130,149,34,156,3,156,130,23,34,157,3,157,130,23,34,158,
    3,160,130,23,46,174,3,174,0,1,4,17,4,17,0,16,4,20,4,20,130,5,34,22,4,22,130,5,34,25,4,25,130,23,50,223,4,233,0,1,0,2,0,77,1,159,1,159,0,19,2,37,2,37,130,5,34,90,2,90,130,5,52,148,2,
    148,0,19,3,20,3,20,0,6,3,22,3,22,0,25,3,24,3,24,130,195,34,25,3,25,130,87,34,29,3,29,130,111,34,30,3,30,130,17,34,31,3,31,130,17,40,32,3,34,0,2,3,35,3,35,130,23,34,36,3,36,130,23,34,
    37,3,37,130,23,34,41,3,41,130,17,34,42,3,42,130,17,34,43,3,43,130,17,34,47,3,47,130,17,40,48,3,48,0,12,3,49,3,49,130,177,40,53,3,53,0,9,3,54,3,54,130,17,34,55,3,55,130,17,40,56,3,58,
    0,20,3,59,3,59,130,23,34,60,3,60,130,23,34,61,3,61,130,23,40,62,3,64,0,21,3,65,3,65,130,23,34,66,3,66,130,161,40,67,3,67,0,13,3,73,3,73,130,5,34,79,3,79,130,5,34,85,3,85,130,23,40,
    86,3,86,0,14,3,87,3,87,130,191,40,88,3,90,0,3,3,91,3,91,130,53,34,92,3,92,130,35,34,93,3,93,130,23,40,94,3,96,0,4,3,98,3,98,130,41,34,99,3,99,130,29,40,100,3,102,0,15,3,103,3,103,130,
    11,40,105,3,105,0,26,3,111,3,111,130,11,40,114,3,116,0,16,3,117,3,117,130,11,46,118,3,118,0,17,3,120,3,122,0,22,3,123,3,123,130,17,65,185,5,34,129,3,129,130,11,34,130,3,130,130,29,
    40,132,3,134,0,23,3,135,3,135,130,17,34,136,3,136,130,17,40,138,3,140,0,24,3,141,3,141,130,17,34,144,3,146,130,11,52,148,3,148,0,18,3,149,3,149,0,10,3,150,3,152,0,5,3,153,3,153,130,
    29,34,154,3,155,130,191,34,156,3,156,130,29,34,157,3,157,130,29,34,161,3,161,130,23,8,32,174,3,174,0,19,4,16,4,16,0,25,4,17,4,17,0,13,4,21,4,21,0,26,4,22,4,22,0,9,4,25,4,25,130,29,
    48,223,4,233,0,19,0,1,0,12,0,22,0,2,0,116,0,134,130,5,92,81,5,32,0,130,21,8,34,45,0,36,0,40,0,44,0,50,0,56,0,68,0,72,0,76,0,82,0,88,0,214,4,115,4,116,4,117,4,118,4,122,4,123,90,227,
    7,44,4,151,4,152,4,153,4,154,4,158,4,159,4,90,197,6,8,33,4,191,4,192,4,193,4,194,4,195,4,197,4,198,4,199,4,200,4,201,4,202,4,203,4,204,4,205,0,4,0,0,0,200,130,3,32,206,130,3,42,212,
    0,1,0,218,0,45,0,206,0,212,130,7,42,224,0,230,0,236,0,242,0,248,0,224,130,3,42,254,1,4,1,10,1,16,1,22,1,28,130,7,32,34,130,3,32,40,130,11,32,46,130,41,32,212,136,3,38,218,1,52,0,212,
    1,58,132,3,38,224,1,64,0,242,1,70,131,3,39,1,76,1,82,1,88,1,94,136,3,38,100,1,106,1,112,1,118,134,3,36,40,1,124,1,130,132,3,34,136,1,40,131,107,35,1,142,1,148,132,131,130,115,35,0,
    230,1,154,130,67,32,160,130,111,40,130,1,166,1,160,1,172,1,178,131,155,130,15,32,16,132,43,34,10,1,184,130,135,38,28,0,1,253,118,254,172,130,5,32,138,132,5,32,48,131,5,33,252,24,130,
    211,34,1,0,40,132,5,130,4,35,0,1,255,196,132,11,32,60,131,5,33,254,152,132,5,32,212,132,17,32,160,131,5,33,1,144,131,5,40,255,176,254,112,0,1,0,0,254,130,5,33,255,216,132,11,32,60,
    131,5,33,254,152,132,5,32,12,132,17,32,140,132,5,32,120,132,17,32,32,132,41,32,76,131,59,33,2,108,132,71,32,104,132,5,32,234,131,5,33,0,80,132,11,32,164,132,89,32,116,132,41,32,106,
    132,5,32,136,133,125,132,11,32,156,131,5,33,1,164,132,11,32,226,132,89,32,180,132,11,32,166,132,5,32,136,132,17,32,40,132,107,32,172,132,77,32,204,132,89,44,140,0,200,0,1,255,236,255,
    116,0,1,0,200,132,35,32,160,132,5,34,12,0,18,130,17,34,28,0,34,130,5,34,1,4,187,130,5,38,3,4,187,4,188,4,189,130,9,36,0,0,14,0,3,130,3,32,14,130,3,36,1,255,229,255,31,132,19,32,0,130,
    5,133,53,34,112,0,118,132,53,32,206,66,117,94,32,1,130,193,54,98,0,45,0,98,0,104,0,110,0,116,0,122,0,128,0,134,0,140,0,146,0,152,130,5,33,158,0,131,1,32,164,130,17,36,134,0,164,0,170,
    130,1,34,104,0,176,134,1,34,182,0,182,130,43,34,188,0,188,130,29,35,194,0,200,0,66,171,5,32,104,130,13,135,3,36,194,0,224,0,218,135,215,34,4,126,0,130,221,33,2,188,131,5,33,1,44,131,
    5,33,3,82,132,17,32,238,132,11,32,12,132,11,32,88,131,5,33,0,230,132,11,32,138,132,23,32,112,131,5,33,4,116,132,11,32,232,132,5,32,32,132,5,32,92,132,71,32,244,132,41,32,128,132,5,
    32,248,132,5,32,28,132,47,32,176,132,11,32,228,132,77,32,200,132,47,32,72,132,11,34,8,0,4,130,213,33,1,0,94,137,11,131,19,32,160,94,107,12,130,167,130,45,34,10,0,72,130,239,44,2,68,
    70,76,84,0,14,99,97,110,115,0,28,96,9,8,32,2,130,9,32,7,96,7,8,33,22,0,96,45,5,36,0,0,3,0,6,134,11,36,1,0,2,0,5,96,15,7,43,108,111,99,108,0,56,114,99,108,116,0,62,132,5,32,68,132,5,
    38,74,115,115,50,48,0,80,132,5,32,86,132,5,132,213,33,0,1,130,125,131,3,34,0,0,1,134,11,133,17,139,23,130,34,132,27,40,3,0,8,0,24,0,40,0,7,95,251,8,131,37,139,15,32,6,130,25,32,74,
    142,31,32,152,130,157,8,46,26,0,10,4,224,4,225,4,226,4,227,4,228,4,229,4,230,4,231,4,232,4,233,0,1,0,10,1,115,1,116,1,119,1,121,1,122,1,125,1,126,1,158,1,245,2,16,130,219,34,1,0,42,
    130,29,32,18,130,79,131,99,32,2,152,41,34,2,0,8,73,99,24,50,20,3,192,0,4,4,16,4,22,0,177,4,25,4,25,0,184,4,223,130,115,32,185,178,143,32,0,131,127,130,139,32,12,130,3,131,11,132,6,
    33,0,40,130,6,36,2,100,108,110,103,130,7,131,11,33,16,115,133,11,32,56,130,15,48,21,67,104,101,114,44,32,67,97,110,115,44,32,79,115,103,101,143,15,36,44,76,97,116,110,130,40,130,2,
    32,1,65,123,8,34,0,33,129,130,15,32,20,130,3,130,2,52,33,121,48,130,33,117,6,9,42,134,72,134,247,13,1,7,2,160,130,33,102,130,18,8,40,98,2,1,1,49,15,48,13,6,9,96,134,72,1,101,3,4,2,
    1,5,0,48,113,6,10,43,6,1,4,1,130,55,2,1,4,160,99,48,97,48,44,138,17,38,28,162,30,128,28,0,60,132,1,44,79,0,98,0,115,0,111,0,108,0,101,0,116,130,3,33,62,0,130,1,33,48,49,142,80,8,65,
    4,32,129,177,108,35,237,188,18,65,167,131,91,91,164,198,175,195,247,133,137,164,85,124,122,148,24,47,118,162,206,110,144,58,160,130,11,106,48,130,4,242,48,130,3,218,160,3,2,1,2,2,19,
    51,0,0,4,82,225,164,175,224,164,197,161,102,130,194,130,13,131,86,134,193,33,1,11,130,167,49,126,49,11,48,9,6,3,85,4,6,19,2,85,83,49,19,48,17,131,12,48,8,19,10,87,97,115,104,105,110,
    103,116,111,110,49,16,48,14,131,20,45,7,19,7,82,101,100,109,111,110,100,49,30,48,28,131,17,34,10,19,21,108,6,20,35,49,40,48,38,131,31,34,3,19,31,139,31,38,100,101,32,83,105,103,110,
    130,85,37,32,80,67,65,32,50,130,228,48,48,30,23,13,50,49,48,57,48,50,49,56,50,54,49,55,90,130,14,32,50,130,14,32,49,134,14,33,48,116,212,159,134,191,32,3,150,191,35,48,130,1,34,65,
    40,11,39,1,5,0,3,130,1,15,0,130,23,9,41,10,2,130,1,1,0,181,138,246,153,0,146,90,63,220,157,185,75,234,114,63,103,108,77,114,43,95,198,14,188,80,172,155,12,142,4,111,201,248,105,179,
    227,95,84,145,229,94,31,196,149,86,93,140,46,112,11,81,46,74,80,122,245,179,130,105,105,217,221,176,198,226,229,216,85,49,116,86,202,140,51,5,103,136,49,55,16,53,39,27,62,36,20,97,
    76,200,116,178,136,87,58,68,10,184,2,249,226,80,246,133,97,238,175,62,12,82,142,242,214,250,78,86,173,57,193,73,130,173,153,55,39,147,102,61,131,173,81,189,182,122,115,79,209,235,248,
    63,109,150,175,12,71,81,71,76,61,228,191,150,189,25,180,86,88,16,122,39,91,105,25,243,99,135,31,5,60,108,253,239,234,248,217,175,50,133,168,218,194,161,92,67,121,176,89,148,201,34,
    216,209,230,32,104,74,42,230,198,157,232,87,97,86,231,130,171,206,99,34,237,110,98,111,47,197,130,98,239,4,220,15,2,126,44,61,85,222,103,109,11,148,99,41,123,241,150,170,24,246,104,
    210,246,55,195,239,61,209,63,212,16,252,201,109,209,163,19,2,3,1,0,1,163,130,1,113,48,130,1,109,48,19,6,3,85,29,37,4,12,48,10,6,8,43,6,1,5,5,7,3,3,48,29,131,20,58,14,4,22,4,20,74,172,
    25,146,125,130,169,115,2,102,216,91,99,217,167,14,188,97,238,99,48,84,131,30,44,17,4,77,48,75,164,73,48,71,49,45,48,43,130,16,35,4,11,19,36,66,48,9,42,73,114,101,108,97,110,100,32,
    79,112,101,110,93,5,44,115,32,76,105,109,105,116,101,100,49,22,48,20,131,46,49,5,19,13,50,51,50,57,53,57,43,52,54,55,52,55,53,48,31,130,21,61,29,35,4,24,48,22,128,20,230,252,95,123,
    187,34,0,88,228,114,78,181,244,33,116,35,50,230,239,172,48,86,131,32,35,31,4,79,48,130,120,47,160,73,160,71,134,69,104,116,116,112,58,47,47,99,114,108,110,57,14,34,112,107,105,131,
    21,32,47,109,152,6,33,115,47,130,153,57,67,111,100,83,105,103,80,67,65,95,50,48,49,48,45,48,55,45,48,54,46,99,114,108,48,90,135,249,39,1,1,4,78,48,76,48,74,135,15,35,48,2,134,62,134,
    98,34,119,119,119,147,98,34,101,114,116,155,91,34,116,48,12,131,179,39,19,1,1,255,4,2,48,0,67,191,14,9,12,3,130,1,1,0,118,229,122,81,116,170,115,197,163,183,52,69,78,46,90,179,65,220,
    77,138,107,180,87,165,127,117,236,12,41,187,225,61,148,247,5,219,231,147,89,20,239,143,139,70,169,123,128,192,131,198,216,96,208,36,57,36,136,59,195,147,49,30,212,188,146,185,53,249,
    45,6,230,127,248,106,136,121,185,227,118,41,37,44,243,229,167,42,195,255,144,224,185,142,236,255,101,16,225,55,165,229,213,147,214,89,48,12,73,184,170,155,5,9,214,21,236,49,188,211,
    93,86,252,157,53,142,230,159,245,61,202,90,39,149,252,133,36,46,18,87,101,18,212,4,79,250,80,32,214,168,215,119,139,237,116,4,80,182,142,196,145,143,78,49,143,234,20,126,71,213,251,
    224,22,246,112,95,90,207,31,128,23,139,83,125,243,118,0,190,163,246,120,92,230,234,132,123,72,164,197,170,235,168,187,7,6,26,232,177,209,239,218,194,242,234,247,76,55,61,236,22,223,
    89,100,178,39,193,139,209,16,113,254,39,194,170,67,159,170,181,204,104,73,231,112,101,148,74,51,196,25,14,237,235,79,220,25,91,113,216,48,130,6,112,48,130,4,88,68,245,5,37,10,97,12,
    82,76,0,131,0,32,3,65,44,14,34,48,129,136,68,77,84,34,50,48,48,68,237,5,32,41,66,188,9,57,82,111,111,116,32,67,101,114,116,105,102,105,99,97,116,101,32,65,117,116,104,111,114,105,116,
    121,68,247,8,41,49,48,48,55,48,54,50,48,52,48,68,247,5,32,53,133,14,32,53,131,14,32,48,69,151,127,69,1,31,8,255,233,14,100,80,121,103,181,196,227,253,9,0,76,158,148,172,247,86,104,
    234,68,216,207,197,88,79,169,165,118,124,109,69,186,211,57,146,180,164,30,249,249,101,130,228,23,210,143,253,68,156,8,232,101,147,206,44,85,132,191,125,8,227,46,43,168,65,43,24,183,
    162,75,110,73,76,107,21,7,222,209,210,194,137,30,113,148,205,181,127,75,180,175,8,216,204,136,214,107,23,148,58,147,206,38,63,236,230,254,52,152,87,213,29,93,73,246,178,42,46,213,133,
    187,89,63,248,144,180,43,131,116,202,43,179,59,70,227,240,70,73,193,23,102,84,201,28,189,29,196,85,98,87,114,248,103,185,37,32,52,222,93,166,165,149,94,171,40,128,205,213,178,158,229,
    3,181,99,211,178,20,200,193,200,138,38,10,89,127,7,236,255,14,237,128,18,53,76,18,166,190,82,91,245,166,218,224,139,11,72,119,214,133,71,213,16,185,198,232,170,238,139,106,45,5,92,
    96,198,180,42,91,156,35,28,95,69,227,26,20,30,111,55,203,25,51,128,106,137,77,163,106,102,99,120,147,213,48,207,149,31,69,1,7,40,227,48,130,1,223,48,16,6,9,71,237,6,38,21,1,4,3,2,1,
    0,68,254,10,68,135,20,32,25,136,48,55,20,2,4,12,30,10,0,83,0,117,0,98,0,67,0,65,48,11,6,3,85,29,15,4,131,70,34,134,48,15,67,251,8,37,5,48,3,1,1,255,68,225,12,51,213,246,86,203,143,
    232,162,92,98,104,209,61,148,144,91,215,206,154,24,196,68,225,63,40,82,111,111,67,101,114,65,117,116,68,225,6,35,54,45,50,51,68,225,71,150,91,35,116,48,129,157,131,230,41,32,4,129,
    149,48,129,146,48,129,143,65,28,8,38,46,3,48,129,129,48,61,69,76,7,35,2,1,22,49,69,76,24,57,80,75,73,47,100,111,99,115,47,67,80,83,47,100,101,102,97,117,108,116,46,104,116,109,48,64,
    136,62,54,2,48,52,30,50,32,29,0,76,0,101,0,103,0,97,0,108,0,95,0,80,0,111,130,7,36,105,0,99,0,121,130,13,34,83,0,116,130,23,32,116,130,31,32,109,130,3,32,110,130,13,34,46,32,29,68,
    70,14,10,18,3,130,2,1,0,26,116,239,87,79,41,123,196,22,133,120,184,80,211,34,252,9,157,172,130,151,248,52,255,42,44,151,149,18,229,228,191,207,191,147,200,227,52,169,219,129,184,220,
    30,0,190,210,53,111,175,229,127,121,149,119,229,2,212,241,235,216,205,78,30,27,97,162,194,90,35,26,240,140,168,98,81,69,103,8,227,63,60,30,147,248,48,133,23,200,57,64,166,215,14,179,
    33,41,229,165,161,105,140,34,147,204,116,152,231,161,71,67,242,83,172,192,15,48,105,127,254,210,37,32,109,111,97,211,223,7,213,217,114,0,44,105,134,118,61,81,219,166,57,72,201,55,97,
    109,7,221,83,25,203,167,214,97,194,191,226,131,171,15,224,107,155,149,214,125,40,81,176,137,74,81,164,154,108,200,183,31,74,26,14,105,169,215,220,193,126,209,73,112,170,182,173,187,
    114,71,99,23,250,166,214,162,166,134,236,168,16,68,155,99,182,178,105,137,6,199,70,134,122,24,63,232,197,29,33,213,123,249,2,35,45,197,65,203,191,29,76,200,22,239,177,156,127,252,34,
    75,73,138,110,21,227,166,127,118,91,209,83,121,145,133,157,213,210,219,61,115,53,243,60,174,84,178,82,71,106,192,170,19,149,210,142,17,218,153,103,94,50,140,251,55,133,209,220,117,
    133,156,135,198,90,87,133,194,191,221,13,143,140,155,45,235,180,238,207,39,211,181,94,105,250,164,22,4,1,167,36,103,115,207,77,79,182,222,5,86,151,122,247,233,82,77,244,119,5,79,133,
    198,216,11,241,142,237,66,9,209,13,118,227,35,86,120,34,38,54,190,202,177,140,110,170,29,228,133,218,71,51,98,143,164,201,145,51,95,113,30,64,175,152,101,201,34,232,66,33,37,138,28,
    45,96,217,55,137,65,137,42,22,15,215,97,60,148,104,96,82,239,214,71,153,160,128,64,238,21,129,119,62,156,224,83,24,26,80,29,56,149,155,30,102,51,19,39,57,23,120,135,54,206,78,195,95,
    178,245,61,71,83,182,224,229,219,11,97,61,42,215,146,44,206,55,90,62,64,66,49,164,31,16,8,194,86,156,191,36,93,81,2,157,106,121,210,23,211,218,193,148,142,7,123,37,113,68,171,6,106,
    230,212,198,223,35,154,150,117,197,49,130,21,105,48,130,21,101,2,1,1,48,129,149,69,174,127,75,234,24,76,146,10,36,160,129,176,48,25,76,190,8,35,9,3,49,12,76,144,10,34,4,48,28,138,13,
    35,11,49,14,48,139,29,34,21,48,47,137,56,8,38,4,49,34,4,32,67,213,245,194,113,131,239,38,35,2,20,155,122,151,235,189,254,141,122,225,205,35,181,116,140,169,105,209,118,187,93,162,48,
    68,138,78,40,12,49,54,48,52,160,20,128,18,116,132,17,37,161,28,128,26,104,116,119,189,5,32,119,73,32,16,75,131,14,9,11,4,130,1,0,103,246,85,59,162,82,249,147,69,135,189,191,181,226,
    152,155,126,198,153,82,192,84,57,192,55,40,241,163,65,51,189,120,10,212,41,23,66,232,234,144,0,80,170,146,236,8,240,36,152,137,201,79,116,218,242,173,231,235,147,156,214,214,59,189,
    234,154,246,192,66,103,27,46,177,190,29,231,253,108,58,161,192,163,87,92,46,128,39,189,170,4,78,8,140,213,128,180,52,18,152,254,220,126,251,75,103,111,96,167,121,36,170,17,37,24,254,
    16,62,167,207,239,120,122,59,231,241,198,222,222,236,13,225,202,223,102,155,77,146,128,173,55,12,151,39,160,242,250,183,89,241,122,69,218,61,225,6,20,13,159,40,19,9,187,209,74,195,
    184,133,216,0,150,84,130,217,164,206,60,248,24,95,229,200,5,40,136,121,73,140,251,92,245,213,65,4,187,25,111,53,251,25,57,208,248,15,238,195,237,84,62,140,131,153,60,104,27,15,251,
    117,17,4,117,107,241,192,131,220,184,87,33,174,225,200,72,64,51,49,203,127,171,181,246,60,44,122,191,190,56,29,132,196,208,40,23,31,113,241,137,161,130,18,241,48,130,18,237,65,94,8,
    38,3,3,1,49,130,18,221,130,19,32,217,78,155,12,33,18,202,130,18,35,198,2,1,3,78,155,17,36,130,1,85,6,11,78,4,6,50,9,16,1,4,160,130,1,68,4,130,1,64,48,130,1,60,2,1,1,134,90,36,132,89,
    10,3,1,78,135,18,8,45,172,2,64,35,216,121,228,214,66,138,118,169,223,113,202,86,124,137,228,246,252,53,109,15,233,226,252,46,192,48,232,247,2,6,97,192,200,29,40,77,24,19,50,48,50,50,
    130,64,58,55,50,49,52,48,48,55,46,55,53,49,90,48,4,128,2,1,244,160,129,212,164,129,209,48,129,206,73,142,84,34,41,48,39,76,75,5,122,52,10,76,67,10,46,80,117,101,114,116,111,32,82,105,
    99,111,49,38,48,36,133,42,8,33,29,84,104,97,108,101,115,32,84,83,83,32,69,83,78,58,56,57,55,65,45,69,51,53,54,45,49,55,48,49,49,37,48,35,131,39,34,3,19,28,73,225,9,61,84,105,109,101,
    45,83,116,97,109,112,32,83,101,114,118,105,99,101,160,130,14,68,48,130,4,245,48,130,3,221,79,129,9,42,1,96,7,32,240,146,72,218,190,235,0,130,0,33,1,96,70,77,14,33,48,124,65,4,84,134,
    217,34,3,19,29,148,177,79,127,14,40,49,49,52,49,57,48,50,50,48,79,127,5,34,52,49,49,134,14,65,163,208,79,218,32,8,254,180,49,128,28,208,242,198,204,241,82,51,62,9,141,245,53,164,90,
    205,164,241,45,66,101,76,141,231,196,139,204,181,183,215,159,43,101,159,206,226,94,188,25,190,18,144,79,226,5,68,164,227,37,94,176,18,142,86,137,219,44,189,231,3,2,23,154,198,33,159,
    144,81,209,21,207,35,213,203,139,60,213,170,201,84,128,115,243,76,244,109,50,214,110,250,59,114,128,42,97,228,199,234,184,24,154,253,42,116,30,56,251,148,64,235,52,116,127,11,253,127,
    196,38,153,201,10,96,33,30,229,1,18,72,105,21,97,231,47,150,169,186,166,109,75,5,34,129,121,80,124,48,204,145,73,164,229,241,98,208,52,185,107,171,104,170,93,9,76,192,206,30,176,144,
    166,96,50,68,107,92,253,196,255,14,92,179,133,59,113,136,152,141,148,70,150,109,198,175,235,201,130,76,147,176,165,183,56,24,147,182,70,150,253,232,237,165,111,146,234,155,191,53,236,
    215,22,160,51,194,228,131,141,226,8,152,227,158,17,171,229,3,211,189,86,144,158,198,141,115,233,80,169,167,65,27,223,221,56,186,79,218,8,36,27,48,130,1,23,74,198,10,51,167,155,30,248,
    183,168,23,224,154,225,117,50,9,117,124,208,235,190,31,99,74,141,13,50,99,58,92,138,49,144,243,67,123,124,70,27,197,51,104,90,133,109,85,74,141,63,37,84,105,109,83,116,97,79,111,12,
    32,49,74,141,71,150,91,79,111,15,80,220,18,32,8,67,194,14,9,12,3,130,1,1,0,21,52,152,8,155,58,5,166,137,80,117,79,39,97,215,23,239,36,165,40,67,247,231,218,160,229,15,210,22,158,222,
    226,62,93,3,85,126,146,203,230,41,214,65,214,56,99,176,27,54,34,99,78,34,220,141,99,107,182,55,8,218,224,147,113,21,255,182,203,242,147,112,255,6,19,222,199,242,136,43,245,207,149,
    111,91,135,23,184,45,157,237,197,242,249,239,208,38,251,244,174,38,110,24,186,75,68,8,134,163,220,35,199,55,233,74,158,63,2,140,46,109,246,39,235,206,247,11,138,91,99,74,242,255,91,
    3,152,16,187,221,128,143,171,159,236,1,128,184,214,7,34,29,34,102,137,62,109,203,137,165,131,202,41,141,103,189,45,132,107,88,51,152,95,9,254,42,207,99,131,22,242,254,63,129,167,164,
    38,139,31,145,206,160,110,137,154,227,13,96,20,49,138,128,168,111,238,217,83,215,238,119,184,113,145,217,52,181,168,198,30,55,1,159,26,216,226,79,20,53,6,84,4,102,38,228,236,241,95,
    9,239,138,70,20,209,76,36,109,205,105,210,131,115,68,19,76,182,168,185,212,48,130,6,113,48,130,4,89,68,248,5,37,10,97,9,129,42,0,131,0,32,2,65,44,14,79,132,147,46,49,50,49,51,54,53,
    53,90,23,13,50,53,48,55,48,130,14,32,52,131,14,69,154,126,79,130,31,8,255,169,29,13,188,119,17,138,58,32,236,252,19,151,245,250,127,105,148,107,116,84,16,213,165,10,0,130,133,251,237,
    124,104,75,44,95,197,195,229,97,194,118,183,62,102,43,91,240,21,83,39,4,49,31,65,27,26,149,29,206,9,19,142,124,97,48,89,177,48,68,15,241,96,136,132,84,67,12,215,77,184,56,8,179,66,
    221,147,172,214,115,48,87,38,130,163,69,13,208,234,245,71,129,205,191,36,96,50,88,96,70,242,88,71,134,50,132,30,116,97,103,145,95,129,84,177,207,147,76,146,193,196,166,93,209,97,19,
    110,40,198,26,249,134,128,187,223,97,252,70,193,39,29,36,103,18,114,26,33,138,175,75,100,137,80,98,177,93,253,119,31,61,240,87,117,172,189,138,66,77,64,81,209,15,156,6,62,103,127,245,
    102,192,3,150,68,126,239,208,75,253,110,229,154,202,177,168,242,122,42,10,49,240,218,78,6,145,182,136,8,53,232,120,28,176,233,153,205,60,231,47,68,186,167,244,220,100,189,164,1,193,
    32,9,147,120,205,252,188,192,201,68,93,94,22,156,1,5,79,34,77,79,130,7,38,230,48,130,1,226,48,16,79,130,26,32,213,68,154,19,16,15,130,1,14,40,160,6,3,85,29,32,1,1,255,79,133,171,10,
    14,7,230,136,81,13,226,198,224,152,63,129,113,3,61,157,163,161,33,111,179,235,166,204,245,49,190,207,5,226,169,254,250,87,109,25,48,179,194,197,102,201,106,223,245,231,240,120,189,
    199,168,158,37,227,249,188,237,107,84,87,8,43,81,130,68,18,251,185,83,140,204,244,96,18,138,118,204,64,64,65,155,220,92,23,255,92,249,94,23,53,152,36,86,75,116,239,66,16,200,175,191,
    127,198,127,242,55,125,90,63,28,242,153,121,74,145,82,0,175,56,15,23,245,47,121,129,101,217,169,181,107,228,199,206,246,202,122,0,111,75,48,68,36,34,60,207,237,3,165,150,143,89,41,
    188,182,253,4,225,112,159,50,74,39,253,85,175,47,254,182,229,142,51,187,98,95,154,219,87,64,233,241,206,153,102,144,140,255,106,98,127,221,197,74,11,145,38,226,57,236,25,74,113,99,
    157,123,33,109,195,156,163,162,60,250,127,125,150,106,144,120,166,109,210,225,156,249,29,252,56,216,148,244,198,165,10,150,134,164,189,158,26,174,4,66,131,184,181,128,155,34,56,32,
    181,37,229,100,236,247,244,191,126,99,89,37,15,122,46,57,87,118,162,113,170,6,138,15,137,22,186,97,167,17,203,154,216,14,71,154,128,197,208,205,167,208,239,125,131,240,225,59,113,9,
    223,93,116,152,34,8,97,218,176,80,30,111,189,241,225,0,223,231,49,7,164,147,58,247,101,71,120,232,248,168,72,171,247,222,114,126,97,107,111,119,169,129,203,167,9,172,57,187,236,198,
    203,216,130,180,114,205,29,244,184,133,1,30,128,251,27,137,42,84,57,178,91,218,200,13,85,153,122,135,115,59,8,230,152,45,234,141,224,51,46,18,41,245,192,47,84,39,33,247,200,172,78,
    218,40,184,177,169,219,150,178,167,66,162,201,207,25,65,77,224,134,249,42,154,163,17,102,48,211,187,116,50,75,223,99,123,245,153,138,47,27,199,33,175,89,181,174,220,68,60,151,80,113,
    215,161,210,197,85,227,105,222,87,193,209,222,48,192,253,204,230,77,251,13,191,93,79,233,157,30,25,56,47,188,207,88,5,46,239,13,160,80,53,218,239,9,39,28,213,179,126,53,30,8,186,218,
    54,219,211,95,143,222,116,136,73,18,161,130,2,210,48,130,2,59,2,1,1,48,129,252,161,76,86,213,34,162,35,10,130,222,8,38,7,6,5,43,14,3,2,26,3,21,0,251,50,146,116,117,139,68,220,200,151,
    42,135,87,21,217,128,169,217,208,162,160,129,131,48,129,128,164,126,70,179,126,32,13,78,131,8,46,1,5,5,0,2,5,0,229,131,1,88,48,34,24,15,78,11,8,37,50,49,51,52,52,90,136,16,33,56,50,
    133,16,35,48,119,48,61,78,120,9,39,4,1,49,47,48,45,48,10,134,64,34,2,1,0,130,11,42,1,0,2,2,38,155,2,1,255,48,7,130,16,35,2,2,17,23,133,32,34,132,82,216,130,15,33,48,54,138,62,38,2,
    49,40,48,38,48,12,137,17,37,3,2,160,10,48,8,130,38,37,2,3,7,161,32,161,135,11,34,1,134,160,72,158,11,130,178,8,145,3,129,129,0,116,74,91,135,48,184,70,206,248,24,215,22,3,66,91,120,
    167,143,223,117,229,24,81,43,113,34,168,19,74,81,245,131,172,250,153,109,94,243,236,207,17,45,155,30,152,104,157,21,116,235,203,200,179,226,41,68,21,186,85,15,202,122,249,122,119,65,
    245,165,180,4,131,87,16,199,158,243,126,9,149,214,203,157,94,251,215,196,117,9,8,110,200,252,109,22,100,64,96,227,110,51,121,113,251,87,24,72,247,64,199,54,163,106,122,153,191,141,
    249,50,138,191,21,218,163,97,201,180,228,248,49,130,3,13,48,130,3,9,2,1,1,48,129,147,65,209,125,36,2,19,51,0,0,78,194,19,82,89,11,36,130,1,74,48,26,65,249,8,35,9,3,49,13,80,95,12,82,
    61,16,8,34,142,67,203,227,164,190,46,42,171,212,203,43,35,8,232,205,139,68,234,185,212,47,255,166,39,214,36,140,254,99,213,5,48,129,250,138,64,8,50,2,47,49,129,234,48,129,231,48,129,
    228,48,129,189,4,32,2,18,163,189,163,115,46,151,91,69,167,175,30,234,43,202,126,149,210,255,143,92,23,9,226,94,72,247,168,176,101,185,48,129,152,67,10,130,65,56,21,8,34,34,4,32,37,
    46,2,71,58,34,79,60,96,121,253,64,44,206,58,54,42,34,224,225,1,169,119,138,25,204,161,18,113,90,162,94,75,47,14,9,6,4,130,1,0,179,153,93,173,181,124,187,96,138,28,14,169,16,7,77,118,
    110,13,241,198,105,105,62,101,245,197,125,37,59,124,91,188,140,159,172,221,187,253,144,49,247,253,229,14,43,128,5,31,194,27,110,77,189,181,45,96,95,18,130,239,101,220,19,152,49,240,
    174,235,105,233,65,79,71,34,116,80,84,140,162,158,195,214,145,13,56,16,201,32,108,225,240,146,62,160,77,209,72,130,162,223,90,180,74,65,119,5,234,10,230,103,147,163,119,111,202,75,
    148,243,204,156,102,128,165,0,20,8,186,190,145,154,78,138,113,216,160,200,152,245,68,196,73,243,59,44,22,72,61,118,19,109,47,156,241,110,132,218,8,208,167,188,241,119,120,84,88,49,
    109,234,117,253,154,15,81,240,136,186,177,80,50,226,119,237,252,164,118,55,11,108,250,83,222,209,231,111,7,180,201,210,130,223,229,156,205,70,79,233,2,145,70,161,40,127,34,123,18,205,
    145,254,186,53,251,12,86,12,161,105,125,62,129,240,234,214,170,186,51,61,50,8,150,34,83,138,109,64,210,44,209,203,149,117,39,61,215,225,238,241,0,0,0,5,250,39,184,61,105,
};

static const char* GetDefaultCompressedFontDataTTF(int* out_size)
{
    *out_size = gadugi_ttf_compressed_size;
    return (const char*)gadugi_ttf_compressed_data;
}
#endif // #ifndef IMGUI_DISABLE_DEFAULT_FONT

#endif // #ifndef IMGUI_DISABLE

// dear imgui, v1.92.1 WIP
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
// [SECTION] ImFontAtlas, ImFontAtlasBuilder
// [SECTION] ImFontAtlas: backend for stb_truetype
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
#include <stdint.h>     // intptr_t

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
    colors[ImGuiCol_InputTextCursor]        = colors[ImGuiCol_Text];
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
    colors[ImGuiCol_TreeLines]              = colors[ImGuiCol_Border];
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
    colors[ImGuiCol_InputTextCursor]        = colors[ImGuiCol_Text];
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
    colors[ImGuiCol_TreeLines]              = colors[ImGuiCol_Border];
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
    colors[ImGuiCol_InputTextCursor]        = colors[ImGuiCol_Text];
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
    colors[ImGuiCol_TreeLines]              = colors[ImGuiCol_Border];
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

ImDrawListSharedData::~ImDrawListSharedData()
{
    IM_ASSERT(DrawLists.Size == 0);
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
    _SetDrawListSharedData(shared_data);
}

ImDrawList::~ImDrawList()
{
    _ClearFreeMemory();
    _SetDrawListSharedData(NULL);
}

void ImDrawList::_SetDrawListSharedData(ImDrawListSharedData* data)
{
    if (_Data != NULL)
        _Data->DrawLists.find_erase_unsorted(this);
    _Data = data;
    if (_Data != NULL)
        _Data->DrawLists.push_back(this);
}

// Initialize before use in a new frame. We always have a command ready in the buffer.
// In the majority of cases, you would want to call PushClipRect() and PushTexture() after this.
void ImDrawList::_ResetForNewFrame()
{
    // Verify that the ImDrawCmd fields we want to memcmp() are contiguous in memory.
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, ClipRect) == 0);
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, TexRef) == sizeof(ImVec4));
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, VtxOffset) == sizeof(ImVec4) + sizeof(ImTextureRef));
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
    _TextureStack.resize(0);
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
    _TextureStack.clear();
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
    draw_cmd.TexRef = _CmdHeader.TexRef;
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

// Compare ClipRect, TexRef and VtxOffset with a single memcmp()
#define ImDrawCmd_HeaderSize                            (offsetof(ImDrawCmd, VtxOffset) + sizeof(unsigned int))
#define ImDrawCmd_HeaderCompare(CMD_LHS, CMD_RHS)       (memcmp(CMD_LHS, CMD_RHS, ImDrawCmd_HeaderSize))    // Compare ClipRect, TexRef, VtxOffset
#define ImDrawCmd_HeaderCopy(CMD_DST, CMD_SRC)          (memcpy(CMD_DST, CMD_SRC, ImDrawCmd_HeaderSize))    // Copy ClipRect, TexRef, VtxOffset
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

void ImDrawList::_OnChangedTexture()
{
    // If current command is used with different settings we need to add a new command
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && curr_cmd->TexRef != _CmdHeader.TexRef)
    {
        AddDrawCmd();
        return;
    }

    // Unlike other _OnChangedXXX functions this may be called by ImFontAtlasUpdateDrawListsTextures() in more locations so we need to handle this case.
    if (curr_cmd->UserCallback != NULL)
        return;

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && ImDrawCmd_AreSequentialIdxOffset(prev_cmd, curr_cmd) && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }
    curr_cmd->TexRef = _CmdHeader.TexRef;
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

void ImDrawList::PushTexture(ImTextureRef tex_ref)
{
    _TextureStack.push_back(tex_ref);
    _CmdHeader.TexRef = tex_ref;
    if (tex_ref._TexData != NULL)
        IM_ASSERT(tex_ref._TexData->WantDestroyNextFrame == false);
    _OnChangedTexture();
}

void ImDrawList::PopTexture()
{
    _TextureStack.pop_back();
    _CmdHeader.TexRef = (_TextureStack.Size == 0) ? ImTextureRef() : _TextureStack.Data[_TextureStack.Size - 1];
    _OnChangedTexture();
}

// This is used by ImGui::PushFont()/PopFont(). It works because we never use _TextureIdStack[] elsewhere than in PushTexture()/PopTexture().
void ImDrawList::_SetTexture(ImTextureRef tex_ref)
{
    if (_CmdHeader.TexRef == tex_ref)
        return;
    _CmdHeader.TexRef = tex_ref;
    _TextureStack.back() = tex_ref;
    _OnChangedTexture();
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

                // Add temporary vertices for the outer edges
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

            // Add vertices for each point on the line
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

void ImDrawList::AddImage(ImTextureRef tex_ref, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = tex_ref != _CmdHeader.TexRef;
    if (push_texture_id)
        PushTexture(tex_ref);

    PrimReserve(6, 4);
    PrimRectUV(p_min, p_max, uv_min, uv_max, col);

    if (push_texture_id)
        PopTexture();
}

void ImDrawList::AddImageQuad(ImTextureRef tex_ref, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& uv1, const ImVec2& uv2, const ImVec2& uv3, const ImVec2& uv4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = tex_ref != _CmdHeader.TexRef;
    if (push_texture_id)
        PushTexture(tex_ref);

    PrimReserve(6, 4);
    PrimQuadUV(p1, p2, p3, p4, uv1, uv2, uv3, uv4, col);

    if (push_texture_id)
        PopTexture();
}

void ImDrawList::AddImageRounded(ImTextureRef tex_ref, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, float rounding, ImDrawFlags flags)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    flags = FixRectCornerFlags(flags);
    if (rounding < 0.5f || (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone)
    {
        AddImage(tex_ref, p_min, p_max, uv_min, uv_max, col);
        return;
    }

    const bool push_texture_id = tex_ref != _CmdHeader.TexRef;
    if (push_texture_id)
        PushTexture(tex_ref);

    int vert_start_idx = VtxBuffer.Size;
    PathRect(p_min, p_max, rounding, flags);
    PathFillConvex(col);
    int vert_end_idx = VtxBuffer.Size;
    ImGui::ShadeVertsLinearUV(this, vert_start_idx, vert_end_idx, p_min, p_max, uv_min, uv_max, true);

    if (push_texture_id)
        PopTexture();
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
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TexRef, VtxOffset
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
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TexRef, VtxOffset
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
    Textures = NULL;
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

// FIXME-NEWATLAS: Oversample specification could be more dynamic. For now, favoring automatic selection.
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
// [SECTION] ImTextureData
//-----------------------------------------------------------------------------
// - ImTextureData::Create()
// - ImTextureData::DestroyPixels()
//-----------------------------------------------------------------------------

int ImTextureDataGetFormatBytesPerPixel(ImTextureFormat format)
{
    switch (format)
    {
    case ImTextureFormat_Alpha8: return 1;
    case ImTextureFormat_RGBA32: return 4;
    }
    IM_ASSERT(0);
    return 0;
}

const char* ImTextureDataGetStatusName(ImTextureStatus status)
{
    switch (status)
    {
    case ImTextureStatus_OK: return "OK";
    case ImTextureStatus_Destroyed: return "Destroyed";
    case ImTextureStatus_WantCreate: return "WantCreate";
    case ImTextureStatus_WantUpdates: return "WantUpdates";
    case ImTextureStatus_WantDestroy: return "WantDestroy";
    }
    return "N/A";
}

const char* ImTextureDataGetFormatName(ImTextureFormat format)
{
    switch (format)
    {
    case ImTextureFormat_Alpha8: return "Alpha8";
    case ImTextureFormat_RGBA32: return "RGBA32";
    }
    return "N/A";
}

void ImTextureData::Create(ImTextureFormat format, int w, int h)
{
    DestroyPixels();
    Format = format;
    Width = w;
    Height = h;
    BytesPerPixel = ImTextureDataGetFormatBytesPerPixel(format);
    UseColors = false;
    Pixels = (unsigned char*)IM_ALLOC(Width * Height * BytesPerPixel);
    IM_ASSERT(Pixels != NULL);
    memset(Pixels, 0, Width * Height * BytesPerPixel);
    UsedRect.x = UsedRect.y = UsedRect.w = UsedRect.h = 0;
    UpdateRect.x = UpdateRect.y = (unsigned short)~0;
    UpdateRect.w = UpdateRect.h = 0;
}

void ImTextureData::DestroyPixels()
{
    if (Pixels)
        IM_FREE(Pixels);
    Pixels = NULL;
    UseColors = false;
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontAtlas, ImFontAtlasBuilder
//-----------------------------------------------------------------------------
// - Default texture data encoded in ASCII
// - ImFontAtlas()
// - ImFontAtlas::Clear()
// - ImFontAtlas::CompactCache()
// - ImFontAtlas::ClearInputData()
// - ImFontAtlas::ClearTexData()
// - ImFontAtlas::ClearFonts()
//-----------------------------------------------------------------------------
// - ImFontAtlasUpdateNewFrame()
// - ImFontAtlasTextureBlockConvert()
// - ImFontAtlasTextureBlockPostProcess()
// - ImFontAtlasTextureBlockPostProcessMultiply()
// - ImFontAtlasTextureBlockFill()
// - ImFontAtlasTextureBlockCopy()
// - ImFontAtlasTextureBlockQueueUpload()
//-----------------------------------------------------------------------------
// - ImFontAtlas::GetTexDataAsAlpha8() [legacy]
// - ImFontAtlas::GetTexDataAsRGBA32() [legacy]
// - ImFontAtlas::Build() [legacy]
//-----------------------------------------------------------------------------
// - ImFontAtlas::AddFont()
// - ImFontAtlas::AddFontDefault()
// - ImFontAtlas::AddFontFromFileTTF()
// - ImFontAtlas::AddFontFromMemoryTTF()
// - ImFontAtlas::AddFontFromMemoryCompressedTTF()
// - ImFontAtlas::AddFontFromMemoryCompressedBase85TTF()
// - ImFontAtlas::RemoveFont()
// - ImFontAtlasBuildNotifySetFont()
//-----------------------------------------------------------------------------
// - ImFontAtlas::AddCustomRect()
// - ImFontAtlas::RemoveCustomRect()
// - ImFontAtlas::GetCustomRect()
// - ImFontAtlas::AddCustomRectFontGlyph() [legacy]
// - ImFontAtlas::AddCustomRectFontGlyphForSize() [legacy]
// - ImFontAtlasGetMouseCursorTexData()
//-----------------------------------------------------------------------------
// - ImFontAtlasBuildMain()
// - ImFontAtlasBuildSetupFontLoader()
// - ImFontAtlasBuildPreloadAllGlyphRanges()
// - ImFontAtlasBuildUpdatePointers()
// - ImFontAtlasBuildRenderBitmapFromString()
// - ImFontAtlasBuildUpdateBasicTexData()
// - ImFontAtlasBuildUpdateLinesTexData()
// - ImFontAtlasBuildAddFont()
// - ImFontAtlasBuildSetupFontBakedEllipsis()
// - ImFontAtlasBuildSetupFontBakedBlanks()
// - ImFontAtlasBuildSetupFontBakedFallback()
// - ImFontAtlasBuildSetupFontSpecialGlyphs()
// - ImFontAtlasBuildDiscardBakes()
// - ImFontAtlasBuildDiscardFontBakedGlyph()
// - ImFontAtlasBuildDiscardFontBaked()
// - ImFontAtlasBuildDiscardFontBakes()
//-----------------------------------------------------------------------------
// - ImFontAtlasAddDrawListSharedData()
// - ImFontAtlasRemoveDrawListSharedData()
// - ImFontAtlasUpdateDrawListsTextures()
// - ImFontAtlasUpdateDrawListsSharedData()
//-----------------------------------------------------------------------------
// - ImFontAtlasBuildSetTexture()
// - ImFontAtlasBuildAddTexture()
// - ImFontAtlasBuildMakeSpace()
// - ImFontAtlasBuildRepackTexture()
// - ImFontAtlasBuildGrowTexture()
// - ImFontAtlasBuildRepackOrGrowTexture()
// - ImFontAtlasBuildGetTextureSizeEstimate()
// - ImFontAtlasBuildCompactTexture()
// - ImFontAtlasBuildInit()
// - ImFontAtlasBuildDestroy()
//-----------------------------------------------------------------------------
// - ImFontAtlasPackInit()
// - ImFontAtlasPackAllocRectEntry()
// - ImFontAtlasPackReuseRectEntry()
// - ImFontAtlasPackDiscardRect()
// - ImFontAtlasPackAddRect()
// - ImFontAtlasPackGetRect()
//-----------------------------------------------------------------------------
// - ImFontBaked_BuildGrowIndex()
// - ImFontBaked_BuildLoadGlyph()
// - ImFontBaked_BuildLoadGlyphAdvanceX()
// - ImFontAtlasDebugLogTextureRequests()
//-----------------------------------------------------------------------------
// - ImFontAtlasGetFontLoaderForStbTruetype()
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

#define IM_FONTGLYPH_INDEX_UNUSED           ((ImU16)-1) // 0xFFFF
#define IM_FONTGLYPH_INDEX_NOT_FOUND        ((ImU16)-2) // 0xFFFE

ImFontAtlas::ImFontAtlas()
{
    memset(this, 0, sizeof(*this));
    TexDesiredFormat = ImTextureFormat_RGBA32;
    TexGlyphPadding = 1;
    TexMinWidth = 512;
    TexMinHeight = 128;
    TexMaxWidth = 8192;
    TexMaxHeight = 8192;
    RendererHasTextures = false; // Assumed false by default, as apps can call e.g Atlas::Build() after backend init and before ImGui can update.
    TexNextUniqueID = 1;
    FontNextUniqueID = 1;
    Builder = NULL;
}

ImFontAtlas::~ImFontAtlas()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    RendererHasTextures = false; // Full Clear() is supported, but ClearTexData() only isn't.
    ClearFonts();
    ClearTexData();
    TexList.clear_delete();
    TexData = NULL;
}

void ImFontAtlas::Clear()
{
    bool backup_renderer_has_textures = RendererHasTextures;
    RendererHasTextures = false; // Full Clear() is supported, but ClearTexData() only isn't.
    ClearFonts();
    ClearTexData();
    RendererHasTextures = backup_renderer_has_textures;
}

void ImFontAtlas::CompactCache()
{
    ImFontAtlasTextureCompact(this);
}

void ImFontAtlas::SetFontLoader(const ImFontLoader* font_loader)
{
    ImFontAtlasBuildSetupFontLoader(this, font_loader);
}

void ImFontAtlas::ClearInputData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");

    for (ImFont* font : Fonts)
        ImFontAtlasFontDestroyOutput(this, font);
    for (ImFontConfig& font_cfg : Sources)
        ImFontAtlasFontDestroySourceData(this, &font_cfg);
    for (ImFont* font : Fonts)
    {
        // When clearing this we lose access to the font name and other information used to build the font.
        font->Sources.clear();
        font->Flags |= ImFontFlags_NoLoadGlyphs;
    }
    Sources.clear();
}

// Clear CPU-side copy of the texture data.
void ImFontAtlas::ClearTexData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    IM_ASSERT(RendererHasTextures == false && "Not supported for dynamic atlases, but you may call Clear().");
    for (ImTextureData* tex : TexList)
        tex->DestroyPixels();
    //Locked = true; // Hoped to be able to lock this down but some reload patterns may not be happy with it.
}

void ImFontAtlas::ClearFonts()
{
    // FIXME-NEWATLAS: Illegal to remove currently bound font.
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    ImFontAtlasBuildDestroy(this);
    ClearInputData();
    Fonts.clear_delete();
    TexIsBuilt = false;
    for (ImDrawListSharedData* shared_data : DrawListSharedDatas)
        if (shared_data->FontAtlas == this)
        {
            shared_data->Font = NULL;
            shared_data->FontScale = shared_data->FontSize = 0.0f;
        }
}

static void ImFontAtlasBuildUpdateRendererHasTexturesFromContext(ImFontAtlas* atlas)
{
    // [LEGACY] Copy back the ImGuiBackendFlags_RendererHasTextures flag from ImGui context.
    // - This is the 1% exceptional case where that dependency if useful, to bypass an issue where otherwise at the
    //   time of an early call to Build(), it would be impossible for us to tell if the backend supports texture update.
    // - Without this hack, we would have quite a pitfall as many legacy codebases have an early call to Build().
    //   Whereas conversely, the portion of people using ImDrawList without ImGui is expected to be pathologically rare.
    for (ImDrawListSharedData* shared_data : atlas->DrawListSharedDatas)
        if (ImGuiContext* imgui_ctx = shared_data->Context)
        {
            atlas->RendererHasTextures = (imgui_ctx->IO.BackendFlags & ImGuiBackendFlags_RendererHasTextures) != 0;
            break;
        }
}

// Called by NewFrame() for atlases owned by a context.
// If you manually manage font atlases, you'll need to call this yourself.
// - 'frame_count' needs to be provided because we can gc/prioritize baked fonts based on their age.
// - 'frame_count' may not match those of all imgui contexts using this atlas, as contexts may be updated as different frequencies. But generally you can use ImGui::GetFrameCount() on one of your context.
void ImFontAtlasUpdateNewFrame(ImFontAtlas* atlas, int frame_count, bool renderer_has_textures)
{
    IM_ASSERT(atlas->Builder == NULL || atlas->Builder->FrameCount < frame_count); // Protection against being called twice.
    atlas->RendererHasTextures = renderer_has_textures;

    // Check that font atlas was built or backend support texture reload in which case we can build now
    if (atlas->RendererHasTextures)
    {
        atlas->TexIsBuilt = true;
        if (atlas->Builder == NULL) // This will only happen if fonts were not already loaded.
            ImFontAtlasBuildMain(atlas);
    }
    else // Legacy backend
    {
        IM_ASSERT_USER_ERROR(atlas->TexIsBuilt, "Backend does not support ImGuiBackendFlags_RendererHasTextures, and font atlas is not built! Update backend OR make sure you called ImGui_ImplXXXX_NewFrame() function for renderer backend, which should call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8().");
    }
    if (atlas->TexIsBuilt && atlas->Builder->PreloadedAllGlyphsRanges)
        IM_ASSERT_USER_ERROR(atlas->RendererHasTextures == false, "Called ImFontAtlas::Build() before ImGuiBackendFlags_RendererHasTextures got set! With new backends: you don't need to call Build().");

    // Clear BakedCurrent cache, this is important because it ensure the uncached path gets taken once.
    // We also rely on ImFontBaked* pointers never crossing frames.
    ImFontAtlasBuilder* builder = atlas->Builder;
    builder->FrameCount = frame_count;
    for (ImFont* font : atlas->Fonts)
        font->LastBaked = NULL;

    // Garbage collect BakedPool
    if (builder->BakedDiscardedCount > 0)
    {
        int dst_n = 0, src_n = 0;
        for (; src_n < builder->BakedPool.Size; src_n++)
        {
            ImFontBaked* p_src = &builder->BakedPool[src_n];
            if (p_src->WantDestroy)
                continue;
            ImFontBaked* p_dst = &builder->BakedPool[dst_n++];
            if (p_dst == p_src)
                continue;
            memcpy(p_dst, p_src, sizeof(ImFontBaked));
            builder->BakedMap.SetVoidPtr(p_dst->BakedId, p_dst);
        }
        IM_ASSERT(dst_n + builder->BakedDiscardedCount == src_n);
        builder->BakedPool.Size -= builder->BakedDiscardedCount;
        builder->BakedDiscardedCount = 0;
    }

    // Update texture status
    for (int tex_n = 0; tex_n < atlas->TexList.Size; tex_n++)
    {
        ImTextureData* tex = atlas->TexList[tex_n];
        bool remove_from_list = false;
        if (tex->Status == ImTextureStatus_OK)
        {
            tex->Updates.resize(0);
            tex->UpdateRect.x = tex->UpdateRect.y = (unsigned short)~0;
            tex->UpdateRect.w = tex->UpdateRect.h = 0;
        }
        if (tex->Status == ImTextureStatus_WantCreate && atlas->RendererHasTextures)
            IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == NULL && "Backend set texture's TexID/BackendUserData but did not update Status to OK.");

        if (tex->Status == ImTextureStatus_Destroyed)
        {
            IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == NULL && "Backend set texture Status to Destroyed but did not clear TexID/BackendUserData!");
            if (tex->WantDestroyNextFrame)
                remove_from_list = true; // Destroy was scheduled by us
            else
                tex->Status = ImTextureStatus_WantCreate; // Destroy was done was backend (e.g. freed resources mid-run)
        }
        else if (tex->WantDestroyNextFrame && tex->Status != ImTextureStatus_WantDestroy)
        {
            // Request destroy. Keep bool as it allows us to keep track of things.
            // We don't destroy pixels right away, as backend may have an in-flight copy from RAM.
            IM_ASSERT(tex->Status == ImTextureStatus_OK || tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates);
            tex->Status = ImTextureStatus_WantDestroy;
        }

        // The backend may need defer destroying by a few frames, to handle texture used by previous in-flight rendering.
        // We allow the texture staying in _WantDestroy state and increment a counter which the backend can use to take its decision.
        if (tex->Status == ImTextureStatus_WantDestroy)
            tex->UnusedFrames++;

        // If a texture has never reached the backend, they don't need to know about it.
        if (tex->Status == ImTextureStatus_WantDestroy && tex->TexID == ImTextureID_Invalid && tex->BackendUserData == NULL)
            remove_from_list = true;

        // Destroy and remove
        if (remove_from_list)
        {
            tex->DestroyPixels();
            IM_DELETE(tex);
            atlas->TexList.erase(atlas->TexList.begin() + tex_n);
            tex_n--;
        }
    }
}

void ImFontAtlasTextureBlockConvert(const unsigned char* src_pixels, ImTextureFormat src_fmt, int src_pitch, unsigned char* dst_pixels, ImTextureFormat dst_fmt, int dst_pitch, int w, int h)
{
    IM_ASSERT(src_pixels != NULL && dst_pixels != NULL);
    if (src_fmt == dst_fmt)
    {
        int line_sz = w * ImTextureDataGetFormatBytesPerPixel(src_fmt);
        for (int ny = h; ny > 0; ny--, src_pixels += src_pitch, dst_pixels += dst_pitch)
            memcpy(dst_pixels, src_pixels, line_sz);
    }
    else if (src_fmt == ImTextureFormat_Alpha8 && dst_fmt == ImTextureFormat_RGBA32)
    {
        for (int ny = h; ny > 0; ny--, src_pixels += src_pitch, dst_pixels += dst_pitch)
        {
            const ImU8* src_p = (const ImU8*)src_pixels;
            ImU32* dst_p = (ImU32*)(void*)dst_pixels;
            for (int nx = w; nx > 0; nx--)
                *dst_p++ = IM_COL32(255, 255, 255, (unsigned int)(*src_p++));
        }
    }
    else if (src_fmt == ImTextureFormat_RGBA32 && dst_fmt == ImTextureFormat_Alpha8)
    {
        for (int ny = h; ny > 0; ny--, src_pixels += src_pitch, dst_pixels += dst_pitch)
        {
            const ImU32* src_p = (const ImU32*)(void*)src_pixels;
            ImU8* dst_p = (ImU8*)dst_pixels;
            for (int nx = w; nx > 0; nx--)
                *dst_p++ = ((*src_p++) >> IM_COL32_A_SHIFT) & 0xFF;
        }
    }
    else
    {
        IM_ASSERT(0);
    }
}

// Source buffer may be written to (used for in-place mods).
// Post-process hooks may eventually be added here.
void ImFontAtlasTextureBlockPostProcess(ImFontAtlasPostProcessData* data)
{
    // Multiply operator (legacy)
    if (data->FontSrc->RasterizerMultiply != 1.0f)
        ImFontAtlasTextureBlockPostProcessMultiply(data, data->FontSrc->RasterizerMultiply);
}

void ImFontAtlasTextureBlockPostProcessMultiply(ImFontAtlasPostProcessData* data, float multiply_factor)
{
    unsigned char* pixels = (unsigned char*)data->Pixels;
    int pitch = data->Pitch;
    if (data->Format == ImTextureFormat_Alpha8)
    {
        for (int ny = data->Height; ny > 0; ny--, pixels += pitch)
        {
            ImU8* p = (ImU8*)pixels;
            for (int nx = data->Width; nx > 0; nx--, p++)
            {
                unsigned int v = ImMin((unsigned int)(*p * multiply_factor), (unsigned int)255);
                *p = (unsigned char)v;
            }
        }
    }
    else if (data->Format == ImTextureFormat_RGBA32) //-V547
    {
        for (int ny = data->Height; ny > 0; ny--, pixels += pitch)
        {
            ImU32* p = (ImU32*)(void*)pixels;
            for (int nx = data->Width; nx > 0; nx--, p++)
            {
                unsigned int a = ImMin((unsigned int)(((*p >> IM_COL32_A_SHIFT) & 0xFF) * multiply_factor), (unsigned int)255);
                *p = IM_COL32((*p >> IM_COL32_R_SHIFT) & 0xFF, (*p >> IM_COL32_G_SHIFT) & 0xFF, (*p >> IM_COL32_B_SHIFT) & 0xFF, a);
            }
        }
    }
    else
    {
        IM_ASSERT(0);
    }
}

// Fill with single color. We don't use this directly but it is convenient for anyone working on uploading custom rects.
void ImFontAtlasTextureBlockFill(ImTextureData* dst_tex, int dst_x, int dst_y, int w, int h, ImU32 col)
{
    if (dst_tex->Format == ImTextureFormat_Alpha8)
    {
        ImU8 col_a = (col >> IM_COL32_A_SHIFT) & 0xFF;
        for (int y = 0; y < h; y++)
            memset((ImU8*)dst_tex->GetPixelsAt(dst_x, dst_y + y), col_a, w);
    }
    else
    {
        for (int y = 0; y < h; y++)
        {
            ImU32* p = (ImU32*)(void*)dst_tex->GetPixelsAt(dst_x, dst_y + y);
            for (int x = w; x > 0; x--, p++)
                *p = col;
        }
    }
}

// Copy block from one texture to another
void ImFontAtlasTextureBlockCopy(ImTextureData* src_tex, int src_x, int src_y, ImTextureData* dst_tex, int dst_x, int dst_y, int w, int h)
{
    IM_ASSERT(src_tex->Pixels != NULL && dst_tex->Pixels != NULL);
    IM_ASSERT(src_tex->Format == dst_tex->Format);
    IM_ASSERT(src_x >= 0 && src_x + w <= src_tex->Width);
    IM_ASSERT(src_y >= 0 && src_y + h <= src_tex->Height);
    IM_ASSERT(dst_x >= 0 && dst_x + w <= dst_tex->Width);
    IM_ASSERT(dst_y >= 0 && dst_y + h <= dst_tex->Height);
    for (int y = 0; y < h; y++)
        memcpy(dst_tex->GetPixelsAt(dst_x, dst_y + y), src_tex->GetPixelsAt(src_x, src_y + y), w * dst_tex->BytesPerPixel);
}

// Queue texture block update for renderer backend
void ImFontAtlasTextureBlockQueueUpload(ImFontAtlas* atlas, ImTextureData* tex, int x, int y, int w, int h)
{
    IM_ASSERT(tex->Status != ImTextureStatus_WantDestroy && tex->Status != ImTextureStatus_Destroyed);
    IM_ASSERT(x >= 0 && x <= 0xFFFF && y >= 0 && y <= 0xFFFF && w >= 0 && x + w <= 0x10000 && h >= 0 && y + h <= 0x10000);
    IM_UNUSED(atlas);

    ImTextureRect req = { (unsigned short)x, (unsigned short)y, (unsigned short)w, (unsigned short)h };
    int new_x1 = ImMax(tex->UpdateRect.w == 0 ? 0 : tex->UpdateRect.x + tex->UpdateRect.w, req.x + req.w);
    int new_y1 = ImMax(tex->UpdateRect.h == 0 ? 0 : tex->UpdateRect.y + tex->UpdateRect.h, req.y + req.h);
    tex->UpdateRect.x = ImMin(tex->UpdateRect.x, req.x);
    tex->UpdateRect.y = ImMin(tex->UpdateRect.y, req.y);
    tex->UpdateRect.w = (unsigned short)(new_x1 - tex->UpdateRect.x);
    tex->UpdateRect.h = (unsigned short)(new_y1 - tex->UpdateRect.y);
    tex->UsedRect.x = ImMin(tex->UsedRect.x, req.x);
    tex->UsedRect.y = ImMin(tex->UsedRect.y, req.y);
    tex->UsedRect.w = (unsigned short)(ImMax(tex->UsedRect.x + tex->UsedRect.w, req.x + req.w) - tex->UsedRect.x);
    tex->UsedRect.h = (unsigned short)(ImMax(tex->UsedRect.y + tex->UsedRect.h, req.y + req.h) - tex->UsedRect.y);
    atlas->TexIsBuilt = false;

    // No need to queue if status is _WantCreate
    if (tex->Status == ImTextureStatus_OK || tex->Status == ImTextureStatus_WantUpdates)
    {
        tex->Status = ImTextureStatus_WantUpdates;
        tex->Updates.push_back(req);
    }
}

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
static void GetTexDataAsFormat(ImFontAtlas* atlas, ImTextureFormat format, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    ImTextureData* tex = atlas->TexData;
    if (!atlas->TexIsBuilt || tex == NULL || tex->Pixels == NULL || atlas->TexDesiredFormat != format)
    {
        atlas->TexDesiredFormat = format;
        atlas->Build();
        tex = atlas->TexData;
    }
    if (out_pixels) { *out_pixels = (unsigned char*)tex->Pixels; };
    if (out_width) { *out_width = tex->Width; };
    if (out_height) { *out_height = tex->Height; };
    if (out_bytes_per_pixel) { *out_bytes_per_pixel = tex->BytesPerPixel; }
}

void ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    GetTexDataAsFormat(this, ImTextureFormat_Alpha8, out_pixels, out_width, out_height, out_bytes_per_pixel);
}

void ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    GetTexDataAsFormat(this, ImTextureFormat_RGBA32, out_pixels, out_width, out_height, out_bytes_per_pixel);
}

bool ImFontAtlas::Build()
{
    ImFontAtlasBuildMain(this);
    return true;
}
#endif // #ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS

ImFont* ImFontAtlas::AddFont(const ImFontConfig* font_cfg_in)
{
    // Sanity Checks
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    IM_ASSERT((font_cfg_in->FontData != NULL && font_cfg_in->FontDataSize > 0) || (font_cfg_in->FontLoader != NULL));
    //IM_ASSERT(font_cfg_in->SizePixels > 0.0f && "Is ImFontConfig struct correctly initialized?");
    IM_ASSERT(font_cfg_in->RasterizerDensity > 0.0f && "Is ImFontConfig struct correctly initialized?");
    if (font_cfg_in->GlyphOffset.x != 0.0f || font_cfg_in->GlyphOffset.y != 0.0f || font_cfg_in->GlyphMinAdvanceX != 0.0f || font_cfg_in->GlyphMaxAdvanceX != FLT_MAX)
        IM_ASSERT(font_cfg_in->SizePixels != 0.0f && "Specifying glyph offset/advances requires a reference size to base it on.");

    // Lazily create builder on the first call to AddFont
    if (Builder == NULL)
        ImFontAtlasBuildInit(this);

    // Create new font
    ImFont* font;
    if (!font_cfg_in->MergeMode)
    {
        font = IM_NEW(ImFont)();
        font->FontId = FontNextUniqueID++;
        font->Flags = font_cfg_in->Flags;
        font->LegacySize = font_cfg_in->SizePixels;
        font->CurrentRasterizerDensity = font_cfg_in->RasterizerDensity;
        Fonts.push_back(font);
    }
    else
    {
        IM_ASSERT(Fonts.Size > 0 && "Cannot use MergeMode for the first font"); // When using MergeMode make sure that a font has already been added before. You can use ImGui::GetIO().Fonts->AddFontDefault() to add the default imgui font.
        font = Fonts.back();
    }

    // Add to list
    Sources.push_back(*font_cfg_in);
    ImFontConfig* font_cfg = &Sources.back();
    if (font_cfg->DstFont == NULL)
        font_cfg->DstFont = font;
    font->Sources.push_back(font_cfg);
    ImFontAtlasBuildUpdatePointers(this); // Pointers to Sources are otherwise dangling after we called Sources.push_back().

    if (font_cfg->FontDataOwnedByAtlas == false)
    {
        font_cfg->FontDataOwnedByAtlas = true;
        font_cfg->FontData = ImMemdup(font_cfg->FontData, (size_t)font_cfg->FontDataSize);
    }

    // Sanity check
    // We don't round cfg.SizePixels yet as relative size of merged fonts are used afterwards.
    if (font_cfg->GlyphExcludeRanges != NULL)
    {
        int size = 0;
        for (const ImWchar* p = font_cfg->GlyphExcludeRanges; p[0] != 0; p++, size++) {}
        IM_ASSERT((size & 1) == 0 && "GlyphExcludeRanges[] size must be multiple of two!");
        IM_ASSERT((size <= 64) && "GlyphExcludeRanges[] size must be small!");
        font_cfg->GlyphExcludeRanges = (ImWchar*)ImMemdup(font_cfg->GlyphExcludeRanges, sizeof(font_cfg->GlyphExcludeRanges[0]) * (size + 1));
    }
    if (font_cfg->FontLoader != NULL)
    {
        IM_ASSERT(font_cfg->FontLoader->FontBakedLoadGlyph != NULL);
        IM_ASSERT(font_cfg->FontLoader->LoaderInit == NULL && font_cfg->FontLoader->LoaderShutdown == NULL); // FIXME-NEWATLAS: Unsupported yet.
    }
    IM_ASSERT(font_cfg->FontLoaderData == NULL);

    if (!ImFontAtlasFontSourceInit(this, font_cfg))
    {
        // Rollback (this is a fragile/rarely exercised code-path. TestSuite's "misc_atlas_add_invalid_font" aim to test this)
        ImFontAtlasFontDestroySourceData(this, font_cfg);
        Sources.pop_back();
        font->Sources.pop_back();
        if (!font_cfg->MergeMode)
        {
            IM_DELETE(font);
            Fonts.pop_back();
        }
        return NULL;
    }
    ImFontAtlasFontSourceAddToFont(this, font, font_cfg);

    return font;
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
        font_cfg.SizePixels = 13.0f * 1.0f;
    if (font_cfg.Name[0] == '\0')
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "ProggyClean.ttf");
    font_cfg.EllipsisChar = (ImWchar)0x0085;
    font_cfg.GlyphOffset.y += 1.0f * IM_TRUNC(font_cfg.SizePixels / 13.0f);  // Add +1 offset per 13 units

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
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    size_t data_size = 0;
    void* data = ImFileLoadToMemory(filename, "rb", &data_size, 0);
    if (!data)
    {
        if (font_cfg_template == NULL || (font_cfg_template->Flags & ImFontFlags_NoLoadError) == 0)
        {
            IMGUI_DEBUG_LOG("While loading '%s'\n", filename);
            IM_ASSERT_USER_ERROR(0, "Could not load font file!");
        }
        return NULL;
    }
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (font_cfg.Name[0] == '\0')
    {
        // Store a short copy of filename into into the font name for convenience
        const char* p;
        for (p = filename + ImStrlen(filename); p > filename && p[-1] != '/' && p[-1] != '\\'; p--) {}
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "%s", p);
    }
    return AddFontFromMemoryTTF(data, (int)data_size, size_pixels, &font_cfg, glyph_ranges);
}

// NB: Transfer ownership of 'ttf_data' to ImFontAtlas, unless font_cfg_template->FontDataOwnedByAtlas == false. Owned TTF buffer will be deleted after Build().
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* font_data, int font_data_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
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

// On font removal we need to remove references (otherwise we could queue removal?)
// We allow old_font == new_font which forces updating all values (e.g. sizes)
static void ImFontAtlasBuildNotifySetFont(ImFontAtlas* atlas, ImFont* old_font, ImFont* new_font)
{
    for (ImDrawListSharedData* shared_data : atlas->DrawListSharedDatas)
    {
        if (shared_data->Font == old_font)
            shared_data->Font = new_font;
        if (ImGuiContext* ctx = shared_data->Context)
        {
            if (ctx->IO.FontDefault == old_font)
                ctx->IO.FontDefault = new_font;
            if (ctx->Font == old_font)
            {
                ImGuiContext* curr_ctx = ImGui::GetCurrentContext();
                bool need_bind_ctx = ctx != curr_ctx;
                if (need_bind_ctx)
                    ImGui::SetCurrentContext(ctx);
                ImGui::SetCurrentFont(new_font, ctx->FontSizeBase, ctx->FontSize);
                if (need_bind_ctx)
                    ImGui::SetCurrentContext(curr_ctx);
            }
            for (ImFontStackData& font_stack_data : ctx->FontStack)
                if (font_stack_data.Font == old_font)
                    font_stack_data.Font = new_font;
        }
    }
}

void ImFontAtlas::RemoveFont(ImFont* font)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    font->ClearOutputData();

    ImFontAtlasFontDestroyOutput(this, font);
    for (ImFontConfig* src : font->Sources)
        ImFontAtlasFontDestroySourceData(this, src);
    for (int src_n = 0; src_n < Sources.Size; src_n++)
        if (Sources[src_n].DstFont == font)
            Sources.erase(&Sources[src_n--]);

    bool removed = Fonts.find_erase(font);
    IM_ASSERT(removed);
    IM_UNUSED(removed);

    ImFontAtlasBuildUpdatePointers(this);

    font->ContainerAtlas = NULL;
    IM_DELETE(font);

    // Notify external systems
    ImFont* new_current_font = Fonts.empty() ? NULL : Fonts[0];
    ImFontAtlasBuildNotifySetFont(this, font, new_current_font);
}

// At it is common to do an AddCustomRect() followed by a GetCustomRect(), we provide an optional 'ImFontAtlasRect* out_r = NULL' argument to retrieve the info straight away.
ImFontAtlasRectId ImFontAtlas::AddCustomRect(int width, int height, ImFontAtlasRect* out_r)
{
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);

    if (Builder == NULL)
        ImFontAtlasBuildInit(this);

    ImFontAtlasRectId r_id = ImFontAtlasPackAddRect(this, width, height);
    if (r_id == ImFontAtlasRectId_Invalid)
        return ImFontAtlasRectId_Invalid;
    if (out_r != NULL)
        GetCustomRect(r_id, out_r);

    if (RendererHasTextures)
    {
        ImTextureRect* r = ImFontAtlasPackGetRect(this, r_id);
        ImFontAtlasTextureBlockQueueUpload(this, TexData, r->x, r->y, r->w, r->h);
    }
    return r_id;
}

void ImFontAtlas::RemoveCustomRect(ImFontAtlasRectId id)
{
    if (ImFontAtlasPackGetRectSafe(this, id) == NULL)
        return;
    ImFontAtlasPackDiscardRect(this, id);
}

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
// This API does not make sense anymore with scalable fonts.
// - Prefer adding a font source (ImFontConfig) using a custom/procedural loader.
// - You may use ImFontFlags_LockBakedSizes to limit an existing font to known baked sizes:
//     ImFont* myfont = io.Fonts->AddFontFromFileTTF(....);
//     myfont->GetFontBaked(16.0f);
//     myfont->Flags |= ImFontFlags_LockBakedSizes;
ImFontAtlasRectId ImFontAtlas::AddCustomRectFontGlyph(ImFont* font, ImWchar codepoint, int width, int height, float advance_x, const ImVec2& offset)
{
    float font_size = font->LegacySize;
    return AddCustomRectFontGlyphForSize(font, font_size, codepoint, width, height, advance_x, offset);
}
// FIXME: we automatically set glyph.Colored=true by default.
// If you need to alter this, you can write 'font->Glyphs.back()->Colored' after calling AddCustomRectFontGlyph().
ImFontAtlasRectId ImFontAtlas::AddCustomRectFontGlyphForSize(ImFont* font, float font_size, ImWchar codepoint, int width, int height, float advance_x, const ImVec2& offset)
{
#ifdef IMGUI_USE_WCHAR32
    IM_ASSERT(codepoint <= IM_UNICODE_CODEPOINT_MAX);
#endif
    IM_ASSERT(font != NULL);
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);

    ImFontBaked* baked = font->GetFontBaked(font_size);

    ImFontAtlasRectId r_id = ImFontAtlasPackAddRect(this, width, height);
    if (r_id == ImFontAtlasRectId_Invalid)
        return ImFontAtlasRectId_Invalid;
    ImTextureRect* r = ImFontAtlasPackGetRect(this, r_id);
    if (RendererHasTextures)
        ImFontAtlasTextureBlockQueueUpload(this, TexData, r->x, r->y, r->w, r->h);

    if (baked->IsGlyphLoaded(codepoint))
        ImFontAtlasBakedDiscardFontGlyph(this, font, baked, baked->FindGlyph(codepoint));

    ImFontGlyph glyph;
    glyph.Codepoint = codepoint;
    glyph.AdvanceX = advance_x;
    glyph.X0 = offset.x;
    glyph.Y0 = offset.y;
    glyph.X1 = offset.x + r->w;
    glyph.Y1 = offset.y + r->h;
    glyph.Visible = true;
    glyph.Colored = true; // FIXME: Arbitrary
    glyph.PackId = r_id;
    ImFontAtlasBakedAddFontGlyph(this, baked, font->Sources[0], &glyph);
    return r_id;
}
#endif // #ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS

bool ImFontAtlas::GetCustomRect(ImFontAtlasRectId id, ImFontAtlasRect* out_r) const
{
    ImTextureRect* r = ImFontAtlasPackGetRectSafe((ImFontAtlas*)this, id);
    if (r == NULL)
        return false;
    IM_ASSERT(TexData->Width > 0 && TexData->Height > 0);   // Font atlas needs to be built before we can calculate UV coordinates
    if (out_r == NULL)
        return true;
    out_r->x = r->x;
    out_r->y = r->y;
    out_r->w = r->w;
    out_r->h = r->h;
    out_r->uv0 = ImVec2((float)(r->x), (float)(r->y)) * TexUvScale;
    out_r->uv1 = ImVec2((float)(r->x + r->w), (float)(r->y + r->h)) * TexUvScale;
    return true;
}

bool ImFontAtlasGetMouseCursorTexData(ImFontAtlas* atlas, ImGuiMouseCursor cursor_type, ImVec2* out_offset, ImVec2* out_size, ImVec2 out_uv_border[2], ImVec2 out_uv_fill[2])
{
    if (cursor_type <= ImGuiMouseCursor_None || cursor_type >= ImGuiMouseCursor_COUNT)
        return false;
    if (atlas->Flags & ImFontAtlasFlags_NoMouseCursors)
        return false;

    ImTextureRect* r = ImFontAtlasPackGetRect(atlas, atlas->Builder->PackIdMouseCursors);
    ImVec2 pos = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][0] + ImVec2((float)r->x, (float)r->y);
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

// When atlas->RendererHasTextures = true, this is only called if no font were loaded.
void ImFontAtlasBuildMain(ImFontAtlas* atlas)
{
    IM_ASSERT(!atlas->Locked && "Cannot modify a locked ImFontAtlas!");
    if (atlas->TexData && atlas->TexData->Format != atlas->TexDesiredFormat)
    {
        ImVec2i new_tex_size = ImFontAtlasTextureGetSizeEstimate(atlas);
        ImFontAtlasBuildDestroy(atlas);
        ImFontAtlasTextureAdd(atlas, new_tex_size.x, new_tex_size.y);
    }

    if (atlas->Builder == NULL)
        ImFontAtlasBuildInit(atlas);

    // Default font is none are specified
    if (atlas->Sources.Size == 0)
        atlas->AddFontDefault();

    // [LEGACY] For backends not supporting RendererHasTextures: preload all glyphs
    ImFontAtlasBuildUpdateRendererHasTexturesFromContext(atlas);
    if (atlas->RendererHasTextures == false) // ~ImGuiBackendFlags_RendererHasTextures
        ImFontAtlasBuildLegacyPreloadAllGlyphRanges(atlas);
    atlas->TexIsBuilt = true;
}

void ImFontAtlasBuildGetOversampleFactors(ImFontConfig* src, ImFontBaked* baked, int* out_oversample_h, int* out_oversample_v)
{
    // Automatically disable horizontal oversampling over size 36
    const float raster_size = baked->Size * baked->RasterizerDensity * src->RasterizerDensity;
    *out_oversample_h = (src->OversampleH != 0) ? src->OversampleH : (raster_size > 36.0f || src->PixelSnapH) ? 1 : 2;
    *out_oversample_v = (src->OversampleV != 0) ? src->OversampleV : 1;
}

// Setup main font loader for the atlas
// Every font source (ImFontConfig) will use this unless ImFontConfig::FontLoader specify a custom loader.
void ImFontAtlasBuildSetupFontLoader(ImFontAtlas* atlas, const ImFontLoader* font_loader)
{
    if (atlas->FontLoader == font_loader)
        return;
    IM_ASSERT(!atlas->Locked && "Cannot modify a locked ImFontAtlas!");

    for (ImFont* font : atlas->Fonts)
        ImFontAtlasFontDestroyOutput(atlas, font);
    if (atlas->Builder && atlas->FontLoader && atlas->FontLoader->LoaderShutdown)
        atlas->FontLoader->LoaderShutdown(atlas);

    atlas->FontLoader = font_loader;
    atlas->FontLoaderName = font_loader ? font_loader->Name : "NULL";
    IM_ASSERT(atlas->FontLoaderData == NULL);

    if (atlas->Builder && atlas->FontLoader && atlas->FontLoader->LoaderInit)
        atlas->FontLoader->LoaderInit(atlas);
    for (ImFont* font : atlas->Fonts)
        ImFontAtlasFontInitOutput(atlas, font);
    for (ImFont* font : atlas->Fonts)
        for (ImFontConfig* src : font->Sources)
            ImFontAtlasFontSourceAddToFont(atlas, font, src);
}

// Preload all glyph ranges for legacy backends.
// This may lead to multiple texture creation which might be a little slower than before.
void ImFontAtlasBuildLegacyPreloadAllGlyphRanges(ImFontAtlas* atlas)
{
    atlas->Builder->PreloadedAllGlyphsRanges = true;
    for (ImFont* font : atlas->Fonts)
    {
        ImFontBaked* baked = font->GetFontBaked(font->LegacySize);
        if (font->FallbackChar != 0)
            baked->FindGlyph(font->FallbackChar);
        if (font->EllipsisChar != 0)
            baked->FindGlyph(font->EllipsisChar);
        for (ImFontConfig* src : font->Sources)
        {
            const ImWchar* ranges = src->GlyphRanges ? src->GlyphRanges : atlas->GetGlyphRangesDefault();
            for (; ranges[0]; ranges += 2)
                for (unsigned int c = ranges[0]; c <= ranges[1] && c <= IM_UNICODE_CODEPOINT_MAX; c++) //-V560
                    baked->FindGlyph((ImWchar)c);
        }
    }
}

// FIXME: May make ImFont::Sources a ImSpan<> and move ownership to ImFontAtlas
void ImFontAtlasBuildUpdatePointers(ImFontAtlas* atlas)
{
    for (ImFont* font : atlas->Fonts)
        font->Sources.resize(0);
    for (ImFontConfig& src : atlas->Sources)
        src.DstFont->Sources.push_back(&src);
}

// Render a white-colored bitmap encoded in a string
void ImFontAtlasBuildRenderBitmapFromString(ImFontAtlas* atlas, int x, int y, int w, int h, const char* in_str, char in_marker_char)
{
    ImTextureData* tex = atlas->TexData;
    IM_ASSERT(x >= 0 && x + w <= tex->Width);
    IM_ASSERT(y >= 0 && y + h <= tex->Height);

    switch (tex->Format)
    {
    case ImTextureFormat_Alpha8:
    {
        ImU8* out_p = (ImU8*)tex->GetPixelsAt(x, y);
        for (int off_y = 0; off_y < h; off_y++, out_p += tex->Width, in_str += w)
            for (int off_x = 0; off_x < w; off_x++)
                out_p[off_x] = (in_str[off_x] == in_marker_char) ? 0xFF : 0x00;
        break;
    }
    case ImTextureFormat_RGBA32:
    {
        ImU32* out_p = (ImU32*)tex->GetPixelsAt(x, y);
        for (int off_y = 0; off_y < h; off_y++, out_p += tex->Width, in_str += w)
            for (int off_x = 0; off_x < w; off_x++)
                out_p[off_x] = (in_str[off_x] == in_marker_char) ? IM_COL32_WHITE : IM_COL32_BLACK_TRANS;
        break;
    }
    }
}

static void ImFontAtlasBuildUpdateBasicTexData(ImFontAtlas* atlas)
{
    // Pack and store identifier so we can refresh UV coordinates on texture resize.
    // FIXME-NEWATLAS: User/custom rects where user code wants to store UV coordinates will need to do the same thing.
    ImFontAtlasBuilder* builder = atlas->Builder;
    ImVec2i pack_size = (atlas->Flags & ImFontAtlasFlags_NoMouseCursors) ? ImVec2i(2, 2) : ImVec2i(FONT_ATLAS_DEFAULT_TEX_DATA_W * 2 + 1, FONT_ATLAS_DEFAULT_TEX_DATA_H);

    ImFontAtlasRect r;
    bool add_and_draw = (atlas->GetCustomRect(builder->PackIdMouseCursors, &r) == false);
    if (add_and_draw)
    {
        builder->PackIdMouseCursors = atlas->AddCustomRect(pack_size.x, pack_size.y, &r);
        IM_ASSERT(builder->PackIdMouseCursors != ImFontAtlasRectId_Invalid);

        // Draw to texture
        if (atlas->Flags & ImFontAtlasFlags_NoMouseCursors)
        {
            // 2x2 white pixels
            ImFontAtlasBuildRenderBitmapFromString(atlas, r.x, r.y, 2, 2, "XX" "XX", 'X');
        }
        else
        {
            // 2x2 white pixels + mouse cursors
            const int x_for_white = r.x;
            const int x_for_black = r.x + FONT_ATLAS_DEFAULT_TEX_DATA_W + 1;
            ImFontAtlasBuildRenderBitmapFromString(atlas, x_for_white, r.y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, '.');
            ImFontAtlasBuildRenderBitmapFromString(atlas, x_for_black, r.y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, 'X');
        }
    }

    // Refresh UV coordinates
    atlas->TexUvWhitePixel = ImVec2((r.x + 0.5f) * atlas->TexUvScale.x, (r.y + 0.5f) * atlas->TexUvScale.y);
}

static void ImFontAtlasBuildUpdateLinesTexData(ImFontAtlas* atlas)
{
    if (atlas->Flags & ImFontAtlasFlags_NoBakedLines)
        return;

    // Pack and store identifier so we can refresh UV coordinates on texture resize.
    ImTextureData* tex = atlas->TexData;
    ImFontAtlasBuilder* builder = atlas->Builder;

    ImFontAtlasRect r;
    bool add_and_draw = atlas->GetCustomRect(builder->PackIdLinesTexData, &r) == false;
    if (add_and_draw)
    {
        ImVec2i pack_size = ImVec2i(IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 2, IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 1);
        builder->PackIdLinesTexData = atlas->AddCustomRect(pack_size.x, pack_size.y, &r);
        IM_ASSERT(builder->PackIdLinesTexData != ImFontAtlasRectId_Invalid);
    }

    // Register texture region for thick lines
    // The +2 here is to give space for the end caps, whilst height +1 is to accommodate the fact we have a zero-width row
    // This generates a triangular shape in the texture, with the various line widths stacked on top of each other to allow interpolation between them
    for (int n = 0; n < IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 1; n++) // +1 because of the zero-width row
    {
        // Each line consists of at least two empty pixels at the ends, with a line of solid pixels in the middle
        const int y = n;
        const int line_width = n;
        const int pad_left = (r.w - line_width) / 2;
        const int pad_right = r.w - (pad_left + line_width);
        IM_ASSERT(pad_left + line_width + pad_right == r.w && y < r.h); // Make sure we're inside the texture bounds before we start writing pixels

        // Write each slice
        if (add_and_draw && tex->Format == ImTextureFormat_Alpha8)
        {
            ImU8* write_ptr = (ImU8*)tex->GetPixelsAt(r.x, r.y + y);
            for (int i = 0; i < pad_left; i++)
                *(write_ptr + i) = 0x00;

            for (int i = 0; i < line_width; i++)
                *(write_ptr + pad_left + i) = 0xFF;

            for (int i = 0; i < pad_right; i++)
                *(write_ptr + pad_left + line_width + i) = 0x00;
        }
        else if (add_and_draw && tex->Format == ImTextureFormat_RGBA32)
        {
            ImU32* write_ptr = (ImU32*)(void*)tex->GetPixelsAt(r.x, r.y + y);
            for (int i = 0; i < pad_left; i++)
                *(write_ptr + i) = IM_COL32(255, 255, 255, 0);

            for (int i = 0; i < line_width; i++)
                *(write_ptr + pad_left + i) = IM_COL32_WHITE;

            for (int i = 0; i < pad_right; i++)
                *(write_ptr + pad_left + line_width + i) = IM_COL32(255, 255, 255, 0);
        }

        // Refresh UV coordinates
        ImVec2 uv0 = ImVec2((float)(r.x + pad_left - 1), (float)(r.y + y)) * atlas->TexUvScale;
        ImVec2 uv1 = ImVec2((float)(r.x + pad_left + line_width + 1), (float)(r.y + y + 1)) * atlas->TexUvScale;
        float half_v = (uv0.y + uv1.y) * 0.5f; // Calculate a constant V in the middle of the row to avoid sampling artifacts
        atlas->TexUvLines[n] = ImVec4(uv0.x, half_v, uv1.x, half_v);
    }
}

//-----------------------------------------------------------------------------------------------------------------------------

// Was tempted to lazily init FontSrc but wouldn't save much + makes it more complicated to detect invalid data at AddFont()
bool ImFontAtlasFontInitOutput(ImFontAtlas* atlas, ImFont* font)
{
    bool ret = true;
    for (ImFontConfig* src : font->Sources)
        if (!ImFontAtlasFontSourceInit(atlas, src))
            ret = false;
    IM_ASSERT(ret); // Unclear how to react to this meaningfully. Assume that result will be same as initial AddFont() call.
    return ret;
}

// Keep source/input FontData
void ImFontAtlasFontDestroyOutput(ImFontAtlas* atlas, ImFont* font)
{
    font->ClearOutputData();
    for (ImFontConfig* src : font->Sources)
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        if (loader && loader->FontSrcDestroy != NULL)
            loader->FontSrcDestroy(atlas, src);
    }
}

//-----------------------------------------------------------------------------------------------------------------------------

bool ImFontAtlasFontSourceInit(ImFontAtlas* atlas, ImFontConfig* src)
{
    const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
    if (loader->FontSrcInit != NULL && !loader->FontSrcInit(atlas, src))
        return false;
    return true;
}

void ImFontAtlasFontSourceAddToFont(ImFontAtlas* atlas, ImFont* font, ImFontConfig* src)
{
    if (src->MergeMode == false)
    {
        font->ClearOutputData();
        //font->FontSize = src->SizePixels;
        font->ContainerAtlas = atlas;
        IM_ASSERT(font->Sources[0] == src);
    }
    atlas->TexIsBuilt = false; // For legacy backends
    ImFontAtlasBuildSetupFontSpecialGlyphs(atlas, font, src);
}

void ImFontAtlasFontDestroySourceData(ImFontAtlas* atlas, ImFontConfig* src)
{
    IM_UNUSED(atlas);
    if (src->FontDataOwnedByAtlas)
        IM_FREE(src->FontData);
    src->FontData = NULL;
    if (src->GlyphExcludeRanges)
        IM_FREE((void*)src->GlyphExcludeRanges);
    src->GlyphExcludeRanges = NULL;
}

// Create a compact, baked "..." if it doesn't exist, by using the ".".
// This may seem overly complicated right now but the point is to exercise and improve a technique which should be increasingly used.
// FIXME-NEWATLAS: This borrows too much from FontLoader's FontLoadGlyph() handlers and suggest that we should add further helpers.
static ImFontGlyph* ImFontAtlasBuildSetupFontBakedEllipsis(ImFontAtlas* atlas, ImFontBaked* baked)
{
    ImFont* font = baked->ContainerFont;
    IM_ASSERT(font->EllipsisChar != 0);

    const ImFontGlyph* dot_glyph = baked->FindGlyphNoFallback((ImWchar)'.');
    if (dot_glyph == NULL)
        dot_glyph = baked->FindGlyphNoFallback((ImWchar)0xFF0E);
    if (dot_glyph == NULL)
        return NULL;
    ImFontAtlasRectId dot_r_id = dot_glyph->PackId; // Deep copy to avoid invalidation of glyphs and rect pointers
    ImTextureRect* dot_r = ImFontAtlasPackGetRect(atlas, dot_r_id);
    const int dot_spacing = 1;
    const float dot_step = (dot_glyph->X1 - dot_glyph->X0) + dot_spacing;

    ImFontAtlasRectId pack_id = ImFontAtlasPackAddRect(atlas, (dot_r->w * 3 + dot_spacing * 2), dot_r->h);
    ImTextureRect* r = ImFontAtlasPackGetRect(atlas, pack_id);

    ImFontGlyph glyph_in = {};
    ImFontGlyph* glyph = &glyph_in;
    glyph->Codepoint = font->EllipsisChar;
    glyph->AdvanceX = ImMax(dot_glyph->AdvanceX, dot_glyph->X0 + dot_step * 3.0f - dot_spacing); // FIXME: Slightly odd for normally mono-space fonts but since this is used for trailing contents.
    glyph->X0 = dot_glyph->X0;
    glyph->Y0 = dot_glyph->Y0;
    glyph->X1 = dot_glyph->X0 + dot_step * 3 - dot_spacing;
    glyph->Y1 = dot_glyph->Y1;
    glyph->Visible = true;
    glyph->PackId = pack_id;
    glyph = ImFontAtlasBakedAddFontGlyph(atlas, baked, NULL, glyph);
    dot_glyph = NULL; // Invalidated

    // Copy to texture, post-process and queue update for backend
    // FIXME-NEWATLAS-V2: Dot glyph is already post-processed as this point, so this would damage it.
    dot_r = ImFontAtlasPackGetRect(atlas, dot_r_id);
    ImTextureData* tex = atlas->TexData;
    for (int n = 0; n < 3; n++)
        ImFontAtlasTextureBlockCopy(tex, dot_r->x, dot_r->y, tex, r->x + (dot_r->w + dot_spacing) * n, r->y, dot_r->w, dot_r->h);
    ImFontAtlasTextureBlockQueueUpload(atlas, tex, r->x, r->y, r->w, r->h);

    return glyph;
}

// Load fallback in order to obtain its index
// (this is called from in hot-path so we avoid extraneous parameters to minimize impact on code size)
static void ImFontAtlasBuildSetupFontBakedFallback(ImFontBaked* baked)
{
    IM_ASSERT(baked->FallbackGlyphIndex == -1);
    IM_ASSERT(baked->FallbackAdvanceX == 0.0f);
    ImFont* font = baked->ContainerFont;
    ImFontGlyph* fallback_glyph = NULL;
    if (font->FallbackChar != 0)
        fallback_glyph = baked->FindGlyphNoFallback(font->FallbackChar);
    if (fallback_glyph == NULL)
    {
        ImFontGlyph* space_glyph = baked->FindGlyphNoFallback((ImWchar)' ');
        ImFontGlyph glyph;
        glyph.Codepoint = 0;
        glyph.AdvanceX = space_glyph ? space_glyph->AdvanceX : IM_ROUND(baked->Size * 0.40f);
        fallback_glyph = ImFontAtlasBakedAddFontGlyph(font->ContainerAtlas, baked, NULL, &glyph);
    }
    baked->FallbackGlyphIndex = baked->Glyphs.index_from_ptr(fallback_glyph); // Storing index avoid need to update pointer on growth and simplify inner loop code
    baked->FallbackAdvanceX = fallback_glyph->AdvanceX;
}

static void ImFontAtlasBuildSetupFontBakedBlanks(ImFontAtlas* atlas, ImFontBaked* baked)
{
    // Mark space as always hidden (not strictly correct/necessary. but some e.g. icons fonts don't have a space. it tends to look neater in previews)
    ImFontGlyph* space_glyph = baked->FindGlyphNoFallback((ImWchar)' ');
    if (space_glyph != NULL)
        space_glyph->Visible = false;

    // Setup Tab character.
    // FIXME: Needs proper TAB handling but it needs to be contextualized (or we could arbitrary say that each string starts at "column 0" ?)
    if (baked->FindGlyphNoFallback('\t') == NULL && space_glyph != NULL)
    {
        ImFontGlyph tab_glyph;
        tab_glyph.Codepoint = '\t';
        tab_glyph.AdvanceX = space_glyph->AdvanceX * IM_TABSIZE;
        ImFontAtlasBakedAddFontGlyph(atlas, baked, NULL, &tab_glyph);
    }
}

// Load/identify special glyphs
// (note that this is called again for fonts with MergeMode)
void ImFontAtlasBuildSetupFontSpecialGlyphs(ImFontAtlas* atlas, ImFont* font, ImFontConfig* src)
{
    IM_UNUSED(atlas);
    IM_ASSERT(font->Sources.contains(src));

    // Find Fallback character. Actual glyph loaded in GetFontBaked().
    const ImWchar fallback_chars[] = { font->FallbackChar, (ImWchar)IM_UNICODE_CODEPOINT_INVALID, (ImWchar)'?', (ImWchar)' ' };
    if (font->FallbackChar == 0)
        for (ImWchar candidate_char : fallback_chars)
            if (candidate_char != 0 && font->IsGlyphInFont(candidate_char))
            {
                font->FallbackChar = (ImWchar)candidate_char;
                break;
            }

    // Setup Ellipsis character. It is required for rendering elided text. We prefer using U+2026 (horizontal ellipsis).
    // However some old fonts may contain ellipsis at U+0085. Here we auto-detect most suitable ellipsis character.
    // FIXME: Note that 0x2026 is rarely included in our font ranges. Because of this we are more likely to use three individual dots.
    const ImWchar ellipsis_chars[] = { src->EllipsisChar, (ImWchar)0x2026, (ImWchar)0x0085 };
    if (font->EllipsisChar == 0)
        for (ImWchar candidate_char : ellipsis_chars)
            if (candidate_char != 0 && font->IsGlyphInFont(candidate_char))
            {
                font->EllipsisChar = candidate_char;
                break;
            }
    if (font->EllipsisChar == 0)
    {
        font->EllipsisChar = 0x0085;
        font->EllipsisAutoBake = true;
    }
}

void ImFontAtlasBakedDiscardFontGlyph(ImFontAtlas* atlas, ImFont* font, ImFontBaked* baked, ImFontGlyph* glyph)
{
    if (glyph->PackId != ImFontAtlasRectId_Invalid)
    {
        ImFontAtlasPackDiscardRect(atlas, glyph->PackId);
        glyph->PackId = ImFontAtlasRectId_Invalid;
    }
    ImWchar c = (ImWchar)glyph->Codepoint;
    IM_ASSERT(font->FallbackChar != c && font->EllipsisChar != c); // Unsupported for simplicity
    IM_ASSERT(glyph >= baked->Glyphs.Data && glyph < baked->Glyphs.Data + baked->Glyphs.Size);
    IM_UNUSED(font);
    baked->IndexLookup[c] = IM_FONTGLYPH_INDEX_UNUSED;
    baked->IndexAdvanceX[c] = baked->FallbackAdvanceX;
}

ImFontBaked* ImFontAtlasBakedAdd(ImFontAtlas* atlas, ImFont* font, float font_size, float font_rasterizer_density, ImGuiID baked_id)
{
    IMGUI_DEBUG_LOG_FONT("[font] Created baked %.2fpx\n", font_size);
    ImFontBaked* baked = atlas->Builder->BakedPool.push_back(ImFontBaked());
    baked->Size = font_size;
    baked->RasterizerDensity = font_rasterizer_density;
    baked->BakedId = baked_id;
    baked->ContainerFont = font;
    baked->LastUsedFrame = atlas->Builder->FrameCount;

    // Initialize backend data
    size_t loader_data_size = 0;
    for (ImFontConfig* src : font->Sources) // Cannot easily be cached as we allow changing backend
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        loader_data_size += loader->FontBakedSrcLoaderDataSize;
    }
    baked->FontLoaderDatas = (loader_data_size > 0) ? IM_ALLOC(loader_data_size) : NULL;
    char* loader_data_p = (char*)baked->FontLoaderDatas;
    for (ImFontConfig* src : font->Sources)
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        if (loader->FontBakedInit)
            loader->FontBakedInit(atlas, src, baked, loader_data_p);
        loader_data_p += loader->FontBakedSrcLoaderDataSize;
    }

    ImFontAtlasBuildSetupFontBakedBlanks(atlas, baked);
    return baked;
}

// FIXME-OPT: This is not a fast query. Adding a BakedCount field in Font might allow to take a shortcut for the most common case.
ImFontBaked* ImFontAtlasBakedGetClosestMatch(ImFontAtlas* atlas, ImFont* font, float font_size, float font_rasterizer_density)
{
    ImFontAtlasBuilder* builder = atlas->Builder;
    for (int step_n = 0; step_n < 2; step_n++)
    {
        ImFontBaked* closest_larger_match = NULL;
        ImFontBaked* closest_smaller_match = NULL;
        for (int baked_n = 0; baked_n < builder->BakedPool.Size; baked_n++)
        {
            ImFontBaked* baked = &builder->BakedPool[baked_n];
            if (baked->ContainerFont != font || baked->WantDestroy)
                continue;
            if (step_n == 0 && baked->RasterizerDensity != font_rasterizer_density) // First try with same density
                continue;
            if (baked->Size > font_size && (closest_larger_match == NULL || baked->Size < closest_larger_match->Size))
                closest_larger_match = baked;
            if (baked->Size < font_size && (closest_smaller_match == NULL || baked->Size > closest_smaller_match->Size))
                closest_smaller_match = baked;
        }
        if (closest_larger_match)
            if (closest_smaller_match == NULL || (closest_larger_match->Size >= font_size * 2.0f && closest_smaller_match->Size > font_size * 0.5f))
                return closest_larger_match;
        if (closest_smaller_match)
            return closest_smaller_match;
    }
    return NULL;
}

void ImFontAtlasBakedDiscard(ImFontAtlas* atlas, ImFont* font, ImFontBaked* baked)
{
    ImFontAtlasBuilder* builder = atlas->Builder;
    IMGUI_DEBUG_LOG_FONT("[font] Discard baked %.2f for \"%s\"\n", baked->Size, font->GetDebugName());

    for (ImFontGlyph& glyph : baked->Glyphs)
        if (glyph.PackId != ImFontAtlasRectId_Invalid)
            ImFontAtlasPackDiscardRect(atlas, glyph.PackId);

    char* loader_data_p = (char*)baked->FontLoaderDatas;
    for (ImFontConfig* src : font->Sources)
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        if (loader->FontBakedDestroy)
            loader->FontBakedDestroy(atlas, src, baked, loader_data_p);
        loader_data_p += loader->FontBakedSrcLoaderDataSize;
    }
    if (baked->FontLoaderDatas)
    {
        IM_FREE(baked->FontLoaderDatas);
        baked->FontLoaderDatas = NULL;
    }
    builder->BakedMap.SetVoidPtr(baked->BakedId, NULL);
    builder->BakedDiscardedCount++;
    baked->ClearOutputData();
    baked->WantDestroy = true;
    font->LastBaked = NULL;
}

// use unused_frames==0 to discard everything.
void ImFontAtlasFontDiscardBakes(ImFontAtlas* atlas, ImFont* font, int unused_frames)
{
    if (ImFontAtlasBuilder* builder = atlas->Builder) // This can be called from font destructor
        for (int baked_n = 0; baked_n < builder->BakedPool.Size; baked_n++)
        {
            ImFontBaked* baked = &builder->BakedPool[baked_n];
            if (baked->LastUsedFrame + unused_frames > atlas->Builder->FrameCount)
                continue;
            if (baked->ContainerFont != font || baked->WantDestroy)
                continue;
            ImFontAtlasBakedDiscard(atlas, font, baked);
        }
}

// use unused_frames==0 to discard everything.
void ImFontAtlasBuildDiscardBakes(ImFontAtlas* atlas, int unused_frames)
{
    ImFontAtlasBuilder* builder = atlas->Builder;
    for (int baked_n = 0; baked_n < builder->BakedPool.Size; baked_n++)
    {
        ImFontBaked* baked = &builder->BakedPool[baked_n];
        if (baked->LastUsedFrame + unused_frames > atlas->Builder->FrameCount)
            continue;
        if (baked->WantDestroy || (baked->ContainerFont->Flags & ImFontFlags_LockBakedSizes))
            continue;
        ImFontAtlasBakedDiscard(atlas, baked->ContainerFont, baked);
    }
}

// Those functions are designed to facilitate changing the underlying structures for ImFontAtlas to store an array of ImDrawListSharedData*
void ImFontAtlasAddDrawListSharedData(ImFontAtlas* atlas, ImDrawListSharedData* data)
{
    IM_ASSERT(!atlas->DrawListSharedDatas.contains(data));
    atlas->DrawListSharedDatas.push_back(data);
}

void ImFontAtlasRemoveDrawListSharedData(ImFontAtlas* atlas, ImDrawListSharedData* data)
{
    IM_ASSERT(atlas->DrawListSharedDatas.contains(data));
    atlas->DrawListSharedDatas.find_erase(data);
}

// Update texture identifier in all active draw lists
void ImFontAtlasUpdateDrawListsTextures(ImFontAtlas* atlas, ImTextureRef old_tex, ImTextureRef new_tex)
{
    for (ImDrawListSharedData* shared_data : atlas->DrawListSharedDatas)
        for (ImDrawList* draw_list : shared_data->DrawLists)
        {
            // Replace in command-buffer
            // (there is not need to replace in ImDrawListSplitter: current channel is in ImDrawList's CmdBuffer[],
            //  other channels will be on SetCurrentChannel() which already needs to compare CmdHeader anyhow)
            if (draw_list->CmdBuffer.Size > 0 && draw_list->_CmdHeader.TexRef == old_tex)
                draw_list->_SetTexture(new_tex);

            // Replace in stack
            for (ImTextureRef& stacked_tex : draw_list->_TextureStack)
                if (stacked_tex == old_tex)
                    stacked_tex = new_tex;
        }
}

// Update texture coordinates in all draw list shared context
// FIXME-NEWATLAS FIXME-OPT: Doesn't seem necessary to update for all, only one bound to current context?
void ImFontAtlasUpdateDrawListsSharedData(ImFontAtlas* atlas)
{
    for (ImDrawListSharedData* shared_data : atlas->DrawListSharedDatas)
        if (shared_data->FontAtlas == atlas)
        {
            shared_data->TexUvWhitePixel = atlas->TexUvWhitePixel;
            shared_data->TexUvLines = atlas->TexUvLines;
        }
}

// Set current texture. This is mostly called from AddTexture() + to handle a failed resize.
static void ImFontAtlasBuildSetTexture(ImFontAtlas* atlas, ImTextureData* tex)
{
    ImTextureRef old_tex_ref = atlas->TexRef;
    atlas->TexData = tex;
    atlas->TexUvScale = ImVec2(1.0f / tex->Width, 1.0f / tex->Height);
    atlas->TexRef._TexData = tex;
    //atlas->TexRef._TexID = tex->TexID; // <-- We intentionally don't do that. It would be misleading and betray promise that both fields aren't set.
    ImFontAtlasUpdateDrawListsTextures(atlas, old_tex_ref, atlas->TexRef);
}

// Create a new texture, discard previous one
ImTextureData* ImFontAtlasTextureAdd(ImFontAtlas* atlas, int w, int h)
{
    ImTextureData* old_tex = atlas->TexData;
    ImTextureData* new_tex;

    // FIXME: Cannot reuse texture because old UV may have been used already (unless we remap UV).
    /*if (old_tex != NULL && old_tex->Status == ImTextureStatus_WantCreate)
    {
        // Reuse texture not yet used by backend.
        IM_ASSERT(old_tex->TexID == ImTextureID_Invalid && old_tex->BackendUserData == NULL);
        old_tex->DestroyPixels();
        old_tex->Updates.clear();
        new_tex = old_tex;
        old_tex = NULL;
    }
    else*/
    {
        // Add new
        new_tex = IM_NEW(ImTextureData)();
        new_tex->UniqueID = atlas->TexNextUniqueID++;
        atlas->TexList.push_back(new_tex);
    }
    if (old_tex != NULL)
    {
        // Queue old as to destroy next frame
        old_tex->WantDestroyNextFrame = true;
        IM_ASSERT(old_tex->Status == ImTextureStatus_OK || old_tex->Status == ImTextureStatus_WantCreate || old_tex->Status == ImTextureStatus_WantUpdates);
    }

    new_tex->Create(atlas->TexDesiredFormat, w, h);
    new_tex->Status = ImTextureStatus_WantCreate;
    atlas->TexIsBuilt = false;

    ImFontAtlasBuildSetTexture(atlas, new_tex);

    return new_tex;
}

#if 0
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"
static void ImFontAtlasDebugWriteTexToDisk(ImTextureData* tex, const char* description)
{
    ImGuiContext& g = *GImGui;
    char buf[128];
    ImFormatString(buf, IM_ARRAYSIZE(buf), "[%05d] Texture #%03d - %s.png", g.FrameCount, tex->UniqueID, description);
    stbi_write_png(buf, tex->Width, tex->Height, tex->BytesPerPixel, tex->Pixels, tex->GetPitch()); // tex->BytesPerPixel is technically not component, but ok for the formats we support.
}
#endif

void ImFontAtlasTextureRepack(ImFontAtlas* atlas, int w, int h)
{
    ImFontAtlasBuilder* builder = atlas->Builder;
    builder->LockDisableResize = true;

    ImTextureData* old_tex = atlas->TexData;
    ImTextureData* new_tex = ImFontAtlasTextureAdd(atlas, w, h);
    new_tex->UseColors = old_tex->UseColors;
    IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: resize+repack %dx%d => Texture #%03d: %dx%d\n", old_tex->UniqueID, old_tex->Width, old_tex->Height, new_tex->UniqueID, new_tex->Width, new_tex->Height);
    //for (int baked_n = 0; baked_n < builder->BakedPool.Size; baked_n++)
    //    IMGUI_DEBUG_LOG_FONT("[font] - Baked %.2fpx, %d glyphs, want_destroy=%d\n", builder->BakedPool[baked_n].FontSize, builder->BakedPool[baked_n].Glyphs.Size, builder->BakedPool[baked_n].WantDestroy);
    //IMGUI_DEBUG_LOG_FONT("[font] - Old packed rects: %d, area %d px\n", builder->RectsPackedCount, builder->RectsPackedSurface);
    //ImFontAtlasDebugWriteTexToDisk(old_tex, "Before Pack");

    // Repack, lose discarded rectangle, copy pixels
    // FIXME-NEWATLAS: This is unstable because packing order is based on RectsIndex
    // FIXME-NEWATLAS-V2: Repacking in batch would be beneficial to packing heuristic, and fix stability.
    // FIXME-NEWATLAS-TESTS: Test calling RepackTexture with size too small to fits existing rects.
    ImFontAtlasPackInit(atlas);
    ImVector<ImTextureRect> old_rects;
    ImVector<ImFontAtlasRectEntry> old_index = builder->RectsIndex;
    old_rects.swap(builder->Rects);

    for (ImFontAtlasRectEntry& index_entry : builder->RectsIndex)
    {
        if (index_entry.IsUsed == false)
            continue;
        ImTextureRect& old_r = old_rects[index_entry.TargetIndex];
        if (old_r.w == 0 && old_r.h == 0)
            continue;
        ImFontAtlasRectId new_r_id = ImFontAtlasPackAddRect(atlas, old_r.w, old_r.h, &index_entry);
        if (new_r_id == ImFontAtlasRectId_Invalid)
        {
            // Undo, grow texture and try repacking again.
            // FIXME-NEWATLAS-TESTS: This is a very rarely exercised path! It needs to be automatically tested properly.
            IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: resize failed. Will grow.\n", new_tex->UniqueID);
            new_tex->WantDestroyNextFrame = true;
            builder->Rects.swap(old_rects);
            builder->RectsIndex = old_index;
            ImFontAtlasBuildSetTexture(atlas, old_tex);
            ImFontAtlasTextureGrow(atlas, w, h); // Recurse
            return;
        }
        IM_ASSERT(ImFontAtlasRectId_GetIndex(new_r_id) == builder->RectsIndex.index_from_ptr(&index_entry));
        ImTextureRect* new_r = ImFontAtlasPackGetRect(atlas, new_r_id);
        ImFontAtlasTextureBlockCopy(old_tex, old_r.x, old_r.y, new_tex, new_r->x, new_r->y, new_r->w, new_r->h);
    }
    IM_ASSERT(old_rects.Size == builder->Rects.Size + builder->RectsDiscardedCount);
    builder->RectsDiscardedCount = 0;
    builder->RectsDiscardedSurface = 0;

    // Patch glyphs UV
    for (int baked_n = 0; baked_n < builder->BakedPool.Size; baked_n++)
        for (ImFontGlyph& glyph : builder->BakedPool[baked_n].Glyphs)
            if (glyph.PackId != ImFontAtlasRectId_Invalid)
            {
                ImTextureRect* r = ImFontAtlasPackGetRect(atlas, glyph.PackId);
                glyph.U0 = (r->x) * atlas->TexUvScale.x;
                glyph.V0 = (r->y) * atlas->TexUvScale.y;
                glyph.U1 = (r->x + r->w) * atlas->TexUvScale.x;
                glyph.V1 = (r->y + r->h) * atlas->TexUvScale.y;
            }

    // Update other cached UV
    ImFontAtlasBuildUpdateLinesTexData(atlas);
    ImFontAtlasBuildUpdateBasicTexData(atlas);

    builder->LockDisableResize = false;
    ImFontAtlasUpdateDrawListsSharedData(atlas);
    //ImFontAtlasDebugWriteTexToDisk(new_tex, "After Pack");
}

void ImFontAtlasTextureGrow(ImFontAtlas* atlas, int old_tex_w, int old_tex_h)
{
    //ImFontAtlasDebugWriteTexToDisk(atlas->TexData, "Before Grow");
    ImFontAtlasBuilder* builder = atlas->Builder;
    if (old_tex_w == -1)
        old_tex_w = atlas->TexData->Width;
    if (old_tex_h == -1)
        old_tex_h = atlas->TexData->Height;

    // FIXME-NEWATLAS-V2: What to do when reaching limits exposed by backend?
    // FIXME-NEWATLAS-V2: Does ImFontAtlasFlags_NoPowerOfTwoHeight makes sense now? Allow 'lock' and 'compact' operations?
    IM_ASSERT(ImIsPowerOfTwo(old_tex_w) && ImIsPowerOfTwo(old_tex_h));
    IM_ASSERT(ImIsPowerOfTwo(atlas->TexMinWidth) && ImIsPowerOfTwo(atlas->TexMaxWidth) && ImIsPowerOfTwo(atlas->TexMinHeight) && ImIsPowerOfTwo(atlas->TexMaxHeight));

    // Grow texture so it follows roughly a square.
    // - Grow height before width, as width imply more packing nodes.
    // - Caller should be taking account of RectsDiscardedSurface and may not need to grow.
    int new_tex_w = (old_tex_h <= old_tex_w) ? old_tex_w : old_tex_w * 2;
    int new_tex_h = (old_tex_h <= old_tex_w) ? old_tex_h * 2 : old_tex_h;

    // Handle minimum size first (for pathologically large packed rects)
    const int pack_padding = atlas->TexGlyphPadding;
    new_tex_w = ImMax(new_tex_w, ImUpperPowerOfTwo(builder->MaxRectSize.x + pack_padding));
    new_tex_h = ImMax(new_tex_h, ImUpperPowerOfTwo(builder->MaxRectSize.y + pack_padding));
    new_tex_w = ImClamp(new_tex_w, atlas->TexMinWidth, atlas->TexMaxWidth);
    new_tex_h = ImClamp(new_tex_h, atlas->TexMinHeight, atlas->TexMaxHeight);
    if (new_tex_w == old_tex_w && new_tex_h == old_tex_h)
        return;

    ImFontAtlasTextureRepack(atlas, new_tex_w, new_tex_h);
}

void ImFontAtlasTextureMakeSpace(ImFontAtlas* atlas)
{
    // Can some baked contents be ditched?
    //IMGUI_DEBUG_LOG_FONT("[font] ImFontAtlasBuildMakeSpace()\n");
    ImFontAtlasBuilder* builder = atlas->Builder;
    ImFontAtlasBuildDiscardBakes(atlas, 2);

    // Currently using a heuristic for repack without growing.
    if (builder->RectsDiscardedSurface < builder->RectsPackedSurface * 0.20f)
        ImFontAtlasTextureGrow(atlas);
    else
        ImFontAtlasTextureRepack(atlas, atlas->TexData->Width, atlas->TexData->Height);
}

ImVec2i ImFontAtlasTextureGetSizeEstimate(ImFontAtlas* atlas)
{
    int min_w = ImUpperPowerOfTwo(atlas->TexMinWidth);
    int min_h = ImUpperPowerOfTwo(atlas->TexMinHeight);
    if (atlas->Builder == NULL || atlas->TexData == NULL || atlas->TexData->Status == ImTextureStatus_WantDestroy)
        return ImVec2i(min_w, min_h);

    ImFontAtlasBuilder* builder = atlas->Builder;
    min_w = ImMax(ImUpperPowerOfTwo(builder->MaxRectSize.x), min_w);
    min_h = ImMax(ImUpperPowerOfTwo(builder->MaxRectSize.y), min_h);
    const int surface_approx = builder->RectsPackedSurface - builder->RectsDiscardedSurface; // Expected surface after repack
    const int surface_sqrt = (int)sqrtf((float)surface_approx);

    int new_tex_w;
    int new_tex_h;
    if (min_w >= min_h)
    {
        new_tex_w = ImMax(min_w, ImUpperPowerOfTwo(surface_sqrt));
        new_tex_h = ImMax(min_h, (int)((surface_approx + new_tex_w - 1) / new_tex_w));
        if ((atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) == 0)
            new_tex_h = ImUpperPowerOfTwo(new_tex_h);
    }
    else
    {
        new_tex_h = ImMax(min_h, ImUpperPowerOfTwo(surface_sqrt));
        if ((atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) == 0)
            new_tex_h = ImUpperPowerOfTwo(new_tex_h);
        new_tex_w = ImMax(min_w, (int)((surface_approx + new_tex_h - 1) / new_tex_h));
    }

    IM_ASSERT(ImIsPowerOfTwo(new_tex_w) && ImIsPowerOfTwo(new_tex_h));
    return ImVec2i(new_tex_w, new_tex_h);
}

// Clear all output. Invalidates all AddCustomRect() return values!
void ImFontAtlasBuildClear(ImFontAtlas* atlas)
{
    ImVec2i new_tex_size = ImFontAtlasTextureGetSizeEstimate(atlas);
    ImFontAtlasBuildDestroy(atlas);
    ImFontAtlasTextureAdd(atlas, new_tex_size.x, new_tex_size.y);
    ImFontAtlasBuildInit(atlas);
    for (ImFontConfig& src : atlas->Sources)
        ImFontAtlasFontSourceInit(atlas, &src);
    for (ImFont* font : atlas->Fonts)
        for (ImFontConfig* src : font->Sources)
            ImFontAtlasFontSourceAddToFont(atlas, font, src);
}

// You should not need to call this manually!
// If you think you do, let us know and we can advise about policies auto-compact.
void ImFontAtlasTextureCompact(ImFontAtlas* atlas)
{
    ImFontAtlasBuilder* builder = atlas->Builder;
    ImFontAtlasBuildDiscardBakes(atlas, 1);

    ImTextureData* old_tex = atlas->TexData;
    ImVec2i old_tex_size = ImVec2i(old_tex->Width, old_tex->Height);
    ImVec2i new_tex_size = ImFontAtlasTextureGetSizeEstimate(atlas);
    if (builder->RectsDiscardedCount == 0 && new_tex_size.x == old_tex_size.x && new_tex_size.y == old_tex_size.y)
        return;

    ImFontAtlasTextureRepack(atlas, new_tex_size.x, new_tex_size.y);
}

// Start packing over current empty texture
void ImFontAtlasBuildInit(ImFontAtlas* atlas)
{
    // Select Backend
    // - Note that we do not reassign to atlas->FontLoader, since it is likely to point to static data which
    //   may mess with some hot-reloading schemes. If you need to assign to this (for dynamic selection) AND are
    //   using a hot-reloading scheme that messes up static data, store your own instance of FontLoader somewhere
    //   and point to it instead of pointing directly to return value of the GetFontLoaderXXX functions.
    if (atlas->FontLoader == NULL)
    {
#ifdef IMGUI_ENABLE_FREETYPE
        atlas->SetFontLoader(ImGuiFreeType::GetFontLoader());
#elif defined(IMGUI_ENABLE_STB_TRUETYPE)
        atlas->SetFontLoader(ImFontAtlasGetFontLoaderForStbTruetype());
#else
        IM_ASSERT(0); // Invalid Build function
#endif
    }

    // Create initial texture size
    if (atlas->TexData == NULL || atlas->TexData->Pixels == NULL)
        ImFontAtlasTextureAdd(atlas, ImUpperPowerOfTwo(atlas->TexMinWidth), ImUpperPowerOfTwo(atlas->TexMinHeight));

    atlas->Builder = IM_NEW(ImFontAtlasBuilder)();
    if (atlas->FontLoader->LoaderInit)
        atlas->FontLoader->LoaderInit(atlas);

    ImFontAtlasBuildUpdateRendererHasTexturesFromContext(atlas);

    ImFontAtlasPackInit(atlas);

    // Add required texture data
    ImFontAtlasBuildUpdateLinesTexData(atlas);
    ImFontAtlasBuildUpdateBasicTexData(atlas);

    // Register fonts
    ImFontAtlasBuildUpdatePointers(atlas);

    // Update UV coordinates etc. stored in bound ImDrawListSharedData instance
    ImFontAtlasUpdateDrawListsSharedData(atlas);

    //atlas->TexIsBuilt = true;
}

// Destroy builder and all cached glyphs. Do not destroy actual fonts.
void ImFontAtlasBuildDestroy(ImFontAtlas* atlas)
{
    for (ImFont* font : atlas->Fonts)
        ImFontAtlasFontDestroyOutput(atlas, font);
    if (atlas->Builder && atlas->FontLoader && atlas->FontLoader->LoaderShutdown)
    {
        atlas->FontLoader->LoaderShutdown(atlas);
        IM_ASSERT(atlas->FontLoaderData == NULL);
    }
    IM_DELETE(atlas->Builder);
    atlas->Builder = NULL;
}

void ImFontAtlasPackInit(ImFontAtlas * atlas)
{
    ImTextureData* tex = atlas->TexData;
    ImFontAtlasBuilder* builder = atlas->Builder;

    // In theory we could decide to reduce the number of nodes, e.g. halve them, and waste a little texture space, but it doesn't seem worth it.
    const int pack_node_count = tex->Width / 2;
    builder->PackNodes.resize(pack_node_count);
    IM_STATIC_ASSERT(sizeof(stbrp_context) <= sizeof(stbrp_context_opaque));
    stbrp_init_target((stbrp_context*)(void*)&builder->PackContext, tex->Width, tex->Height, builder->PackNodes.Data, builder->PackNodes.Size);
    builder->RectsPackedSurface = builder->RectsPackedCount = 0;
    builder->MaxRectSize = ImVec2i(0, 0);
    builder->MaxRectBounds = ImVec2i(0, 0);
}

// This is essentially a free-list pattern, it may be nice to wrap it into a dedicated type.
static ImFontAtlasRectId ImFontAtlasPackAllocRectEntry(ImFontAtlas* atlas, int rect_idx)
{
    ImFontAtlasBuilder* builder = (ImFontAtlasBuilder*)atlas->Builder;
    int index_idx;
    ImFontAtlasRectEntry* index_entry;
    if (builder->RectsIndexFreeListStart < 0)
    {
        builder->RectsIndex.resize(builder->RectsIndex.Size + 1);
        index_idx = builder->RectsIndex.Size - 1;
        index_entry = &builder->RectsIndex[index_idx];
        memset(index_entry, 0, sizeof(*index_entry));
    }
    else
    {
        index_idx = builder->RectsIndexFreeListStart;
        index_entry = &builder->RectsIndex[index_idx];
        IM_ASSERT(index_entry->IsUsed == false && index_entry->Generation > 0); // Generation is incremented during DiscardRect
        builder->RectsIndexFreeListStart = index_entry->TargetIndex;
    }
    index_entry->TargetIndex = rect_idx;
    index_entry->IsUsed = 1;
    return ImFontAtlasRectId_Make(index_idx, index_entry->Generation);
}

// Overwrite existing entry
static ImFontAtlasRectId ImFontAtlasPackReuseRectEntry(ImFontAtlas* atlas, ImFontAtlasRectEntry* index_entry)
{
    IM_ASSERT(index_entry->IsUsed);
    index_entry->TargetIndex = atlas->Builder->Rects.Size - 1;
    int index_idx = atlas->Builder->RectsIndex.index_from_ptr(index_entry);
    return ImFontAtlasRectId_Make(index_idx, index_entry->Generation);
}

// This is expected to be called in batches and followed by a repack
void ImFontAtlasPackDiscardRect(ImFontAtlas* atlas, ImFontAtlasRectId id)
{
    IM_ASSERT(id != ImFontAtlasRectId_Invalid);

    ImTextureRect* rect = ImFontAtlasPackGetRect(atlas, id);
    if (rect == NULL)
        return;

    ImFontAtlasBuilder* builder = atlas->Builder;
    int index_idx = ImFontAtlasRectId_GetIndex(id);
    ImFontAtlasRectEntry* index_entry = &builder->RectsIndex[index_idx];
    IM_ASSERT(index_entry->IsUsed && index_entry->TargetIndex >= 0);
    index_entry->IsUsed = false;
    index_entry->TargetIndex = builder->RectsIndexFreeListStart;
    index_entry->Generation++;

    const int pack_padding = atlas->TexGlyphPadding;
    builder->RectsIndexFreeListStart = index_idx;
    builder->RectsDiscardedCount++;
    builder->RectsDiscardedSurface += (rect->w + pack_padding) * (rect->h + pack_padding);
    rect->w = rect->h = 0; // Clear rectangle so it won't be packed again
}

// Important: Calling this may recreate a new texture and therefore change atlas->TexData
// FIXME-NEWFONTS: Expose other glyph padding settings for custom alteration (e.g. drop shadows). See #7962
ImFontAtlasRectId ImFontAtlasPackAddRect(ImFontAtlas* atlas, int w, int h, ImFontAtlasRectEntry* overwrite_entry)
{
    IM_ASSERT(w > 0 && w <= 0xFFFF);
    IM_ASSERT(h > 0 && h <= 0xFFFF);

    ImFontAtlasBuilder* builder = (ImFontAtlasBuilder*)atlas->Builder;
    const int pack_padding = atlas->TexGlyphPadding;
    builder->MaxRectSize.x = ImMax(builder->MaxRectSize.x, w);
    builder->MaxRectSize.y = ImMax(builder->MaxRectSize.y, h);

    // Pack
    ImTextureRect r = { 0, 0, (unsigned short)w, (unsigned short)h };
    for (int attempts_remaining = 3; attempts_remaining >= 0; attempts_remaining--)
    {
        // Try packing
        stbrp_rect pack_r = {};
        pack_r.w = w + pack_padding;
        pack_r.h = h + pack_padding;
        stbrp_pack_rects((stbrp_context*)(void*)&builder->PackContext, &pack_r, 1);
        r.x = (unsigned short)pack_r.x;
        r.y = (unsigned short)pack_r.y;
        if (pack_r.was_packed)
            break;

        // If we ran out of attempts, return fallback
        if (attempts_remaining == 0 || builder->LockDisableResize)
        {
            IMGUI_DEBUG_LOG_FONT("[font] Failed packing %dx%d rectangle. Returning fallback.\n", w, h);
            return ImFontAtlasRectId_Invalid;
        }

        // Resize or repack atlas! (this should be a rare event)
        ImFontAtlasTextureMakeSpace(atlas);
    }

    builder->MaxRectBounds.x = ImMax(builder->MaxRectBounds.x, r.x + r.w + pack_padding);
    builder->MaxRectBounds.y = ImMax(builder->MaxRectBounds.y, r.y + r.h + pack_padding);
    builder->RectsPackedCount++;
    builder->RectsPackedSurface += (w + pack_padding) * (h + pack_padding);

    builder->Rects.push_back(r);
    if (overwrite_entry != NULL)
        return ImFontAtlasPackReuseRectEntry(atlas, overwrite_entry); // Write into an existing entry instead of adding one (used during repack)
    else
        return ImFontAtlasPackAllocRectEntry(atlas, builder->Rects.Size - 1);
}

// Generally for non-user facing functions: assert on invalid ID.
ImTextureRect* ImFontAtlasPackGetRect(ImFontAtlas* atlas, ImFontAtlasRectId id)
{
    IM_ASSERT(id != ImFontAtlasRectId_Invalid);
    int index_idx = ImFontAtlasRectId_GetIndex(id);
    ImFontAtlasBuilder* builder = (ImFontAtlasBuilder*)atlas->Builder;
    ImFontAtlasRectEntry* index_entry = &builder->RectsIndex[index_idx];
    IM_ASSERT(index_entry->Generation == ImFontAtlasRectId_GetGeneration(id));
    IM_ASSERT(index_entry->IsUsed);
    return &builder->Rects[index_entry->TargetIndex];
}

// For user-facing functions: return NULL on invalid ID.
// Important: return pointer is valid until next call to AddRect(), e.g. FindGlyph(), CalcTextSize() can all potentially invalidate previous pointers.
ImTextureRect* ImFontAtlasPackGetRectSafe(ImFontAtlas* atlas, ImFontAtlasRectId id)
{
    if (id == ImFontAtlasRectId_Invalid)
        return NULL;
    int index_idx = ImFontAtlasRectId_GetIndex(id);
    if (atlas->Builder == NULL)
        ImFontAtlasBuildInit(atlas);
    ImFontAtlasBuilder* builder = (ImFontAtlasBuilder*)atlas->Builder;
    if (index_idx >= builder->RectsIndex.Size)
        return NULL;
    ImFontAtlasRectEntry* index_entry = &builder->RectsIndex[index_idx];
    if (index_entry->Generation != ImFontAtlasRectId_GetGeneration(id) || !index_entry->IsUsed)
        return NULL;
    return &builder->Rects[index_entry->TargetIndex];
}

// Important! This assume by ImFontConfig::GlyphExcludeRanges[] is a SMALL ARRAY (e.g. <10 entries)
// Use "Input Glyphs Overlap Detection Tool" to display a list of glyphs provided by multiple sources in order to set this array up.
static bool ImFontAtlasBuildAcceptCodepointForSource(ImFontConfig* src, ImWchar codepoint)
{
    if (const ImWchar* exclude_list = src->GlyphExcludeRanges)
        for (; exclude_list[0] != 0; exclude_list += 2)
            if (codepoint >= exclude_list[0] && codepoint <= exclude_list[1])
                return false;
    return true;
}

static void ImFontBaked_BuildGrowIndex(ImFontBaked* baked, int new_size)
{
    IM_ASSERT(baked->IndexAdvanceX.Size == baked->IndexLookup.Size);
    if (new_size <= baked->IndexLookup.Size)
        return;
    baked->IndexAdvanceX.resize(new_size, -1.0f);
    baked->IndexLookup.resize(new_size, IM_FONTGLYPH_INDEX_UNUSED);
}

static void ImFontAtlas_FontHookRemapCodepoint(ImFontAtlas* atlas, ImFont* font, ImWchar* c)
{
    IM_UNUSED(atlas);
    if (font->RemapPairs.Data.Size != 0)
        *c = (ImWchar)font->RemapPairs.GetInt((ImGuiID)*c, (int)*c);
}

static ImFontGlyph* ImFontBaked_BuildLoadGlyph(ImFontBaked* baked, ImWchar codepoint, float* only_load_advance_x)
{
    ImFont* font = baked->ContainerFont;
    ImFontAtlas* atlas = font->ContainerAtlas;
    if (atlas->Locked || (font->Flags & ImFontFlags_NoLoadGlyphs))
    {
        // Lazily load fallback glyph
        if (baked->FallbackGlyphIndex == -1 && baked->LockLoadingFallback == 0)
            ImFontAtlasBuildSetupFontBakedFallback(baked);
        return NULL;
    }

    // User remapping hooks
    ImWchar src_codepoint = codepoint;
    ImFontAtlas_FontHookRemapCodepoint(atlas, font, &codepoint);

    //char utf8_buf[5];
    //IMGUI_DEBUG_LOG("[font] BuildLoadGlyph U+%04X (%s)\n", (unsigned int)codepoint, ImTextCharToUtf8(utf8_buf, (unsigned int)codepoint));

    // Special hook
    // FIXME-NEWATLAS: it would be nicer if this used a more standardized way of hooking
    if (codepoint == font->EllipsisChar && font->EllipsisAutoBake)
        if (ImFontGlyph* glyph = ImFontAtlasBuildSetupFontBakedEllipsis(atlas, baked))
            return glyph;

    // Call backend
    char* loader_user_data_p = (char*)baked->FontLoaderDatas;
    int src_n = 0;
    for (ImFontConfig* src : font->Sources)
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        if (!src->GlyphExcludeRanges || ImFontAtlasBuildAcceptCodepointForSource(src, codepoint))
        {
            if (only_load_advance_x == NULL)
            {
                ImFontGlyph glyph_buf;
                if (loader->FontBakedLoadGlyph(atlas, src, baked, loader_user_data_p, codepoint, &glyph_buf, NULL))
                {
                    // FIXME: Add hooks for e.g. #7962
                    glyph_buf.Codepoint = src_codepoint;
                    glyph_buf.SourceIdx = src_n;
                    return ImFontAtlasBakedAddFontGlyph(atlas, baked, src, &glyph_buf);
                }
            }
            else
            {
                // Special mode but only loading glyphs metrics. Will rasterize and pack later.
                if (loader->FontBakedLoadGlyph(atlas, src, baked, loader_user_data_p, codepoint, NULL, only_load_advance_x))
                {
                    ImFontAtlasBakedAddFontGlyphAdvancedX(atlas, baked, src, codepoint, *only_load_advance_x);
                    return NULL;
                }
            }
        }
        loader_user_data_p += loader->FontBakedSrcLoaderDataSize;
        src_n++;
    }

    // Lazily load fallback glyph
    if (baked->LockLoadingFallback)
        return NULL;
    if (baked->FallbackGlyphIndex == -1)
        ImFontAtlasBuildSetupFontBakedFallback(baked);

    // Mark index as not found, so we don't attempt the search twice
    ImFontBaked_BuildGrowIndex(baked, codepoint + 1);
    baked->IndexAdvanceX[codepoint] = baked->FallbackAdvanceX;
    baked->IndexLookup[codepoint] = IM_FONTGLYPH_INDEX_NOT_FOUND;
    return NULL;
}

static float ImFontBaked_BuildLoadGlyphAdvanceX(ImFontBaked* baked, ImWchar codepoint)
{
    if (baked->Size >= IMGUI_FONT_SIZE_THRESHOLD_FOR_LOADADVANCEXONLYMODE)
    {
        // First load AdvanceX value used by CalcTextSize() API then load the rest when loaded by drawing API.
        float only_advance_x = 0.0f;
        ImFontGlyph* glyph = ImFontBaked_BuildLoadGlyph(baked, (ImWchar)codepoint, &only_advance_x);
        return glyph ? glyph->AdvanceX : only_advance_x;
    }
    else
    {
        ImFontGlyph* glyph = ImFontBaked_BuildLoadGlyph(baked, (ImWchar)codepoint, NULL);
        return glyph ? glyph->AdvanceX : baked->FallbackAdvanceX;
    }
}

// The point of this indirection is to not be inlined in debug mode in order to not bloat inner loop.b
IM_MSVC_RUNTIME_CHECKS_OFF
static float BuildLoadGlyphGetAdvanceOrFallback(ImFontBaked* baked, unsigned int codepoint)
{
    return ImFontBaked_BuildLoadGlyphAdvanceX(baked, (ImWchar)codepoint);
}
IM_MSVC_RUNTIME_CHECKS_RESTORE

#ifndef IMGUI_DISABLE_DEBUG_TOOLS
void ImFontAtlasDebugLogTextureRequests(ImFontAtlas* atlas)
{
    // [DEBUG] Log texture update requests
    ImGuiContext& g = *GImGui;
    IM_UNUSED(g);
    for (ImTextureData* tex : atlas->TexList)
    {
        if ((g.IO.BackendFlags & ImGuiBackendFlags_RendererHasTextures) == 0)
            IM_ASSERT(tex->Updates.Size == 0);
        if (tex->Status == ImTextureStatus_WantCreate)
            IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: create %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
        else if (tex->Status == ImTextureStatus_WantDestroy)
            IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: destroy %dx%d, texid=0x%" IM_PRIX64 ", backend_data=%p\n", tex->UniqueID, tex->Width, tex->Height, tex->TexID, tex->BackendUserData);
        else if (tex->Status == ImTextureStatus_WantUpdates)
        {
            IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: update %d regions, texid=0x%" IM_PRIX64 ", backend_data=0x%" IM_PRIX64 "\n", tex->UniqueID, tex->Updates.Size, tex->TexID, (ImU64)(intptr_t)tex->BackendUserData);
            for (const ImTextureRect& r : tex->Updates)
            {
                IM_UNUSED(r);
                IM_ASSERT(r.x >= 0 && r.y >= 0);
                IM_ASSERT(r.x + r.w <= tex->Width && r.y + r.h <= tex->Height); // In theory should subtract PackPadding but it's currently part of atlas and mid-frame change would wreck assert.
                //IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: update (% 4d..%-4d)->(% 4d..%-4d), texid=0x%" IM_PRIX64 ", backend_data=0x%" IM_PRIX64 "\n", tex->UniqueID, r.x, r.y, r.x + r.w, r.y + r.h, tex->TexID, (ImU64)(intptr_t)tex->BackendUserData);
            }
        }
    }
}
#endif

//-------------------------------------------------------------------------
// [SECTION] ImFontAtlas: backend for stb_truetype
//-------------------------------------------------------------------------
// (imstb_truetype.h in included near the top of this file, when IMGUI_ENABLE_STB_TRUETYPE is set)
//-------------------------------------------------------------------------

#ifdef IMGUI_ENABLE_STB_TRUETYPE

// One for each ConfigData
struct ImGui_ImplStbTrueType_FontSrcData
{
    stbtt_fontinfo  FontInfo;
    float           ScaleFactor;
};

static bool ImGui_ImplStbTrueType_FontSrcInit(ImFontAtlas* atlas, ImFontConfig* src)
{
    IM_UNUSED(atlas);

    ImGui_ImplStbTrueType_FontSrcData* bd_font_data = IM_NEW(ImGui_ImplStbTrueType_FontSrcData);
    IM_ASSERT(src->FontLoaderData == NULL);

    // Initialize helper structure for font loading and verify that the TTF/OTF data is correct
    const int font_offset = stbtt_GetFontOffsetForIndex((unsigned char*)src->FontData, src->FontNo);
    if (font_offset < 0)
    {
        IM_DELETE(bd_font_data);
        IM_ASSERT_USER_ERROR(0, "stbtt_GetFontOffsetForIndex(): FontData is incorrect, or FontNo cannot be found.");
        return false;
    }
    if (!stbtt_InitFont(&bd_font_data->FontInfo, (unsigned char*)src->FontData, font_offset))
    {
        IM_DELETE(bd_font_data);
        IM_ASSERT_USER_ERROR(0, "stbtt_InitFont(): failed to parse FontData. It is correct and complete? Check FontDataSize.");
        return false;
    }
    src->FontLoaderData = bd_font_data;

    if (src->MergeMode && src->SizePixels == 0.0f)
        src->SizePixels = src->DstFont->Sources[0]->SizePixels;

    if (src->SizePixels >= 0.0f)
        bd_font_data->ScaleFactor = stbtt_ScaleForPixelHeight(&bd_font_data->FontInfo, 1.0f);
    else
        bd_font_data->ScaleFactor = stbtt_ScaleForMappingEmToPixels(&bd_font_data->FontInfo, 1.0f);
    if (src->MergeMode && src->SizePixels != 0.0f)
        bd_font_data->ScaleFactor *= src->SizePixels / src->DstFont->Sources[0]->SizePixels; // FIXME-NEWATLAS: Should tidy up that a bit

    return true;
}

static void ImGui_ImplStbTrueType_FontSrcDestroy(ImFontAtlas* atlas, ImFontConfig* src)
{
    IM_UNUSED(atlas);
    ImGui_ImplStbTrueType_FontSrcData* bd_font_data = (ImGui_ImplStbTrueType_FontSrcData*)src->FontLoaderData;
    IM_DELETE(bd_font_data);
    src->FontLoaderData = NULL;
}

static bool ImGui_ImplStbTrueType_FontSrcContainsGlyph(ImFontAtlas* atlas, ImFontConfig* src, ImWchar codepoint)
{
    IM_UNUSED(atlas);

    ImGui_ImplStbTrueType_FontSrcData* bd_font_data = (ImGui_ImplStbTrueType_FontSrcData*)src->FontLoaderData;
    IM_ASSERT(bd_font_data != NULL);

    int glyph_index = stbtt_FindGlyphIndex(&bd_font_data->FontInfo, (int)codepoint);
    return glyph_index != 0;
}

static bool ImGui_ImplStbTrueType_FontBakedInit(ImFontAtlas* atlas, ImFontConfig* src, ImFontBaked* baked, void*)
{
    IM_UNUSED(atlas);

    ImGui_ImplStbTrueType_FontSrcData* bd_font_data = (ImGui_ImplStbTrueType_FontSrcData*)src->FontLoaderData;
    if (src->MergeMode == false)
    {
        // FIXME-NEWFONTS: reevaluate how to use sizing metrics
        // FIXME-NEWFONTS: make use of line gap value
        float scale_for_layout = bd_font_data->ScaleFactor * baked->Size;
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&bd_font_data->FontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);
        baked->Ascent = ImCeil(unscaled_ascent * scale_for_layout);
        baked->Descent = ImFloor(unscaled_descent * scale_for_layout);
    }
    return true;
}

static bool ImGui_ImplStbTrueType_FontBakedLoadGlyph(ImFontAtlas* atlas, ImFontConfig* src, ImFontBaked* baked, void*, ImWchar codepoint, ImFontGlyph* out_glyph, float* out_advance_x)
{
    // Search for first font which has the glyph
    ImGui_ImplStbTrueType_FontSrcData* bd_font_data = (ImGui_ImplStbTrueType_FontSrcData*)src->FontLoaderData;
    IM_ASSERT(bd_font_data);
    int glyph_index = stbtt_FindGlyphIndex(&bd_font_data->FontInfo, (int)codepoint);
    if (glyph_index == 0)
        return false;

    // Fonts unit to pixels
    int oversample_h, oversample_v;
    ImFontAtlasBuildGetOversampleFactors(src, baked, &oversample_h, &oversample_v);
    const float scale_for_layout = bd_font_data->ScaleFactor * baked->Size;
    const float rasterizer_density = src->RasterizerDensity * baked->RasterizerDensity;
    const float scale_for_raster_x = bd_font_data->ScaleFactor * baked->Size * rasterizer_density * oversample_h;
    const float scale_for_raster_y = bd_font_data->ScaleFactor * baked->Size * rasterizer_density * oversample_v;

    // Obtain size and advance
    int x0, y0, x1, y1;
    int advance, lsb;
    stbtt_GetGlyphBitmapBoxSubpixel(&bd_font_data->FontInfo, glyph_index, scale_for_raster_x, scale_for_raster_y, 0, 0, &x0, &y0, &x1, &y1);
    stbtt_GetGlyphHMetrics(&bd_font_data->FontInfo, glyph_index, &advance, &lsb);

    // Load metrics only mode
    if (out_advance_x != NULL)
    {
        IM_ASSERT(out_glyph == NULL);
        *out_advance_x = advance * scale_for_layout;
        return true;
    }

    // Prepare glyph
    out_glyph->Codepoint = codepoint;
    out_glyph->AdvanceX = advance * scale_for_layout;

    // Pack and retrieve position inside texture atlas
    // (generally based on stbtt_PackFontRangesRenderIntoRects)
    const bool is_visible = (x0 != x1 && y0 != y1);
    if (is_visible)
    {
        const int w = (x1 - x0 + oversample_h - 1);
        const int h = (y1 - y0 + oversample_v - 1);
        ImFontAtlasRectId pack_id = ImFontAtlasPackAddRect(atlas, w, h);
        if (pack_id == ImFontAtlasRectId_Invalid)
        {
            // Pathological out of memory case (TexMaxWidth/TexMaxHeight set too small?)
            IM_ASSERT(pack_id != ImFontAtlasRectId_Invalid && "Out of texture memory.");
            return false;
        }
        ImTextureRect* r = ImFontAtlasPackGetRect(atlas, pack_id);

        // Render
        stbtt_GetGlyphBitmapBox(&bd_font_data->FontInfo, glyph_index, scale_for_raster_x, scale_for_raster_y, &x0, &y0, &x1, &y1);
        ImFontAtlasBuilder* builder = atlas->Builder;
        builder->TempBuffer.resize(w * h * 1);
        unsigned char* bitmap_pixels = builder->TempBuffer.Data;
        memset(bitmap_pixels, 0, w * h * 1);
        stbtt_MakeGlyphBitmapSubpixel(&bd_font_data->FontInfo, bitmap_pixels, r->w - oversample_h + 1, r->h - oversample_v + 1, w,
            scale_for_raster_x, scale_for_raster_y, 0, 0, glyph_index);

        // Oversampling
        // (those functions conveniently assert if pixels are not cleared, which is another safety layer)
        if (oversample_h > 1)
            stbtt__h_prefilter(bitmap_pixels, r->w, r->h, r->w, oversample_h);
        if (oversample_v > 1)
            stbtt__v_prefilter(bitmap_pixels, r->w, r->h, r->w, oversample_v);

        const float ref_size = baked->ContainerFont->Sources[0]->SizePixels;
        const float offsets_scale = (ref_size != 0.0f) ? (baked->Size / ref_size) : 1.0f;
        float font_off_x = (src->GlyphOffset.x * offsets_scale);
        float font_off_y = (src->GlyphOffset.y * offsets_scale);
        if (src->PixelSnapH) // Snap scaled offset. This is to mitigate backward compatibility issues for GlyphOffset, but a better design would be welcome.
            font_off_x = IM_ROUND(font_off_x);
        if (src->PixelSnapV)
            font_off_y = IM_ROUND(font_off_y);
        font_off_x += stbtt__oversample_shift(oversample_h);
        font_off_y += stbtt__oversample_shift(oversample_v) + IM_ROUND(baked->Ascent);
        float recip_h = 1.0f / (oversample_h * rasterizer_density);
        float recip_v = 1.0f / (oversample_v * rasterizer_density);

        // Register glyph
        // r->x r->y are coordinates inside texture (in pixels)
        // glyph.X0, glyph.Y0 are drawing coordinates from base text position, and accounting for oversampling.
        out_glyph->X0 = x0 * recip_h + font_off_x;
        out_glyph->Y0 = y0 * recip_v + font_off_y;
        out_glyph->X1 = (x0 + (int)r->w) * recip_h + font_off_x;
        out_glyph->Y1 = (y0 + (int)r->h) * recip_v + font_off_y;
        out_glyph->Visible = true;
        out_glyph->PackId = pack_id;
        ImFontAtlasBakedSetFontGlyphBitmap(atlas, baked, src, out_glyph, r, bitmap_pixels, ImTextureFormat_Alpha8, w);
    }

    return true;
}

const ImFontLoader* ImFontAtlasGetFontLoaderForStbTruetype()
{
    static ImFontLoader loader;
    loader.Name = "stb_truetype";
    loader.FontSrcInit = ImGui_ImplStbTrueType_FontSrcInit;
    loader.FontSrcDestroy = ImGui_ImplStbTrueType_FontSrcDestroy;
    loader.FontSrcContainsGlyph = ImGui_ImplStbTrueType_FontSrcContainsGlyph;
    loader.FontBakedInit = ImGui_ImplStbTrueType_FontBakedInit;
    loader.FontBakedDestroy = NULL;
    loader.FontBakedLoadGlyph = ImGui_ImplStbTrueType_FontBakedLoadGlyph;
    return &loader;
}

#endif // IMGUI_ENABLE_STB_TRUETYPE

//-------------------------------------------------------------------------
// [SECTION] ImFontAtlas: glyph ranges helpers
//-------------------------------------------------------------------------
// - GetGlyphRangesDefault()
// Obsolete functions since 1.92:
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

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
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
#endif // #ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS

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

ImFontBaked::ImFontBaked()
{
    memset(this, 0, sizeof(*this));
    FallbackGlyphIndex = -1;
}

void ImFontBaked::ClearOutputData()
{
    FallbackAdvanceX = 0.0f;
    Glyphs.clear();
    IndexAdvanceX.clear();
    IndexLookup.clear();
    FallbackGlyphIndex = -1;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
}

ImFont::ImFont()
{
    memset(this, 0, sizeof(*this));
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    Scale = 1.0f;
#endif
}

ImFont::~ImFont()
{
    ClearOutputData();
}

void ImFont::ClearOutputData()
{
    if (ImFontAtlas* atlas = ContainerAtlas)
        ImFontAtlasFontDiscardBakes(atlas, this, 0);
    FallbackChar = EllipsisChar = 0;
    memset(Used8kPagesMap, 0, sizeof(Used8kPagesMap));
    LastBaked = NULL;
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

// x0/y0/x1/y1 are offset from the character upper-left layout position, in pixels. Therefore x0/y0 are often fairly close to zero.
// Not to be mistaken with texture coordinates, which are held by u0/v0/u1/v1 in normalized format (0.0..1.0 on each texture axis).
// - 'src' is not necessarily == 'this->Sources' because multiple source fonts+configs can be used to build one target font.
ImFontGlyph* ImFontAtlasBakedAddFontGlyph(ImFontAtlas* atlas, ImFontBaked* baked, ImFontConfig* src, const ImFontGlyph* in_glyph)
{
    int glyph_idx = baked->Glyphs.Size;
    baked->Glyphs.push_back(*in_glyph);
    ImFontGlyph* glyph = &baked->Glyphs[glyph_idx];
    IM_ASSERT(baked->Glyphs.Size < 0xFFFE); // IndexLookup[] hold 16-bit values and -1/-2 are reserved.

    // Set UV from packed rectangle
    if (glyph->PackId != ImFontAtlasRectId_Invalid)
    {
        ImTextureRect* r = ImFontAtlasPackGetRect(atlas, glyph->PackId);
        IM_ASSERT(glyph->U0 == 0.0f && glyph->V0 == 0.0f && glyph->U1 == 0.0f && glyph->V1 == 0.0f);
        glyph->U0 = (r->x) * atlas->TexUvScale.x;
        glyph->V0 = (r->y) * atlas->TexUvScale.y;
        glyph->U1 = (r->x + r->w) * atlas->TexUvScale.x;
        glyph->V1 = (r->y + r->h) * atlas->TexUvScale.y;
        baked->MetricsTotalSurface += r->w * r->h;
    }

    if (src != NULL)
    {
        // Clamp & recenter if needed
        const float ref_size = baked->ContainerFont->Sources[0]->SizePixels;
        const float offsets_scale = (ref_size != 0.0f) ? (baked->Size / ref_size) : 1.0f;
        float advance_x = ImClamp(glyph->AdvanceX, src->GlyphMinAdvanceX * offsets_scale, src->GlyphMaxAdvanceX * offsets_scale);
        if (advance_x != glyph->AdvanceX)
        {
            float char_off_x = src->PixelSnapH ? ImTrunc((advance_x - glyph->AdvanceX) * 0.5f) : (advance_x - glyph->AdvanceX) * 0.5f;
            glyph->X0 += char_off_x;
            glyph->X1 += char_off_x;
        }

        // Snap to pixel
        if (src->PixelSnapH)
            advance_x = IM_ROUND(advance_x);

        // Bake spacing
        glyph->AdvanceX = advance_x + src->GlyphExtraAdvanceX;
    }
    if (glyph->Colored)
        atlas->TexPixelsUseColors = atlas->TexData->UseColors = true;

    // Update lookup tables
    const int codepoint = glyph->Codepoint;
    ImFontBaked_BuildGrowIndex(baked, codepoint + 1);
    baked->IndexAdvanceX[codepoint] = glyph->AdvanceX;
    baked->IndexLookup[codepoint] = (ImU16)glyph_idx;
    const int page_n = codepoint / 8192;
    baked->ContainerFont->Used8kPagesMap[page_n >> 3] |= 1 << (page_n & 7);

    return glyph;
}

// FIXME: Code is duplicated with code above.
void ImFontAtlasBakedAddFontGlyphAdvancedX(ImFontAtlas* atlas, ImFontBaked* baked, ImFontConfig* src, ImWchar codepoint, float advance_x)
{
    IM_UNUSED(atlas);
    if (src != NULL)
    {
        // Clamp & recenter if needed
        const float ref_size = baked->ContainerFont->Sources[0]->SizePixels;
        const float offsets_scale = (ref_size != 0.0f) ? (baked->Size / ref_size) : 1.0f;
        advance_x = ImClamp(advance_x, src->GlyphMinAdvanceX * offsets_scale, src->GlyphMaxAdvanceX * offsets_scale);

        // Snap to pixel
        if (src->PixelSnapH)
            advance_x = IM_ROUND(advance_x);

        // Bake spacing
        advance_x += src->GlyphExtraAdvanceX;
    }

    ImFontBaked_BuildGrowIndex(baked, codepoint + 1);
    baked->IndexAdvanceX[codepoint] = advance_x;
}

// Copy to texture, post-process and queue update for backend
void ImFontAtlasBakedSetFontGlyphBitmap(ImFontAtlas* atlas, ImFontBaked* baked, ImFontConfig* src, ImFontGlyph* glyph, ImTextureRect* r, const unsigned char* src_pixels, ImTextureFormat src_fmt, int src_pitch)
{
    ImTextureData* tex = atlas->TexData;
    IM_ASSERT(r->x + r->w <= tex->Width && r->y + r->h <= tex->Height);
    ImFontAtlasTextureBlockConvert(src_pixels, src_fmt, src_pitch, (unsigned char*)tex->GetPixelsAt(r->x, r->y), tex->Format, tex->GetPitch(), r->w, r->h);
    ImFontAtlasPostProcessData pp_data = { atlas, baked->ContainerFont, src, baked, glyph, tex->GetPixelsAt(r->x, r->y), tex->Format, tex->GetPitch(), r->w, r->h };
    ImFontAtlasTextureBlockPostProcess(&pp_data);
    ImFontAtlasTextureBlockQueueUpload(atlas, tex, r->x, r->y, r->w, r->h);
}

void ImFont::AddRemapChar(ImWchar from_codepoint, ImWchar to_codepoint)
{
    RemapPairs.SetInt((ImGuiID)from_codepoint, (int)to_codepoint);
}

// Find glyph, load if necessary, return fallback if missing
ImFontGlyph* ImFontBaked::FindGlyph(ImWchar c)
{
    if (c < (size_t)IndexLookup.Size) IM_LIKELY
    {
        const int i = (int)IndexLookup.Data[c];
        if (i == IM_FONTGLYPH_INDEX_NOT_FOUND)
            return &Glyphs.Data[FallbackGlyphIndex];
        if (i != IM_FONTGLYPH_INDEX_UNUSED)
            return &Glyphs.Data[i];
    }
    ImFontGlyph* glyph = ImFontBaked_BuildLoadGlyph(this, c, NULL);
    return glyph ? glyph : &Glyphs.Data[FallbackGlyphIndex];
}

// Attempt to load but when missing, return NULL instead of FallbackGlyph
ImFontGlyph* ImFontBaked::FindGlyphNoFallback(ImWchar c)
{
    if (c < (size_t)IndexLookup.Size) IM_LIKELY
    {
        const int i = (int)IndexLookup.Data[c];
        if (i == IM_FONTGLYPH_INDEX_NOT_FOUND)
            return NULL;
        if (i != IM_FONTGLYPH_INDEX_UNUSED)
            return &Glyphs.Data[i];
    }
    LockLoadingFallback = true; // This is actually a rare call, not done in hot-loop, so we prioritize not adding extra cruft to ImFontBaked_BuildLoadGlyph() call sites.
    ImFontGlyph* glyph = ImFontBaked_BuildLoadGlyph(this, c, NULL);
    LockLoadingFallback = false;
    return glyph;
}

bool ImFontBaked::IsGlyphLoaded(ImWchar c)
{
    if (c < (size_t)IndexLookup.Size) IM_LIKELY
    {
        const int i = (int)IndexLookup.Data[c];
        if (i == IM_FONTGLYPH_INDEX_NOT_FOUND)
            return false;
        if (i != IM_FONTGLYPH_INDEX_UNUSED)
            return true;
    }
    return false;
}

// This is not fast query
bool ImFont::IsGlyphInFont(ImWchar c)
{
    ImFontAtlas* atlas = ContainerAtlas;
    ImFontAtlas_FontHookRemapCodepoint(atlas, this, &c);
    for (ImFontConfig* src : Sources)
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        if (loader->FontSrcContainsGlyph != NULL && loader->FontSrcContainsGlyph(atlas, src, c))
            return true;
    }
    return false;
}

// This is manually inlined in CalcTextSizeA() and CalcWordWrapPosition(), with a non-inline call to BuildLoadGlyphGetAdvanceOrFallback().
IM_MSVC_RUNTIME_CHECKS_OFF
float ImFontBaked::GetCharAdvance(ImWchar c)
{
    if ((int)c < IndexAdvanceX.Size)
    {
        // Missing glyphs fitting inside index will have stored FallbackAdvanceX already.
        const float x = IndexAdvanceX.Data[c];
        if (x >= 0.0f)
            return x;
    }
    return ImFontBaked_BuildLoadGlyphAdvanceX(this, c);
}
IM_MSVC_RUNTIME_CHECKS_RESTORE

ImGuiID ImFontAtlasBakedGetId(ImGuiID font_id, float baked_size, float rasterizer_density)
{
    struct { ImGuiID FontId; float BakedSize; float RasterizerDensity; } hashed_data;
    hashed_data.FontId = font_id;
    hashed_data.BakedSize = baked_size;
    hashed_data.RasterizerDensity = rasterizer_density;
    return ImHashData(&hashed_data, sizeof(hashed_data));
}

// ImFontBaked pointers are valid for the entire frame but shall never be kept between frames.
ImFontBaked* ImFont::GetFontBaked(float size, float density)
{
    ImFontBaked* baked = LastBaked;

    // Round font size
    // - ImGui::PushFont() will already round, but other paths calling GetFontBaked() directly also needs it (e.g. ImFontAtlasBuildPreloadAllGlyphRanges)
    size = ImGui::GetRoundedFontSize(size);

    if (density < 0.0f)
        density = CurrentRasterizerDensity;
    if (baked && baked->Size == size && baked->RasterizerDensity == density)
        return baked;

    ImFontAtlas* atlas = ContainerAtlas;
    ImFontAtlasBuilder* builder = atlas->Builder;
    baked = ImFontAtlasBakedGetOrAdd(atlas, this, size, density);
    if (baked == NULL)
        return NULL;
    baked->LastUsedFrame = builder->FrameCount;
    LastBaked = baked;
    return baked;
}

ImFontBaked* ImFontAtlasBakedGetOrAdd(ImFontAtlas* atlas, ImFont* font, float font_size, float font_rasterizer_density)
{
    // FIXME-NEWATLAS: Design for picking a nearest size based on some criteria?
    // FIXME-NEWATLAS: Altering font density won't work right away.
    IM_ASSERT(font_size > 0.0f && font_rasterizer_density > 0.0f);
    ImGuiID baked_id = ImFontAtlasBakedGetId(font->FontId, font_size, font_rasterizer_density);
    ImFontAtlasBuilder* builder = atlas->Builder;
    ImFontBaked** p_baked_in_map = (ImFontBaked**)builder->BakedMap.GetVoidPtrRef(baked_id);
    ImFontBaked* baked = *p_baked_in_map;
    if (baked != NULL)
    {
        IM_ASSERT(baked->Size == font_size && baked->ContainerFont == font && baked->BakedId == baked_id);
        return baked;
    }

    // If atlas is locked, find closest match
    // FIXME-OPT: This is not an optimal query.
    if ((font->Flags & ImFontFlags_LockBakedSizes) || atlas->Locked)
    {
        baked = ImFontAtlasBakedGetClosestMatch(atlas, font, font_size, font_rasterizer_density);
        if (baked != NULL)
            return baked;
        if (atlas->Locked)
        {
            IM_ASSERT(!atlas->Locked && "Cannot use dynamic font size with a locked ImFontAtlas!"); // Locked because rendering backend does not support ImGuiBackendFlags_RendererHasTextures!
            return NULL;
        }
    }

    // Create new
    baked = ImFontAtlasBakedAdd(atlas, font, font_size, font_rasterizer_density, baked_id);
    *p_baked_in_map = baked; // To avoid 'builder->BakedMap.SetVoidPtr(baked_id, baked);' while we can.
    return baked;
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

// Simple word-wrapping for English, not full-featured. Please submit failing cases!
// This will return the next location to wrap from. If no wrapping if necessary, this will fast-forward to e.g. text_end.
// FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)
const char* ImFont::CalcWordWrapPosition(float size, const char* text, const char* text_end, float wrap_width)
{
    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    ImFontBaked* baked = GetFontBaked(size);
    const float scale = size / baked->Size;

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

        // Optimized inline version of 'float char_width = GetCharAdvance((ImWchar)c);'
        float char_width = (c < (unsigned int)baked->IndexAdvanceX.Size) ? baked->IndexAdvanceX.Data[c] : -1.0f;
        if (char_width < 0.0f)
            char_width = BuildLoadGlyphGetAdvanceOrFallback(baked, c);

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
            inside_word = (c != '.' && c != ',' && c != ';' && c != '!' && c != '?' && c != '\"' && c != 0x3001 && c != 0x3002);
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
        return s + ImTextCountUtf8BytesFromChar(s, text_end);
    return s;
}

ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining)
{
    if (!text_end)
        text_end = text_begin + ImStrlen(text_begin); // FIXME-OPT: Need to avoid this.

    const float line_height = size;
    ImFontBaked* baked = GetFontBaked(size);
    const float scale = size / baked->Size;

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
                word_wrap_eol = CalcWordWrapPosition(size, s, text_end, wrap_width - line_width);

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

        // Optimized inline version of 'float char_width = GetCharAdvance((ImWchar)c);'
        float char_width = (c < (unsigned int)baked->IndexAdvanceX.Size) ? baked->IndexAdvanceX.Data[c] : -1.0f;
        if (char_width < 0.0f)
            char_width = BuildLoadGlyphGetAdvanceOrFallback(baked, c);
        char_width *= scale;

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
void ImFont::RenderChar(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, ImWchar c, const ImVec4* cpu_fine_clip)
{
    ImFontBaked* baked = GetFontBaked(size);
    const ImFontGlyph* glyph = baked->FindGlyph(c);
    if (!glyph || !glyph->Visible)
        return;
    if (glyph->Colored)
        col |= ~IM_COL32_A_MASK;
    float scale = (size >= 0.0f) ? (size / baked->Size) : 1.0f;
    float x = IM_TRUNC(pos.x);
    float y = IM_TRUNC(pos.y);

    float x1 = x + glyph->X0 * scale;
    float x2 = x + glyph->X1 * scale;
    if (cpu_fine_clip && (x1 > cpu_fine_clip->z || x2 < cpu_fine_clip->x))
        return;
    float y1 = y + glyph->Y0 * scale;
    float y2 = y + glyph->Y1 * scale;
    float u1 = glyph->U0;
    float v1 = glyph->V0;
    float u2 = glyph->U1;
    float v2 = glyph->V1;

    // Always CPU fine clip. Code extracted from RenderText().
    // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
    if (cpu_fine_clip != NULL)
    {
        if (x1 < cpu_fine_clip->x) { u1 = u1 + (1.0f - (x2 - cpu_fine_clip->x) / (x2 - x1)) * (u2 - u1); x1 = cpu_fine_clip->x; }
        if (y1 < cpu_fine_clip->y) { v1 = v1 + (1.0f - (y2 - cpu_fine_clip->y) / (y2 - y1)) * (v2 - v1); y1 = cpu_fine_clip->y; }
        if (x2 > cpu_fine_clip->z) { u2 = u1 + ((cpu_fine_clip->z - x1) / (x2 - x1)) * (u2 - u1); x2 = cpu_fine_clip->z; }
        if (y2 > cpu_fine_clip->w) { v2 = v1 + ((cpu_fine_clip->w - y1) / (y2 - y1)) * (v2 - v1); y2 = cpu_fine_clip->w; }
        if (y1 >= y2)
            return;
    }
    draw_list->PrimReserve(6, 4);
    draw_list->PrimRectUV(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(u1, v1), ImVec2(u2, v2), col);
}

// Note: as with every ImDrawList drawing function, this expects that the font atlas texture is bound.
void ImFont::RenderText(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width, bool cpu_fine_clip)
{
    // Align to be pixel perfect
begin:
    float x = IM_TRUNC(pos.x);
    float y = IM_TRUNC(pos.y);
    if (y > clip_rect.w)
        return;

    if (!text_end)
        text_end = text_begin + ImStrlen(text_begin); // ImGui:: functions generally already provides a valid text_end, so this is merely to handle direct calls.

    const float line_height = size;
    ImFontBaked* baked = GetFontBaked(size);

    const float scale = size / baked->Size;
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
                // FIXME-OPT: This is not optimal as do first do a search for \n before calling CalcWordWrapPosition().
                // If the specs for CalcWordWrapPosition() were reworked to optionally return on \n we could combine both.
                // However it is still better than nothing performing the fast-forward!
                s = CalcWordWrapPosition(size, s, line_end ? line_end : text_end, wrap_width);
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
    const int cmd_count = draw_list->CmdBuffer.Size;

    const ImU32 col_untinted = col | ~IM_COL32_A_MASK;
    const char* word_wrap_eol = NULL;

    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
                word_wrap_eol = CalcWordWrapPosition(size, s, text_end, wrap_width - (x - origin_x));

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

        const ImFontGlyph* glyph = baked->FindGlyph((ImWchar)c);
        //if (glyph == NULL)
        //    continue;

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

    // Edge case: calling RenderText() with unloaded glyphs triggering texture change. It doesn't happen via ImGui:: calls because CalcTextSize() is always used.
    if (cmd_count != draw_list->CmdBuffer.Size) //-V547
    {
        IM_ASSERT(draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ElemCount == 0);
        draw_list->CmdBuffer.pop_back();
        draw_list->PrimUnreserve(idx_count_max, vtx_count_max);
        draw_list->AddDrawCmd();
        //IMGUI_DEBUG_LOG("RenderText: cancel and retry to missing glyphs.\n"); // [DEBUG]
        //draw_list->AddRectFilled(pos, pos + ImVec2(10, 10), IM_COL32(255, 0, 0, 255)); // [DEBUG]
        goto begin;
        //RenderText(draw_list, size, pos, col, clip_rect, text_begin, text_end, wrap_width, cpu_fine_clip); // FIXME-OPT: Would a 'goto begin' be better for code-gen?
        //return;
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

// File: 'Roboto-Medium.ttf' (162588 bytes)
// Exported using binary_to_compressed_c.exe -u8 "Roboto-Medium.ttf" roboto
static const unsigned int roboto_compressed_size = 115741;
static const unsigned char roboto_compressed_data[115741] =
{
    87,188,0,0,0,0,0,0,0,2,123,28,0,4,0,0,37,0,1,0,0,0,17,130,4,8,46,4,0,16,71,80,79,83,125,170,113,140,0,2,8,168,0,0,89,12,71,83,85,66,76,156,40,224,0,2,97,180,0,0,25,104,79,83,47,50,
    161,11,177,182,0,0,1,152,130,53,60,96,99,109,97,112,64,38,72,114,0,0,26,108,0,0,18,200,99,118,116,32,4,151,43,74,0,0,47,188,130,31,44,86,102,112,103,109,123,249,97,171,0,0,45,52,130,
    51,40,188,103,97,115,112,0,8,0,19,130,95,32,156,130,31,8,40,12,103,108,121,102,174,158,98,233,0,0,57,220,0,1,203,204,104,100,109,120,61,63,60,32,0,0,21,128,0,0,4,236,104,101,97,100,
    248,123,171,8,130,59,32,28,130,47,33,54,104,130,16,35,10,239,10,155,130,15,32,84,130,15,40,36,104,109,116,120,36,243,68,245,130,15,59,248,0,0,19,136,108,111,99,97,221,222,102,173,0,
    0,48,20,0,0,9,198,109,97,120,112,7,18,2,131,31,32,120,130,47,43,32,110,97,109,101,61,99,111,76,0,2,5,130,223,41,2,212,112,111,115,116,255,109,0,100,130,143,32,124,131,31,43,112,114,
    101,112,27,177,248,54,0,0,46,240,130,15,34,204,0,1,130,5,44,2,0,0,17,64,164,109,95,15,60,245,0,27,130,138,38,0,0,0,196,240,17,46,130,6,44,0,208,219,78,148,250,36,253,213,9,92,8,115,
    130,15,35,9,0,2,0,133,0,38,1,0,0,7,108,254,12,130,131,38,107,250,36,254,65,9,92,132,73,133,27,132,5,33,4,226,131,17,130,5,38,143,0,22,0,78,0,5,134,31,33,14,0,130,66,35,2,22,0,6,130,
    15,36,3,4,149,1,244,130,25,36,0,5,154,5,51,130,225,32,31,133,7,38,3,209,0,102,2,0,0,136,106,45,0,0,224,0,10,255,80,0,33,127,0,0,0,33,130,3,56,0,71,79,79,71,0,64,0,0,255,253,6,0,254,
    0,0,102,7,154,2,0,32,0,1,159,131,27,35,4,58,5,176,130,12,36,32,0,2,3,140,130,247,131,18,130,3,32,1,130,38,131,3,39,2,37,0,143,2,152,0,101,130,151,33,96,4,130,31,8,43,5,224,0,99,5,29,
    0,86,1,90,0,82,2,202,0,128,2,210,0,40,3,137,0,27,4,117,0,68,1,194,0,28,2,160,0,71,2,60,0,135,3,42,0,2,130,47,32,105,130,3,32,168,130,3,32,81,130,3,32,79,130,3,32,52,130,3,32,129,130,
    3,32,117,130,3,32,69,130,3,32,104,130,3,8,46,93,2,31,0,130,1,231,0,46,4,17,0,63,4,122,0,145,4,42,0,128,3,228,0,60,7,40,0,91,5,83,0,18,5,12,0,148,5,57,0,102,5,58,0,148,4,134,130,3,32,
    101,130,15,48,114,0,106,5,175,0,148,2,66,0,163,4,113,0,45,5,11,130,23,36,84,0,148,7,1,130,27,32,174,130,3,32,134,130,47,32,29,132,7,34,96,4,254,130,27,36,212,0,74,4,219,130,39,62,55,
    0,125,5,45,0,18,7,10,0,48,5,16,0,41,4,224,0,7,4,209,0,80,2,49,0,132,3,88,0,20,130,7,58,12,3,107,0,53,3,156,0,3,2,148,0,49,4,84,0,90,4,129,0,124,4,48,0,79,4,132,130,3,42,75,0,83,2,214,
    0,45,4,137,0,82,130,119,42,121,2,11,0,125,2,1,255,181,4,45,130,7,36,11,0,140,6,246,130,43,36,115,0,121,4,142,130,43,131,55,36,139,0,79,2,208,130,19,58,33,0,75,2,169,0,8,4,114,0,119,
    3,245,0,22,5,242,0,33,4,6,0,31,3,229,0,12,130,7,40,82,2,175,0,56,2,2,0,174,130,7,52,27,5,81,0,117,2,30,0,134,4,125,0,100,4,181,0,94,5,157,0,93,130,167,60,25,1,252,0,136,4,248,0,90,
    3,133,0,93,6,68,0,87,3,145,0,141,3,226,0,87,4,109,0,127,132,15,46,219,0,135,3,10,0,127,4,74,0,95,2,246,0,60,130,3,52,55,2,155,0,112,4,187,0,146,3,237,0,69,2,66,0,142,2,16,0,109,130,
    23,34,128,3,167,130,135,8,52,226,0,93,5,208,0,89,6,43,0,80,6,87,0,103,3,228,0,66,7,133,255,246,4,68,0,77,5,132,0,105,4,202,0,148,4,231,0,136,6,193,0,72,4,167,0,103,4,145,0,67,4,136,
    130,215,58,151,0,130,5,176,0,31,2,26,0,143,4,152,0,142,4,100,0,34,2,79,0,33,5,147,0,144,130,31,8,54,126,7,180,0,100,7,58,0,91,2,12,0,139,5,136,0,81,2,208,255,228,5,138,0,88,4,158,0,
    79,5,164,0,125,4,242,0,119,2,38,255,181,4,60,0,89,3,230,0,148,3,176,0,114,3,220,130,187,32,124,130,247,62,11,0,129,2,178,0,120,2,77,0,41,3,216,0,122,3,31,0,73,2,108,0,130,0,0,252,142,
    0,0,253,94,130,7,32,115,130,7,32,62,130,7,32,12,130,7,36,28,2,93,0,198,130,71,32,103,131,219,55,4,117,0,155,5,191,0,25,5,122,0,91,5,56,0,32,4,144,0,108,5,177,0,155,130,7,54,71,5,239,
    0,74,5,170,0,68,5,91,0,107,4,132,0,86,4,198,0,150,4,14,130,35,42,136,0,84,4,96,0,96,4,26,0,97,131,187,51,4,161,0,115,2,170,0,169,4,106,0,22,4,19,0,100,4,243,0,45,130,23,38,128,4,55,
    0,82,4,144,130,3,34,45,0,63,130,47,34,128,5,208,130,75,8,38,201,0,79,6,148,0,102,4,179,0,118,4,123,255,225,6,113,0,51,5,254,0,34,5,89,0,104,8,136,0,45,8,143,0,155,6,91,0,49,130,119,
    42,146,5,8,0,144,6,6,0,36,7,162,130,87,8,40,214,0,73,5,168,0,148,5,169,0,45,5,10,0,57,6,95,0,79,5,249,0,146,5,137,0,142,7,155,0,152,7,249,0,152,6,26,0,24,6,249,130,207,8,52,7,0,144,
    5,80,0,107,7,84,0,160,4,247,0,32,4,125,0,91,4,143,0,143,3,90,0,133,4,246,0,39,6,118,0,30,4,22,0,77,4,152,0,134,4,110,0,143,4,154,0,33,6,3,130,7,32,151,130,15,130,19,37,3,245,0,35,5,
    211,130,227,32,211,130,15,48,102,0,95,6,142,0,134,6,236,0,126,5,23,0,31,6,111,130,39,32,104,130,3,56,60,0,81,6,132,0,145,4,112,0,39,4,113,255,219,4,60,0,84,6,209,0,30,6,228,130,51,
    34,137,255,238,131,91,45,7,73,0,136,6,79,0,112,4,103,255,224,7,40,130,159,62,1,0,134,5,12,0,28,4,96,0,10,7,66,0,172,6,54,0,157,6,237,0,128,5,230,0,130,9,50,0,163,130,195,46,143,4,32,
    0,40,3,240,0,51,5,122,0,95,4,136,130,227,38,26,0,16,4,14,0,32,135,15,33,7,69,130,83,34,68,0,116,135,91,47,5,26,0,102,4,74,0,92,4,255,0,109,0,0,252,102,65,235,6,40,123,0,0,253,165,0,
    0,250,36,130,3,32,77,131,127,51,5,19,0,148,4,134,0,124,4,106,0,143,3,161,0,126,4,183,0,155,130,107,8,36,126,5,44,0,144,4,171,0,142,6,149,0,52,5,164,0,61,7,208,0,148,5,170,0,126,8,71,
    0,155,6,245,0,126,6,42,0,103,130,91,62,97,7,49,0,45,5,112,0,38,5,116,0,128,4,115,0,116,5,135,0,133,6,36,0,22,4,195,255,203,5,33,130,67,36,120,0,142,5,175,130,83,42,136,0,126,5,136,
    0,81,4,166,0,91,130,3,38,93,4,199,0,52,3,83,130,59,56,7,0,82,6,241,0,104,6,221,0,94,6,83,0,60,5,40,0,47,4,123,0,72,4,62,130,203,8,36,190,0,66,6,157,0,64,7,253,0,148,6,158,0,119,5,4,
    0,93,4,44,0,85,5,170,0,33,5,29,0,68,5,85,0,129,3,44,130,135,39,20,0,0,8,41,0,0,4,134,7,41,2,185,0,0,2,10,0,0,1,92,130,19,36,127,0,0,2,48,130,11,36,162,0,0,0,209,130,3,130,2,35,2,161,
    0,71,131,3,37,5,41,0,157,6,48,130,67,8,116,157,0,4,1,192,0,99,1,188,0,51,1,206,0,50,1,168,0,74,3,20,0,108,3,27,0,64,3,8,0,50,4,93,0,64,4,153,0,92,2,203,0,136,3,250,0,138,5,166,0,138,
    1,108,0,71,7,167,0,74,2,114,0,108,2,105,0,84,3,156,0,45,2,246,0,53,3,92,0,105,4,181,0,95,6,112,0,33,6,184,0,152,8,147,0,148,7,136,0,53,6,140,0,124,4,140,0,94,5,245,0,33,4,52,0,40,4,
    162,130,195,8,52,94,0,79,5,125,0,40,5,228,0,112,3,226,0,76,8,46,0,144,5,9,0,109,5,20,0,150,6,53,0,89,6,221,0,84,6,209,0,91,6,162,0,88,4,145,0,98,5,150,0,166,4,217,130,139,8,40,131,
    0,158,4,178,0,59,8,69,0,94,2,45,255,175,4,142,0,101,4,122,0,145,4,17,0,60,4,42,0,128,4,12,0,36,2,91,0,161,2,152,130,211,48,241,0,69,5,27,0,45,4,168,0,24,4,188,0,45,7,35,133,3,40,5,
    17,0,45,6,183,0,75,0,130,0,59,8,48,0,89,8,53,0,92,4,51,0,58,4,147,0,79,2,16,255,176,1,179,0,92,3,161,0,117,135,3,35,4,11,0,117,133,3,33,255,76,130,7,32,122,131,23,47,2,5,0,148,4,158,
    0,9,4,96,0,118,4,128,0,79,130,131,38,118,3,224,0,118,3,197,130,15,48,166,0,84,4,222,0,118,1,252,0,133,3,213,0,36,4,91,130,23,36,185,0,118,6,6,130,27,32,221,130,3,32,192,130,47,32,109,
    132,7,34,76,4,92,130,7,36,52,0,62,4,59,130,39,50,132,0,103,4,123,0,9,6,7,0,40,4,94,0,21,4,60,0,5,130,211,36,65,2,246,0,75,130,3,32,128,69,139,8,34,246,0,53,130,11,32,79,130,3,32,77,
    130,3,32,54,134,31,40,70,3,185,0,144,2,178,0,150,130,75,50,10,4,187,0,86,5,68,0,155,5,40,0,155,4,48,0,129,5,57,130,7,35,45,0,129,4,131,107,38,102,0,56,4,77,0,14,130,47,32,118,131,111,
    32,4,131,143,130,119,37,3,152,0,66,4,216,130,143,48,25,0,68,5,157,0,80,5,84,0,80,4,228,0,95,5,145,130,155,130,227,41,7,84,0,36,7,87,0,118,5,151,130,15,32,215,130,39,32,113,130,11,40,
    89,0,39,6,58,0,26,4,70,130,59,32,228,130,19,32,92,130,3,32,203,130,31,36,70,0,31,5,93,130,11,40,140,0,65,6,132,0,118,7,10,130,43,36,90,0,10,6,32,130,19,32,103,130,3,36,128,0,60,6,146,
    130,7,36,136,0,67,4,34,130,23,32,146,130,67,32,157,130,35,32,26,130,3,48,110,0,36,5,240,0,79,4,90,0,5,4,196,0,21,6,149,130,79,130,71,33,4,140,130,27,36,254,0,10,4,210,130,27,131,111,
    131,183,130,203,37,3,247,0,70,8,54,130,75,40,235,0,40,4,136,0,124,4,61,130,183,44,152,0,79,3,164,0,91,4,161,0,76,4,148,130,19,32,159,130,51,44,75,0,83,4,137,0,81,5,122,0,107,5,162,
    130,3,36,134,0,155,5,224,130,7,44,226,0,107,4,27,0,151,4,130,0,110,3,185,130,71,38,87,0,15,4,190,0,53,65,87,6,65,107,20,40,4,107,0,102,4,46,0,67,6,130,103,43,4,180,0,115,4,235,0,98,
    2,38,255,181,132,3,39,27,0,143,2,27,255,251,2,130,7,47,4,96,0,118,1,254,0,0,2,160,0,71,5,88,255,247,131,3,47,4,143,255,212,4,219,0,45,2,169,255,232,5,83,0,18,152,3,32,57,130,99,35,
    134,0,148,4,138,3,39,2,66,255,200,2,66,0,163,130,7,32,203,130,3,40,191,5,174,0,148,5,134,0,102,144,3,35,55,0,125,5,138,3,39,4,224,0,7,4,84,0,90,152,3,32,48,65,31,6,35,75,0,83,4,134,
    3,37,2,26,255,180,2,26,130,203,34,26,255,183,130,11,33,171,4,72,143,7,32,142,130,43,139,3,35,114,0,119,4,138,3,35,3,229,0,12,131,3,131,215,131,111,144,7,131,215,130,111,32,5,151,7,
    32,58,130,211,32,26,130,99,131,247,131,139,158,7,39,5,114,0,106,4,137,0,82,152,7,42,175,0,148,4,113,0,121,2,66,255,179,130,199,32,159,130,7,32,185,130,7,32,165,130,7,32,223,130,7,52,
    203,2,66,0,23,2,11,0,0,2,66,0,157,6,179,0,163,4,12,0,125,130,47,38,45,2,38,255,181,5,11,130,59,32,45,130,15,34,84,0,148,130,35,32,138,130,207,131,7,32,85,132,7,34,161,0,140,132,7,36,
    231,0,140,5,174,130,39,34,115,0,121,143,7,43,4,115,255,165,5,134,0,102,4,142,0,79,143,7,33,4,254,130,83,34,208,0,124,134,7,32,79,134,7,40,56,4,212,0,74,4,33,0,75,160,7,38,219,0,45,
    2,169,0,8,66,75,5,134,7,36,209,0,8,5,55,130,175,34,114,0,119,167,7,47,7,10,0,48,5,242,0,33,4,224,0,7,3,229,0,12,131,7,39,4,209,0,80,4,6,0,82,143,7,51,7,133,255,246,6,193,0,72,5,132,
    0,105,4,136,0,79,4,122,255,166,132,3,38,59,0,36,4,158,0,9,152,3,37,128,0,79,3,224,0,69,35,5,134,7,39,1,252,255,166,1,252,0,131,130,7,32,169,130,3,33,157,4,69,27,7,32,192,130,87,139,
    3,35,132,0,103,4,139,3,34,60,0,5,140,103,130,91,32,4,139,3,34,122,0,106,143,107,131,15,35,4,166,0,84,140,3,34,222,0,118,130,119,32,145,130,3,32,151,130,3,32,189,130,139,32,21,130,3,
    32,124,69,179,11,33,3,185,69,187,5,131,7,132,159,131,163,143,167,32,92,68,247,6,131,7,35,52,0,62,4,139,3,35,59,0,36,4,135,3,143,199,134,15,36,6,7,0,40,4,131,211,69,239,6,35,4,42,0,
    65,131,3,63,5,83,0,18,4,234,255,74,6,19,255,83,2,166,255,86,5,154,255,167,5,68,254,225,5,111,255,178,2,170,255,135,131,31,33,5,12,68,31,6,34,209,0,80,76,199,7,33,5,11,76,191,17,47,
    4,219,0,45,4,224,0,7,5,16,0,41,2,66,255,191,131,11,47,4,132,0,86,4,96,0,96,4,136,0,126,2,170,0,169,130,11,52,128,4,152,0,142,4,142,0,79,4,187,0,146,3,245,0,22,4,6,0,31,130,107,32,204,
    132,27,131,23,36,96,0,128,6,148,68,151,6,38,117,0,155,4,212,0,74,68,143,6,40,191,4,113,0,45,5,40,0,155,131,127,35,5,10,0,57,136,159,131,39,36,134,0,148,5,168,134,151,32,175,68,179,
    6,32,177,130,43,32,29,130,11,32,57,130,79,130,159,131,155,39,4,84,0,90,4,75,0,83,130,135,32,134,132,135,77,51,6,32,3,76,223,5,32,31,131,27,55,3,90,0,133,4,33,0,75,2,11,0,125,2,26,255,
    171,2,1,255,181,4,110,0,143,131,35,66,203,7,143,7,66,219,7,59,1,90,0,82,2,152,0,101,4,74,0,143,2,38,255,177,1,188,0,51,7,1,0,148,6,246,0,124,131,175,132,127,134,171,135,135,37,5,170,
    0,68,5,201,74,119,7,39,255,241,8,115,0,79,9,107,130,179,38,214,0,73,4,22,0,77,68,199,7,131,99,47,4,14,0,32,2,66,0,163,7,162,0,22,6,118,0,30,131,11,68,255,15,67,67,7,32,4,68,215,6,39,
    5,136,0,81,4,60,0,89,131,3,135,55,32,4,135,91,39,168,0,148,4,152,0,134,5,135,7,68,75,7,75,11,7,135,7,34,80,0,107,130,63,40,81,5,10,0,57,3,229,0,12,144,7,56,137,0,142,4,102,0,95,6,249,
    0,155,6,111,0,143,5,16,0,41,4,6,0,31,4,132,130,215,38,169,0,45,4,154,0,33,143,167,138,15,36,16,4,84,255,154,154,31,32,18,69,231,20,33,83,0,140,23,135,239,153,7,37,255,213,4,75,255,
    142,151,31,39,2,66,0,163,2,26,0,143,130,7,36,148,2,11,0,120,69,79,15,138,15,36,39,4,142,255,163,152,31,78,67,7,159,7,69,71,15,78,115,6,78,123,7,151,7,69,71,11,32,3,69,79,6,132,7,35,
    162,0,79,4,130,3,55,5,40,0,155,4,110,0,143,5,175,0,148,4,151,0,134,4,219,0,45,3,245,0,35,65,147,7,32,5,65,171,6,135,7,39,4,117,0,155,3,90,0,133,66,27,7,76,99,7,39,4,113,0,121,5,7,255,
    208,131,3,47,4,117,255,240,3,90,255,226,5,60,255,227,4,68,255,174,66,47,8,134,99,37,7,1,0,148,6,3,130,115,65,235,6,66,171,7,135,115,43,4,96,0,96,4,101,0,2,6,48,0,129,81,103,16,38,160,
    0,93,4,180,0,125,71,95,8,70,235,7,65,223,5,48,13,4,134,0,72,4,75,0,1,2,66,254,246,2,26,254,226,65,119,6,54,22,4,254,0,50,2,208,255,110,5,55,0,113,4,114,0,15,4,223,254,172,5,12,130,
    243,36,129,0,124,5,58,130,7,34,132,0,79,136,7,71,163,6,71,111,7,71,119,14,32,120,67,163,8,135,123,32,29,133,71,71,51,6,32,114,71,43,8,71,19,6,39,5,45,0,18,3,245,0,22,135,7,70,227,8,
    70,215,6,63,5,204,254,28,4,158,0,9,4,28,255,42,5,26,255,55,2,56,255,57,4,202,255,147,4,120,254,232,4,238,255,164,132,27,32,96,70,175,5,35,4,42,0,65,75,207,7,33,4,91,75,199,6,75,195,
    7,46,59,0,36,4,60,0,5,4,94,0,21,1,252,255,157,131,11,33,3,224,70,51,5,39,4,52,0,62,1,252,0,133,131,23,70,83,7,35,4,70,0,31,136,91,32,185,134,95,32,228,134,83,35,222,0,118,4,131,87,
    32,216,130,7,32,109,130,3,35,128,0,79,4,131,95,130,91,130,55,32,66,131,131,32,4,131,19,130,111,36,5,254,0,10,4,130,55,130,23,36,31,5,157,0,80,68,187,11,8,33,4,75,0,83,2,26,0,120,0,
    0,0,1,0,0,4,228,9,11,4,0,0,2,2,2,3,6,5,7,6,2,3,3,4,5,132,4,32,5,135,0,33,2,2,130,9,34,4,8,6,130,0,39,5,5,6,6,3,5,6,5,132,12,32,6,131,13,32,8,130,5,37,2,4,2,4,4,3,132,46,130,5,36,2,
    2,5,2,8,131,12,44,3,5,3,5,4,7,5,4,5,3,2,3,6,130,67,42,6,5,2,6,4,7,4,4,5,7,4,130,26,32,3,130,28,130,24,38,4,4,7,7,7,4,8,130,83,32,6,132,52,131,40,130,136,35,9,8,2,6,130,6,32,6,130,14,
    32,4,130,0,131,144,33,4,3,130,176,130,2,32,3,130,86,34,6,6,6,130,49,32,7,130,5,132,117,32,5,130,18,130,15,131,11,130,77,38,5,5,7,7,6,10,10,130,29,34,7,9,5,130,41,130,13,34,9,9,7,130,
    178,32,8,130,171,34,4,6,7,132,38,32,5,131,154,32,5,130,21,34,7,5,5,130,2,38,5,8,8,5,5,8,7,130,2,130,220,39,7,8,7,10,9,5,4,6,130,82,133,13,130,9,130,120,130,2,33,5,6,130,55,130,246,
    42,5,7,6,9,6,9,8,7,6,8,6,130,27,32,7,132,40,130,26,130,51,32,8,130,20,37,5,5,9,7,9,7,130,24,52,6,6,4,5,9,5,9,3,2,2,5,2,2,1,0,3,3,6,7,4,2,130,0,34,3,4,3,130,178,36,4,6,2,9,3,130,10,
    36,4,5,7,8,10,130,123,35,7,5,5,6,130,32,32,9,130,4,32,8,130,14,131,79,34,9,2,5,131,0,34,3,3,2,130,13,39,8,8,6,8,0,9,9,5,130,75,34,4,4,4,131,23,32,4,132,30,131,11,33,2,4,132,207,132,
    45,133,208,32,3,136,0,131,108,32,6,130,141,132,49,130,4,130,58,32,6,130,0,32,5,130,79,37,5,5,6,7,5,6,130,19,32,6,65,10,9,130,16,130,20,33,5,7,133,70,34,5,4,9,132,216,32,5,131,0,130,
    53,36,7,7,5,5,4,130,188,133,93,131,35,32,6,130,208,131,224,32,3,130,99,32,5,130,4,130,36,130,2,133,182,130,241,133,14,134,17,66,83,10,130,50,136,13,32,4,65,107,6,65,113,5,130,5,32,
    6,136,26,65,90,6,130,19,34,5,3,2,134,1,35,8,5,5,2,130,33,36,2,5,2,5,3,130,113,132,34,132,4,35,6,5,6,3,131,1,137,61,131,32,135,34,133,93,33,8,7,131,191,132,33,33,8,8,138,103,34,5,5,
    4,130,0,131,203,138,61,135,10,130,25,33,4,4,132,12,132,236,32,4,132,43,146,39,130,18,32,7,132,22,37,6,6,7,3,6,6,130,139,33,6,5,131,147,32,8,132,255,33,6,3,131,27,132,4,32,4,130,152,
    40,5,5,7,5,5,5,3,3,5,130,28,32,6,131,39,67,85,6,134,148,33,4,5,130,168,130,110,35,5,4,8,7,131,1,38,5,4,2,3,5,2,2,65,181,5,130,85,40,7,6,5,10,11,5,5,6,5,130,67,32,9,130,109,130,9,33,
    8,8,132,15,33,9,7,131,6,65,76,8,34,5,6,4,132,1,34,5,8,7,130,44,138,22,65,91,10,66,30,6,140,210,35,3,2,3,2,153,45,130,25,65,218,9,32,4,130,170,32,4,133,116,67,55,7,36,5,4,9,7,7,130,
    18,32,6,135,16,132,125,132,174,65,34,5,67,52,8,132,101,32,3,142,154,32,5,132,232,130,100,33,3,5,130,223,131,191,32,8,66,145,5,32,6,130,16,67,133,5,33,2,5,133,72,40,2,5,4,4,5,2,2,4,
    5,130,0,33,4,4,67,223,5,134,200,36,7,6,5,6,6,130,11,36,2,0,0,0,3,134,3,36,28,0,3,0,1,130,11,131,7,40,10,0,0,6,136,0,4,6,108,130,15,38,234,0,128,0,6,0,106,130,9,8,231,2,0,13,0,126,0,
    160,0,172,0,173,0,191,0,198,0,207,0,230,0,239,0,254,1,15,1,17,1,37,1,39,1,48,1,83,1,95,1,103,1,126,1,127,1,143,1,146,1,161,1,176,1,240,1,255,2,27,2,55,2,89,2,188,2,199,2,201,2,221,
    2,243,3,1,3,3,3,9,3,15,3,35,3,138,3,140,3,146,3,161,3,176,3,185,3,201,3,206,3,210,3,214,4,37,4,47,4,69,4,79,4,98,4,111,4,121,4,134,4,206,4,215,4,225,4,245,5,1,5,16,5,19,30,1,30,63,
    30,133,30,241,30,243,30,249,31,77,32,11,32,17,32,21,32,30,32,34,32,39,32,48,32,51,32,58,32,60,32,68,32,116,32,127,32,164,32,170,32,172,32,177,32,186,32,189,33,5,33,19,33,22,33,34,33,
    38,33,46,33,94,34,2,34,6,34,15,34,18,34,26,34,30,34,43,34,72,34,96,34,101,37,202,238,2,246,195,251,4,254,255,255,253,255,255,0,131,0,131,235,32,32,130,235,32,161,130,235,60,174,0,192,
    0,199,0,208,0,231,0,240,0,255,1,16,1,18,1,38,1,40,1,49,1,84,1,96,1,104,134,235,34,160,1,175,130,235,34,250,2,24,134,235,32,198,130,235,32,216,130,235,32,0,136,235,32,132,130,235,44,
    142,3,147,3,163,3,177,3,186,3,202,3,209,130,235,8,40,0,4,38,4,48,4,70,4,80,4,99,4,112,4,122,4,136,4,207,4,216,4,226,4,246,5,2,5,17,30,0,30,62,30,128,30,160,30,242,30,244,130,235,42,
    0,32,16,32,19,32,23,32,32,32,37,130,235,34,50,32,57,136,235,36,163,32,166,32,171,130,235,34,185,32,188,140,235,32,91,134,235,32,17,138,235,32,100,130,235,32,1,130,235,32,1,130,235,
    32,252,130,235,50,1,0,0,255,246,255,228,1,164,255,194,1,152,255,193,0,0,1,139,130,3,32,134,130,3,32,130,130,3,32,128,130,3,32,126,130,3,32,118,130,3,45,120,255,21,255,6,255,4,254,247,
    254,234,1,186,0,130,0,55,254,100,254,67,0,239,253,215,253,214,253,200,253,179,253,167,253,166,253,161,253,156,253,137,130,81,34,202,255,201,130,32,34,0,253,9,130,11,40,170,252,253,
    252,250,0,0,252,185,130,3,32,177,130,3,32,166,130,3,36,160,0,0,254,244,130,3,32,241,130,11,8,46,73,0,0,229,174,229,110,229,31,229,78,228,179,229,76,229,92,225,91,225,87,0,0,225,84,
    225,83,225,81,225,73,227,117,225,65,227,109,225,56,225,9,224,255,0,0,224,218,130,3,8,52,213,224,206,224,205,224,134,224,121,224,119,224,108,223,147,224,97,224,53,223,146,222,171,223,
    134,223,133,223,126,223,123,223,111,223,83,223,60,223,57,219,213,19,159,10,223,6,163,2,171,1,175,0,1,131,143,140,3,130,73,33,0,228,130,215,32,14,130,3,32,40,138,3,32,106,140,37,36,
    0,1,106,1,116,141,17,137,13,33,1,98,131,11,130,33,36,134,0,0,1,158,131,11,130,7,32,182,130,3,36,254,0,0,2,38,130,3,32,72,130,3,32,88,130,3,32,226,130,3,36,242,0,0,3,6,133,35,139,5,
    33,2,248,139,13,135,11,36,2,232,0,0,2,130,3,135,15,171,7,10,182,2,75,2,76,2,77,2,78,2,79,2,80,0,129,2,71,2,91,2,92,2,93,2,94,2,95,2,96,0,130,0,131,2,97,2,98,2,99,2,100,2,101,0,132,
    0,133,2,102,2,103,2,104,2,105,2,106,2,107,0,134,0,135,2,118,2,119,2,120,2,121,2,122,2,123,0,136,0,137,2,124,2,125,2,126,2,127,2,128,0,138,2,70,4,70,0,139,2,72,0,140,2,175,2,176,2,177,
    2,178,2,179,2,180,0,141,2,181,2,182,2,183,2,184,2,185,2,186,2,187,2,188,0,142,0,143,2,189,2,190,2,191,2,192,2,193,2,194,2,195,0,144,0,145,2,196,2,197,2,198,2,199,2,200,2,201,0,146,
    0,147,2,216,2,217,2,220,2,221,2,222,2,223,2,73,2,74,2,81,2,108,2,247,2,248,2,249,2,250,2,214,2,215,2,218,2,219,0,173,0,174,3,82,0,175,3,83,3,84,3,85,0,176,0,177,3,92,3,93,3,94,0,178,
    3,95,3,96,0,179,3,97,3,98,0,180,3,99,0,181,3,100,0,182,3,101,3,102,0,183,3,103,0,184,0,185,3,104,3,105,3,106,3,107,3,108,3,109,3,110,3,111,0,195,3,113,3,114,0,196,3,112,0,197,0,198,
    0,199,0,200,0,201,0,202,0,203,3,115,0,204,0,205,3,176,3,121,0,209,3,122,0,210,3,123,3,124,3,125,3,126,0,211,0,212,0,213,3,128,3,177,3,129,0,214,3,130,0,215,3,131,3,132,0,216,3,133,
    0,217,0,218,0,219,3,134,3,127,0,220,3,135,3,136,3,137,3,138,3,139,3,140,3,141,0,221,0,222,3,142,3,143,0,233,0,234,0,235,0,236,3,144,0,237,0,238,0,239,3,145,0,240,0,241,0,242,0,243,
    3,146,0,244,3,147,3,148,0,245,3,149,0,246,3,150,3,178,3,151,1,1,3,152,1,2,3,153,3,154,3,155,3,156,1,3,1,4,1,5,3,157,3,179,3,158,1,6,1,7,1,8,4,92,3,180,3,181,1,22,1,23,1,24,1,25,3,182,
    3,183,3,185,3,184,1,39,1,40,4,97,4,98,4,91,1,41,1,42,1,43,1,44,1,45,4,93,4,94,1,46,1,47,4,86,4,87,3,186,3,187,4,72,4,73,1,48,1,49,4,95,4,96,1,50,1,51,4,74,4,75,1,52,1,53,1,54,1,55,
    1,56,1,57,3,188,3,189,4,76,4,77,3,190,3,191,4,105,4,106,4,78,4,79,1,58,1,59,4,80,4,81,1,60,1,61,1,62,4,90,1,63,1,64,4,88,4,89,3,192,3,193,3,194,1,65,1,66,4,103,4,104,1,67,1,68,4,99,
    4,100,4,82,4,83,4,101,4,102,1,69,3,205,3,204,3,206,3,207,3,208,3,209,3,210,1,70,1,71,4,84,4,85,3,231,3,232,1,72,1,73,3,233,3,234,4,107,4,108,1,74,3,235,4,109,3,236,3,237,1,105,1,106,
    4,111,4,110,1,127,4,71,1,133,0,12,0,130,0,33,12,64,130,4,130,2,33,1,4,130,4,135,2,32,1,130,8,95,94,7,131,7,36,13,0,0,0,13,130,3,32,3,130,3,32,32,130,3,32,126,130,3,32,4,130,3,32,160,
    133,3,33,2,68,130,7,32,161,130,3,32,172,130,3,32,99,130,3,32,173,133,3,33,2,69,130,7,32,174,130,3,32,191,130,3,32,111,130,3,32,192,130,3,36,197,0,0,2,75,130,7,32,198,134,3,32,129,130,
    7,32,199,130,3,32,207,130,23,32,82,130,7,32,208,133,3,33,2,71,130,7,32,209,130,3,32,214,130,23,32,91,130,7,32,215,130,3,32,216,130,3,32,130,130,3,32,217,130,3,32,221,130,23,32,97,130,
    7,32,222,130,3,32,223,130,3,32,132,130,3,32,224,130,3,32,229,130,23,32,102,130,7,32,230,134,3,32,134,130,7,32,231,130,3,32,239,130,23,32,109,130,7,32,240,134,3,32,135,130,7,32,241,
    130,3,32,246,130,23,32,118,130,7,32,247,130,3,32,248,130,3,32,136,130,3,32,249,130,3,32,253,130,23,32,124,130,7,32,254,134,3,32,138,130,7,36,255,0,0,1,15,130,23,130,179,33,1,16,130,
    11,130,3,33,2,70,130,7,32,17,133,3,32,4,131,11,32,18,130,11,32,37,130,35,32,146,130,7,32,38,133,3,33,0,139,130,7,32,39,133,3,33,2,72,68,247,6,32,48,130,35,32,166,130,11,32,49,133,3,
    33,0,140,130,7,32,50,130,3,32,55,130,23,32,175,130,7,32,56,133,3,33,0,141,130,7,32,57,130,3,32,64,130,23,32,181,130,7,32,65,130,3,32,66,130,139,32,142,130,7,32,67,130,3,32,73,130,23,
    32,189,130,7,32,74,130,3,32,75,130,23,32,144,130,7,32,76,130,3,32,81,130,23,32,196,130,7,32,82,130,3,32,83,130,23,32,146,130,7,32,84,130,3,32,95,130,23,32,202,130,7,32,96,130,3,32,
    97,130,11,32,216,130,7,32,98,130,3,32,101,130,11,32,220,130,7,32,102,130,3,32,103,130,11,32,73,130,7,32,104,130,3,32,126,130,11,32,224,130,7,32,127,133,3,33,0,148,130,7,32,143,133,
    3,33,0,149,130,7,131,87,130,3,33,0,150,130,11,32,160,130,3,32,161,130,107,32,151,130,7,131,195,32,176,130,11,32,153,130,11,32,240,133,3,33,3,170,130,7,32,250,133,3,33,2,81,130,7,32,
    251,133,3,33,2,108,130,7,32,252,130,3,32,255,130,107,32,247,130,3,32,24,130,3,32,25,130,3,32,214,130,3,32,26,130,3,32,27,130,3,32,218,130,3,32,55,133,3,33,0,155,130,7,32,89,133,3,33,
    0,156,130,7,32,188,133,3,33,3,171,130,7,32,198,130,3,32,199,130,119,32,157,130,7,32,201,133,3,33,0,159,133,231,33,2,221,66,175,6,32,243,130,15,130,3,36,0,166,0,0,3,130,18,33,3,1,130,
    4,32,167,130,11,32,3,133,3,33,0,169,130,7,32,9,133,3,32,0,130,179,33,3,15,130,11,130,3,33,0,171,130,7,32,35,133,3,33,0,172,130,7,32,132,130,3,32,133,66,235,5,33,3,134,130,11,131,3,
    32,82,130,7,32,135,133,3,33,0,175,130,7,32,136,130,3,32,138,130,3,32,83,130,3,32,140,134,3,32,86,130,7,32,142,130,3,32,146,130,3,32,87,130,3,32,147,130,3,32,148,130,71,32,176,130,7,
    32,149,130,3,32,151,130,3,32,92,130,3,32,152,133,3,33,0,178,130,7,32,153,130,3,32,154,130,3,32,95,130,3,32,155,133,3,33,0,179,130,7,130,255,33,3,157,130,7,32,97,130,3,32,158,133,3,
    33,0,180,130,7,32,159,134,3,32,99,130,7,32,160,133,3,33,0,181,130,7,32,161,134,3,32,100,130,7,32,163,133,3,33,0,182,130,7,32,164,130,3,32,165,130,3,32,101,130,3,32,166,133,3,33,0,183,
    130,7,32,167,134,3,32,103,130,7,32,168,130,3,32,169,130,167,32,184,130,7,32,170,130,3,32,176,130,3,32,104,130,3,32,177,130,3,32,185,130,23,32,186,130,7,135,3,32,111,130,11,32,187,133,
    3,33,0,195,130,7,32,188,130,3,32,189,130,3,32,113,130,3,32,190,133,3,33,0,196,130,7,32,191,134,3,32,112,130,7,32,192,130,3,32,198,68,35,5,33,3,199,130,11,131,3,32,115,130,7,32,200,
    130,3,32,201,130,23,32,204,130,7,32,202,130,3,32,206,130,3,32,116,130,3,32,209,130,3,32,210,130,23,131,15,32,214,130,11,130,3,36,0,208,0,0,4,130,18,131,3,32,3,130,159,33,4,1,130,15,
    130,3,33,3,121,130,7,32,2,133,3,32,0,130,55,33,4,3,130,11,130,3,33,3,122,130,7,130,44,32,4,130,3,33,0,210,130,11,32,5,130,3,32,8,130,75,32,123,130,7,32,9,130,3,36,11,0,0,0,211,130,
    7,32,12,133,3,33,3,128,130,7,32,13,133,3,32,3,130,247,33,4,14,130,11,130,3,33,3,129,130,7,32,15,133,3,32,0,130,135,45,4,16,0,0,4,16,0,0,3,130,0,0,4,17,133,3,33,0,215,130,7,36,18,0,
    0,4,19,130,23,32,131,130,7,32,20,133,3,33,0,216,130,7,32,21,133,3,33,3,133,130,7,32,22,130,3,32,24,68,231,5,33,4,25,130,11,130,3,33,3,134,130,7,32,26,133,3,33,3,127,130,7,32,27,133,
    3,33,0,220,130,7,32,28,130,3,32,34,66,115,5,33,4,35,130,11,32,36,130,59,32,221,130,7,32,37,133,3,33,3,142,130,7,32,38,130,3,32,47,130,23,32,223,130,7,32,48,133,3,33,3,143,130,7,32,
    49,130,3,32,52,130,23,32,233,130,7,32,53,133,3,33,3,144,130,7,32,54,130,3,32,56,130,23,32,237,130,7,32,57,133,3,33,3,145,130,7,32,58,130,3,32,61,69,67,5,33,4,62,130,11,130,3,33,3,146,
    130,7,32,63,133,3,33,0,244,130,7,32,64,130,3,32,65,130,143,32,147,130,7,32,66,133,3,33,0,245,130,7,32,67,133,3,33,3,149,130,7,32,68,133,3,33,0,246,130,7,32,69,133,3,33,3,150,130,7,
    32,70,130,3,32,79,130,95,32,247,130,7,32,80,133,3,33,3,178,130,7,32,81,133,3,33,3,151,130,7,32,82,133,3,33,1,1,130,7,32,83,133,3,33,3,152,130,7,32,84,133,3,33,1,2,130,7,32,85,130,3,
    32,88,130,131,32,153,130,7,32,89,130,3,36,91,0,0,1,3,130,7,32,92,133,3,33,3,157,130,7,32,93,133,3,33,3,179,130,7,32,94,133,3,33,3,158,130,7,32,95,130,3,32,97,130,47,32,6,130,7,32,98,
    134,3,32,92,130,7,32,99,130,3,36,111,0,0,1,9,130,7,32,112,130,3,32,113,130,95,32,180,130,7,32,114,130,3,32,117,130,23,32,22,130,7,32,118,130,3,32,119,130,23,32,182,130,7,32,120,133,
    3,33,3,185,130,7,32,121,133,3,33,3,184,130,7,32,122,130,3,32,134,130,47,32,26,130,7,32,136,130,3,32,137,130,11,32,39,130,7,32,138,130,3,32,139,133,123,33,4,140,130,7,131,3,130,183,
    33,4,141,130,11,32,145,130,35,32,41,130,7,32,146,130,3,32,147,133,183,33,4,148,130,7,32,149,130,23,32,46,130,7,32,150,130,3,32,151,130,3,32,86,130,3,32,152,130,3,130,247,33,3,186,130,
    7,32,154,130,3,32,155,130,3,32,72,130,3,32,156,130,3,130,247,33,1,48,130,7,32,158,130,3,32,159,134,235,32,160,130,7,32,161,130,71,32,50,130,7,32,162,130,3,32,163,130,3,32,74,130,3,
    32,164,130,3,32,169,130,23,32,52,130,7,32,170,130,3,32,171,67,199,5,33,4,172,130,11,32,173,130,3,32,76,130,3,32,174,130,3,32,175,130,23,32,190,130,7,32,176,130,3,32,177,130,3,32,105,
    130,3,32,178,130,3,32,179,130,3,32,78,130,3,32,180,130,3,32,181,130,71,32,58,130,7,32,182,130,3,32,183,65,215,5,33,4,184,130,7,32,186,130,23,32,60,130,7,32,187,134,3,32,90,130,7,32,
    188,130,3,32,189,130,23,32,63,130,7,32,190,130,3,32,191,65,203,5,33,4,192,130,7,32,194,130,107,32,192,130,7,32,195,130,3,32,196,71,15,5,33,4,197,130,11,32,198,130,3,32,103,130,3,32,
    199,130,3,32,200,130,23,32,67,130,7,32,201,130,3,32,202,65,183,6,32,203,130,7,32,204,66,55,5,33,4,205,130,7,32,206,130,3,32,101,130,3,32,207,130,3,32,215,130,83,131,79,32,216,130,11,
    130,3,33,1,69,130,7,32,217,133,3,33,3,205,130,7,32,218,133,3,33,3,204,130,7,32,219,130,3,32,223,130,47,32,206,130,7,32,224,130,3,32,225,130,107,32,70,130,7,32,226,130,3,32,245,130,
    23,32,211,130,7,32,246,130,3,32,247,66,139,5,33,4,248,130,7,32,249,130,23,32,231,130,7,32,250,130,3,32,251,130,47,32,72,130,7,32,252,130,3,32,253,130,23,32,233,130,7,32,254,130,3,32,
    255,130,3,36,107,0,0,5,0,133,3,33,1,74,130,7,32,1,133,3,33,3,235,130,7,32,2,130,3,32,16,71,227,5,33,5,17,130,11,130,3,33,4,109,130,7,32,18,130,3,32,19,130,71,36,236,0,0,30,0,130,3,
    130,51,33,3,174,130,7,32,62,130,3,32,63,130,23,32,172,130,7,32,128,130,3,32,133,70,15,5,33,30,160,130,11,32,241,130,11,32,238,130,7,32,242,130,3,32,243,130,11,32,165,130,7,32,244,130,
    3,40,249,0,0,4,64,0,0,31,77,133,3,36,4,169,0,0,32,130,83,33,32,11,130,119,32,91,130,11,32,16,130,3,32,17,72,31,5,33,32,19,130,11,32,20,130,11,32,105,130,7,32,21,133,3,33,4,111,130,
    7,32,23,130,3,130,136,33,1,107,130,7,130,60,33,32,34,130,35,32,115,130,11,32,37,130,3,32,39,130,11,32,118,130,7,32,48,133,3,33,1,121,130,7,32,50,130,3,32,51,70,99,5,33,32,57,130,11,
    32,58,130,35,32,122,130,7,32,60,133,3,32,3,131,131,32,68,130,11,130,3,33,1,124,130,7,32,116,133,3,33,1,125,130,7,32,127,133,3,33,1,126,130,7,32,163,133,3,33,4,110,130,7,32,164,133,
    3,32,1,131,31,32,166,130,11,32,170,130,83,36,128,0,0,32,171,133,3,33,4,71,130,7,32,172,133,3,33,1,133,130,7,32,177,133,3,33,1,134,130,7,32,185,130,3,32,186,130,47,32,135,130,7,32,188,
    130,3,32,189,130,11,36,137,0,0,33,5,133,3,33,1,139,130,7,130,251,32,33,130,3,33,1,140,130,11,32,22,133,3,33,1,141,130,7,130,235,32,33,130,3,33,1,142,130,11,32,38,133,3,32,0,130,79,
    33,33,46,130,11,130,3,33,1,143,130,7,32,91,130,3,32,94,130,83,36,144,0,0,34,2,133,3,33,1,148,130,7,32,6,133,3,32,0,130,139,33,34,15,130,11,130,3,33,1,149,130,7,32,17,130,3,36,18,0,
    0,1,150,130,7,32,26,133,3,33,1,152,130,7,32,30,133,3,33,1,153,130,7,32,43,133,3,33,1,154,130,7,32,72,133,3,33,1,155,130,7,32,96,133,3,33,1,156,130,7,32,100,130,3,32,101,130,71,36,157,
    0,0,37,202,133,3,37,1,159,0,0,238,1,130,3,130,135,37,1,160,0,0,246,195,133,3,36,1,162,0,0,251,130,23,41,251,4,0,0,1,164,0,0,254,255,133,3,37,1,170,0,0,255,252,130,3,32,253,130,23,8,
    75,171,176,0,44,75,176,9,80,88,177,1,1,142,89,184,1,255,133,176,132,29,177,9,3,95,94,45,176,1,44,32,32,69,105,68,176,1,96,45,176,2,44,176,1,42,33,45,176,3,44,32,70,176,3,37,70,82,88,
    35,89,32,138,32,138,73,100,138,32,70,32,104,97,100,176,4,37,132,7,130,24,41,101,138,89,47,32,176,0,83,88,105,130,5,38,84,88,33,176,64,89,27,136,10,38,101,89,89,58,45,176,4,131,70,130,
    49,130,45,33,138,89,130,64,32,106,134,64,130,7,132,18,8,41,47,253,45,176,5,44,75,32,176,3,38,80,88,81,88,176,128,68,27,176,64,68,89,27,33,33,32,69,176,192,80,88,176,192,68,27,33,89,
    89,45,176,6,136,163,130,171,34,125,105,24,133,173,39,7,44,176,6,42,45,176,8,133,65,41,83,88,176,64,27,176,0,89,138,138,131,79,41,83,88,35,33,176,128,138,138,27,138,130,193,130,95,132,
    15,32,192,141,15,34,184,1,0,143,16,32,64,139,16,39,176,3,37,69,184,1,128,80,130,74,130,6,34,35,33,27,131,16,35,35,33,35,33,130,161,41,89,68,45,176,9,44,75,83,88,69,130,162,32,33,130,
    162,42,10,44,176,41,69,45,176,11,44,176,42,130,6,63,12,44,177,39,1,136,32,138,83,88,185,64,0,4,0,99,184,8,0,136,84,88,185,0,41,3,232,112,89,27,176,35,130,172,36,32,136,184,16,0,136,
    19,131,229,52,13,44,176,64,136,184,32,0,90,88,177,42,0,68,27,185,0,42,3,232,68,130,90,46,12,43,176,0,43,0,178,1,13,2,43,1,178,14,1,130,5,49,183,14,58,48,37,27,16,0,8,43,0,183,1,56,
    46,36,26,17,130,10,38,183,2,78,64,50,35,21,131,9,37,3,72,59,46,33,20,131,9,32,4,136,19,37,5,48,40,31,22,14,131,19,37,6,99,81,63,45,27,131,9,34,7,64,52,134,59,37,8,91,74,58,41,25,131,
    19,37,9,131,100,78,58,35,131,9,37,10,119,98,76,54,33,131,9,35,11,145,119,92,133,19,37,12,118,96,75,54,29,131,19,37,13,44,36,28,20,12,130,9,36,0,178,15,13,7,130,162,49,32,69,125,105,
    24,68,178,176,19,1,115,178,80,19,1,116,178,128,131,4,38,112,19,1,117,178,15,31,130,19,43,111,31,1,117,0,42,0,204,0,145,0,158,130,3,8,70,236,0,114,0,178,0,125,0,86,0,95,0,78,0,96,1,
    4,0,196,0,0,0,20,254,96,0,20,2,155,0,16,255,57,0,13,254,151,0,18,3,33,0,11,4,58,0,20,4,141,0,16,5,176,0,20,6,24,0,21,6,192,0,16,2,91,0,18,7,4,0,5,130,51,133,2,33,96,0,135,1,10,170,
    154,0,196,1,64,1,191,2,88,2,244,3,14,3,58,3,105,3,156,3,193,3,227,3,249,4,32,4,55,4,139,4,185,5,10,5,125,5,193,6,39,6,143,6,188,7,58,7,164,7,176,7,188,7,219,8,2,8,33,8,135,9,51,9,115,
    9,221,10,48,10,121,10,185,10,239,11,78,11,139,11,166,11,217,12,32,12,68,12,157,12,217,13,51,13,126,13,222,14,55,14,165,14,207,15,13,15,62,15,141,15,216,16,9,16,65,16,101,16,124,16,
    161,16,200,16,227,17,4,17,131,17,227,18,55,18,148,19,8,19,81,19,203,20,11,20,69,20,144,20,215,20,242,21,93,21,166,21,244,22,88,22,184,22,245,23,99,23,174,23,244,24,36,24,114,24,187,
    24,252,25,52,25,119,25,142,25,207,26,19,26,80,26,178,27,21,27,118,27,217,27,248,28,147,28,196,29,101,29,227,29,239,30,12,30,188,30,210,31,17,31,84,31,167,32,25,32,57,32,138,32,182,
    32,214,33,11,33,57,33,131,33,143,33,169,33,195,33,221,34,70,34,170,34,232,35,99,35,180,36,32,36,222,37,86,37,171,38,29,38,124,38,218,38,245,39,65,39,138,39,199,40,30,40,121,40,253,
    41,153,41,201,42,44,42,146,42,255,43,99,43,183,44,17,44,66,44,165,44,220,45,4,45,12,45,59,45,94,45,150,45,194,46,5,46,58,46,126,46,158,46,190,46,199,46,245,47,39,47,67,47,92,47,161,
    47,169,47,207,47,252,48,117,48,163,48,227,49,17,49,77,49,194,50,28,50,133,50,248,51,104,51,155,52,15,52,141,52,231,53,48,53,163,53,208,54,40,54,152,54,233,55,66,55,159,55,245,56,57,
    56,120,56,228,57,54,57,150,58,14,58,94,58,211,59,52,59,163,60,24,60,140,60,221,61,25,61,113,61,205,62,57,62,184,62,241,63,58,63,128,63,236,64,34,64,99,64,160,64,233,65,66,65,166,65,
    242,66,104,66,231,67,65,67,169,68,19,68,57,68,142,68,251,69,121,69,178,70,3,70,74,70,148,70,234,71,24,71,68,71,206,72,4,72,70,72,131,72,199,73,27,73,125,73,199,74,56,74,176,75,9,75,
    129,75,239,76,99,76,211,77,55,77,115,77,210,78,49,78,152,79,29,79,158,79,235,80,57,80,165,81,18,81,132,81,245,82,126,83,6,83,164,84,55,84,165,85,15,85,83,85,153,86,4,86,107,87,43,87,
    227,88,92,88,219,89,48,89,131,89,184,89,212,90,7,90,29,90,51,91,4,91,114,91,218,92,49,92,160,92,204,92,245,93,74,93,149,93,235,94,61,94,141,94,226,95,65,95,143,95,237,96,67,96,210,
    97,92,97,162,97,229,98,55,98,134,98,201,99,56,99,183,100,23,100,108,100,202,101,37,101,140,101,238,102,72,102,87,102,103,102,182,103,30,103,165,104,23,104,128,104,230,105,74,105,181,
    106,31,106,131,106,240,107,75,107,157,107,239,108,64,108,182,108,225,152,1,9,184,233,108,241,108,251,109,5,109,32,109,67,109,101,109,133,109,164,109,176,109,188,109,238,110,44,110,
    141,110,177,110,189,110,205,110,230,111,180,111,208,111,236,111,255,112,19,112,90,112,220,113,126,114,10,114,22,114,230,115,75,115,201,116,126,116,228,117,94,117,182,118,36,118,193,
    119,34,119,184,120,22,120,120,120,146,120,172,120,198,120,224,121,75,121,113,121,169,121,191,121,243,122,133,122,199,123,70,123,133,123,148,123,163,123,220,123,239,124,24,124,49,124,
    61,124,160,124,245,125,142,126,24,126,143,127,72,127,72,128,248,129,97,129,142,130,11,130,60,130,82,130,193,131,27,131,104,131,217,132,47,132,117,132,188,133,10,133,45,133,107,133,
    239,134,68,134,140,134,204,135,2,135,96,135,186,135,213,136,0,136,67,136,103,136,185,136,242,137,70,137,143,137,234,138,66,138,171,138,213,139,14,139,63,139,137,139,210,140,3,140,59,
    140,131,140,172,140,254,141,113,141,179,142,18,142,110,142,155,143,31,143,127,143,149,143,232,144,150,144,255,145,98,145,171,145,241,146,51,146,116,146,234,147,83,147,201,147,243,148,
    40,148,155,148,206,149,24,149,74,149,141,149,251,150,76,150,175,151,12,151,133,151,248,152,136,152,216,153,23,153,108,153,194,154,61,154,187,154,247,155,79,155,152,155,219,156,20,156,
    85,156,141,156,203,157,33,157,45,157,121,157,239,158,126,158,209,159,19,159,148,159,249,160,95,160,193,161,80,161,92,161,173,161,249,162,71,162,136,162,247,163,92,163,186,164,48,164,
    194,165,71,165,222,166,83,166,178,167,5,167,101,167,109,167,185,168,30,168,129,168,242,169,109,169,192,170,34,170,109,170,201,171,42,171,84,171,171,171,215,172,46,172,118,172,138,172,
    158,172,176,172,196,172,214,172,237,173,1,173,95,173,133,174,2,174,102,174,184,174,192,174,200,174,208,174,219,174,227,175,73,130,1,13,93,81,175,193,176,49,176,146,176,212,177,55,177,
    78,177,101,177,124,177,142,177,166,177,185,177,197,177,209,177,232,177,255,178,22,178,46,178,69,178,92,178,115,178,139,178,157,178,180,178,203,178,226,178,249,179,17,179,40,179,58,
    179,81,179,105,179,128,179,151,179,169,179,191,179,213,179,236,180,4,180,16,180,28,180,51,180,69,180,91,180,114,180,136,180,158,180,181,180,205,180,222,180,245,181,7,181,29,181,46,
    181,70,181,93,181,111,181,133,181,156,181,174,181,197,181,220,181,237,182,4,182,27,182,133,183,39,183,57,183,75,183,98,183,120,183,143,183,166,183,184,183,201,183,219,183,235,184,2,
    184,19,184,42,184,64,184,87,184,110,184,219,185,114,185,137,185,154,185,177,185,199,185,222,185,244,186,11,186,34,186,46,186,64,186,87,186,105,186,128,186,146,186,169,186,192,186,215,
    186,238,186,249,187,4,187,27,187,39,187,51,187,74,187,97,187,109,187,121,187,144,187,167,187,179,187,191,187,212,187,233,187,245,188,1,188,24,188,42,188,54,188,66,188,89,188,106,188,
    127,188,150,188,167,188,190,188,213,188,237,189,5,189,23,189,41,189,53,189,65,189,83,189,100,189,118,189,136,189,159,189,181,189,193,189,205,189,217,189,229,189,247,190,8,190,20,190,
    32,190,44,190,56,190,79,190,91,190,114,190,136,190,154,190,176,190,199,190,222,190,241,191,4,191,28,191,47,191,141,191,239,192,6,192,29,192,52,192,74,192,98,192,121,192,144,192,167,
    192,190,192,208,192,225,192,248,193,10,193,33,193,56,193,104,193,152,193,168,193,191,193,214,193,236,193,253,194,21,194,45,194,57,194,69,194,92,194,115,194,137,194,160,194,183,194,
    205,194,228,194,252,195,14,195,37,195,55,195,77,195,94,195,118,195,141,195,164,195,186,195,210,195,233,195,255,196,22,196,125,196,143,196,165,196,188,196,205,196,222,196,244,197,10,
    197,33,197,142,197,164,197,186,197,209,197,232,197,244,198,10,198,28,198,51,198,74,198,85,198,107,198,130,198,142,198,164,198,176,198,197,198,209,198,232,198,244,199,11,199,28,199,
    51,199,70,199,88,199,100,199,117,199,135,199,157,199,169,199,186,199,198,199,220,199,232,199,254,200,15,200,38,200,57,200,76,200,173,200,196,200,218,200,241,201,8,201,31,201,53,201,
    64,201,76,201,88,201,100,201,112,201,124,201,136,201,163,201,171,201,179,201,187,201,195,201,203,201,211,201,219,201,227,201,235,201,243,201,251,202,3,202,11,202,19,202,43,202,67,202,
    85,202,103,202,121,202,138,202,164,202,172,202,180,202,188,202,196,202,204,202,228,202,251,203,13,203,31,203,49,203,73,203,96,203,206,203,214,203,238,203,246,203,254,204,21,204,44,
    204,52,204,60,204,68,204,76,204,99,204,107,204,115,204,123,204,131,204,139,204,147,204,155,204,163,204,171,204,179,204,202,204,210,204,218,205,46,205,54,205,62,205,85,205,108,205,116,
    205,124,205,148,205,156,205,179,205,201,205,224,205,247,206,14,206,37,206,56,206,75,206,98,206,115,206,135,206,166,206,178,206,196,206,204,206,227,206,245,207,1,207,13,207,36,207,59,
    207,82,207,105,207,113,207,121,207,145,207,169,207,181,207,193,207,205,207,217,207,229,207,241,207,249,208,1,208,9,208,32,208,55,208,63,208,86,208,109,208,133,208,156,208,164,208,172,
    208,195,208,217,208,241,208,249,209,16,209,40,209,64,209,88,209,111,209,134,209,156,209,180,209,204,209,228,209,252,210,4,210,12,210,36,210,59,210,83,210,106,210,124,210,141,210,165,
    210,188,210,212,210,236,211,4,211,27,211,55,211,83,211,95,211,107,211,115,211,127,211,139,211,151,211,163,211,181,211,199,211,224,211,242,212,11,212,29,212,48,212,66,212,85,212,103,
    212,119,212,134,212,153,212,171,212,190,212,208,212,227,212,245,213,8,213,26,213,42,213,58,213,70,213,82,213,100,213,118,213,136,213,153,213,178,213,196,213,221,213,239,214,2,214,20,
    214,39,214,57,214,73,214,88,214,106,214,124,214,136,214,148,214,160,214,172,214,190,214,208,214,227,214,245,215,8,215,26,215,45,215,63,215,82,215,100,215,116,215,131,215,143,215,161,
    215,173,215,191,215,203,215,221,215,233,215,250,216,6,216,18,216,30,216,42,216,60,216,78,216,96,216,114,216,132,216,150,216,168,216,186,216,204,216,221,216,233,216,245,217,1,217,13,
    217,31,217,49,217,67,217,84,217,206,217,232,217,244,218,0,218,12,218,24,218,36,218,48,218,60,218,72,218,84,218,96,218,108,218,120,218,132,218,144,218,156,218,168,218,180,218,192,218,
    200,219,45,219,146,219,208,220,15,220,109,220,204,220,231,221,2,221,14,221,26,221,38,221,50,221,62,221,74,221,149,221,228,222,62,222,150,222,158,222,170,222,180,222,188,222,196,222,
    204,222,212,222,220,222,228,222,246,223,8,223,31,223,54,223,78,223,102,223,126,223,150,223,174,223,198,223,222,223,246,224,14,224,38,224,62,224,86,224,98,224,110,224,122,224,134,224,
    146,224,158,224,170,224,182,224,194,224,212,224,230,224,242,224,254,225,10,225,22,225,34,225,46,225,58,225,70,225,88,225,106,225,118,225,130,225,142,225,154,225,166,225,178,225,196,
    225,213,225,225,225,237,225,249,226,5,226,17,226,29,226,41,226,53,226,65,226,77,226,89,226,101,226,113,226,125,226,133,226,141,226,149,226,157,226,165,226,173,226,181,226,189,226,197,
    226,205,226,213,226,221,226,229,226,253,227,20,227,43,227,61,227,69,227,77,227,101,227,109,227,127,227,149,227,157,227,165,227,173,227,181,227,204,227,212,227,220,227,228,227,236,227,
    244,227,252,228,4,228,12,228,153,229,10,229,107,229,115,229,127,229,145,229,162,229,170,229,182,229,194,229,206,229,218,229,230,0,0,0,5,0,100,0,0,3,40,5,176,0,3,0,6,0,9,0,12,0,15,0,
    111,178,12,16,17,17,18,57,176,12,16,176,0,208,131,5,32,6,132,5,32,9,132,5,47,13,208,0,176,0,69,88,176,2,47,27,177,2,31,62,89,132,12,32,0,130,12,39,0,15,62,89,178,4,2,0,130,57,33,178,
    5,133,6,32,7,133,6,32,8,132,6,36,176,10,220,178,12,132,9,33,178,13,132,6,8,74,176,2,16,176,14,220,48,49,33,33,17,33,3,17,1,1,17,1,3,33,1,53,1,33,3,40,253,60,2,196,54,254,238,254,186,
    1,12,228,2,3,254,254,1,2,253,253,5,176,250,164,5,7,253,125,2,119,251,17,2,120,253,94,2,94,136,2,94,0,2,0,143,255,242,1,163,132,191,38,13,0,59,178,6,14,15,130,127,36,176,6,16,176,1,
    147,167,32,12,130,167,32,12,131,167,41,6,13,10,43,88,33,216,27,244,89,130,40,8,45,176,1,47,48,49,1,35,3,33,1,52,54,50,22,21,20,6,34,38,1,126,209,23,1,0,254,249,74,128,74,72,132,72,
    1,173,4,3,250,195,57,75,75,57,55,74,74,130,115,44,101,3,244,2,64,6,0,0,4,0,9,0,37,65,14,5,32,3,130,89,56,3,33,62,89,176,2,208,176,2,47,176,7,208,176,7,47,176,3,16,176,8,208,176,8,47,
    130,93,36,3,35,17,51,5,131,4,38,1,19,35,139,174,1,45,130,4,38,5,119,254,125,2,12,137,131,4,130,83,32,96,130,78,32,188,130,199,36,27,0,31,0,141,133,83,132,173,65,98,7,32,16,130,96,32,
    16,135,12,32,2,130,12,32,2,130,199,37,176,0,69,88,176,26,130,12,32,26,130,12,35,178,29,12,2,131,252,36,29,47,178,0,3,136,222,38,4,208,176,29,16,176,6,132,5,38,11,208,176,11,47,178,
    8,137,28,35,11,16,176,14,130,19,40,16,176,18,208,176,8,16,176,20,132,37,38,22,208,176,0,16,176,24,132,17,33,30,208,130,187,38,35,3,35,19,35,53,33,132,3,35,51,3,51,19,130,3,35,21,35,
    3,51,130,3,130,5,50,19,35,2,207,224,76,168,76,231,1,5,58,243,1,17,78,167,78,225,130,3,56,208,238,58,221,251,76,167,118,224,58,224,1,154,254,102,1,154,158,1,57,159,1,160,254,96,131,
    3,59,159,254,199,158,254,102,2,56,1,57,0,1,0,100,255,45,4,38,6,155,0,44,0,125,178,42,45,46,130,192,146,252,32,9,130,226,32,9,135,252,32,35,130,12,32,35,135,252,32,32,130,12,32,32,130,
    12,35,178,25,12,32,130,59,37,176,25,16,178,2,1,135,252,35,178,15,9,35,66,200,5,33,178,19,137,20,34,39,35,9,131,20,35,35,16,178,42,136,20,130,229,8,139,52,38,38,39,38,53,52,54,55,53,
    51,21,22,22,21,35,52,38,35,34,6,21,20,22,4,30,2,21,20,6,7,21,35,53,38,38,53,51,20,22,51,50,54,3,51,108,252,70,233,202,173,160,174,190,242,113,97,96,108,107,1,0,146,100,54,207,185,159,
    198,213,243,127,116,114,119,1,124,85,111,89,38,125,245,166,214,20,218,220,25,245,196,126,145,104,97,87,105,94,80,103,134,90,169,210,19,195,194,22,240,198,126,138,110,0,0,5,0,99,255,
    236,5,137,5,197,0,13,0,26,0,39,0,53,0,57,0,137,178,5,58,59,131,159,37,5,16,176,19,208,176,130,5,32,27,132,5,32,40,132,5,46,54,208,0,176,54,47,176,56,47,176,0,69,88,176,3,130,252,32,
    3,65,22,7,32,37,130,12,47,37,15,62,89,176,3,16,176,10,208,176,10,47,178,17,2,66,5,8,35,3,16,178,24,137,13,44,37,16,176,30,208,176,30,47,176,37,16,178,43,137,22,35,30,16,178,50,136,
    13,40,48,49,19,52,54,51,50,22,21,130,252,36,35,34,38,53,23,132,249,39,53,53,52,38,34,6,21,1,136,26,32,32,139,25,32,35,130,26,8,123,5,39,1,23,99,170,138,140,169,169,138,135,175,170,
    77,63,62,76,77,126,75,2,18,174,135,136,173,167,254,232,171,170,79,62,64,73,78,61,62,77,254,2,125,2,199,125,4,152,132,169,169,137,72,131,168,165,140,6,69,85,85,73,73,69,86,87,71,252,
    208,134,166,166,141,71,130,169,167,137,5,68,87,83,75,75,70,84,84,74,244,72,4,114,72,0,3,0,86,255,236,5,17,5,196,0,28,0,37,0,49,0,152,178,46,50,51,17,18,57,176,46,16,176,16,208,131,
    5,32,30,67,247,6,66,50,12,36,27,47,27,177,27,66,50,7,32,24,130,12,32,24,130,12,35,178,32,27,9,130,58,35,178,40,9,27,131,6,34,3,32,40,131,6,34,16,40,32,131,6,32,19,133,27,34,17,19,24,
    131,13,34,25,24,19,131,6,34,22,17,25,130,6,36,176,27,16,178,29,66,57,8,35,178,31,29,17,131,20,35,9,16,178,47,136,20,40,48,49,19,52,54,55,38,38,53,65,39,5,8,142,20,6,7,7,1,54,53,51,
    16,7,23,33,39,6,32,36,5,50,55,1,7,6,21,20,22,3,20,23,55,55,54,53,52,38,35,34,6,86,110,162,85,67,208,176,159,203,92,105,99,1,25,61,211,126,214,254,230,82,156,254,80,254,253,1,226,123,
    107,254,194,31,120,130,25,103,111,31,62,86,66,71,84,1,137,101,169,116,107,150,70,171,199,187,138,91,153,76,72,254,180,120,147,254,243,172,253,97,117,229,35,82,1,119,22,91,117,101,126,
    3,170,84,127,76,25,55,86,57,81,96,0,0,1,0,82,3,252,1,11,6,0,0,4,0,22,68,177,19,8,35,48,49,1,3,35,17,51,1,11,26,159,185,5,131,254,121,2,4,0,1,0,128,254,49,2,162,6,95,0,16,0,16,178,7,
    17,18,130,230,38,0,176,4,47,176,13,47,131,223,8,55,18,18,55,23,6,2,3,7,16,18,23,7,38,2,2,128,124,240,134,48,141,175,8,1,171,154,48,134,241,123,2,80,231,1,159,1,71,66,142,107,254,73,
    254,229,86,254,209,254,37,124,135,66,1,73,1,157,130,87,32,40,130,87,32,81,130,87,32,18,131,87,33,19,20,135,87,32,14,130,87,8,63,1,20,2,2,7,39,54,18,17,53,16,2,39,39,55,22,18,18,23,
    2,81,122,248,135,48,150,175,152,142,31,48,128,240,128,8,2,64,222,254,99,254,173,65,135,116,1,221,1,50,23,1,22,1,201,138,28,136,62,254,196,254,121,208,0,130,93,42,27,2,77,3,116,5,176,
    0,14,0,32,133,233,49,4,47,27,177,4,31,62,89,176,0,208,25,176,0,47,24,176,9,130,7,34,9,47,24,130,243,8,53,37,55,5,3,51,3,37,23,5,19,7,3,3,39,1,76,254,207,55,1,46,15,179,15,1,41,54,254,
    202,200,145,180,178,146,3,204,88,169,117,1,88,254,162,115,172,88,254,246,106,1,32,254,233,102,131,101,46,68,0,146,4,42,4,182,0,11,0,26,0,176,9,47,130,91,36,176,9,16,178,6,66,2,8,34,
    176,3,208,130,95,8,32,33,21,33,17,35,17,33,53,33,17,51,2,174,1,124,254,132,236,254,130,1,126,236,3,33,222,254,79,1,177,222,1,149,130,175,55,28,254,184,1,93,0,235,0,9,0,24,178,9,10,
    11,17,18,57,0,176,10,47,178,5,70,162,8,62,48,49,19,39,54,54,55,53,51,7,6,6,159,131,58,43,1,219,1,1,105,254,184,78,91,135,70,189,175,106,213,131,141,48,71,2,9,2,84,2,205,0,3,0,17,0,
    176,2,47,178,1,136,135,131,132,42,53,33,2,84,253,243,2,13,2,9,196,130,111,46,135,255,245,1,162,1,0,0,10,0,34,178,0,11,12,69,113,8,41,6,47,27,177,6,15,62,89,178,0,138,121,34,1,50,22,
    68,0,5,51,52,54,1,20,68,74,74,68,65,76,74,1,0,77,58,57,75,74,116,77,131,121,38,2,255,131,2,254,5,176,130,121,35,19,0,176,0,68,151,5,32,2,72,27,6,8,34,48,49,23,35,1,51,193,191,2,61,
    191,125,6,45,0,0,2,0,105,255,236,4,34,5,196,0,13,0,27,0,70,178,3,28,29,130,125,36,176,3,16,176,17,67,181,6,32,10,130,57,33,10,31,72,85,6,32,3,130,12,32,3,130,144,36,176,10,16,178,17,
    136,208,130,46,33,178,24,136,13,8,82,48,49,1,16,2,35,34,2,3,53,16,18,51,50,18,19,39,52,38,35,34,6,7,17,20,22,51,50,54,55,4,34,235,240,236,239,3,235,241,239,235,3,243,112,122,119,112,
    3,114,122,117,112,3,2,101,254,198,254,193,1,55,1,49,252,1,58,1,58,254,206,254,207,20,205,191,181,192,254,182,204,200,185,197,131,213,42,168,0,0,2,255,5,181,0,6,0,57,66,67,5,32,5,130,
    139,32,5,72,238,17,33,0,5,131,192,35,4,47,178,3,136,145,34,178,2,3,131,20,53,48,49,33,35,17,5,53,37,51,2,255,242,254,155,2,56,31,4,145,122,205,209,131,91,46,81,0,0,4,64,5,196,0,25,
    0,78,178,17,26,27,65,127,8,32,17,130,98,32,17,144,98,34,3,17,0,130,33,36,176,17,16,178,9,137,98,32,22,133,20,33,0,16,65,9,12,36,33,33,53,1,54,68,77,6,8,65,21,35,52,54,54,51,50,22,21,
    20,6,7,1,33,4,64,252,45,1,229,105,89,117,99,118,130,243,121,225,147,212,245,123,140,254,156,2,164,167,2,17,117,157,79,104,128,144,125,133,213,118,213,188,109,239,152,254,131,0,1,0,
    79,255,236,4,21,130,161,38,41,0,110,178,7,42,43,136,161,32,15,130,161,32,15,135,161,69,83,7,34,178,1,15,131,195,46,176,1,47,178,31,1,1,113,178,159,1,1,93,178,63,130,9,36,176,15,16,
    178,7,65,182,9,35,1,16,178,40,136,13,35,178,21,40,1,69,74,6,32,34,136,20,35,48,49,1,51,147,191,32,22,130,197,35,4,35,34,36,71,125,6,8,84,53,52,38,35,35,1,134,148,112,131,109,112,98,
    126,243,119,213,132,218,249,125,99,120,125,254,243,219,210,254,244,243,129,109,113,130,136,134,143,3,71,1,114,108,104,115,113,91,112,184,103,219,195,98,173,44,41,176,122,196,232,224,
    186,96,120,120,114,115,124,0,0,2,0,52,0,0,4,88,5,176,0,10,0,14,0,73,65,229,5,70,52,12,32,4,130,237,39,4,15,62,89,178,1,9,4,131,171,130,224,72,90,9,44,176,6,208,176,1,16,176,11,208,
    178,8,6,11,130,29,33,178,13,132,36,131,194,8,52,21,35,17,35,17,33,39,1,51,1,33,17,7,3,163,181,181,243,253,139,7,2,116,251,253,144,1,125,18,2,7,195,254,188,1,68,148,3,216,252,87,2,96,
    32,0,0,1,0,129,255,236,4,58,130,135,38,29,0,106,178,26,30,31,65,109,8,32,1,130,127,32,1,65,109,7,32,13,130,12,32,13,130,140,130,116,66,107,11,34,7,1,13,130,47,36,176,7,47,178,26,65,
    70,8,35,178,5,7,26,131,20,35,13,16,178,20,137,20,33,17,20,131,20,35,178,29,26,20,130,27,55,48,49,19,19,33,21,33,3,54,51,50,18,21,20,0,35,34,36,39,51,22,22,51,50,66,60,6,8,58,7,174,
    79,3,14,253,188,40,101,127,208,231,255,0,223,200,254,249,11,235,14,124,100,112,125,138,121,66,92,54,2,210,2,222,210,254,164,58,254,246,225,222,254,249,227,186,106,113,160,138,133,155,
    35,51,0,0,2,0,117,130,203,44,55,5,183,0,20,0,31,0,98,178,21,32,33,130,108,35,176,21,16,176,76,53,7,32,0,130,198,32,0,144,211,33,0,16,68,176,10,34,178,7,0,135,211,33,5,7,131,9,33,178,
    21,136,197,36,176,13,16,178,27,136,13,39,48,49,1,21,35,6,6,7,136,197,8,92,0,17,53,16,0,33,3,34,6,7,21,20,22,50,54,16,38,3,97,30,204,244,23,117,182,193,223,254,251,212,218,254,241,1,
    117,1,94,236,80,133,31,136,216,126,128,5,183,201,3,218,200,123,254,240,215,222,254,237,1,66,1,5,83,1,127,1,178,253,73,90,75,74,162,191,162,1,8,166,0,0,1,0,69,0,0,4,54,5,176,0,6,0,50,
    68,7,18,32,1,130,205,38,1,15,62,89,176,5,16,65,148,11,32,0,68,0,6,61,1,1,35,1,33,53,33,4,54,253,186,255,2,69,253,15,3,241,5,41,250,215,4,237,195,0,0,3,0,104,69,9,6,42,23,0,33,0,43,
    0,116,178,9,44,45,72,70,5,39,176,26,208,176,9,16,176,36,69,17,6,32,21,130,99,32,21,65,49,7,32,9,130,12,32,9,130,112,35,178,41,9,21,131,45,38,41,47,178,31,41,1,113,65,252,11,34,3,26,
    41,130,25,35,178,15,41,26,133,78,33,178,31,65,47,8,36,176,21,16,178,37,136,13,37,48,49,1,20,6,7,67,91,8,72,175,10,41,3,52,38,34,6,21,20,22,50,54,137,9,8,85,4,2,110,95,114,123,254,252,
    216,217,254,251,124,112,94,109,240,204,205,240,211,129,212,127,125,220,123,31,110,186,108,109,186,109,4,48,107,167,48,53,184,116,192,225,226,191,117,186,50,48,167,107,186,218,218,252,
    175,108,133,132,109,107,128,124,2,253,95,123,117,101,100,118,118,0,0,2,0,93,255,250,4,18,5,196,0,21,130,251,36,100,178,9,34,35,133,170,33,176,22,73,187,19,32,17,130,243,32,17,131,243,
    34,22,17,9,130,39,36,124,176,22,47,24,75,227,11,33,0,2,131,22,36,176,17,16,178,18,136,219,33,176,9,73,162,11,130,233,57,6,35,34,2,53,52,54,54,51,50,0,17,21,16,0,5,35,53,51,54,54,3,
    50,54,55,53,134,235,8,85,3,30,122,163,192,228,116,214,141,220,1,2,254,156,254,159,29,35,215,230,220,73,128,35,132,210,125,126,2,97,129,1,13,219,144,234,130,254,184,254,237,68,254,118,
    254,98,3,201,3,201,1,15,84,74,95,161,196,173,132,137,168,0,255,255,0,130,255,245,1,157,4,81,0,38,0,18,251,0,0,7,0,18,255,251,3,81,130,23,36,46,254,184,1,136,130,23,32,39,130,17,50,
    230,3,81,0,6,0,16,18,0,0,1,0,63,0,164,3,132,4,78,130,15,36,23,178,0,7,8,67,245,8,32,5,130,238,35,5,27,62,89,130,180,8,35,5,21,1,53,1,21,1,54,2,78,252,187,3,69,2,119,224,243,1,117,193,
    1,116,243,0,2,0,145,1,100,3,239,3,214,0,3,130,105,44,37,0,176,7,47,176,3,208,176,3,47,178,0,65,2,9,35,7,16,178,4,72,5,14,41,17,33,53,33,3,239,252,162,3,94,131,3,37,3,12,202,253,142,
    201,131,139,36,128,0,165,3,224,146,139,32,2,130,139,32,2,133,139,32,37,131,137,50,53,2,234,253,150,3,96,252,160,2,124,227,239,254,140,193,254,140,239,130,139,48,60,255,244,3,152,5,
    196,0,24,0,35,0,94,178,9,36,37,65,205,6,32,28,65,205,6,78,185,12,32,34,130,82,37,34,15,62,89,178,28,72,100,8,41,176,0,208,176,0,47,178,4,0,16,131,56,32,16,70,232,12,34,12,16,0,130,
    20,33,178,21,132,27,37,48,49,1,52,54,54,75,47,7,33,21,35,70,224,6,36,7,7,6,7,3,75,104,7,8,54,34,38,1,94,66,195,26,40,93,90,86,105,243,2,237,195,201,225,152,123,66,2,244,74,63,64,74,
    72,132,71,1,172,133,158,189,40,61,71,94,99,97,83,177,206,204,183,163,158,121,75,144,254,201,59,73,79,217,7,48,91,254,59,6,217,5,144,0,54,0,66,0,124,178,59,67,68,130,125,36,176,59,16,
    176,35,130,203,35,42,47,176,51,77,119,10,76,78,7,32,8,130,209,32,8,130,12,35,178,5,51,8,130,45,33,178,15,132,6,39,176,15,47,176,8,16,178,58,77,79,8,39,176,21,208,176,51,16,178,27,137,
    16,35,42,16,178,35,137,13,35,15,16,178,64,136,13,130,233,8,33,6,2,35,34,39,6,6,35,34,38,55,54,18,54,51,50,22,23,3,6,51,50,54,55,18,0,33,34,4,2,7,6,18,4,131,13,32,23,131,33,45,36,39,
    38,19,18,18,36,51,50,4,18,1,6,22,131,22,8,157,19,38,35,34,6,6,205,12,222,190,181,61,51,135,74,146,151,18,16,127,195,110,84,129,87,52,19,133,102,131,6,17,254,193,254,192,196,254,209,
    178,9,12,139,1,31,207,84,183,64,38,61,207,105,254,254,148,91,94,11,12,222,1,129,246,249,1,103,178,252,3,13,74,81,54,96,30,45,50,47,111,140,2,6,250,254,223,154,76,76,240,201,163,1,6,
    143,42,66,253,205,198,219,174,1,113,1,136,196,254,141,237,241,254,163,182,40,34,137,40,49,215,204,211,1,38,1,18,1,181,242,219,254,101,254,140,136,141,95,83,1,237,19,209,0,2,0,18,0,
    0,5,66,5,176,0,7,0,10,0,70,75,149,15,82,77,7,65,81,7,74,136,8,39,9,4,2,17,18,57,176,9,66,215,11,33,178,10,132,20,41,48,49,1,33,3,33,1,51,1,33,130,7,8,38,3,195,253,204,118,254,249,2,
    38,227,2,39,254,248,253,156,1,166,211,1,83,254,173,5,176,250,80,2,31,2,92,0,3,0,148,0,0,4,163,130,127,40,14,0,22,0,31,0,109,178,2,70,153,5,37,2,16,176,17,208,176,130,5,32,30,66,171,
    6,71,115,12,82,213,8,34,23,0,1,131,135,40,23,47,178,31,23,1,113,178,15,67,86,8,35,178,8,15,23,131,25,35,0,16,178,16,72,233,12,32,30,136,13,39,48,49,51,17,33,50,4,21,69,107,8,32,1,130,
    14,36,54,53,52,39,37,71,110,6,8,69,35,148,1,243,247,1,2,108,104,118,129,254,249,245,254,234,1,25,119,134,232,254,210,248,118,133,123,130,246,5,176,198,196,100,160,44,32,177,124,205,
    220,2,145,254,57,118,105,227,5,186,107,98,108,96,0,1,0,102,255,236,4,235,5,196,0,29,0,64,178,3,72,55,10,82,54,12,75,67,8,33,12,16,81,3,10,34,176,3,16,70,51,10,53,48,49,1,6,0,35,34,
    36,2,39,53,52,18,36,51,50,0,23,35,38,38,35,71,69,5,130,164,8,86,55,4,235,22,254,212,249,174,254,247,144,3,146,1,17,179,241,1,38,24,252,18,147,142,165,177,2,169,163,149,150,20,1,218,
    233,254,251,165,1,48,201,136,206,1,58,170,254,250,239,157,139,241,233,129,236,248,134,156,0,0,2,0,148,0,0,4,210,5,176,0,11,0,21,0,70,178,2,22,23,17,18,57,176,2,16,176,21,65,113,27,
    36,176,1,16,178,12,65,66,8,36,176,0,16,178,13,65,80,15,40,18,21,21,20,2,4,35,3,17,131,161,8,43,53,52,38,35,148,1,174,193,1,43,164,165,254,207,197,166,165,199,213,2,206,196,5,176,172,
    254,196,204,73,207,254,198,170,4,228,251,230,249,233,81,237,250,0,1,132,145,32,76,132,145,32,78,66,137,5,36,6,47,27,177,6,71,98,7,73,245,8,34,11,6,4,131,170,32,11,69,84,12,32,4,82,
    94,11,36,176,6,16,178,8,69,98,12,35,17,33,21,33,133,3,55,3,231,253,170,2,187,252,72,3,177,253,76,2,86,2,138,254,64,202,5,176,204,254,110,134,127,32,49,130,127,34,9,0,64,67,9,26,32,
    178,66,252,19,33,176,4,78,98,11,37,48,49,1,33,17,35,134,111,45,219,253,182,253,3,157,253,96,2,74,2,105,253,151,131,107,32,79,130,107,46,106,255,236,4,240,5,196,0,30,0,85,178,11,31,
    32,70,101,8,32,11,130,242,32,11,135,242,66,35,8,32,11,77,103,25,35,178,30,11,3,130,61,34,176,30,47,73,128,12,34,37,6,4,66,56,5,8,81,16,0,33,50,4,23,35,2,33,34,6,7,21,20,18,51,50,55,
    17,33,53,33,4,240,79,254,232,178,183,254,230,153,3,1,60,1,27,243,1,30,29,248,42,254,249,170,177,3,199,177,194,82,254,212,2,40,189,103,106,166,1,53,206,114,1,74,1,115,240,226,1,7,245,
    237,112,236,254,251,88,1,29,192,65,41,5,39,5,24,5,176,0,11,0,76,65,169,18,78,43,12,67,171,7,36,176,0,69,88,176,65,195,8,34,9,6,0,68,64,6,75,185,9,41,48,49,33,35,17,33,17,35,17,51,130,
    5,52,51,5,24,252,253,117,253,253,2,139,252,2,135,253,121,5,176,253,162,2,94,130,121,36,163,0,0,1,159,130,121,34,3,0,29,86,238,26,132,74,36,51,1,159,252,252,130,42,130,53,36,45,255,
    236,3,228,130,10,38,15,0,47,178,5,16,17,65,109,8,74,245,12,39,5,47,27,177,5,15,62,89,66,224,10,41,48,49,1,51,17,20,4,35,34,38,76,226,7,43,2,232,252,254,251,214,228,248,252,115,109,
    102,130,149,41,252,3,209,246,230,205,116,117,135,119,65,21,10,34,12,0,83,66,63,18,32,8,130,94,32,8,86,39,20,32,11,130,25,32,11,131,120,34,0,4,2,130,154,40,180,106,0,122,0,2,93,178,
    6,133,13,37,101,6,117,6,2,93,130,137,8,43,7,17,35,17,51,17,55,1,33,1,1,33,2,54,165,253,253,140,1,170,1,50,253,227,2,60,254,212,2,117,175,254,58,5,176,253,85,173,1,254,253,123,252,213,
    133,141,33,4,38,130,243,34,5,0,40,146,141,32,2,130,115,32,2,132,115,67,39,10,46,37,33,21,33,17,51,1,145,2,149,252,110,253,202,202,65,59,5,36,148,0,0,6,106,130,10,34,14,0,110,133,71,
    65,52,12,88,109,12,65,222,7,36,176,0,69,88,176,132,239,70,57,7,87,223,8,34,1,0,4,131,226,35,101,1,117,1,130,226,32,7,133,13,35,106,7,122,7,130,13,32,10,134,13,34,10,122,10,131,240,
    34,9,2,33,130,241,36,19,1,35,1,19,130,7,8,36,1,220,1,164,1,163,1,71,252,25,254,82,181,254,83,25,252,5,176,251,164,4,92,250,80,1,224,2,130,251,158,4,97,253,127,254,32,136,177,33,5,23,
    130,10,36,9,0,76,178,1,81,183,6,131,145,80,42,12,65,142,12,66,164,12,67,117,7,35,178,2,5,0,130,171,33,178,7,132,6,36,48,49,33,35,1,130,135,49,51,1,17,51,5,23,253,253,119,253,253,2,
    139,251,4,9,251,247,130,136,54,243,4,13,0,2,0,102,255,236,5,30,5,196,0,16,0,30,0,70,178,4,31,32,130,61,36,176,4,16,176,20,69,81,6,69,255,12,65,43,8,32,12,78,13,11,130,46,67,198,12,
    34,1,20,2,67,199,6,44,52,18,36,32,4,18,23,7,52,2,35,34,2,67,201,5,8,64,18,53,5,30,148,254,237,179,177,254,235,151,1,151,1,19,1,100,1,19,150,1,253,183,168,164,185,2,187,166,168,181,
    2,178,214,254,189,173,173,1,64,209,82,213,1,70,173,171,254,191,213,5,242,1,2,254,255,235,84,240,254,250,1,0,246,70,5,6,44,212,5,176,0,10,0,19,0,77,178,10,20,21,131,179,35,10,16,176,
    12,134,179,86,201,12,32,1,77,86,6,35,178,11,1,3,69,130,17,32,3,70,200,11,36,48,49,1,17,35,71,95,5,8,47,4,35,37,33,50,54,53,52,38,39,33,1,145,253,2,45,244,1,31,254,231,253,254,211,1,
    48,135,142,144,126,254,201,2,29,253,227,5,176,254,209,214,238,203,127,120,118,141,2,130,149,46,96,255,4,5,26,5,196,0,21,0,35,0,70,178,8,74,185,5,35,8,16,176,32,134,149,81,145,12,66,
    104,8,35,17,16,178,25,66,205,8,36,176,8,16,178,32,77,83,12,37,2,7,23,7,37,6,71,78,10,37,4,18,23,7,52,38,65,78,11,41,25,131,118,250,164,254,202,61,70,176,65,82,6,57,177,180,1,19,150,
    1,254,184,168,163,185,2,185,167,169,181,2,178,207,254,209,89,195,148,245,13,65,86,13,40,246,254,254,255,234,85,236,254,246,65,85,9,44,222,5,176,0,14,0,23,0,90,178,5,24,25,88,55,6,32,
    16,134,191,32,4,84,248,8,73,98,10,35,178,15,2,4,131,39,32,15,84,37,11,35,178,11,1,15,71,152,6,38,14,208,176,4,16,178,23,70,215,13,65,99,6,37,6,7,1,21,33,1,65,102,7,8,65,2,171,254,230,
    253,2,0,252,1,18,141,126,1,71,254,241,253,194,1,4,128,144,133,132,254,245,2,49,253,207,5,176,226,214,146,197,53,253,161,13,2,252,129,112,117,128,2,0,0,1,0,74,255,236,4,138,5,196,0,
    39,0,99,178,17,40,41,69,33,8,81,120,12,32,29,130,182,39,29,15,62,89,178,2,29,9,130,33,35,178,14,9,29,130,6,33,176,9,70,157,12,32,2,139,163,33,178,34,132,41,33,176,29,78,224,14,41,52,
    38,36,39,38,53,52,36,51,50,89,209,12,78,244,7,89,207,8,8,65,141,135,254,160,104,199,1,31,229,152,238,136,252,143,133,124,137,148,1,84,206,96,254,233,239,158,254,247,147,253,164,153,
    132,133,1,119,96,104,106,65,125,201,176,228,112,207,126,114,129,106,95,80,107,101,129,167,112,182,215,117,206,137,124,136,107,0,130,219,42,45,0,0,4,176,5,176,0,7,0,46,70,173,18,32,
    2,69,9,6,34,176,6,16,77,165,11,33,4,208,71,197,7,49,53,33,4,176,254,58,251,254,62,4,131,4,228,251,28,4,228,204,130,83,36,125,255,236,4,189,130,83,36,16,0,60,178,4,87,129,6,35,0,69,
    88,176,65,47,12,77,96,12,70,251,8,73,49,11,8,49,1,17,20,0,35,34,0,53,17,51,17,20,22,51,32,17,17,4,189,254,215,247,250,254,218,252,148,144,1,36,5,176,252,51,232,254,241,1,11,237,3,204,
    252,50,146,154,1,52,3,198,130,123,36,18,0,0,5,29,130,123,34,6,0,56,78,161,12,75,35,12,68,236,12,68,210,8,37,0,1,3,17,18,57,130,217,56,1,33,1,35,1,33,2,149,1,114,1,22,253,244,245,253,
    246,1,21,1,61,4,115,250,80,130,87,38,1,0,48,0,0,6,229,130,9,38,12,0,96,178,5,13,14,82,249,21,69,65,12,72,183,21,36,0,69,88,176,6,86,189,8,132,123,33,178,5,133,6,32,10,132,6,130,137,
    33,19,51,130,137,130,2,8,38,51,19,1,51,5,10,224,251,254,176,242,254,235,254,229,243,254,176,251,226,1,22,212,1,104,4,72,250,80,4,39,251,217,5,176,251,186,4,70,130,255,36,41,0,0,4,233,
    130,157,32,11,71,103,7,140,248,72,125,12,70,145,12,32,7,130,137,39,7,15,62,89,178,0,1,4,130,210,33,178,6,133,6,34,3,0,6,131,13,72,146,5,65,26,5,34,1,33,1,132,2,8,40,2,137,1,50,1,36,
    254,72,1,194,254,217,254,199,254,198,254,218,1,195,254,71,1,36,3,162,2,14,253,46,253,34,2,22,253,234,2,222,2,210,0,130,149,32,7,130,149,32,214,130,149,34,8,0,49,66,101,5,140,149,132,
    123,74,202,16,133,136,133,115,61,17,35,17,1,33,2,111,1,79,1,24,254,24,254,254,23,1,25,2,254,2,178,252,104,253,232,2,24,3,152,131,97,36,80,0,0,4,140,130,97,34,9,0,68,133,97,140,84,71,
    209,18,35,178,4,0,2,130,218,36,176,7,16,178,5,68,54,8,35,178,9,5,7,130,20,62,48,49,37,33,21,33,53,1,33,53,33,21,1,130,3,10,251,196,2,241,253,20,4,31,202,202,164,4,64,204,160,131,111,
    48,132,254,188,2,28,6,142,0,7,0,34,0,176,4,47,176,7,75,113,15,87,91,9,130,194,53,35,17,51,21,33,17,33,2,28,165,165,254,104,1,152,5,208,249,169,189,7,210,131,71,36,20,255,131,3,100,
    130,183,37,3,0,19,0,176,2,79,169,5,32,0,84,111,6,54,48,49,19,51,1,35,20,240,2,96,240,5,176,249,211,0,1,0,12,254,188,1,166,132,117,32,37,132,45,35,1,47,176,2,139,191,33,176,1,75,106,
    13,32,19,130,116,49,53,51,17,35,12,1,154,254,102,167,167,6,142,248,46,189,6,87,131,119,36,53,2,217,3,53,130,119,34,6,0,39,67,39,12,70,211,9,36,208,178,1,7,3,86,66,5,34,176,5,208,130,
    196,34,3,35,1,130,142,49,1,181,178,206,1,43,171,1,42,205,4,166,254,51,2,215,253,41,130,151,42,3,255,65,3,152,0,0,0,3,0,27,65,125,5,67,84,10,93,206,7,45,48,49,5,33,53,33,3,152,252,107,
    3,149,191,191,131,131,37,49,4,209,2,9,6,131,53,41,36,0,176,1,47,178,15,1,1,93,82,18,5,41,180,15,3,31,3,2,93,178,0,1,131,129,36,25,176,0,47,24,130,128,8,34,35,1,33,2,9,202,254,242,1,
    21,4,209,1,47,0,0,2,0,90,255,236,3,251,4,78,0,30,0,41,0,133,178,23,42,43,131,175,32,23,71,11,9,37,23,47,27,177,23,27,89,153,6,66,239,12,79,38,8,34,2,23,4,130,52,33,178,11,132,6,37,
    176,11,47,176,23,16,79,43,11,34,18,11,15,130,30,46,64,9,12,18,28,18,44,18,60,18,4,93,176,4,16,84,158,11,36,11,16,178,35,7,137,227,49,33,38,39,6,35,34,38,53,52,36,51,51,53,52,38,35,
    34,6,88,212,7,41,23,17,20,23,21,37,50,54,55,53,95,162,5,8,62,3,3,16,12,116,168,163,206,1,1,239,149,94,96,83,106,243,118,203,125,190,226,3,41,253,253,72,127,32,131,135,136,93,31,70,
    121,186,137,173,185,71,84,101,83,64,89,155,88,191,173,254,24,146,87,17,175,70,59,204,94,86,70,83,130,253,46,124,255,236,4,50,6,0,0,15,0,27,0,100,178,19,90,151,5,32,19,72,159,5,32,9,
    66,74,5,36,12,47,27,177,12,65,0,7,77,50,8,68,122,12,34,5,12,3,130,225,33,178,10,132,6,32,176,79,113,16,90,181,12,47,20,2,35,34,39,7,35,17,51,17,54,51,50,18,17,39,131,224,35,7,17,22,
    51,130,213,8,47,4,50,225,197,190,106,12,220,243,105,178,198,226,243,124,118,158,64,65,159,114,124,2,2,18,252,254,214,137,117,6,0,253,210,124,254,218,254,248,7,176,176,138,254,66,141,
    170,172,89,177,5,43,3,245,4,78,0,28,0,75,178,0,29,30,69,73,8,32,15,130,180,32,15,135,180,82,169,8,84,37,9,35,178,3,8,15,130,44,35,178,19,15,8,130,6,36,176,15,16,178,22,67,193,8,34,
    48,49,37,130,140,33,51,14,130,168,39,0,17,53,52,0,51,50,22,80,24,10,8,70,2,57,91,120,4,229,4,118,202,117,227,254,246,1,8,228,193,243,6,229,4,119,92,118,128,1,127,174,106,78,101,175,
    102,1,38,1,3,25,247,1,41,225,183,93,120,171,174,39,176,173,0,0,2,0,79,255,236,4,3,6,0,0,14,0,25,0,100,178,23,26,27,131,117,39,23,16,176,3,208,0,176,6,65,103,5,32,3,130,178,32,3,135,
    178,32,12,130,12,32,12,75,221,7,136,191,34,5,3,12,130,55,33,178,10,132,6,33,176,12,85,249,12,32,3,72,187,13,8,68,19,52,18,51,50,23,17,51,17,35,39,6,35,34,2,55,20,22,51,50,55,17,38,
    35,34,6,79,232,195,172,106,243,220,12,109,182,190,235,243,127,117,149,69,67,149,118,128,2,37,250,1,47,120,2,42,250,0,112,132,1,50,242,165,185,133,1,206,130,187,131,185,32,83,130,185,
    44,11,4,78,0,21,0,29,0,131,178,22,30,31,130,129,36,176,22,16,176,8,73,107,6,132,156,135,182,67,18,8,34,26,0,8,131,39,51,26,47,180,191,26,207,26,2,93,180,95,26,111,26,2,113,180,31,26,
    47,131,6,34,239,26,255,130,6,38,178,140,26,1,93,178,12,66,250,8,32,176,82,78,12,40,178,18,8,0,17,18,57,176,8,65,155,13,35,5,34,0,53,86,209,5,36,18,17,21,33,22,131,214,8,64,23,6,6,3,
    34,6,7,33,53,38,38,2,89,231,254,225,125,226,139,221,241,253,61,11,157,119,167,105,131,65,217,164,100,123,17,1,207,8,114,20,1,35,242,30,162,255,142,254,230,254,254,98,134,156,135,125,
    97,107,3,159,140,125,18,122,125,72,197,6,43,2,214,6,21,0,20,0,83,178,7,21,22,66,73,8,132,223,32,33,67,255,11,143,236,37,176,4,16,176,16,208,82,101,11,33,1,208,131,181,81,186,13,34,
    35,53,51,130,182,8,60,51,50,23,7,38,35,34,21,21,51,21,35,17,210,165,165,200,180,64,72,6,40,53,174,220,220,3,134,180,99,180,196,18,190,8,179,96,180,252,122,0,0,2,0,82,254,86,4,12,4,
    78,0,25,0,36,0,131,178,34,37,38,130,147,36,176,34,16,176,11,76,59,11,135,140,36,6,47,27,177,6,135,12,32,11,130,12,33,11,17,134,179,32,23,130,12,39,23,15,62,89,178,5,3,23,131,65,80,
    236,12,34,178,15,17,131,20,33,178,21,133,27,32,23,88,62,11,35,176,3,16,178,92,164,11,66,82,5,32,55,79,177,6,39,39,55,22,51,50,54,53,53,66,93,14,8,75,82,237,196,185,106,11,219,254,247,
    225,119,227,59,115,112,164,121,140,105,175,190,241,242,133,118,147,71,69,147,120,133,2,37,252,1,45,129,109,251,231,213,246,99,80,146,133,131,127,73,117,1,46,246,163,187,126,1,220,123,
    190,0,1,0,121,0,0,3,248,6,0,0,16,0,66,178,10,73,247,6,32,16,67,31,5,32,2,87,148,6,37,176,0,69,88,176,13,92,22,7,68,135,11,36,176,2,16,178,10,67,198,10,39,1,54,51,32,19,17,35,17,68,
    98,5,62,35,17,51,1,108,119,182,1,90,5,243,97,94,146,72,243,243,3,196,138,254,117,253,61,2,186,112,93,130,252,251,130,116,47,0,2,0,125,0,0,1,144,5,213,0,3,0,13,0,62,103,85,23,65,102,
    7,32,1,77,175,6,130,119,38,176,12,208,176,12,47,178,103,94,9,81,34,6,103,88,9,62,127,243,243,254,254,71,132,72,72,132,71,4,58,1,25,56,74,74,56,55,73,73,0,0,2,255,181,254,75,1,133,130,
    115,40,12,0,22,0,73,178,3,23,24,96,27,6,76,205,7,69,128,12,32,4,130,115,35,4,17,62,89,95,21,10,42,176,12,16,176,21,208,176,21,47,178,15,88,91,8,47,48,49,1,17,20,6,35,34,39,53,22,51,
    50,55,17,3,137,135,41,122,165,159,67,62,38,48,121,3,21,135,140,41,251,102,166,175,17,192,9,132,4,163,137,150,41,1,0,125,0,0,4,54,6,0,0,81,109,13,67,8,7,67,245,12,103,149,12,81,109,
    9,32,8,81,109,12,133,13,81,109,20,8,42,1,220,108,243,243,76,1,43,1,36,254,110,1,189,254,231,1,208,111,254,159,6,0,252,138,95,1,81,254,61,253,137,0,1,0,140,0,0,1,127,6,0,0,82,151,13,
    135,141,68,118,7,65,116,6,34,127,243,243,130,42,130,53,44,124,0,0,6,121,4,78,0,29,0,119,178,4,86,41,10,69,106,12,36,7,47,27,177,7,67,211,12,135,12,101,61,12,32,21,130,38,32,21,81,111,
    17,40,3,27,17,18,57,178,5,7,21,74,8,6,87,14,10,40,24,208,48,49,1,23,54,51,50,131,3,33,22,23,66,136,6,8,62,6,7,19,35,17,38,35,34,7,17,35,17,1,97,7,114,198,217,80,118,214,179,175,2,243,
    90,104,83,105,21,1,243,5,190,146,61,243,4,58,113,133,166,166,198,193,253,57,2,192,103,96,89,72,253,26,2,200,191,119,252,240,4,58,67,35,7,130,213,36,16,0,83,178,11,77,27,10,140,213,
    32,0,130,174,32,0,135,200,32,14,130,12,32,14,135,187,75,166,8,34,1,14,3,66,105,5,33,178,11,67,52,11,130,173,67,53,12,44,1,94,7,120,195,1,82,6,243,89,101,147,72,130,153,44,125,145,254,
    125,253,53,2,189,103,99,133,252,254,130,144,70,221,6,32,61,130,145,40,15,0,26,0,67,178,12,27,28,104,124,5,33,176,24,79,143,11,70,218,15,92,182,11,32,4,97,208,13,32,19,92,177,5,45,23,
    23,20,6,6,35,34,0,53,23,20,22,50,54,73,10,5,8,45,79,126,228,148,219,1,17,11,1,123,229,150,229,254,237,243,138,246,137,141,121,119,140,2,39,159,255,137,254,230,233,57,160,252,138,1,
    49,254,9,167,189,192,185,164,192,189,130,155,36,124,254,96,4,48,134,155,34,110,178,19,133,155,72,225,6,35,0,69,88,176,67,93,12,36,9,47,27,177,9,69,82,12,69,69,7,74,141,8,72,235,48,
    36,17,35,17,51,23,72,235,15,8,46,4,48,228,192,178,107,243,224,10,107,184,198,225,242,129,120,149,65,66,150,116,131,2,18,251,254,213,117,253,255,5,218,110,130,254,217,254,250,6,162,
    190,123,254,32,126,187,0,130,199,32,79,130,199,32,2,130,199,36,14,0,25,0,107,72,65,14,105,19,8,140,186,72,254,12,135,199,84,41,8,72,75,12,65,139,11,72,72,20,33,55,51,130,198,69,234,
    14,8,42,79,232,198,181,106,14,216,243,106,170,194,234,243,131,116,144,70,70,142,116,133,2,38,254,1,42,127,107,250,38,1,252,112,1,47,246,166,189,123,1,236,118,186,67,139,5,33,2,180,
    130,191,38,13,0,70,178,9,14,15,71,95,13,70,184,12,135,12,32,5,86,48,6,33,176,11,106,219,12,34,9,11,5,130,60,34,48,49,1,67,68,6,61,51,23,54,51,50,23,2,179,48,51,167,58,243,232,6,88,
    156,52,34,3,92,8,128,253,28,4,58,121,141,14,130,121,36,75,255,236,3,202,130,121,38,38,0,105,178,9,39,40,81,123,13,135,108,32,28,130,108,39,28,15,62,89,178,2,28,9,130,33,39,176,2,16,
    176,22,208,176,9,90,223,11,35,178,13,22,16,130,26,43,180,12,13,28,13,2,93,176,28,16,178,36,67,55,8,35,178,33,36,2,131,27,37,3,33,19,33,2,93,107,85,10,33,51,50,107,82,11,32,22,102,84,
    6,33,38,53,98,118,5,8,61,2,219,107,248,83,182,236,182,194,239,243,104,86,80,101,94,1,30,163,79,242,196,133,208,116,236,5,120,99,96,100,1,38,65,68,52,40,88,167,140,188,192,153,70,93,
    74,62,56,62,63,87,122,87,146,181,96,168,97,86,93,73,0,130,219,44,8,255,236,2,114,5,65,0,20,0,82,178,0,72,181,10,32,19,130,206,32,19,135,219,71,44,8,38,19,16,176,1,208,176,0,130,2,34,
    47,176,1,95,38,11,33,176,13,89,209,11,72,208,5,130,196,8,52,17,51,21,35,17,20,22,51,50,55,21,6,35,32,17,17,35,53,51,17,1,173,191,191,49,63,42,43,83,77,254,232,178,178,5,65,254,249,
    180,253,164,62,55,10,188,23,1,53,2,101,180,1,7,130,149,42,119,255,236,3,247,4,58,0,16,0,83,71,197,8,35,0,69,88,176,69,106,12,132,149,135,162,70,66,12,32,15,130,188,39,15,15,62,89,178,
    0,2,13,65,139,5,71,214,12,41,37,6,35,34,38,53,17,51,17,20,130,152,130,6,58,35,3,12,107,197,176,181,243,171,177,62,243,229,106,126,206,195,2,189,253,70,206,127,3,9,251,198,130,139,36,
    22,0,0,3,218,130,139,81,205,20,66,106,12,76,227,15,34,178,0,5,81,205,6,53,19,51,1,35,1,51,1,250,229,251,254,137,211,254,134,252,1,52,3,6,251,198,130,84,130,95,36,33,0,0,5,204,130,10,
    81,203,20,67,130,12,66,228,20,77,80,21,33,0,11,131,121,33,178,5,133,6,32,10,132,6,34,48,49,1,131,135,33,3,3,130,138,8,35,19,19,51,4,51,172,237,254,217,200,232,228,200,254,216,237,175,
    222,183,1,79,2,235,251,198,2,231,253,25,4,58,253,29,2,227,0,130,155,32,31,130,251,32,232,130,155,81,201,13,135,135,36,10,47,27,177,10,78,217,20,32,7,81,201,8,38,10,4,17,18,57,178,6,
    133,6,81,201,15,38,19,33,1,1,33,3,3,131,5,46,2,1,206,1,14,254,181,1,86,254,244,216,215,254,242,130,7,50,182,1,12,2,214,1,100,253,235,253,219,1,114,254,142,2,37,2,21,130,145,36,12,254,
    75,3,214,130,145,36,15,0,63,178,0,90,41,10,77,175,12,32,5,130,126,39,5,17,62,89,178,0,5,15,77,157,5,36,176,1,208,176,5,97,32,11,34,48,49,1,130,125,8,48,2,35,34,39,53,23,50,54,55,55,
    1,33,1,247,220,1,3,254,82,99,237,53,64,46,92,93,27,35,254,132,1,6,1,92,2,222,251,34,254,239,18,188,3,67,79,93,4,53,0,130,129,36,82,0,0,3,192,130,129,81,229,13,66,135,15,84,170,10,81,
    229,40,49,128,2,64,252,146,2,37,253,229,3,79,194,194,159,2,215,196,154,131,111,46,56,254,152,2,145,6,61,0,23,0,54,178,18,24,25,130,207,34,0,176,12,74,209,5,32,0,130,231,33,0,23,130,
    231,34,6,0,12,130,23,36,176,6,47,178,5,77,12,8,35,178,18,5,6,68,231,5,8,36,36,3,53,52,35,53,50,53,53,54,54,55,23,6,7,21,20,7,22,21,21,22,23,2,97,254,159,7,193,193,3,181,176,48,173,
    6,173,130,2,55,254,152,99,1,96,213,225,178,226,212,180,222,50,140,56,250,216,225,91,92,227,213,250,56,131,133,36,174,254,242,1,85,106,173,24,43,1,35,17,51,1,85,167,167,254,242,6,190,
    131,45,32,27,130,179,32,117,130,179,32,24,130,179,34,5,25,26,132,179,32,11,133,179,32,24,130,179,32,24,131,179,34,17,24,11,130,23,36,176,17,47,178,18,137,179,34,5,18,17,132,179,44,
    23,54,55,53,52,55,38,53,53,38,39,55,22,130,173,8,61,20,51,21,34,21,21,20,6,7,27,176,4,182,182,4,176,48,182,178,194,194,179,181,219,57,255,208,231,86,86,234,207,255,57,140,51,229,185,
    200,225,178,225,197,187,229,51,0,1,0,117,1,131,4,220,3,47,0,23,0,63,178,17,65,53,6,37,15,47,178,3,24,15,130,95,36,176,3,47,176,15,68,176,12,38,3,16,176,11,208,176,3,89,234,12,45,15,
    16,176,23,208,48,49,1,20,6,35,34,46,2,81,69,6,8,63,51,50,30,2,51,50,54,53,4,220,190,142,74,125,154,67,38,67,77,193,182,148,74,133,145,67,39,67,84,3,18,176,223,56,137,33,104,84,171,
    219,59,132,34,112,84,0,2,0,134,254,148,1,153,4,77,0,3,0,15,0,62,178,7,16,131,222,36,176,7,16,176,0,72,217,6,68,196,12,44,3,47,27,177,3,23,62,89,176,13,16,178,7,99,230,14,37,48,49,19,
    51,19,33,132,140,8,52,38,53,52,54,51,50,22,170,209,24,254,255,1,7,72,65,66,72,72,66,65,72,2,150,251,254,5,55,56,75,75,56,55,75,75,0,1,0,100,255,11,4,10,5,38,0,32,0,93,178,27,33,34,
    107,45,13,67,194,12,92,102,13,34,178,3,10,132,164,34,10,16,176,115,140,5,43,17,16,176,20,208,176,20,47,178,24,17,10,130,69,34,176,17,16,91,2,12,37,37,50,54,55,51,6,113,231,5,36,2,53,
    53,52,18,114,7,5,8,65,23,35,38,38,35,34,3,7,20,22,2,79,89,120,6,228,4,197,146,200,183,204,204,183,200,158,185,4,228,7,118,91,230,16,1,127,174,104,80,136,205,28,234,234,34,1,31,220,
    28,213,1,32,34,225,224,28,216,156,96,117,254,200,72,176,173,0,130,195,46,94,0,0,4,124,5,195,0,31,0,101,178,26,32,33,136,195,36,18,47,27,177,18,85,173,7,94,23,8,32,4,71,77,8,38,176,
    8,208,178,30,5,18,95,119,6,32,31,137,23,41,12,208,176,30,16,176,15,208,178,22,133,29,32,18,90,146,11,53,48,49,1,23,20,7,33,7,33,53,51,54,54,53,39,35,53,51,39,52,54,32,71,126,8,8,73,
    23,33,21,1,253,7,64,2,184,1,251,231,82,39,43,7,161,155,8,250,1,150,232,245,105,94,89,103,9,1,55,2,86,176,135,85,202,202,9,111,91,185,199,242,202,234,218,184,95,105,130,104,242,199,
    0,2,0,93,255,229,5,79,4,241,0,27,0,40,0,63,178,2,41,42,70,147,5,33,176,31,117,105,11,45,15,62,89,176,16,208,176,16,47,176,2,16,178,32,67,17,8,36,176,16,16,178,38,136,13,8,130,48,49,
    37,6,35,34,39,7,39,55,38,53,52,55,39,55,23,54,51,50,23,55,23,7,22,21,20,7,23,7,1,20,22,22,50,54,54,52,38,38,34,6,6,4,61,159,203,202,158,129,141,135,100,109,144,141,142,155,192,194,
    155,145,142,148,107,98,139,142,252,120,110,190,220,190,109,109,189,222,190,109,107,127,126,132,144,137,156,197,200,165,147,144,145,115,117,148,145,151,159,202,193,156,141,145,2,123,
    120,206,117,118,206,238,204,117,117,204,0,0,1,0,25,0,0,4,192,5,176,0,22,0,114,85,91,5,36,22,47,27,177,22,65,128,7,74,18,8,34,0,12,22,81,220,6,35,1,208,178,15,133,12,51,15,47,176,19,
    208,176,19,47,180,15,19,31,19,2,93,176,4,208,176,4,130,18,35,16,178,18,4,84,177,7,36,176,6,208,176,15,66,109,7,35,15,16,178,14,137,25,33,10,208,87,124,5,33,33,21,132,1,112,18,5,58,
    53,33,53,33,1,33,2,109,1,59,1,24,254,119,1,13,254,163,1,93,254,163,252,254,158,1,98,130,3,32,25,130,18,58,25,3,52,2,124,253,54,152,138,151,254,211,1,45,151,138,152,2,202,0,2,0,136,
    254,242,1,109,130,197,36,3,0,7,0,24,130,199,68,144,5,32,6,98,196,6,42,178,5,1,3,43,48,49,19,17,51,17,130,102,46,51,136,229,229,229,254,242,3,27,252,229,3,200,2,246,130,61,48,90,254,
    38,4,140,5,196,0,47,0,61,0,130,178,32,62,63,131,234,39,32,16,176,48,208,0,176,7,133,74,32,32,130,74,39,32,31,62,89,178,57,32,7,131,29,32,57,93,113,11,35,178,2,57,19,77,227,6,32,14,
    66,144,8,34,178,11,14,131,20,33,178,50,133,48,35,50,16,178,44,137,27,34,26,50,44,133,99,33,178,39,137,20,34,36,44,39,130,20,47,48,49,1,20,7,22,21,20,4,35,34,36,53,55,20,22,101,18,5,
    45,39,46,2,53,52,55,38,38,53,52,36,51,50,4,117,137,10,45,22,22,37,38,39,6,21,20,22,31,2,54,53,52,130,199,8,129,171,135,254,242,234,246,254,224,242,156,136,121,141,134,187,188,190,93,
    169,65,68,1,19,230,240,1,12,243,145,120,123,139,120,1,131,194,90,253,205,81,76,108,99,149,179,46,115,136,1,199,184,89,100,185,173,198,217,207,1,110,120,95,79,77,91,55,51,110,154,109,
    184,90,50,136,100,170,204,225,204,106,128,95,82,84,87,104,113,153,110,21,28,40,124,81,86,47,53,16,47,117,81,97,0,2,0,93,4,223,3,35,5,204,0,8,0,17,0,34,0,176,7,47,178,15,7,1,93,178,
    2,5,65,252,8,36,11,208,176,7,16,66,250,5,43,48,49,19,52,54,50,22,20,6,34,38,37,135,8,41,93,67,118,68,68,118,67,1,200,68,131,7,41,68,5,86,50,68,68,100,68,68,49,133,6,62,0,3,0,87,255,
    236,5,226,5,196,0,26,0,40,0,54,0,142,178,31,55,56,17,18,57,176,31,16,176,9,208,131,5,32,51,67,101,6,49,51,47,27,177,51,15,62,89,176,45,208,176,45,47,178,2,51,45,131,38,44,2,47,180,
    15,2,31,2,2,93,178,9,45,51,131,16,38,9,47,180,0,9,16,9,130,16,34,13,9,2,130,16,33,178,16,104,92,8,36,176,2,16,178,23,136,13,35,178,26,2,9,130,31,37,176,45,16,178,31,8,136,205,35,51,
    16,178,37,136,13,39,48,49,1,20,6,32,38,53,69,175,5,65,143,7,32,21,65,174,5,53,37,52,2,36,35,34,4,2,16,18,4,32,36,18,37,52,18,36,32,4,18,16,96,78,5,8,124,4,94,175,254,192,189,191,158,
    163,173,156,92,88,92,103,104,91,89,90,1,166,150,254,238,163,159,254,239,156,155,1,17,1,64,1,19,152,250,239,187,1,75,1,128,1,74,187,187,254,184,194,193,254,183,188,2,84,152,162,213,
    180,113,174,213,165,149,96,83,136,118,117,118,134,81,98,133,166,1,29,171,164,254,224,254,172,254,224,167,170,1,32,167,202,1,90,199,199,254,166,254,108,254,166,201,200,1,90,0,2,0,141,
    2,179,3,17,5,196,0,26,0,36,0,143,178,13,84,39,5,35,13,16,176,28,65,57,6,43,20,47,27,177,20,31,62,89,178,3,37,20,71,61,6,39,0,208,176,0,47,178,1,3,131,15,33,178,10,132,6,39,176,10,47,
    176,20,16,178,13,65,32,9,34,16,10,13,130,46,8,33,178,204,16,1,93,64,19,12,16,28,16,44,16,60,16,76,16,92,16,108,16,124,16,140,16,9,93,178,186,16,1,113,176,3,105,220,12,35,10,16,178,
    31,136,66,51,48,49,1,39,6,35,34,38,53,52,54,51,51,53,52,35,34,6,21,39,130,10,8,113,50,22,21,17,20,23,37,50,54,55,53,35,6,6,21,20,2,96,17,77,124,118,131,168,173,102,116,65,73,173,175,
    136,137,154,26,254,160,40,84,27,106,76,86,2,193,68,82,123,105,110,121,51,127,51,48,14,104,129,145,132,254,196,97,81,130,36,25,137,1,60,49,88,255,255,0,87,0,138,3,133,3,169,0,38,1,122,
    235,0,0,7,1,122,1,82,0,0,0,1,0,127,1,118,3,194,3,37,0,5,0,26,0,176,4,47,122,236,5,33,176,4,78,72,11,130,156,50,35,17,33,53,33,3,194,200,253,133,3,67,1,118,1,4,171,0,4,66,143,8,53,13,
    0,27,0,49,0,58,0,157,178,10,59,60,17,18,57,176,10,16,176,18,208,131,5,32,49,132,5,66,151,7,97,193,12,44,10,47,27,177,10,15,62,89,176,3,16,178,18,66,71,8,130,52,33,178,24,136,13,35,
    178,29,10,3,122,174,6,34,31,3,10,131,9,44,31,47,180,0,31,16,31,2,93,178,50,29,31,131,16,35,50,47,178,28,137,47,34,37,28,50,132,47,40,16,176,44,208,176,31,16,178,58,136,26,34,48,49,
    19,66,119,12,66,147,14,32,17,130,223,49,50,22,21,20,7,22,22,20,22,23,21,35,38,53,52,38,35,39,68,107,6,33,35,87,66,126,15,33,5,17,66,162,15,56,253,37,151,1,25,153,172,120,65,52,7,10,
    155,13,66,77,158,143,69,93,71,93,141,2,217,66,132,14,32,203,66,164,14,8,44,91,254,175,3,82,135,125,117,63,29,111,163,68,23,16,34,160,76,67,134,62,54,70,59,1,0,1,0,135,5,18,3,94,5,176,
    0,3,0,17,0,176,1,47,178,2,123,149,8,117,177,5,58,3,94,253,41,2,215,5,18,158,0,2,0,127,3,175,2,139,5,196,0,9,0,19,0,57,178,0,99,61,6,66,214,6,101,39,8,38,10,208,176,10,47,178,5,66,117,
    8,34,176,0,16,67,245,10,130,85,8,51,50,22,20,6,35,34,38,52,54,19,50,54,53,52,38,34,6,20,22,1,135,106,154,152,108,109,155,157,107,53,69,69,106,72,73,5,196,158,220,155,155,220,158,254,
    120,71,53,52,76,76,104,72,130,125,44,95,0,1,3,243,4,252,0,11,0,15,0,70,118,235,5,86,121,11,37,9,16,176,0,208,176,118,251,15,35,176,13,16,178,70,31,10,44,5,14,6,17,18,57,180,11,5,27,
    5,2,93,119,23,13,8,32,1,33,53,33,2,156,1,87,254,169,216,254,155,1,101,216,1,50,252,175,3,81,3,131,199,254,124,1,132,199,1,121,251,130,248,47,0,1,0,60,2,155,2,178,5,187,0,23,0,89,178,
    8,75,9,6,32,0,130,135,116,147,12,41,0,47,27,177,0,19,62,89,178,22,65,5,8,35,178,2,0,22,130,122,35,178,3,15,0,77,67,5,33,178,8,137,27,32,12,132,20,33,178,19,132,6,131,150,35,53,1,54,
    53,92,106,8,8,47,51,50,22,21,20,15,2,33,2,178,253,156,1,29,113,54,52,58,66,186,169,135,143,156,106,98,140,1,115,2,155,125,1,5,103,67,42,53,66,54,116,153,128,115,107,102,87,113,130,
    165,46,55,2,143,2,169,5,186,0,36,0,125,178,30,37,38,73,233,8,32,13,130,152,32,13,72,104,7,32,23,130,12,32,23,131,165,34,1,23,13,130,33,55,124,176,1,47,24,182,64,1,80,1,96,1,3,113,178,
    144,1,1,93,176,13,16,178,6,137,166,33,9,1,131,39,33,176,1,110,48,11,35,178,18,35,1,130,60,33,178,27,132,67,36,176,23,16,178,30,136,48,130,201,33,51,50,142,199,32,7,81,114,6,104,97,
    7,8,54,52,39,35,1,12,81,132,54,62,48,65,186,165,130,143,163,135,149,177,143,135,171,186,69,60,63,61,134,92,4,108,97,35,53,39,35,99,124,121,105,119,51,41,142,106,126,127,113,38,53,55,
    42,101,1,0,130,227,49,112,4,209,2,72,6,0,0,3,0,35,0,176,2,47,178,15,2,130,176,54,0,208,176,0,47,180,15,0,31,0,2,93,176,2,16,176,3,208,25,176,3,47,24,130,137,45,33,1,35,1,51,1,21,254,
    235,195,6,0,254,209,130,63,44,146,254,96,4,31,4,58,0,18,0,96,178,13,121,253,6,34,0,69,88,130,63,33,27,177,85,170,8,33,7,47,86,141,10,32,16,130,12,33,16,17,89,199,6,88,242,12,68,104,
    8,32,13,81,198,11,34,178,11,13,96,123,5,35,1,17,22,22,81,28,6,8,40,39,6,35,34,39,17,35,17,1,132,2,89,106,168,59,243,223,7,92,147,121,77,242,4,58,253,132,141,130,121,3,18,251,198,86,
    107,55,254,62,5,218,115,245,5,41,3,86,5,176,0,10,0,43,178,2,121,33,10,98,138,12,91,227,8,8,38,1,0,8,17,18,57,48,49,33,17,35,34,36,53,52,36,51,33,17,2,132,80,230,254,247,1,10,230,1,
    33,2,8,254,214,213,255,250,80,0,130,87,38,142,2,69,1,169,3,82,130,87,34,22,178,8,134,87,45,2,47,177,8,10,43,88,216,27,220,89,48,49,19,88,159,6,51,35,34,38,142,74,134,75,78,64,65,76,
    2,202,58,78,78,58,59,74,74,130,63,46,109,254,65,1,201,0,3,0,14,0,52,178,9,15,16,130,117,93,53,8,86,218,8,35,6,16,177,7,134,79,35,178,13,7,14,130,35,34,178,1,13,131,6,34,48,49,37,66,
    3,5,8,32,39,50,54,53,52,38,39,55,1,62,11,150,172,155,7,66,71,71,80,32,3,54,27,146,105,118,137,47,42,45,35,5,139,130,105,44,128,2,160,2,2,5,179,0,6,0,57,178,1,114,157,15,120,163,12,
    38,19,62,89,178,4,5,0,104,131,5,33,178,3,66,131,11,44,35,17,7,53,37,51,2,2,185,201,1,111,19,130,80,53,58,48,146,119,0,2,0,119,2,178,3,44,5,196,0,12,0,26,0,64,178,9,86,137,5,34,9,16,
    176,89,231,7,32,2,122,61,6,130,26,32,2,72,202,5,72,188,24,37,48,49,19,52,54,32,126,163,14,32,55,66,249,6,8,63,119,191,1,54,192,188,157,158,190,175,93,80,78,91,1,93,79,78,93,4,97,160,
    195,194,166,72,159,195,196,163,5,98,110,108,97,80,97,110,109,102,0,255,255,0,93,0,138,3,153,3,169,0,38,1,123,9,0,0,7,1,123,1,126,0,131,23,46,89,0,0,5,131,5,171,0,39,1,213,255,217,2,
    152,130,7,46,124,1,27,0,8,1,7,1,216,2,197,0,0,0,16,118,21,13,37,48,49,255,255,0,80,130,51,34,204,5,174,131,43,35,0,240,0,8,132,59,34,208,2,155,130,51,34,214,3,26,137,51,32,9,130,230,
    35,9,31,62,89,132,51,32,103,130,51,34,252,5,187,132,95,32,168,132,51,44,216,3,62,0,0,1,7,1,215,0,48,2,155,135,103,75,153,7,53,48,49,0,2,0,66,254,127,3,165,4,78,0,25,0,35,0,97,178,16,
    36,37,115,62,5,33,176,29,70,24,6,32,33,130,96,32,33,79,33,7,32,16,130,12,40,16,23,62,89,176,33,16,178,29,79,147,14,35,178,3,0,16,115,122,24,34,22,16,0,130,27,39,48,49,1,6,6,7,7,6,73,
    244,6,33,51,6,72,206,5,8,37,55,55,54,55,55,19,20,6,34,38,53,52,54,50,22,2,118,2,53,73,103,90,98,89,88,106,243,2,239,194,206,226,155,92,78,10,2,247,91,96,5,54,2,149,124,145,79,106,97,
    106,94,93,100,83,177,208,201,184,165,163,93,72,115,53,1,79,234,8,47,0,2,255,246,0,0,7,87,5,176,0,15,0,18,0,119,103,67,18,107,63,12,102,232,8,32,17,109,228,5,35,17,47,178,2,105,150,
    9,32,6,81,23,11,33,178,11,133,34,33,11,47,109,116,10,33,176,0,70,161,12,32,18,132,34,36,48,49,33,33,3,130,1,36,1,33,21,33,19,132,3,8,70,1,33,3,7,87,252,126,15,254,10,184,254,222,3,
    67,3,224,253,122,17,2,36,253,228,20,2,151,250,237,1,121,27,1,84,254,172,5,176,197,254,104,197,254,54,1,103,2,136,0,0,1,0,77,0,214,3,236,4,134,0,11,0,56,0,176,3,47,178,9,12,3,66,200,
    6,33,10,9,131,9,35,178,4,3,9,130,16,35,178,1,10,4,130,6,41,176,3,16,176,5,208,178,7,4,10,132,36,62,16,176,11,208,48,49,19,1,1,55,1,1,23,1,1,7,1,1,77,1,60,254,196,148,1,59,1,60,148,
    254,196,132,4,42,254,197,1,108,1,66,1,66,150,254,190,132,4,59,254,190,150,1,65,254,191,0,0,3,0,105,255,161,5,34,5,238,0,23,0,32,0,41,0,102,178,16,99,201,5,37,16,16,176,29,208,176,130,
    5,32,38,117,149,19,65,75,8,33,26,16,131,145,33,178,35,132,6,36,176,35,16,176,27,131,52,24,64,238,10,42,176,26,16,176,36,208,176,4,16,178,38,106,245,13,40,4,35,34,39,7,35,55,38,17,114,
    67,5,8,108,23,55,51,7,22,19,5,20,23,1,38,35,34,2,7,5,52,39,1,22,51,50,18,53,5,34,148,254,237,180,164,132,91,169,145,195,150,1,20,178,197,143,87,167,147,157,1,252,68,71,1,246,87,135,
    164,185,2,2,191,44,254,23,78,105,169,181,2,178,214,254,189,173,75,150,238,195,1,103,67,213,1,68,175,101,143,243,193,254,195,75,207,128,3,58,85,254,255,235,8,166,114,252,220,54,1,0,
    246,0,0,114,101,5,45,126,5,176,0,12,0,20,0,87,178,2,21,22,17,114,101,5,32,15,73,36,15,35,0,69,88,176,70,52,7,35,178,1,10,0,131,39,35,1,47,178,14,133,9,33,14,47,94,79,11,32,1,114,118,
    13,45,1,17,51,50,4,21,20,4,35,35,17,35,17,19,130,12,62,54,52,38,39,1,135,241,244,1,18,254,238,243,242,243,243,246,125,145,140,122,5,176,254,232,238,200,199,239,254,212,130,9,57,37,
    254,26,130,222,132,2,0,0,1,0,136,255,236,4,155,6,21,0,44,0,91,178,35,45,46,72,25,8,36,5,47,27,177,5,93,192,7,93,92,12,70,96,8,34,14,5,21,123,70,5,33,178,28,65,115,8,35,178,34,21,5,
    107,237,5,32,178,24,68,197,11,34,33,35,17,76,88,5,37,20,14,2,21,20,30,130,3,38,6,35,34,38,39,55,22,79,90,5,35,46,2,53,52,72,224,5,8,91,7,1,122,242,229,206,187,215,27,69,22,65,178,81,
    217,198,80,171,38,49,45,127,54,97,90,70,174,81,126,92,80,184,4,4,81,214,238,187,169,62,98,113,65,39,44,84,148,137,75,171,185,39,25,195,28,37,86,67,49,91,136,136,80,88,201,77,81,97,
    247,0,0,3,0,72,255,236,6,132,4,80,0,41,0,52,0,60,0,202,178,2,61,62,65,123,6,38,45,208,176,2,16,176,56,65,129,6,102,63,12,90,210,8,40,0,208,176,0,47,178,12,5,23,131,51,40,12,47,178,
    143,12,1,93,176,23,90,109,11,130,13,41,176,27,208,176,27,47,178,56,0,27,131,37,44,56,47,180,31,56,47,56,2,113,180,239,56,255,131,6,34,95,56,111,131,6,42,191,56,207,56,2,93,178,140,
    56,1,93,82,120,11,35,0,16,178,35,65,53,8,34,176,5,16,65,46,10,36,176,12,16,178,47,82,148,8,36,176,27,16,178,53,136,41,34,48,49,5,119,126,6,32,53,77,180,5,32,38,77,181,8,37,23,54,23,
    50,18,21,99,115,6,35,55,23,6,6,77,193,9,8,125,22,1,34,6,7,33,53,52,38,4,230,253,140,65,214,134,176,200,238,233,191,95,88,91,115,242,253,197,223,111,131,200,212,238,253,73,9,152,134,
    137,107,61,73,70,209,252,152,58,136,45,196,104,120,93,3,43,99,127,16,1,196,109,20,161,77,84,176,156,158,172,71,91,103,89,66,19,146,185,133,135,2,254,253,235,137,139,158,58,34,166,56,
    64,184,59,43,209,2,95,70,65,79,2,231,138,127,30,113,122,0,2,0,103,255,236,4,64,6,44,0,29,0,43,0,101,178,7,124,65,5,35,7,16,176,40,65,115,6,36,25,47,27,177,25,66,91,7,32,7,130,12,49,
    7,15,62,89,178,15,7,25,17,18,57,176,15,47,178,17,15,7,131,9,32,25,94,117,11,33,176,15,98,237,11,34,176,7,16,127,181,10,8,37,48,49,1,18,17,21,20,2,6,35,34,38,38,53,52,54,54,51,50,23,
    38,39,7,39,55,38,39,55,22,23,55,23,3,39,38,38,35,34,70,36,7,8,88,3,66,254,126,229,140,138,226,126,113,206,132,146,113,49,126,204,78,172,126,162,75,238,177,180,78,143,1,32,123,78,126,
    139,141,110,111,137,5,23,254,247,254,111,82,166,254,249,146,126,226,136,149,231,125,91,169,122,135,109,114,82,42,195,50,135,120,109,253,25,18,48,56,168,149,126,168,200,173,0,0,3,0,
    67,0,147,4,55,4,204,130,9,38,13,0,25,0,82,178,4,102,63,5,37,4,16,176,0,208,176,130,5,32,17,130,247,123,4,13,35,3,16,177,9,73,13,7,32,4,70,246,10,34,16,177,17,135,22,32,23,136,22,130,
    222,35,33,53,33,1,77,38,8,32,3,67,74,6,131,238,45,4,55,252,12,3,244,254,9,68,74,74,68,67,74,130,0,32,67,133,9,51,2,70,212,1,178,76,114,75,75,114,76,252,74,58,76,76,58,57,74,74,130,
    159,50,79,255,119,4,61,4,187,0,21,0,29,0,37,0,102,178,4,38,39,73,33,5,33,176,27,132,169,32,35,96,17,19,91,239,8,34,24,4,15,130,45,33,178,32,132,6,36,176,32,16,176,25,131,52,86,174,
    10,39,176,24,16,176,33,208,176,15,66,219,11,33,48,49,96,46,5,39,23,55,51,7,22,17,20,6,130,181,37,39,7,35,55,38,19,69,131,5,32,6,69,130,6,8,86,54,79,126,228,148,106,88,71,145,102,196,
    123,229,150,93,90,72,145,102,206,243,64,1,43,47,57,119,140,2,9,58,254,216,43,51,123,137,2,39,159,255,137,34,143,208,153,254,192,160,252,138,30,147,207,150,1,54,156,98,2,97,22,189,167,
    148,93,253,167,17,192,0,0,2,0,130,254,96,4,55,6,0,0,15,0,26,0,100,96,81,14,105,51,19,36,6,47,27,177,6,96,71,69,105,51,16,46,4,55,227,194,178,107,243,243,106,176,197,227,243,131,118,
    96,70,7,49,247,254,209,117,253,255,7,160,253,215,119,254,218,254,250,5,166,186,96,71,8,44,31,0,0,5,157,5,176,0,19,0,23,0,107,72,107,5,78,74,12,104,108,8,43,20,8,15,17,18,57,176,20,
    47,178,16,20,132,9,41,16,47,176,0,208,176,16,16,178,23,68,59,9,43,3,208,176,8,16,176,5,208,176,20,16,178,24,66,243,10,35,23,16,176,10,131,42,40,176,13,208,176,15,16,176,18,208,24,66,
    31,9,8,66,17,35,17,35,53,51,17,51,17,33,17,51,1,33,53,33,5,30,127,127,252,253,117,252,124,124,252,2,139,252,252,121,2,139,253,117,4,174,162,251,244,2,135,253,121,4,12,162,1,2,254,254,
    1,2,253,162,186,0,1,0,143,0,0,1,130,4,58,100,5,14,99,187,12,34,15,62,89,100,5,6,34,130,243,243,130,42,130,53,36,142,0,0,4,107,130,10,34,12,0,95,133,239,36,4,47,27,177,4,135,53,100,
    201,34,32,6,114,56,5,45,6,47,180,31,6,47,6,2,113,178,143,6,1,93,24,65,185,11,34,10,1,6,74,43,5,130,219,130,217,130,215,8,32,1,1,33,1,239,111,242,242,85,1,80,1,44,254,97,1,185,254,203,
    1,172,254,84,4,58,254,80,1,176,253,243,253,211,130,151,32,34,130,151,38,54,5,176,0,13,0,91,24,75,250,18,111,215,8,33,1,12,131,92,42,176,1,47,176,0,208,176,1,16,178,2,65,125,12,32,6,
    77,255,11,8,60,176,3,16,176,8,208,176,9,208,176,0,16,176,11,208,176,10,208,48,49,1,55,21,7,17,33,21,33,17,7,53,55,17,51,1,161,234,234,2,149,252,110,130,130,253,3,103,71,147,71,253,
    246,202,2,135,39,147,39,2,150,0,94,203,5,39,2,46,6,0,0,11,0,74,133,145,36,10,47,27,177,10,104,252,12,36,15,62,89,178,1,73,177,5,151,145,34,208,176,7,132,128,33,9,208,130,140,134,128,
    32,35,134,126,52,154,148,148,243,134,134,243,3,121,53,146,53,253,25,2,144,47,146,47,2,222,130,121,46,144,254,75,5,9,5,176,0,19,0,103,178,6,20,21,120,65,21,113,239,17,98,213,15,36,176,
    0,69,88,176,78,18,8,32,4,94,49,11,35,178,13,0,12,130,86,34,178,18,14,75,215,6,103,31,5,37,55,22,51,50,53,53,118,121,6,8,36,5,9,190,169,70,60,14,40,58,123,253,129,252,252,2,127,5,176,
    250,24,183,198,17,199,12,184,49,4,21,251,235,5,176,251,236,4,20,130,173,46,126,254,75,4,6,4,78,0,26,0,97,178,21,27,28,136,173,101,129,25,36,10,47,27,177,10,135,173,24,75,136,8,34,1,
    24,3,130,59,34,176,10,16,110,46,10,34,176,3,16,24,67,248,10,102,65,6,33,22,23,138,173,104,207,8,8,61,1,92,13,115,196,176,181,1,187,166,69,58,14,40,59,124,93,105,145,75,243,4,58,150,
    170,214,210,253,27,180,194,17,198,12,176,2,217,120,112,103,252,224,4,58,0,2,0,100,255,236,7,45,5,196,0,23,0,35,0,145,178,1,77,55,5,35,1,16,176,26,119,63,19,32,14,130,176,32,14,78,240,
    7,96,164,12,73,158,7,33,176,14,99,28,12,34,18,0,14,131,203,33,18,47,138,189,33,176,0,108,148,11,131,217,110,10,9,34,176,12,16,75,94,10,35,48,49,33,33,118,59,5,40,17,52,18,36,51,50,
    23,33,21,124,80,6,32,5,108,182,6,8,92,7,17,20,22,7,45,252,157,167,121,167,254,247,148,2,145,1,11,168,123,167,3,92,253,76,2,86,253,170,2,187,251,125,99,104,114,91,161,175,1,178,20,147,
    1,13,170,1,58,172,1,18,150,20,204,254,110,200,254,64,28,13,4,56,14,207,188,254,202,193,209,0,0,3,0,91,255,236,6,242,4,79,0,30,0,42,0,50,0,155,178,25,51,52,72,13,5,39,176,36,208,176,
    25,16,176,46,107,95,19,68,38,12,36,23,47,27,177,23,103,79,7,32,27,130,12,32,27,130,12,35,178,5,8,23,130,71,35,178,47,23,8,130,6,44,176,47,47,180,31,47,47,47,2,113,178,140,47,108,229,
    13,73,230,12,33,178,25,132,53,36,176,34,208,176,3,72,114,11,43,176,43,208,48,49,19,52,0,51,50,23,54,73,119,10,39,54,55,23,6,6,35,34,39,130,3,35,0,17,23,20,24,70,186,9,32,37,73,128,
    6,8,104,91,1,15,224,249,134,65,183,109,214,238,253,86,11,145,117,89,143,71,79,71,205,120,247,140,134,246,227,254,242,242,134,121,119,134,135,120,117,136,3,225,85,120,20,1,181,113,2,
    39,248,1,47,177,84,94,1,254,253,236,136,139,158,42,50,158,63,65,174,174,1,45,1,2,9,170,186,185,192,166,190,186,186,137,121,25,111,122,0,0,1,0,139,0,0,2,149,6,21,0,12,0,50,178,3,13,
    116,181,9,36,4,47,27,177,4,105,138,15,32,176,67,124,12,37,48,49,51,17,52,54,108,252,7,56,17,139,194,176,63,89,25,42,50,163,4,156,182,195,21,185,11,186,251,104,0,2,0,81,255,121,223,
    5,38,22,0,30,0,91,178,0,121,223,5,32,23,65,140,6,70,163,12,66,130,7,40,178,5,15,0,17,18,57,176,5,96,15,15,66,119,13,33,5,16,127,244,12,47,5,32,0,17,53,33,38,38,35,34,7,7,39,55,54,51,
    130,14,8,81,21,20,2,4,39,50,54,55,33,21,20,22,2,184,254,220,254,189,3,208,5,223,204,167,151,52,49,33,176,218,1,58,1,107,162,254,229,169,150,190,18,253,47,186,20,1,96,1,73,137,224,240,
    52,19,198,15,72,254,139,254,183,107,195,254,195,175,212,218,189,31,185,191,0,1,255,228,254,75,2,211,6,21,130,195,34,113,178,20,126,121,10,36,21,47,27,177,21,65,37,7,32,16,130,12,32,
    16,70,129,7,32,29,130,12,32,29,100,137,12,36,17,62,89,176,29,119,46,12,130,194,105,164,9,42,176,0,16,176,14,208,176,15,208,176,21,141,217,33,1,35,68,38,7,37,22,51,50,53,17,35,110,114,
    11,8,75,7,21,51,2,132,201,181,164,72,54,15,7,68,18,120,165,165,194,177,61,91,25,38,59,157,1,201,3,134,252,53,176,192,17,191,3,10,174,3,202,180,98,182,195,21,188,10,173,103,0,2,0,88,
    255,236,5,170,6,46,0,24,0,38,0,91,178,4,39,40,17,18,57,176,4,16,176,73,197,7,32,13,85,221,11,79,75,8,34,15,13,4,75,103,6,32,22,88,169,8,34,176,13,16,77,196,10,130,67,76,156,10,33,48,
    49,123,134,12,42,51,50,23,54,54,53,51,20,6,7,22,122,63,15,36,16,148,254,237,180,122,59,8,48,255,162,79,76,187,121,124,87,4,253,184,168,164,185,2,185,168,79,59,7,34,173,1,64,123,146,
    5,50,168,13,131,130,164,209,35,167,223,18,246,254,254,255,235,84,236,254,246,79,55,6,42,79,255,236,4,187,4,168,0,23,0,34,130,217,34,20,35,36,131,177,32,20,115,245,9,72,34,12,32,20,
    130,230,39,20,15,62,89,178,6,4,20,99,167,6,32,13,137,217,32,20,65,131,11,131,217,123,22,11,74,154,6,136,208,34,21,20,6,67,186,7,60,50,54,53,52,38,35,34,6,79,125,228,148,225,138,53,
    48,167,88,103,63,2,123,231,149,227,254,236,242,138,106,213,7,8,44,161,253,137,149,19,106,114,134,179,37,125,158,29,160,252,138,1,46,1,1,9,167,189,192,185,167,189,189,0,0,1,0,125,255,
    236,6,61,6,1,0,24,0,84,178,12,99,169,6,36,0,69,88,176,24,130,178,32,24,69,190,7,32,17,130,12,24,77,96,8,95,237,8,34,1,12,24,80,4,6,32,8,137,204,32,12,70,124,14,32,21,134,185,121,71,
    17,38,109,94,181,187,197,254,215,121,76,10,40,220,10,130,161,228,214,9,253,165,121,83,18,45,119,255,236,5,40,4,147,0,25,0,97,178,7,26,24,77,251,9,99,63,12,124,105,8,24,64,203,11,41,
    176,13,16,176,19,208,178,21,19,8,131,173,35,21,47,178,3,136,173,34,178,6,21,132,20,32,8,70,136,11,39,48,49,1,20,6,7,17,35,92,120,5,32,17,104,37,7,130,200,39,55,55,5,40,143,162,229,
    6,104,44,8,42,72,65,5,2,4,147,178,165,11,252,207,104,54,11,49,136,7,66,76,76,0,1,255,181,254,75,1,147,4,58,0,12,0,130,116,68,239,10,108,49,12,32,4,111,143,17,130,129,8,55,17,6,6,35,
    34,39,55,22,51,50,53,17,1,147,1,184,167,70,56,15,39,58,124,4,58,251,133,178,194,17,191,13,192,4,108,0,0,2,0,89,255,236,3,248,4,79,0,22,0,30,0,94,178,8,31,32,131,226,34,8,16,176,68,
    244,7,109,196,12,75,152,8,33,12,0,132,245,33,12,47,115,47,13,33,176,8,71,108,12,35,12,16,178,26,74,41,8,130,146,42,50,0,21,21,20,6,6,39,34,2,53,68,252,5,37,6,7,39,54,54,19,68,244,7,
    8,51,0,228,1,20,123,218,134,213,239,2,170,11,143,119,86,139,78,79,70,210,145,86,120,19,254,75,113,4,79,254,212,246,31,154,251,141,1,1,1,237,136,136,161,39,53,158,62,67,252,96,142,116,
    70,23,6,53,148,4,224,3,67,6,1,0,8,0,69,0,176,4,47,178,15,4,1,93,178,80,131,4,32,112,130,4,47,176,2,208,176,2,47,176,1,208,25,176,1,47,24,176,4,98,40,6,42,180,15,7,31,7,2,93,178,3,7,
    4,131,244,35,1,16,176,5,130,33,34,5,47,24,130,170,61,21,35,39,7,35,53,1,51,3,67,195,150,149,193,1,15,143,4,235,11,156,156,13,1,20,0,0,1,0,114,130,109,32,52,132,109,32,37,136,109,93,
    222,5,36,180,15,1,31,1,130,82,34,0,4,1,131,82,36,8,208,176,8,47,130,77,61,55,51,21,1,35,1,53,51,1,210,146,208,254,233,150,254,235,206,5,102,155,10,254,233,1,24,9,0,255,255,92,143,8,
    35,6,0,112,0,131,95,45,117,4,204,2,251,5,230,0,11,0,47,0,176,3,130,205,50,3,1,93,176,6,208,176,6,47,180,15,6,31,6,2,93,176,3,16,91,74,10,39,176,6,16,176,11,208,176,11,131,105,62,20,
    6,32,38,53,51,20,22,50,54,53,2,251,176,254,218,176,182,75,132,74,5,230,126,156,156,126,66,73,73,66,131,93,46,129,4,223,1,135,5,213,0,9,0,29,178,3,10,11,130,167,32,0,130,165,34,178,
    15,8,97,84,12,89,16,9,37,34,38,129,68,126,68,130,2,51,5,89,53,71,71,53,52,70,70,0,0,2,0,120,4,141,2,51,6,42,130,69,37,20,0,42,0,176,5,130,165,32,5,130,165,39,19,208,176,19,47,178,0,
    10,96,205,8,35,5,16,178,13,136,13,93,8,11,32,7,72,57,7,8,50,34,6,1,86,93,128,125,96,97,125,127,17,66,46,47,65,63,98,63,6,42,123,170,120,120,170,123,208,47,65,64,48,46,67,67,0,1,0,41,
    254,82,1,161,0,60,0,15,0,34,178,15,105,181,10,32,10,74,213,6,33,178,5,93,196,10,40,33,6,6,21,20,51,50,55,23,87,42,6,8,57,1,140,87,74,71,44,46,21,73,92,95,116,244,56,94,49,68,23,142,
    44,110,91,181,108,0,1,0,122,4,219,3,87,5,245,0,21,0,64,0,176,3,47,176,8,208,176,8,47,182,15,8,31,8,47,8,3,93,176,103,213,5,32,11,130,23,34,16,178,15,136,97,130,22,33,178,18,137,13,
    35,15,16,176,21,103,230,13,96,98,5,8,52,51,50,54,53,3,87,127,96,39,57,105,43,26,38,53,149,127,95,57,161,52,38,54,5,233,110,146,17,60,12,57,46,8,110,150,90,57,47,0,0,2,0,73,4,209,3,
    86,5,255,0,3,0,7,131,135,91,217,30,43,176,0,16,176,5,208,176,5,47,176,2,16,66,23,5,130,144,8,35,176,7,208,25,176,7,47,24,48,49,1,51,1,35,3,51,3,35,2,104,238,254,246,197,144,233,222,
    185,5,255,254,210,1,46,254,210,130,105,51,130,254,106,1,236,255,190,0,11,0,23,0,61,0,176,24,47,176,3,208,130,247,58,64,15,0,3,16,3,32,3,48,3,64,3,80,3,96,3,7,93,176,15,208,176,15,47,
    178,9,9,65,202,8,35,3,16,178,21,136,13,34,48,49,23,81,162,10,100,63,8,8,47,35,34,6,130,105,78,73,106,106,73,78,105,101,48,34,33,45,45,33,34,48,238,73,99,97,75,74,94,96,72,33,46,45,
    34,36,48,48,0,0,1,252,142,4,209,254,102,6,0,92,201,5,123,53,7,24,86,36,7,33,1,16,67,178,5,99,88,6,123,52,5,33,254,102,123,52,9,38,1,253,94,4,209,255,54,135,63,65,47,7,36,1,208,176,
    1,47,67,131,6,93,9,16,33,254,33,93,9,8,8,35,255,255,252,115,4,219,255,80,5,245,0,7,0,164,251,249,0,0,0,1,253,62,4,230,254,153,6,127,0,14,0,37,0,176,0,47,65,88,5,33,178,1,126,58,5,32,
    7,69,241,9,33,13,1,77,189,6,8,49,39,54,54,53,52,35,55,50,22,21,20,6,7,21,253,81,7,73,65,150,7,169,171,78,72,4,230,146,5,28,35,72,123,104,88,60,78,10,69,0,0,2,252,12,4,228,255,52,5,
    238,130,237,35,7,0,55,0,130,164,24,87,15,8,130,234,65,202,5,45,6,208,176,6,47,182,15,6,31,6,47,6,3,93,65,149,5,42,176,0,16,176,4,208,25,176,4,47,24,65,3,5,55,1,35,3,51,254,7,208,254,
    213,1,6,2,34,195,245,250,4,228,1,10,254,246,1,10,131,191,49,28,254,148,254,47,255,139,0,8,0,17,0,176,2,47,178,6,5,65,185,7,34,48,49,5,101,11,7,33,253,28,90,17,5,38,241,53,71,71,106,
    70,70,130,55,39,0,198,4,233,1,226,6,65,130,155,32,23,131,55,130,153,131,253,94,71,12,43,51,3,35,1,3,223,140,144,6,65,254,168,130,39,38,103,4,223,3,186,6,175,130,9,40,12,0,21,0,59,0,
    176,20,47,69,98,8,65,128,20,33,176,20,68,161,6,138,148,34,176,15,208,130,211,130,89,136,155,101,176,8,46,1,238,229,130,146,254,168,68,118,67,67,118,68,2,86,101,190,5,36,6,175,254,214,
    47,101,185,12,34,255,255,0,93,213,6,36,2,6,0,120,0,131,203,42,155,0,0,4,55,5,176,0,5,0,43,81,103,10,24,71,146,16,32,4,74,216,11,24,66,4,7,42,4,55,253,96,252,3,156,4,228,251,28,130,
    65,38,2,0,25,0,0,5,160,130,9,36,3,0,6,0,47,24,71,23,23,35,15,62,89,178,106,52,9,34,178,6,2,66,27,6,8,35,51,1,33,37,33,1,2,111,243,2,62,250,121,1,85,2,224,254,152,5,176,250,80,202,3,
    187,0,3,0,91,255,236,5,19,5,196,130,9,38,20,0,34,0,118,178,8,73,251,5,37,8,16,176,1,208,176,130,5,105,213,7,80,108,12,71,123,8,53,2,8,16,17,18,57,124,176,2,47,24,180,96,2,112,2,2,93,
    180,48,2,64,130,6,36,178,0,2,1,113,82,19,10,33,176,16,86,92,12,32,8,125,240,11,85,111,5,32,5,24,70,123,29,8,60,3,163,254,64,1,192,1,112,148,254,237,179,176,254,238,153,3,150,1,20,1,
    100,1,19,150,1,252,183,169,164,185,2,187,166,169,181,2,121,194,137,214,254,189,173,170,1,60,205,93,213,1,68,175,171,254,191,213,5,239,1,5,24,70,131,10,42,1,0,32,0,0,5,18,5,176,0,6,
    24,65,57,7,100,178,12,32,1,120,196,7,24,71,194,8,38,15,62,89,178,0,3,1,24,66,200,8,8,32,51,1,33,2,152,254,151,254,241,1,254,245,1,255,254,240,4,68,251,188,5,176,250,80,0,0,3,0,108,
    0,0,4,46,130,91,38,3,0,7,0,11,0,75,65,169,5,96,46,12,24,65,55,19,34,5,8,2,77,182,5,24,90,55,11,33,8,16,113,229,12,33,55,33,92,137,5,32,3,130,7,58,108,3,194,252,62,100,2,246,253,10,
    87,3,153,252,103,202,202,3,77,198,3,41,204,0,1,0,155,130,219,32,20,130,127,34,7,0,56,133,123,32,6,106,14,6,32,176,97,89,8,79,167,7,127,187,8,33,6,16,93,48,10,38,48,49,33,35,17,33,17,
    130,3,39,5,20,252,253,127,252,4,121,66,131,6,34,1,0,71,130,219,32,77,130,91,34,12,0,60,146,215,32,3,130,104,32,3,130,91,65,243,11,34,5,208,176,140,197,33,176,7,106,255,5,37,21,33,53,
    1,1,53,130,200,8,37,1,3,28,254,117,2,188,251,250,1,201,254,55,3,226,253,107,1,136,2,208,253,250,202,151,2,66,2,63,152,204,253,255,0,0,3,0,74,130,211,32,174,130,119,40,21,0,28,0,35,
    0,108,178,11,81,169,5,37,11,16,176,25,208,176,130,5,24,71,165,7,32,20,130,129,32,20,67,94,7,91,219,8,47,19,20,10,17,18,57,176,19,47,176,0,208,178,9,10,20,131,12,41,9,47,176,12,208,
    176,9,16,178,33,89,231,8,131,75,32,19,76,185,12,32,32,131,171,38,22,4,22,21,20,6,7,110,14,5,8,97,36,38,16,54,36,55,53,51,1,20,22,23,17,6,6,5,52,38,39,17,54,54,3,124,161,1,3,142,136,
    124,133,169,253,162,254,252,143,142,1,3,164,253,253,198,170,147,150,167,3,116,166,148,145,169,4,255,3,143,254,158,154,246,72,77,3,169,169,1,140,250,1,62,255,143,3,177,253,31,160,176,
    2,2,174,4,183,159,160,182,4,253,82,2,179,0,0,1,0,68,130,233,32,92,130,233,36,23,0,92,178,0,100,219,10,76,176,12,108,171,12,24,71,217,12,122,106,8,34,21,11,22,76,28,5,36,176,0,208,176,
    21,24,79,66,12,32,9,131,213,46,54,54,53,17,51,17,6,0,7,17,35,17,38,0,39,130,11,32,22,130,209,8,49,51,3,76,131,144,253,3,254,233,246,252,240,254,232,4,252,1,143,128,252,2,67,23,190,
    167,1,241,254,6,246,254,207,25,254,138,1,117,23,1,48,245,1,255,254,11,157,194,24,3,108,131,179,40,107,0,0,4,221,5,195,0,37,130,179,34,7,38,39,130,119,66,28,5,36,26,47,27,177,26,65,
    141,7,32,15,130,12,32,15,66,120,7,32,36,130,12,32,36,131,12,33,15,16,24,91,206,11,33,14,208,131,173,32,26,87,55,12,38,17,16,176,34,208,176,35,130,179,34,37,54,18,97,207,7,42,21,20,
    18,23,21,33,53,51,38,2,53,94,99,5,8,95,4,18,21,21,20,2,7,51,21,33,2,223,116,123,1,157,144,142,155,127,119,254,7,216,107,120,142,1,5,164,165,1,6,144,119,107,212,254,16,207,32,1,16,231,
    109,202,218,217,205,100,235,254,235,30,207,203,103,1,31,158,98,182,1,29,159,158,254,226,181,101,151,254,220,103,203,0,0,2,0,86,255,235,4,121,4,78,0,22,0,33,0,121,178,31,34,35,130,211,
    36,176,31,16,176,19,81,122,6,118,75,12,76,146,12,24,64,165,21,32,3,66,100,8,35,178,10,19,12,130,76,33,178,21,132,6,33,176,12,66,111,12,32,19,69,18,14,33,17,22,74,52,6,35,39,6,35,34,
    130,234,39,16,18,51,50,23,55,1,20,24,64,184,8,8,58,3,253,3,70,17,10,24,51,76,162,53,102,193,195,227,228,196,181,103,19,254,28,122,118,140,70,70,138,115,127,4,58,252,250,123,4,180,30,
    163,162,1,29,248,13,1,10,1,54,151,131,253,191,158,173,136,1,199,142,197,130,229,46,150,254,119,4,106,5,196,0,20,0,40,0,101,178,39,111,197,5,35,39,16,176,0,130,229,32,15,24,69,52,13,
    32,176,122,23,8,37,15,62,89,178,39,0,131,195,36,176,39,47,178,36,137,216,34,6,36,39,130,216,33,176,0,69,228,12,130,223,32,30,136,34,34,48,49,1,72,156,5,35,22,22,21,20,130,209,45,39,
    17,35,17,52,54,54,1,52,38,35,34,6,21,131,236,8,92,54,53,52,38,39,35,53,51,50,2,105,207,242,99,88,121,130,242,209,165,122,242,124,217,1,76,113,93,96,129,88,157,113,137,122,103,123,72,
    212,5,196,216,178,95,155,48,44,189,130,205,236,83,254,56,5,169,115,193,112,254,109,90,118,126,104,252,229,82,137,110,109,145,1,185,0,0,1,0,32,254,95,3,245,4,58,0,8,0,56,178,0,9,10,
    24,72,83,13,82,93,7,102,93,12,78,186,8,34,0,7,4,69,236,5,34,19,51,1,130,165,8,42,1,51,2,14,236,251,254,143,243,254,143,251,1,59,2,255,251,240,254,53,1,208,4,11,0,0,2,0,84,255,236,4,
    56,6,32,0,31,0,43,0,98,178,22,93,121,5,32,22,82,17,9,36,3,47,27,177,3,82,229,7,32,22,130,12,32,22,107,13,6,32,9,65,24,8,35,178,14,22,3,130,110,36,176,14,47,178,41,137,20,34,29,41,14,
    112,123,5,82,24,12,49,19,52,54,51,50,22,23,21,38,35,34,6,21,20,23,22,18,23,81,64,7,38,52,54,55,39,38,38,19,111,58,8,8,79,34,6,208,212,183,73,113,79,151,105,78,90,188,224,222,2,122,
    225,149,226,254,238,184,137,2,91,104,118,137,121,119,135,145,109,121,137,4,234,145,165,22,27,195,53,61,52,93,66,79,254,234,204,28,155,246,135,1,35,1,3,165,255,34,5,40,137,253,125,162,
    188,188,182,120,203,23,190,0,1,0,96,130,231,40,12,4,77,0,39,0,139,178,22,24,75,175,15,65,77,7,32,37,130,223,32,37,130,223,35,178,23,9,37,130,188,47,124,176,23,47,24,180,64,23,80,23,
    2,93,180,208,23,224,130,6,33,178,24,79,130,8,35,178,3,24,23,130,36,122,82,15,33,23,16,130,20,38,178,28,13,1,93,178,11,130,4,35,176,37,16,178,66,67,9,35,178,33,30,24,130,30,38,180,4,
    33,20,33,2,93,24,99,92,14,32,35,66,73,5,37,20,22,51,51,21,35,100,160,8,8,63,20,4,35,34,36,96,105,98,87,97,248,210,191,255,242,122,89,94,114,96,105,199,209,210,125,102,98,130,242,254,
    252,203,213,254,248,1,50,92,127,32,36,121,72,150,165,181,145,60,79,77,63,60,75,173,3,147,63,87,89,66,155,186,178,0,130,251,42,97,254,126,3,202,5,176,0,30,0,74,80,147,6,33,0,176,67,
    33,19,97,180,9,83,219,11,35,178,1,28,0,130,179,33,176,21,24,84,35,14,34,21,1,6,131,164,8,82,23,23,22,22,21,20,6,7,39,54,53,54,39,39,38,39,38,53,16,1,55,33,53,3,202,254,96,86,70,61,
    75,221,97,79,122,82,125,93,2,110,104,196,74,57,1,37,220,253,196,5,176,145,254,10,109,186,107,84,90,24,66,31,98,81,71,186,62,101,103,70,61,33,27,50,105,80,139,1,32,1,81,253,195,131,
    179,46,126,254,97,4,6,4,78,0,17,0,83,178,12,18,19,89,145,34,36,7,47,27,177,7,89,145,7,94,129,8,34,1,3,15,130,59,33,176,3,70,67,11,34,48,49,1,127,193,12,8,54,7,17,35,17,1,92,12,119,
    193,182,173,3,243,94,104,150,70,243,4,58,131,151,196,197,251,156,4,83,110,105,122,252,239,4,58,0,3,0,115,255,236,4,44,5,196,0,13,0,22,0,30,0,121,178,3,81,221,5,36,3,16,176,19,208,130,
    95,33,176,27,24,97,249,27,35,178,14,3,10,130,135,47,124,176,14,47,24,180,96,14,112,14,2,93,180,48,14,64,130,6,39,178,0,14,1,113,176,10,16,24,68,110,11,32,14,68,133,12,130,187,32,27,
    67,87,8,24,98,38,15,33,5,33,84,116,5,8,51,21,5,33,21,20,22,50,54,55,4,44,248,227,223,250,5,246,230,226,246,5,253,58,1,212,122,113,111,122,1,212,254,44,123,224,119,2,2,114,254,196,254,
    182,1,65,1,45,233,1,53,1,76,130,12,53,211,35,48,206,203,203,206,239,42,208,209,202,202,0,0,1,0,169,255,244,2,97,130,239,34,12,0,40,75,165,10,67,32,7,36,9,47,27,177,9,75,165,13,130,
    144,124,81,11,54,1,156,50,62,42,43,74,86,254,232,4,58,252,246,61,54,10,188,23,1,53,3,17,130,89,44,22,255,238,4,74,5,251,0,25,0,80,178,3,84,49,7,69,167,5,71,247,7,37,176,0,69,88,176,
    16,130,99,32,16,130,99,33,176,11,71,73,11,38,178,15,0,11,17,18,57,94,124,5,33,176,0,84,213,14,8,80,50,22,23,1,22,23,23,55,23,6,35,34,38,39,3,3,33,1,39,38,39,35,7,39,54,1,18,108,120,
    31,1,171,36,49,32,17,4,42,52,109,117,43,202,246,254,247,1,129,91,34,73,34,27,3,59,5,251,85,80,251,191,86,7,1,1,192,10,88,111,2,20,253,55,4,15,218,75,3,2,182,16,130,175,44,100,254,118,
    3,212,5,196,0,44,0,86,178,3,100,231,6,32,22,133,175,32,42,130,162,35,42,31,62,89,74,118,10,35,178,8,45,42,131,159,33,8,47,101,120,10,33,178,29,133,20,32,29,103,160,12,34,36,9,8,126,
    239,8,49,6,21,20,33,51,21,35,32,17,20,22,4,22,23,22,21,6,6,130,180,42,54,53,52,38,36,39,38,38,53,52,54,116,60,7,8,79,23,3,131,138,87,122,136,1,28,137,140,254,158,129,1,25,111,35,81,
    2,123,80,131,53,46,63,254,253,76,127,118,163,144,110,124,1,2,227,153,125,4,218,36,86,75,184,198,254,227,98,136,66,37,24,56,109,72,187,59,100,57,80,41,35,45,68,32,53,183,148,145,196,
    45,40,142,97,166,197,44,0,130,223,46,45,255,244,4,207,4,58,0,20,0,92,178,11,21,22,67,97,8,72,29,12,74,133,7,32,176,110,161,8,39,15,62,89,176,19,16,178,0,69,7,8,33,176,10,24,76,109,
    12,42,0,16,176,13,208,176,14,208,176,17,208,96,38,5,126,112,12,8,61,33,17,35,17,35,53,33,4,169,159,49,63,38,47,74,86,254,232,254,180,243,171,4,124,3,124,253,182,62,55,10,188,23,1,53,
    2,83,252,132,3,124,190,0,2,0,128,254,96,4,49,4,78,0,14,0,26,0,87,178,17,27,28,130,163,33,176,17,71,225,5,76,13,8,121,57,12,67,254,7,100,34,8,34,9,0,7,130,52,73,165,11,90,73,12,43,48,
    49,1,50,18,17,21,20,2,35,34,39,130,153,34,52,0,3,91,128,9,8,56,21,2,86,224,251,224,193,179,106,243,1,3,16,67,149,118,125,124,114,102,119,4,78,254,203,254,239,15,242,254,229,119,253,
    253,3,219,242,1,33,252,213,117,173,179,184,197,193,160,0,0,1,0,82,254,138,3,233,130,177,38,34,0,77,178,27,35,36,94,245,13,135,169,39,20,47,27,177,20,23,62,89,80,143,5,130,5,97,104,
    10,35,178,28,35,0,130,53,33,176,28,103,177,14,33,50,22,116,173,10,36,4,22,22,23,20,66,40,7,8,87,39,38,38,39,53,52,54,54,2,56,196,237,228,109,96,113,131,148,1,46,96,49,1,127,76,127,
    51,42,60,65,238,237,1,120,220,4,78,221,187,97,116,188,170,26,131,155,86,57,83,66,72,191,56,101,55,78,44,40,42,15,55,254,209,39,157,250,137,0,0,2,0,82,255,236,4,126,4,58,0,15,0,27,0,
    76,178,7,28,29,119,26,5,74,51,8,32,14,130,180,32,14,72,117,12,32,15,130,193,32,14,79,250,11,33,176,7,68,196,12,41,0,16,176,25,208,48,49,1,33,22,71,226,7,37,53,52,0,55,33,1,92,227,10,
    8,59,4,126,254,245,186,122,222,145,226,254,240,1,12,223,2,65,252,199,133,122,117,129,131,117,118,135,3,118,146,251,142,236,131,1,44,1,3,12,238,1,35,2,253,216,169,187,188,189,156,179,
    176,0,0,1,0,63,255,236,3,236,130,171,36,16,0,73,178,1,24,69,47,10,126,115,12,66,185,8,32,15,140,163,66,172,21,131,166,68,197,5,8,40,23,6,35,32,3,17,33,53,33,3,236,254,152,43,51,39,
    55,38,80,108,254,236,5,254,174,3,173,3,121,253,176,59,59,22,177,44,1,57,2,84,193,130,135,36,128,255,235,4,8,130,135,36,18,0,56,178,14,111,255,23,40,14,47,27,177,14,15,62,89,178,75,
    69,9,36,176,0,16,176,8,86,204,6,8,70,17,16,51,50,54,53,38,3,51,22,17,16,0,35,34,38,39,17,1,114,161,113,145,3,110,241,115,254,252,231,203,209,1,4,58,253,118,254,253,233,160,231,1,29,
    230,254,226,254,244,254,193,226,216,2,149,0,2,0,68,254,34,5,133,4,65,0,26,0,35,0,95,109,53,10,36,27,208,0,176,25,68,167,5,32,17,130,123,32,17,65,180,7,32,6,24,74,80,11,93,112,8,24,
    74,211,9,131,149,41,24,208,176,13,16,176,27,208,176,17,78,90,11,62,48,49,5,36,0,53,52,18,55,23,6,6,7,20,22,23,17,52,54,51,50,22,22,21,20,0,5,17,35,19,54,130,190,8,76,38,35,34,21,2,
    101,254,252,254,227,126,115,152,72,76,2,154,148,158,124,147,236,135,254,222,254,245,243,243,149,165,2,141,116,55,14,28,1,55,255,164,1,5,83,146,70,188,104,161,205,30,2,128,119,146,141,
    251,146,243,254,215,26,254,49,2,148,25,193,151,151,191,62,0,0,1,0,79,130,215,40,126,4,58,0,24,0,68,178,0,91,1,6,32,13,133,207,32,20,130,194,32,20,135,207,71,226,8,32,23,71,28,8,38,
    176,1,208,176,20,16,176,130,197,43,6,208,176,15,16,176,12,208,48,49,1,17,131,160,65,95,5,130,174,54,17,36,0,3,17,51,17,16,5,17,3,82,147,167,5,112,238,121,254,225,254,243,243,130,186,
    49,245,1,243,1,29,4,58,252,125,27,206,164,226,1,20,227,254,237,130,20,51,202,26,254,50,1,208,30,1,51,1,10,1,237,254,24,254,162,60,3,130,130,163,36,102,255,236,6,45,130,163,36,32,0,
    86,178,26,125,203,10,77,76,12,32,24,130,173,32,24,78,51,7,24,68,142,8,24,82,88,11,52,0,28,17,18,57,176,14,208,176,0,16,176,19,208,176,19,47,178,26,5,24,70,27,5,32,2,87,209,6,130,171,
    33,22,22,66,34,7,8,87,16,2,35,34,39,6,35,34,2,16,55,1,229,134,7,97,88,91,96,251,2,95,90,88,97,7,133,241,141,213,203,232,92,92,230,203,214,141,4,58,254,233,237,189,203,157,148,1,70,
    254,175,142,152,203,189,239,1,21,232,253,200,254,210,222,222,1,46,2,56,232,0,2,0,118,255,236,4,152,5,196,0,32,0,41,0,107,178,15,109,89,5,35,15,16,176,33,72,195,6,78,251,12,32,6,24,
    84,144,7,34,36,26,6,130,156,34,176,36,47,72,171,11,35,2,208,178,11,133,23,32,6,98,245,12,41,36,16,176,30,208,176,26,16,178,39,65,158,8,38,48,49,1,6,7,21,20,130,192,36,0,53,17,55,17,
    68,8,5,8,113,53,38,0,39,53,52,54,51,50,22,21,17,54,55,1,20,22,23,17,38,35,34,6,4,152,58,68,250,213,211,254,254,236,130,110,98,109,209,255,0,3,197,165,167,188,75,42,253,170,125,107,
    4,109,52,67,2,87,20,11,117,218,253,1,5,212,1,29,2,254,222,125,143,134,131,124,38,1,19,192,27,169,204,208,187,254,206,12,11,1,35,108,162,32,1,69,154,73,0,0,1,255,225,0,0,4,158,5,195,
    0,26,0,66,178,0,99,223,10,80,125,12,32,13,130,231,39,13,15,62,89,178,0,4,13,95,23,5,71,202,10,40,176,18,208,176,4,16,176,23,208,130,196,33,19,54,96,249,7,37,7,1,17,35,17,1,130,175,
    33,7,39,130,18,8,74,22,23,2,63,210,43,122,96,70,66,38,13,40,65,31,254,217,252,254,219,33,64,43,10,36,60,74,103,125,44,3,7,1,248,100,96,26,194,5,69,253,107,253,238,2,16,2,151,69,5,193,
    27,100,108,0,0,2,0,51,255,236,6,84,4,58,0,18,0,38,0,112,178,8,39,40,131,127,35,8,16,176,30,65,143,6,67,200,17,66,87,7,68,230,8,32,17,68,230,11,35,178,8,17,6,131,66,45,15,208,176,16,
    208,176,21,208,176,22,208,176,10,16,105,119,10,35,178,31,16,10,131,32,32,36,131,207,34,35,22,21,66,89,8,42,17,52,55,35,53,33,1,38,39,33,6,66,129,5,8,91,55,53,51,21,22,22,51,50,54,6,
    84,128,55,202,188,238,92,92,238,189,200,54,111,6,33,254,197,4,61,252,198,60,4,83,75,92,102,1,250,2,99,93,75,83,3,131,158,175,254,226,254,212,226,226,1,46,1,28,177,156,183,253,252,160,
    173,177,156,190,202,151,149,232,238,143,151,202,0,1,0,34,255,242,5,188,5,176,0,24,0,110,178,17,94,223,10,36,23,47,27,177,23,24,104,29,15,37,176,0,69,88,176,19,130,25,32,19,131,238,
    32,23,140,225,34,4,23,9,131,192,35,4,47,176,9,84,103,11,33,176,4,109,115,12,33,0,16,133,253,45,48,49,1,33,17,54,51,50,4,16,4,35,39,50,68,177,5,8,51,7,17,35,17,33,53,33,4,144,254,19,
    148,114,251,1,24,254,238,254,1,137,140,1,143,143,134,120,253,254,124,4,110,4,228,254,116,38,240,254,80,236,191,121,132,119,135,32,253,116,4,228,204,130,193,44,104,255,236,4,239,5,196,
    0,31,0,113,178,3,24,65,3,10,24,92,173,12,101,96,8,32,12,126,114,12,34,23,12,3,78,87,8,41,48,23,64,23,2,93,180,96,23,112,78,94,10,35,0,23,1,113,24,109,10,10,33,176,3,77,137,11,24,98,
    222,24,32,33,109,195,5,8,84,54,55,4,238,22,254,212,248,175,254,245,145,1,146,1,17,180,243,1,37,24,252,18,148,142,161,176,8,1,251,254,4,7,171,157,147,150,20,1,217,232,254,251,165,1,
    54,207,123,207,1,58,170,254,246,236,156,142,229,210,202,221,229,135,157,0,0,2,0,45,0,0,8,65,5,176,0,25,0,34,0,116,178,9,35,36,78,226,5,102,83,8,96,135,12,95,223,12,75,183,7,35,178,
    0,24,8,131,52,35,0,47,176,24,65,153,12,32,16,24,82,222,12,32,0,82,59,12,39,18,16,176,27,208,176,28,208,130,226,8,100,33,30,2,21,20,4,7,33,17,33,3,2,2,6,35,35,53,55,62,2,55,19,33,17,
    17,33,50,54,53,52,38,39,5,13,1,49,153,235,127,254,235,229,253,202,254,66,26,15,99,188,158,64,40,87,95,49,10,28,3,171,1,41,126,145,143,122,3,161,1,117,212,135,206,253,5,4,228,253,205,
    254,248,254,221,134,202,3,8,106,215,209,2,201,253,38,253,244,147,117,115,143,2,130,233,32,155,130,233,32,71,130,233,40,19,0,28,0,135,178,1,29,30,94,182,6,32,20,24,66,5,11,66,145,7,
    36,19,47,27,177,19,135,12,76,161,8,24,80,184,11,35,178,0,16,19,133,246,40,178,159,0,1,93,178,4,13,2,66,159,6,130,233,24,101,104,9,33,176,4,24,68,211,12,32,13,76,200,14,34,33,17,51,
    130,231,45,22,22,21,20,4,35,33,17,33,17,35,17,51,1,134,248,53,35,1,151,2,128,252,1,43,156,238,127,254,227,243,253,224,253,128,252,252,3,124,130,241,8,35,146,148,124,3,69,2,107,253,
    210,110,203,133,205,247,2,122,253,134,5,176,253,8,254,24,134,112,111,131,0,1,0,49,0,0,5,200,130,231,34,21,0,86,77,209,5,86,102,12,65,194,20,33,176,20,67,106,13,33,16,20,133,202,24,
    100,188,10,40,176,0,16,176,18,208,176,19,208,67,89,6,34,32,4,21,130,172,24,81,152,7,8,42,33,53,33,4,146,254,17,131,143,1,12,1,7,252,125,154,140,134,252,254,138,4,97,4,228,254,155,27,
    236,229,254,55,1,202,139,122,28,253,77,4,228,204,0,130,161,36,146,254,152,5,13,130,161,34,11,0,72,123,149,8,124,12,8,98,58,8,65,125,7,70,142,7,32,176,131,25,87,37,8,117,49,10,32,3,
    130,147,36,19,51,17,33,17,131,3,8,32,35,17,33,146,253,2,129,253,254,75,253,254,55,5,176,251,26,4,230,250,80,254,152,1,104,0,2,0,144,0,0,4,193,130,119,40,13,0,22,0,91,178,16,23,24,118,
    59,5,33,176,3,105,63,19,69,104,8,32,12,65,27,12,34,2,12,10,131,53,34,2,47,178,124,36,9,105,255,13,35,48,49,1,33,65,211,7,32,7,130,9,65,207,7,48,39,4,44,253,97,1,42,160,238,124,254,
    235,239,253,211,3,156,130,13,58,41,128,143,140,124,4,228,254,159,110,202,133,204,248,2,5,176,253,8,254,18,139,115,110,128,2,0,130,175,36,36,254,154,5,220,130,175,40,14,0,20,0,101,178,
    18,21,22,131,121,35,18,16,176,11,134,175,35,11,47,27,177,24,101,11,8,32,4,130,12,33,4,23,122,106,6,89,179,7,40,176,4,16,176,1,208,176,2,16,89,167,11,77,174,5,70,42,5,33,11,16,77,43,
    10,130,185,8,73,35,17,33,17,35,3,51,54,18,55,19,33,17,51,33,33,17,33,3,2,5,207,240,252,65,244,8,117,87,104,15,38,3,150,185,251,219,2,112,254,87,24,27,254,154,1,102,254,154,2,48,84,
    1,65,203,2,134,251,26,4,26,254,102,254,101,0,0,1,0,22,0,0,7,155,130,183,34,21,0,125,66,129,5,24,93,244,12,102,115,12,88,40,12,136,181,24,84,19,12,131,12,101,192,8,40,16,9,2,17,18,57,
    176,16,47,24,73,223,10,38,176,4,208,178,8,16,0,132,23,39,16,176,11,208,178,19,0,16,130,12,132,205,37,35,17,35,1,33,1,130,2,32,51,109,214,8,8,33,4,255,163,252,170,254,155,254,197,1,
    213,254,74,1,50,1,92,157,252,150,1,89,1,49,254,78,1,209,254,198,2,116,253,140,131,3,39,3,7,2,169,253,160,2,96,132,3,34,89,252,247,131,215,46,73,255,237,4,127,5,195,0,41,0,134,178,37,
    42,43,77,237,8,65,135,12,32,23,106,90,7,32,11,24,111,90,12,34,40,11,23,130,47,52,124,176,40,47,24,178,16,40,1,93,180,48,40,64,40,2,93,180,96,40,112,131,6,34,160,40,176,130,6,35,178,
    6,40,3,130,37,33,178,37,73,66,8,35,178,17,37,40,130,17,33,176,23,87,107,11,35,178,28,37,31,133,224,125,241,8,34,54,51,50,24,105,169,10,34,34,38,38,125,45,8,8,68,38,35,35,53,51,32,3,
    108,148,127,109,146,252,132,234,141,250,1,21,120,108,122,129,254,212,250,154,249,125,252,156,120,134,163,143,138,171,162,1,12,4,35,98,116,115,91,119,186,103,218,196,99,166,48,42,171,
    127,196,231,110,190,123,94,129,126,101,123,111,200,131,253,42,148,0,0,5,13,5,176,0,9,0,69,93,207,18,32,7,130,246,32,7,94,42,16,92,146,12,34,4,0,2,130,181,33,178,9,132,6,63,48,49,1,
    51,17,35,17,1,35,17,51,17,4,16,253,253,253,129,253,253,5,176,250,80,4,13,251,243,5,176,251,242,131,113,32,45,134,113,36,17,0,77,178,4,84,19,10,68,43,12,92,254,12,32,9,82,175,6,33,176,
    0,65,124,11,34,176,9,16,24,83,33,13,130,120,70,108,12,34,5,13,252,70,89,13,132,137,70,76,15,46,0,1,0,57,255,235,4,221,5,176,0,15,0,73,178,24,76,253,16,68,176,15,34,178,0,15,73,148,
    5,32,16,100,222,5,33,176,6,71,30,11,35,178,13,6,15,93,161,8,8,68,7,6,35,39,55,22,51,50,55,55,1,33,2,160,1,36,1,25,254,5,46,100,224,104,2,24,61,108,44,52,254,14,1,20,2,183,2,249,251,
    72,91,178,6,200,4,92,123,4,36,0,3,0,79,255,196,6,24,5,236,0,22,0,31,0,40,0,85,178,10,89,17,5,37,10,16,176,30,208,176,130,5,44,32,208,0,176,10,47,176,21,47,178,20,21,10,130,105,41,176,
    20,47,176,0,208,178,11,10,21,131,12,36,11,47,176,8,208,92,78,11,130,52,32,20,66,100,11,8,36,176,32,208,48,49,1,50,4,18,21,20,2,4,35,21,35,53,35,38,36,2,53,52,18,36,51,53,51,1,34,6,
    21,20,22,23,51,17,130,1,8,76,50,54,53,52,38,35,3,174,187,1,22,153,153,254,235,188,243,23,169,254,236,152,154,1,20,190,243,254,251,170,193,187,171,23,243,17,171,191,191,173,5,38,152,
    254,240,172,170,254,241,151,190,190,1,150,1,13,170,173,1,18,151,198,254,111,207,188,180,205,2,3,14,252,242,207,182,185,208,70,29,5,40,161,5,189,5,176,0,11,0,59,70,29,34,70,16,19,32,
    6,70,16,9,33,51,3,70,16,7,36,176,20,232,251,209,70,16,5,37,251,28,253,213,1,95,131,107,36,142,0,0,4,238,130,107,34,17,0,63,66,211,18,68,182,12,66,103,7,42,178,14,1,9,17,18,57,176,14,
    47,178,77,218,9,41,48,49,1,17,35,17,6,35,32,36,92,155,5,8,37,51,50,55,17,4,238,252,162,176,254,251,254,244,1,252,1,126,151,174,164,5,176,250,80,2,61,41,230,232,1,206,254,48,139,118,
    42,2,167,131,129,36,152,0,0,7,3,132,237,32,72,146,129,95,232,12,67,98,12,85,153,8,32,1,68,43,8,35,176,5,208,176,131,250,32,1,71,10,6,32,51,130,7,41,1,150,1,188,252,1,185,252,249,149,
    134,250,36,26,4,230,250,80,130,111,38,1,0,152,254,162,7,173,130,9,37,15,0,84,0,176,11,91,64,18,153,124,32,13,77,57,7,143,124,42,176,9,208,176,10,208,176,2,208,48,49,137,133,34,51,3,
    35,138,137,36,170,20,222,249,221,137,140,37,251,18,253,224,1,94,130,134,39,0,2,0,24,0,0,5,212,71,159,6,34,94,178,1,71,159,5,35,1,16,176,14,123,11,28,34,2,0,10,71,145,5,36,176,0,16,
    178,12,65,13,9,32,2,85,132,11,71,162,15,32,19,71,162,21,46,24,2,135,1,42,160,238,125,254,233,238,253,212,254,117,130,13,50,41,128,143,140,124,5,176,253,211,110,201,134,205,247,2,4,
    237,253,203,71,161,8,38,3,0,155,0,0,6,88,130,177,34,11,0,15,130,189,36,109,178,2,25,26,132,139,37,16,176,13,208,176,2,106,7,9,70,33,12,113,153,12,73,143,12,107,218,8,34,0,8,11,74,91,
    6,32,16,137,208,32,8,71,171,14,35,33,50,22,22,75,57,5,33,51,1,74,55,10,34,39,1,152,72,101,9,39,253,4,192,252,252,251,64,1,132,198,33,3,131,72,102,7,33,250,80,72,106,11,73,25,9,38,11,
    0,20,0,77,178,14,72,105,5,35,14,16,176,1,72,105,19,66,122,8,33,0,9,135,165,65,118,10,32,9,84,201,14,138,165,75,210,8,33,1,141,138,161,143,155,73,2,9,44,1,0,107,255,236,4,241,5,196,
    0,31,0,127,77,123,12,75,158,12,81,106,8,53,9,19,28,17,18,57,124,176,9,47,24,180,96,9,112,9,2,93,180,208,9,224,131,6,34,48,9,64,130,6,36,178,0,9,1,113,73,9,11,32,28,71,162,12,34,0,6,
    3,130,62,33,176,19,66,69,11,35,178,15,9,12,69,165,5,8,101,22,22,51,50,54,55,33,53,33,38,38,35,34,6,7,35,54,0,51,50,4,18,23,21,20,2,4,35,34,0,39,1,104,20,151,147,156,171,6,253,254,2,
    2,8,177,160,140,149,18,252,24,1,37,242,179,1,16,147,1,143,254,244,176,248,254,212,22,1,217,158,134,228,215,204,216,228,140,158,238,1,8,168,254,200,205,123,207,254,199,168,1,5,232,0,
    0,2,0,160,255,236,7,7,130,235,40,23,0,37,0,126,178,18,38,39,73,237,6,32,29,95,197,11,70,99,7,73,68,12,99,24,8,75,33,12,34,14,10,13,130,65,90,129,11,24,111,243,10,131,229,90,100,9,34,
    176,4,16,24,94,40,12,111,206,8,41,35,17,35,17,51,17,51,54,18,36,132,236,24,107,92,13,52,7,7,148,254,237,179,167,254,248,158,14,182,252,252,179,6,154,1,15,173,178,24,107,97,19,45,152,
    1,28,189,253,163,5,176,253,113,201,1,53,165,24,107,102,18,130,253,48,32,0,0,4,95,5,176,0,12,0,21,0,97,178,16,22,23,73,206,6,32,10,91,101,19,24,65,78,12,99,199,8,34,17,10,0,131,52,32,
    17,24,106,30,12,34,5,1,17,131,20,32,10,78,139,11,8,68,48,49,33,17,33,1,33,1,38,17,52,36,55,33,17,1,20,22,51,51,17,35,34,6,3,98,254,230,254,231,254,241,1,69,254,1,19,246,1,239,253,4,
    138,138,235,235,140,136,2,32,253,224,2,107,120,1,17,209,233,2,250,80,3,233,123,138,2,0,134,130,179,46,91,255,235,4,60,6,19,0,26,0,38,0,84,178,14,81,197,5,33,14,16,92,25,8,36,17,47,
    27,177,17,124,197,16,34,0,17,7,67,219,6,32,25,88,172,5,65,135,10,32,7,85,134,13,54,1,50,18,21,21,20,0,35,34,0,17,53,16,18,55,54,54,53,51,20,6,6,7,130,2,33,54,23,124,173,8,8,90,52,38,
    2,122,204,246,254,245,229,223,254,238,248,246,138,81,196,66,136,166,152,159,27,145,147,118,134,132,122,121,133,133,3,254,254,239,234,12,234,254,222,1,40,1,0,70,1,94,1,152,51,28,63,
    54,101,126,79,35,32,164,145,149,195,159,165,156,174,175,176,140,163,0,3,0,143,0,0,4,58,4,58,0,14,0,21,0,28,0,120,178,2,79,35,5,35,2,16,176,21,68,205,12,32,1,130,215,32,1,121,180,15,
    35,178,22,1,0,130,215,47,124,176,22,47,24,180,64,22,80,22,2,93,180,208,22,224,130,6,33,178,15,90,59,8,35,178,8,15,22,24,116,118,20,65,3,9,36,48,49,51,17,33,97,120,11,32,1,130,14,8,
    62,53,52,35,37,51,50,53,52,39,35,143,1,183,222,232,93,91,106,124,223,209,254,248,1,10,187,190,254,249,200,207,196,211,4,58,155,145,75,119,32,22,134,91,151,158,1,205,254,243,134,135,
    174,122,128,4,0,1,0,133,0,0,3,77,130,211,104,211,13,135,188,76,113,8,104,211,20,42,3,77,254,42,242,2,200,3,118,252,138,130,65,38,2,0,39,254,190,4,197,130,9,36,14,0,20,0,91,77,143,10,
    33,4,208,24,86,17,8,33,4,47,122,153,10,72,155,8,32,0,69,196,9,41,6,208,176,7,208,176,12,16,176,9,130,8,33,16,176,77,133,5,32,4,69,217,13,8,62,55,54,54,55,19,33,17,51,17,35,17,33,17,
    35,19,33,33,17,33,7,2,129,101,69,7,14,2,239,150,242,253,74,246,1,1,118,1,159,254,239,7,14,194,113,203,158,1,158,252,136,253,252,1,66,254,190,2,4,2,167,207,254,214,130,245,36,30,0,0,
    6,92,130,169,34,21,0,130,77,129,10,24,90,78,12,135,12,84,106,12,77,129,35,34,17,2,17,77,129,5,35,143,16,1,93,77,134,41,39,3,33,1,1,33,19,51,17,130,1,32,19,131,10,62,4,53,129,243,128,
    249,254,214,1,103,254,172,1,41,245,114,243,115,246,1,41,254,173,1,105,254,210,1,179,254,77,131,3,39,2,51,2,7,254,87,1,169,131,3,36,253,252,253,202,0,130,217,44,77,255,236,3,196,4,77,
    0,39,0,141,178,30,97,215,10,36,37,47,27,177,37,124,18,12,38,15,62,89,178,25,37,8,130,172,47,124,176,25,47,24,180,64,25,80,25,2,93,180,208,25,224,130,6,33,178,22,66,147,9,34,3,22,25,
    85,126,5,33,178,16,137,20,34,13,22,16,130,20,43,180,3,13,19,13,2,93,176,37,16,178,30,137,27,34,33,25,30,130,27,43,64,9,11,33,27,33,43,33,59,33,4,93,114,179,5,100,39,5,77,123,15,32,
    54,24,74,207,12,8,56,3,176,87,79,186,242,203,124,204,114,242,118,90,89,105,92,96,174,180,163,94,82,80,110,242,240,185,201,224,3,18,72,121,36,65,186,149,177,83,153,105,66,89,83,67,79,
    70,175,2,132,66,74,79,60,143,183,164,130,251,38,134,0,0,4,18,4,58,77,129,14,91,164,12,66,216,16,77,129,13,32,7,77,129,5,132,6,77,129,11,52,3,32,242,242,254,88,242,242,4,58,251,198,
    2,210,253,46,4,58,253,46,0,130,113,32,143,130,113,32,101,130,113,34,12,0,104,66,71,5,66,223,12,121,82,12,66,58,12,62,11,47,27,177,11,15,62,89,178,6,2,4,17,18,57,124,176,6,47,24,180,
    211,6,227,6,2,93,180,67,6,83,130,6,34,178,19,6,107,110,12,33,178,10,125,130,20,47,253,123,243,243,107,1,43,1,44,254,121,1,168,254,196,1,125,130,9,34,250,253,204,131,161,32,33,130,161,
    32,20,130,161,36,15,0,77,178,4,112,217,10,89,153,12,78,35,12,73,113,9,78,35,12,105,247,12,75,207,5,58,33,3,2,6,35,35,39,55,54,54,55,19,4,20,243,254,206,20,19,171,176,75,1,50,80,73,
    10,130,114,47,251,198,3,118,254,135,254,240,237,202,5,11,173,229,1,206,65,47,6,33,5,111,130,141,34,12,0,89,65,47,5,69,42,12,36,11,47,27,177,11,65,161,7,86,49,8,80,236,16,73,95,9,24,
    92,68,12,32,8,132,13,130,153,39,1,33,17,35,17,1,35,1,130,5,61,33,2,255,1,64,1,48,243,254,214,165,254,213,243,1,50,1,43,3,15,251,198,2,204,253,52,2,208,253,48,127,61,5,36,134,0,0,4,
    17,130,10,34,11,0,126,133,147,92,24,12,24,92,67,12,71,87,12,120,14,8,32,9,71,87,5,8,36,9,47,180,191,9,207,9,2,93,178,191,9,1,113,180,47,9,63,9,2,114,178,95,9,1,114,180,239,9,255,9,
    2,113,180,31,9,47,130,6,40,178,143,9,1,93,180,143,9,159,131,30,83,166,9,107,167,8,56,51,17,33,17,51,4,17,243,254,91,243,243,1,165,243,1,181,254,75,4,58,254,61,1,195,66,225,10,108,13,
    13,70,106,15,24,119,42,12,32,176,108,13,22,39,4,18,243,254,90,243,3,140,69,189,6,42,1,0,35,0,0,3,208,4,58,0,7,109,69,7,65,7,12,24,112,229,24,33,176,5,84,242,5,52,35,17,33,53,33,3,208,
    254,161,243,254,165,3,173,3,121,252,135,3,121,193,130,82,51,0,84,254,96,5,127,6,0,0,26,0,36,0,47,0,127,178,7,48,49,95,75,6,39,32,208,176,7,16,176,42,208,24,106,33,21,65,121,12,36,19,
    47,27,177,19,96,205,7,86,111,12,82,79,8,33,10,16,102,88,10,33,176,16,24,70,29,12,38,40,208,176,30,16,176,45,130,169,45,19,16,18,51,50,23,17,51,17,54,51,50,18,17,96,232,7,37,6,35,34,
    2,39,37,24,107,169,8,105,147,10,8,92,84,209,187,76,62,242,64,86,186,211,212,183,83,69,242,61,79,175,209,9,4,55,116,106,45,37,33,51,220,252,186,108,106,45,33,34,42,104,112,2,14,1,9,
    1,55,28,1,206,254,46,32,254,203,254,224,243,254,230,30,254,86,1,166,26,1,3,227,60,182,199,13,253,58,10,1,75,162,169,10,2,201,10,193,0,1,0,134,254,191,4,165,4,58,79,195,5,32,8,78,91,
    10,24,94,173,12,97,202,12,32,15,99,94,12,32,176,79,195,15,54,134,243,1,166,243,147,20,221,252,210,4,58,252,136,3,120,252,136,253,253,1,65,0,130,107,42,95,0,0,3,224,4,59,0,17,0,72,82,
    37,12,34,9,47,27,24,100,10,9,123,175,12,79,202,8,42,13,1,9,17,18,57,124,176,13,47,24,112,135,10,37,48,49,33,35,17,6,24,96,162,7,8,32,22,51,50,55,17,51,3,224,243,94,104,222,234,243,
    105,108,98,100,243,1,105,22,213,199,1,76,254,180,118,98,23,2,12,131,131,36,134,0,0,6,3,132,239,79,197,11,67,234,12,69,166,20,79,197,39,40,121,1,82,243,1,83,242,250,131,135,252,35,3,
    120,251,198,130,111,38,1,0,126,254,191,6,180,130,9,34,15,0,75,72,132,8,68,251,12,36,3,47,27,177,3,148,124,79,197,22,32,9,79,188,19,32,113,133,128,36,185,20,221,250,187,137,131,65,132,
    5,130,125,38,2,0,31,0,0,4,234,130,9,38,13,0,21,0,91,178,0,75,191,5,79,184,7,121,112,12,113,138,8,42,0,12,8,17,18,57,176,0,47,176,12,90,55,12,24,76,222,12,33,176,8,94,101,11,8,62,48,
    49,1,51,50,22,22,21,20,6,7,33,17,33,53,33,17,17,51,50,54,52,38,39,2,74,238,133,198,103,236,196,254,29,254,200,2,43,237,89,103,101,86,2,226,92,166,110,167,202,1,3,118,196,253,229,254,
    163,89,164,95,1,0,74,227,5,33,5,201,130,167,42,11,0,15,0,23,0,109,178,7,24,25,67,173,6,36,13,208,176,7,16,24,120,139,8,67,157,12,99,12,12,79,177,22,32,14,134,204,32,178,79,177,26,137,
    187,79,177,6,133,189,33,1,130,135,189,38,243,4,71,243,243,251,185,140,192,37,4,58,251,198,4,58,137,195,38,2,0,143,0,0,4,34,132,195,38,19,0,77,178,14,20,21,131,193,79,173,10,140,187,
    65,110,9,32,10,135,161,79,173,10,32,8,79,173,14,138,161,65,91,7,138,157,142,151,137,147,46,1,0,81,255,236,3,232,4,78,0,32,0,125,178,16,97,33,10,72,35,12,68,216,8,130,118,75,19,9,35,
    178,30,8,16,130,193,47,124,176,30,47,24,180,64,30,80,30,2,93,178,3,30,0,130,18,38,178,28,3,1,93,178,11,131,4,32,27,73,126,8,115,152,13,35,178,21,27,24,130,41,49,180,4,21,20,21,2,93,
    48,49,1,34,6,21,35,52,54,54,51,123,46,6,32,35,87,11,9,79,188,5,8,61,2,1,85,118,229,116,202,114,220,1,11,121,220,145,123,200,110,229,118,86,102,126,12,254,172,1,83,14,126,3,139,105,
    79,100,175,104,254,210,252,25,155,252,136,103,186,117,93,119,153,137,168,132,143,0,0,2,0,145,255,236,6,56,130,225,38,20,0,31,0,133,178,21,24,136,33,16,73,26,12,103,84,12,36,17,47,27,
    177,17,24,106,121,17,33,17,19,132,245,32,1,130,245,41,208,1,224,1,2,93,180,64,1,80,130,6,24,68,75,11,32,12,124,18,12,32,4,24,67,113,13,40,1,51,54,36,51,50,0,23,23,126,147,5,38,39,35,
    17,35,17,51,1,24,105,106,9,8,50,1,132,204,27,1,10,203,219,1,17,11,1,123,229,150,210,254,243,21,202,243,243,1,185,138,246,136,141,120,119,140,2,135,207,248,254,230,233,57,160,252,138,
    1,4,212,254,60,4,58,253,216,126,153,8,48,2,0,39,0,0,3,223,4,58,0,13,0,22,0,97,178,20,83,141,5,35,20,16,176,4,79,145,6,68,101,12,73,97,12,32,5,130,239,32,5,130,239,35,178,18,0,1,130,
    226,34,176,18,47,101,12,10,35,178,7,3,18,131,20,32,0,95,40,11,73,111,5,42,35,3,35,19,38,38,53,52,54,55,3,79,146,8,8,58,223,242,227,231,252,255,100,107,233,198,188,101,79,239,224,89,
    106,4,58,251,198,1,141,254,115,1,181,42,156,101,151,193,2,254,160,68,85,1,56,90,0,0,1,255,219,254,75,3,248,6,0,0,33,0,139,178,21,34,35,130,101,34,0,176,30,70,124,5,65,156,12,32,10,
    122,245,6,32,176,127,104,8,130,172,47,182,159,30,175,30,191,30,3,93,178,47,30,1,93,178,15,131,4,34,33,24,30,130,68,34,176,33,47,104,254,10,35,178,2,24,4,24,69,243,17,32,4,93,215,11,
    40,176,0,16,176,26,208,176,33,16,94,224,6,36,21,54,51,32,19,24,65,216,7,34,51,50,53,24,69,255,8,8,76,35,53,51,53,51,21,33,2,119,254,245,119,182,1,90,5,185,166,70,58,15,39,59,123,97,
    94,146,72,243,158,158,243,1,11,4,173,233,138,254,117,252,254,178,196,17,191,13,191,2,237,112,93,130,252,251,4,173,171,168,168,0,1,0,84,255,236,3,249,4,78,0,29,0,122,178,22,30,24,139,
    127,9,103,23,12,67,255,9,78,132,8,34,178,25,15,77,22,9,38,31,25,47,25,2,113,178,67,95,9,35,178,3,0,27,130,224,38,180,4,3,20,3,2,93,24,115,89,13,35,178,19,25,22,130,27,38,178,28,19,
    1,93,178,11,130,4,33,48,49,24,115,106,24,8,76,33,21,33,18,2,62,89,120,6,228,3,120,202,116,228,254,248,1,8,228,192,245,4,228,7,118,91,110,125,10,1,91,254,166,25,174,104,80,102,176,100,
    1,39,1,2,25,247,1,41,226,182,96,117,148,141,168,254,236,0,0,2,0,30,0,0,6,154,4,58,0,22,0,31,0,121,178,9,67,109,5,32,9,85,81,9,66,125,12,69,146,12,102,39,8,32,1,127,102,5,35,1,47,176,
    0,70,95,12,115,197,13,32,1,67,97,12,32,8,24,108,199,13,34,1,17,51,70,109,8,35,3,2,6,7,76,12,6,130,22,8,68,50,54,53,52,38,39,3,250,248,195,229,233,195,254,25,254,230,21,19,168,175,78,
    2,50,82,71,10,20,2,243,237,88,104,100,86,4,58,254,135,3,188,159,160,193,2,3,118,254,135,254,242,238,1,202,5,11,175,227,1,206,253,197,254,193,88,77,72,81,1,130,231,32,134,130,231,32,
    177,130,231,38,18,0,27,0,130,178,1,105,135,5,32,1,115,187,9,36,2,47,27,177,2,79,171,20,109,68,12,137,244,41,17,11,17,18,57,176,1,47,178,4,133,9,34,4,47,176,24,82,14,12,33,176,4,67,
    120,11,33,176,11,96,158,11,37,48,49,1,33,17,51,138,243,35,17,35,17,51,135,236,37,35,1,121,1,165,243,135,239,45,91,243,243,2,152,237,90,102,100,91,2,159,1,155,135,232,37,1,221,254,35,
    4,58,131,223,50,90,75,70,84,0,0,1,255,238,0,0,3,248,6,0,0,24,0,121,24,66,223,8,32,21,67,147,18,32,7,130,231,32,7,69,35,7,136,205,37,191,21,1,93,178,47,131,4,32,15,131,4,34,24,15,21,
    131,220,32,24,67,143,13,33,4,7,131,20,130,214,70,215,10,39,0,16,176,17,208,176,24,16,96,176,6,67,129,5,32,35,67,120,16,36,139,254,225,119,182,24,114,80,8,41,139,139,243,1,31,4,181,
    241,138,254,24,114,85,9,52,4,181,170,161,161,0,1,0,134,254,154,4,18,4,58,0,11,0,69,0,176,74,209,19,73,103,12,24,82,38,9,34,69,88,176,73,228,18,89,30,9,45,35,17,33,17,1,121,1,166,243,
    254,181,243,254,178,73,90,5,37,251,198,254,154,1,102,77,189,5,44,136,255,235,6,193,5,176,0,30,0,96,178,6,24,70,141,10,93,3,12,100,138,12,24,141,119,12,36,4,47,27,177,4,24,118,134,16,
    34,6,0,4,109,37,14,33,26,208,131,146,33,20,6,104,111,5,36,38,53,17,51,17,103,174,5,130,158,134,8,8,56,6,193,249,210,229,109,113,233,207,243,253,103,94,105,114,1,1,109,99,97,110,5,176,
    251,255,214,238,165,165,239,213,4,1,251,252,117,130,129,119,4,3,251,252,116,131,127,121,4,3,0,1,0,112,255,235,5,237,130,200,148,189,66,213,7,74,29,12,132,189,76,37,12,145,189,32,21,
    149,189,32,6,146,189,136,198,8,38,5,237,1,218,189,199,96,102,203,184,213,243,84,70,83,102,244,92,79,74,91,4,58,253,78,193,220,142,142,221,195,2,175,253,81,114,108,108,114,137,7,49,
    0,2,255,224,0,0,4,33,6,24,0,18,0,27,0,113,178,21,67,155,5,33,21,16,98,47,8,36,15,47,27,177,15,85,223,7,65,229,8,34,18,15,9,70,244,6,111,51,9,33,178,2,133,20,41,2,47,176,0,16,176,11,
    208,176,18,90,33,6,33,178,19,69,149,8,33,176,9,67,138,17,68,124,7,38,35,53,51,17,51,17,33,67,138,7,8,65,39,2,163,254,222,247,196,229,229,192,254,18,174,174,243,1,34,254,222,237,91,
    101,99,87,4,58,254,201,3,206,174,173,211,4,4,58,171,1,51,254,205,253,91,254,130,101,89,85,105,2,0,1,0,152,255,237,6,205,5,197,0,37,0,142,178,14,120,247,10,32,36,130,197,32,36,24,103,
    203,12,135,12,89,88,7,37,176,0,69,88,176,34,130,38,39,34,15,62,89,178,0,34,36,131,223,42,0,47,178,31,0,1,113,178,8,36,28,131,14,32,5,91,123,12,130,228,33,15,208,130,234,94,96,11,35,
    18,208,176,28,71,37,11,33,178,24,132,57,34,48,49,1,88,95,5,34,0,23,35,89,90,5,102,221,8,38,51,6,0,35,34,36,2,72,215,6,8,88,148,181,11,150,1,9,171,241,1,38,24,252,18,147,142,161,171,
    11,1,233,254,22,2,168,162,149,150,20,252,22,254,211,248,172,254,248,147,3,180,252,252,3,79,190,1,29,155,254,250,239,157,139,221,204,195,225,242,134,156,233,254,251,161,1,52,202,253,
    116,5,176,0,0,1,0,134,255,236,5,186,4,78,0,35,0,146,178,13,36,37,97,199,8,72,36,12,32,35,130,239,32,35,66,136,7,24,76,34,7,37,176,0,69,88,176,32,130,25,39,32,15,62,89,178,14,4,27,89,
    97,8,42,64,14,80,14,2,93,176,0,208,176,4,96,108,11,35,178,8,14,11,130,35,34,176,14,16,87,5,10,33,176,27,104,36,12,34,22,19,15,131,34,38,15,16,176,30,208,48,49,73,212,5,32,22,65,12,
    5,32,3,65,11,9,36,14,2,35,34,36,65,10,6,38,121,157,20,1,4,210,193,71,76,5,40,219,26,1,124,254,133,10,125,110,71,106,7,8,45,211,254,253,20,158,243,243,2,113,222,255,226,182,96,117,254,
    230,171,138,142,104,80,102,176,100,254,220,254,58,4,58,0,0,2,0,28,0,0,5,23,5,176,0,11,0,14,102,37,7,32,8,130,213,32,8,65,249,7,83,204,12,101,128,21,34,13,8,2,131,181,32,13,99,137,15,
    32,14,132,23,99,124,7,8,32,3,33,1,51,1,33,1,33,3,3,131,126,225,115,143,254,250,2,6,245,2,0,254,250,253,224,1,83,168,1,170,254,86,131,3,39,5,176,250,80,2,104,1,248,131,153,36,10,0,0,
    4,69,130,164,34,11,0,16,140,153,84,216,20,150,153,33,2,8,134,153,94,158,10,35,4,208,178,15,141,153,32,35,130,153,130,3,57,3,39,7,2,228,93,195,91,104,247,1,169,231,1,171,247,254,92,
    248,100,25,25,1,23,254,233,131,3,57,4,58,251,198,1,196,1,6,100,100,0,2,0,172,0,0,7,48,5,176,0,19,0,22,0,124,83,75,5,24,135,222,12,24,107,21,12,69,121,20,32,176,122,152,11,132,12,105,
    69,8,34,21,2,4,125,2,12,32,6,68,24,9,41,10,208,176,6,16,176,14,208,178,22,132,35,35,48,49,1,33,130,187,35,33,3,35,17,130,1,34,3,33,19,104,54,5,8,35,33,3,1,168,1,104,1,43,245,2,0,254,
    250,142,126,226,114,143,254,250,152,254,219,252,252,2,98,1,83,169,2,103,3,73,250,80,65,113,7,42,1,171,254,85,5,176,252,184,1,249,0,130,215,38,157,0,0,6,24,4,58,130,215,34,24,0,127,
    138,215,65,115,7,132,215,69,147,28,154,215,33,0,16,75,238,5,35,47,176,1,208,103,51,11,48,11,208,176,7,208,176,1,16,176,20,208,176,21,208,178,23,18,131,254,130,218,36,51,19,51,1,35,
    134,218,33,35,19,76,210,5,8,33,51,3,39,7,1,144,254,248,231,1,171,247,106,93,195,91,104,247,109,186,243,243,1,237,248,100,25,25,1,196,2,118,251,198,65,175,7,65,179,5,33,253,138,65,177,
    6,32,128,130,217,52,110,5,176,0,26,0,29,0,122,178,27,30,31,17,18,57,176,27,16,176,13,24,88,103,11,24,130,117,15,145,204,32,19,108,146,6,34,178,0,25,131,165,34,176,0,47,110,41,11,44,
    14,208,176,15,208,176,0,16,176,24,208,178,27,133,32,32,25,107,198,14,42,22,22,23,17,35,17,38,38,35,35,7,130,7,34,35,34,6,131,6,8,63,54,54,33,1,33,1,19,33,4,122,254,241,5,252,2,118,
    143,104,6,252,126,143,117,3,252,3,250,1,15,254,133,4,228,253,142,233,254,47,3,40,4,217,216,254,141,1,108,129,111,11,253,175,2,92,110,126,254,144,1,108,225,219,2,136,130,229,42,169,
    0,2,0,130,0,0,5,100,4,58,144,227,106,215,7,32,5,130,188,32,5,85,66,20,74,65,12,136,227,46,4,5,0,17,18,57,176,4,47,176,7,208,176,4,16,89,20,10,109,90,5,33,178,27,133,32,32,5,141,227,
    36,51,53,54,54,55,130,207,130,234,37,21,35,53,38,38,39,136,234,32,21,130,227,8,57,130,2,197,204,254,235,3,244,254,234,198,190,2,243,1,94,114,47,1,242,45,121,96,3,1,133,149,254,214,
    178,206,210,13,1,219,254,36,17,211,199,179,177,127,114,2,3,254,95,1,164,110,124,186,2,105,1,34,0,130,225,48,163,0,0,8,179,5,176,0,32,0,35,0,151,178,28,36,37,131,159,34,28,16,176,24,
    78,197,7,99,24,12,97,106,12,86,208,12,73,148,12,79,147,12,42,25,47,27,177,25,15,62,89,178,9,7,132,251,32,9,78,176,11,48,176,9,16,176,13,208,176,3,16,176,28,208,176,23,208,178,33,133,
    35,32,11,95,50,13,42,33,17,52,55,33,17,35,17,51,17,33,65,4,5,65,239,15,49,1,19,33,2,197,59,254,159,252,252,3,48,254,135,4,229,254,132,65,245,7,56,5,252,127,145,115,3,2,8,233,254,46,
    1,96,161,101,253,154,5,176,253,123,2,133,253,120,65,248,8,55,9,253,173,2,92,113,124,254,145,3,57,1,170,0,0,2,0,143,0,0,7,118,4,58,65,17,6,32,29,65,17,5,32,29,65,17,14,24,116,174,20,
    65,17,48,32,11,65,17,8,81,129,9,65,17,15,133,35,65,17,15,33,53,54,65,17,13,66,22,18,55,2,149,1,53,254,183,243,243,2,165,254,236,3,244,254,234,197,190,2,242,1,94,115,46,66,28,10,47,
    176,148,100,254,88,4,58,254,39,1,217,254,36,17,212,198,66,33,20,48,40,254,64,3,170,7,136,0,39,0,48,0,167,178,2,49,50,99,133,6,36,40,208,0,176,44,76,109,5,36,5,47,27,177,5,24,102,200,
    12,87,110,7,66,10,8,32,5,98,14,12,34,38,5,17,130,69,43,124,176,38,47,24,178,16,38,1,93,178,64,130,4,38,180,96,38,112,38,2,93,126,227,10,35,178,12,35,38,130,39,34,176,17,16,24,85,13,
    10,50,178,15,44,1,93,176,44,16,176,41,208,176,41,47,180,15,41,31,41,130,52,34,40,44,41,131,41,51,48,208,176,48,47,48,49,1,52,38,35,33,53,33,50,4,21,20,6,7,130,4,56,4,35,35,6,21,20,
    23,7,38,38,39,52,54,55,51,54,54,53,52,33,35,53,51,32,3,24,76,255,7,8,76,2,150,133,122,254,229,1,21,237,1,11,125,110,1,12,254,247,232,53,122,152,82,132,162,2,177,164,63,114,137,254,
    207,137,137,1,16,148,147,207,254,234,151,254,235,206,4,33,94,106,199,207,181,112,163,44,87,254,197,232,3,99,107,65,153,40,183,127,134,139,2,1,125,101,243,199,3,159,24,77,64,7,46,2,
    0,51,254,72,3,136,6,28,0,39,0,48,0,149,65,59,26,66,78,7,36,23,47,27,177,23,65,59,7,32,18,130,12,37,18,15,62,89,176,5,24,102,198,12,34,37,18,5,130,233,41,124,176,37,47,24,180,64,37,
    80,37,130,252,32,36,66,62,8,33,178,12,66,175,5,32,18,65,49,11,65,44,17,33,41,44,65,44,5,65,41,9,36,22,21,20,6,7,131,4,65,41,13,32,50,65,41,6,32,50,65,41,9,8,100,116,115,105,254,228,
    1,23,220,248,97,87,217,246,208,54,126,144,81,130,150,2,169,161,53,108,119,254,249,145,149,226,160,146,208,254,233,150,254,235,205,2,254,60,71,185,165,141,79,119,36,66,172,150,175,4,
    98,107,65,145,48,182,112,125,135,1,80,63,148,169,3,18,155,11,254,234,1,23,10,0,0,3,0,95,255,236,5,23,5,196,0,16,0,23,0,30,0,102,178,4,31,32,131,158,37,4,16,176,17,208,176,130,5,32,
    24,70,81,6,77,138,12,72,3,8,32,12,95,224,11,35,178,20,4,12,130,59,36,124,176,20,47,24,130,58,113,254,11,35,20,16,178,28,65,39,8,24,83,86,14,51,32,4,18,23,1,34,6,7,33,38,38,3,50,54,
    55,33,22,22,5,23,24,72,91,17,51,253,164,160,182,8,2,188,8,180,160,159,179,10,253,68,10,184,2,178,214,24,72,96,15,43,1,239,240,217,219,238,251,202,229,222,217,234,131,219,48,79,255,
    236,4,61,4,78,0,15,0,22,0,29,0,103,178,4,71,39,5,130,213,32,16,132,219,24,80,205,7,75,31,12,102,171,8,86,239,9,33,178,27,134,216,43,27,47,24,180,64,27,80,27,2,93,178,19,136,206,131,
    234,119,186,9,37,48,49,19,52,54,54,84,189,10,33,17,1,133,212,32,19,133,226,8,70,79,125,228,148,218,1,19,11,1,123,231,149,227,254,236,1,247,107,133,16,253,255,16,132,107,106,133,16,
    2,0,16,133,2,39,161,253,137,254,231,234,57,160,252,138,1,46,1,1,254,147,146,137,136,147,2,221,149,130,130,149,0,0,1,0,16,0,0,4,243,5,194,130,211,34,70,178,2,94,23,10,24,72,94,12,36,
    15,47,27,177,15,24,115,69,16,39,1,12,15,17,18,57,176,6,24,64,255,14,34,23,55,19,130,176,8,44,23,7,35,6,7,1,35,1,33,2,97,27,27,228,53,156,122,45,2,24,84,39,254,152,244,254,14,1,13,1,
    139,114,111,2,247,172,151,1,215,2,124,251,148,5,176,130,135,32,32,130,135,37,24,4,78,0,17,0,130,135,108,195,10,32,5,71,149,11,97,131,12,122,64,8,34,1,5,14,77,89,6,32,10,74,11,8,34,
    48,49,1,130,135,39,18,51,50,23,7,38,35,34,132,137,8,58,51,1,227,20,20,122,90,207,67,39,23,12,32,34,59,13,254,246,211,254,146,251,1,110,97,97,1,190,1,34,22,192,6,54,42,252,226,4,58,
    0,2,0,95,255,118,5,23,6,46,0,19,0,39,0,85,178,5,40,41,133,94,120,5,8,102,84,12,94,159,8,38,6,208,176,13,16,176,16,131,5,33,178,26,136,136,36,176,23,208,176,3,24,125,64,11,130,69,130,
    156,54,16,0,7,21,35,53,38,0,3,53,16,0,55,53,51,21,22,0,17,39,52,38,39,130,19,38,6,6,21,21,20,22,23,130,19,8,75,54,54,53,5,23,254,243,233,198,232,254,239,3,1,18,233,198,234,1,13,253,
    130,120,198,121,133,132,123,198,121,128,2,178,254,218,254,139,35,126,126,35,1,115,1,29,85,1,36,1,122,35,113,114,35,254,134,254,217,6,206,245,35,96,97,35,245,207,76,199,253,37,96,95,
    35,246,207,130,213,38,79,255,136,4,61,4,180,130,213,38,37,0,88,178,3,38,39,131,213,32,3,89,201,9,81,152,12,88,129,8,130,32,130,216,36,16,16,176,13,208,93,75,14,32,20,132,216,32,29,
    137,233,38,26,208,48,49,19,52,18,132,206,32,18,130,196,32,2,132,226,34,2,53,1,130,200,136,219,132,218,8,85,79,221,189,184,191,221,223,191,184,187,221,2,80,82,90,90,80,184,79,88,86,
    79,184,2,39,218,1,38,31,110,109,31,254,216,221,17,219,254,217,29,107,108,31,1,38,221,254,167,30,181,151,130,178,31,96,96,33,178,149,131,174,33,104,0,0,3,0,136,255,235,6,181,7,63,0,
    42,0,61,0,70,0,186,178,48,71,72,131,207,37,48,16,176,9,208,176,130,5,32,69,68,103,6,81,255,12,76,120,12,83,73,12,34,11,47,27,97,136,5,127,24,5,33,176,18,83,252,15,65,209,9,35,178,30,
    11,18,131,106,39,35,208,176,19,16,176,42,208,130,43,46,176,54,208,176,54,47,176,44,208,176,44,47,178,43,8,24,79,17,7,42,176,44,16,176,50,208,176,50,47,178,57,140,19,46,66,208,176,66,
    47,176,70,208,176,70,47,48,49,1,50,24,92,243,7,44,6,35,34,38,39,17,52,54,51,21,34,6,21,82,86,7,8,63,51,17,22,22,51,50,54,53,17,52,38,35,19,21,35,34,46,2,35,34,21,21,35,53,52,51,50,
    30,2,1,54,55,53,51,21,20,6,7,4,244,206,242,1,241,208,227,114,114,227,206,240,4,243,207,95,102,102,95,105,114,245,1,113,104,131,9,8,90,106,33,83,138,191,48,20,104,134,235,37,70,201,
    111,254,41,65,3,169,96,59,5,176,250,221,253,234,221,251,158,158,246,213,2,32,221,253,204,142,128,253,237,128,142,129,119,1,130,254,121,115,128,142,128,2,19,128,142,1,227,134,35,75,
    10,104,16,34,220,15,79,26,254,135,82,60,104,103,49,120,31,0,0,3,0,116,255,235,5,209,5,227,65,127,6,34,175,178,9,65,127,5,37,9,16,176,58,208,176,130,5,32,70,65,127,6,77,19,12,75,72,
    8,52,18,16,176,0,208,176,0,47,176,11,16,176,7,208,178,9,18,11,17,18,57,65,116,53,35,45,208,176,45,65,116,12,32,45,65,116,18,38,54,16,176,65,208,176,65,65,116,12,32,21,83,197,8,65,116,
    8,38,21,20,22,51,50,54,55,121,217,7,33,53,53,65,116,29,8,77,58,186,220,1,212,181,197,97,99,194,178,211,4,220,187,73,91,83,67,80,94,1,236,1,94,81,66,84,91,73,189,36,83,138,193,44,21,
    104,135,235,37,70,197,112,254,48,65,3,169,96,59,4,71,229,204,248,204,231,145,145,224,197,1,3,205,231,195,117,124,245,124,117,112,106,202,202,106,112,132,10,46,1,231,134,35,76,9,104,
    16,34,220,15,78,27,254,133,65,112,7,48,2,0,136,255,235,6,193,7,17,0,30,0,38,0,125,178,6,105,67,5,32,6,74,201,9,68,147,12,87,231,8,37,4,208,178,6,8,13,101,181,6,32,17,67,170,9,44,13,
    16,176,21,208,176,21,47,176,17,16,176,26,130,8,39,16,176,30,208,176,30,47,176,130,23,44,37,208,176,37,47,176,38,208,176,38,47,178,32,66,182,9,32,38,131,106,34,176,35,47,84,254,32,39,
    37,53,33,23,33,21,35,53,85,6,20,39,252,57,3,85,1,254,166,181,85,14,27,38,231,122,122,127,127,0,2,85,19,5,33,5,177,132,241,32,137,138,241,32,37,66,89,6,132,241,73,108,7,85,14,12,36,
    30,47,27,177,30,79,135,28,35,178,6,8,21,85,217,16,137,253,35,31,208,176,31,140,253,32,31,24,76,174,8,33,1,17,85,62,28,135,253,85,70,20,39,252,157,3,56,4,254,178,181,85,78,27,34,252,
    123,123,130,253,46,1,0,102,254,140,4,182,5,197,0,24,0,83,178,23,124,15,10,32,10,24,169,63,11,32,0,130,12,32,0,118,203,16,38,10,16,176,14,208,176,10,123,239,12,32,2,89,189,11,50,48,
    49,1,35,17,38,0,53,17,52,18,36,51,32,0,21,35,16,33,68,113,5,8,48,23,51,3,52,251,211,255,0,141,1,1,163,1,0,1,31,252,254,221,140,169,169,138,159,254,140,1,102,32,1,71,249,1,17,175,1,
    24,155,254,247,233,1,38,223,188,254,237,182,223,130,36,41,0,92,254,137,3,243,4,78,0,26,130,169,32,25,126,67,10,95,93,12,152,169,32,15,132,169,24,163,216,10,130,169,24,151,124,9,133,
    169,40,2,53,53,52,54,54,51,50,22,24,67,236,11,8,55,23,51,2,213,243,179,211,121,219,146,124,198,111,229,116,88,113,130,126,112,152,254,137,1,106,32,1,35,220,28,155,252,137,103,187,118,
    91,122,189,168,27,161,187,2,0,1,0,109,0,0,4,147,5,62,0,19,130,1,33,176,14,89,88,10,34,15,62,89,130,101,40,5,7,37,3,35,19,37,55,5,132,3,33,51,3,130,15,8,73,2,91,1,33,72,254,221,181,
    175,225,254,223,71,1,37,202,254,222,73,1,35,185,172,228,1,37,76,254,224,1,193,172,128,170,254,193,1,142,171,128,171,1,104,171,130,171,1,70,254,107,171,127,170,0,1,252,102,4,162,255,
    57,5,253,0,7,0,17,0,176,0,47,178,3,6,70,15,7,130,103,55,21,39,55,33,39,23,21,253,23,177,1,2,34,1,177,5,32,126,1,238,108,1,220,0,130,55,54,115,5,23,255,109,6,21,0,15,0,46,0,176,11,47,
    176,7,208,176,7,47,178,0,67,130,9,44,11,16,176,4,208,176,4,47,176,11,16,178,12,136,22,130,84,8,39,50,21,21,35,53,52,35,34,4,7,35,53,51,54,36,254,127,238,136,106,54,254,226,139,41,39,
    121,1,24,6,21,220,34,16,104,119,1,134,1,119,130,101,53,253,123,5,22,254,114,6,96,0,5,0,12,0,176,1,47,176,5,208,176,5,47,130,67,52,53,51,7,23,7,253,123,189,1,59,82,5,220,132,150,112,
    68,0,1,253,165,130,43,32,156,135,43,33,3,47,69,178,5,130,43,58,39,55,39,51,21,253,247,82,59,1,189,5,22,68,112,150,132,0,8,250,36,254,196,1,191,5,175,130,85,56,26,0,39,0,53,0,66,0,79,
    0,92,0,106,0,122,0,176,69,47,176,83,47,176,96,47,24,176,212,7,84,11,7,24,87,245,11,42,69,16,176,16,208,176,69,16,178,76,9,71,61,8,38,23,208,176,83,16,176,30,131,5,33,178,90,137,22,
    38,37,208,176,96,16,176,43,131,5,33,178,103,137,22,38,50,208,176,56,16,178,63,136,16,130,167,34,52,54,50,66,49,8,35,1,52,54,51,137,13,32,19,136,13,32,34,24,176,235,8,35,35,52,38,35,
    133,13,140,53,140,12,134,66,130,38,137,65,131,52,63,253,17,115,190,116,112,51,48,46,51,1,222,116,93,95,117,113,53,46,44,51,72,117,93,95,116,112,53,92,51,254,203,130,19,130,9,36,46,
    45,51,253,79,135,40,34,253,77,116,134,50,33,254,222,135,40,32,53,130,8,131,59,51,45,51,4,243,84,104,104,84,46,55,53,48,254,235,84,104,103,85,49,52,130,9,42,9,85,103,104,84,49,52,55,
    46,253,249,131,29,131,9,33,254,228,133,39,35,55,46,5,26,136,49,138,39,33,85,103,133,59,8,45,0,8,250,77,254,99,1,140,5,198,0,4,0,9,0,14,0,19,0,24,0,29,0,34,0,39,0,47,0,176,33,47,176,
    22,47,176,18,47,176,11,47,176,27,47,176,38,67,30,5,82,46,12,8,57,2,47,27,177,2,17,62,89,48,49,5,23,3,35,19,3,39,19,51,3,1,55,5,21,37,5,7,37,53,5,1,55,37,23,5,1,7,5,39,37,3,39,3,55,
    19,1,23,19,7,3,254,80,11,122,96,70,58,12,130,4,8,60,2,29,13,1,77,254,166,251,117,13,254,179,1,90,3,156,2,1,64,68,254,219,252,243,2,254,192,69,1,38,43,17,148,65,198,3,96,17,148,66,196,
    60,14,254,173,1,97,4,162,14,1,82,254,160,254,17,12,124,98,71,59,131,4,60,1,174,16,153,68,200,252,142,17,153,69,200,2,228,2,1,70,69,254,213,252,227,2,254,187,71,1,43,0,90,129,8,38,98,
    0,18,0,27,0,116,90,129,18,71,29,12,123,75,12,90,142,7,33,176,17,24,73,187,11,35,178,2,13,9,117,44,8,39,176,11,208,176,12,208,176,2,74,18,12,90,132,29,34,53,51,21,90,132,32,35,5,5,253,
    254,90,132,5,38,5,5,171,178,178,252,144,90,130,7,43,0,2,0,148,0,0,4,217,5,176,0,14,130,207,34,77,178,4,91,81,5,32,4,95,213,9,118,114,12,103,166,8,39,22,3,1,17,18,57,176,22,88,106,12,
    32,3,91,45,14,52,17,35,17,33,50,4,21,20,7,23,7,39,6,35,19,54,53,52,38,39,33,130,18,60,55,39,55,1,145,253,2,45,244,1,31,117,122,109,136,121,170,249,28,144,126,254,201,1,48,79,58,115,
    110,24,157,90,7,50,193,119,135,100,150,55,1,67,53,74,118,141,2,254,4,22,128,100,0,130,173,24,140,121,7,40,19,0,34,0,110,178,23,35,36,123,137,5,33,176,16,71,169,6,104,84,12,71,182,12,
    32,10,98,97,11,32,7,94,218,6,35,178,9,16,7,130,65,33,178,14,132,6,33,176,16,96,120,12,32,7,85,77,13,32,1,134,199,33,34,39,130,215,34,51,23,54,24,149,105,13,130,211,44,23,54,4,48,110,
    106,111,104,89,112,178,107,243,24,140,133,12,46,70,50,106,110,89,34,2,18,244,151,122,99,120,54,117,24,140,140,16,54,33,123,100,103,88,0,1,0,143,0,0,4,52,7,16,0,7,0,50,178,1,8,9,91,
    5,13,90,8,16,113,32,20,49,17,51,4,52,253,88,253,2,178,243,4,228,251,28,5,176,1,96,130,87,38,126,0,0,3,91,5,115,130,87,32,43,110,51,18,113,113,29,41,17,51,3,91,254,22,243,1,235,242,
    107,182,5,34,1,57,0,130,81,46,155,254,198,4,157,5,176,0,20,0,91,178,15,21,22,132,169,32,9,67,203,5,118,8,12,83,200,8,32,19,127,178,12,33,3,19,131,220,39,176,3,47,176,9,16,178,10,80,
    143,9,33,3,16,101,105,10,96,250,5,8,58,32,0,17,16,0,35,39,50,54,53,2,37,35,17,35,17,33,4,55,253,96,168,1,34,1,60,254,246,243,1,131,136,2,254,171,188,252,3,156,4,228,254,95,254,205,
    254,236,254,244,254,214,186,179,194,1,123,9,253,135,130,159,48,1,0,126,254,226,3,219,4,58,0,21,0,74,178,11,22,23,132,169,32,10,133,169,24,72,203,12,83,54,8,32,20,141,169,33,20,10,130,
    50,130,169,32,178,81,84,9,131,152,46,21,51,32,0,21,20,6,6,7,39,54,53,52,38,35,132,153,8,34,3,70,254,43,73,1,1,1,32,94,171,115,85,222,155,142,78,243,2,200,3,118,229,254,250,221,96,194,
    141,29,174,74,212,129,151,91,237,5,38,1,0,144,0,0,5,54,130,159,34,20,0,97,122,99,18,82,177,12,91,248,12,114,191,8,34,15,10,12,131,151,38,15,47,178,159,15,1,93,118,70,10,35,178,1,8,
    15,131,25,49,5,208,176,15,16,176,18,208,48,49,9,2,33,1,35,21,35,53,131,163,8,39,51,17,51,53,51,21,51,1,5,13,254,124,1,173,254,193,254,211,65,163,89,253,253,89,163,55,1,27,5,176,253,
    91,252,245,2,109,233,233,253,147,130,11,51,154,254,254,2,102,0,0,1,0,142,0,0,4,174,4,58,0,20,0,92,133,171,67,87,12,65,68,12,24,78,171,12,32,3,24,89,205,7,118,246,5,32,176,24,117,16,
    12,34,178,1,9,81,74,5,34,208,176,14,136,166,32,3,142,166,8,38,19,4,148,254,196,1,86,254,203,216,47,155,87,242,242,87,155,39,207,4,58,253,254,253,200,1,172,178,178,254,84,4,58,254,80,
    199,199,1,176,130,163,40,52,0,0,6,162,5,176,0,14,65,79,7,82,84,12,74,201,12,65,79,12,132,202,43,15,62,89,178,8,6,2,17,18,57,176,8,118,169,11,32,176,24,110,8,12,35,178,12,1,8,24,64,
    195,9,8,43,33,53,33,17,51,1,33,1,1,33,3,182,173,252,254,39,2,213,139,1,173,1,54,254,12,2,31,254,208,2,112,253,144,4,236,196,253,156,2,100,253,71,253,9,130,159,38,61,0,0,5,168,4,58,
    130,159,32,107,65,67,5,110,233,12,74,191,12,149,159,33,9,10,132,159,41,9,47,178,47,9,1,113,178,140,9,115,232,13,142,169,33,0,9,148,169,8,32,64,123,242,254,106,2,136,108,1,42,1,45,254,
    120,1,168,254,197,1,172,254,84,3,118,196,254,80,1,176,253,249,253,205,130,169,42,148,0,0,7,131,5,176,0,13,0,135,93,79,18,66,153,12,93,235,21,34,1,2,6,100,238,6,33,159,1,130,164,33,
    111,1,130,174,32,223,131,4,36,15,1,1,114,178,130,19,34,113,178,63,130,14,40,180,47,1,63,1,2,114,178,124,130,36,33,176,2,86,170,11,33,176,1,83,230,14,36,33,17,33,21,33,117,121,5,57,
    17,51,1,145,2,139,3,103,253,149,252,253,117,253,253,3,82,2,94,195,251,19,2,135,253,121,96,71,5,38,126,0,0,5,102,4,58,130,189,32,102,93,53,18,98,214,12,150,189,32,12,131,189,37,124,
    176,1,47,24,180,105,251,5,172,156,50,113,1,165,2,80,254,163,243,254,91,243,243,2,119,1,195,196,252,138,113,77,5,39,0,1,0,155,254,196,7,239,130,166,44,22,0,104,178,16,23,24,17,18,57,
    0,176,7,68,137,5,100,32,12,24,73,109,8,34,0,69,88,24,83,138,8,35,178,1,21,7,65,86,5,36,176,7,16,178,8,69,50,9,102,73,13,32,21,86,213,11,35,48,49,1,51,69,62,16,38,17,35,17,33,5,20,125,
    69,64,12,40,145,252,253,127,252,4,121,3,65,69,65,14,32,137,70,67,5,130,187,44,126,254,230,6,186,4,58,0,24,0,87,178,18,78,35,6,101,114,6,24,119,140,12,24,86,191,9,130,187,69,82,7,35,
    178,1,23,8,133,187,69,221,10,33,176,23,106,109,14,69,80,9,69,81,9,131,172,47,4,10,125,1,7,1,44,93,171,115,85,117,105,165,154,127,114,67,5,49,2,148,254,251,222,97,191,142,29,173,40,
    143,103,130,151,254,54,3,120,18,5,51,0,2,0,103,255,235,5,215,5,197,0,37,0,50,0,133,178,22,51,52,131,123,34,22,16,176,24,122,173,7,73,161,12,36,29,47,27,177,29,94,110,17,39,208,176,
    0,47,178,2,4,29,73,153,6,24,130,128,12,32,176,106,89,15,32,178,24,68,28,9,39,176,2,16,176,41,208,176,29,24,187,192,13,43,5,34,39,6,35,34,36,2,39,53,52,18,83,115,5,39,21,20,18,51,50,
    55,38,17,130,16,45,51,50,18,17,21,16,7,22,51,1,20,22,23,54,130,17,8,110,38,35,34,6,21,5,215,223,179,148,183,187,254,212,169,3,125,225,140,102,126,219,178,49,41,226,237,184,194,243,
    187,92,106,253,142,101,99,162,96,88,84,94,21,71,71,174,1,54,191,201,175,1,30,161,212,225,189,184,215,254,249,7,203,1,68,203,240,1,53,254,191,254,250,198,254,218,202,20,2,25,132,213,
    72,143,1,9,213,174,171,175,161,0,2,0,97,255,235,4,201,4,78,0,34,0,46,0,140,178,4,47,48,89,51,6,93,185,7,34,11,47,27,117,17,9,32,26,130,12,32,26,80,222,20,32,0,130,25,39,0,15,62,89,
    178,2,4,26,65,36,6,32,11,100,111,12,104,211,13,36,0,16,178,34,3,77,110,8,41,2,16,176,37,208,176,26,16,178,43,82,39,8,33,48,49,65,36,5,38,0,17,53,52,18,51,21,86,154,5,34,51,55,38,24,
    136,40,7,40,21,20,7,22,51,1,20,23,54,130,16,8,94,38,35,34,6,21,4,201,186,147,122,144,229,254,212,219,170,64,75,154,125,37,143,182,148,150,189,129,77,88,254,14,120,99,61,49,50,59,18,
    54,57,1,66,1,4,66,207,1,12,202,4,148,123,73,166,204,2,149,226,122,187,234,255,205,119,211,148,17,1,143,170,108,99,169,123,107,135,120,106,0,1,0,45,254,161,6,183,5,176,0,15,0,79,0,176,
    13,67,146,5,99,124,17,66,37,7,24,115,11,8,32,2,24,82,214,12,36,5,208,176,14,16,126,202,11,8,43,10,208,48,49,1,33,53,33,21,33,17,33,17,51,17,51,3,35,17,33,1,141,254,160,3,190,254,159,
    2,129,252,176,20,231,251,209,4,236,196,196,251,222,4,230,24,66,255,9,38,38,254,191,5,58,4,58,130,139,32,75,136,139,87,47,12,32,15,24,95,7,7,32,3,69,52,12,35,0,208,176,15,140,126,40,
    3,16,176,8,208,176,6,16,176,132,135,32,35,130,135,130,127,136,135,37,27,245,2,195,219,1,115,191,6,38,3,119,195,195,253,75,3,115,193,10,44,128,0,0,4,225,5,176,0,24,0,79,178,5,67,239,
    6,24,85,239,8,65,8,7,95,215,12,65,21,7,41,178,5,14,0,17,18,57,176,5,47,131,126,32,5,24,75,106,12,41,17,208,48,49,1,17,22,23,22,23,130,135,33,54,55,130,4,35,35,17,6,7,94,132,5,8,62,
    17,1,125,2,79,53,110,163,108,100,253,253,96,112,163,246,250,1,5,176,254,44,152,57,39,5,1,43,254,220,10,25,2,167,250,80,2,60,24,10,235,229,6,234,223,1,205,0,1,0,116,0,0,3,245,4,59,0,
    22,0,81,178,6,69,79,6,131,163,83,160,12,69,242,12,76,190,8,33,15,1,91,238,5,34,15,47,24,24,85,249,10,33,176,4,73,51,8,32,33,138,152,32,51,130,177,134,175,8,39,3,245,243,69,49,163,182,
    190,1,242,1,130,163,59,59,243,1,105,14,5,138,139,19,208,177,1,80,254,176,172,31,1,11,254,239,6,14,2,12,0,130,157,44,133,0,0,4,229,5,176,0,17,0,70,178,5,90,101,10,132,131,65,65,7,95,
    250,12,78,31,7,34,178,5,1,65,65,6,99,156,10,34,48,49,51,131,131,34,51,32,4,98,182,6,42,34,7,17,133,252,160,178,1,5,1,12,24,68,205,8,8,34,253,195,41,230,233,254,51,1,208,139,118,42,
    253,89,0,0,2,0,22,255,233,5,188,5,196,0,28,0,36,0,100,178,22,37,38,69,11,6,67,237,7,35,14,47,27,177,24,116,146,8,67,211,8,34,30,0,14,131,39,34,30,47,178,83,45,10,40,4,208,176,30,16,
    176,10,208,176,24,87,190,12,33,176,14,97,116,13,46,5,32,0,17,53,38,38,53,51,20,23,52,18,36,23,130,13,8,107,21,33,21,20,22,51,50,55,23,6,6,1,33,53,52,38,35,34,6,3,220,254,210,254,170,
    155,167,181,141,148,1,8,158,1,8,1,34,252,152,203,189,177,172,49,67,216,254,5,2,108,154,148,142,176,23,1,84,1,43,60,24,212,170,182,42,174,1,28,160,1,254,156,254,185,132,53,202,215,70,
    197,40,46,3,108,31,184,192,221,0,2,255,203,255,236,4,139,4,78,0,26,0,33,0,140,178,32,34,35,131,181,32,32,90,103,9,74,168,12,136,221,34,28,0,13,131,39,51,28,47,180,191,28,207,28,2,93,
    180,95,28,111,28,2,113,180,31,28,47,130,6,40,178,143,28,1,93,180,239,28,255,131,11,32,17,71,103,9,130,254,32,28,135,254,32,21,68,182,8,33,178,23,133,76,33,13,16,24,171,185,12,36,5,
    34,36,39,39,65,5,5,40,54,36,51,50,18,17,21,33,22,65,3,9,8,49,38,38,34,6,2,216,212,254,230,20,3,130,134,169,104,31,1,7,187,221,241,253,61,11,157,119,168,103,132,65,218,254,109,1,207,
    8,114,202,122,20,251,209,50,29,193,147,149,48,197,243,24,161,94,10,54,2,150,18,122,125,140,0,0,1,0,144,254,191,4,237,5,176,0,22,0,102,178,21,66,255,6,32,16,68,188,5,24,83,84,12,68,
    201,17,58,15,62,89,178,7,4,2,17,18,57,124,176,7,47,24,180,0,7,16,7,2,93,176,10,208,176,16,72,63,11,36,176,7,16,178,22,136,234,34,48,49,1,24,121,138,9,8,64,22,0,21,16,0,35,39,32,17,
    2,37,33,1,149,8,253,253,113,1,178,1,50,254,34,233,1,0,254,240,244,1,1,9,2,254,174,254,248,2,113,253,143,5,176,253,164,2,92,253,138,31,254,215,249,254,243,254,211,194,1,111,1,122,6,
    0,130,191,38,142,254,234,4,67,4,58,130,191,34,89,178,13,134,191,73,15,6,93,125,12,67,207,12,110,135,8,34,20,21,15,132,191,32,20,130,191,37,64,20,80,20,2,93,67,45,10,35,178,0,20,14,
    75,1,5,33,22,22,77,165,6,35,39,52,38,39,136,191,8,34,2,205,175,188,94,170,115,85,224,2,141,139,174,242,242,85,1,65,1,45,2,97,41,227,173,96,186,136,28,173,71,202,118,133,9,76,94,5,33,
    1,176,131,169,44,155,254,75,5,19,5,176,0,20,0,116,178,10,24,91,167,10,91,241,12,81,40,12,78,77,8,24,176,86,8,38,17,62,89,178,2,0,18,132,179,24,104,233,16,24,162,218,13,33,176,2,87,
    80,11,110,152,7,32,20,24,112,254,9,8,37,33,17,35,17,1,151,2,127,253,190,169,69,60,14,36,62,123,253,129,252,5,176,253,131,2,125,250,24,183,198,17,199,12,186,2,152,253,151,75,17,6,37,
    254,75,4,9,4,58,130,187,34,109,178,11,143,187,121,14,20,150,187,32,3,137,187,37,64,2,80,2,2,93,163,180,32,6,142,180,63,113,1,165,243,1,186,166,69,58,15,39,59,124,254,91,243,4,58,254,
    61,1,195,251,133,179,193,17,191,13,192,1,231,130,172,44,58,0,0,2,0,81,255,235,5,30,5,196,0,24,113,197,28,69,69,7,41,8,47,27,177,8,15,62,89,178,13,114,94,5,35,13,47,176,0,88,146,12,
    24,113,197,13,34,13,16,178,93,98,9,42,48,49,1,32,0,17,21,20,2,4,39,130,7,24,118,194,10,53,1,50,54,55,33,21,20,22,2,113,1,64,1,109,160,254,227,169,254,220,254,189,24,118,194,8,8,62,
    27,166,1,41,150,190,18,253,47,186,5,196,254,140,254,182,107,193,254,194,177,1,1,96,1,73,137,224,240,52,19,198,13,74,250,252,218,189,31,185,191,0,0,1,0,91,255,235,4,75,5,176,0,27,0,
    107,178,11,28,29,17,18,57,77,90,18,92,177,8,72,86,12,35,178,4,2,0,130,47,35,178,27,11,2,130,6,41,124,176,27,47,24,176,5,208,178,16,132,14,33,176,11,75,67,11,36,176,27,16,178,25,68,
    176,8,72,123,5,39,23,1,22,22,21,20,4,35,119,114,9,8,51,53,52,38,35,35,53,2,255,253,146,3,145,1,254,134,200,218,254,229,234,139,226,126,252,135,104,121,144,153,145,140,4,228,204,163,
    254,79,24,234,194,197,232,103,191,131,95,128,127,100,148,133,172,131,195,38,93,254,117,4,70,4,58,130,195,32,92,136,195,32,11,68,109,5,39,2,47,27,177,2,27,62,89,24,66,121,10,34,178,
    4,0,131,175,134,182,130,181,196,180,8,82,244,253,155,3,140,1,254,136,203,215,254,234,235,137,228,123,243,137,108,122,148,154,147,143,3,118,196,155,254,67,25,233,191,194,234,104,191,
    129,96,133,128,105,150,131,171,255,255,0,52,254,75,4,137,5,176,0,38,0,176,82,0,0,38,1,222,164,41,0,7,1,175,1,53,0,0,255,255,0,45,254,73,3,162,130,209,44,38,0,235,85,0,0,39,1,222,255,
    157,255,122,132,31,37,11,255,254,0,2,0,130,50,33,4,131,130,61,40,11,0,20,0,80,178,4,21,22,75,41,6,123,66,7,71,201,12,80,236,8,34,0,1,3,111,139,5,32,176,24,98,240,12,33,176,0,121,214,
    14,43,17,51,17,33,34,38,38,53,52,36,55,1,130,9,8,45,6,21,20,22,23,3,134,253,253,218,157,238,128,1,21,235,1,52,254,215,124,146,139,121,3,155,2,21,250,80,116,212,136,204,252,3,253,47,
    2,6,137,117,116,145,3,0,130,157,36,104,0,0,6,176,130,157,38,24,0,33,0,96,178,7,70,251,5,36,7,16,176,25,208,24,107,88,18,70,251,8,39,7,8,0,17,18,57,176,7,117,128,14,33,178,17,133,23,
    35,25,208,176,7,67,29,11,42,176,25,16,176,33,208,48,49,33,34,36,131,168,32,33,130,180,46,51,54,54,55,54,38,39,51,22,22,7,6,6,7,37,135,186,36,2,114,236,254,226,132,183,8,51,252,75,94,
    108,5,2,33,29,245,31,38,2,4,243,204,254,177,254,214,125,144,142,122,253,211,206,250,3,2,21,251,26,2,138,125,74,217,76,94,204,69,212,252,3,202,2,6,138,116,117,146,1,130,207,49,94,255,
    231,6,127,6,24,0,31,0,43,0,131,178,25,44,45,17,130,167,130,126,32,42,134,207,36,6,47,27,177,6,113,217,7,75,16,12,32,24,24,91,228,21,33,3,24,131,65,32,24,111,250,12,32,16,132,20,33,
    178,26,132,6,33,176,3,72,186,11,34,176,28,16,24,132,157,12,32,19,127,181,7,33,6,22,132,242,51,39,51,23,22,7,14,2,35,4,39,6,35,34,2,39,1,38,35,34,6,72,216,5,8,97,39,94,228,195,163,101,
    243,2,78,67,116,130,4,4,64,236,23,47,3,2,125,226,140,254,255,85,107,203,185,224,11,2,174,71,131,115,127,122,118,141,69,6,2,14,1,10,1,54,120,2,66,251,79,79,105,2,183,169,190,213,89,
    183,131,168,249,133,4,183,179,1,5,222,1,81,104,193,205,158,170,114,68,0,1,0,60,255,231,5,227,5,176,0,41,0,99,178,35,42,24,82,215,9,24,78,255,12,113,255,8,34,1,42,9,79,95,6,32,0,71,
    174,8,33,176,9,24,99,187,14,32,1,131,34,34,34,16,178,72,188,10,33,26,34,131,55,35,48,49,19,53,104,219,5,42,33,53,33,22,4,21,20,7,22,19,21,136,245,32,22,132,245,59,6,38,39,53,52,38,
    35,230,167,147,132,254,243,254,165,1,100,250,1,6,255,246,5,1,60,51,101,114,130,243,8,49,245,26,43,2,2,122,218,138,167,178,8,124,103,2,98,205,1,109,117,209,205,1,211,204,230,100,63,
    254,254,77,57,73,2,182,163,190,213,98,202,103,169,248,133,4,167,170,62,110,126,0,130,227,45,47,255,226,4,254,4,58,0,36,0,96,178,15,37,24,144,223,9,36,29,47,27,177,29,70,153,7,76,86,
    8,24,66,182,9,35,178,7,14,29,130,203,34,178,22,37,131,6,36,176,22,47,178,20,68,245,8,33,176,29,24,91,98,12,34,34,20,22,130,41,35,48,49,37,6,65,195,6,130,205,34,6,6,35,132,205,38,35,
    35,39,51,54,53,52,130,6,8,69,33,22,22,16,7,22,23,3,1,2,78,90,96,3,4,65,236,45,24,1,4,233,188,158,160,8,162,230,2,194,185,203,255,6,1,20,203,228,176,185,6,235,88,2,143,127,150,169,134,
    128,57,204,242,3,113,131,72,127,189,4,131,150,195,2,166,254,202,74,48,172,130,209,46,72,254,186,4,55,5,176,0,34,0,95,178,11,35,36,130,122,34,0,176,23,69,35,5,65,184,12,32,27,130,225,
    39,27,15,62,89,178,1,9,27,65,184,38,33,27,16,75,139,10,34,48,49,19,130,187,130,188,34,33,33,39,65,177,8,41,51,21,20,6,7,39,54,54,55,35,65,171,5,8,59,151,1,206,145,129,254,235,254,234,
    3,1,46,239,1,3,228,227,3,205,100,90,131,36,56,8,163,60,3,126,116,2,92,195,1,115,111,235,195,3,220,201,223,102,71,254,246,134,172,99,216,75,77,57,119,73,49,177,132,113,133,130,203,38,
    116,254,169,4,26,4,58,132,203,32,6,134,203,32,24,138,203,65,160,7,24,77,234,8,34,1,9,28,135,203,65,142,9,66,132,13,34,16,0,1,131,34,32,28,70,181,11,132,203,36,50,53,52,38,35,130,203,
    40,50,23,22,21,20,7,22,23,21,141,204,8,56,35,179,1,225,210,107,99,254,225,4,1,32,227,120,106,173,177,2,187,104,85,131,38,56,6,166,43,1,195,1,155,179,142,74,83,193,100,89,146,158,79,
    60,195,36,172,101,218,71,77,61,126,79,30,131,84,166,0,130,199,38,66,255,235,7,127,5,176,130,199,34,98,178,0,134,199,24,90,52,8,72,65,7,40,31,47,27,177,31,15,62,89,176,24,69,249,12,
    32,13,79,223,12,104,124,12,33,176,31,65,143,11,35,178,23,31,13,130,216,48,48,49,1,33,3,2,2,6,7,35,53,55,54,54,19,19,33,101,89,5,35,55,54,39,51,67,69,5,8,70,34,38,53,4,7,254,97,24,14,
    97,185,156,74,40,122,104,15,28,3,142,76,63,110,127,4,4,65,246,28,41,2,2,127,224,140,195,198,4,227,253,224,254,246,254,211,138,2,202,3,9,223,1,28,2,223,251,188,82,100,180,167,187,216,
    102,199,102,167,251,132,193,189,130,213,42,64,255,235,6,90,4,58,0,33,0,98,76,247,6,69,245,5,78,238,12,32,30,130,213,32,30,135,213,32,5,126,1,6,33,176,12,140,213,32,5,24,111,43,12,32,
    30,75,254,11,35,178,22,30,12,136,213,130,212,32,39,130,212,32,55,130,212,36,22,22,51,50,54,131,212,69,16,5,8,69,34,38,39,3,23,254,247,19,17,168,173,83,2,50,80,73,10,20,2,225,1,81,69,
    88,103,4,4,64,236,22,48,3,2,112,199,125,194,199,1,3,116,254,154,254,233,244,3,202,5,11,173,229,1,206,253,43,82,100,160,153,181,200,80,177,124,155,230,124,190,185,130,211,45,148,255,
    231,7,134,5,176,0,29,0,101,178,20,30,125,59,9,75,101,12,32,25,130,198,32,25,110,51,12,135,224,32,17,130,25,32,17,130,12,33,178,4,69,3,8,35,178,9,0,23,130,193,33,178,28,132,6,33,176,
    28,24,131,71,11,36,48,49,1,17,20,69,218,13,36,6,38,39,53,33,112,48,5,8,49,17,5,10,77,62,112,126,4,4,65,246,23,47,3,2,124,226,142,187,195,9,253,130,252,252,2,126,5,176,251,188,86,96,
    2,179,166,187,216,89,183,131,168,247,135,4,192,195,255,253,151,75,127,5,32,0,130,199,46,119,255,227,6,92,4,58,0,28,0,120,178,27,29,30,91,197,13,24,75,15,12,135,12,32,2,130,186,32,2,
    135,199,32,26,130,12,32,26,130,12,34,178,7,8,77,161,9,41,208,7,224,7,2,93,180,64,7,80,130,6,73,103,10,33,176,26,72,86,11,33,178,18,132,50,130,218,135,198,33,51,17,69,200,14,8,57,4,
    3,3,26,254,80,243,243,1,176,243,2,82,70,94,100,3,4,64,235,26,43,2,2,112,199,126,254,138,19,1,186,254,70,4,58,254,67,1,189,253,45,82,102,2,166,145,175,206,93,191,97,155,230,124,8,1,
    132,130,217,46,93,255,235,4,187,5,197,0,33,0,71,178,0,34,35,24,167,245,13,80,211,16,33,9,16,77,145,10,24,106,47,13,33,178,26,88,161,6,40,5,34,36,2,39,17,52,18,36,107,62,7,32,21,65,
    148,14,8,63,2,187,172,254,235,155,2,154,1,23,173,223,136,63,134,162,157,197,196,158,125,131,3,3,53,245,39,19,1,2,129,234,21,156,1,24,173,1,15,175,1,29,158,89,184,68,231,188,255,0,182,
    233,2,133,116,149,204,177,88,88,139,205,110,0,130,181,46,85,255,235,3,231,4,78,0,30,0,68,178,19,31,32,136,181,24,66,16,12,105,149,8,71,65,9,35,178,5,11,19,130,44,33,176,19,126,128,
    13,45,37,54,54,55,52,39,51,22,7,6,6,35,34,0,100,47,6,107,251,5,8,54,21,21,20,22,2,90,81,69,2,19,235,29,2,4,210,181,231,254,226,124,226,146,187,96,46,99,138,114,139,148,175,2,67,71,
    119,103,140,82,160,176,1,49,248,30,151,250,139,66,189,58,189,164,32,154,191,130,163,44,33,255,231,5,90,5,176,0,25,0,77,178,5,24,107,153,7,34,69,88,176,118,204,12,24,111,251,8,76,33,
    12,39,176,4,208,176,5,208,176,22,24,135,152,12,34,14,22,2,67,186,6,35,53,33,21,33,65,83,9,68,135,5,42,6,38,39,1,227,254,62,4,128,254,62,66,229,6,8,32,245,27,43,3,2,125,226,140,187,
    195,9,4,227,205,205,252,135,84,96,2,182,163,187,216,98,202,103,168,249,133,4,192,195,130,163,40,68,255,227,4,203,4,58,0,23,131,163,33,24,25,76,197,13,103,29,12,35,15,62,89,176,147,
    163,32,21,141,163,32,21,152,163,8,51,6,6,35,4,3,1,137,254,187,3,139,254,173,82,69,94,99,3,4,64,235,44,25,1,4,241,194,254,137,19,3,119,195,195,253,240,84,100,2,132,116,147,158,124,126,
    55,204,242,8,1,132,0,130,161,46,129,255,235,4,255,5,197,0,40,0,115,178,38,41,42,136,161,34,22,47,27,24,161,154,9,65,233,8,32,3,68,39,9,34,36,22,11,130,44,43,124,176,36,47,24,178,115,
    36,1,93,178,96,131,4,87,214,9,35,178,6,3,37,130,32,35,178,16,37,36,83,64,5,24,73,247,10,35,178,27,36,30,65,107,5,53,20,22,51,50,54,53,51,20,6,4,35,32,36,53,52,37,38,38,53,52,36,33,
    102,75,10,8,88,20,33,51,21,35,34,6,1,127,183,153,134,174,252,141,254,253,160,254,243,254,191,1,14,118,130,1,47,1,9,151,250,139,253,163,124,144,170,1,51,182,191,157,163,1,152,101,126,
    129,94,130,190,105,233,196,253,87,49,166,98,197,219,105,186,119,89,117,115,99,217,200,112,0,0,2,0,103,4,111,2,214,5,215,0,5,0,13,0,27,101,197,9,39,176,1,208,176,1,47,176,11,101,192,
    6,8,59,48,49,1,19,51,21,3,35,1,51,21,22,23,7,38,53,1,147,112,211,230,93,254,212,177,3,76,80,176,4,152,1,63,21,254,193,1,84,95,123,70,72,90,190,0,255,255,0,71,2,9,2,84,2,205,0,6,0,17,
    0,147,15,47,157,2,109,4,153,3,49,0,70,1,151,224,0,76,205,64,131,35,36,129,2,109,5,209,133,19,35,133,0,102,102,132,19,8,33,4,254,63,3,153,0,0,0,39,0,67,0,1,254,254,1,6,0,67,1,0,0,28,
    0,182,0,2,16,2,32,2,3,93,180,131,6,39,2,113,182,128,2,144,2,160,130,15,51,48,49,0,1,0,99,4,32,1,150,6,26,0,8,0,29,178,8,9,10,65,191,8,39,0,47,27,177,0,33,62,89,102,148,5,130,211,57,
    23,6,7,21,35,53,54,54,1,26,124,91,3,213,1,103,6,26,77,133,144,152,138,96,209,0,130,69,38,51,4,0,1,101,6,0,144,69,32,4,130,69,32,4,131,69,61,0,208,176,0,47,48,49,19,39,54,55,53,51,21,
    20,6,175,124,90,3,213,105,4,0,77,131,146,158,138,103,132,67,38,50,254,214,1,100,0,202,130,67,32,24,136,137,36,9,47,178,4,13,103,96,9,134,62,50,6,6,173,123,85,3,218,1,102,254,214,78,
    127,148,147,133,93,208,0,130,131,32,74,130,131,32,124,132,131,32,22,77,142,10,139,194,8,56,21,22,23,7,38,38,53,53,1,31,3,90,124,77,105,6,0,158,143,134,77,62,209,103,138,255,255,0,108,
    4,32,2,239,6,26,0,38,1,108,9,0,0,7,1,108,1,89,0,0,255,255,0,64,4,0,2,192,130,85,35,38,1,109,13,131,23,8,36,109,1,91,0,0,0,2,0,50,254,194,2,170,0,255,0,9,0,18,0,33,178,11,19,20,17,18,
    57,176,11,16,176,5,208,0,176,19,139,181,34,176,14,208,136,247,35,6,7,6,23,65,1,7,33,177,127,131,194,57,55,49,248,127,88,4,218,102,254,194,78,137,157,201,186,108,114,100,65,78,142,150,
    203,182,99,221,130,209,39,64,0,0,4,30,5,176,0,24,122,145,21,94,206,34,32,10,72,119,12,33,4,208,130,157,24,190,174,9,58,17,51,17,33,4,30,254,136,243,254,141,1,115,243,1,120,3,114,252,
    142,3,114,200,1,118,254,138,130,123,36,92,254,96,4,57,130,123,32,19,123,227,7,94,134,12,95,61,12,24,74,23,12,37,2,47,27,177,2,17,73,214,6,87,107,12,24,79,73,8,32,6,68,4,8,34,176,14,
    16,97,70,10,42,176,9,208,176,16,208,176,17,208,176,6,24,99,101,8,39,33,33,17,35,17,33,53,33,132,3,130,176,32,21,130,9,33,4,57,131,180,35,142,1,114,254,130,3,130,184,43,254,136,1,120,
    254,96,1,160,194,2,180,196,131,189,35,196,253,76,0,130,193,57,136,2,6,2,68,3,219,0,13,0,22,178,3,14,15,17,18,57,0,176,3,47,177,10,10,43,24,155,173,9,33,51,50,24,154,124,7,8,45,39,136,
    121,100,103,120,119,103,99,121,2,3,3,95,121,121,98,37,94,119,115,93,0,255,255,0,138,255,245,3,111,1,0,0,38,0,18,3,0,0,7,0,18,1,205,0,134,23,33,5,40,136,23,32,39,133,23,131,31,34,3,
    134,0,131,127,46,71,2,9,1,33,2,205,0,3,0,24,178,0,4,5,134,127,72,72,10,8,41,48,49,1,35,53,51,1,33,218,218,2,9,196,0,0,6,0,74,255,236,7,95,5,196,0,21,0,35,0,39,0,52,0,65,0,78,0,184,
    178,40,79,80,130,59,37,176,40,16,176,2,208,131,5,32,27,132,5,32,38,132,5,32,53,132,5,37,71,208,0,176,36,47,103,216,7,73,174,12,84,88,8,33,3,208,130,252,35,178,5,3,18,131,75,49,7,208,
    176,7,47,176,18,16,176,14,208,176,14,47,178,16,18,3,79,249,6,35,32,208,176,32,131,24,34,178,43,2,67,117,7,36,176,3,16,178,50,137,13,41,43,16,176,56,208,176,50,16,176,63,130,42,34,16,
    178,69,137,25,35,25,16,178,76,136,13,130,219,32,52,24,180,50,7,32,21,111,139,9,33,53,1,105,154,5,132,17,38,38,53,1,39,1,23,3,69,194,5,38,53,52,38,34,6,21,5,139,12,32,1,139,12,8,43,
    3,47,172,136,150,78,78,149,134,175,169,138,151,78,78,148,138,172,253,27,168,133,138,171,171,136,133,170,1,119,125,2,199,125,176,79,62,64,74,78,124,77,1,199,134,8,8,52,251,78,77,63,
    62,76,77,126,75,1,101,130,170,111,111,167,140,71,129,170,110,110,170,134,3,123,131,170,170,137,70,130,169,169,137,252,27,72,4,114,72,252,56,68,87,82,76,75,70,84,84,74,74,136,9,8,33,
    2,234,69,85,85,73,72,70,86,87,73,0,0,1,0,108,0,138,2,51,3,169,0,6,0,16,0,176,5,47,178,2,7,5,93,131,5,130,233,55,19,35,1,53,1,51,1,60,247,167,254,224,1,32,167,2,25,254,113,1,134,19,
    1,134,131,55,32,84,130,55,32,27,135,55,35,0,47,178,3,123,55,5,61,3,47,48,49,19,1,21,1,35,19,3,251,1,32,254,224,167,247,247,3,169,254,122,19,254,122,1,143,1,144,130,111,48,45,0,109,
    3,113,5,39,0,3,0,9,0,176,0,47,176,2,130,48,36,55,39,1,23,170,131,234,32,109,131,181,52,0,255,255,0,53,2,147,2,190,5,168,3,7,1,216,0,0,2,147,0,19,68,249,5,78,249,8,35,13,208,48,49,131,
    133,44,105,2,140,2,255,5,186,0,15,0,83,178,10,118,7,10,75,231,12,87,77,12,37,13,47,27,177,13,19,67,199,6,32,7,130,12,32,7,130,12,35,178,1,3,13,131,245,35,3,16,178,10,94,90,8,46,48,
    49,1,23,54,51,32,17,17,35,17,38,35,34,7,130,6,60,1,1,32,75,144,1,3,197,5,125,99,39,197,5,172,121,135,254,201,254,9,1,218,173,89,253,210,3,32,130,219,46,95,0,0,4,124,5,195,0,39,0,142,
    178,31,40,41,70,115,8,32,23,130,102,32,23,74,211,7,32,6,130,12,39,6,15,62,89,178,39,6,23,130,33,36,176,39,47,178,13,66,94,8,37,176,1,208,176,6,16,24,96,122,10,39,176,9,208,176,39,16,
    176,16,132,5,47,35,208,176,35,47,182,15,35,31,35,47,35,3,93,178,37,137,54,32,17,130,25,38,16,176,20,208,176,23,16,72,78,12,33,35,30,72,78,5,39,33,23,20,7,33,7,33,53,24,171,121,8,34,
    35,53,51,24,171,125,15,8,55,33,23,33,3,50,254,208,2,64,2,184,1,251,231,82,39,43,2,165,160,4,156,151,5,250,1,150,232,245,105,95,88,103,6,1,63,254,198,5,1,53,1,212,46,135,85,202,202,
    9,111,91,55,145,121,144,161,24,171,140,7,60,161,144,121,0,5,0,33,0,0,6,79,5,176,0,27,0,31,0,35,0,38,0,41,0,189,178,10,42,43,130,134,37,176,10,16,176,31,208,131,5,32,33,132,5,32,38,
    132,5,33,40,208,24,125,159,18,65,48,12,36,12,47,27,177,12,76,209,7,32,9,130,12,32,9,130,12,35,178,5,9,26,94,26,6,43,1,208,176,1,47,178,15,1,1,93,178,3,65,188,8,36,176,5,16,178,7,137,
    13,32,37,130,107,35,208,176,14,208,130,22,35,176,29,208,176,130,128,44,17,208,176,3,16,176,30,208,176,34,208,176,18,130,65,60,16,176,25,208,176,39,208,176,21,208,176,9,16,176,36,208,
    176,23,16,176,41,208,48,49,1,51,21,35,21,130,3,42,17,35,1,33,17,35,17,35,53,51,53,130,3,33,17,51,130,13,33,51,1,130,11,44,5,51,39,35,1,53,35,1,51,39,5,119,216,130,0,38,253,254,201,
    254,173,252,211,130,0,62,252,1,53,1,87,251,254,113,148,243,254,103,238,95,143,2,140,47,253,163,43,43,3,197,160,151,160,254,18,1,238,131,3,130,10,8,34,1,235,254,21,1,235,252,222,151,
    151,151,254,126,75,1,215,68,0,2,0,152,255,236,6,58,5,176,0,30,0,37,0,162,178,33,118,243,5,33,33,16,106,153,8,36,21,47,27,177,21,66,79,7,32,25,130,12,32,25,24,142,171,20,103,65,12,32,
    19,130,38,36,19,15,62,89,176,24,142,184,13,24,115,64,21,42,178,32,19,21,17,18,57,176,32,47,178,115,126,10,41,29,16,176,28,208,176,28,47,176,21,120,25,11,34,48,49,1,24,118,19,12,45,
    35,6,6,7,35,17,35,17,33,50,22,23,51,17,130,1,8,92,1,51,50,17,52,39,35,6,51,191,50,63,38,47,83,77,254,232,120,28,244,202,158,250,1,140,212,253,24,117,242,191,251,95,146,244,230,160,
    3,134,253,164,61,56,10,188,23,1,53,2,101,173,187,3,253,229,5,176,195,179,1,7,254,249,254,173,1,0,247,6,0,255,255,0,148,255,236,8,60,5,176,0,38,0,54,0,0,0,7,0,87,4,114,132,7,32,53,130,
    12,32,83,130,23,50,31,0,35,0,39,0,43,0,46,0,49,0,52,0,235,178,50,53,54,131,197,37,50,16,176,30,208,176,130,5,32,34,132,5,32,39,132,5,32,42,132,5,32,46,132,5,32,48,24,108,35,19,36,31,
    47,27,177,31,65,100,7,32,27,130,12,32,27,135,12,120,101,8,81,251,8,38,15,62,89,178,9,16,2,131,108,32,9,112,156,6,39,178,15,5,1,93,176,1,208,66,136,14,32,9,68,97,11,8,33,176,45,208,
    176,14,208,176,48,208,176,18,208,176,9,16,176,37,208,176,41,208,176,33,208,176,21,208,176,7,16,176,38,208,176,130,165,130,180,56,22,208,176,1,16,176,29,208,176,25,208,176,16,16,176,
    47,208,176,44,208,176,31,16,176,50,132,23,50,52,208,48,49,1,33,19,51,3,51,21,35,7,51,21,33,3,35,3,132,3,36,53,51,39,35,53,130,21,32,19,130,27,36,1,51,55,35,5,132,3,34,39,35,1,130,10,
    8,78,55,35,1,7,51,4,152,1,49,87,251,98,154,191,37,228,254,247,126,243,144,254,242,146,242,127,254,253,222,37,185,148,98,251,88,1,52,108,212,253,206,159,42,234,3,14,159,33,233,254,166,
    186,42,101,1,176,38,86,253,50,47,85,1,167,8,16,4,7,1,169,254,87,160,162,160,253,219,2,37,131,3,130,10,131,17,36,1,169,253,21,162,131,0,8,32,254,0,190,185,185,2,1,31,0,2,0,124,0,0,6,
    16,4,58,0,13,0,27,0,107,178,8,28,29,17,18,57,176,8,66,207,9,24,77,237,12,36,22,47,27,177,22,66,207,7,90,200,8,35,0,69,88,176,85,191,8,66,147,10,32,0,78,6,12,34,5,17,9,130,90,35,178,
    16,9,17,68,246,5,41,50,22,23,17,35,17,52,38,35,33,130,6,39,1,17,51,17,33,50,54,55,130,6,8,69,6,6,35,3,12,187,174,2,243,90,105,254,174,243,1,153,243,1,80,106,89,1,244,1,239,220,4,58,
    192,203,254,181,1,66,109,99,252,138,4,58,251,198,2,214,253,237,97,104,2,174,253,87,188,213,0,1,0,94,255,237,4,48,5,195,0,35,0,138,178,21,24,72,91,10,78,21,12,68,163,8,33,35,22,131,
    142,36,176,35,47,178,0,69,170,9,32,9,99,106,13,40,16,176,12,208,176,35,16,176,14,132,5,46,19,208,176,19,47,182,15,19,31,19,47,19,3,93,178,24,173,28,10,32,22,86,140,11,46,176,19,16,
    176,30,208,176,16,16,176,32,208,48,49,1,96,16,7,35,35,32,0,3,68,160,6,33,54,0,124,68,8,33,33,21,130,1,8,85,3,106,254,156,6,163,152,110,95,28,120,128,255,0,254,218,8,172,172,172,173,
    13,1,44,253,106,133,28,102,101,151,162,9,1,99,254,156,1,100,2,15,174,172,33,204,29,1,32,1,2,141,128,141,255,1,27,31,205,34,172,164,141,128,0,0,4,0,33,0,0,5,212,5,176,0,26,0,31,0,36,
    0,41,0,227,178,12,69,215,5,37,12,16,176,28,208,176,130,5,32,35,132,5,69,209,7,99,210,12,41,1,47,27,177,1,15,62,89,176,11,68,59,11,130,190,56,176,32,47,64,19,0,32,16,32,32,32,48,32,
    64,32,80,32,96,32,112,32,128,32,9,93,131,224,58,30,47,182,176,30,192,30,208,30,3,93,64,11,0,30,16,30,32,30,48,30,64,30,5,93,178,38,69,219,9,53,39,208,176,39,47,64,15,48,39,64,39,80,
    39,96,39,112,39,128,39,144,39,7,24,94,187,12,50,38,16,176,3,208,176,30,16,176,6,208,176,32,16,176,15,208,178,18,137,63,130,187,35,29,208,176,7,132,25,32,10,132,37,34,20,208,176,130,
    49,36,23,208,48,49,1,69,242,10,40,33,50,4,23,51,21,35,23,7,130,4,45,6,6,35,1,39,33,21,33,37,33,38,39,33,1,130,8,36,50,1,214,253,184,130,0,8,83,2,45,173,1,1,60,228,189,2,1,188,225,54,
    250,189,1,21,3,253,190,2,67,253,189,1,240,70,114,254,200,1,244,254,12,1,49,123,2,29,253,227,3,31,160,72,160,1,9,136,129,160,38,34,160,125,133,1,194,40,72,232,59,2,254,59,55,0,1,0,40,
    0,0,4,12,5,176,0,26,0,109,178,22,27,28,72,71,8,75,1,12,71,22,8,32,25,82,83,11,38,176,1,208,176,25,16,176,130,193,33,20,47,131,243,34,20,16,178,127,163,10,130,254,33,20,16,75,28,6,32,
    9,87,89,8,34,178,13,9,96,225,6,54,35,22,23,51,7,35,6,6,7,1,21,33,1,39,51,50,54,55,33,55,33,38,35,130,4,8,53,3,217,218,51,15,202,50,151,22,220,201,1,210,254,225,254,3,1,253,112,131,
    22,253,230,51,1,227,49,216,254,243,54,3,174,4,249,75,101,182,165,175,17,253,223,13,2,81,153,93,76,182,155,204,0,130,203,36,33,255,236,4,81,130,203,36,30,0,145,178,27,83,49,10,36,17,
    47,27,177,17,24,76,151,12,38,15,62,89,178,19,17,5,130,237,47,176,19,47,176,23,208,176,23,47,178,0,23,1,93,178,24,77,87,9,38,25,208,176,8,208,176,9,130,27,44,16,176,22,208,176,11,208,
    176,10,208,176,19,16,24,222,80,10,39,176,21,208,176,12,208,176,13,131,22,37,176,18,208,176,15,208,131,239,33,5,16,95,167,10,34,178,30,5,68,31,6,43,21,6,2,4,35,34,39,17,7,53,55,53,130,
    3,37,17,51,21,55,21,7,131,3,46,17,54,54,53,53,4,81,2,150,254,237,178,107,140,220,130,0,33,252,225,130,0,8,34,170,178,2,255,89,210,254,195,171,20,2,93,87,199,87,137,87,200,87,1,59,215,
    90,200,90,137,90,200,89,253,251,2,252,248,77,131,243,41,79,0,0,5,15,4,58,0,23,0,24,135,55,13,106,222,12,70,69,12,68,231,12,87,94,7,35,178,21,11,23,24,74,52,12,32,12,65,8,9,39,9,208,
    48,49,1,22,0,19,102,240,6,36,35,17,6,6,21,130,11,8,54,18,0,55,53,51,3,40,224,1,3,4,243,1,129,114,243,113,130,243,3,1,4,223,243,3,106,41,254,146,254,236,191,184,197,239,42,253,106,2,
    149,42,243,199,177,186,1,20,1,112,43,209,0,2,0,40,130,175,44,51,5,176,0,22,0,31,0,120,178,24,32,33,131,117,32,24,24,73,71,9,79,34,12,32,2,86,96,6,35,178,6,2,12,131,39,33,6,47,74,174,
    11,47,1,208,176,6,16,176,10,208,176,10,47,178,15,10,1,93,24,73,73,11,32,20,132,27,32,21,130,27,40,16,176,23,208,176,12,16,178,31,136,208,35,48,49,37,33,130,191,68,189,6,45,17,33,50,
    4,21,20,4,7,33,21,33,1,33,50,115,236,5,37,3,51,254,190,252,205,130,0,8,66,2,45,241,1,32,254,238,244,254,196,1,66,254,190,1,45,136,144,141,124,254,196,231,231,231,203,107,203,2,200,
    251,208,212,241,3,107,1,54,126,125,112,142,3,0,4,0,112,255,236,5,137,5,197,0,25,0,38,0,52,0,56,0,148,178,26,57,58,131,183,37,26,16,176,0,208,176,130,5,32,39,132,5,39,55,208,0,176,53,
    47,176,55,91,98,18,32,36,130,241,39,36,15,62,89,176,9,16,176,78,110,5,34,13,9,3,131,66,33,9,16,69,161,11,35,3,16,178,22,69,230,8,35,178,25,3,9,131,34,44,36,16,176,29,208,176,29,47,
    176,36,16,178,42,136,29,36,176,29,16,178,49,136,13,24,178,199,24,130,249,78,87,9,35,32,38,53,23,78,56,8,8,35,35,34,6,21,5,39,1,23,2,177,159,255,0,162,158,130,128,161,170,65,54,52,66,
    67,106,64,1,24,174,135,136,173,167,254,232,171,24,232,210,8,61,253,251,126,2,199,126,4,37,115,146,167,138,71,130,171,148,115,53,64,84,74,74,69,85,67,49,253,64,134,166,24,232,209,22,
    50,2,0,76,255,235,3,144,5,249,0,23,0,33,0,90,178,1,34,35,131,207,36,1,16,176,24,208,24,92,138,13,38,15,62,89,178,6,12,0,66,11,7,68,86,8,33,176,19,103,223,15,32,6,131,60,65,247,15,58,
    5,34,38,53,6,35,53,50,55,17,54,54,51,50,22,21,21,20,2,7,21,20,22,51,3,54,54,107,159,5,8,78,7,2,219,225,237,97,96,97,96,3,178,154,136,172,215,178,104,108,212,77,87,43,32,86,3,21,235,
    229,19,187,24,1,233,191,214,180,155,38,173,254,169,103,77,142,122,2,68,75,204,102,41,63,64,178,0,0,4,0,144,0,0,7,194,5,192,0,3,0,15,0,29,0,39,0,166,178,30,40,41,131,167,37,30,16,176,
    1,208,176,130,5,104,136,5,32,16,130,209,40,0,69,88,176,38,47,27,177,38,68,147,7,32,36,24,81,56,11,32,6,24,140,182,11,32,33,130,12,32,33,76,144,7,91,238,8,130,220,49,13,208,176,13,47,
    176,2,208,176,2,47,178,0,2,1,93,178,1,65,227,8,36,176,13,16,178,19,137,13,35,6,16,178,26,136,13,35,178,32,36,33,130,149,35,178,37,31,38,87,104,8,40,1,52,54,32,22,21,21,20,6,65,246,
    16,37,1,33,1,17,35,17,130,4,8,90,51,7,151,253,159,2,97,253,118,190,1,56,191,186,254,194,189,175,92,81,79,91,92,80,79,92,254,199,254,244,254,13,244,1,11,1,246,242,1,156,149,2,47,159,
    193,192,166,78,156,194,194,162,6,96,108,108,99,81,95,109,109,98,251,163,4,10,251,246,5,176,251,243,4,13,0,0,2,0,109,3,148,4,87,5,176,0,12,0,20,0,109,77,146,5,140,244,79,122,8,24,121,
    196,8,38,31,62,89,178,1,21,6,130,178,33,176,1,130,239,33,9,1,130,9,34,178,3,1,132,16,37,4,208,178,8,1,9,132,26,41,16,176,11,208,176,6,16,177,13,10,82,159,5,52,176,1,16,176,15,208,176,
    13,16,176,17,208,176,18,208,48,49,1,3,35,3,130,209,39,51,19,19,51,17,35,1,35,130,10,8,43,35,53,33,3,232,124,62,124,111,137,129,133,133,111,254,17,138,117,141,1,140,5,9,254,139,1,116,
    254,140,2,28,254,131,1,125,253,228,1,189,254,69,1,187,95,131,187,44,150,255,236,4,145,4,78,0,21,0,28,0,98,24,104,215,10,32,22,65,215,6,83,213,12,68,191,8,34,25,10,2,131,160,36,25,47,
    178,15,10,82,20,8,36,2,16,178,19,12,135,13,33,178,21,133,34,35,10,16,178,22,136,34,8,101,48,49,37,6,35,34,38,2,53,52,18,54,51,50,22,22,23,21,33,17,22,51,50,55,1,34,7,17,33,17,38,4,
    20,183,187,145,244,135,144,248,132,133,227,132,3,253,0,119,154,196,172,254,144,151,122,2,28,115,94,114,157,1,1,147,143,1,3,159,139,243,144,62,254,184,110,122,3,42,122,254,235,1,30,
    113,255,255,0,89,255,245,5,203,5,153,0,39,1,213,255,217,2,134,130,7,46,124,0,251,0,0,1,7,1,220,3,33,0,0,0,16,65,147,13,33,48,49,130,51,38,84,255,245,6,104,5,180,130,43,36,215,0,29,
    2,148,130,7,34,124,1,168,134,51,32,190,137,51,39,13,47,27,177,13,31,62,89,132,51,32,91,130,51,34,92,5,168,130,43,36,217,0,12,2,147,132,51,32,140,134,51,32,178,137,51,32,1,130,51,32,
    1,135,51,32,88,130,51,34,26,5,163,130,43,36,219,0,34,2,142,132,51,32,51,134,51,32,112,24,174,109,19,60,0,2,0,98,255,235,4,67,5,245,0,25,0,38,0,91,178,19,39,40,17,18,57,176,19,16,176,
    32,208,24,112,142,8,78,146,7,33,178,0,91,28,5,35,0,47,178,2,133,9,32,11,81,2,12,32,0,100,70,12,33,19,16,107,30,12,39,1,50,23,38,38,35,34,7,24,158,41,9,91,74,6,33,18,23,99,84,6,8,89,
    54,53,53,38,38,2,56,174,119,26,197,132,124,139,29,60,110,143,1,13,1,39,122,227,148,227,254,243,254,244,123,133,132,122,121,133,22,139,4,4,125,194,229,53,183,25,44,254,78,254,114,53,
    193,254,211,167,1,36,247,13,223,1,18,194,167,164,154,176,208,197,85,76,95,0,1,0,166,255,27,4,244,5,176,0,7,0,39,0,176,4,70,43,5,67,38,8,37,4,16,176,1,208,176,24,100,176,14,36,5,35,
    17,33,17,130,3,46,4,244,244,253,153,243,4,78,229,5,212,250,44,6,149,130,75,36,64,254,243,4,193,130,75,37,12,0,53,0,176,3,133,75,32,8,24,83,166,7,34,3,16,178,98,165,9,32,176,24,144,
    184,34,62,143,253,238,3,68,251,127,2,79,253,177,4,71,252,246,2,18,2,67,253,115,195,151,2,200,2,198,152,195,253,115,130,111,39,158,2,109,3,239,3,49,0,24,235,139,23,40,3,239,252,175,
    3,81,2,109,196,130,43,36,59,0,0,4,146,130,155,36,8,0,60,178,0,89,61,6,107,79,6,66,25,7,32,176,24,198,213,8,35,15,62,89,178,102,170,6,32,7,112,54,11,63,48,49,1,1,51,1,35,3,35,53,33,
    2,65,1,120,217,254,23,197,216,209,1,103,1,43,4,133,250,80,2,65,197,130,137,59,94,255,236,7,223,4,78,0,26,0,42,0,57,0,114,178,7,58,59,17,18,57,176,7,16,176,34,208,131,5,32,50,67,193,
    6,32,4,24,88,232,11,32,9,82,3,6,44,176,4,16,176,22,208,176,22,47,178,7,22,4,131,54,38,18,208,176,18,47,178,20,133,12,32,22,83,9,11,130,42,32,178,24,128,185,9,40,176,46,208,176,30,16,
    176,55,208,130,161,8,41,20,6,6,35,34,38,39,2,33,34,38,38,53,53,52,18,54,51,32,19,18,33,50,22,22,23,7,52,38,35,34,7,6,7,21,22,23,22,51,50,54,53,85,195,5,36,55,55,53,38,39,130,24,8,120,
    6,7,223,128,230,144,141,233,85,170,254,223,143,229,129,129,228,142,1,36,169,169,1,36,142,228,129,1,239,146,122,164,110,40,15,15,46,107,159,121,149,250,93,146,123,105,172,43,7,15,40,
    110,164,121,146,2,17,152,253,144,163,167,254,182,142,255,153,21,152,1,0,143,254,185,1,71,143,253,151,4,154,198,201,74,66,36,69,85,195,195,162,5,157,195,179,144,26,36,66,74,201,195,
    0,0,1,255,175,254,75,2,168,6,21,0,21,0,61,178,2,22,23,76,13,8,36,14,47,27,177,14,102,197,12,34,17,62,89,88,118,11,32,14,99,27,13,32,5,107,245,8,35,55,17,52,54,78,30,6,8,45,21,1,144,
    182,170,66,63,18,44,37,138,2,192,178,63,89,25,42,50,163,79,176,182,19,189,13,157,4,244,179,195,21,185,11,184,0,0,2,0,101,1,1,4,21,3,250,130,131,36,43,0,120,178,16,103,81,5,40,16,16,
    176,28,208,0,176,25,47,73,21,5,36,176,8,208,176,8,130,11,34,16,176,10,130,8,96,75,11,33,176,3,98,225,11,40,176,13,16,176,21,208,176,25,16,77,225,5,131,8,35,32,208,176,30,24,102,241,
    12,32,25,103,64,11,62,176,35,16,176,43,208,48,49,19,54,54,51,54,23,23,22,51,50,55,21,6,35,34,39,39,38,7,34,6,7,21,148,21,8,58,101,48,132,66,82,76,156,70,81,132,101,102,127,81,70,152,
    79,84,66,135,48,48,128,66,84,79,152,70,81,135,101,102,131,81,70,156,76,82,66,132,48,3,142,50,56,2,34,78,32,126,217,106,32,76,36,2,66,60,203,130,15,33,36,76,132,15,33,78,34,130,15,48,
    0,1,0,145,0,128,3,239,4,195,0,19,0,55,0,176,19,126,167,12,38,4,208,176,19,16,176,7,132,5,36,15,208,176,15,47,123,219,10,33,176,8,130,16,34,16,176,11,130,186,41,1,33,7,39,55,35,53,33,
    55,33,130,3,8,73,23,7,51,21,33,7,33,3,239,253,226,128,109,93,176,1,33,126,254,97,2,16,134,110,99,189,254,209,125,1,172,1,100,228,62,166,201,223,202,237,62,175,202,223,255,255,0,60,
    0,19,3,141,4,107,0,103,0,32,0,0,0,139,64,0,57,154,0,7,1,151,255,158,253,166,130,29,32,128,130,29,32,224,132,29,32,34,140,29,59,226,253,166,0,2,0,36,0,0,3,235,5,176,0,5,0,9,0,56,178,
    6,10,11,17,18,57,176,6,24,97,229,14,72,241,7,106,119,8,34,6,0,3,130,39,33,178,8,132,6,8,43,48,49,1,51,1,1,35,1,1,3,19,19,1,164,196,1,131,254,128,197,254,126,1,225,237,242,236,5,176,
    253,39,253,41,2,215,1,214,254,42,254,41,1,215,0,130,143,48,161,0,171,1,188,5,7,0,39,0,18,0,26,0,182,1,7,131,7,33,4,7,87,237,6,36,17,220,48,49,0,130,151,48,99,2,127,2,62,4,57,0,3,0,
    7,0,42,178,0,8,9,130,111,37,176,5,208,0,176,2,69,83,10,34,27,62,89,130,26,32,2,107,2,6,33,4,208,130,137,35,35,17,51,1,131,3,60,0,157,157,1,62,157,157,2,127,1,186,254,70,1,186,0,1,0,
    69,255,103,1,90,1,6,0,8,0,12,69,153,5,37,208,176,0,47,48,49,92,150,6,48,6,6,197,128,73,3,201,1,83,153,77,115,123,100,79,93,186,130,169,50,45,0,0,5,26,6,21,0,38,0,74,0,0,0,7,0,74,2,
    68,130,7,38,2,0,24,0,0,4,23,130,23,38,23,0,27,0,115,178,9,82,173,5,32,9,107,13,9,36,9,47,27,177,9,67,119,7,32,4,130,12,32,4,82,173,7,118,226,12,32,23,130,25,32,23,74,37,7,32,25,130,
    12,32,25,131,12,36,4,16,176,19,208,113,9,10,39,176,1,208,176,9,16,178,15,77,8,10,55,51,17,35,53,51,53,62,2,51,50,22,23,7,38,35,34,6,21,21,51,21,35,17,33,130,247,8,42,189,165,165,1,
    106,194,136,80,147,79,37,138,114,111,100,213,213,2,103,243,243,3,134,180,74,127,182,92,34,26,201,48,97,97,68,180,252,122,4,58,0,1,0,130,221,33,4,44,130,197,36,22,0,99,178,18,116,209,
    10,32,18,130,137,32,18,135,189,92,225,12,69,86,8,35,0,69,88,176,98,31,8,32,18,70,136,12,39,14,16,176,5,208,176,14,16,24,123,112,10,37,176,8,208,48,49,1,130,167,133,166,87,34,5,46,54,
    54,51,50,5,17,35,3,57,102,74,196,220,220,243,130,182,42,215,196,122,1,68,243,5,63,14,184,91,130,162,130,179,39,97,183,195,48,250,27,0,2,131,169,33,6,147,130,169,47,40,0,44,0,181,178,
    20,45,46,17,18,57,176,20,16,176,107,173,7,32,8,130,177,32,8,135,177,132,151,135,12,32,43,130,25,32,43,65,124,7,32,33,130,12,32,33,135,12,113,244,12,65,163,12,32,40,130,38,32,40,65,
    150,7,32,37,130,12,32,37,135,12,32,42,130,12,32,42,131,12,33,33,16,24,117,46,10,36,176,38,208,176,1,68,207,15,32,22,24,65,206,13,65,177,5,69,83,9,33,21,33,65,190,17,41,35,17,33,17,
    33,35,17,51,210,165,24,212,84,8,33,1,116,65,204,6,56,38,136,115,111,100,213,213,243,254,140,4,206,243,243,3,134,180,99,180,196,18,190,8,179,96,65,215,13,33,3,134,65,219,9,41,6,147,
    6,21,0,39,0,165,178,19,90,103,10,24,166,195,12,65,54,12,65,2,12,24,66,216,12,32,31,130,245,32,31,65,54,7,32,39,130,12,32,39,65,15,7,32,36,78,246,7,35,0,69,88,176,66,192,10,32,178,24,
    91,119,10,32,8,69,230,12,32,21,65,22,11,42,176,1,16,176,38,208,176,34,208,48,49,65,31,18,66,32,6,32,17,66,55,10,33,33,17,65,27,13,66,49,5,66,64,5,33,254,140,65,21,10,66,57,5,66,74,
    9,49,252,122,0,1,0,45,255,236,4,209,6,21,0,36,0,133,178,19,108,1,10,24,101,45,12,67,160,12,32,35,24,94,2,11,89,43,8,33,35,16,24,102,13,10,32,176,89,30,21,32,176,130,36,80,186,9,24,
    95,228,11,38,14,16,176,24,208,176,25,68,170,5,32,20,70,169,6,38,32,17,17,35,53,51,53,131,234,32,17,67,30,5,43,52,54,51,50,22,23,17,51,4,203,191,49,89,19,6,51,178,178,69,108,163,243,
    165,165,194,176,101,241,114,191,3,134,253,164,62,55,89,13,6,47,180,248,32,185,251,103,3,134,180,98,182,195,56,49,254,142,130,237,49,75,255,236,6,128,6,24,0,76,0,167,178,70,77,78,17,
    18,57,76,30,5,32,71,130,211,32,71,67,30,7,32,64,130,12,32,64,65,218,7,65,7,12,32,75,130,25,32,75,24,146,5,15,37,176,0,69,88,176,44,130,25,37,44,15,62,89,176,75,65,7,12,86,180,16,39,
    13,208,176,14,208,176,71,16,109,47,11,35,64,16,178,32,79,253,9,35,44,16,178,52,136,13,37,48,49,1,35,17,20,71,184,6,47,38,39,17,35,53,51,53,52,38,35,34,6,21,20,30,2,24,197,73,12,95,
    65,5,39,38,53,51,22,22,51,50,54,130,36,40,38,39,38,53,52,54,51,50,23,133,6,63,22,21,21,51,6,121,191,113,38,47,83,77,135,144,1,172,172,96,88,79,88,29,33,28,244,104,86,80,101,94,1,30,
    24,207,160,12,8,43,107,248,83,182,236,182,91,77,45,217,174,201,222,191,3,134,253,183,136,10,188,23,170,162,2,78,180,88,98,105,84,69,58,105,102,121,77,70,93,74,62,56,62,63,24,207,185,
    10,8,85,59,65,68,52,40,88,167,140,188,23,108,79,129,165,202,197,79,0,22,0,89,254,114,7,236,5,174,0,13,0,26,0,40,0,55,0,61,0,67,0,73,0,79,0,86,0,90,0,94,0,98,0,102,0,106,0,110,0,118,
    0,122,0,126,0,130,0,134,0,138,0,142,1,192,178,16,143,144,17,18,57,176,16,16,176,0,208,131,5,32,27,132,5,32,48,132,5,32,60,132,5,32,62,132,5,32,70,132,5,32,74,132,5,32,80,132,5,32,87,
    132,5,32,91,132,5,32,97,132,5,32,99,132,5,32,103,132,5,32,109,132,5,32,112,132,5,32,119,132,5,32,123,132,5,32,127,132,5,32,132,132,5,32,136,132,5,36,140,208,0,176,61,71,84,5,47,70,
    47,27,177,70,31,62,89,178,125,68,3,43,178,124,121,130,4,33,120,129,130,4,33,128,57,130,4,34,10,70,61,131,169,33,10,47,73,204,6,40,14,208,176,14,47,176,10,16,176,72,213,5,34,111,14,
    15,130,30,39,124,176,111,47,24,178,80,11,79,32,9,33,80,111,132,53,34,16,178,30,136,20,36,176,3,16,178,37,137,13,50,15,16,176,41,208,176,41,47,176,14,16,176,46,208,176,46,47,178,52,
    137,28,40,61,16,176,107,208,176,103,208,176,130,220,35,62,208,178,63,79,122,8,54,176,101,208,176,105,208,176,109,208,176,60,208,176,57,16,176,65,208,176,70,16,178,71,137,31,38,91,208,
    176,87,208,176,74,131,22,48,176,96,208,176,92,208,176,88,208,176,75,208,176,68,16,176,78,130,186,34,16,178,81,137,101,41,71,16,176,95,208,176,15,16,178,118,137,19,8,33,120,16,176,132,
    208,176,121,16,176,133,208,176,124,16,176,136,208,176,125,16,176,137,208,176,128,16,176,140,208,176,129,16,176,141,76,29,5,36,35,34,38,39,53,67,175,5,40,19,17,51,50,22,21,20,7,22,130,
    4,33,35,1,66,190,5,43,21,20,22,51,50,54,53,1,51,17,20,6,130,42,34,53,51,20,132,14,8,32,17,51,21,51,21,33,53,51,53,51,17,1,17,33,21,35,21,37,53,33,17,35,53,1,21,51,50,53,52,39,19,53,
    33,130,28,133,3,32,1,138,11,32,19,131,29,35,38,35,35,1,85,6,7,130,7,32,37,138,11,8,76,3,55,129,100,102,128,2,126,104,101,128,2,67,188,98,114,84,50,52,208,254,143,74,65,64,74,74,66,
    64,73,3,186,92,105,82,88,109,93,104,41,54,249,196,113,196,5,40,199,111,248,109,1,53,196,5,236,1,54,111,252,92,126,103,98,203,1,22,253,91,1,21,253,92,1,20,2,10,137,11,40,188,93,118,
    58,60,93,252,241,113,132,0,34,7,34,111,132,0,61,1,212,98,121,120,94,117,95,124,120,94,254,179,2,37,73,77,84,32,13,70,45,155,1,72,69,78,78,69,112,131,4,8,40,1,79,254,134,78,93,81,83,
    91,54,44,252,201,1,59,202,113,113,202,254,197,6,31,1,29,116,169,169,116,254,227,169,252,182,169,83,82,4,3,74,116,132,0,33,249,56,132,94,46,113,3,196,80,41,30,254,211,252,126,250,252,
    21,249,126,133,6,8,37,0,5,0,92,253,213,7,215,8,115,0,3,0,28,0,32,0,36,0,40,0,76,0,176,33,47,176,37,47,176,0,208,176,0,47,176,33,16,83,89,6,34,32,2,0,94,155,5,85,89,6,40,4,208,176,4,
    47,178,13,0,2,131,21,33,13,47,88,251,5,35,178,7,4,20,130,15,35,178,25,20,4,130,6,41,48,49,9,3,5,52,54,55,54,54,68,117,5,33,7,51,84,107,5,44,20,7,6,6,21,23,35,21,51,3,51,21,35,131,3,
    8,82,4,24,3,191,252,65,252,68,4,15,30,36,74,92,167,149,144,160,2,203,2,58,43,57,56,93,91,47,202,202,202,75,4,4,2,4,4,6,82,252,49,252,49,3,207,241,58,58,24,39,135,74,128,151,139,127,
    51,52,64,52,95,60,65,92,76,91,170,253,76,4,10,158,4,0,1,0,58,0,0,3,234,5,176,24,245,163,60,8,36,3,234,253,212,244,2,44,253,68,3,176,5,41,250,215,4,237,195,0,0,2,0,79,254,86,4,23,4,
    78,0,27,0,38,0,131,178,31,81,91,5,35,31,16,176,12,79,45,11,24,114,20,20,36,12,47,27,177,12,102,197,7,32,24,116,236,6,40,178,6,4,24,17,18,57,176,12,77,165,11,34,178,16,18,131,20,33,
    178,22,133,27,32,24,87,172,11,33,176,4,91,196,11,57,48,49,19,52,54,54,51,50,23,55,51,17,20,0,35,34,38,39,55,22,51,50,54,53,53,6,130,11,34,38,55,20,130,12,8,83,55,17,38,35,34,6,79,109,
    205,133,191,105,16,209,254,251,239,85,185,73,53,130,144,142,131,106,174,127,204,114,243,143,120,149,70,69,148,124,141,2,38,160,251,141,134,114,252,28,246,254,246,47,45,176,76,156,155,
    22,119,140,252,157,159,192,129,1,217,123,193,0,0,1,255,176,254,75,1,142,0,205,0,13,0,46,102,251,8,32,14,68,221,5,32,5,130,205,32,5,130,218,33,178,10,75,75,8,46,176,14,16,176,13,208,
    176,13,47,48,49,37,17,20,7,130,143,132,154,56,53,17,1,142,112,91,149,70,56,14,36,61,124,205,254,247,200,98,79,17,198,12,178,1,5,130,97,46,0,92,254,154,1,79,0,181,0,3,0,18,0,176,4,24,
    165,187,12,8,59,48,49,1,35,17,51,1,79,243,243,254,154,2,27,0,2,0,117,4,208,2,247,6,220,0,12,0,32,0,123,0,176,3,47,176,6,208,176,6,47,64,11,15,6,31,6,47,6,63,6,79,6,5,93,176,3,16,178,
    9,6,84,96,8,38,6,16,176,12,208,176,12,130,42,46,16,176,16,208,176,16,47,176,19,208,176,19,47,64,13,93,244,5,41,63,19,79,19,95,19,6,93,176,16,80,219,6,36,176,19,16,178,26,24,81,54,8,
    130,22,33,178,29,137,13,32,26,94,0,6,34,20,6,32,24,136,18,8,33,19,20,130,231,8,64,38,35,34,6,21,39,52,54,51,50,22,51,50,54,53,2,247,176,254,222,176,175,76,70,72,74,144,95,71,56,129,
    42,31,42,104,97,69,47,136,44,30,44,5,176,101,123,123,101,53,58,60,51,1,15,75,107,71,50,37,27,77,108,71,50,36,132,221,42,213,2,246,7,8,0,13,0,28,0,89,132,221,35,7,208,176,7,131,221,
    40,7,31,7,47,7,63,7,79,7,133,221,32,10,137,221,32,7,65,112,6,32,176,130,8,70,65,5,45,20,208,176,20,47,178,15,14,20,17,18,57,178,21,69,191,8,35,178,27,14,15,130,17,34,48,49,1,132,174,
    33,53,51,69,71,5,33,39,39,24,168,55,11,8,46,7,2,246,175,145,146,175,173,80,68,69,77,223,8,72,63,146,7,158,159,78,68,1,5,176,98,121,121,98,52,57,58,51,25,118,2,23,26,54,96,80,68,47,
    58,8,58,0,132,179,36,211,3,0,6,126,130,179,34,17,0,93,65,145,27,138,179,87,184,8,130,8,65,145,5,132,185,60,64,15,15,14,31,14,47,14,63,14,79,14,95,14,111,14,7,93,176,16,16,176,17,208,
    25,176,17,47,24,144,183,61,51,7,35,3,0,175,150,149,177,177,76,73,71,76,101,182,169,128,5,176,97,124,122,99,52,60,60,52,206,192,132,153,45,231,3,92,6,209,0,6,0,26,0,141,0,176,1,71,103,
    7,32,4,130,80,35,4,47,24,176,24,169,222,7,63,3,16,176,5,208,176,5,47,64,9,15,5,31,5,47,5,63,5,4,93,178,2,5,3,17,18,57,176,10,208,176,10,130,24,41,63,10,79,10,95,10,111,10,4,93,66,221,
    5,130,169,44,13,31,13,47,13,63,13,79,13,95,13,111,13,130,169,32,10,66,92,7,35,13,16,178,20,65,158,9,35,10,16,178,23,137,13,36,20,16,176,26,208,130,201,38,35,39,7,35,37,51,55,66,55,
    18,8,41,3,92,193,179,178,193,1,42,147,186,89,61,49,123,36,27,41,90,89,60,42,127,38,26,44,4,231,142,142,237,223,62,95,66,44,27,24,64,96,65,45,28,133,225,35,4,10,6,203,130,225,34,21,
    0,96,176,225,33,3,5,131,225,39,1,16,176,7,208,176,7,47,133,216,35,178,8,7,13,130,21,33,178,14,136,179,35,178,20,8,7,66,54,5,133,180,32,23,66,47,13,134,175,8,40,22,187,185,7,63,56,129,
    7,137,140,73,56,1,4,231,162,162,250,116,125,5,24,29,62,105,89,75,55,65,7,59,0,2,255,76,4,218,3,92,6,131,130,171,39,10,0,91,0,176,3,47,176,65,135,18,35,1,208,176,1,67,210,7,32,9,67,
    210,7,37,4,93,178,2,3,6,104,44,5,82,241,6,34,7,208,25,130,178,35,24,176,8,16,65,161,5,40,182,15,10,31,10,47,10,3,93,65,91,8,8,35,5,35,3,51,3,92,213,159,159,212,1,35,161,254,135,157,
    215,221,4,218,142,142,250,92,1,11,0,2,0,122,4,231,4,139,6,144,138,139,36,5,208,176,5,47,70,246,5,48,64,9,15,0,31,0,47,0,63,0,4,93,176,3,16,176,2,130,107,38,2,47,24,178,4,3,0,131,131,
    32,6,130,14,34,6,47,24,131,25,36,9,208,176,9,47,65,61,5,42,182,15,7,31,7,47,7,3,93,176,9,131,146,36,25,176,10,47,24,130,139,8,32,51,5,35,39,7,35,1,51,3,35,1,157,161,1,35,212,159,159,
    213,3,51,222,216,157,5,225,250,142,142,1,169,254,245,67,65,5,32,212,67,65,60,36,17,208,176,17,47,96,98,5,67,65,18,35,17,16,176,16,130,178,32,16,67,65,17,34,37,51,23,67,65,11,38,254,
    148,183,114,128,5,177,67,66,7,44,205,192,0,0,1,0,148,4,105,1,169,6,43,111,141,16,111,211,19,53,7,35,53,52,54,1,38,131,63,2,1,211,85,6,43,83,109,124,134,133,89,182,131,225,39,9,0,0,
    4,148,4,141,0,24,247,243,15,32,29,106,20,6,117,65,12,32,6,105,173,7,39,9,4,2,17,18,57,176,9,83,253,11,33,178,10,132,20,8,43,48,49,37,33,7,35,1,51,1,35,1,33,3,3,63,254,30,95,245,1,215,
    223,1,213,246,254,6,1,84,170,249,249,4,141,251,115,1,178,1,186,0,3,0,118,130,123,32,10,130,123,42,14,0,22,0,31,0,164,178,30,32,33,131,85,37,30,16,176,2,208,176,130,5,32,17,71,193,6,
    32,1,130,118,32,1,135,144,132,222,38,15,62,89,178,23,1,0,131,45,8,34,23,47,180,175,23,191,23,2,93,180,111,23,127,23,2,113,178,255,23,1,113,178,15,23,1,114,180,143,23,159,23,2,114,178,
    95,130,11,33,178,207,131,21,32,63,130,4,35,180,31,23,47,131,45,34,191,23,207,131,28,82,107,9,35,178,8,15,23,131,80,32,0,24,64,225,12,33,1,16,106,66,10,24,131,175,16,39,3,17,51,50,54,
    53,52,39,122,52,5,8,68,38,35,35,118,1,175,222,235,89,91,96,112,226,221,226,228,102,100,180,250,212,91,99,103,101,198,4,141,165,156,79,131,35,23,143,99,163,171,1,251,254,199,85,65,158,
    5,170,2,72,69,79,70,0,0,1,0,79,255,240,4,67,4,157,0,27,0,78,178,3,24,64,197,10,32,11,130,247,32,11,135,247,84,143,8,34,15,11,3,131,166,32,11,86,69,12,32,3,98,246,11,35,178,27,3,11,
    68,23,5,51,6,4,35,34,0,17,53,52,54,54,51,50,4,23,35,38,38,35,32,17,75,164,5,8,70,55,4,66,17,254,247,217,236,254,236,126,236,156,214,1,4,20,243,12,125,114,254,237,134,135,120,124,13,
    1,132,191,213,1,44,1,11,68,169,255,138,218,194,112,105,254,142,72,185,181,98,112,0,2,0,118,0,0,4,42,4,141,0,11,0,19,0,70,178,19,20,21,130,102,36,176,19,16,176,2,65,169,27,34,176,1,
    16,24,138,171,11,127,175,14,40,51,17,33,50,4,22,23,21,20,130,171,8,47,3,17,51,32,19,53,16,37,118,1,123,164,1,3,144,2,143,254,249,168,131,130,1,71,6,254,201,4,141,138,251,159,61,163,
    254,139,3,201,252,249,1,92,67,1,96,8,0,1,131,143,33,3,181,132,143,32,78,92,17,10,65,50,7,32,4,89,39,6,35,178,11,6,4,131,168,32,11,66,174,11,33,176,4,83,203,12,122,132,12,38,48,49,1,
    33,17,33,21,134,3,55,3,95,254,10,2,76,252,193,3,60,253,183,1,246,1,248,254,202,194,4,141,196,254,242,134,127,32,158,130,127,34,9,0,64,133,127,132,114,67,59,15,32,178,67,46,19,131,127,
    32,6,73,156,8,132,113,32,35,133,115,46,3,91,254,14,243,3,40,253,203,1,242,1,219,254,37,131,107,32,213,130,107,44,84,255,240,4,72,4,157,0,28,0,92,178,26,120,253,10,32,10,130,229,32,
    10,66,37,16,34,14,3,10,131,242,35,10,16,178,17,136,100,33,176,3,24,85,134,11,33,178,27,133,34,33,27,47,24,66,220,12,44,37,7,6,33,34,0,17,53,16,0,51,50,22,66,50,9,8,58,32,55,53,35,53,
    33,4,72,23,150,254,213,248,254,220,1,22,244,215,250,25,237,18,121,108,254,228,160,1,40,70,249,1,235,147,24,139,1,46,1,9,65,1,9,1,44,195,192,100,92,254,137,64,183,186,57,200,177,0,65,
    39,5,39,4,104,4,141,0,11,0,134,65,167,18,140,193,67,223,7,32,176,24,149,28,8,38,15,62,89,178,9,6,0,68,112,5,45,180,175,9,191,9,2,93,178,63,9,1,113,178,207,131,4,130,9,34,114,178,255,
    131,9,46,15,9,1,114,180,111,9,127,9,2,113,180,223,9,239,131,38,32,95,131,18,34,28,9,44,131,11,92,66,9,34,48,49,33,92,136,5,46,51,17,33,17,51,4,104,243,253,244,243,243,2,12,243,65,110,
    5,35,254,17,1,239,130,179,32,133,130,184,32,119,130,179,34,3,0,29,133,179,36,2,47,27,177,2,68,134,15,132,74,36,51,1,119,242,242,130,42,130,53,36,36,255,240,3,100,130,10,38,14,0,34,
    178,5,15,16,82,139,8,32,5,130,60,32,5,131,201,32,11,65,142,8,33,48,49,79,40,14,53,2,113,243,227,178,202,225,244,183,75,87,4,141,252,224,174,207,192,175,173,94,93,65,63,11,34,12,0,75,
    66,103,18,32,8,130,91,32,8,66,116,15,37,176,0,69,88,176,11,24,99,18,7,34,6,2,4,130,138,41,176,6,16,176,1,208,178,10,1,6,68,30,5,33,7,17,24,248,69,9,63,1,240,135,243,243,110,1,79,1,
    44,254,67,1,211,254,222,1,219,131,254,168,4,141,253,253,134,1,125,253,247,253,124,65,197,5,33,3,148,130,219,34,5,0,40,146,133,32,2,130,107,32,2,92,253,5,75,223,7,48,48,49,37,33,21,
    33,17,51,1,105,2,43,252,226,243,194,194,65,35,5,36,118,0,0,5,143,130,10,36,14,0,96,178,1,65,35,10,32,0,130,65,32,0,140,199,67,201,15,132,212,132,238,85,151,7,24,103,154,8,34,1,0,4,
    130,212,33,178,7,133,6,32,10,132,6,34,48,49,9,24,248,55,13,8,35,178,1,81,1,78,1,62,242,25,254,160,168,254,161,25,242,4,141,252,181,3,75,251,115,1,59,2,58,252,139,3,112,253,203,254,
    197,136,163,33,4,103,130,10,34,9,0,69,133,235,32,5,130,156,32,5,135,143,65,113,12,66,177,12,69,166,8,34,2,5,0,24,248,48,21,34,4,103,242,66,110,5,36,242,3,27,252,229,130,129,55,228,
    3,28,0,0,2,0,79,255,240,4,111,4,157,0,14,0,28,0,70,178,3,29,30,73,86,6,32,18,69,91,6,70,13,20,32,176,70,6,16,24,95,120,12,33,16,0,69,255,5,38,18,54,51,50,0,17,39,81,165,12,8,51,4,111,
    254,223,237,236,254,218,133,240,155,240,1,32,242,150,136,134,152,153,135,136,148,2,44,254,248,254,204,1,53,1,12,46,172,1,7,139,254,199,254,245,8,183,192,192,183,53,178,199,195,182,
    131,167,46,118,0,0,4,44,4,141,0,10,0,19,0,77,178,4,70,3,5,32,4,79,111,9,36,3,47,27,177,3,65,40,7,32,1,106,233,6,32,178,24,248,37,23,24,77,7,9,106,95,5,43,33,50,22,21,20,6,7,39,51,50,
    54,53,130,174,8,32,35,1,105,243,1,229,212,253,241,212,254,242,104,119,121,101,243,1,153,254,103,4,141,213,173,169,198,3,196,88,84,87,105,131,145,44,76,255,48,4,108,4,157,0,20,0,34,
    0,70,24,179,35,10,32,31,65,57,6,32,17,130,132,32,17,65,186,12,36,15,62,89,176,17,106,41,12,32,8,103,127,13,37,1,20,6,7,23,7,98,219,5,34,39,53,52,65,63,20,8,59,108,110,99,207,157,254,
    246,50,52,154,242,132,1,130,241,156,239,1,34,241,151,137,134,151,151,136,137,149,2,44,163,241,72,152,136,201,9,139,1,1,170,57,171,1,5,142,254,200,254,244,8,183,192,195,182,51,176,201,
    195,182,71,75,6,52,57,4,141,0,13,0,22,0,97,178,5,23,24,17,18,57,176,5,16,176,15,80,183,11,67,13,12,66,243,7,32,13,130,207,32,13,130,12,35,178,14,2,4,131,52,32,14,70,215,11,35,178,10,
    0,14,131,20,24,83,75,12,35,48,49,1,35,112,133,5,38,21,20,7,1,21,33,1,65,94,7,8,57,2,72,223,243,1,200,218,240,225,1,18,254,252,254,52,213,108,108,105,111,213,1,169,254,87,4,141,183,
    170,235,91,254,37,11,2,107,95,78,81,96,0,1,0,62,255,240,3,239,4,157,0,37,0,99,178,9,38,39,68,217,8,32,9,130,141,32,9,135,167,24,66,180,8,34,3,28,9,130,33,35,178,13,9,28,130,6,24,172,
    71,13,33,176,3,24,69,43,12,32,33,132,41,33,176,28,94,242,11,130,175,35,52,38,36,38,86,228,7,32,35,66,184,5,35,20,22,23,22,24,138,119,10,8,75,33,50,54,3,2,104,254,207,176,83,246,195,
    210,254,243,120,101,95,110,113,143,221,192,248,204,138,229,126,244,1,0,97,111,1,50,66,79,76,98,131,92,146,187,200,160,81,93,77,64,58,76,35,54,178,142,153,174,93,170,113,192,74,0,1,
    0,36,0,0,4,22,4,141,0,7,0,46,70,149,18,68,207,7,34,176,6,16,119,206,10,34,176,4,208,71,171,7,49,53,33,4,22,254,126,243,254,131,3,242,3,201,252,55,3,201,196,130,83,36,103,255,240,4,
    30,130,83,38,15,0,53,178,12,16,17,65,37,8,68,49,12,72,152,8,106,195,10,35,8,16,176,15,131,90,36,17,20,4,32,36,24,122,147,7,8,32,55,17,4,30,254,255,254,74,255,0,241,126,108,229,4,4,
    141,253,1,190,224,221,193,2,255,253,0,115,104,212,3,7,0,130,113,32,9,130,197,32,114,130,113,24,246,107,8,67,128,12,36,7,47,27,177,7,65,157,7,70,132,8,34,1,3,5,70,1,5,62,23,55,1,33,
    1,35,1,33,2,42,19,18,1,34,1,1,254,70,246,254,71,1,1,1,56,77,75,3,87,251,115,69,11,5,36,40,0,0,5,229,130,10,32,12,24,137,233,12,67,80,12,135,12,68,164,21,35,0,69,88,176,76,59,8,34,0,
    1,3,130,123,33,178,5,133,6,32,10,24,247,250,11,32,3,24,230,46,7,62,74,175,236,254,230,235,216,219,235,254,230,236,177,216,214,1,43,3,98,251,115,3,65,252,191,4,141,252,156,3,100,130,
    245,32,21,130,245,32,74,130,147,34,11,0,83,65,187,5,76,36,12,72,81,12,99,46,12,32,7,24,94,160,7,34,0,1,4,131,134,32,6,133,6,34,3,0,6,131,13,72,102,5,34,48,49,1,24,230,45,11,8,35,39,
    242,1,28,254,137,1,140,254,224,255,250,254,228,1,129,254,136,1,26,2,250,1,147,253,190,253,181,1,153,254,103,2,75,2,66,130,145,32,5,130,145,32,54,65,135,11,140,145,65,135,12,65,255,
    8,133,132,130,111,104,199,5,59,1,33,2,29,1,14,1,11,254,93,242,254,100,1,11,2,122,2,19,253,7,254,108,1,161,2,236,0,130,97,36,65,0,0,3,243,130,97,32,9,24,247,243,12,68,34,15,66,172,10,
    32,178,24,247,243,39,8,38,120,2,123,252,78,2,108,253,149,3,160,194,194,141,3,60,196,138,0,0,2,0,75,255,245,2,170,3,32,0,13,0,23,0,70,178,3,24,25,70,145,6,32,16,68,161,6,37,10,47,27,
    177,10,25,78,40,6,32,3,130,12,33,3,15,130,12,32,10,108,7,15,32,21,107,219,13,35,35,34,38,53,88,67,5,8,44,21,39,52,35,34,7,21,20,51,50,55,2,170,158,144,146,159,158,145,144,160,187,117,
    114,3,119,111,4,1,62,159,170,170,158,152,157,174,173,158,12,169,159,184,169,154,130,255,32,128,130,148,35,2,3,19,0,24,184,27,8,32,5,130,115,32,5,135,128,32,1,70,106,6,35,176,5,16,176,
    87,5,5,32,3,138,120,50,33,35,17,7,53,37,51,2,2,185,201,1,111,19,2,58,48,146,119,130,81,32,60,130,81,32,178,130,225,33,23,0,24,218,175,18,135,88,71,223,7,108,208,11,40,2,22,0,17,18,
    57,178,3,15,131,6,34,176,15,16,24,191,100,10,35,178,12,0,15,131,27,32,21,132,6,36,48,49,33,33,53,24,218,175,40,33,125,1,24,218,173,18,35,255,245,2,169,130,163,36,36,0,127,178,30,93,
    203,10,32,13,130,239,32,13,135,163,32,23,97,95,6,35,178,0,23,13,130,124,52,124,176,0,47,24,180,80,0,96,0,2,113,182,128,0,144,0,160,0,3,93,24,218,175,14,34,10,0,6,78,238,6,32,36,65,
    39,8,34,178,18,36,132,208,35,23,16,178,30,137,20,24,65,135,8,89,145,5,37,34,6,21,35,52,54,89,253,6,32,21,84,177,12,39,52,39,35,1,12,81,132,54,24,218,175,20,33,1,210,24,218,175,24,42,
    2,0,53,0,0,2,190,3,21,0,10,25,15,231,14,135,224,67,50,8,34,1,9,4,131,182,35,1,47,178,2,136,161,44,176,6,208,176,1,16,176,11,208,178,8,11,6,70,118,5,88,232,5,8,50,1,51,21,35,21,35,53,
    33,39,1,51,1,51,53,7,2,95,95,95,187,254,154,9,1,109,189,254,139,186,14,1,58,151,163,163,121,1,249,254,37,242,22,0,0,1,0,79,255,245,2,174,130,131,36,26,0,106,178,13,114,105,10,36,2,
    47,27,177,2,135,136,32,13,71,124,6,34,176,2,16,24,217,72,10,35,178,7,2,13,130,120,36,176,7,47,178,24,136,150,35,178,5,24,7,131,20,108,237,12,35,178,17,19,24,130,20,35,178,26,24,19,
    130,6,39,48,49,19,19,33,21,33,7,65,78,5,38,6,35,34,38,39,51,22,65,102,6,8,47,7,98,52,1,236,254,172,20,62,71,131,140,163,140,129,173,2,185,5,114,117,67,66,67,53,1,127,1,150,150,148,
    27,134,122,120,153,132,99,82,125,56,68,40,0,0,2,0,77,130,189,44,185,3,34,0,19,0,30,0,91,178,20,31,32,98,57,6,32,12,24,111,69,11,135,197,115,47,8,33,0,16,109,151,10,110,185,10,32,20,
    136,197,33,176,12,109,158,11,37,48,49,1,21,34,6,138,175,37,53,53,52,54,51,3,130,18,8,56,21,20,51,50,54,53,52,2,50,145,137,13,71,107,117,135,168,134,147,171,240,222,150,45,66,15,127,
    53,68,3,34,153,95,98,69,142,122,119,153,167,155,49,210,232,254,87,36,23,36,145,70,54,116,0,1,0,54,130,188,34,174,3,21,90,25,14,135,168,71,133,8,35,5,16,178,4,136,147,32,178,127,106,
    5,130,140,58,1,35,1,33,53,33,2,174,254,173,196,1,83,254,76,2,120,2,172,253,84,2,127,150,0,0,3,68,191,8,43,19,0,28,0,36,0,150,178,7,37,38,17,105,79,5,38,20,208,176,7,16,176,34,74,31,
    11,135,112,32,7,70,32,7,34,34,7,17,130,45,56,124,176,34,47,24,182,128,34,144,34,160,34,3,93,180,80,34,96,34,2,113,180,0,34,16,131,6,41,64,34,80,34,2,93,180,208,34,224,130,13,33,178,
    25,137,158,34,2,34,25,130,59,35,178,12,25,34,133,112,32,178,65,78,10,33,17,16,24,226,116,13,33,20,7,72,211,6,34,53,52,55,72,240,6,8,93,1,50,54,52,38,34,6,20,22,19,52,34,21,20,22,50,
    54,2,151,113,132,161,142,140,164,132,113,155,129,130,155,254,228,53,64,65,106,64,64,151,196,51,96,49,2,65,116,55,61,128,106,122,121,107,128,61,55,116,105,118,118,253,224,51,90,48,48,
    90,51,1,171,86,86,39,48,48,0,2,0,70,255,247,2,163,3,32,0,19,0,31,0,96,178,20,83,101,5,35,20,16,176,8,24,80,209,11,135,255,24,125,164,8,34,2,16,8,132,255,39,2,47,24,176,16,16,178,17,
    136,221,34,176,2,16,139,207,32,8,66,30,14,34,6,35,34,134,198,43,23,21,20,6,7,35,53,50,54,39,50,55,92,106,5,130,204,8,55,1,231,66,90,126,135,170,132,139,162,2,220,224,19,143,121,99,
    78,35,66,52,51,65,60,1,54,57,138,125,120,164,166,151,59,215,217,1,147,82,172,52,69,72,65,78,57,55,68,0,1,0,144,2,135,3,45,107,171,27,49,45,253,99,2,157,2,135,170,0,3,0,150,4,72,2,162,
    6,149,130,9,49,15,0,27,0,78,0,176,13,47,176,25,208,176,25,47,178,7,9,90,118,8,35,2,208,176,2,86,82,7,32,15,86,82,7,39,79,0,95,0,111,0,7,93,130,222,37,176,3,208,25,176,3,130,247,35,
    13,16,178,19,136,54,38,48,49,1,51,7,35,7,68,250,6,131,230,32,55,89,167,5,8,46,52,38,35,34,6,1,188,230,245,149,130,110,78,76,108,105,79,81,107,99,52,37,36,48,48,36,37,52,6,149,194,222,
    78,100,101,77,74,99,98,75,37,49,49,37,39,51,51,130,155,50,10,254,74,4,27,4,78,0,41,0,54,0,67,0,155,178,8,68,69,122,5,6,38,48,208,176,8,16,176,58,113,131,11,32,27,71,97,6,37,22,47,27,
    177,22,17,130,12,38,38,16,176,40,208,176,40,25,30,65,11,35,178,8,22,38,132,65,36,47,178,15,22,8,131,9,35,15,47,178,53,80,106,8,35,178,27,53,15,130,20,34,178,31,8,132,37,35,22,16,178,
    48,92,23,9,35,8,16,178,58,137,13,35,38,16,178,65,136,13,130,242,37,35,22,21,21,20,6,130,237,39,39,6,21,20,23,51,22,22,133,13,32,36,66,173,5,33,55,38,71,204,6,35,23,33,1,6,130,32,40,
    22,51,50,54,53,52,39,37,3,115,156,8,8,125,34,6,21,4,27,138,58,115,206,128,81,69,37,115,194,195,202,143,250,154,217,254,245,182,50,117,90,100,252,199,85,75,1,113,253,48,36,49,136,114,
    134,172,147,254,234,64,122,89,88,119,117,184,117,3,160,85,105,22,100,169,95,18,35,47,74,3,1,154,142,88,166,98,155,121,165,89,50,72,119,81,49,158,95,22,162,202,20,251,229,19,72,48,66,
    77,94,64,107,9,2,2,179,75,102,103,78,18,74,102,102,77,0,2,0,86,255,235,4,95,4,78,0,16,0,29,0,110,24,127,7,10,32,9,65,83,6,36,9,47,27,177,9,65,83,7,24,77,214,12,68,107,8,24,98,31,12,
    39,0,9,2,17,18,57,178,11,132,6,33,176,2,119,94,12,35,9,16,178,27,65,44,10,8,83,37,6,35,34,2,53,53,16,18,51,50,23,55,51,3,19,35,1,20,22,51,50,54,55,53,38,38,35,34,6,3,99,110,242,199,
    230,232,199,233,113,28,221,108,115,221,253,199,124,116,96,124,23,17,125,99,115,127,196,217,1,32,244,15,1,10,1,54,215,195,253,226,253,228,1,249,160,172,171,166,47,165,185,197,0,130,
    209,46,155,0,0,4,242,5,176,0,22,0,30,0,97,178,24,69,217,5,32,24,106,183,9,24,65,210,12,73,6,8,131,196,32,15,130,235,39,15,15,62,89,178,23,3,1,87,44,5,74,29,11,34,9,0,23,73,209,5,24,
    123,205,10,79,133,10,8,78,7,22,19,21,20,23,21,33,38,39,53,52,38,35,37,33,50,54,53,52,33,33,1,151,252,2,41,245,255,247,229,5,71,254,252,59,4,123,112,254,211,1,20,144,129,254,248,254,
    227,2,86,253,170,5,176,217,205,227,101,69,254,246,115,169,61,26,49,184,121,116,128,202,113,109,230,0,0,1,0,130,197,33,5,48,130,197,34,12,0,88,75,177,5,24,91,75,33,82,195,21,34,47,178,
    31,24,151,79,35,61,2,67,172,252,252,139,1,172,1,54,254,12,2,32,254,208,2,112,253,144,5,176,253,156,2,100,253,71,253,9,131,145,40,129,0,0,4,53,6,0,0,12,76,67,7,24,208,28,12,24,143,205,
    12,66,26,12,83,85,8,39,7,8,2,17,18,57,176,7,79,127,14,91,148,7,40,17,35,17,51,17,51,1,33,1,130,2,60,226,111,242,242,105,1,15,1,28,254,159,1,143,254,230,1,217,254,39,6,0,252,156,1,158,
    254,17,253,181,65,29,6,38,18,5,176,0,11,0,76,65,29,5,65,212,12,32,7,24,166,113,11,65,225,12,24,104,183,8,34,0,3,1,130,139,33,178,5,133,6,33,9,0,77,224,6,139,131,59,151,252,252,6,2,
    25,1,56,253,165,2,127,254,200,2,154,253,102,5,176,253,127,2,129,253,53,253,27,65,15,7,36,34,6,24,0,10,140,131,103,64,7,32,6,24,179,99,11,140,131,32,9,112,130,6,34,178,0,6,133,131,133,
    6,32,8,140,131,65,6,5,58,115,242,242,1,89,1,42,254,80,1,220,254,219,1,235,254,21,6,24,252,132,1,158,254,12,253,186,131,129,46,62,255,19,3,239,5,115,0,42,0,111,178,19,43,44,80,99,21,
    24,85,157,8,34,3,34,9,80,92,5,35,176,12,208,176,89,1,12,33,176,9,111,237,11,35,178,16,24,19,131,40,38,34,16,176,31,208,176,34,111,77,11,35,178,38,3,40,130,26,80,111,10,35,55,53,51,
    21,24,114,164,9,80,114,7,40,7,21,35,53,38,38,53,51,20,80,116,9,36,207,169,160,166,203,80,117,8,36,195,174,160,189,227,80,117,12,39,134,180,16,217,220,21,192,141,80,121,9,41,134,172,
    17,225,225,19,199,154,192,74,131,235,46,56,0,0,4,26,4,157,0,31,0,110,178,27,32,33,24,79,61,13,79,177,16,34,31,19,5,130,33,33,176,31,127,61,12,32,5,24,127,196,11,52,176,7,208,176,8,
    208,176,0,16,176,12,208,176,31,16,176,14,208,176,19,16,123,232,11,34,23,31,26,133,234,48,33,22,7,33,7,33,53,51,54,54,39,39,35,53,51,39,38,24,113,86,10,8,60,23,23,33,3,71,254,133,6,
    80,2,152,1,252,101,10,41,43,3,1,160,155,3,6,216,191,194,217,243,87,80,77,87,5,4,1,128,1,229,178,112,195,195,11,147,125,7,147,105,206,238,212,188,97,106,126,121,105,0,1,0,14,130,209,
    41,63,4,141,0,24,0,149,178,0,25,131,108,66,202,5,79,8,12,36,24,47,27,177,24,135,222,85,189,8,34,0,12,24,130,155,35,178,9,12,1,130,6,34,176,9,47,77,193,5,58,64,13,15,4,31,4,47,4,63,
    4,79,4,95,4,6,93,182,207,4,223,4,239,4,3,93,178,6,24,64,66,12,32,10,137,13,130,245,32,9,95,238,7,37,6,16,176,19,208,176,115,23,7,79,101,5,37,51,21,33,7,21,33,130,1,34,35,53,33,130,
    1,8,32,39,33,53,51,1,33,2,37,1,15,1,11,254,190,213,254,218,16,1,54,254,202,242,254,202,1,54,9,254,211,220,254,190,79,133,6,45,183,147,29,42,145,217,217,145,54,17,147,2,73,0,130,235,
    36,118,0,0,3,151,130,235,38,5,0,50,178,1,6,7,24,82,139,13,89,141,15,130,131,24,74,17,13,47,33,17,35,17,33,3,151,253,210,243,3,33,3,201,252,55,130,72,34,0,2,0,81,201,7,40,3,0,8,0,60,
    178,5,9,10,84,17,6,32,2,70,185,6,88,128,20,35,178,5,0,2,130,39,24,99,109,10,8,33,48,49,33,33,1,51,3,39,7,3,33,4,114,251,151,1,185,246,105,18,19,222,1,227,4,141,254,201,75,77,253,111,
    0,3,86,107,8,41,3,0,18,0,32,0,118,178,7,33,74,168,6,38,176,1,208,176,7,16,176,120,43,7,36,15,47,27,177,15,135,205,32,7,130,12,38,7,15,62,89,178,3,15,131,239,47,124,176,3,47,24,180,
    96,3,112,3,2,93,180,48,3,64,130,6,36,178,0,3,1,113,138,233,32,176,25,4,170,12,130,88,70,104,13,35,33,53,33,5,86,161,27,39,3,56,254,90,1,166,1,55,86,167,20,35,1,223,195,118,86,169,28,
    32,1,65,79,8,38,8,0,56,178,7,9,10,65,163,8,65,71,20,90,111,13,34,7,2,0,130,46,65,73,5,8,37,1,33,1,39,7,1,10,254,255,1,185,246,1,186,254,255,254,222,18,19,4,141,251,115,3,86,75,77,0,
    3,0,66,0,0,3,85,4,141,130,9,40,7,0,11,0,94,178,4,12,13,85,127,5,36,176,0,208,176,4,75,101,9,82,147,12,80,83,8,90,148,9,34,178,7,10,131,115,36,176,7,47,178,4,72,83,8,34,176,10,16,116,
    196,10,131,143,38,53,33,3,33,53,33,19,130,3,43,3,85,252,237,3,19,73,253,126,2,130,73,131,9,38,195,1,56,196,1,10,196,66,157,6,33,4,98,130,147,34,7,0,63,24,112,239,12,36,6,47,27,177,
    6,89,84,20,69,208,8,119,177,14,91,40,6,40,33,4,98,244,253,251,243,3,236,66,173,7,34,1,0,68,130,247,32,230,130,99,37,12,0,75,178,0,13,24,215,97,9,84,233,12,89,14,8,32,1,136,210,83,249,
    6,35,176,8,16,178,102,156,9,130,252,46,8,17,18,57,48,49,1,1,33,21,33,53,1,1,53,130,6,8,40,1,2,144,254,230,2,112,252,94,1,63,254,193,3,124,253,186,1,22,2,69,254,127,196,152,1,183,1,
    166,152,196,254,143,0,3,0,80,0,0,5,77,130,133,40,17,0,22,0,28,0,111,178,8,89,55,5,36,8,16,176,20,208,130,98,33,176,26,67,59,6,32,16,130,249,32,16,84,178,12,37,15,62,89,178,15,16,131,
    117,41,176,15,47,176,0,208,178,9,8,16,68,124,6,34,6,208,176,24,116,27,12,33,176,15,86,177,11,130,83,37,176,20,16,176,27,208,130,173,8,91,22,4,21,20,4,7,21,35,53,38,36,53,52,36,55,53,
    51,1,2,5,17,4,5,52,38,39,17,36,3,73,240,1,20,254,233,237,243,240,254,234,1,23,239,243,253,249,4,1,24,254,236,3,25,144,130,1,18,4,21,15,246,202,208,250,15,109,108,15,249,208,205,247,
    13,120,253,183,254,253,21,2,42,21,251,133,129,10,253,214,21,0,0,1,132,219,32,3,130,219,34,24,0,75,69,79,12,32,18,130,203,32,18,69,66,16,34,22,12,18,131,190,32,22,131,203,42,176,18,
    16,176,23,208,176,4,208,176,22,112,147,12,32,10,131,179,47,54,54,53,17,51,17,6,7,6,7,17,35,17,38,2,3,130,12,8,52,20,22,23,17,51,3,35,127,110,243,1,104,125,250,243,227,251,2,243,112,
    125,243,1,221,24,194,167,1,47,254,205,227,147,175,29,254,232,1,23,22,1,42,1,0,1,54,254,209,168,192,24,2,175,130,161,44,95,0,0,4,132,4,157,0,35,0,92,178,7,24,70,1,10,32,25,130,161,32,
    25,135,161,32,15,74,132,6,36,176,0,69,88,176,71,188,7,34,176,15,16,94,153,11,38,14,208,176,0,208,176,25,24,89,51,12,38,17,16,176,32,208,176,33,130,178,32,37,130,178,78,64,6,32,21,130,
    170,36,21,33,53,51,38,96,213,6,8,64,0,21,21,20,6,7,51,21,33,2,173,120,108,148,141,138,148,118,116,254,48,176,189,131,242,156,234,1,42,99,89,182,254,47,200,34,201,176,43,158,172,169,
    164,40,177,199,35,200,196,155,1,39,22,145,236,132,254,227,237,25,141,223,74,196,0,130,197,44,36,255,236,5,82,4,141,0,25,0,107,178,22,24,86,35,15,135,197,32,14,130,197,32,14,89,129,
    7,32,24,130,12,32,24,24,85,140,22,35,178,8,2,14,77,138,5,36,176,14,16,178,15,111,245,8,33,176,8,66,68,11,44,48,49,1,33,53,33,21,33,21,54,51,50,22,130,194,33,35,53,90,240,5,8,42,34,
    7,17,35,1,126,254,166,3,173,254,160,138,141,218,240,240,235,115,118,116,117,129,133,243,3,201,196,196,238,39,212,198,188,192,189,84,105,114,103,38,253,231,98,57,10,36,29,0,143,178,
    3,24,89,217,10,87,134,20,32,178,98,57,19,33,178,21,98,78,5,45,21,47,178,255,21,1,113,178,15,21,1,114,178,63,131,9,32,207,130,4,47,180,111,21,127,21,2,113,180,175,21,191,21,2,93,178,
    95,131,28,32,143,131,4,24,102,189,9,33,176,3,72,100,12,32,29,98,122,25,33,34,3,130,239,36,22,22,51,50,54,98,124,20,40,251,22,1,128,254,128,10,126,131,98,129,19,55,207,196,148,159,98,
    112,0,2,0,36,0,0,7,21,4,141,0,23,0,32,0,118,178,4,70,183,5,36,4,16,176,24,208,72,107,5,67,27,12,86,147,8,33,0,69,95,11,9,33,176,18,125,79,12,32,11,24,89,76,11,41,178,20,18,3,17,18,
    57,176,20,47,24,69,103,11,130,225,93,85,12,55,20,6,7,33,17,33,3,6,2,6,35,35,55,55,54,54,55,19,33,17,51,50,22,37,100,12,5,8,64,38,35,7,21,249,207,254,21,254,164,14,11,88,172,145,52,
    1,38,96,78,12,21,3,59,236,218,250,253,64,241,103,117,118,102,1,127,171,210,2,3,201,254,156,239,254,255,117,205,2,7,159,237,2,43,254,108,208,12,254,142,107,83,81,99,0,92,27,5,33,7,24,
    130,229,37,19,0,28,0,193,178,24,183,191,17,36,19,47,27,177,19,87,248,12,135,12,32,16,130,25,32,16,92,40,16,34,0,16,19,131,214,47,0,47,180,175,0,191,0,2,93,178,63,0,1,113,178,207,131,
    4,130,9,39,114,178,95,0,1,114,178,255,131,14,32,15,130,9,42,180,111,0,127,0,2,113,180,223,0,239,130,43,35,180,31,0,47,130,6,33,178,159,130,25,35,178,4,13,2,131,72,36,4,47,176,0,16,
    119,167,10,33,176,4,79,22,12,32,13,66,233,15,45,17,51,17,51,50,22,22,21,20,6,35,33,17,33,24,151,105,13,8,33,105,1,253,243,242,140,210,111,255,210,254,31,254,3,243,243,2,240,241,103,
    117,118,102,2,158,1,239,254,108,95,171,112,175,208,97,150,5,33,253,168,65,31,7,46,1,0,36,0,0,5,82,4,141,0,21,0,87,178,18,124,5,10,90,250,12,36,20,47,27,177,20,65,10,15,34,176,3,16,
    71,94,11,37,0,208,178,8,20,3,67,174,5,68,143,10,67,157,12,32,23,130,175,34,52,38,35,67,153,11,39,134,142,222,235,4,243,116,116,67,151,6,53,237,38,207,203,254,152,1,90,124,105,38,253,
    231,0,0,1,0,118,254,159,4,97,130,159,38,11,0,79,178,3,12,13,130,95,121,204,13,65,173,7,72,19,20,72,137,13,32,8,71,56,8,38,176,9,208,48,49,33,33,130,141,33,33,17,98,184,5,44,97,254,
    138,243,254,126,243,2,5,243,254,159,1,130,111,35,252,54,3,202,66,61,5,33,4,40,132,125,38,20,0,94,178,8,21,22,132,223,95,161,9,140,117,97,77,8,24,86,70,12,35,178,3,10,8,131,53,35,3,
    47,176,8,101,189,12,124,188,12,8,61,48,49,1,33,21,51,22,22,16,6,35,33,17,33,1,50,54,53,52,38,39,35,17,3,178,253,183,252,207,244,248,217,254,31,3,60,254,168,104,115,112,102,246,3,203,
    224,3,196,254,168,204,4,141,252,54,99,84,79,93,1,254,156,130,169,36,39,254,175,5,21,130,169,40,15,0,21,0,91,178,19,22,23,102,79,6,36,5,208,0,176,13,110,201,10,92,94,12,98,99,13,76,
    246,6,36,13,16,176,10,208,130,171,39,176,16,208,176,17,208,176,5,103,29,11,37,48,49,55,62,2,55,24,170,101,16,8,51,130,74,66,35,5,12,3,61,150,242,252,247,243,1,1,116,1,240,254,161,7,
    13,195,81,134,180,126,1,193,252,54,253,236,1,81,254,175,2,20,3,6,252,254,174,0,1,0,26,0,0,6,31,130,171,36,21,0,158,178,1,66,115,10,96,93,12,36,14,47,27,177,14,65,221,20,24,117,21,12,
    68,152,12,32,21,130,51,32,21,131,212,34,12,3,14,131,255,45,12,47,178,63,12,1,113,178,95,12,1,114,178,207,130,9,48,180,175,12,191,12,2,93,180,143,12,159,12,2,114,176,15,208,120,116,
    11,37,4,208,178,8,15,4,130,55,35,178,19,1,15,73,66,5,33,35,17,130,1,33,3,33,24,170,131,13,56,3,245,95,243,96,252,254,211,1,92,254,196,1,30,247,84,243,84,247,1,30,254,194,1,94,130,18,
    35,213,254,43,1,130,3,39,2,84,2,57,254,32,1,224,131,3,36,253,208,253,163,0,130,245,44,66,255,240,3,231,4,157,0,39,0,138,178,38,121,93,10,66,67,12,123,31,8,32,10,78,144,11,35,178,6,
    10,22,130,144,33,178,38,132,6,37,176,38,47,178,207,38,130,214,32,63,130,4,40,180,175,38,191,38,2,93,178,255,130,11,34,178,15,38,130,231,32,95,131,4,32,35,66,235,8,35,178,16,35,38,131,
    59,34,28,22,10,24,92,23,16,33,48,49,116,110,6,32,35,85,138,7,45,7,22,22,21,20,4,35,34,38,39,38,53,51,22,84,130,5,8,63,35,53,51,54,2,226,112,107,91,102,243,243,195,216,244,110,93,111,
    110,254,254,220,93,175,63,124,243,11,202,119,116,224,148,154,199,3,67,70,79,70,60,148,179,167,150,91,138,39,36,145,91,159,181,45,47,91,159,147,87,72,166,3,176,4,77,227,5,43,4,110,4,
    141,0,9,0,76,178,0,10,11,24,91,53,13,74,75,12,79,174,15,32,176,24,66,14,12,34,4,3,0,130,59,35,178,9,5,8,65,159,5,60,51,17,35,17,1,35,17,51,17,3,123,243,243,253,238,243,243,4,141,251,
    115,3,35,252,221,4,141,252,224,134,119,32,64,130,119,34,12,0,119,75,89,25,100,143,12,82,158,12,79,84,8,34,6,2,5,130,112,46,176,6,47,178,63,6,1,113,178,95,6,1,114,178,207,130,9,45,180,
    175,6,191,6,2,93,180,143,6,159,6,2,114,66,76,10,32,178,102,25,8,32,35,82,55,11,60,211,106,243,243,99,1,56,1,29,254,114,1,173,254,209,1,213,254,43,4,141,254,32,1,224,253,197,253,174,
    97,71,6,32,85,130,175,38,16,0,77,178,4,17,18,65,39,21,82,49,20,33,176,0,80,193,12,32,9,68,106,11,84,137,6,36,3,6,2,6,7,71,102,6,8,37,4,85,243,254,164,15,12,87,170,140,58,1,39,98,74,
    12,22,4,141,251,115,3,201,254,159,237,254,254,120,1,205,4,11,160,230,2,43,0,130,145,36,31,255,236,4,57,130,145,36,15,0,67,178,0,97,133,10,78,207,12,78,6,12,69,6,7,35,178,1,8,15,130,
    192,125,210,10,130,135,8,32,23,19,33,1,14,2,35,39,55,23,50,55,1,33,2,41,19,243,1,10,254,112,56,90,126,90,102,1,87,96,51,254,91,130,29,46,75,55,2,121,252,126,126,105,56,5,192,4,97,3,
    127,131,133,36,118,254,175,5,36,130,133,36,11,0,66,178,9,70,5,6,32,3,68,213,5,96,14,12,67,75,12,97,163,8,69,248,10,40,0,208,48,49,37,51,3,35,17,69,248,7,52,98,194,20,221,252,67,243,
    2,5,244,195,253,236,1,81,4,141,252,54,3,202,130,247,33,65,0,98,209,5,34,17,0,70,24,168,31,17,72,34,12,101,160,16,34,13,1,9,130,247,33,176,13,78,125,11,32,48,24,168,29,7,35,39,17,51,
    17,116,186,5,60,51,4,22,243,134,129,234,240,1,243,111,121,130,133,243,1,170,38,210,209,1,102,254,158,119,108,38,2,31,66,187,5,33,6,14,132,243,34,65,178,7,134,243,67,12,8,144,116,78,
    232,10,36,3,16,176,6,208,79,47,5,36,176,6,16,176,10,70,235,5,32,51,135,242,42,6,14,250,104,243,1,95,243,1,96,243,133,237,134,241,36,118,254,175,6,209,130,111,36,15,0,65,178,11,65,233,
    6,65,99,19,94,63,8,32,0,68,99,8,39,176,13,208,176,9,208,176,7,131,108,33,176,14,65,98,14,131,119,38,6,15,194,20,221,250,150,133,118,65,105,11,132,123,38,2,0,10,0,0,5,27,130,123,40,
    12,0,21,0,94,178,8,22,23,71,103,6,32,20,74,139,6,65,228,12,69,242,8,32,7,74,126,11,35,178,10,7,3,131,53,35,10,47,176,3,77,169,13,84,192,11,34,48,49,1,74,115,5,37,53,33,17,51,50,22,
    71,104,8,51,5,27,249,207,254,21,254,162,2,82,235,219,249,254,50,102,117,113,98,249,74,92,6,48,196,254,108,208,254,154,107,83,79,99,2,254,142,255,255,0,118,130,171,32,169,130,171,42,
    38,2,8,0,0,0,7,1,194,4,50,130,7,109,209,5,72,43,7,34,77,178,3,72,43,5,32,3,72,43,9,80,49,20,35,178,7,4,6,86,248,6,32,19,65,78,9,74,3,12,132,178,32,35,130,174,131,176,137,177,42,4,40,
    255,210,254,31,243,242,140,210,111,136,175,41,175,208,4,141,254,108,95,171,254,212,134,174,46,0,0,1,0,60,255,240,4,48,4,157,0,29,0,135,76,217,12,75,223,12,43,26,47,27,177,26,15,62,
    89,178,0,26,18,130,143,33,178,3,136,140,35,178,9,18,26,79,204,5,108,36,8,32,113,108,26,7,108,65,6,108,50,8,36,178,95,9,1,114,24,120,95,11,32,18,24,65,209,11,33,178,14,132,73,130,207,
    76,186,5,40,33,53,33,2,35,34,6,7,35,24,166,143,11,8,66,36,39,1,47,13,124,120,130,128,10,254,127,1,128,22,251,114,125,12,243,20,1,4,214,226,1,23,12,1,123,234,155,220,254,248,15,1,132,
    112,98,159,148,196,1,49,105,112,194,218,254,232,240,117,169,255,136,218,186,0,0,2,0,118,255,240,6,65,130,235,40,19,0,33,0,175,178,4,34,35,131,185,32,4,24,67,99,9,80,187,12,77,218,20,
    107,23,12,35,178,13,8,11,67,202,5,8,32,180,175,13,191,13,2,93,180,111,13,127,13,2,113,178,255,13,1,113,178,15,13,1,114,180,143,13,159,13,2,114,178,95,130,11,33,178,207,131,21,32,63,
    130,4,35,180,31,13,47,130,45,131,16,65,14,12,35,16,16,178,23,65,88,8,33,176,3,113,16,13,37,1,16,0,35,34,0,24,116,0,7,33,54,0,106,103,17,48,6,65,254,223,237,222,254,226,19,188,242,242,
    188,20,1,29,220,106,109,18,42,16,226,254,30,4,141,254,24,233,1,15,106,112,14,38,2,0,67,0,0,4,18,67,101,6,34,90,178,6,67,101,5,33,6,16,24,89,255,8,67,101,12,88,138,8,40,17,9,7,17,18,
    57,176,17,47,82,100,11,34,1,10,17,88,48,9,32,7,74,34,13,8,67,51,1,38,53,52,54,51,33,17,35,17,35,3,19,20,22,51,51,17,35,34,6,67,1,22,214,240,211,1,204,243,241,230,46,97,107,221,221,
    97,107,2,10,86,209,163,185,251,115,1,188,254,68,3,34,74,89,1,74,87,0,0,1,0,10,0,0,3,255,130,165,38,13,0,80,178,1,14,15,103,247,21,43,2,47,27,177,2,15,62,89,178,7,2,8,67,63,6,32,4,80,
    31,9,35,1,208,176,8,66,121,11,38,176,7,16,176,12,208,48,105,186,5,63,35,53,51,17,33,21,33,17,51,2,167,214,243,212,212,3,33,253,210,214,1,230,254,26,1,230,170,1,253,196,254,199,131,
    131,36,26,254,175,6,109,130,131,36,25,0,164,178,8,80,199,6,69,11,6,74,164,12,70,98,7,36,176,0,69,88,176,24,68,232,12,106,89,8,34,23,9,17,131,160,38,23,47,178,63,23,1,113,115,66,9,115,
    107,7,115,90,6,32,7,66,23,8,35,178,0,7,23,86,174,5,72,79,10,131,193,37,11,208,178,15,23,7,132,76,43,16,176,18,208,176,17,16,176,20,208,176,24,131,215,38,19,51,17,35,17,35,3,74,174,
    17,38,4,193,238,190,208,171,253,74,176,18,40,2,93,254,101,253,237,1,81,1,74,178,18,47,0,1,0,118,254,175,4,124,4,141,0,16,0,136,178,0,72,137,6,32,4,71,113,5,36,12,47,27,177,12,82,146,
    12,135,12,65,1,12,104,166,8,34,13,9,12,67,68,5,36,178,63,13,1,113,67,42,9,67,83,7,67,66,6,71,165,9,35,178,0,8,13,131,49,33,6,16,65,1,10,35,48,49,1,1,132,229,32,1,73,81,8,40,2,147,1,
    33,200,208,155,254,194,73,85,7,35,2,82,254,112,135,211,73,87,5,71,71,5,33,4,254,130,201,38,20,0,128,178,5,21,22,66,79,8,32,20,130,198,32,20,135,185,69,156,12,32,17,130,25,32,17,78,
    238,7,92,22,9,33,17,20,79,249,5,79,242,5,40,95,0,1,114,178,207,0,1,113,80,8,6,42,180,143,0,159,0,2,114,176,4,208,176,116,247,13,37,12,208,178,8,12,0,74,175,6,35,53,51,21,51,91,191,
    5,35,35,21,35,53,132,201,8,37,1,105,71,163,55,1,56,1,28,254,114,1,174,254,209,254,194,62,163,71,243,243,2,173,222,222,1,224,253,196,253,175,1,213,203,203,254,43,106,63,5,36,36,0,0,
    5,78,130,10,36,14,0,133,178,9,111,239,10,67,185,12,73,4,12,74,207,12,24,90,233,8,34,8,2,7,130,144,42,176,8,47,178,63,8,1,113,178,95,8,131,203,130,9,43,180,175,8,191,8,2,93,180,143,
    8,159,8,74,207,12,33,176,7,79,236,11,24,132,103,23,39,2,225,106,243,254,160,2,83,74,226,14,36,3,202,195,254,32,133,204,51,0,2,0,79,255,235,5,152,4,165,0,35,0,46,0,140,178,21,47,48,
    131,137,33,21,16,25,52,1,8,36,27,47,27,177,27,78,166,12,87,102,20,87,247,9,33,4,27,24,127,221,37,77,19,9,42,176,2,16,176,38,208,176,27,16,178,44,67,118,8,56,48,49,5,34,39,6,35,32,0,
    3,53,52,0,51,21,34,6,21,21,20,22,51,51,55,38,130,15,45,18,51,50,18,23,21,16,7,22,51,1,16,23,54,99,129,5,8,86,17,5,152,227,174,145,169,254,218,254,172,4,1,8,219,113,127,203,192,27,27,
    192,2,220,191,198,221,1,163,95,92,253,148,190,162,1,83,91,179,16,57,62,1,60,1,24,58,254,1,46,204,180,177,38,203,205,2,170,1,30,44,234,1,13,254,252,236,72,254,255,173,11,1,210,254,244,
    111,120,243,53,160,144,254,210,255,255,0,106,251,7,60,38,1,210,0,0,0,7,1,222,0,59,254,213,0,1,0,21,254,175,4,139,4,141,0,15,0,90,178,10,73,157,6,32,7,67,143,5,92,20,12,75,151,12,32,
    11,114,180,6,68,158,12,38,178,0,15,11,17,18,57,81,187,10,35,178,10,11,15,66,159,5,33,19,33,67,100,6,33,3,3,130,9,33,33,2,107,176,6,35,9,196,207,146,107,177,15,37,254,119,253,237,1,
    81,107,181,10,36,36,254,175,6,46,132,161,34,92,178,9,134,161,32,2,24,80,126,10,85,254,12,65,210,15,90,234,11,32,8,24,79,236,11,36,176,10,208,176,11,93,139,7,33,13,208,75,189,8,8,37,
    33,53,33,21,33,17,33,17,51,5,106,196,20,222,252,68,254,164,3,162,254,172,2,6,242,195,253,236,1,81,3,201,196,196,252,250,3,202,107,235,6,33,4,22,130,151,36,23,0,79,178,4,24,108,19,10,
    68,198,12,36,22,47,27,177,22,75,84,16,41,16,1,12,17,18,57,176,16,47,178,69,158,9,39,176,4,208,176,16,16,176,19,130,138,41,33,35,17,6,7,21,35,53,38,38,75,213,5,8,45,23,53,51,21,54,55,
    17,51,4,22,243,76,86,163,204,207,2,243,84,86,163,74,88,243,1,170,22,10,204,200,13,209,191,1,106,254,159,107,105,12,243,242,9,24,2,31,131,155,36,118,0,0,4,75,130,155,76,101,15,65,210,
    12,84,136,12,71,150,8,34,4,16,1,131,155,35,4,47,178,13,66,195,10,38,19,51,17,54,51,50,22,24,203,101,10,8,47,118,243,134,128,237,239,243,117,116,129,133,243,4,141,254,86,38,214,209,
    254,158,1,97,124,105,38,253,224,0,2,0,10,255,240,5,168,4,163,0,27,0,35,0,100,178,13,36,37,131,84,35,13,16,176,29,75,121,6,81,134,12,67,115,8,34,32,14,0,131,39,32,32,24,127,125,12,41,
    3,208,176,32,16,176,10,208,176,0,89,227,12,32,14,24,74,212,13,8,72,5,32,0,39,38,38,53,51,20,22,23,62,2,51,32,0,17,21,33,18,33,50,55,55,23,6,6,3,34,6,7,33,53,52,38,3,201,254,250,254,
    192,12,174,191,193,84,88,9,143,241,145,1,0,1,23,252,192,18,1,79,134,115,47,65,59,197,161,128,160,8,2,76,149,130,255,61,234,11,221,187,93,118,12,146,228,126,254,229,254,247,149,254,
    208,43,18,186,33,44,3,238,165,140,22,134,149,0,130,221,36,79,255,240,4,129,130,221,36,22,0,30,0,94,24,237,105,23,80,127,12,24,123,163,59,37,23,21,20,6,6,35,130,210,8,75,53,33,38,38,
    35,34,7,7,39,54,54,19,50,54,55,33,21,20,22,2,57,1,11,1,59,2,140,249,150,254,254,254,235,3,63,7,179,166,134,118,45,65,64,201,152,129,158,10,253,180,148,4,163,254,220,249,122,155,249,
    136,1,28,1,8,149,150,154,44,17,186,34,43,252,18,163,142,132,201,46,1,0,66,255,236,3,232,4,141,0,25,0,105,178,18,89,89,23,67,239,8,32,2,113,104,11,42,178,4,2,0,17,18,57,178,25,11,2,
    130,6,39,176,25,47,176,5,208,178,15,133,12,116,244,13,35,25,16,178,24,73,69,8,37,48,49,1,33,53,33,24,123,157,9,34,53,51,22,82,16,5,8,46,35,35,53,2,141,253,222,3,82,1,254,198,162,194,
    255,0,223,208,247,243,4,113,101,115,115,241,125,3,201,196,155,254,192,20,191,139,168,192,185,161,73,80,90,83,176,187,0,95,35,9,38,14,0,21,0,28,0,126,117,145,10,35,15,208,176,3,95,35,
    9,75,144,21,130,162,124,90,10,34,19,11,3,130,196,47,124,176,19,47,24,180,96,19,112,19,2,93,180,48,19,64,130,6,41,178,240,19,1,93,178,0,19,1,113,130,82,24,79,9,11,34,19,16,178,88,115,
    12,95,39,13,35,1,34,6,7,24,159,27,9,117,201,13,48,253,240,121,148,14,2,54,14,147,120,121,145,14,253,204,15,149,117,209,18,43,1,127,157,149,149,157,252,219,157,147,147,157,98,139,11,
    36,39,0,174,178,37,83,229,10,36,29,47,27,177,29,92,105,16,34,6,29,12,130,205,61,176,6,47,178,15,6,1,93,176,1,208,176,1,47,178,207,1,1,93,64,9,31,1,47,1,63,1,79,1,4,130,212,130,16,33,
    178,2,97,153,9,35,6,16,178,7,108,84,12,93,253,9,54,176,14,208,176,15,208,176,7,16,176,17,208,176,6,16,176,19,208,176,2,16,176,22,130,87,40,16,176,24,208,176,29,16,178,36,68,45,8,35,
    178,33,36,1,130,125,44,178,12,33,1,93,48,49,1,33,21,33,23,21,130,4,32,6,98,210,6,36,55,35,53,51,53,98,213,17,44,1,196,1,131,254,130,3,1,123,254,115,18,38,98,218,5,8,60,52,18,150,161,
    3,158,153,1,6,216,191,196,215,243,84,83,77,87,5,2,186,146,66,22,147,69,53,195,195,14,108,147,14,74,146,39,206,238,208,182,90,103,126,121,0,0,1,0,70,255,240,3,176,4,158,0,34,0,160,178,
    10,24,118,239,10,69,132,12,68,232,8,34,34,22,9,130,165,48,176,34,47,178,15,34,1,93,180,16,34,32,34,2,93,178,0,98,182,12,80,253,10,41,0,16,176,12,208,176,34,16,176,14,132,5,49,19,208,
    176,19,47,178,207,19,1,93,182,31,19,47,19,63,19,3,130,59,130,13,32,178,24,99,3,27,38,29,208,176,16,16,176,31,24,99,3,12,38,34,36,39,35,53,51,53,130,3,43,54,54,51,50,23,7,38,35,34,7,
    33,21,130,1,8,76,3,78,254,131,17,123,111,80,121,27,118,110,212,254,255,26,151,146,146,152,26,255,211,108,122,22,91,117,214,34,1,124,254,125,1,131,1,132,106,104,28,191,31,208,196,146,
    92,147,195,214,32,191,28,214,147,92,0,0,4,0,118,0,0,7,199,4,158,0,3,0,15,0,29,0,39,0,170,24,92,73,35,66,69,7,36,36,47,27,177,36,74,192,20,32,33,24,92,73,34,40,182,0,2,16,2,32,2,3,93,
    110,181,10,32,176,111,95,12,33,176,6,108,143,11,24,92,77,15,35,37,33,53,33,24,92,77,17,73,27,5,36,6,21,1,35,1,90,155,6,59,7,136,253,197,2,59,253,138,191,1,54,192,190,254,202,193,175,
    90,83,80,88,2,93,79,78,93,254,166,121,190,7,56,200,149,1,242,150,185,184,156,72,150,184,184,155,5,87,101,98,84,83,87,100,99,91,252,180,121,215,13,46,40,0,0,4,170,4,141,0,21,0,30,0,
    140,178,13,105,189,5,35,13,16,176,23,70,171,6,71,209,12,97,47,8,33,6,3,67,99,7,32,5,66,250,8,33,176,1,24,96,79,9,24,67,86,8,49,182,143,10,159,10,175,10,3,93,180,31,10,47,10,2,113,178,
    9,137,47,32,19,132,47,41,20,208,176,10,16,176,22,208,176,12,79,186,13,33,37,33,24,96,99,12,36,22,16,6,7,33,120,26,11,36,246,254,245,243,208,130,0,8,51,1,235,209,246,237,200,254,246,
    1,11,254,245,248,97,115,117,94,249,153,153,153,182,77,183,2,58,211,254,180,205,5,77,1,4,103,85,86,101,0,2,0,124,255,236,4,70,6,0,0,15,0,26,25,0,107,36,89,5,12,48,6,47,27,177,6,15,62,
    89,178,5,12,3,17,18,57,178,10,132,6,131,179,82,98,10,103,228,12,40,48,49,1,20,2,35,34,39,7,25,41,159,19,8,50,4,70,243,199,192,109,17,210,243,105,178,204,240,243,139,123,154,68,71,153,
    122,138,2,17,244,254,207,142,122,6,0,253,210,124,254,214,254,250,8,166,187,133,254,55,135,188,0,0,1,0,80,130,189,40,0,4,78,0,29,0,75,178,23,95,75,10,24,82,32,12,86,38,8,84,70,9,35,
    178,3,8,16,131,176,32,20,98,75,5,81,45,12,24,182,50,11,37,53,53,52,54,54,51,127,83,6,75,168,5,8,51,2,66,90,122,6,228,4,122,202,116,230,254,242,122,225,152,195,244,6,228,7,120,92,121,
    133,133,174,105,79,102,176,100,1,43,254,25,158,251,135,228,180,95,118,179,178,27,173,176,0,2,0,79,130,165,42,23,6,0,0,17,0,28,0,100,178,26,98,237,5,32,26,108,11,5,75,89,6,24,82,222,
    12,79,209,7,75,76,13,34,6,4,13,131,178,32,11,132,6,32,176,93,174,12,33,176,4,95,207,11,34,48,49,19,132,181,44,23,17,51,17,35,39,6,35,34,38,38,53,55,24,74,196,10,8,60,112,205,130,172,
    106,243,211,17,108,187,126,203,116,243,141,123,148,70,70,146,125,141,2,38,159,253,140,119,2,41,250,0,117,137,140,253,155,1,157,194,129,1,215,125,193,0,255,255,0,91,0,0,2,178,5,181,
    0,6,0,21,179,123,187,5,46,236,4,85,4,78,0,15,0,25,0,67,178,4,26,27,130,151,130,130,67,29,8,140,204,88,217,8,124,53,9,130,43,82,157,10,135,174,36,0,21,21,20,6,130,173,8,62,0,53,23,20,
    22,50,54,53,52,38,34,6,76,130,235,150,230,1,32,127,237,152,230,254,225,242,149,252,147,151,248,149,2,39,159,253,139,254,205,252,13,157,252,141,1,49,254,9,160,196,196,181,159,197,198,
    0,2,0,124,254,96,4,68,130,151,40,16,0,27,0,110,178,25,28,29,131,151,33,25,16,24,173,77,8,36,13,47,27,177,13,110,69,7,24,95,82,12,32,7,130,25,33,7,17,111,179,6,86,118,8,34,6,13,4,130,
    65,33,178,11,132,6,33,176,13,95,46,12,32,4,71,235,14,132,186,8,69,39,17,35,17,51,23,54,51,50,18,23,7,52,38,35,34,7,17,22,51,50,54,4,68,111,200,129,177,108,243,217,14,108,186,193,239,
    10,241,145,124,146,68,69,147,120,147,2,17,158,253,138,116,254,0,5,218,113,133,254,235,236,39,159,194,120,254,23,120,195,0,130,201,32,79,130,201,32,22,134,201,32,107,138,201,32,4,124,
    103,11,24,77,31,20,32,9,130,201,32,9,135,201,82,26,8,66,59,12,33,178,20,85,255,12,72,178,11,24,77,7,9,38,35,17,6,35,34,2,39,66,55,11,8,44,111,205,134,183,107,17,210,243,106,170,190,
    246,11,242,147,120,144,70,72,140,126,143,2,38,162,252,138,130,110,250,38,1,252,112,1,28,226,39,158,197,118,1,244,115,198,131,197,36,83,255,236,4,11,130,197,36,22,0,30,0,124,75,19,18,
    109,173,12,75,241,8,58,27,8,0,17,18,57,176,27,47,180,191,27,207,27,2,93,180,95,27,111,27,2,113,180,31,27,47,130,6,40,178,143,27,1,93,180,239,27,255,131,11,32,12,74,66,8,81,9,14,32,
    8,84,254,11,34,48,49,5,24,122,154,8,40,18,21,21,33,22,22,51,50,54,76,5,11,8,53,2,118,242,254,207,125,226,139,221,241,253,62,15,169,141,85,146,49,58,63,189,167,102,124,16,1,208,115,
    20,1,40,247,33,158,249,139,254,244,247,123,133,157,47,32,166,50,57,3,159,141,124,26,112,127,131,225,36,81,254,86,4,4,130,225,38,25,0,36,0,131,178,34,116,171,5,33,34,16,25,43,65,130,
    32,0,24,78,198,13,32,2,65,200,10,8,77,81,231,195,189,107,17,208,254,250,237,87,175,55,53,117,131,142,130,106,174,190,234,242,129,115,151,67,68,148,118,128,2,38,253,1,43,134,114,252,
    16,242,254,254,46,33,176,63,150,148,34,118,1,47,246,168,183,133,1,209,127,181,0,0,1,0,107,255,235,5,38,5,197,0,29,0,64,178,12,69,117,10,36,12,47,27,177,12,24,91,139,15,70,13,17,32,
    26,66,126,8,32,48,25,63,31,21,8,66,2,21,21,20,18,51,50,54,55,5,36,23,254,210,249,182,254,220,160,1,158,1,32,183,251,1,52,23,253,22,163,144,172,204,210,172,145,155,22,1,218,233,254,
    250,180,1,69,210,60,213,1,74,180,254,243,233,152,146,254,230,239,52,235,254,228,143,150,138,165,38,32,0,85,178,12,33,34,83,141,8,152,165,68,160,10,100,61,12,33,178,32,70,214,5,33,32,
    47,107,10,12,53,37,6,4,35,34,36,2,39,53,52,18,36,51,50,4,23,35,2,33,34,2,7,134,185,8,67,17,33,53,33,5,38,70,254,220,176,192,254,206,173,2,159,1,35,183,248,1,43,31,249,46,254,233,170,
    211,3,232,188,100,155,31,254,221,2,31,188,95,114,178,1,72,209,49,217,1,79,182,240,227,1,7,254,229,233,51,236,254,223,48,36,1,27,192,0,114,69,5,43,5,23,5,176,0,11,0,21,0,70,178,3,87,
    71,5,32,3,24,64,111,9,36,1,47,27,177,1,24,126,107,16,32,1,97,253,12,32,0,90,163,11,8,64,48,49,51,17,33,50,4,18,23,21,20,2,4,7,3,17,51,50,18,53,53,52,2,35,155,1,190,200,1,65,178,3,176,
    254,192,204,196,174,220,248,241,218,5,176,177,254,195,200,56,204,254,191,178,3,4,228,251,230,1,14,240,38,234,1,12,131,149,42,107,255,235,5,114,5,197,0,17,0,32,130,149,101,207,9,79,
    201,7,32,13,130,149,32,13,135,149,125,201,8,70,177,16,32,29,66,7,11,130,142,65,77,11,34,18,23,7,130,149,38,34,2,21,21,20,22,22,130,164,8,64,55,5,114,166,254,216,180,178,254,216,170,
    1,165,1,42,180,178,1,38,168,4,251,220,173,169,223,102,182,110,164,216,10,2,195,206,254,176,186,186,1,78,201,49,203,1,77,192,183,254,185,198,18,228,1,34,254,219,232,37,147,241,134,1,
    9,218,133,183,32,3,132,183,34,20,0,35,130,183,32,8,80,129,5,35,8,16,176,32,69,85,6,32,16,130,183,32,16,135,183,99,99,8,33,16,16,102,87,11,35,8,16,178,32,141,183,36,7,23,7,37,6,136,
    187,33,32,4,143,186,56,53,5,114,151,137,239,165,254,213,67,62,179,254,218,170,2,167,1,40,1,104,1,39,168,1,130,190,8,48,170,222,102,181,111,174,217,2,198,202,254,189,98,192,148,245,
    13,183,1,77,203,46,208,1,82,187,183,254,175,206,5,236,1,31,254,221,239,29,151,242,132,1,32,245,0,0,1,0,151,130,198,38,239,4,140,0,6,0,50,103,58,5,96,81,12,69,66,8,40,4,0,5,17,18,57,
    176,4,47,91,93,10,53,48,49,33,35,17,5,53,37,51,2,239,243,254,155,2,57,31,3,105,122,205,208,130,83,44,110,0,0,4,44,4,158,0,25,0,89,178,9,79,239,10,88,126,12,136,90,32,24,136,252,35,
    178,2,24,0,130,101,35,178,3,0,17,130,6,34,176,17,16,74,197,10,33,178,12,132,20,34,178,23,17,86,142,5,37,33,33,53,1,54,54,106,14,6,8,56,35,52,54,54,51,50,22,21,20,6,7,1,33,4,44,252,
    96,1,251,70,57,105,90,103,123,243,121,215,133,202,234,87,110,254,177,2,73,159,1,186,63,99,64,72,90,120,96,115,188,106,183,156,90,159,102,254,214,82,199,6,42,3,151,5,196,0,7,0,50,178,
    3,8,24,158,21,9,87,86,12,89,44,8,33,6,16,24,100,132,10,8,33,48,49,1,51,17,33,17,35,17,33,2,164,243,253,210,243,2,46,5,196,254,5,252,55,4,141,0,1,0,15,254,163,3,242,130,9,34,25,0,89,
    80,245,8,46,12,47,176,0,69,88,176,2,47,27,177,2,29,62,89,84,57,10,127,164,6,35,178,5,12,2,131,251,36,5,47,176,12,16,102,138,10,36,176,5,16,178,23,24,111,159,8,34,178,25,23,115,216,
    6,37,33,53,33,21,1,22,131,249,36,4,35,34,39,55,80,228,5,8,64,38,35,35,53,2,158,253,186,3,119,254,157,171,219,144,254,242,176,199,206,57,157,173,164,196,170,183,72,3,201,196,143,254,
    128,26,247,176,163,243,132,103,182,88,184,146,150,146,123,0,0,2,0,53,254,196,4,139,4,140,0,10,0,14,0,82,66,9,5,24,67,210,12,87,139,12,75,165,8,74,255,9,44,176,6,16,176,5,208,176,5,
    47,178,8,6,0,131,193,40,0,16,176,12,208,178,13,9,2,130,12,8,61,48,49,37,51,21,35,17,35,17,33,39,1,51,1,33,17,7,3,213,182,182,242,253,88,6,2,166,250,253,100,1,170,23,194,195,254,197,
    1,59,148,3,249,252,54,2,128,42,0,255,255,0,75,2,141,2,170,5,184,3,7,1,212,130,158,32,152,24,120,237,7,44,10,47,27,177,10,31,62,89,176,16,208,48,49,131,39,38,53,2,152,2,190,5,173,130,
    39,32,216,139,39,24,121,21,13,130,79,32,79,130,79,32,174,132,39,32,217,132,39,32,16,133,227,68,238,7,33,48,49,130,35,32,77,130,35,34,185,5,186,130,75,32,218,139,75,32,0,130,115,32,
    0,131,115,32,20,134,115,32,54,130,115,133,75,32,219,139,75,32,5,130,39,32,5,130,39,132,75,137,191,32,220,132,35,32,25,133,111,32,17,24,111,226,7,35,25,208,176,17,79,166,5,131,197,36,
    70,2,143,2,163,132,237,32,221,139,121,32,8,130,45,32,8,130,81,33,176,26,131,121,55,0,1,0,102,254,160,4,30,4,140,0,28,0,93,178,25,29,30,17,18,57,0,176,14,87,73,13,24,196,120,12,33,1,
    14,130,34,34,176,7,47,82,69,10,35,178,5,7,25,131,20,24,101,9,12,34,178,17,19,131,20,34,178,28,25,126,142,10,36,3,54,55,54,18,75,65,5,66,89,9,8,54,34,6,7,135,90,3,41,253,154,45,101,
    134,207,237,133,245,165,228,181,74,132,189,143,171,142,120,83,102,27,1,117,3,23,210,254,170,50,2,2,254,247,228,152,243,130,117,178,99,179,148,135,162,53,59,0,130,187,36,67,254,196,
    4,16,130,187,37,6,0,37,0,176,1,103,214,13,139,180,32,0,24,68,149,7,125,212,5,8,35,4,16,253,182,243,2,62,253,50,3,205,4,6,250,190,5,5,195,0,2,0,79,255,240,6,109,4,157,0,20,0,30,0,145,
    178,22,79,77,5,32,22,73,37,9,90,70,12,83,154,12,36,0,47,27,177,0,91,31,7,66,212,8,32,11,70,209,11,35,178,16,0,11,87,58,6,32,17,68,155,8,34,176,0,16,24,94,212,11,32,2,86,38,12,32,10,
    24,132,194,13,44,33,33,5,34,0,17,53,52,18,54,51,5,33,24,78,63,7,35,5,55,17,39,77,239,5,8,57,6,109,253,71,254,173,236,254,218,133,240,155,1,83,2,184,253,183,1,246,254,10,2,76,251,244,
    205,207,134,152,153,16,1,53,1,12,46,172,1,7,139,16,196,254,242,195,254,202,15,8,3,20,9,192,183,53,178,199,130,249,46,115,254,180,4,84,4,160,0,24,0,36,0,83,178,31,74,31,5,32,31,97,169,
    5,32,20,92,227,14,32,20,85,73,12,34,25,20,12,130,213,36,124,176,25,47,24,24,112,157,11,24,112,114,14,39,5,50,54,55,6,35,34,2,78,166,5,36,0,17,21,20,2,68,89,5,33,19,50,81,31,7,8,65,
    20,22,1,233,152,189,25,114,170,209,247,123,218,135,241,1,20,145,254,243,178,158,132,47,125,209,176,82,136,127,109,135,138,137,200,190,90,1,18,229,153,237,128,254,209,254,246,206,229,
    254,178,178,60,182,47,1,233,120,172,165,180,177,146,138,176,0,130,199,36,98,255,235,4,133,130,199,40,13,0,26,0,70,178,3,27,28,130,155,33,176,3,81,15,9,65,193,12,80,34,8,24,78,200,16,
    110,129,9,51,48,49,1,16,0,35,34,38,2,53,16,0,51,50,22,18,7,52,38,32,91,7,5,130,209,8,66,4,133,254,227,243,158,243,130,1,31,242,159,242,129,242,155,254,246,153,154,134,133,151,2,2,62,
    254,233,254,196,142,1,12,199,1,22,1,62,142,254,243,167,184,199,200,186,44,181,205,197,180,255,255,255,181,254,75,1,147,4,58,2,6,0,155,0,0,145,15,43,0,143,0,0,1,130,4,58,0,6,0,140,132,
    31,34,251,254,92,132,15,32,38,131,15,130,21,36,163,210,10,255,255,141,37,35,0,1,0,118,130,249,42,22,4,156,0,33,0,101,178,1,34,35,74,69,8,36,21,47,27,177,21,83,18,7,32,31,130,12,32,
    31,66,166,7,32,16,109,205,7,32,31,70,124,11,35,178,10,31,21,100,52,6,35,25,208,178,8,70,28,8,33,176,21,73,158,13,32,37,70,20,9,8,77,19,38,35,34,21,17,35,17,54,54,51,50,22,23,3,22,22,
    21,20,6,35,34,39,1,235,75,72,77,92,124,116,84,202,70,81,177,239,1,209,207,120,205,104,249,161,170,217,175,124,108,219,49,101,82,88,71,163,1,1,57,249,253,28,2,240,215,213,97,111,254,
    212,23,164,129,175,202,54,0,130,219,8,32,71,2,9,2,84,2,205,2,6,0,17,0,0,0,2,255,247,0,0,4,240,5,176,0,15,0,29,0,130,178,16,30,31,131,160,35,16,16,176,6,73,15,6,24,184,146,12,72,1,8,
    32,3,72,92,5,45,3,47,178,207,3,1,93,178,63,3,1,113,178,111,131,4,32,31,131,4,32,159,131,19,37,15,3,1,114,178,2,77,156,9,35,17,208,176,0,97,185,11,33,176,5,125,131,11,43,176,3,16,176,
    29,208,48,49,51,17,35,53,74,157,5,48,21,21,20,2,4,35,19,35,17,51,50,54,53,53,52,38,35,130,9,8,46,178,187,187,1,174,193,1,43,164,165,254,207,197,63,229,163,203,213,206,196,177,229,2,
    140,170,2,122,172,254,196,204,73,207,254,198,170,2,140,254,62,253,240,70,237,250,254,82,64,223,225,46,1,255,212,0,0,4,22,6,0,0,24,0,116,178,12,121,187,6,24,198,187,41,37,47,21,1,93,
    178,15,131,4,41,24,15,21,17,18,57,176,24,47,178,24,196,17,11,33,4,15,131,20,32,4,76,75,14,39,176,17,208,176,24,16,176,19,98,227,5,35,54,51,32,19,111,105,9,8,50,17,35,53,51,53,51,21,
    51,2,113,231,119,182,1,90,5,243,97,94,146,72,243,195,195,243,231,4,199,254,253,138,254,117,253,61,2,186,112,93,130,252,251,4,199,170,143,143,0,1,0,45,130,193,36,176,5,176,0,15,124,
    189,7,72,49,8,73,120,8,38,15,62,89,178,15,10,2,131,139,32,15,139,160,40,176,4,208,176,15,16,176,6,208,119,110,13,99,125,14,8,47,53,33,21,33,17,51,3,185,207,251,211,211,254,62,4,131,
    254,58,207,3,18,252,238,3,18,170,1,40,204,204,254,216,0,1,255,232,255,236,2,133,5,65,0,28,0,114,178,0,71,165,6,131,125,36,27,47,27,177,27,81,99,7,97,168,8,38,27,16,176,1,208,176,27,
    76,19,12,32,4,131,16,37,176,23,208,176,23,47,73,85,5,33,176,23,24,100,86,12,35,8,208,176,17,65,70,12,47,27,16,176,28,208,176,28,47,48,49,1,17,51,21,35,21,130,3,33,17,20,24,101,107,
    13,8,68,35,53,51,17,1,173,191,191,216,216,49,63,42,43,83,77,254,232,210,210,178,178,5,65,254,249,180,165,170,254,243,62,55,10,188,23,1,53,1,22,170,165,180,1,7,255,255,0,18,0,0,5,66,
    7,54,2,38,0,37,0,0,1,7,0,68,1,35,1,54,73,135,7,32,4,130,202,38,4,31,62,89,176,12,220,73,135,5,143,45,34,117,1,194,137,45,67,246,8,32,13,140,45,32,55,136,91,34,157,0,195,146,91,32,15,
    140,45,32,44,136,45,44,164,0,197,1,55,0,9,0,176,4,47,176,22,140,35,32,2,136,35,34,106,0,238,130,81,32,22,73,117,5,136,173,38,18,220,176,27,208,48,49,136,221,32,148,136,47,38,162,1,
    88,1,106,0,12,132,83,35,16,220,176,21,139,37,32,177,133,37,39,0,7,1,223,1,94,1,28,130,61,38,102,254,60,4,235,5,196,130,23,42,39,0,0,0,7,0,121,1,201,255,251,130,23,38,148,0,0,4,76,7,
    61,130,23,32,41,65,51,5,35,0,232,1,61,65,51,7,40,6,47,27,177,6,31,62,89,176,65,5,7,143,45,34,117,1,135,146,45,32,14,134,225,133,45,32,62,136,91,34,157,0,136,146,45,32,16,140,45,32,
    9,136,45,34,106,0,179,130,45,65,15,6,136,137,35,19,220,176,28,132,233,33,255,200,130,175,32,160,132,185,32,45,133,185,33,255,151,137,93,32,2,130,185,32,2,131,185,32,5,134,93,36,163,
    0,0,2,125,138,45,34,117,0,53,137,45,32,3,130,45,32,3,131,45,32,6,133,45,33,255,203,130,45,33,122,7,131,185,133,91,34,157,255,55,146,91,32,8,134,45,32,191,130,45,33,133,7,131,185,133,
    45,34,106,255,98,137,185,136,137,35,11,220,176,20,132,185,39,0,148,0,0,5,23,7,44,130,233,32,50,132,185,34,164,0,238,66,29,5,35,5,47,176,21,133,83,39,0,102,255,236,5,30,7,54,130,35,
    32,51,132,35,34,68,1,58,66,111,9,81,190,8,32,32,150,45,34,117,1,217,137,45,32,13,130,221,32,13,131,221,32,33,140,45,32,55,136,91,34,157,0,218,146,91,32,35,140,45,131,173,133,137,34,
    164,0,220,130,173,32,19,66,167,5,136,91,32,34,140,45,32,2,136,91,34,106,1,5,66,213,9,136,183,35,38,220,176,47,65,11,5,36,125,255,236,4,189,132,231,32,57,134,231,32,17,137,139,77,19,
    8,32,18,134,93,143,45,34,117,1,176,130,45,32,9,130,139,34,47,176,19,140,35,131,221,133,81,34,157,0,177,146,81,65,103,7,133,81,131,175,133,45,32,106,130,221,33,54,0,66,117,6,136,127,
    35,24,220,176,33,133,175,36,7,0,0,4,214,132,175,32,61,132,175,34,117,1,135,137,93,82,138,8,32,11,134,139,42,90,255,236,3,251,6,0,2,38,0,69,132,45,36,68,0,173,0,0,67,93,7,41,23,47,27,
    177,23,27,62,89,176,43,150,45,34,117,1,76,130,45,130,221,35,23,47,176,44,140,35,32,1,134,81,35,6,0,157,77,145,79,32,46,139,43,33,5,246,136,43,34,164,79,1,144,123,32,45,140,43,32,204,
    136,43,34,106,120,0,65,5,7,136,167,35,49,220,176,58,65,5,5,133,215,32,94,136,215,36,162,0,226,0,52,144,47,35,47,220,176,55,139,47,32,124,133,47,49,0,7,1,223,0,232,255,231,255,255,0,
    79,254,60,3,245,4,78,130,23,32,71,130,233,38,7,0,121,1,61,255,251,130,23,38,83,255,236,4,11,6,0,130,23,32,73,65,55,6,32,161,65,55,9,41,8,47,27,177,8,27,62,89,176,31,134,187,143,45,
    34,117,1,64,65,55,5,34,8,47,176,67,33,7,133,35,32,1,134,81,35,6,0,157,65,65,55,8,136,79,66,195,7,132,43,33,5,204,136,43,33,106,108,65,11,8,136,43,35,37,220,176,46,132,219,33,255,180,
    130,161,34,140,5,249,130,45,32,140,130,9,36,6,0,68,131,249,65,101,7,32,2,130,169,32,2,131,169,68,133,7,36,143,0,0,2,105,138,43,33,117,33,136,43,32,3,130,43,32,3,131,43,68,131,7,32,
    183,130,43,34,102,5,250,134,87,37,7,0,157,255,35,255,136,45,136,89,68,131,7,32,171,130,45,34,113,5,197,136,45,34,106,255,78,130,45,66,199,6,136,45,68,131,9,38,121,0,0,3,248,5,246,130,
    47,32,82,132,181,42,164,85,1,0,9,0,176,3,47,176,28,65,85,6,38,79,255,236,4,61,6,0,130,33,32,83,65,131,6,32,182,65,131,9,90,222,8,151,45,34,117,1,85,130,45,70,241,5,32,29,140,81,32,
    1,134,81,35,6,0,157,86,65,131,8,136,79,65,211,7,132,125,132,159,131,125,35,6,0,164,88,132,159,35,4,47,176,38,139,77,33,5,204,136,159,34,106,0,129,130,113,71,63,11,50,27,62,89,176,34,
    220,176,43,208,48,49,255,255,0,119,255,236,3,247,132,207,32,89,134,207,32,175,137,207,36,7,47,27,177,7,131,47,68,105,7,143,45,34,117,1,78,133,207,32,6,68,105,9,133,35,131,207,131,81,
    130,207,32,79,136,207,136,79,68,103,7,132,43,132,173,133,43,33,106,122,66,83,8,136,43,68,101,9,36,12,254,75,3,214,132,171,32,93,132,171,34,117,1,22,133,125,34,1,47,176,135,161,132,
    35,132,81,131,35,35,6,0,106,66,136,81,32,15,130,205,32,15,131,205,35,23,220,176,32,72,23,10,33,6,234,72,61,8,36,112,0,190,1,58,73,27,24,38,90,255,236,3,251,5,180,68,13,8,35,112,72,
    4,0,68,135,5,32,42,72,189,12,32,28,136,79,34,160,0,246,73,15,18,72,9,7,133,79,32,230,68,47,8,34,160,0,128,69,7,18,32,45,131,91,51,0,2,0,18,254,82,5,66,5,176,0,22,0,25,0,116,178,25,
    26,27,92,195,6,32,22,77,147,6,32,22,130,222,32,22,86,163,7,123,45,12,32,1,130,25,32,1,78,132,7,32,12,130,12,36,12,17,62,89,178,24,138,66,10,43,1,16,176,17,208,176,17,47,178,23,20,22,
    131,85,33,23,47,81,35,10,35,178,25,22,20,130,20,56,48,49,1,1,35,6,6,21,20,51,50,55,23,6,35,34,38,53,52,55,3,33,3,33,1,130,4,8,53,3,27,2,39,62,87,74,71,44,46,21,73,92,95,116,149,115,
    253,204,118,254,249,2,38,98,1,166,211,5,176,250,80,56,94,49,68,23,142,44,110,91,141,98,1,73,254,173,5,176,252,111,2,92,0,130,211,48,90,254,82,3,251,4,78,0,45,0,56,0,166,178,23,57,58,
    132,125,34,16,176,47,134,211,69,100,8,36,0,69,88,176,41,130,185,32,41,93,138,15,32,176,131,25,32,30,130,25,32,30,132,224,38,208,176,0,47,178,2,23,93,157,5,132,6,38,176,11,47,176,23,
    16,178,101,190,10,34,18,11,15,130,102,48,64,9,12,18,28,18,44,18,60,18,4,93,176,41,16,178,36,79,120,9,35,4,16,178,46,82,48,9,35,11,16,178,50,136,13,36,48,49,37,38,39,25,75,34,27,65,
    34,14,49,3,50,54,55,53,35,34,6,21,20,22,2,255,11,13,116,168,163,25,75,49,16,32,42,65,56,9,62,118,72,127,32,131,135,136,93,7,25,69,121,186,137,173,185,71,84,101,83,64,89,155,88,191,
    173,254,24,146,87,17,65,70,8,51,140,1,8,70,59,204,94,86,70,83,255,255,0,102,255,236,4,235,7,75,74,195,5,44,1,7,0,117,1,192,1,75,0,9,0,176,12,69,231,9,38,79,255,236,3,245,6,0,70,81,
    5,132,35,40,41,0,0,0,9,0,176,15,47,70,47,8,133,71,32,76,136,71,34,157,0,193,130,71,32,19,72,209,5,73,91,16,133,81,32,1,134,81,35,6,0,157,42,67,229,8,67,103,8,68,181,7,133,89,32,41,
    136,89,36,161,1,167,1,84,67,101,7,136,89,68,193,10,35,3,245,5,222,136,171,34,161,1,16,130,169,134,135,136,91,32,37,73,89,9,130,253,137,181,34,158,0,216,136,253,70,185,7,143,171,33,
    158,65,135,251,73,251,7,38,148,0,0,4,210,7,62,130,115,42,40,0,0,1,7,0,158,0,103,1,61,130,117,36,176,1,47,176,26,69,89,9,39,5,91,6,2,0,38,0,72,131,35,45,1,162,4,1,4,252,0,6,0,176,30,
    47,48,49,76,51,7,33,6,241,75,169,8,36,112,0,131,1,65,76,51,24,71,77,5,32,180,71,77,8,34,112,60,4,131,111,35,8,47,176,30,75,249,12,32,35,136,79,34,160,0,187,75,109,9,75,249,8,77,91,
    7,133,79,32,230,136,79,33,160,116,71,201,17,139,237,34,76,7,27,136,89,36,161,1,110,1,70,144,169,32,20,72,71,11,33,5,222,72,117,8,34,161,1,39,65,143,9,71,249,8,32,38,131,45,49,0,1,0,
    148,254,82,4,76,5,176,0,27,0,128,178,17,28,29,83,21,8,68,157,12,36,15,47,27,177,15,67,201,20,68,183,7,33,178,26,68,137,5,33,26,47,113,126,10,33,176,20,83,55,11,38,176,3,208,176,15,
    16,178,24,144,212,9,33,176,22,85,190,13,36,1,33,17,33,21,67,143,14,32,55,24,100,5,7,37,231,253,170,2,187,111,67,122,9,45,135,253,147,3,177,253,76,2,86,2,138,254,64,202,67,105,8,59,
    134,95,5,176,204,254,110,0,0,2,0,83,254,109,4,11,4,78,0,35,0,43,0,165,178,17,44,45,90,219,5,112,51,8,32,25,130,212,32,25,24,80,137,12,135,225,80,34,7,35,178,2,17,25,131,52,33,12,16,
    69,124,10,35,178,40,25,17,131,20,56,40,47,180,31,40,47,40,2,113,180,191,40,207,40,2,93,178,143,40,1,93,180,95,40,111,131,18,34,239,40,255,130,6,33,178,29,83,56,10,32,16,24,212,161,
    10,33,178,35,133,67,32,25,105,90,11,36,48,49,37,6,7,69,176,14,34,38,0,39,86,16,5,24,171,106,8,43,1,34,6,7,33,53,38,38,3,250,73,113,65,13,9,36,80,207,254,251,6,96,247,5,49,61,11,157,
    119,167,105,254,197,100,123,17,1,207,8,114,184,106,51,65,28,8,56,102,82,13,1,19,215,58,162,255,142,254,230,254,254,98,134,156,135,2,86,140,125,18,122,125,79,65,8,78,229,9,34,158,0,
    159,66,189,18,32,17,74,171,12,74,135,9,41,158,88,0,0,9,0,176,8,47,176,67,195,7,50,106,255,236,4,240,7,76,2,38,0,43,0,0,1,7,0,157,0,190,68,157,9,36,11,47,27,177,11,77,203,11,37,82,254,
    86,4,12,6,131,79,32,75,130,45,35,6,0,157,64,67,13,8,74,81,8,32,39,134,123,133,89,32,49,136,89,34,160,0,241,146,89,135,135,132,89,33,5,230,130,45,133,89,33,160,115,145,89,32,40,140,
    89,32,41,136,89,34,161,1,164,68,247,9,136,179,135,135,133,89,32,222,134,89,36,7,0,161,1,38,67,103,9,136,181,32,45,135,91,37,253,249,4,240,5,196,133,91,42,0,7,1,162,1,187,254,146,255,
    255,0,132,69,33,6,169,135,69,37,1,185,1,39,0,126,74,195,6,32,41,68,89,9,35,5,24,7,62,130,35,33,44,0,65,75,5,32,226,65,155,9,32,7,24,80,156,7,80,129,7,38,121,0,0,3,248,7,94,130,45,32,
    76,134,45,34,23,1,93,131,81,32,16,73,243,8,39,255,179,0,0,2,144,7,51,130,35,79,235,5,36,164,255,57,1,62,68,81,7,80,71,8,32,7,133,127,33,255,159,130,45,34,124,5,239,75,149,8,36,164,
    255,37,255,250,131,81,34,2,47,176,68,207,6,33,255,185,130,35,34,144,6,241,136,81,34,112,255,50,69,77,9,80,199,15,33,255,165,132,81,32,173,136,81,36,112,255,30,255,253,76,111,23,33,
    255,223,130,45,34,101,7,35,136,91,34,160,255,106,137,255,136,91,135,173,32,203,130,45,34,81,5,223,136,91,34,160,255,86,76,113,18,134,45,39,0,23,254,88,1,159,5,176,133,91,47,0,6,0,163,
    238,6,255,255,0,0,254,82,1,144,5,213,130,21,40,77,0,0,0,6,0,163,215,0,130,21,38,157,0,0,1,163,7,27,136,135,34,161,0,28,69,135,9,136,135,83,153,7,42,163,255,236,6,38,5,176,0,38,0,45,
    130,67,37,7,0,46,2,66,0,131,69,35,125,254,75,3,130,91,130,23,131,91,36,7,0,78,2,11,132,23,42,45,255,236,4,171,7,55,2,38,0,46,65,183,5,33,1,104,74,139,9,93,11,9,65,147,6,36,181,254,
    75,2,107,132,229,32,155,133,45,35,255,40,255,222,71,163,12,35,27,62,89,176,67,175,7,36,148,253,249,5,24,132,229,32,47,131,139,47,1,162,1,157,254,146,255,255,0,125,253,249,4,54,6,0,
    130,115,32,79,134,23,32,45,132,23,38,148,0,0,4,38,7,54,130,23,32,48,132,93,34,117,0,41,137,139,84,85,8,77,181,6,39,0,138,0,0,2,98,7,145,130,45,32,80,134,45,34,26,1,145,66,77,12,32,
    33,130,139,78,17,6,32,0,130,139,33,4,38,132,139,130,91,33,0,7,130,139,32,109,132,115,36,85,253,249,1,127,132,139,130,69,131,23,33,0,16,137,139,33,5,177,130,93,132,139,38,1,162,2,10,
    4,171,0,94,71,6,86,101,7,33,48,49,130,205,32,140,130,135,37,231,6,2,0,38,0,132,135,130,89,34,141,4,252,135,41,36,8,47,27,177,8,130,135,71,251,8,138,131,37,0,161,1,202,253,212,134,65,
    34,235,6,0,133,65,33,0,7,130,23,34,100,253,175,69,29,5,35,5,23,7,54,82,235,8,34,117,1,235,65,15,9,94,45,8,65,249,7,38,121,0,0,3,248,6,0,78,149,6,36,7,0,117,1,82,76,247,5,32,3,67,129,
    8,39,0,148,253,249,5,23,5,176,133,81,131,237,33,1,220,132,237,38,121,253,249,3,248,4,78,133,59,132,23,32,65,132,23,83,109,5,32,55,136,129,34,158,1,3,137,129,72,93,16,133,129,32,1,79,
    23,8,34,158,106,0,68,83,6,77,201,6,34,255,165,0,131,163,32,3,134,33,38,7,1,162,255,96,4,253,65,43,7,36,21,47,27,177,21,65,43,7,38,102,255,236,5,30,6,234,83,11,8,34,112,0,213,77,115,
    9,74,103,19,35,4,61,5,180,79,29,8,34,112,81,4,86,49,6,32,27,83,91,12,32,28,136,79,34,160,1,13,137,201,136,79,69,191,7,79,65,5,32,230,79,31,8,34,160,0,137,77,115,9,79,111,8,73,131,7,
    132,171,33,7,53,136,91,34,165,1,99,83,183,9,83,229,8,35,33,220,176,37,78,79,5,133,93,32,255,136,93,34,165,0,223,79,125,18,32,29,83,55,8,38,148,0,0,4,222,7,54,130,47,32,54,66,233,5,
    33,1,113,130,95,80,31,5,74,119,7,35,124,0,0,2,75,187,5,32,86,133,35,33,0,173,130,83,130,35,34,11,47,176,86,37,8,37,253,249,4,222,5,176,133,71,49,0,7,1,162,1,110,254,146,255,255,0,79,
    253,249,2,180,4,78,130,23,130,59,131,23,33,0,10,67,3,8,34,222,7,55,136,119,34,158,0,137,136,119,32,28,65,87,6,32,56,130,119,34,250,6,1,133,59,38,1,6,0,158,198,0,0,133,117,32,18,134,
    33,36,74,255,236,4,138,132,189,32,55,133,153,33,1,142,133,69,35,9,47,176,42,134,35,38,75,255,236,3,202,6,0,130,69,32,87,134,35,32,58,133,189,130,35,70,143,7,133,71,131,141,133,71,34,
    157,0,143,84,155,18,141,81,131,151,131,81,35,6,0,157,59,71,129,8,24,90,12,8,136,89,37,254,65,4,138,5,196,130,125,130,89,49,0,7,0,121,1,157,0,0,255,255,0,75,254,56,3,202,4,78,133,149,
    132,23,34,68,255,247,130,23,34,74,253,249,139,47,37,1,162,1,137,254,146,130,23,34,75,253,249,139,47,130,23,32,48,132,23,143,185,34,158,0,166,65,1,8,84,155,7,65,1,5,137,175,38,158,82,
    0,0,9,0,176,130,255,84,225,7,32,45,130,117,34,176,5,176,130,141,36,56,0,0,0,7,130,93,32,119,132,93,38,8,253,249,2,114,5,65,130,23,32,88,133,23,33,0,200,132,23,35,45,254,68,4,138,47,
    130,213,34,139,0,3,130,165,36,8,254,65,2,165,137,47,35,0,121,0,220,132,237,38,45,0,0,4,176,7,55,133,95,37,1,7,0,158,0,152,67,235,18,32,13,65,177,6,41,8,255,236,3,39,6,131,0,38,0,134,
    117,38,1,205,5,125,255,255,0,86,19,5,44,44,2,38,0,57,0,0,1,7,0,164,0,179,86,241,9,103,32,8,76,53,7,81,217,5,32,246,130,45,32,89,130,45,35,6,0,164,81,85,101,8,32,13,109,108,7,135,43,
    132,89,33,6,234,136,89,37,112,0,172,1,58,0,86,191,5,70,113,7,133,79,32,180,136,79,34,112,74,4,76,177,12,82,165,11,132,79,33,7,28,136,79,34,160,0,228,66,79,18,141,169,32,230,134,89,
    36,7,0,160,0,130,83,1,18,140,171,33,7,148,136,91,34,162,1,70,90,111,5,35,0,47,176,22,90,149,8,132,173,33,6,94,136,83,41,162,0,228,0,52,0,12,0,176,6,139,37,133,167,32,53,136,75,34,165,
    1,58,68,93,9,65,81,8,35,19,220,176,23,83,171,8,35,4,46,5,255,136,85,34,165,0,216,130,169,133,85,130,37,32,21,130,37,48,0,1,0,125,254,137,4,189,5,176,0,31,0,87,178,28,32,24,88,137,9,
    36,24,47,27,177,24,82,31,7,24,155,50,8,24,152,124,8,43,23,62,89,178,4,19,24,17,18,57,178,9,81,10,9,32,19,121,56,11,33,176,24,100,233,5,34,1,17,20,76,89,16,49,32,0,53,17,51,17,20,22,
    51,32,17,17,4,189,133,126,61,79,82,13,7,51,54,255,0,254,219,252,148,144,1,36,5,176,252,50,152,228,61,41,89,55,82,13,5,46,85,69,1,12,235,3,205,252,50,146,154,1,52,3,198,130,187,38,119,
    254,82,3,247,4,58,130,187,34,102,178,26,138,187,82,7,12,32,18,130,200,32,18,24,144,163,16,131,200,32,10,130,25,33,10,17,130,200,32,5,137,193,41,31,16,176,15,208,176,15,47,176,18,113,
    9,11,33,176,23,96,89,5,32,33,77,32,14,81,219,5,131,201,34,51,50,55,130,6,33,3,226,77,20,9,42,146,5,107,197,176,181,243,171,177,62,243,77,2,8,8,37,140,97,98,126,206,195,2,189,253,70,
    206,127,3,9,251,198,255,255,0,48,0,0,6,229,7,55,2,38,0,59,0,0,1,7,0,157,1,168,70,143,18,74,231,6,39,0,33,0,0,5,204,6,1,130,45,32,91,134,45,32,10,66,125,9,32,11,130,207,32,11,73,119,
    11,89,103,5,131,91,89,103,5,34,157,0,136,89,103,26,36,12,254,75,3,214,132,91,85,11,5,33,157,23,81,163,17,66,215,7,133,89,32,2,130,135,133,89,34,106,0,179,66,139,9,72,145,8,35,16,220,
    176,25,66,139,5,38,80,0,0,4,140,7,54,130,47,32,62,132,183,34,117,1,131,137,137,76,67,8,72,193,7,38,82,0,0,3,192,6,0,130,45,32,94,134,45,32,27,67,99,18,135,45,133,91,32,20,136,91,36,
    161,1,106,1,63,67,237,7,136,91,70,133,7,132,91,33,5,222,136,91,34,161,1,2,77,55,9,86,69,8,135,45,133,91,32,55,136,91,34,158,0,155,69,177,5,35,7,47,176,14,69,1,6,132,81,33,6,1,134,81,
    36,6,0,158,51,0,130,81,32,176,137,33,39,255,246,0,0,7,87,7,66,130,33,32,129,133,207,35,2,187,1,66,68,143,12,34,31,62,89,92,181,8,36,72,255,236,6,132,132,79,32,134,134,45,34,113,0,1,
    86,57,6,32,63,134,115,42,105,255,161,5,34,7,128,2,38,0,131,133,35,35,1,224,1,128,135,81,68,11,8,70,83,7,38,79,255,119,4,61,5,254,130,45,32,137,134,45,34,48,255,254,86,183,12,32,27,
    130,127,78,135,6,43,255,166,0,0,4,42,4,141,2,38,1,189,131,45,47,1,222,255,22,255,110,0,70,0,178,31,23,1,113,178,111,131,4,24,119,102,8,41,182,175,23,191,23,207,23,3,114,178,130,18,
    38,114,178,95,23,1,114,182,131,16,37,223,23,3,113,178,63,130,42,42,180,223,23,239,23,2,93,180,31,23,47,130,6,35,48,49,255,255,223,95,33,0,36,130,191,32,22,132,191,32,205,130,191,44,
    6,1,222,50,190,0,8,0,178,0,11,1,93,131,127,33,0,9,130,31,34,148,6,30,130,223,32,186,130,31,38,7,0,68,0,199,0,30,65,13,12,32,29,96,225,10,143,45,34,117,1,102,137,45,109,53,8,71,13,7,
    133,45,32,31,134,91,35,6,0,157,103,145,89,67,203,7,133,43,32,20,136,43,34,164,105,31,96,221,14,132,33,33,5,234,136,169,34,106,0,146,130,123,89,157,11,131,169,96,221,9,132,47,33,6,124,
    136,47,37,162,0,252,0,82,0,143,47,35,16,220,176,24,67,149,5,133,47,32,153,133,47,49,0,7,1,223,1,2,0,4,255,255,0,79,254,65,4,67,4,157,130,23,52,188,0,0,0,7,0,121,1,107,0,0,255,255,0,
    118,0,0,3,181,6,30,130,23,32,190,65,57,6,32,150,65,11,9,109,56,8,65,11,7,143,45,34,117,1,53,137,45,24,67,254,8,67,69,7,133,45,32,31,134,91,35,6,0,157,54,65,57,8,136,89,74,191,7,132,
    43,33,5,234,136,43,34,106,97,30,135,229,136,43,96,227,9,32,166,130,171,32,126,132,181,32,194,133,181,33,255,117,137,135,32,2,109,147,6,33,176,5,67,89,6,36,131,0,0,2,91,136,45,35,6,
    0,117,19,136,135,32,3,130,43,33,3,29,78,75,9,33,255,169,130,43,33,88,6,131,179,133,89,34,157,255,21,146,89,78,167,6,33,255,157,130,45,32,99,132,181,133,45,34,106,255,64,65,205,9,136,
    135,92,93,9,38,118,0,0,4,103,6,20,130,229,32,199,132,183,40,164,0,136,0,31,0,9,0,176,96,225,10,36,79,255,240,4,111,132,173,32,200,132,35,34,68,0,213,137,129,107,1,8,76,159,7,143,45,
    34,117,1,116,130,45,75,165,5,92,51,9,131,81,131,211,131,81,35,6,0,157,117,136,255,136,79,32,33,86,227,8,131,43,131,161,133,43,33,164,119,132,159,34,11,47,176,68,75,6,32,0,132,113,132,
    243,133,159,34,106,0,160,137,243,136,79,35,36,220,176,45,66,145,5,32,103,130,207,32,30,132,207,32,206,134,207,32,181,137,207,37,8,47,27,177,8,29,80,87,10,143,45,34,117,1,84,137,45,
    24,68,86,8,69,203,7,133,45,131,217,32,206,130,91,130,217,32,85,136,217,136,89,70,223,7,132,43,132,183,133,135,34,106,0,128,137,183,136,45,92,25,9,36,5,0,0,4,54,132,183,32,210,77,117,
    5,33,1,45,137,137,24,67,26,8,96,209,7,32,9,130,45,34,148,5,210,67,249,8,34,112,98,34,68,127,30,32,4,67,211,8,34,160,0,154,137,89,39,4,47,27,177,4,29,62,89,70,139,5,47,0,2,0,9,254,82,
    4,148,4,141,0,22,0,25,0,113,91,245,18,24,119,101,12,87,61,7,36,176,0,69,88,176,91,245,37,35,178,23,20,0,91,242,18,32,0,91,242,23,44,39,33,7,35,1,3,33,3,2,191,1,213,54,73,27,9,47,157,
    89,254,30,95,245,1,215,60,1,84,170,4,141,251,115,73,32,8,52,146,97,235,249,4,141,253,37,1,186,0,255,255,0,79,255,240,4,67,6,30,68,135,5,37,1,7,0,117,1,99,66,175,8,66,211,11,34,67,6,
    31,134,35,35,6,0,157,100,66,175,17,32,32,66,175,10,34,67,5,252,136,79,36,161,1,74,0,39,65,121,7,66,141,10,102,219,5,133,125,137,89,34,158,123,30,78,209,6,135,79,36,106,0,0,4,42,132,
    123,39,189,0,0,1,6,0,158,248,132,33,35,1,47,176,24,134,113,68,169,5,32,210,68,169,8,33,112,49,65,233,8,69,47,22,32,4,136,43,33,160,105,136,201,136,43,70,59,7,133,87,131,201,69,137,
    5,34,161,1,28,137,201,136,45,32,20,89,101,6,38,118,254,82,3,181,4,141,89,101,21,24,75,22,12,89,101,29,34,27,22,4,121,81,5,114,193,10,89,101,75,37,95,254,10,2,76,94,66,38,9,35,135,253,
    251,3,24,125,119,9,66,36,8,38,134,95,4,141,196,254,242,70,145,9,70,53,9,33,158,77,65,51,17,78,19,7,37,84,255,240,4,72,6,131,43,39,192,0,0,1,6,0,157,104,136,43,40,10,47,27,177,10,29,
    62,89,176,68,217,7,133,43,35,4,2,38,1,131,43,36,7,0,160,0,155,67,117,9,136,45,66,7,7,132,45,33,5,252,136,45,34,161,1,78,65,141,9,136,45,92,131,7,38,84,253,249,4,72,4,157,133,45,39,
    0,7,1,162,1,106,254,146,71,93,5,33,4,104,132,159,32,193,133,159,32,123,136,159,71,45,8,32,16,66,87,5,39,255,145,0,0,2,110,6,20,130,67,70,75,5,34,164,255,23,70,27,5,87,19,10,32,151,
    131,35,33,5,210,136,35,35,112,255,16,0,66,125,8,70,247,15,33,255,189,130,45,33,67,6,131,241,133,81,34,160,255,72,137,241,136,45,86,183,7,38,21,254,82,1,141,4,141,133,91,37,0,6,0,163,
    236,0,130,193,32,124,130,183,34,130,5,252,133,21,36,1,6,0,161,251,67,93,8,136,65,75,181,7,36,36,255,240,4,55,132,237,32,195,130,53,36,7,0,157,0,244,137,111,68,166,8,32,19,133,239,37,
    0,118,253,249,4,104,132,111,40,196,0,0,0,7,1,162,1,18,65,51,7,32,3,73,203,5,32,197,130,69,35,6,0,117,10,65,51,8,73,155,8,71,127,6,131,67,33,3,148,132,67,130,43,132,67,85,225,5,38,118,
    0,0,3,148,4,144,130,181,130,23,32,1,131,91,34,149,3,138,85,225,12,36,29,62,89,48,49,65,161,5,139,65,37,0,161,1,114,253,70,133,23,35,4,103,6,30,71,169,8,34,117,1,133,70,181,18,135,249,
    32,118,130,203,32,103,132,135,32,199,134,203,32,120,65,255,8,34,103,6,31,136,69,34,158,0,157,137,69,68,1,16,71,89,5,32,210,130,45,32,200,132,249,34,112,112,34,68,157,6,32,29,71,157,
    12,32,4,134,33,36,7,0,160,0,168,72,27,18,69,27,11,34,111,6,29,136,45,34,165,0,254,130,45,38,12,0,176,11,47,176,31,84,129,8,36,118,0,0,4,57,132,233,32,203,130,117,36,7,0,117,1,23,69,
    145,5,35,4,47,176,25,65,171,10,32,57,132,223,130,35,37,0,7,1,162,1,24,136,223,32,57,132,223,130,23,37,1,6,0,158,47,30,85,155,14,36,62,255,240,3,239,132,93,32,204,134,93,32,65,133,93,
    32,9,72,81,9,133,35,32,31,130,167,131,35,35,6,0,157,66,65,241,8,118,185,8,83,251,7,38,62,254,65,3,239,4,157,133,43,42,0,7,0,121,1,79,0,0,255,255,0,143,67,33,158,89,132,137,84,135,10,
    35,36,253,249,4,76,67,8,132,195,32,37,132,195,32,36,130,255,32,22,132,195,32,205,130,161,130,195,32,71,68,111,17,32,13,65,7,6,34,36,254,71,139,67,130,125,34,57,0,6,130,125,38,103,255,
    240,4,30,6,20,130,149,32,206,132,67,34,164,87,31,70,131,7,72,179,8,66,247,7,132,43,33,5,210,136,43,35,112,80,34,0,82,233,5,68,9,6,32,0,132,33,33,6,4,134,33,36,7,0,160,0,136,65,229,
    9,72,169,8,140,79,33,6,124,136,45,47,162,0,234,0,82,0,12,0,176,0,47,176,21,220,176,26,73,133,9,34,52,6,29,136,37,34,165,0,222,66,11,5,130,37,32,18,107,129,5,41,0,1,0,103,254,130,4,
    30,4,141,130,25,36,97,178,27,31,32,95,187,8,36,23,47,27,177,23,70,85,7,72,112,12,32,13,130,25,40,13,23,62,89,176,0,69,88,176,81,151,7,35,178,4,18,0,130,59,34,176,13,16,114,205,11,32,
    18,113,216,11,38,48,49,1,17,6,6,7,100,77,13,34,38,38,39,82,66,5,40,50,55,17,4,30,1,125,119,127,82,66,7,49,64,205,242,2,241,126,108,229,4,4,141,252,252,129,189,50,86,90,82,64,5,55,93,
    73,6,214,187,3,5,253,0,115,104,212,3,7,255,255,0,40,0,0,5,229,6,31,130,231,33,208,0,81,123,5,32,25,73,181,18,71,115,7,73,227,5,131,45,32,210,130,45,35,6,0,157,46,65,251,8,65,103,8,
    65,251,7,132,43,33,5,234,130,89,133,43,33,106,89,76,161,8,136,43,81,29,9,38,65,0,0,3,243,6,30,130,45,32,211,132,135,34,117,1,48,65,195,18,68,63,7,132,45,33,5,252,136,45,34,161,1,23,
    70,109,9,70,41,8,74,245,7,132,45,132,227,131,91,35,6,0,158,72,70,85,17,111,113,12,33,6,65,102,143,5,60,0,6,0,173,191,0,255,255,255,74,0,0,4,176,6,65,0,38,0,41,100,0,0,7,0,173,254,132,
    0,131,23,36,83,0,0,5,124,132,23,32,44,134,23,32,141,132,23,42,86,0,0,2,3,6,67,0,38,0,45,134,23,42,144,0,2,255,255,255,167,255,236,5,50,132,47,33,51,20,133,71,32,225,131,47,32,254,130,
    5,33,5,58,132,23,32,61,134,47,32,27,131,23,33,255,178,130,119,32,241,132,23,32,185,134,47,32,236,132,23,54,135,255,244,2,218,6,154,2,38,0,194,0,0,1,7,0,174,255,32,255,235,0,28,100,
    121,10,44,27,62,89,176,24,220,176,16,208,176,24,16,176,108,115,6,42,18,0,0,5,66,5,176,2,6,0,37,131,69,37,0,148,0,0,4,163,132,15,32,38,136,15,32,76,132,15,32,41,132,15,32,80,130,31,
    32,140,132,15,32,62,132,15,130,47,33,5,24,132,15,32,44,132,15,32,163,130,123,32,159,132,15,32,45,142,31,32,47,135,15,33,6,106,132,31,32,49,136,31,32,23,132,15,32,50,132,15,38,102,255,
    236,5,30,5,196,130,143,32,51,136,127,32,212,132,31,32,52,87,133,9,132,15,32,56,132,15,32,7,130,143,32,214,132,15,32,61,132,15,130,165,33,4,233,132,15,32,60,131,15,32,255,111,63,47,
    83,193,47,41,86,255,235,4,121,6,65,2,38,0,81,53,5,44,173,1,80,0,0,0,9,0,176,19,47,176,36,68,241,6,36,96,255,236,4,12,132,35,74,149,5,34,173,1,25,89,189,8,69,147,7,36,126,254,97,4,6,
    132,35,73,89,5,34,173,1,35,92,157,8,32,20,134,71,38,169,255,244,2,97,6,44,65,225,6,36,6,0,173,15,235,68,245,6,66,169,7,32,128,130,141,34,8,6,162,130,33,40,202,0,0,1,6,0,174,29,243,
    66,1,7,24,175,66,8,41,30,220,176,21,208,176,30,16,176,39,68,213,5,43,142,0,0,4,107,4,58,2,6,0,141,0,76,93,5,36,236,4,61,4,78,130,15,32,83,132,15,36,146,254,96,4,31,132,31,32,118,132,
    15,36,22,0,0,3,218,132,15,32,90,132,15,32,31,130,15,32,232,132,15,32,92,131,15,33,255,204,130,165,34,146,5,183,66,135,8,36,106,255,111,255,235,80,135,7,24,116,91,8,35,20,220,176,29,
    133,127,132,179,33,5,191,136,179,34,106,108,243,135,45,136,179,35,26,220,176,35,92,27,10,33,6,65,130,45,130,157,37,1,7,0,173,1,34,65,39,5,34,4,47,176,71,225,7,132,81,33,6,52,134,81,
    131,35,34,13,255,243,65,41,6,113,3,10,35,6,45,6,50,130,35,35,205,0,0,1,130,35,35,2,44,255,241,134,35,32,35,102,59,12,114,53,40,39,0,155,0,0,4,55,7,61,130,83,32,176,132,83,34,117,1,
    130,97,17,9,115,117,8,32,8,76,125,6,44,74,255,236,4,138,5,196,0,39,0,99,178,17,24,78,95,10,37,9,47,27,177,9,31,70,13,6,32,29,130,12,46,29,15,62,89,178,2,29,9,17,18,57,178,14,9,29,130,
    6,33,176,9,127,13,12,33,2,16,24,71,238,10,33,178,34,132,41,36,176,29,16,178,37,105,112,10,36,1,52,38,36,39,25,122,81,101,34,255,255,0,68,5,14,67,133,48,52,45,255,236,3,228,5,176,2,
    6,0,46,0,0,255,255,0,155,0,0,5,48,131,15,33,1,227,68,53,8,38,24,7,54,2,38,0,47,65,105,6,32,110,87,55,9,97,9,8,67,21,7,38,57,255,235,4,221,7,35,130,45,32,221,132,45,34,160,0,217,65,
    151,9,39,15,47,27,177,15,31,62,89,113,227,8,69,17,31,130,139,33,4,55,132,155,32,176,69,33,20,32,148,130,171,32,13,132,109,32,219,133,109,33,1,29,137,109,88,3,8,71,17,7,130,45,69,15,
    16,69,79,11,69,15,15,130,125,33,5,20,132,125,32,181,69,31,20,130,47,39,4,235,5,196,2,6,0,39,69,47,20,69,31,14,39,0,90,255,236,3,251,4,78,130,47,32,69,132,47,36,83,255,236,4,11,132,
    15,32,73,132,15,42,134,0,0,4,18,5,217,2,38,0,239,133,205,35,0,151,255,243,73,151,7,132,205,32,27,117,195,10,32,79,130,61,68,59,11,36,124,254,96,4,48,132,77,33,84,0,130,62,32,0,130,
    31,35,3,245,4,78,25,117,237,157,45,255,255,0,12,254,75,3,214,4,58,2,6,0,93,68,227,19,32,0,113,197,44,51,0,133,0,0,3,77,5,243,2,38,0,235,0,0,1,7,0,117,0,194,65,67,9,97,45,8,32,8,68,
    117,6,38,75,255,236,3,202,4,78,130,123,32,87,132,123,32,125,130,51,34,144,5,213,130,15,32,77,131,15,32,255,113,141,46,37,255,181,254,75,1,133,132,63,32,78,132,79,38,143,0,0,4,101,5,
    242,130,141,32,240,130,89,130,141,35,1,68,255,242,83,103,12,34,27,62,89,102,65,7,65,9,5,33,5,230,130,45,90,253,5,33,160,74,90,253,17,75,69,7,91,179,5,32,54,91,179,8,34,68,2,8,67,149,
    9,103,169,8,86,217,7,91,179,5,32,0,91,179,8,34,68,1,106,91,179,18,135,45,143,91,34,117,2,167,137,91,37,12,47,27,177,12,31,138,181,143,91,34,117,2,9,137,91,70,131,8,68,31,7,133,91,32,
    2,136,183,34,106,1,211,130,91,41,12,0,176,1,47,176,22,220,176,13,70,121,5,132,83,33,5,204,136,175,34,106,1,53,130,83,143,37,72,63,5,32,54,92,1,8,34,68,0,232,137,167,67,253,8,32,10,
    65,235,6,113,139,13,36,6,0,68,119,0,109,47,6,76,199,7,42,82,3,252,1,11,6,0,3,6,0,11,99,3,14,32,33,130,239,33,1,208,130,163,41,48,49,255,255,0,101,3,244,2,64,132,39,32,6,130,39,32,44,
    24,68,179,10,24,120,171,7,24,121,200,8,40,9,16,176,6,208,176,6,47,176,137,61,42,143,255,242,3,200,5,176,0,38,0,5,130,61,46,7,0,5,2,37,0,0,255,255,255,177,254,75,2,115,102,207,10,36,
    158,255,63,255,222,96,83,14,42,51,4,0,1,101,6,0,2,6,1,109,73,225,9,37,7,54,2,38,0,49,66,95,5,33,2,144,65,1,9,32,2,121,59,7,32,17,65,1,6,36,124,0,0,6,121,130,61,34,38,0,81,134,45,32,
    160,72,247,8,79,131,7,38,18,254,109,5,66,5,176,75,127,6,48,7,0,166,1,122,0,3,255,255,0,90,254,113,3,251,4,78,130,23,32,69,132,181,36,166,0,173,0,7,122,119,51,35,5,13,7,61,130,69,69,
    175,5,34,68,1,74,69,175,18,86,163,7,118,15,15,33,68,0,118,61,27,69,61,5,32,243,69,61,8,34,68,0,196,69,61,18,135,91,36,68,0,0,5,92,130,231,50,6,0,184,0,0,255,255,0,79,254,34,5,126,4,
    58,2,6,0,204,132,15,52,16,0,0,4,243,6,252,2,38,1,24,0,0,1,7,0,171,4,73,1,14,73,43,7,70,199,8,32,17,123,175,7,33,255,241,130,47,38,24,5,208,2,38,1,25,133,47,35,3,229,255,226,135,47,
    40,17,47,27,177,17,27,62,89,176,97,25,9,50,79,254,75,8,100,4,78,0,38,0,83,0,0,0,7,0,93,4,142,132,119,38,102,254,75,9,92,5,196,130,23,32,51,133,23,32,5,130,215,45,255,255,0,73,254,58,
    4,127,5,195,2,38,0,218,131,23,37,1,176,1,146,255,160,130,23,38,77,254,59,3,196,4,77,130,23,32,238,134,23,34,57,255,161,130,23,35,102,254,62,4,124,39,10,130,47,34,214,255,164,132,231,
    32,62,119,169,11,130,23,32,74,132,23,75,219,15,34,32,254,95,130,39,36,58,2,6,0,188,76,107,20,32,22,130,180,34,155,7,35,130,119,32,217,132,239,33,160,2,71,121,10,32,13,130,239,33,13,
    31,130,239,81,143,7,38,30,0,0,6,92,5,217,130,45,32,237,133,45,33,1,135,65,155,9,99,93,8,135,45,72,209,14,126,15,6,32,28,116,163,81,125,141,8,125,189,41,37,90,255,236,3,251,5,121,95,
    38,95,117,5,49,5,176,2,6,0,129,0,0,255,255,0,72,255,236,6,132,4,80,130,15,65,213,5,37,148,0,0,4,76,7,113,49,83,52,81,255,235,5,30,6,219,2,38,1,69,0,0,1,7,0,106,0,194,1,15,75,235,12,
    35,31,62,89,176,123,191,9,32,89,130,215,34,248,4,79,130,153,32,156,132,169,132,15,36,5,205,2,38,0,130,15,37,1,6,0,106,105,1,140,61,32,27,140,61,66,13,5,32,9,66,13,8,34,106,2,21,125,
    57,9,32,13,66,13,7,35,29,220,176,38,69,223,5,66,15,5,32,191,66,15,8,35,106,1,127,255,76,137,8,66,15,8,137,47,38,73,255,237,4,127,7,23,66,253,5,132,205,34,163,1,75,135,143,70,237,8,
    122,215,9,32,77,130,205,34,196,5,204,67,21,5,131,189,32,78,119,83,8,32,37,130,141,32,37,131,189,35,47,220,176,56,125,199,9,34,13,6,241,68,149,8,34,112,0,229,109,185,9,70,71,8,68,57,
    7,68,103,5,32,167,68,103,6,36,6,0,112,95,247,71,209,7,97,187,8,135,43,74,159,5,32,9,136,89,33,106,1,65,23,10,136,89,32,17,82,245,8,133,91,32,191,68,195,8,34,106,0,143,65,23,9,74,1,
    8,137,47,37,102,255,236,5,30,7,125,165,41,37,79,255,236,4,61,5,121,107,41,32,95,130,95,38,23,5,196,2,6,1,22,78,175,13,33,1,23,132,15,132,31,35,7,6,2,38,131,31,39,1,7,0,106,1,19,1,58,
    78,127,12,66,101,13,136,127,131,63,36,1,6,0,106,115,65,151,8,73,155,8,123,145,8,33,0,107,130,173,38,241,7,24,2,38,0,230,66,195,6,34,227,1,76,135,93,36,19,47,27,177,19,131,93,35,39,
    220,176,48,65,199,5,36,81,255,236,3,232,132,221,32,254,130,47,130,93,32,89,136,93,65,59,8,35,40,220,176,49,133,45,38,57,255,235,4,221,6,241,76,169,8,36,112,0,161,1,65,72,17,14,38,12,
    254,75,3,214,5,180,73,145,8,34,112,18,4,142,33,132,69,33,7,9,136,69,34,106,0,209,66,249,9,70,39,8,93,165,9,121,237,45,133,93,32,60,136,93,34,165,1,47,146,93,35,22,220,176,18,133,211,
    131,93,34,246,5,255,134,175,36,7,0,165,0,160,72,201,9,132,141,35,27,62,89,176,137,47,36,142,0,0,4,238,132,189,32,224,65,97,5,33,1,15,137,95,24,68,191,8,35,25,220,176,34,133,95,38,95,
    0,0,3,224,5,191,130,95,32,248,65,97,5,32,103,67,181,8,106,201,8,137,45,50,155,0,0,6,88,7,10,0,38,0,229,11,0,0,39,0,45,4,185,130,53,38,7,0,106,1,194,1,62,67,189,16,35,32,220,176,41,
    133,101,38,143,0,0,5,201,5,191,130,55,32,253,130,189,36,39,0,140,4,71,134,55,32,116,67,13,9,102,105,8,35,31,220,176,40,133,55,42,41,254,75,5,81,5,176,2,38,0,60,130,55,48,7,1,175,3,
    195,0,0,255,255,0,31,254,75,4,86,4,58,130,23,32,92,133,23,33,2,200,66,205,8,38,3,6,0,2,6,0,72,132,15,32,45,130,63,32,253,132,63,32,220,133,39,33,4,111,132,23,32,33,130,23,32,7,132,
    63,32,241,133,23,33,3,121,132,23,34,18,254,151,73,43,12,34,172,5,13,73,43,6,32,155,73,43,12,35,172,4,64,0,24,68,215,9,32,187,70,199,8,36,170,5,5,1,60,88,127,6,68,49,7,38,90,255,236,
    3,251,6,133,123,97,8,36,170,4,143,0,6,101,121,6,123,179,11,34,74,7,177,135,71,39,1,183,0,191,1,33,0,23,74,135,5,79,145,7,36,177,14,9,244,176,24,78,55,7,130,85,35,4,212,6,124,134,85,
    44,6,1,183,73,236,0,12,0,176,23,47,176,44,67,15,8,38,16,0,0,5,66,7,174,136,85,36,182,0,196,1,43,135,85,39,4,47,27,177,4,31,62,89,132,85,40,19,208,48,49,0,255,255,255,154,132,171,32,
    121,136,85,34,182,78,246,134,85,32,42,136,85,32,18,132,85,32,222,136,85,36,181,0,195,1,19,131,37,35,4,47,176,11,66,183,8,131,159,34,87,6,169,136,73,34,181,77,222,150,73,32,214,136,
    73,32,180,130,159,32,5,147,73,35,3,251,6,161,136,73,34,180,78,208,145,73,37,254,151,5,66,7,55,133,73,35,0,39,0,157,130,147,35,54,0,7,0,65,191,12,33,6,1,133,67,37,0,38,0,157,77,0,131,
    29,65,197,12,65,39,9,36,179,0,239,1,48,134,135,32,14,104,109,8,65,199,5,65,27,9,34,179,121,251,134,135,32,45,70,119,8,65,27,5,137,73,32,184,164,73,32,184,151,73,33,8,62,65,27,8,36,
    178,0,238,1,54,149,147,33,7,8,65,27,8,34,178,120,0,149,147,33,8,24,136,73,36,177,0,241,1,60,134,73,32,20,101,181,8,133,221,32,226,136,73,34,177,123,6,134,73,33,51,220,24,66,43,7,37,
    18,254,151,5,66,7,73,181,6,37,0,39,0,160,0,246,130,147,65,101,14,33,5,230,133,67,132,31,33,128,0,65,103,11,38,148,254,158,4,76,5,176,130,31,32,41,134,23,44,203,0,10,255,255,0,83,254,
    148,4,11,4,78,130,23,32,73,134,23,32,143,81,157,8,34,76,7,194,122,35,8,43,170,4,202,1,67,0,9,0,176,6,47,176,88,183,7,38,83,255,236,4,11,6,133,133,59,37,1,7,0,170,4,131,67,93,5,122,
    197,16,32,51,136,71,36,164,0,138,1,62,134,71,32,23,119,253,11,33,5,246,134,71,36,6,0,164,67,1,119,253,6,118,177,11,34,15,7,184,135,69,37,1,183,0,132,1,40,67,77,7,45,7,47,27,177,7,31,
    62,89,177,15,9,244,176,21,67,77,5,32,0,131,155,34,200,6,124,135,83,39,1,183,61,236,0,12,0,176,130,153,32,32,113,173,7,39,255,213,0,0,4,76,7,181,136,85,36,182,0,137,1,50,135,85,32,6,
    130,85,32,6,135,85,67,249,6,33,255,142,132,241,32,121,136,85,34,182,66,246,134,85,32,30,135,85,33,0,148,130,85,34,146,7,229,136,85,36,181,0,136,1,26,109,251,6,35,12,220,176,19,69,93,
    5,131,159,34,75,6,169,136,73,34,181,65,222,148,73,34,76,7,221,136,73,32,180,130,159,32,12,148,73,34,11,6,161,136,73,34,180,66,208,145,73,33,254,158,130,233,32,62,133,73,35,0,39,0,157,
    130,147,37,61,0,7,0,172,4,66,5,10,33,6,1,133,67,36,0,38,0,157,65,66,59,5,66,11,5,38,163,0,0,2,17,7,194,118,167,8,34,170,3,120,66,11,5,35,2,47,176,4,65,195,6,130,41,35,1,253,6,126,130,
    35,46,140,0,0,1,7,0,170,3,100,255,255,0,9,0,176,138,35,36,148,254,154,1,167,119,27,8,38,7,0,172,3,120,0,6,130,37,34,120,254,158,119,29,10,131,23,34,92,0,10,130,23,38,102,254,148,5,
    30,5,196,130,83,77,189,5,35,172,5,29,0,78,93,5,36,146,4,61,4,78,130,23,77,237,5,36,172,4,157,255,254,127,9,5,35,5,30,7,187,115,173,8,36,170,5,28,1,60,131,131,32,20,126,229,9,38,79,
    255,236,4,61,6,133,87,133,8,34,170,4,152,66,179,5,35,4,47,176,27,116,81,10,34,97,7,177,135,71,41,1,183,0,214,1,33,0,12,0,176,130,71,32,33,75,117,8,131,73,34,221,6,124,134,73,36,6,1,
    183,82,236,67,217,6,32,29,71,225,8,34,39,255,236,130,145,32,174,136,73,36,182,0,219,1,43,134,73,32,31,135,73,33,255,163,132,147,32,121,136,73,34,182,87,246,134,73,32,27,136,73,74,155,
    5,32,222,136,73,34,181,0,218,69,249,5,132,219,76,9,7,131,147,34,96,6,169,136,73,34,181,86,222,150,73,32,214,136,73,32,180,130,147,32,5,143,147,32,0,65,39,5,32,161,136,73,34,180,87,
    208,145,73,33,254,148,130,221,32,55,133,73,35,0,39,0,157,130,147,69,249,5,65,167,10,33,6,1,133,67,36,0,38,0,157,86,66,85,5,65,173,5,43,88,255,236,5,170,7,51,2,38,0,151,0,131,23,38,
    117,1,211,1,51,255,255,132,121,34,187,6,0,130,23,40,152,0,0,1,7,0,117,1,88,90,115,8,32,37,65,161,6,143,59,34,68,1,52,148,59,34,68,0,185,136,59,32,35,140,59,32,184,136,119,36,170,5,
    22,1,57,136,119,32,133,136,119,34,170,4,155,66,25,5,34,9,47,176,141,59,32,41,136,59,36,164,0,214,1,52,135,59,33,5,246,134,59,37,6,0,164,91,1,0,115,227,5,32,46,135,117,37,254,148,5,
    170,6,46,136,57,35,172,5,6,0,66,203,5,36,139,4,187,4,168,133,57,49,0,7,0,172,4,154,255,247,255,255,0,125,254,148,4,189,5,176,114,27,5,132,23,32,242,132,47,38,119,254,148,3,247,4,58,
    114,3,5,132,23,32,65,132,23,114,75,5,32,187,133,47,37,1,7,0,170,4,243,66,251,5,33,0,47,121,181,8,114,149,5,131,225,34,89,0,0,132,35,32,145,133,225,32,6,137,35,130,71,35,6,61,7,66,130,
    71,32,153,132,35,36,117,1,215,1,66,97,213,14,130,71,35,5,40,5,236,130,35,32,154,134,35,34,87,255,236,83,91,6,32,28,24,74,111,9,140,71,34,68,1,56,136,71,80,153,7,143,71,34,68,0,184,
    136,71,32,26,140,71,32,199,136,143,36,170,5,26,1,72,134,143,140,71,33,6,113,136,143,36,170,4,154,255,242,134,143,141,71,32,56,136,71,34,164,0,218,68,195,5,32,4,92,237,9,133,143,32,
    226,134,71,36,6,0,164,90,237,134,69,66,119,7,38,125,254,139,6,61,6,1,133,69,49,0,7,0,172,5,25,255,247,255,255,0,119,254,148,5,40,4,147,133,57,131,23,45,4,69,0,0,255,255,0,7,254,164,
    4,214,5,176,85,83,5,132,23,34,198,0,16,130,23,38,12,254,15,3,214,4,58,76,89,5,132,71,34,70,255,123,130,23,38,7,0,0,4,214,7,187,85,131,8,34,170,4,202,65,197,5,35,1,47,176,9,85,121,12,
    32,133,76,149,8,34,170,4,89,65,197,5,130,35,85,123,7,133,71,32,44,136,71,36,164,0,138,1,55,77,107,6,32,20,139,71,33,5,246,77,141,8,34,164,25,1,134,33,32,27,131,33,49,0,2,0,79,255,236,
    4,178,6,0,0,22,0,33,0,140,178,31,25,77,157,8,36,16,208,0,176,19,24,86,163,10,24,64,244,7,24,127,20,12,46,2,47,27,177,2,15,62,89,178,47,19,1,93,178,15,131,4,39,22,2,19,17,18,57,176,
    22,24,82,77,12,34,4,12,6,130,20,33,178,14,132,6,42,176,15,208,176,22,16,176,17,208,176,6,115,173,12,24,86,220,14,55,1,35,17,35,39,6,35,34,2,17,52,18,51,50,23,53,35,53,51,53,51,21,51,
    1,24,100,202,9,8,59,4,178,175,220,12,109,182,190,235,232,195,172,106,251,251,243,175,252,144,127,117,149,69,67,149,118,128,4,201,251,55,112,132,1,50,1,7,250,1,47,120,243,170,141,141,
    252,157,165,185,133,1,206,130,187,255,255,0,79,254,174,132,243,58,38,0,72,0,0,0,39,1,222,1,133,2,66,1,7,0,67,0,153,255,109,0,18,0,178,47,28,130,206,37,31,28,1,113,178,159,130,9,33,
    48,49,130,51,42,155,254,154,5,126,5,176,2,38,1,227,130,51,52,7,1,176,4,47,0,0,255,255,0,143,254,154,4,194,4,58,2,38,0,240,133,23,33,3,115,132,23,32,148,130,47,32,219,131,47,33,0,44,
    133,23,33,4,140,132,23,32,134,130,47,32,213,132,47,32,243,133,23,32,3,82,215,5,32,45,130,23,119,239,10,35,1,176,2,77,132,47,36,35,254,154,3,208,132,47,32,245,133,47,33,1,230,132,23,
    32,41,130,95,32,34,77,185,10,34,176,3,211,132,23,32,31,130,71,32,39,77,185,10,34,176,2,216,132,23,32,142,130,47,32,173,132,47,32,224,133,71,33,4,94,132,23,32,95,130,47,34,164,4,59,
    78,159,5,32,0,130,215,33,3,85,132,23,130,47,33,4,238,139,47,33,2,207,132,23,130,47,33,3,224,139,47,33,1,198,132,23,32,155,130,71,32,55,132,47,32,176,133,95,33,1,7,132,23,32,133,130,
    191,32,77,132,143,32,235,133,23,33,0,236,132,23,36,22,254,154,8,5,132,47,32,217,133,23,33,6,182,132,23,36,30,254,154,6,180,132,47,32,237,133,23,33,5,101,132,23,42,22,254,67,5,188,5,
    196,2,38,1,63,133,23,45,2,237,255,169,255,255,255,203,254,70,4,139,4,78,130,23,32,64,133,23,38,1,245,255,172,255,255,0,124,205,5,36,0,2,6,0,76,130,23,54,2,255,208,0,0,4,193,5,176,0,
    19,0,28,0,110,178,0,29,30,17,18,57,176,24,73,86,7,24,95,250,12,24,117,136,8,34,19,16,10,131,36,32,19,66,202,12,32,2,133,20,45,2,47,176,0,16,176,12,208,176,19,16,176,14,208,24,148,99,
    14,32,10,24,90,112,11,45,48,49,1,35,21,33,50,22,22,21,20,4,7,33,24,85,36,7,8,56,3,17,33,50,54,53,52,38,39,2,109,224,1,42,160,238,124,254,235,239,253,211,192,192,253,224,224,1,41,128,
    143,140,124,4,71,196,110,202,133,204,248,2,4,71,170,191,191,253,199,254,18,139,115,110,128,2,0,64,201,201,44,1,255,240,0,0,4,55,5,176,0,13,0,73,79,137,5,36,8,47,27,177,8,96,92,7,68,
    92,8,39,13,8,2,17,18,57,176,13,65,135,11,40,176,4,208,176,13,16,176,6,208,24,140,148,13,24,121,122,16,63,141,246,252,171,171,3,156,253,96,246,2,159,253,97,2,159,170,2,103,204,254,101,
    0,1,255,226,0,0,3,77,4,58,142,123,68,229,7,182,123,41,33,17,35,17,35,53,51,17,33,21,130,1,56,2,127,254,248,242,163,163,2,200,254,42,1,8,1,209,254,47,1,209,170,1,191,196,251,0,130,125,
    36,227,0,0,5,68,130,249,34,20,0,116,146,249,66,142,12,24,92,247,8,24,200,184,8,36,15,62,89,178,14,65,19,5,32,14,24,71,58,11,33,178,7,133,20,24,142,142,13,49,7,16,176,10,208,176,4,16,
    176,12,208,178,18,1,14,17,18,57,65,36,9,53,53,51,21,51,21,35,21,51,1,33,1,1,33,2,87,172,252,204,204,252,213,213,24,149,129,14,44,4,63,170,199,199,170,243,2,100,253,71,253,9,130,187,
    39,174,0,0,4,73,6,0,0,141,187,91,199,7,132,187,65,70,15,32,176,141,187,33,16,2,130,133,32,176,143,187,32,16,24,123,95,17,161,187,32,17,133,187,42,1,246,111,242,231,231,242,196,196,
    105,1,24,149,176,12,53,4,187,170,155,155,170,253,225,1,158,254,17,253,181,0,255,255,0,148,254,126,5,97,109,5,50,219,0,0,0,39,0,160,1,29,1,61,1,7,0,16,4,128,255,198,96,57,12,24,85,253,
    11,37,134,254,126,4,228,5,96,103,6,131,53,35,0,151,255,243,131,53,33,3,135,142,53,96,111,11,131,107,32,233,70,3,9,130,99,34,140,255,198,130,131,131,77,32,227,70,3,9,35,0,16,3,134,132,
    23,130,47,33,7,50,132,47,32,49,130,155,130,147,33,5,213,132,23,36,143,254,126,6,65,132,47,32,242,133,23,33,4,228,132,23,32,45,130,203,32,220,83,125,9,130,95,32,127,132,23,32,33,130,
    173,32,230,83,125,9,130,95,32,137,130,165,47,1,0,7,0,0,4,214,5,176,0,14,0,86,178,10,15,24,170,233,9,67,118,12,36,11,47,27,177,11,67,131,16,42,6,2,8,17,18,57,176,6,47,178,5,24,72,159,
    9,35,1,208,178,10,66,114,5,35,6,16,176,14,24,124,247,10,34,1,33,1,130,2,44,51,3,195,213,254,202,122,254,103,1,25,1,79,130,1,54,24,254,103,134,2,4,253,252,2,4,170,3,2,253,78,2,178,252,
    254,0,0,1,0,91,55,6,130,149,32,99,145,149,66,73,7,84,187,8,36,0,69,88,176,2,130,162,24,222,203,31,136,165,35,178,10,11,0,131,186,33,13,208,132,162,33,5,35,67,163,5,8,35,1,51,19,19,
    51,1,51,3,96,220,243,206,162,254,187,251,243,236,251,254,188,175,1,254,96,1,160,170,3,145,253,1,2,255,252,111,131,157,98,145,6,35,0,17,0,99,67,169,5,65,31,12,32,14,130,137,32,14,65,
    44,15,32,176,131,163,24,132,20,8,34,17,11,2,131,126,32,17,70,69,12,32,4,133,20,41,7,208,176,17,16,176,9,208,178,13,132,15,35,48,49,1,35,65,58,5,33,35,53,66,215,6,55,51,3,219,135,1,
    149,254,217,254,199,254,198,254,218,1,150,129,115,254,130,1,36,1,50,130,1,57,36,254,131,121,2,149,253,107,2,22,253,234,2,149,170,2,113,253,242,2,14,253,143,0,1,0,102,243,6,142,179,
    65,74,7,24,195,52,12,68,93,12,137,179,32,14,147,179,133,20,137,179,132,15,133,179,35,3,3,33,1,132,179,8,89,19,19,33,1,51,3,87,149,1,38,254,244,216,215,254,242,1,37,138,130,254,239,
    1,12,202,206,1,14,254,238,140,1,215,254,41,1,114,254,142,1,215,170,1,185,254,156,1,100,254,71,255,255,0,96,255,236,4,12,4,77,2,6,0,190,0,0,255,255,0,2,0,0,4,49,5,176,2,38,0,42,0,0,
    0,7,1,222,255,114,254,105,130,23,35,129,2,109,5,24,227,93,15,32,81,130,43,34,64,5,196,130,59,89,111,9,32,21,132,15,89,111,5,32,52,130,31,32,88,130,75,34,6,0,24,132,91,32,129,130,107,
    32,58,132,15,32,25,132,15,41,93,255,250,4,18,5,196,0,6,0,78,207,5,32,125,130,31,32,54,132,15,33,20,20,131,139,32,106,130,15,34,240,7,75,24,73,223,8,36,117,1,189,1,75,111,179,6,32,33,
    75,101,6,38,82,254,86,4,12,6,0,24,73,213,8,34,117,1,63,96,37,8,32,39,103,173,9,24,70,13,12,34,68,1,76,96,119,9,32,6,82,135,6,32,176,86,211,7,24,70,13,15,34,68,0,179,130,81,32,19,66,
    129,5,104,119,8,32,18,24,82,145,12,32,33,87,37,8,36,171,4,119,1,51,122,65,12,39,31,62,89,176,12,220,176,16,82,151,5,38,13,255,236,3,251,5,236,87,49,8,36,171,4,1,255,254,135,47,32,23,
    24,87,155,9,24,88,169,7,38,72,0,0,4,76,7,40,83,223,8,34,171,4,60,90,179,9,136,187,35,13,220,176,17,133,95,36,1,255,236,4,11,132,95,40,73,0,0,1,7,0,171,3,245,137,95,90,87,8,32,31,105,
    53,7,37,254,246,0,0,2,30,132,95,32,45,133,47,33,2,234,137,95,97,147,8,35,5,220,176,9,132,95,33,254,226,130,47,34,10,5,228,82,135,8,36,171,2,214,255,246,135,191,132,47,32,27,130,239,
    136,47,32,0,81,87,5,32,33,82,51,8,34,171,4,142,65,31,9,99,157,8,35,32,220,176,36,132,95,33,0,22,130,191,32,61,132,191,32,83,133,143,33,4,10,137,191,36,4,47,27,177,4,131,95,32,28,24,
    84,97,8,37,50,0,0,4,222,7,131,95,32,54,134,47,32,38,65,127,18,32,25,106,83,7,33,255,110,130,191,32,180,132,95,32,86,133,47,33,3,98,137,95,93,17,8,32,15,84,71,8,32,113,130,143,32,189,
    132,95,32,57,133,47,33,4,101,137,95,105,144,8,35,18,220,176,22,133,191,36,15,255,236,3,247,132,95,80,3,5,34,171,4,3,146,95,136,47,50,254,172,0,0,5,2,6,65,0,38,0,207,100,0,0,7,0,173,
    253,76,99,5,43,148,254,158,4,163,5,176,2,38,0,38,0,131,23,50,172,4,185,0,10,255,255,0,124,254,139,4,50,6,0,2,38,0,70,134,23,34,203,255,247,130,23,131,47,32,210,132,47,32,40,134,23,
    32,148,132,47,36,79,254,148,4,3,132,47,32,72,134,23,41,180,0,0,255,255,0,148,253,249,4,138,47,37,1,162,1,72,254,146,130,23,32,79,130,23,138,47,130,23,32,104,132,23,130,95,33,5,24,71,
    79,10,34,172,5,38,132,95,36,121,254,158,3,248,132,95,32,76,134,95,32,161,132,23,32,148,130,215,105,135,17,35,9,0,176,4,24,76,131,8,39,0,125,0,0,4,54,7,61,130,203,43,79,0,0,1,7,0,117,
    1,107,1,61,0,141,35,34,148,254,223,134,119,32,47,134,95,34,233,0,75,130,167,34,125,254,202,24,75,9,11,37,0,172,4,121,0,54,65,7,6,32,38,132,167,32,48,65,55,12,36,120,254,158,1,139,132,
    167,32,80,133,23,33,3,92,133,167,35,254,158,6,106,71,247,10,34,172,5,214,132,23,32,124,130,23,34,121,4,78,130,155,32,81,133,47,33,5,217,132,23,34,148,254,154,24,74,7,11,37,0,172,5,
    40,0,6,130,119,34,121,254,158,24,74,7,11,130,143,32,141,133,47,37,0,0,4,212,7,66,130,71,32,52,134,227,32,114,81,167,5,32,3,24,95,147,9,38,124,254,96,4,48,5,247,130,35,32,84,134,35,
    39,157,255,247,0,9,0,176,12,108,185,9,34,148,254,158,24,72,129,11,130,95,32,186,132,95,34,114,254,158,24,72,129,11,35,0,172,3,86,132,23,34,74,254,148,24,71,153,12,42,172,4,213,0,0,
    255,255,0,75,254,139,24,71,105,11,130,71,34,124,255,247,130,23,34,45,254,151,24,70,243,12,36,172,4,195,0,3,130,23,34,8,254,148,24,71,35,11,130,47,32,20,132,71,38,18,0,0,5,29,7,56,130,
    179,32,58,132,179,34,164,0,176,81,239,5,24,88,43,10,38,22,0,0,3,218,5,237,130,35,32,90,130,35,36,6,0,164,24,248,81,1,6,69,61,8,37,254,158,5,29,5,176,133,69,33,0,7,130,93,32,239,132,
    189,38,22,254,158,3,218,4,58,133,57,132,23,32,87,132,23,36,48,254,158,6,229,132,47,32,59,65,149,6,32,230,132,23,32,33,130,71,32,204,132,47,32,91,134,23,32,78,132,23,36,80,254,158,4,
    140,132,47,32,62,133,23,33,4,193,132,23,32,82,130,95,32,192,132,47,32,94,134,23,32,99,131,23,51,254,28,255,236,5,100,5,215,0,38,0,51,70,0,0,7,1,90,253,181,132,237,42,9,0,0,4,148,5,
    30,2,38,1,186,132,47,40,173,255,118,254,221,255,255,255,42,130,225,39,241,5,33,0,38,1,190,60,130,47,37,0,173,254,100,254,224,130,23,32,55,130,47,34,164,5,28,130,23,32,193,134,23,34,
    113,254,219,130,23,36,57,0,0,1,179,132,47,32,194,134,23,32,115,132,47,38,147,255,240,4,121,5,30,130,47,33,200,10,133,71,32,205,131,95,33,254,232,130,71,32,114,132,23,32,210,134,47,
    32,34,132,119,32,164,130,23,32,142,132,23,32,243,134,47,32,222,131,23,32,0,132,167,35,4,141,2,6,131,167,119,103,6,32,10,132,15,32,187,132,199,32,118,130,175,32,181,132,15,32,190,132,
    15,32,65,130,15,32,243,132,15,80,193,5,130,31,33,4,104,132,15,32,193,132,31,32,133,130,175,32,119,132,15,32,194,132,15,137,31,32,196,135,15,33,5,143,132,31,80,137,5,32,79,130,199,34,
    111,4,157,130,127,94,179,5,131,47,32,44,132,31,32,201,132,47,32,36,130,183,32,22,132,15,32,205,132,15,32,5,130,15,32,54,132,15,32,210,132,15,32,21,130,15,32,74,132,15,32,209,131,15,
    24,64,1,48,132,79,116,69,40,24,65,21,44,32,0,131,45,39,151,6,30,2,38,1,234,0,68,173,5,32,35,119,107,5,34,4,47,176,107,161,7,36,62,255,240,3,239,132,255,32,204,65,63,19,176,207,32,36,
    130,79,37,100,4,141,2,6,1,96,27,5,130,131,33,4,104,132,131,32,196,120,77,16,32,15,72,145,6,42,31,255,236,4,57,6,4,2,38,2,1,130,35,35,6,0,160,122,116,191,8,119,45,16,66,47,35,32,151,
    132,127,130,243,66,79,5,32,3,66,63,11,131,143,32,110,131,107,33,1,254,132,143,34,160,0,186,117,135,18,32,13,125,219,9,66,45,12,66,109,15,66,61,19,32,98,132,125,32,239,66,125,8,66,77,
    11,131,47,38,67,4,157,2,6,1,188,66,93,20,66,77,12,55,0,1,0,66,254,57,3,231,4,157,0,40,0,164,178,39,41,42,17,18,57,0,176,23,85,103,5,32,10,124,207,7,24,204,176,12,24,147,160,15,32,25,
    130,50,35,178,39,25,10,130,6,46,176,39,47,178,95,39,1,114,178,63,39,1,113,178,207,131,4,32,255,131,4,32,15,130,19,47,180,111,39,127,39,2,113,180,175,39,191,39,2,93,178,143,130,18,33,
    178,191,131,4,32,36,113,118,8,35,178,16,36,39,24,92,95,8,33,178,29,133,82,32,25,24,181,85,14,35,52,38,35,34,24,174,61,9,43,6,7,22,22,21,20,6,7,17,35,17,38,24,147,184,28,36,187,172,
    243,155,176,24,147,182,24,41,134,174,24,254,65,1,194,24,172,135,24,147,186,7,48,0,1,0,118,254,154,5,44,4,141,0,15,0,168,178,3,16,24,179,135,9,34,12,47,27,24,138,6,9,24,112,231,12,32,
    1,130,25,32,1,120,153,7,86,152,12,24,125,148,8,34,10,6,9,131,226,47,10,47,180,175,10,191,10,2,93,178,63,10,1,113,178,207,131,4,130,9,34,114,178,255,131,9,46,15,10,1,114,180,111,10,
    127,10,2,113,180,223,10,239,130,38,35,180,31,10,47,130,6,33,178,95,130,25,33,178,5,24,141,74,12,32,14,78,89,8,81,71,6,24,244,95,9,60,51,5,44,243,196,253,244,243,243,2,12,243,196,254,
    154,1,102,1,219,254,37,4,141,254,17,1,239,252,40,130,225,42,79,254,67,4,67,4,157,0,30,0,94,121,123,12,32,14,130,199,24,150,130,8,32,4,130,12,32,4,127,209,7,135,199,38,176,6,208,178,
    18,14,3,131,202,32,14,24,110,205,12,32,3,121,113,11,35,178,30,3,14,81,223,5,32,6,65,162,5,35,2,39,53,52,24,190,72,20,40,12,198,169,243,181,207,1,126,236,24,190,72,17,45,159,208,27,
    254,73,1,185,36,1,31,221,79,169,255,24,190,77,11,24,136,53,9,34,2,6,1,69,43,5,52,10,254,58,5,168,4,163,2,38,2,23,0,0,0,7,1,176,2,230,255,160,67,163,5,37,4,110,5,210,2,38,67,147,6,44,
    112,0,130,0,34,0,9,0,176,0,47,176,10,68,35,11,131,35,68,35,6,33,112,66,132,33,32,2,24,80,241,9,38,80,0,0,5,77,4,141,130,109,40,241,0,0,255,255,0,18,254,85,100,61,12,36,163,1,130,0,
    3,130,23,34,90,254,89,100,61,12,36,163,0,181,0,7,130,23,34,148,254,92,97,15,12,36,163,1,64,0,10,130,23,34,83,254,82,97,15,12,34,163,1,4,132,95,38,120,254,158,1,139,4,58,76,87,5,39,
    0,7,0,172,3,92,0,10,130,215,40,15,0,186,0,3,0,1,4,9,130,11,32,94,130,3,133,11,36,1,0,26,0,94,134,23,36,2,0,14,0,120,134,11,32,3,138,23,32,4,138,11,36,5,0,44,0,134,134,35,32,6,130,23,
    32,178,134,11,36,7,0,64,0,204,134,11,36,9,0,12,1,12,134,11,36,11,0,20,1,24,134,11,36,12,0,38,1,44,134,11,36,13,0,92,1,82,134,11,36,14,0,84,1,174,134,11,36,16,0,12,2,2,134,11,32,17,
    130,11,58,14,0,67,0,111,0,112,0,121,0,114,0,105,0,103,0,104,0,116,0,32,0,50,0,48,0,49,130,1,34,32,0,71,130,29,32,111,130,23,34,108,0,101,130,23,38,73,0,110,0,99,0,46,130,9,34,65,0,
    108,130,1,34,32,0,82,136,53,32,115,130,21,32,82,130,37,32,115,130,3,34,114,0,118,130,5,32,100,130,41,32,82,130,63,32,98,130,3,32,116,130,3,34,32,0,77,132,21,36,105,0,117,0,109,130,
    57,32,101,130,87,32,117,130,69,32,97,130,123,32,86,132,53,32,115,130,77,32,111,130,99,131,129,32,46,130,131,133,133,32,53,130,143,32,59,136,149,32,52,130,57,33,111,0,135,83,32,45,142,
    83,137,25,32,32,130,75,131,147,32,97,130,57,32,116,130,97,34,97,0,100,130,99,33,109,0,131,109,32,107,130,19,34,111,0,102,130,5,139,223,32,46,140,237,141,25,32,99,130,191,36,109,0,67,
    0,104,130,73,36,105,0,115,0,116,130,93,32,97,132,169,133,225,32,101,130,23,32,116,130,23,131,189,32,76,130,27,32,99,130,107,32,110,130,15,32,101,130,117,32,32,130,239,32,110,132,125,
    32,114,132,139,32,104,130,27,36,32,0,65,0,112,130,139,32,99,130,81,32,101,130,21,141,51,32,44,130,17,65,17,21,32,104,130,121,32,116,130,57,34,58,0,47,130,1,33,119,0,131,1,32,46,130,
    71,137,75,32,46,130,169,34,114,0,103,130,29,32,108,140,135,32,115,130,17,34,76,0,73,130,195,46,69,0,78,0,83,0,69,0,45,0,50,0,46,0,48,65,79,12,65,161,11,33,3,0,132,0,33,255,106,130,
    187,132,9,142,4,44,1,0,2,0,8,0,2,255,255,0,15,0,1,130,27,8,32,10,0,92,0,172,0,4,68,70,76,84,0,26,99,121,114,108,0,40,103,114,101,107,0,54,108,97,116,110,0,68,0,4,130,35,32,0,130,45,
    36,2,0,0,0,4,138,13,34,1,0,5,138,13,34,2,0,6,138,13,42,3,0,7,0,8,99,112,115,112,0,50,132,5,32,56,132,5,32,62,132,5,38,68,107,101,114,110,0,74,145,5,130,95,32,1,132,145,34,1,0,3,132,
    11,32,2,132,5,130,4,33,0,1,130,91,34,5,0,12,133,1,33,1,222,134,37,32,8,130,7,32,10,130,133,34,36,0,72,130,9,32,222,130,213,9,236,37,0,38,0,39,0,40,0,41,0,42,0,43,0,44,0,45,0,46,0,47,
    0,48,0,49,0,50,0,51,0,52,0,53,0,54,0,55,0,56,0,57,0,58,0,59,0,60,0,61,0,62,0,101,0,103,0,146,0,176,0,177,0,178,0,179,0,180,0,181,0,182,0,183,0,184,0,185,0,209,0,210,0,211,0,212,0,213,
    0,214,0,215,0,216,0,217,0,218,0,219,0,220,0,221,0,222,0,223,0,224,0,225,0,226,0,227,0,228,0,229,0,230,0,231,0,232,1,44,1,48,1,50,1,56,1,58,1,60,1,62,1,63,1,69,1,70,1,127,1,133,1,138,
    1,141,2,70,2,71,2,73,2,75,2,76,2,77,2,78,2,79,2,80,2,81,2,82,2,83,2,84,2,85,2,86,2,87,2,88,2,89,2,90,2,91,2,92,2,93,2,94,2,95,2,96,2,97,2,98,2,99,2,100,2,101,2,130,2,132,2,134,2,136,
    2,138,2,140,2,142,2,144,2,146,2,148,2,150,2,152,2,154,2,156,2,158,2,160,2,162,2,164,2,166,2,168,2,170,2,172,2,174,2,177,2,179,2,181,2,183,2,185,2,187,2,189,2,191,2,193,2,196,2,198,
    2,200,2,202,2,204,2,206,2,208,2,210,2,212,2,216,2,218,2,220,2,222,2,224,2,226,2,228,2,230,2,232,2,234,2,236,2,238,2,240,2,241,2,243,2,245,3,82,3,83,3,84,3,85,3,86,3,87,3,88,3,90,3,
    91,3,92,3,93,3,94,3,95,3,96,3,97,3,99,3,100,3,101,3,102,3,103,3,104,3,105,3,121,3,122,3,123,3,124,3,125,3,126,3,127,3,128,3,129,3,130,3,131,3,132,3,133,3,134,3,135,3,136,3,137,3,138,
    3,139,3,140,3,141,3,142,3,186,3,188,3,190,3,211,3,217,3,223,4,72,4,74,4,78,4,86,4,88,4,93,4,105,0,2,0,0,0,2,0,10,59,186,0,1,3,108,0,4,0,0,1,177,6,178,54,158,54,158,6,220,7,50,55,64,
    54,76,54,202,59,138,55,216,7,56,58,222,58,222,56,30,58,140,53,234,131,9,8,53,59,138,54,86,10,114,10,244,56,62,56,30,54,164,54,120,57,50,59,0,54,42,11,94,55,182,54,220,55,238,11,160,
    12,202,12,212,57,150,57,150,55,248,54,220,54,24,13,202,56,36,14,44,57,144,130,5,40,70,54,220,14,136,57,202,55,64,130,85,8,54,64,15,2,15,252,16,250,17,216,18,118,56,36,18,124,57,150,
    21,58,23,20,24,38,24,64,24,70,24,76,26,70,26,76,26,130,26,180,27,50,28,168,30,90,32,24,58,222,33,78,34,224,57,50,37,46,131,127,33,54,246,131,5,8,51,37,248,39,146,57,160,40,112,41,50,
    41,192,42,30,42,248,57,40,43,130,57,144,44,76,44,118,45,220,54,220,48,98,48,160,49,210,51,144,54,220,50,84,50,218,51,4,51,90,51,144,55,64,130,145,35,164,56,36,51,130,167,45,57,202,
    57,40,58,140,58,140,57,40,54,158,51,224,131,243,49,54,158,53,82,53,120,53,130,53,140,53,170,53,188,53,206,53,224,131,253,33,59,138,131,1,33,56,62,130,59,32,64,137,3,35,54,202,55,216,
    133,1,131,137,133,3,133,41,132,45,33,30,56,132,1,35,59,0,55,182,140,1,33,238,55,132,1,35,57,150,55,248,135,1,35,56,36,56,36,130,87,32,182,135,3,33,54,202,133,1,33,59,138,130,95,32,
    238,143,3,35,58,222,57,150,137,113,65,145,5,33,53,234,131,1,133,25,131,29,35,57,150,57,150,130,61,32,248,135,3,33,54,24,131,1,33,56,62,132,1,132,151,133,157,41,54,120,59,0,56,36,59,
    0,54,42,131,1,130,127,32,216,65,219,5,130,181,38,64,54,76,55,216,54,42,132,105,32,140,133,21,41,54,86,56,62,59,0,57,50,58,222,130,5,130,191,130,181,36,248,55,216,57,202,66,25,7,36,
    58,140,54,246,55,130,55,34,57,202,55,134,71,41,54,86,54,202,56,62,57,50,55,182,130,245,46,248,54,220,56,36,57,144,55,238,57,40,56,36,54,120,131,1,131,125,65,119,5,131,187,132,255,36,
    216,55,238,54,164,130,31,32,202,131,27,130,21,36,50,57,144,58,222,65,29,8,130,29,130,73,33,238,57,130,21,131,219,32,55,130,83,33,54,246,130,45,134,3,131,23,136,43,32,64,130,117,163,
    3,131,83,154,3,133,251,32,55,65,75,8,65,87,11,133,131,35,56,30,56,30,65,71,5,33,56,36,132,181,34,140,58,222,131,245,37,57,144,57,202,57,40,131,147,38,57,150,57,160,57,202,58,65,71,
    5,40,0,59,138,0,2,0,139,0,4,130,1,34,0,0,6,130,1,36,1,0,11,0,12,130,19,34,19,0,19,130,19,40,37,0,42,0,5,0,44,0,54,130,21,50,56,0,63,0,22,0,69,0,70,0,30,0,73,0,74,0,32,0,76,130,1,34,
    34,0,79,130,1,40,35,0,81,0,84,0,36,0,86,130,1,34,40,0,88,130,1,36,41,0,90,0,93,130,61,38,95,0,95,0,46,0,138,130,1,34,47,0,156,130,1,42,48,0,176,0,180,0,49,0,182,0,184,130,85,38,186,
    0,186,0,57,0,188,130,1,40,58,0,191,0,192,0,59,0,194,130,1,34,61,0,196,130,1,40,62,0,198,0,205,0,63,0,209,130,1,40,71,0,211,0,221,0,72,0,223,130,1,36,83,0,225,0,227,130,109,38,229,0,
    238,0,87,0,240,130,1,10,138,97,0,245,0,247,0,98,0,250,0,251,0,101,0,253,0,255,0,103,1,2,1,4,0,106,1,9,1,9,0,109,1,12,1,12,0,110,1,23,1,25,0,111,1,33,1,33,0,114,1,43,1,45,0,115,1,48,
    1,48,0,118,1,50,1,50,0,119,1,73,1,73,0,120,1,108,1,109,0,121,1,111,1,113,0,123,1,186,1,186,0,126,1,189,1,189,0,127,1,196,1,197,0,128,1,200,1,200,0,130,1,202,1,203,0,131,1,205,1,205,
    0,133,2,40,2,40,0,134,2,42,2,43,0,135,2,70,2,71,0,137,2,73,2,73,0,139,2,75,2,108,0,140,2,110,2,113,0,174,2,118,2,123,0,178,2,128,2,136,0,184,2,138,2,138,0,193,2,140,2,140,0,194,2,142,
    2,142,0,195,2,144,2,144,0,196,2,146,2,155,0,197,2,164,2,166,0,207,2,168,2,168,0,210,2,170,2,170,0,211,2,172,2,172,0,212,2,174,2,174,0,213,2,177,2,177,0,214,2,179,2,179,0,215,2,181,
    2,181,0,216,2,183,2,183,0,217,2,185,2,185,0,218,2,187,2,187,0,219,2,189,2,201,0,220,2,203,2,203,0,233,2,205,2,205,0,234,2,207,2,207,0,235,2,218,2,218,0,236,2,220,2,220,0,237,2,222,
    2,222,0,238,2,224,2,224,0,239,2,226,2,226,0,240,2,228,2,228,0,241,2,230,2,230,0,242,2,232,2,232,0,243,2,234,2,234,0,244,2,236,2,236,0,245,2,238,2,241,0,246,2,243,2,243,0,250,2,245,
    2,245,0,251,3,82,3,87,0,252,3,90,3,105,1,2,3,108,3,108,1,18,3,112,3,112,1,19,3,114,3,114,1,20,3,118,3,118,1,21,3,121,3,122,1,22,3,124,3,133,1,24,3,135,3,137,1,34,3,139,3,144,1,37,3,
    146,3,147,1,43,3,149,3,152,1,45,3,158,3,159,1,49,3,161,3,161,1,51,3,163,3,163,1,52,3,165,3,168,1,53,3,171,3,176,1,57,3,178,3,178,1,63,3,182,3,183,1,64,3,188,3,188,1,66,3,190,3,199,
    1,67,3,202,3,203,1,77,3,205,3,208,1,79,3,215,3,216,1,83,3,220,3,220,1,85,3,222,3,228,1,86,3,233,3,234,1,93,3,238,4,22,1,95,4,24,4,24,1,136,4,26,4,39,1,137,4,47,4,47,1,151,4,50,4,50,
    1,152,4,52,4,52,1,153,4,64,4,69,1,154,4,72,4,72,1,160,4,74,4,74,1,161,4,76,4,76,1,162,4,78,4,79,1,163,4,84,4,87,1,165,4,90,4,90,1,169,4,92,4,93,1,170,4,95,4,95,1,172,4,99,4,99,1,173,
    4,101,4,101,1,174,4,105,4,105,1,175,4,169,4,169,1,176,0,10,0,56,255,196,0,209,130,3,36,213,255,196,1,50,130,3,36,58,255,196,2,218,130,3,32,220,130,3,40,222,255,196,3,141,255,196,4,
    76,130,31,58,21,0,58,0,20,0,59,0,38,0,61,0,22,1,24,0,20,2,101,0,22,2,236,0,38,2,238,130,7,36,240,0,22,3,87,130,3,32,102,130,3,32,105,130,3,36,159,0,38,3,161,130,3,32,163,130,3,32,165,
    130,15,40,182,0,20,3,190,0,22,4,64,130,3,32,66,130,3,32,68,130,3,130,39,45,0,1,0,19,255,8,0,206,0,16,254,238,0,18,130,3,40,37,255,64,0,46,255,48,0,56,130,107,40,69,255,222,0,71,255,
    235,0,72,130,3,32,73,130,3,32,75,130,3,32,83,130,3,32,85,130,3,44,86,255,230,0,89,255,234,0,90,255,232,0,93,130,3,32,147,130,19,32,152,130,3,32,154,130,19,32,177,130,67,32,179,130,
    3,32,186,130,15,32,188,130,27,32,199,130,7,32,200,130,3,32,202,130,27,32,209,130,87,32,213,130,3,36,246,255,235,1,2,130,3,36,12,255,64,1,23,130,7,36,25,255,232,1,29,130,7,32,33,130,
    3,36,50,0,20,1,57,130,7,32,58,130,7,130,115,33,1,76,130,11,32,86,130,3,36,110,254,238,1,114,130,3,32,118,130,3,32,119,130,3,40,186,255,192,2,75,255,64,2,76,130,3,32,77,130,3,32,78,
    130,3,32,79,130,3,32,80,130,3,32,81,130,3,36,102,255,222,2,103,130,3,32,104,130,3,32,105,130,3,32,106,130,3,32,107,130,3,32,108,130,3,36,109,255,235,2,110,130,3,32,111,130,3,32,112,
    130,3,32,113,130,3,32,119,130,3,32,120,130,3,32,121,130,3,32,122,130,3,32,123,130,3,36,124,255,234,2,125,130,3,32,126,130,3,32,127,130,3,36,128,255,232,2,129,130,3,32,130,130,95,32,
    131,130,71,32,132,130,7,32,133,130,7,32,134,130,7,32,135,130,7,32,137,130,51,32,139,130,3,32,141,130,3,32,143,130,3,32,145,130,3,32,147,130,3,32,149,130,3,32,151,130,3,32,153,130,3,
    32,155,130,3,32,157,130,3,32,159,130,3,32,161,130,3,32,163,130,3,36,177,255,48,2,197,130,7,32,199,130,3,32,201,130,3,36,218,0,20,2,220,130,3,32,222,130,3,32,225,130,119,32,227,130,
    3,32,229,130,3,32,231,130,3,32,233,130,3,32,235,130,3,40,239,255,232,3,82,255,64,3,90,130,3,39,106,255,235,3,110,255,234,3,130,203,33,3,114,130,23,32,117,130,11,32,118,130,19,32,119,
    130,7,35,126,255,48,3,130,175,41,3,141,0,20,3,143,255,222,3,144,130,23,32,146,130,3,32,148,130,3,32,149,130,43,32,151,130,7,32,158,130,7,32,166,130,3,32,174,130,75,32,175,130,35,32,
    178,130,19,32,183,130,15,32,184,130,7,32,189,130,3,32,191,130,11,32,196,130,27,32,197,130,27,32,198,130,7,32,199,130,7,32,203,130,23,32,205,130,3,32,206,130,3,32,216,130,3,32,218,130,
    3,32,220,130,3,32,224,130,43,32,226,130,3,32,228,130,3,32,235,130,15,32,238,130,47,32,239,130,47,32,240,130,7,32,241,130,7,32,242,130,7,32,243,130,7,32,244,130,7,32,245,130,7,32,246,
    130,7,32,247,130,7,32,248,130,7,32,249,130,7,32,250,130,7,32,251,130,7,32,252,130,7,32,253,130,7,32,254,130,7,40,255,255,222,4,0,255,64,4,1,130,7,32,2,130,7,32,3,130,7,32,4,130,7,32,
    5,130,7,36,7,255,235,4,9,130,3,32,11,130,3,32,13,130,3,32,15,130,3,32,17,130,3,32,19,130,3,32,21,130,3,32,27,130,3,32,29,130,3,32,31,130,3,32,33,130,3,32,35,130,3,32,37,130,3,32,39,
    130,3,32,41,130,3,32,43,130,3,32,45,130,3,32,47,130,3,32,49,130,3,36,51,255,234,4,53,130,3,32,55,130,3,32,57,130,3,32,59,130,3,32,61,130,3,32,63,130,3,36,65,255,232,4,67,130,3,32,69,
    130,3,54,76,0,20,0,32,0,56,255,223,0,58,255,228,0,59,255,236,0,61,255,221,0,209,130,15,40,213,255,223,1,24,255,228,1,50,130,7,32,58,130,3,44,186,0,14,2,101,255,221,2,218,255,223,2,
    220,130,3,32,222,130,3,36,236,255,236,2,238,130,19,36,240,255,221,3,87,130,3,32,102,130,3,32,105,130,3,40,141,255,223,3,159,255,236,3,161,130,3,32,163,130,3,32,165,130,19,40,182,255,
    228,3,190,255,221,4,64,130,3,32,66,130,3,32,68,130,3,35,76,255,223,4,130,47,33,0,26,130,129,32,206,130,129,32,237,130,125,32,208,130,125,36,206,0,213,255,206,130,125,32,237,130,125,
    36,206,1,58,255,206,130,121,32,208,130,121,32,206,130,121,34,206,2,222,130,15,38,238,255,208,2,240,255,208,130,117,34,208,3,102,130,7,32,105,130,3,36,141,255,206,3,165,130,7,34,182,
    255,237,130,105,32,208,130,105,38,208,4,66,255,208,4,68,130,3,34,76,255,206,130,105,44,208,0,16,0,46,255,238,0,57,255,238,2,97,130,3,32,98,130,3,32,99,130,3,32,100,130,3,32,177,130,
    3,32,224,130,3,32,226,130,3,32,228,130,3,32,230,130,3,32,232,130,3,40,234,255,238,3,126,255,238,4,50,130,3,32,52,130,59,34,74,0,6,130,69,32,11,130,3,44,13,0,20,0,65,0,18,0,71,255,232,
    0,72,130,3,32,73,130,3,32,75,130,3,32,85,130,3,36,97,0,19,0,147,130,7,32,152,130,3,32,186,130,3,32,199,130,3,32,200,130,3,36,246,255,232,1,2,130,3,32,29,130,3,32,33,130,3,32,57,130,
    3,130,51,33,1,76,130,7,32,86,130,3,36,108,0,16,1,109,130,3,32,111,130,3,32,112,130,3,40,113,0,16,2,109,255,232,2,110,130,3,32,111,130,3,32,112,130,3,32,113,130,3,32,137,130,3,32,139,
    130,3,32,141,130,3,32,143,130,3,32,145,130,3,130,111,33,2,149,130,7,32,151,130,3,32,153,130,3,32,155,130,3,32,157,130,3,32,159,130,3,32,161,130,3,36,163,255,232,3,106,130,3,32,144,
    130,3,32,148,130,3,32,151,130,3,36,167,0,16,3,168,130,3,32,171,130,3,32,178,130,15,32,184,130,3,32,189,130,3,32,203,130,3,32,205,130,3,32,206,130,3,32,218,130,3,36,235,255,232,4,7,
    130,3,32,9,130,3,32,11,130,3,32,13,130,3,32,15,130,3,32,17,130,3,32,19,130,3,32,21,130,3,32,41,130,3,32,43,130,3,32,45,130,3,32,49,130,235,48,2,0,245,255,214,1,109,255,152,0,61,0,71,
    255,236,0,72,130,3,32,73,130,3,32,75,130,3,32,85,130,3,32,147,130,3,32,152,130,3,32,186,130,3,32,199,130,3,32,200,130,3,36,246,255,236,1,2,130,3,32,29,130,3,32,33,130,3,32,57,130,3,
    130,47,33,1,76,130,7,36,86,255,236,2,109,130,3,32,110,130,3,32,111,130,3,32,112,130,3,32,113,130,3,32,137,130,3,32,139,130,3,32,141,130,3,32,143,130,3,32,145,130,3,130,91,33,2,149,
    130,7,32,151,130,3,32,153,130,3,32,155,130,3,32,157,130,3,32,159,130,3,32,161,130,3,36,163,255,236,3,106,130,3,32,144,130,3,32,148,130,3,32,151,130,3,32,178,130,3,32,184,130,3,32,189,
    130,3,32,203,130,3,32,205,130,3,32,206,130,3,32,218,130,3,34,235,255,236,130,255,34,236,4,9,130,7,32,11,130,3,32,13,130,3,32,15,130,3,32,17,130,3,32,19,130,3,32,21,130,3,32,41,130,
    3,32,43,130,3,32,45,130,3,32,49,130,203,38,24,0,83,255,226,1,23,130,3,40,109,0,24,2,119,255,226,2,120,130,3,32,121,130,3,32,122,130,3,32,123,130,3,32,197,130,3,32,199,130,3,36,201,
    255,226,3,112,130,3,32,118,130,3,32,146,130,3,32,216,130,3,36,220,255,226,4,27,130,3,32,29,130,3,32,31,130,3,32,33,130,3,32,35,130,3,32,37,130,3,32,39,130,3,46,47,255,226,0,6,0,16,
    255,132,0,18,255,132,1,110,130,3,32,114,130,3,32,118,130,3,32,119,130,19,34,16,0,46,130,129,32,57,130,233,32,97,130,3,32,98,130,3,32,99,130,3,32,100,130,3,32,177,130,3,32,224,130,3,
    32,226,130,3,32,228,130,3,32,230,130,3,32,232,130,3,32,234,130,229,32,126,130,185,32,50,130,3,32,52,130,59,38,30,0,6,255,242,0,11,130,3,36,90,255,243,0,93,130,3,32,188,130,3,44,245,
    255,245,1,25,255,243,1,108,255,242,1,109,130,3,32,111,130,3,32,112,130,3,40,113,255,242,2,128,255,243,2,129,130,3,36,239,255,243,3,114,130,3,32,149,130,3,32,158,130,3,32,166,130,3,
    36,167,255,242,3,168,130,3,32,171,130,3,32,183,130,15,32,191,130,3,32,224,130,3,32,226,130,3,36,228,255,243,4,65,130,3,32,67,130,3,32,69,130,99,34,62,0,39,130,5,32,43,130,3,32,51,130,
    3,32,53,130,3,32,131,130,3,32,146,130,3,32,151,130,3,32,178,130,3,36,195,0,13,0,210,130,133,32,7,130,3,32,22,130,3,32,26,130,3,32,28,130,3,32,30,130,3,32,32,130,3,32,56,130,3,32,85,
    130,137,32,40,130,3,32,41,130,3,130,75,33,2,44,130,7,32,82,130,3,32,92,130,3,130,205,33,2,94,130,7,32,95,130,3,32,96,130,3,32,136,130,3,32,138,130,3,32,140,130,3,32,142,130,3,32,156,
    130,3,130,185,33,2,160,130,7,32,162,130,3,32,196,130,3,32,198,130,3,32,200,130,3,32,249,130,177,32,86,130,3,32,99,130,3,32,137,130,3,130,51,33,3,185,130,7,32,188,130,3,32,215,130,3,
    32,217,130,3,32,219,130,201,32,26,130,3,32,28,130,3,32,30,130,3,32,32,130,3,32,34,130,3,32,36,130,3,32,38,130,3,32,40,130,3,32,42,130,3,32,44,130,3,32,46,130,3,32,48,130,3,32,169,130,
    215,32,63,130,249,38,230,0,43,255,230,0,51,130,3,32,53,130,3,32,131,130,3,32,146,130,3,32,151,130,3,32,178,130,3,38,183,255,194,0,195,0,16,130,253,38,230,1,7,255,230,1,22,130,3,32,
    26,130,3,32,28,130,3,32,30,130,3,32,32,130,3,32,56,130,3,36,85,255,230,2,40,130,3,32,41,130,3,130,79,130,253,34,230,2,82,130,11,32,92,130,3,32,93,130,3,32,94,130,3,32,95,130,3,32,96,
    130,3,32,136,130,3,32,138,130,3,32,140,130,3,32,142,130,3,32,156,130,3,32,158,130,3,32,160,130,3,32,162,130,3,32,196,130,3,32,198,130,3,32,200,130,3,36,249,255,230,3,86,130,3,32,99,
    130,3,32,137,130,3,130,51,130,253,34,230,3,188,130,11,32,215,130,3,32,217,130,3,36,219,255,230,4,26,130,3,32,28,130,3,32,30,130,3,32,32,130,3,32,34,130,3,32,36,130,3,32,38,130,3,32,
    40,130,3,32,42,130,3,32,44,130,3,32,46,130,3,32,48,130,3,32,169,130,219,46,55,0,37,255,228,0,60,255,210,0,61,255,211,0,177,130,11,32,179,130,3,44,195,255,226,0,217,255,210,1,12,255,
    228,2,75,130,3,32,76,130,3,32,77,130,3,32,78,130,3,32,79,130,3,32,80,130,3,32,81,130,3,36,101,255,211,2,130,130,7,32,132,130,3,32,134,130,3,32,238,130,15,40,240,255,211,3,82,255,228,
    3,87,130,7,32,90,130,7,32,102,130,7,36,103,255,210,3,105,130,7,32,130,130,15,32,142,130,11,32,165,130,11,32,174,130,11,32,190,130,7,32,193,130,15,32,196,130,11,32,198,130,3,32,207,
    130,11,32,233,130,3,32,238,130,11,32,240,130,3,32,242,130,3,32,244,130,3,32,246,130,3,32,248,130,3,32,250,130,3,32,252,130,3,36,254,255,228,4,0,130,3,32,2,130,3,32,4,130,3,36,64,255,
    211,4,66,130,3,32,68,130,3,36,78,255,210,4,86,130,3,130,111,39,0,39,0,16,255,70,0,18,130,3,34,37,255,205,130,221,34,205,0,179,130,7,34,198,255,242,130,217,38,205,1,110,255,70,1,114,
    130,3,32,118,130,3,34,119,255,70,130,233,38,205,2,76,255,205,2,77,130,3,32,78,130,3,32,79,130,3,32,80,130,3,32,81,130,3,32,130,130,3,32,132,130,3,34,134,255,205,130,221,34,205,3,90,
    130,7,32,130,130,3,32,174,130,3,32,196,130,3,32,198,130,3,32,238,130,3,32,240,130,3,32,242,130,3,32,244,130,3,32,246,130,3,32,248,130,3,32,250,130,3,32,252,130,3,36,254,255,205,4,0,
    130,3,32,2,130,3,32,4,130,135,44,1,0,195,0,14,0,175,0,71,255,220,0,72,130,3,32,73,130,3,32,75,130,3,36,81,255,193,0,82,130,3,36,83,255,214,0,84,130,7,32,85,130,19,40,89,255,221,0,90,
    255,225,0,93,130,3,32,147,130,15,32,152,130,3,32,154,130,19,32,186,130,7,32,188,130,19,36,190,255,230,0,192,130,43,48,193,255,235,0,194,255,233,0,196,255,240,0,197,255,231,0,199,130,
    31,32,200,130,3,36,201,255,227,0,202,130,47,44,203,255,206,0,204,255,212,0,205,255,219,0,235,130,47,32,239,130,3,32,240,130,3,32,242,130,3,32,243,130,3,32,244,130,3,32,246,130,47,32,
    247,130,7,32,249,130,3,32,250,130,3,32,253,130,3,40,255,255,193,1,2,255,220,1,4,130,7,40,23,255,214,1,25,255,225,1,29,130,15,32,33,130,3,32,53,130,19,32,57,130,7,32,68,130,7,32,73,
    130,3,32,75,130,11,32,76,130,3,36,86,255,220,2,109,130,3,32,110,130,3,32,111,130,3,32,112,130,3,32,113,130,3,40,118,255,193,2,119,255,214,2,120,130,3,32,121,130,3,32,122,130,3,32,123,
    130,3,36,124,255,221,2,125,130,3,32,126,130,3,32,127,130,3,36,128,255,225,2,129,130,3,32,137,130,51,32,139,130,3,32,141,130,3,32,143,130,3,32,145,130,3,32,147,130,3,32,149,130,3,32,
    151,130,3,32,153,130,3,32,155,130,3,32,157,130,3,32,159,130,3,32,161,130,3,32,163,130,3,32,190,130,103,32,192,130,3,32,194,130,3,32,195,130,3,32,197,130,99,32,199,130,3,32,201,130,
    3,32,225,130,95,32,227,130,3,32,229,130,3,32,231,130,3,32,233,130,3,32,235,130,3,52,239,255,225,3,106,255,220,3,108,255,193,3,110,255,221,3,112,255,214,3,114,130,19,32,117,130,11,32,
    118,130,11,32,119,130,7,32,144,130,31,32,145,130,31,32,146,130,15,32,147,130,7,32,148,130,15,32,149,130,35,32,151,130,7,32,152,130,15,32,157,130,3,32,158,130,15,32,166,130,3,32,173,
    130,11,32,178,130,23,32,179,130,7,32,183,130,15,32,184,130,11,32,189,130,3,32,191,130,11,32,203,130,7,32,205,130,3,32,206,130,3,32,212,130,31,32,214,130,3,32,216,130,83,32,218,130,
    15,32,220,130,7,32,224,130,35,32,226,130,3,32,228,130,3,32,232,130,27,36,235,255,220,4,7,130,3,32,9,130,3,32,11,130,3,32,13,130,3,32,15,130,3,32,17,130,3,32,19,130,3,32,21,130,3,36,
    27,255,214,4,29,130,3,32,31,130,3,32,33,130,3,32,35,130,3,32,37,130,3,32,39,130,3,32,41,130,31,32,43,130,3,32,45,130,3,32,47,130,15,32,49,130,7,36,51,255,221,4,53,130,3,32,55,130,3,
    32,57,130,3,32,59,130,3,32,61,130,3,32,63,130,3,36,65,255,225,4,67,130,3,32,69,130,3,36,73,255,193,4,75,130,3,32,85,130,3,32,98,130,3,32,100,130,3,42,102,255,193,0,118,0,6,255,218,
    0,11,130,3,36,71,255,240,0,72,130,3,32,73,130,3,32,75,130,3,32,85,130,3,40,89,255,239,0,90,255,220,0,93,130,3,32,147,130,15,32,152,130,3,32,154,130,19,32,186,130,7,32,188,130,19,44,
    193,255,236,0,195,0,15,0,197,255,234,0,199,130,19,32,200,130,3,36,201,255,206,0,202,130,35,40,203,255,231,0,246,255,240,1,2,130,3,36,25,255,220,1,29,130,7,32,33,130,3,32,57,130,3,130,
    95,33,1,76,130,7,32,86,130,3,36,108,255,218,1,109,130,3,32,111,130,3,32,112,130,3,40,113,255,218,2,109,255,240,2,110,130,3,32,111,130,3,32,112,130,3,32,113,130,3,36,124,255,239,2,125,
    130,3,32,126,130,3,32,127,130,3,36,128,255,220,2,129,130,3,32,137,130,27,32,139,130,3,32,141,130,3,32,143,130,3,32,145,130,3,130,171,33,2,149,130,7,32,151,130,3,32,153,130,3,32,155,
    130,3,32,157,130,3,32,159,130,3,32,161,130,3,32,163,130,3,32,225,130,67,32,227,130,3,32,229,130,3,32,231,130,3,32,233,130,3,32,235,130,3,44,239,255,220,3,106,255,240,3,110,255,239,
    3,114,130,11,32,117,130,7,32,119,130,3,32,144,130,19,32,148,130,3,32,149,130,19,32,151,130,7,32,158,130,7,32,166,130,3,36,167,255,218,3,168,130,3,32,171,130,3,32,178,130,23,32,183,
    130,19,32,184,130,7,32,189,130,3,32,191,130,11,32,203,130,7,32,205,130,3,32,206,130,3,32,218,130,3,32,224,130,19,32,226,130,3,32,228,130,3,36,235,255,240,4,7,130,3,32,9,130,3,32,11,
    130,3,32,13,130,3,32,15,130,3,32,17,130,3,32,19,130,3,32,21,130,3,32,41,130,3,32,43,130,3,32,45,130,3,32,49,130,3,36,51,255,239,4,53,130,3,32,55,130,3,32,57,130,3,32,59,130,3,32,61,
    130,3,32,63,130,3,36,65,255,220,4,67,130,3,42,69,255,220,0,68,0,16,0,12,0,18,130,3,36,71,255,231,0,72,130,3,32,73,130,3,32,75,130,3,32,85,130,3,32,147,130,3,32,152,130,3,32,186,130,
    3,36,195,0,15,0,199,130,7,32,200,130,3,36,246,255,231,1,2,130,3,32,29,130,3,32,33,130,3,32,57,130,3,130,51,33,1,76,130,7,32,86,130,3,36,110,0,12,1,114,130,3,32,118,130,3,40,119,0,12,
    2,109,255,231,2,110,130,3,32,111,130,3,32,112,130,3,32,113,130,3,32,137,130,3,32,139,130,3,32,141,130,3,32,143,130,3,32,145,130,3,130,111,33,2,149,130,7,32,151,130,3,32,153,130,3,32,
    155,130,3,32,157,130,3,32,159,130,3,32,161,130,3,36,163,255,231,3,106,130,3,32,144,130,3,32,148,130,3,32,151,130,3,32,178,130,3,32,184,130,3,32,189,130,3,32,203,130,3,32,205,130,3,
    32,206,130,3,32,218,130,3,36,235,255,231,4,7,130,3,32,9,130,3,32,11,130,3,32,13,130,3,32,15,130,3,32,17,130,3,32,19,130,3,32,21,130,3,32,41,130,3,32,43,130,3,32,45,130,3,32,49,130,
    219,58,6,0,201,255,234,0,236,255,238,0,245,255,213,0,253,255,237,1,51,255,236,1,88,255,236,0,1,130,17,32,192,130,5,42,201,0,32,0,126,0,6,0,12,0,11,130,3,76,163,11,32,74,130,15,36,75,
    255,232,0,83,130,65,32,85,130,7,32,90,130,33,32,93,130,3,76,175,11,32,188,130,15,36,195,255,144,0,197,130,7,32,199,130,35,32,200,130,3,32,201,130,55,32,246,76,191,6,39,23,255,234,1,
    25,0,11,1,76,199,25,38,12,1,109,0,12,1,111,130,3,32,112,130,3,32,113,130,3,40,186,255,191,1,188,255,238,1,192,130,169,32,200,130,177,32,202,130,7,46,204,255,245,1,205,0,14,1,207,0,
    13,1,210,0,13,76,235,20,36,119,255,234,2,120,130,3,32,121,130,3,32,122,130,3,32,123,130,3,36,128,0,11,2,129,130,3,77,7,54,33,2,197,130,67,32,199,130,3,32,201,130,3,44,239,0,11,3,106,
    255,232,3,112,255,234,3,114,130,11,32,118,130,7,32,144,130,15,32,146,130,7,32,148,130,7,32,149,130,19,32,151,130,7,32,158,130,7,32,166,130,3,36,167,0,12,3,168,130,3,32,171,130,3,32,
    178,130,23,32,183,130,19,77,55,7,32,191,130,11,77,59,11,32,216,130,67,32,218,130,35,32,220,130,7,32,224,130,27,32,226,130,3,32,228,130,3,77,79,35,36,27,255,234,4,29,130,3,32,31,130,
    3,32,33,130,3,32,35,130,3,32,37,130,3,32,39,130,3,77,107,11,32,47,130,15,40,49,255,232,4,65,0,11,4,67,130,3,48,69,0,11,0,1,0,245,255,226,0,13,0,92,255,237,0,94,130,3,32,237,130,3,40,
    245,255,192,2,242,255,237,2,244,130,3,36,246,255,237,3,150,130,3,32,194,130,3,32,208,130,3,36,234,255,237,4,79,130,3,32,87,130,39,32,12,130,53,32,242,130,53,36,242,0,237,255,242,130,
    49,32,242,130,49,36,242,2,246,255,242,130,49,34,242,3,194,130,7,32,208,130,3,34,234,255,242,130,49,42,242,4,87,255,242,0,31,0,90,255,244,132,53,32,93,130,7,36,94,255,243,0,188,130,
    7,130,61,37,1,25,255,244,2,128,130,3,32,129,130,3,32,239,130,3,34,242,255,243,130,77,32,243,130,77,38,243,3,114,255,244,3,149,130,3,32,150,130,77,32,158,130,7,32,166,130,3,32,183,130,
    3,32,191,130,3,135,101,32,224,130,11,32,226,130,3,32,228,130,3,32,234,130,113,36,65,255,244,4,67,130,3,32,69,130,3,32,79,130,15,131,125,38,93,0,6,255,202,0,11,130,3,40,56,255,210,0,
    58,255,212,0,60,130,125,34,61,255,211,130,149,32,230,130,149,36,239,0,93,255,230,130,145,34,230,0,209,130,31,32,213,130,3,32,217,130,31,32,221,130,233,36,224,255,225,0,229,130,47,50,
    237,255,239,0,245,255,201,0,253,255,209,1,8,255,229,1,24,255,212,130,185,46,230,1,31,255,227,1,50,255,210,1,51,255,196,1,58,130,7,36,60,255,225,1,77,130,27,52,78,255,245,1,79,255,231,
    1,87,255,100,1,88,255,201,1,108,255,202,1,109,130,3,32,111,130,3,32,112,130,3,38,113,255,202,2,101,255,211,130,249,42,230,2,129,255,230,2,218,255,210,2,220,130,3,32,222,130,3,32,238,
    130,23,32,239,130,19,36,240,255,211,3,87,130,3,74,147,5,34,244,3,105,130,11,44,114,255,230,3,129,255,237,3,141,255,210,3,142,130,241,32,149,130,15,36,150,255,239,3,158,130,7,32,165,
    130,31,32,166,130,7,36,167,255,202,3,168,130,3,32,171,130,3,36,182,255,212,3,183,130,19,32,190,130,27,32,191,130,7,32,193,130,51,32,194,130,47,32,207,130,7,32,208,130,7,32,223,130,
    75,32,224,130,23,32,225,130,7,32,226,130,7,32,227,130,7,32,228,130,7,36,229,255,225,3,233,130,35,44,234,255,239,4,64,255,211,4,65,255,230,4,66,130,7,32,67,130,7,32,68,130,7,32,69,130,
    7,40,76,255,210,4,78,255,244,4,79,130,35,36,80,255,225,4,82,130,3,32,86,130,15,32,87,130,15,130,171,39,0,108,0,6,255,192,0,11,130,3,52,56,255,157,0,58,255,199,0,60,255,240,0,61,255,
    171,0,81,255,210,0,82,130,3,32,84,130,3,32,192,130,3,32,209,130,31,36,211,255,245,0,213,130,7,32,217,130,35,32,220,130,11,44,221,255,234,0,224,255,229,0,229,255,193,0,235,130,35,32,
    239,130,3,32,240,130,3,32,242,130,3,32,243,130,3,32,244,130,3,36,245,255,205,0,247,130,7,32,249,130,3,32,250,130,3,32,253,130,3,36,255,255,210,1,4,130,3,44,24,255,199,1,50,255,157,
    1,51,255,204,1,53,130,15,32,58,130,11,40,60,255,229,1,63,255,223,1,68,130,15,32,73,130,3,48,77,255,206,1,79,255,234,1,81,255,245,1,87,255,158,1,88,130,15,36,108,255,192,1,109,130,3,
    32,111,130,3,32,112,130,3,44,113,255,192,2,101,255,171,2,118,255,210,2,190,130,3,32,192,130,3,32,194,130,3,32,195,130,3,36,218,255,157,2,220,130,3,32,222,130,3,32,238,130,35,36,240,
    255,171,3,87,130,3,32,102,130,3,36,103,255,240,3,105,130,7,44,108,255,210,3,129,255,234,3,141,255,157,3,142,130,19,32,145,130,15,32,147,130,3,32,152,130,3,32,157,130,3,32,165,130,35,
    36,167,255,192,3,168,130,3,32,171,130,3,32,173,130,19,32,179,130,3,36,182,255,199,3,190,130,27,32,193,130,51,32,207,130,3,32,212,130,19,32,214,130,3,32,223,130,75,32,225,130,3,32,227,
    130,3,36,229,255,229,3,232,130,19,32,233,130,31,40,236,255,245,4,64,255,171,4,66,130,3,32,68,130,3,130,223,49,4,75,255,210,4,76,255,157,4,78,255,240,4,80,255,229,4,82,130,3,32,85,130,
    19,32,86,130,15,32,98,130,7,32,100,130,3,32,102,130,3,32,103,130,59,130,171,39,0,111,0,6,255,177,0,11,130,3,52,56,255,158,0,58,255,197,0,60,255,242,0,61,255,168,0,81,255,207,0,82,130,
    3,32,84,130,3,36,92,255,239,0,192,130,7,32,209,130,35,32,213,130,3,32,217,130,35,35,221,255,236,0,67,43,5,34,194,0,235,130,27,32,237,130,35,32,239,130,7,32,240,130,3,32,242,130,3,32,
    243,130,3,32,244,130,3,36,245,255,198,0,247,130,7,32,249,130,3,32,250,130,3,32,253,130,3,36,255,255,207,1,4,130,3,44,24,255,197,1,50,255,158,1,51,255,192,1,53,130,15,32,58,130,11,34,
    60,255,225,65,177,6,34,207,1,73,130,19,52,77,255,205,1,79,255,232,1,87,255,159,1,88,255,198,1,108,255,177,1,109,130,3,32,111,130,3,32,112,130,3,44,113,255,177,2,101,255,168,2,118,255,
    207,2,190,130,3,32,192,130,3,32,194,130,3,32,195,130,3,36,218,255,158,2,220,130,3,32,222,130,3,32,238,130,35,36,240,255,168,3,87,130,3,32,102,130,3,36,103,255,242,3,105,130,7,44,108,
    255,207,3,129,255,236,3,141,255,158,3,142,130,19,32,145,130,15,32,147,130,3,36,150,255,239,3,152,130,7,32,157,130,3,32,165,130,39,36,167,255,177,3,168,130,3,32,171,130,3,32,173,130,
    19,32,179,130,3,36,182,255,197,3,190,130,27,32,193,130,55,67,99,5,34,242,3,208,130,55,32,212,130,27,32,214,130,3,32,223,130,87,32,225,130,3,32,227,130,3,36,229,255,225,3,232,130,19,
    32,233,130,43,67,99,5,38,168,4,66,255,168,4,68,130,3,130,231,43,4,75,255,207,4,76,255,158,4,78,255,242,67,95,12,32,85,130,23,32,86,130,19,36,87,255,239,4,98,130,11,32,100,130,3,32,
    102,130,3,130,187,43,0,77,0,56,255,190,0,81,255,225,0,82,130,3,32,84,130,3,36,90,255,239,0,93,130,3,32,188,130,3,32,192,130,15,32,209,130,31,32,213,130,3,36,229,255,201,0,235,130,15,
    32,239,130,3,32,240,130,3,32,242,130,3,32,243,130,3,32,244,130,3,36,245,255,223,0,247,130,7,32,249,130,3,32,250,130,3,32,253,130,3,36,255,255,225,1,4,130,3,52,8,255,237,1,25,255,239,
    1,31,255,235,1,50,255,190,1,51,255,223,1,53,130,23,32,58,130,11,36,63,255,233,1,68,130,11,32,73,130,3,48,78,255,245,1,88,255,224,2,118,255,225,2,128,255,239,2,129,130,3,32,190,130,
    11,32,192,130,3,32,194,130,3,32,195,130,3,36,218,255,190,2,220,130,3,32,222,130,3,40,239,255,239,3,108,255,225,3,114,130,7,36,141,255,190,3,145,130,11,32,147,130,3,32,149,130,15,32,
    152,130,7,32,157,130,3,32,158,130,11,32,166,130,3,32,173,130,11,32,179,130,3,32,183,130,11,32,191,130,3,32,212,130,11,32,214,130,3,32,224,130,11,32,226,130,3,32,228,130,3,40,232,255,
    225,4,65,255,239,4,67,130,3,32,69,130,3,130,147,33,4,75,130,19,36,76,255,190,4,85,130,7,32,98,130,3,32,100,130,3,32,102,130,219,40,100,0,56,255,230,0,58,255,231,66,235,6,38,231,0,81,
    255,214,0,82,130,3,32,84,130,3,36,92,255,241,0,192,130,7,32,209,130,35,32,213,130,3,66,235,5,38,238,0,224,255,232,0,229,130,15,32,235,130,27,32,237,130,35,32,239,130,7,32,240,130,3,
    32,242,130,3,32,243,130,3,32,244,130,3,36,245,255,208,0,247,130,7,32,249,130,3,32,250,130,3,32,253,130,3,36,255,255,214,1,4,130,3,44,24,255,231,1,50,255,230,1,51,255,206,1,53,130,15,
    32,58,130,11,36,60,255,232,1,68,130,11,32,73,130,3,32,77,130,31,36,79,255,237,1,87,130,23,44,88,255,208,2,101,255,231,2,118,255,214,2,190,130,3,32,192,130,3,32,194,130,3,32,195,130,
    3,36,218,255,230,2,220,130,3,32,222,130,3,32,238,130,35,36,240,255,231,3,87,130,3,32,102,130,3,66,211,5,44,231,3,108,255,214,3,129,255,238,3,141,255,230,66,211,6,34,214,3,147,130,19,
    36,150,255,241,3,152,130,7,32,157,130,3,32,165,130,47,32,173,130,7,32,179,130,3,32,182,130,11,32,190,130,3,66,199,5,38,241,3,207,255,242,3,208,130,43,32,212,130,27,32,214,130,3,32,
    223,130,75,32,225,130,3,32,227,130,3,36,229,255,232,3,232,130,19,66,199,5,38,241,4,64,255,231,4,66,130,3,32,68,130,3,130,199,39,4,75,255,214,4,76,255,230,66,199,6,38,241,4,80,255,232,
    4,82,130,3,32,85,130,23,66,199,5,34,241,4,98,130,11,32,100,130,3,32,102,130,3,46,105,255,231,0,147,0,37,0,16,0,39,255,232,0,43,130,3,32,51,130,3,32,53,130,3,36,56,255,224,0,58,130,
    3,36,61,255,223,0,131,130,15,32,146,130,3,32,151,130,3,32,177,130,43,32,178,130,7,32,179,130,7,32,209,130,31,32,210,130,11,32,211,130,11,32,213,130,11,36,216,0,20,0,220,130,11,68,153,
    5,38,224,0,236,0,19,0,241,130,15,44,248,255,224,1,3,0,16,1,7,255,232,1,12,130,7,32,22,130,7,32,24,130,19,32,26,130,7,32,28,130,3,32,30,130,3,32,32,130,3,32,50,130,19,32,56,130,7,32,
    58,130,7,36,60,255,225,1,61,130,7,32,64,130,7,44,69,255,233,1,77,255,223,1,79,255,222,1,81,130,63,32,85,130,35,32,87,130,15,40,89,255,242,2,40,255,232,2,41,130,3,130,187,33,2,44,130,
    7,36,75,0,16,2,76,130,3,32,77,130,3,32,78,130,3,32,79,130,3,32,80,130,3,130,55,33,2,82,130,31,32,92,130,3,32,93,130,3,32,94,130,3,32,95,130,3,32,96,130,3,36,101,255,223,2,130,130,35,
    32,132,130,3,32,134,130,3,32,136,130,19,32,138,130,3,32,140,130,3,32,142,130,3,32,156,130,3,32,158,130,3,32,160,130,3,32,162,130,3,32,196,130,3,32,198,130,3,32,200,130,3,36,218,255,
    224,2,220,130,3,32,222,130,3,32,238,130,71,32,240,130,3,40,249,255,232,3,82,0,16,3,86,130,7,130,167,33,3,90,130,11,32,99,130,11,36,102,255,223,3,105,130,3,130,107,33,3,137,130,15,130,
    95,37,3,141,255,224,3,165,130,19,32,174,130,35,32,182,130,11,32,185,130,23,32,188,130,3,32,190,130,19,32,196,130,19,32,198,130,3,32,215,130,15,32,217,130,3,32,219,130,3,36,229,255,
    225,3,230,130,39,32,236,130,23,32,237,130,3,32,238,130,3,32,240,130,3,32,242,130,3,32,244,130,3,32,246,130,3,32,248,130,3,32,250,130,3,32,252,130,3,36,254,0,16,4,0,130,3,32,2,130,3,
    32,4,130,3,36,26,255,232,4,28,130,3,32,30,130,3,32,32,130,3,32,34,130,3,32,36,130,3,32,38,130,3,32,40,130,3,32,42,130,3,32,44,130,3,32,46,130,3,32,48,130,3,36,64,255,223,4,66,130,3,
    32,68,130,3,40,76,255,224,4,80,255,225,4,81,130,7,32,82,130,7,32,83,130,7,32,103,130,83,32,104,130,3,130,215,49,4,169,255,232,0,50,0,27,255,242,0,56,255,241,0,58,255,244,73,247,6,34,
    240,0,209,130,15,72,113,5,34,241,0,217,130,23,72,113,5,42,243,0,229,255,241,1,24,255,244,1,50,130,7,32,58,130,3,36,77,255,242,1,79,130,3,72,33,5,42,242,2,101,255,240,2,218,255,241,
    2,220,130,3,32,222,130,3,32,238,130,15,36,240,255,240,3,87,130,3,32,102,130,3,36,103,255,244,3,105,130,7,40,129,255,243,3,141,255,241,3,142,130,15,32,165,130,15,32,182,130,7,32,190,
    130,7,32,193,130,7,32,207,130,3,32,223,130,31,32,225,130,3,32,227,130,3,32,233,130,15,71,189,5,32,240,130,217,46,240,4,68,255,240,4,76,255,241,4,78,255,244,4,86,130,3,35,103,255,245,
    4,130,83,37,0,102,0,37,0,15,130,201,32,230,130,201,42,230,0,60,0,14,0,61,255,230,0,177,130,19,32,179,130,3,32,209,130,11,32,211,130,19,32,213,130,7,36,216,0,19,0,217,130,11,32,220,
    130,3,34,221,0,11,73,71,6,42,230,0,230,255,244,0,236,0,18,0,241,130,47,48,245,255,231,0,248,255,232,0,253,255,231,1,3,0,15,1,12,130,3,34,24,255,230,68,149,6,34,231,1,58,130,11,40,60,
    255,229,1,61,255,232,1,77,130,11,32,79,130,3,34,81,0,14,68,145,6,38,231,2,75,0,15,2,76,130,3,32,77,130,3,32,78,130,3,32,79,130,3,32,80,130,3,32,81,130,3,36,101,255,230,2,130,130,7,
    32,132,130,3,32,134,130,3,68,165,13,42,230,2,240,255,230,3,82,0,15,3,87,130,7,32,90,130,7,32,102,130,7,36,103,0,14,3,105,130,7,35,129,0,11,3,130,59,33,3,141,130,11,32,142,130,19,32,
    165,130,7,32,174,130,35,32,182,130,7,32,190,130,3,32,193,130,19,32,196,130,15,32,198,130,3,32,207,130,11,32,223,130,47,32,225,130,3,32,227,130,3,40,229,255,229,3,230,255,232,3,233,
    130,23,32,236,130,3,32,237,130,35,32,238,130,3,32,240,130,3,32,242,130,3,32,244,130,3,32,246,130,3,32,248,130,3,32,250,130,3,32,252,130,3,36,254,0,15,4,0,130,3,32,2,130,3,32,4,130,
    3,36,64,255,230,4,66,130,3,32,68,130,3,32,76,130,3,44,78,0,14,4,80,255,229,4,81,255,232,4,82,130,7,32,83,130,7,32,86,130,19,130,175,33,4,104,130,47,130,179,39,0,55,0,6,255,191,0,11,
    130,3,44,56,255,159,0,58,255,201,0,61,255,173,0,209,130,11,32,213,130,3,73,27,5,58,230,0,229,255,196,0,245,255,205,0,253,255,213,1,24,255,201,1,50,255,159,1,51,255,204,1,58,130,7,60,
    60,255,230,1,63,255,223,1,77,255,209,1,79,255,236,1,87,255,161,1,88,255,207,1,108,255,191,1,109,130,3,32,111,130,3,32,112,130,3,44,113,255,191,2,101,255,173,2,218,255,159,2,220,130,
    3,32,222,130,3,32,238,130,15,36,240,255,173,3,87,130,3,32,102,130,3,32,105,130,3,40,129,255,236,3,141,255,159,3,165,130,11,36,167,255,191,3,168,130,3,32,171,130,3,36,182,255,201,3,
    190,130,19,72,139,13,36,230,4,64,255,173,130,245,34,173,4,68,130,7,34,76,255,159,130,241,32,230,130,237,33,230,4,130,75,41,0,48,0,56,255,227,0,60,255,229,130,213,32,228,130,213,34,
    227,0,211,130,11,32,213,130,19,36,216,255,226,0,217,130,11,32,220,130,3,40,221,255,233,0,241,255,234,1,3,130,3,34,50,255,227,130,213,40,227,1,81,255,229,1,87,255,228,130,177,32,228,
    130,177,32,227,130,177,38,227,2,222,255,227,2,238,130,19,35,240,255,228,3,130,27,33,3,102,130,7,36,103,255,229,3,105,130,7,34,129,255,233,130,181,34,227,3,142,130,15,32,165,130,15,
    32,190,130,3,32,193,130,11,32,207,130,3,32,223,130,27,32,225,130,3,32,227,130,3,32,233,130,15,32,236,130,3,34,237,255,234,130,185,36,228,4,66,255,228,130,185,42,228,4,76,255,227,4,
    78,255,229,4,86,130,3,130,83,33,4,104,130,31,130,87,33,0,35,130,193,32,226,130,193,131,189,32,226,130,189,34,228,0,213,130,185,34,216,255,225,130,189,37,228,0,220,255,228,0,131,189,
    32,236,130,7,34,241,255,235,130,193,36,235,1,50,255,226,130,193,32,226,130,193,131,185,36,226,2,220,255,226,130,185,34,226,3,103,136,165,32,226,130,165,34,228,3,193,130,15,32,207,130,
    3,141,157,33,228,3,130,75,35,3,237,255,235,130,145,32,226,130,145,32,228,130,145,33,228,4,130,59,130,145,34,235,0,23,130,141,40,235,0,61,255,243,0,209,255,235,130,137,131,109,32,235,
    130,109,40,235,2,101,255,243,2,218,255,235,130,109,32,235,130,109,34,235,2,238,130,15,36,240,255,243,3,87,130,3,32,102,130,3,32,105,130,3,36,141,255,235,3,165,130,7,34,190,255,243,
    130,251,32,243,130,251,32,243,130,251,32,243,130,105,33,235,4,130,31,39,0,54,0,81,255,239,0,82,130,3,32,84,130,3,36,92,255,240,0,192,130,7,32,235,130,3,36,236,255,238,0,237,130,15,
    32,239,130,11,32,240,130,3,32,242,130,3,32,243,130,3,32,244,130,3,32,245,130,27,32,247,130,7,32,249,130,3,32,250,130,3,32,253,130,3,36,255,255,239,1,4,130,3,40,8,255,244,1,31,255,241,
    1,51,130,11,32,53,130,3,32,68,130,3,32,73,130,3,36,88,255,239,2,118,130,3,32,190,130,3,32,192,130,3,32,194,130,3,36,195,255,239,3,108,130,3,32,145,130,3,32,147,130,3,36,150,255,240,
    3,152,130,7,32,157,130,3,32,173,130,3,32,179,130,3,32,194,130,19,32,208,130,3,32,212,130,11,32,214,130,3,32,232,130,3,35,234,255,240,4,130,83,37,4,75,255,239,4,79,130,11,32,85,130,
    7,32,87,130,7,32,98,130,7,32,100,130,3,32,102,130,143,32,34,92,111,10,38,245,0,93,255,245,0,188,130,3,36,245,255,244,0,253,130,161,36,8,255,245,1,25,130,3,32,51,130,3,32,88,130,3,92,
    127,21,42,245,2,129,255,245,2,239,255,245,3,114,130,3,32,149,130,3,32,158,130,3,32,166,130,3,92,127,13,34,245,3,191,130,19,32,224,130,3,32,226,130,3,36,228,255,245,4,65,130,3,32,67,
    130,3,32,69,130,115,38,50,0,81,255,238,0,82,130,3,32,84,130,3,32,192,130,3,32,235,130,3,36,236,0,20,0,239,130,7,32,240,130,3,32,242,130,3,32,243,130,3,32,244,130,3,36,245,255,237,0,
    247,130,7,32,248,130,7,32,249,130,7,32,250,130,3,34,251,255,208,130,181,38,238,0,255,255,238,1,4,130,3,36,51,255,237,1,53,130,7,32,61,130,7,32,68,130,7,32,73,130,3,40,88,255,237,2,
    118,255,238,2,190,130,3,32,192,130,3,32,194,130,3,36,195,255,238,3,108,130,3,32,145,130,3,32,147,130,3,32,152,130,3,32,157,130,3,32,173,130,3,32,179,130,3,32,212,130,3,32,214,130,3,
    39,230,255,237,3,232,255,238,4,130,71,33,4,75,130,7,36,81,255,237,4,83,130,3,32,85,130,11,32,98,130,3,32,100,130,3,32,102,130,135,34,10,0,6,130,207,36,11,255,245,1,108,130,3,32,109,
    130,3,32,111,130,3,32,112,130,3,36,113,255,245,3,167,130,3,32,168,130,3,32,171,130,35,33,89,0,87,51,15,40,83,255,199,0,85,255,240,0,147,130,3,32,152,130,3,32,186,130,3,32,199,87,23,
    6,33,246,255,87,11,5,38,23,255,199,1,27,255,235,87,15,24,32,188,130,27,36,192,255,233,1,200,130,7,34,202,255,235,87,11,20,36,119,255,199,2,120,130,3,32,121,130,3,32,122,130,3,32,123,
    130,3,87,7,55,32,197,130,59,32,199,130,3,40,201,255,199,3,106,255,240,3,112,130,7,32,118,130,3,32,144,130,11,32,146,130,7,32,148,130,7,32,151,130,3,32,178,130,3,86,215,7,86,211,11,
    32,216,130,35,32,218,130,27,32,220,130,7,86,207,35,36,27,255,199,4,29,130,3,32,31,130,3,32,33,130,3,32,35,130,3,32,37,130,3,32,39,130,3,86,235,11,32,47,130,15,42,49,255,240,0,161,0,
    6,0,13,0,11,130,3,32,69,130,13,36,71,255,192,0,72,130,3,32,73,130,3,32,74,130,19,32,75,130,7,36,83,255,226,0,85,130,7,85,147,9,34,192,0,152,130,15,32,186,130,3,32,188,130,57,36,198,
    255,214,0,199,130,11,32,200,130,3,48,203,255,213,0,236,255,200,0,241,255,215,0,246,255,192,1,2,130,3,48,3,255,215,1,23,255,226,1,25,0,11,1,27,255,236,1,29,130,19,36,31,0,12,1,33,130,
    7,32,57,130,3,130,99,33,1,76,130,7,32,78,130,31,32,80,130,3,32,86,130,11,36,108,0,13,1,109,130,3,32,111,130,3,32,112,130,3,32,113,130,3,85,171,35,36,102,255,240,2,103,130,3,32,104,
    130,3,32,105,130,3,32,106,130,3,32,107,130,3,32,108,130,3,36,109,255,192,2,110,130,3,32,111,130,3,32,112,130,3,32,113,130,3,97,1,19,85,199,7,32,131,130,51,32,133,130,3,32,135,130,3,
    32,137,130,43,32,139,130,3,32,141,130,3,32,143,130,3,32,145,130,3,32,147,130,3,32,149,130,3,32,151,130,3,32,153,130,3,32,155,130,3,32,157,130,3,32,159,130,3,32,161,130,3,32,163,130,
    3,97,77,10,32,2,85,211,5,36,192,3,112,255,226,85,211,6,42,226,3,143,255,240,3,144,255,192,3,146,130,19,32,148,130,7,85,215,5,33,192,3,85,215,9,38,13,3,168,0,13,3,171,130,3,32,175,130,
    43,32,178,130,35,85,219,5,34,192,3,189,130,11,36,191,0,11,3,197,130,23,32,199,130,3,32,203,130,15,32,205,130,3,32,206,130,3,32,216,130,79,32,218,130,7,32,220,130,7,85,227,13,38,192,
    3,237,255,215,3,239,130,47,32,241,130,3,32,243,130,3,32,245,130,3,32,247,130,3,32,249,130,3,32,251,130,3,32,253,130,3,36,255,255,240,4,1,130,3,32,3,130,3,32,5,130,3,36,7,255,192,4,
    9,130,3,32,11,130,3,32,13,130,3,32,15,130,3,32,17,130,3,32,19,130,3,32,21,130,3,98,21,27,32,41,130,31,32,43,130,3,32,45,130,3,36,47,255,226,4,49,130,7,86,23,10,46,4,104,255,215,0,15,
    0,236,0,20,0,241,0,16,0,130,141,36,0,248,255,240,0,130,133,49,1,0,0,22,1,3,0,16,1,51,255,230,1,61,255,220,1,88,130,153,32,230,130,3,36,237,0,16,4,81,130,149,32,83,130,3,32,104,130,
    51,38,76,0,71,255,238,0,72,130,3,32,73,130,3,32,75,130,3,32,85,130,3,32,147,130,3,32,152,130,3,32,186,130,3,32,199,130,3,32,200,130,3,34,236,0,18,130,101,32,14,130,101,34,227,0,246,
    130,15,38,248,255,227,0,251,255,184,130,109,36,227,1,2,255,238,130,109,34,14,1,29,130,7,32,33,130,3,36,51,255,186,1,57,130,7,35,61,255,217,1,130,83,33,1,76,130,11,32,86,130,3,40,88,
    255,227,2,109,255,238,2,110,130,3,32,111,130,3,32,112,130,3,32,113,130,3,32,137,130,3,32,139,130,3,32,141,130,3,32,143,130,3,32,145,130,3,130,131,33,2,149,130,7,32,151,130,3,32,153,
    130,3,32,155,130,3,32,157,130,3,32,159,130,3,32,161,130,3,36,163,255,238,3,106,130,3,32,144,130,3,32,148,130,3,32,151,130,3,32,178,130,3,32,184,130,3,32,189,130,3,32,203,130,3,32,205,
    130,3,32,206,130,3,32,218,130,3,36,230,255,227,3,235,130,7,40,237,0,14,4,7,255,238,4,9,130,3,32,11,130,3,32,13,130,3,32,15,130,3,32,17,130,3,32,19,130,3,32,21,130,3,32,41,130,3,32,
    43,130,3,32,45,130,3,32,49,130,3,36,81,255,227,4,83,130,3,42,104,0,14,0,32,0,90,255,192,0,93,130,3,32,188,130,3,60,245,255,128,0,248,255,238,0,253,255,240,1,8,255,219,1,25,255,192,
    1,31,255,220,1,51,255,71,1,61,130,241,48,78,0,7,1,80,255,244,1,88,255,127,2,128,255,192,2,129,130,3,36,239,255,192,3,114,130,3,32,149,130,3,32,158,130,3,32,166,130,3,32,183,130,3,32,
    191,130,3,32,224,130,3,32,226,130,3,32,228,130,3,32,230,130,121,36,65,255,192,4,67,130,3,32,69,130,3,32,81,130,15,32,83,130,107,32,33,87,159,6,32,240,130,133,48,244,0,188,255,244,0,
    236,255,239,0,237,255,240,0,241,255,243,130,137,36,238,1,3,255,243,87,171,15,87,159,10,33,240,3,87,159,17,37,240,3,208,255,240,3,87,159,13,37,240,3,237,255,243,4,87,163,13,38,240,4,
    87,255,240,4,104,130,103,42,10,0,6,255,214,0,11,255,214,1,108,130,3,32,109,130,3,32,111,130,3,32,112,130,3,36,113,255,214,3,167,130,3,32,168,130,3,32,171,130,35,36,21,0,92,255,224,
    130,159,40,224,0,245,255,118,0,248,255,194,130,163,8,34,211,1,8,255,217,1,31,255,219,1,51,255,30,1,61,255,237,1,78,255,240,1,80,255,242,1,88,255,86,3,150,255,224,3,194,130,3,32,208,
    130,3,40,230,255,194,3,234,255,224,4,79,130,3,36,81,255,194,4,83,130,3,32,87,130,79,32,13,130,77,32,100,130,77,32,210,130,77,32,217,156,77,36,230,255,210,4,81,130,3,36,83,255,210,0,
    9,130,53,32,106,130,49,32,198,155,49,32,0,131,219,32,215,130,219,32,215,130,219,38,215,1,109,255,215,1,111,130,3,32,112,130,3,34,113,255,215,130,219,34,215,3,168,130,7,42,171,255,215,
    0,92,0,71,255,152,0,72,130,3,32,73,130,3,32,75,130,3,36,83,255,112,0,85,130,7,40,87,255,24,0,91,0,11,0,147,130,11,32,152,130,3,32,186,130,3,32,199,130,3,32,200,130,3,36,246,255,152,
    1,2,130,3,36,23,255,112,1,29,130,7,32,33,130,3,32,57,130,3,130,63,33,1,76,130,7,36,86,255,152,2,109,130,3,32,110,130,3,32,111,130,3,32,112,130,3,32,113,130,3,36,119,255,112,2,120,130,
    3,32,121,130,3,32,122,130,3,32,123,130,3,32,137,130,23,32,139,130,3,32,141,130,3,32,143,130,3,32,145,130,3,130,115,33,2,149,130,7,32,151,130,3,32,153,130,3,32,155,130,3,32,157,130,
    3,32,159,130,3,32,161,130,3,32,163,130,3,32,197,130,59,32,199,130,3,32,201,130,3,36,209,255,24,2,211,130,3,32,213,130,3,32,215,130,3,44,217,255,24,3,106,255,152,3,112,255,112,3,118,
    130,3,32,144,130,11,32,146,130,7,32,148,130,7,32,151,130,3,32,153,130,31,32,178,130,7,32,184,130,3,32,189,130,3,32,203,130,3,32,205,130,3,32,206,130,3,32,216,130,39,32,218,130,7,32,
    220,130,7,36,235,255,152,4,7,130,3,32,9,130,3,32,11,130,3,32,13,130,3,32,15,130,3,32,17,130,3,32,19,130,3,32,21,130,3,36,27,255,112,4,29,130,3,32,31,130,3,32,33,130,3,32,35,130,3,32,
    37,130,3,32,39,130,3,32,41,130,31,32,43,130,3,32,45,130,3,32,47,130,15,42,49,255,152,0,9,1,188,255,242,1,192,130,3,32,200,130,3,32,202,130,3,52,205,255,192,1,206,255,236,1,207,255,
    199,1,208,255,216,1,210,255,191,0,2,130,13,32,238,130,13,32,245,130,9,72,175,6,45,0,7,1,200,255,239,1,202,255,240,1,205,255,187,134,49,32,183,130,35,32,213,130,49,34,180,0,4,130,21,
    32,238,130,53,34,241,1,209,130,75,34,210,255,234,132,17,32,233,130,17,32,235,130,35,32,241,130,35,32,229,132,17,32,242,132,35,34,208,255,245,130,17,32,238,130,93,92,235,6,41,0,11,0,
    91,255,204,1,186,0,19,130,159,32,243,130,159,32,241,130,109,32,242,130,109,32,242,130,87,35,189,1,206,255,131,91,32,184,130,73,32,215,130,55,52,183,0,4,0,74,0,20,0,88,0,50,0,91,0,17,
    1,109,0,16,0,8,130,63,40,229,0,183,255,203,0,204,255,228,130,71,32,13,130,71,32,237,130,71,32,235,130,71,32,236,130,71,32,236,130,107,38,16,0,11,1,87,255,230,130,43,8,48,88,0,14,0,
    129,254,215,0,195,255,152,0,198,255,199,0,216,255,18,0,236,255,82,1,74,255,207,1,186,255,128,0,9,0,13,0,15,0,65,0,12,0,86,255,235,0,97,0,14,130,21,32,203,130,81,32,233,130,81,32,231,
    130,81,32,231,130,81,34,231,0,1,130,125,32,11,132,43,32,20,130,43,32,17,130,43,32,226,130,43,32,19,130,43,32,180,130,43,32,217,130,43,32,217,130,43,32,217,130,43,32,217,130,177,32,
    13,130,121,43,65,255,244,0,97,255,239,1,64,255,237,0,94,181,11,32,214,94,181,12,32,18,130,143,40,174,0,229,0,18,0,234,255,224,130,151,62,173,0,238,255,214,0,252,255,223,1,0,255,210,
    1,6,255,224,1,27,255,206,1,43,255,221,1,45,255,226,1,49,130,15,32,55,130,3,34,61,255,233,130,87,32,218,130,195,34,189,1,84,130,43,62,87,0,17,0,29,0,35,255,175,0,88,255,239,0,91,255,
    223,0,153,255,238,0,183,255,229,0,184,255,209,0,195,130,29,46,201,255,200,0,216,0,19,0,229,255,197,0,245,255,202,130,131,48,208,1,51,255,129,1,60,255,101,1,61,255,133,1,63,255,102,
    130,81,52,221,1,69,255,242,1,77,255,177,1,79,255,202,1,87,255,169,1,88,255,200,130,219,32,245,130,219,46,245,1,205,255,199,1,206,255,241,1,207,255,205,1,208,130,145,36,210,255,196,
    0,8,130,77,71,71,5,32,8,130,25,34,31,255,243,130,85,34,241,1,78,130,7,32,80,130,3,38,88,255,241,0,5,0,74,130,139,34,91,255,234,130,55,32,240,130,55,38,237,1,210,255,240,0,2,130,55,
    38,245,1,109,255,192,0,9,95,209,6,32,184,130,17,36,226,1,8,255,240,77,149,6,32,235,130,69,32,245,130,127,32,236,130,37,58,144,0,1,1,186,255,235,0,6,0,74,0,13,0,197,0,11,0,198,255,234,
    0,201,0,12,0,236,130,159,32,27,130,101,34,58,0,4,130,141,34,86,255,191,130,253,8,36,209,0,109,255,108,0,124,255,110,0,129,255,67,0,134,255,172,0,137,255,161,0,183,255,184,0,190,255,
    126,0,194,255,123,0,197,255,155,130,65,32,121,130,121,34,178,0,203,130,19,54,204,255,125,0,205,255,124,0,216,255,175,0,229,0,15,0,233,255,228,0,234,255,160,130,93,36,116,0,238,255,
    128,130,153,34,178,0,252,130,35,50,253,255,178,0,254,255,128,1,0,255,121,1,1,0,40,1,6,255,125,130,177,50,127,1,27,255,102,1,31,255,218,1,43,255,129,1,45,255,152,1,49,130,23,58,51,255,
    179,1,55,255,160,1,61,255,124,1,63,255,154,1,64,255,108,1,69,255,230,1,74,255,107,130,217,52,146,1,80,255,173,1,84,255,123,1,87,0,15,1,88,255,145,1,89,255,242,130,227,38,175,1,188,
    255,185,1,192,130,3,32,200,130,3,32,202,130,3,40,204,255,188,1,205,255,241,1,208,130,3,36,209,255,237,0,2,130,149,32,104,130,113,40,238,0,23,0,183,255,212,0,193,130,19,65,225,5,32,
    224,130,203,36,231,0,204,255,229,130,203,36,238,0,216,0,18,130,199,32,233,130,187,36,215,1,51,255,215,130,135,32,211,130,135,32,214,130,135,32,197,130,135,40,231,1,77,0,13,1,79,0,12,
    130,123,32,214,132,123,33,188,255,66,241,11,34,233,0,1,130,99,32,241,130,109,44,245,255,214,1,109,255,136,0,10,0,229,255,195,130,77,36,207,0,253,255,212,130,81,36,206,1,60,255,231,
    81,221,14,32,160,130,77,62,209,0,48,0,86,255,126,0,91,255,157,0,109,254,241,0,124,254,244,0,129,254,171,0,134,255,94,0,137,255,75,130,179,52,114,0,190,255,15,0,194,255,10,0,197,255,
    65,0,198,255,7,0,201,255,104,130,187,32,15,130,187,32,14,130,187,40,12,0,216,255,99,0,229,0,5,130,191,44,189,0,234,255,73,0,236,254,254,0,238,255,19,130,125,36,104,0,252,255,14,130,
    129,8,38,104,0,254,255,19,1,0,255,7,1,1,0,48,1,6,255,14,1,8,255,17,1,27,254,231,1,31,255,172,1,43,255,21,1,45,255,60,1,49,130,23,8,52,51,255,106,1,55,255,73,1,61,255,12,1,63,255,63,
    1,64,254,241,1,69,255,192,1,74,254,239,1,78,255,49,1,80,255,95,1,84,255,10,1,87,0,5,1,88,255,48,1,89,255,213,0,20,130,189,36,193,0,183,255,197,130,153,32,180,130,133,32,215,130,121,
    32,185,130,117,32,233,130,101,44,178,1,27,255,210,1,31,255,200,1,51,255,160,130,85,32,197,130,77,32,228,130,73,32,204,130,73,32,204,130,65,32,203,130,65,40,239,1,188,255,232,1,192,
    255,230,68,63,8,44,8,0,216,0,21,0,236,0,21,1,60,255,228,130,53,32,229,130,139,52,228,1,77,255,227,1,79,255,226,1,87,255,228,0,34,0,10,255,226,0,13,130,123,48,14,255,207,0,65,0,18,0,
    74,255,234,0,86,255,216,0,88,130,7,54,97,0,19,0,109,255,174,0,124,255,205,0,129,255,160,0,134,255,193,0,137,255,192,130,163,34,208,0,187,130,31,36,190,255,198,0,191,130,61,46,193,255,
    233,0,194,255,214,0,197,255,232,0,198,255,186,130,191,52,233,0,203,255,203,0,204,255,218,0,205,255,199,1,117,255,211,1,186,255,171,130,159,32,205,130,159,32,203,130,159,42,203,1,202,
    255,203,1,205,255,243,1,208,130,3,50,209,255,239,0,9,0,129,255,223,0,180,255,243,0,182,255,240,0,195,130,93,32,216,130,15,34,229,255,224,130,167,32,224,130,61,8,37,237,1,209,255,245,
    0,2,7,138,0,4,0,0,10,94,18,54,0,33,0,29,0,0,255,219,255,136,255,206,255,197,255,236,255,165,255,164,0,170,0,33,254,227,170,44,140,42,33,255,136,133,14,35,255,208,255,244,130,129,32,
    235,130,129,50,239,255,179,255,217,255,106,255,245,255,206,0,12,0,17,255,201,0,18,130,189,133,38,154,5,33,255,229,130,63,32,232,130,3,32,201,141,36,33,255,243,141,15,145,13,32,255,
    170,189,33,255,235,139,63,37,255,171,0,0,255,234,130,3,32,213,133,21,33,255,225,133,7,39,0,0,255,134,255,234,255,233,135,13,135,7,36,255,237,0,0,255,130,3,130,15,32,20,130,3,132,2,
    35,255,239,255,230,132,8,153,4,32,18,139,26,135,173,33,255,228,134,21,32,17,130,9,32,17,131,253,130,15,32,17,130,3,138,2,131,155,138,14,174,10,131,151,131,87,137,213,37,0,0,255,233,
    255,216,135,70,33,255,163,136,9,32,92,135,9,35,254,224,0,19,135,11,49,0,0,255,192,255,51,255,232,255,50,255,163,254,233,255,242,255,133,137,25,147,9,37,255,78,255,245,255,243,130,53,
    65,177,17,32,15,130,21,32,111,130,3,32,167,131,55,37,254,108,255,205,255,220,130,13,32,72,131,13,131,3,37,255,136,255,88,255,167,130,1,38,48,255,180,255,228,0,16,130,19,54,16,0,15,
    0,16,255,191,255,174,255,196,255,203,0,0,255,126,255,124,0,0,254,254,130,25,40,0,254,240,255,40,255,240,255,179,131,11,41,255,181,255,210,255,212,0,0,255,210,141,119,35,255,228,255,
    245,131,31,135,3,33,255,41,131,9,33,255,99,131,5,133,3,39,255,213,255,223,255,225,0,0,65,29,6,32,14,133,21,65,223,8,135,14,34,0,255,113,131,10,33,255,196,131,5,137,3,37,255,230,0,0,
    255,235,130,3,32,231,132,19,132,61,33,255,235,132,75,32,17,130,17,34,17,255,209,130,5,132,2,33,255,100,132,6,132,4,35,255,106,255,193,130,211,32,216,130,3,50,198,255,227,0,17,255,160,
    0,18,0,17,0,18,255,217,255,236,255,226,132,34,132,4,41,255,25,0,13,0,0,255,104,255,160,130,231,66,133,5,131,15,32,235,130,19,131,3,66,111,10,33,255,237,66,57,6,132,55,145,4,33,255,
    239,145,19,135,17,33,255,241,135,9,147,7,67,85,27,33,255,245,133,59,33,255,242,143,57,131,23,65,121,13,143,33,155,15,145,79,137,45,68,7,29,33,255,176,137,41,173,9,33,255,168,153,47,
    35,255,241,255,240,131,29,133,5,131,9,135,3,33,255,235,130,9,38,16,0,0,255,226,255,237,130,5,32,220,130,13,141,2,67,243,13,32,83,141,28,33,0,15,130,51,32,241,131,209,65,131,25,33,255,
    215,131,51,33,255,89,131,5,143,3,35,255,236,0,0,67,199,9,143,29,155,15,131,201,145,205,155,49,131,27,37,255,51,255,95,255,85,130,1,38,102,255,107,255,189,0,7,130,19,55,7,0,5,0,7,255,
    126,255,97,255,134,255,146,0,0,255,15,255,12,0,0,254,54,0,130,0,33,254,30,130,15,34,209,255,106,130,5,32,192,130,14,138,2,33,255,159,130,19,32,200,130,3,32,173,135,20,33,255,231,131,
    9,32,255,69,149,13,32,201,132,19,38,165,255,175,255,189,255,174,130,3,35,210,255,233,0,65,79,7,134,7,37,255,202,0,0,255,187,130,23,34,0,254,119,132,45,32,57,131,5,137,3,36,255,236,
    0,0,255,130,3,137,17,139,9,65,59,21,33,255,121,139,35,141,11,33,255,181,135,15,67,23,15,130,187,36,2,0,120,0,6,130,1,34,0,0,11,130,1,34,1,0,16,130,1,32,2,130,177,50,18,0,3,0,37,0,41,
    0,4,0,44,0,52,0,9,0,56,0,62,130,21,38,69,0,71,0,25,0,73,130,1,34,28,0,76,130,1,40,29,0,81,0,84,0,30,0,86,130,1,34,34,0,90,130,1,40,35,0,92,0,94,0,36,0,138,130,1,40,39,0,176,0,179,0,
    40,0,188,130,1,34,44,0,192,130,1,34,45,0,198,130,1,40,46,0,211,0,212,0,47,0,214,130,1,34,49,0,217,130,1,40,50,0,219,0,221,0,51,0,223,130,1,34,54,0,225,130,1,34,55,0,227,130,1,34,56,
    0,229,130,1,34,57,0,235,130,1,34,58,0,237,130,1,34,59,0,246,130,1,34,60,0,251,130,1,10,28,61,0,253,0,254,0,62,1,3,1,4,0,64,1,9,1,9,0,66,1,12,1,12,0,67,1,23,1,25,0,68,1,43,1,45,0,71,
    1,48,1,48,0,74,1,50,1,50,0,75,1,73,1,73,0,76,1,108,1,114,0,77,1,118,1,119,0,84,2,40,2,40,0,86,2,42,2,43,0,87,2,70,2,71,0,89,2,73,2,73,0,91,2,75,2,113,0,92,2,118,2,123,0,131,2,128,2,
    144,0,137,2,146,2,155,0,154,2,164,2,166,0,164,2,168,2,168,0,167,2,170,2,170,0,168,2,172,2,172,0,169,2,174,2,174,0,170,2,177,2,177,0,171,2,179,2,179,0,172,2,181,2,181,0,173,2,183,2,
    183,0,174,2,185,2,185,0,175,2,187,2,187,0,176,2,189,2,201,0,177,2,203,2,203,0,190,2,205,2,205,0,191,2,207,2,207,0,192,2,218,2,218,0,193,2,220,2,220,0,194,2,222,2,222,0,195,2,224,2,
    224,0,196,2,226,2,226,0,197,2,228,2,228,0,198,2,230,2,230,0,199,2,232,2,232,0,200,2,234,2,234,0,201,2,236,2,236,0,202,2,238,2,246,0,203,3,82,3,87,0,212,3,90,3,105,0,218,3,108,3,108,
    0,234,3,112,3,112,0,235,3,114,3,114,0,236,3,118,3,118,0,237,3,121,3,122,0,238,3,124,3,133,0,240,3,135,3,137,0,250,3,139,3,144,0,253,3,146,3,152,1,3,3,158,3,159,1,10,3,161,3,161,1,12,
    3,163,3,163,1,13,3,165,3,168,1,14,3,171,3,176,1,18,3,178,3,178,1,24,3,182,3,183,1,25,3,188,3,199,1,27,3,202,3,203,1,39,3,205,3,208,1,41,3,215,3,216,1,45,3,220,3,220,1,47,3,222,3,228,
    1,48,3,233,3,234,1,55,3,238,4,22,1,57,4,24,4,24,1,98,4,26,4,39,1,99,4,47,4,47,1,113,4,50,4,50,1,114,4,52,4,52,1,115,4,64,4,69,1,116,4,72,4,72,1,122,4,74,4,74,1,123,4,76,4,76,1,124,
    4,78,4,79,1,125,4,84,4,87,1,127,4,90,4,90,1,131,4,92,4,93,1,132,4,95,4,95,1,134,4,99,4,99,1,135,4,101,4,101,1,136,4,105,4,105,1,137,4,169,4,169,1,138,0,2,1,78,0,16,130,1,34,1,0,18,
    130,1,34,1,0,37,130,1,34,2,0,38,130,1,34,3,0,39,130,1,34,4,0,40,130,1,34,5,0,41,130,1,40,6,0,44,0,45,0,7,0,46,130,1,34,8,0,47,130,1,34,9,0,48,130,1,36,10,0,49,0,50,130,23,38,51,0,51,
    0,5,0,52,130,1,34,11,0,56,130,1,34,12,0,57,130,1,34,8,0,58,130,1,34,13,0,59,130,1,34,14,0,60,130,1,34,15,0,61,130,1,34,16,0,62,130,1,34,17,0,69,130,1,34,18,0,70,130,1,34,19,0,71,130,
    1,34,20,0,73,130,1,34,21,0,76,130,1,40,22,0,81,0,82,0,22,0,83,130,1,34,23,0,84,130,1,34,19,0,86,130,1,34,24,0,90,130,1,34,25,0,92,130,1,34,26,0,93,130,1,34,25,0,94,130,1,34,27,0,138,
    130,1,34,19,0,176,130,1,34,28,0,177,130,1,34,2,0,178,130,1,34,5,0,179,130,1,34,2,0,188,130,1,34,25,0,192,130,1,34,22,0,198,130,1,40,19,0,211,0,212,0,29,0,214,130,1,34,7,0,217,130,1,
    36,15,0,219,0,220,130,203,38,221,0,221,0,30,0,223,130,1,34,7,0,225,130,1,34,7,0,227,130,1,34,29,0,229,130,1,34,29,0,235,130,1,34,31,0,237,130,1,34,26,0,246,130,1,34,19,0,251,130,1,
    34,32,0,253,130,1,34,32,0,254,130,1,42,19,1,3,1,4,0,32,1,9,1,9,130,5,8,50,12,1,12,0,2,1,23,1,23,0,23,1,24,1,24,0,13,1,25,1,25,0,25,1,43,1,43,0,19,1,44,1,44,0,28,1,45,1,45,0,31,1,48,
    1,48,0,9,1,50,1,50,130,5,34,73,1,73,130,17,40,110,1,110,0,1,1,114,1,114,130,5,52,118,1,119,0,1,2,40,2,40,0,4,2,42,2,43,0,5,2,70,2,71,130,5,46,73,2,73,0,12,2,75,2,81,0,2,2,82,2,82,130,
    29,46,83,2,86,0,6,2,87,2,91,0,7,2,92,2,96,130,35,8,50,97,2,100,0,8,2,101,2,101,0,16,2,102,2,108,0,18,2,109,2,109,0,20,2,110,2,113,0,21,2,118,2,118,0,22,2,119,2,123,0,23,2,128,2,129,
    0,25,2,130,2,130,130,77,34,131,2,131,130,41,34,132,2,132,130,11,34,133,2,133,130,11,34,134,2,134,130,11,34,135,2,135,130,11,34,136,2,136,130,107,34,137,2,137,130,71,34,138,2,138,130,
    11,34,139,2,139,130,11,34,140,2,140,130,11,34,141,2,141,130,11,34,142,2,142,130,11,34,143,2,143,130,11,34,144,2,144,130,137,34,146,2,146,130,155,34,147,2,147,130,119,34,148,2,148,130,
    11,34,149,2,149,130,11,34,150,2,150,130,11,34,151,2,151,130,11,34,152,2,152,130,11,34,153,2,153,130,11,34,154,2,154,130,11,34,155,2,155,130,11,34,164,2,164,130,209,34,165,2,165,130,
    173,34,166,2,166,130,11,34,168,2,168,130,5,34,170,2,170,130,5,34,172,2,172,130,5,34,174,2,174,130,5,34,177,2,177,130,239,46,179,2,179,0,9,2,181,2,181,0,10,2,183,2,183,130,5,34,185,
    2,185,130,5,34,187,2,187,130,5,34,189,2,189,130,41,34,190,2,190,130,77,34,191,2,191,130,11,34,192,2,192,130,11,34,193,2,193,130,11,34,194,2,195,130,11,34,196,2,196,130,179,40,197,2,
    197,0,23,2,198,2,198,130,11,34,199,2,199,130,11,34,200,2,200,130,11,34,201,2,201,130,11,40,203,2,203,0,24,2,205,2,205,130,5,34,207,2,207,130,5,40,218,2,218,0,12,2,220,2,220,130,5,34,
    222,2,222,130,5,34,224,2,224,130,143,34,226,2,226,130,5,34,228,2,228,130,5,34,230,2,230,130,5,34,232,2,232,130,5,34,234,2,234,130,5,52,236,2,236,0,14,2,238,2,238,0,16,2,239,2,239,0,
    25,2,240,2,240,130,11,46,241,2,241,0,17,2,242,2,242,0,27,2,243,2,243,130,11,34,244,2,244,130,11,34,245,2,245,130,11,8,38,246,2,246,0,27,3,82,3,82,0,2,3,83,3,83,0,6,3,84,3,85,0,7,3,
    86,3,86,0,5,3,87,3,87,0,16,3,90,3,90,130,29,40,91,3,91,0,3,3,92,3,92,130,35,40,93,3,93,0,17,3,94,3,95,130,41,40,96,3,96,0,9,3,97,3,98,130,11,34,99,3,99,130,53,46,100,3,100,0,11,3,101,
    3,101,0,12,3,102,3,102,130,65,40,103,3,103,0,15,3,104,3,104,130,35,34,105,3,105,130,17,52,108,3,108,0,22,3,112,3,112,0,23,3,114,3,114,0,25,3,118,3,118,130,11,34,121,3,121,130,95,40,
    122,3,122,0,28,3,124,3,125,130,47,40,126,3,126,0,8,3,127,3,128,130,101,40,129,3,129,0,30,3,130,3,130,130,143,40,131,3,131,0,3,3,132,3,132,130,41,34,133,3,133,130,53,34,135,3,136,130,
    47,34,137,3,137,130,131,34,139,3,139,130,131,40,140,3,140,0,4,3,141,3,141,130,137,34,142,3,142,130,131,46,143,3,143,0,18,3,144,3,144,0,21,3,146,3,146,130,113,46,147,3,147,0,19,3,148,
    3,148,0,20,3,149,3,149,130,137,40,150,3,150,0,26,3,151,3,151,130,35,40,152,3,152,0,31,3,158,3,158,130,23,40,159,3,159,0,14,3,161,3,161,130,5,34,163,3,163,130,5,34,165,3,165,130,203,
    34,166,3,166,130,29,34,172,3,172,130,125,34,173,3,173,130,215,34,174,3,174,130,161,34,175,3,175,130,107,34,176,3,176,130,155,34,178,3,178,130,77,40,182,3,182,0,13,3,183,3,183,130,47,
    34,188,3,188,130,155,34,189,3,189,130,119,34,190,3,190,130,71,34,191,3,191,130,23,34,192,3,192,130,71,34,193,3,193,130,173,34,194,3,194,130,137,34,195,3,195,130,17,34,196,3,196,130,
    83,34,197,3,197,130,83,34,198,3,198,130,11,34,199,3,199,130,11,34,202,3,202,130,95,34,203,3,203,130,95,34,205,3,206,130,5,34,207,3,207,130,59,34,208,3,208,130,59,40,215,3,215,0,5,3,
    216,3,216,130,233,34,220,3,220,130,5,34,222,3,222,130,239,40,223,3,223,0,30,3,224,3,224,130,113,34,225,3,225,130,11,34,226,3,226,130,11,34,227,3,227,130,11,34,228,3,228,130,11,34,233,
    3,233,130,71,34,234,3,234,130,71,34,238,3,238,130,113,34,239,3,239,130,113,34,240,3,240,130,11,34,241,3,241,130,11,34,242,3,242,130,11,34,243,3,243,130,11,34,244,3,244,130,11,34,245,
    3,245,130,11,34,246,3,246,130,11,34,247,3,247,130,11,34,248,3,248,130,11,34,249,3,249,130,11,34,250,3,250,130,11,34,251,3,251,130,11,34,252,3,252,130,11,34,253,3,253,130,11,34,254,
    3,254,130,11,46,255,3,255,0,18,4,0,4,0,0,2,4,1,4,1,130,11,34,2,4,2,130,11,34,3,4,3,130,11,34,4,4,4,130,11,34,5,4,5,130,11,46,6,4,6,0,6,4,7,4,7,0,21,4,8,4,8,130,11,34,9,4,9,130,11,34,
    10,4,10,130,11,34,11,4,11,130,11,34,12,4,12,130,11,34,13,4,13,130,11,34,14,4,14,130,11,34,15,4,15,130,11,34,16,4,16,130,11,34,17,4,17,130,11,34,18,4,18,130,11,34,19,4,19,130,11,34,
    20,4,20,130,11,34,21,4,21,130,11,40,22,4,22,0,7,4,24,4,24,130,5,46,26,4,26,0,5,4,27,4,27,0,23,4,28,4,28,130,11,34,29,4,29,130,11,34,30,4,30,130,11,34,31,4,31,130,11,34,32,4,32,130,
    11,34,33,4,33,130,11,34,34,4,34,130,11,34,35,4,35,130,11,34,36,4,36,130,11,34,37,4,37,130,11,34,38,4,38,130,11,34,39,4,39,130,11,34,47,4,47,130,5,40,50,4,50,0,8,4,52,4,52,130,5,46,
    64,4,64,0,16,4,65,4,65,0,25,4,66,4,66,130,11,34,67,4,67,130,11,34,68,4,68,130,11,34,69,4,69,130,11,40,72,4,72,0,9,4,74,4,74,130,149,8,32,76,4,76,0,12,4,78,4,78,0,15,4,79,4,79,0,26,
    4,84,4,84,0,28,4,85,4,85,0,31,4,86,4,86,130,23,34,87,4,87,130,23,46,90,4,90,0,22,4,92,4,92,0,29,4,93,4,93,130,35,34,95,4,95,130,71,34,99,4,99,130,71,34,101,4,101,130,5,34,105,4,105,
    130,101,42,169,4,169,0,5,0,2,1,109,0,6,130,1,32,1,74,171,10,34,22,0,17,130,1,34,25,0,18,130,1,33,22,0,71,233,5,38,39,0,39,0,8,0,43,130,1,34,8,0,46,130,1,34,26,0,51,130,1,34,8,0,53,
    130,1,34,8,0,55,130,1,34,27,0,56,130,1,34,9,0,57,130,1,34,10,0,58,130,1,34,11,0,59,130,1,34,12,0,60,130,1,34,23,0,61,130,1,34,13,0,62,130,1,34,24,0,69,130,1,40,3,0,71,0,73,0,4,0,75,
    130,1,36,4,0,81,0,82,130,141,34,83,0,83,130,139,34,84,0,84,130,11,34,85,0,85,130,29,38,87,0,87,0,7,0,89,130,1,34,14,0,90,130,1,34,15,0,92,130,1,34,28,0,93,130,1,34,15,0,94,130,1,34,
    16,0,131,130,1,34,8,0,146,130,1,34,8,0,147,130,1,34,4,0,151,130,1,34,8,0,152,130,1,34,4,0,154,130,1,33,14,0,71,227,9,32,8,71,227,6,34,186,0,186,130,95,38,188,0,188,0,15,0,192,130,1,
    36,5,0,199,0,200,130,17,38,202,0,202,0,14,0,209,130,1,34,9,0,210,130,1,34,8,0,211,130,1,34,17,0,213,130,1,34,9,0,217,130,1,34,23,0,220,130,1,34,17,0,221,130,1,34,21,0,224,130,1,34,
    18,0,235,130,1,34,5,0,237,130,1,36,28,0,239,0,240,130,191,40,241,0,241,0,19,0,242,0,244,130,11,34,246,0,246,130,89,34,247,0,247,130,11,40,248,0,248,0,20,0,249,0,250,130,11,34,253,0,
    253,130,5,52,255,0,255,0,5,1,2,1,2,0,4,1,3,1,3,0,19,1,4,1,4,130,17,46,7,1,7,0,8,1,12,1,12,0,2,1,22,1,22,130,11,52,23,1,23,0,6,1,24,1,24,0,11,1,25,1,25,0,15,1,26,1,26,130,23,34,28,1,
    28,130,5,34,29,1,29,130,65,34,30,1,30,130,11,34,32,1,32,130,5,34,33,1,33,130,17,72,37,5,34,53,1,53,130,83,34,56,1,56,130,23,34,57,1,57,130,23,40,58,1,58,0,9,1,68,1,68,130,23,34,73,
    1,73,130,5,34,75,1,76,130,23,40,81,1,81,0,17,1,85,1,85,130,41,34,86,1,86,130,17,52,105,1,106,0,25,1,108,1,109,0,1,1,110,1,110,0,22,1,111,1,113,130,11,34,114,1,114,130,11,46,118,1,119,
    0,22,2,40,2,41,0,8,2,43,2,44,130,5,36,69,2,69,0,25,72,103,10,36,8,2,92,2,96,130,23,8,46,97,2,100,0,10,2,101,2,101,0,13,2,102,2,108,0,3,2,109,2,113,0,4,2,118,2,118,0,5,2,119,2,123,0,
    6,2,124,2,127,0,14,2,128,2,129,0,15,72,91,10,33,3,2,72,91,9,33,3,2,72,91,9,36,3,2,136,2,136,130,89,34,137,2,137,130,71,34,138,2,138,130,11,34,139,2,139,130,11,34,140,2,140,130,11,34,
    141,2,141,130,11,34,142,2,142,130,11,34,143,2,143,130,11,34,145,2,145,130,5,34,147,2,147,130,5,34,149,2,149,130,5,34,151,2,151,130,5,34,153,2,153,130,5,34,155,2,155,130,5,34,156,2,
    156,130,47,34,157,2,157,130,11,34,158,2,158,130,11,34,159,2,159,130,11,34,160,2,160,130,11,34,161,2,161,130,11,34,162,2,162,130,11,34,163,2,163,130,11,40,177,2,177,0,26,2,190,2,190,
    130,197,34,192,2,192,130,5,34,194,2,195,130,5,34,196,2,196,130,35,34,197,2,197,130,215,34,198,2,198,130,11,34,199,2,199,130,11,34,200,2,200,130,11,34,201,2,201,130,11,46,208,2,208,
    0,27,2,209,2,209,0,7,2,210,2,210,130,11,34,211,2,211,130,11,34,212,2,212,130,11,34,213,2,213,130,11,34,214,2,214,130,11,34,215,2,215,130,11,34,216,2,216,130,11,34,217,2,217,130,11,
    40,218,2,218,0,9,2,220,2,220,130,5,34,222,2,222,130,5,46,224,2,224,0,10,2,225,2,225,0,14,2,226,2,226,130,11,34,227,2,227,130,11,34,228,2,228,130,11,34,229,2,229,130,11,34,230,2,230,
    130,11,34,231,2,231,130,11,34,232,2,232,130,11,34,233,2,233,130,11,34,234,2,234,130,11,34,235,2,235,130,11,52,236,2,236,0,12,2,238,2,238,0,13,2,239,2,239,0,15,2,240,2,240,130,11,46,
    241,2,241,0,24,2,242,2,242,0,16,2,243,2,243,130,11,34,244,2,244,130,11,34,245,2,245,130,11,34,246,2,246,130,11,36,249,2,249,0,8,72,103,6,34,86,3,86,130,11,36,87,3,87,0,13,72,91,6,40,
    93,3,93,0,24,3,99,3,99,130,23,34,102,3,102,130,23,40,103,3,103,0,23,3,105,3,105,130,11,8,32,106,3,106,0,4,3,108,3,108,0,5,3,110,3,110,0,14,3,112,3,112,0,6,3,114,3,114,0,15,3,117,3,
    117,130,17,34,118,3,118,130,17,34,119,3,119,130,11,42,126,3,126,0,26,3,129,3,129,0,21,72,43,6,34,137,3,137,130,89,34,140,3,140,130,5,40,141,3,141,0,9,3,142,3,142,130,95,40,143,3,143,
    0,3,3,144,3,144,130,95,34,145,3,145,130,95,34,146,3,146,130,71,34,147,3,147,130,11,34,148,3,148,130,23,34,149,3,149,130,101,40,150,3,150,0,28,3,151,3,151,130,17,34,152,3,152,130,29,
    40,153,3,153,0,7,3,157,3,157,130,11,34,158,3,158,130,35,40,159,3,159,0,12,3,161,3,161,130,5,34,163,3,163,130,5,34,165,3,165,130,191,34,166,3,166,130,29,40,167,3,168,0,1,3,171,3,171,
    130,5,34,173,3,173,130,53,72,37,9,36,3,3,178,3,178,130,89,34,179,3,179,130,23,40,182,3,182,0,11,3,183,3,183,130,53,34,184,3,184,130,23,34,185,3,185,130,185,34,188,3,188,130,5,34,189,
    3,189,130,17,34,190,3,190,130,89,34,191,3,191,130,35,34,193,3,193,130,203,34,194,3,194,130,161,72,37,9,33,3,3,72,37,9,36,3,3,203,3,203,130,53,34,205,3,206,130,5,34,207,3,207,130,47,
    34,208,3,208,130,47,34,212,3,212,130,113,34,214,3,214,130,5,34,215,3,215,130,95,40,216,3,216,0,6,3,217,3,217,130,11,34,218,3,218,130,47,34,219,3,219,130,11,34,220,3,220,130,23,40,223,
    3,223,0,21,3,224,3,224,130,119,34,225,3,225,130,11,34,226,3,226,130,11,34,227,3,227,130,11,34,228,3,228,130,11,46,229,3,229,0,18,3,230,3,230,0,20,3,232,3,232,130,89,34,233,3,233,130,
    113,34,234,3,234,130,113,34,235,3,235,130,83,43,236,3,236,0,17,3,237,3,237,0,19,3,72,91,9,33,3,3,72,91,9,33,3,3,72,91,9,33,3,3,72,91,9,33,3,3,72,91,9,33,3,3,72,91,9,33,3,3,72,91,9,
    33,3,3,72,91,9,33,3,3,72,91,9,32,3,72,91,10,33,3,4,72,91,9,33,3,4,72,91,9,42,3,4,7,4,7,0,4,4,9,4,9,130,5,34,11,4,11,130,5,34,13,4,13,130,5,34,15,4,15,130,5,34,17,4,17,130,5,34,19,4,
    19,130,5,34,21,4,21,130,5,46,26,4,26,0,8,4,27,4,27,0,6,4,28,4,28,130,11,34,29,4,29,130,11,34,30,4,30,130,11,34,31,4,31,130,11,34,32,4,32,130,11,34,33,4,33,130,11,34,34,4,34,130,11,
    34,35,4,35,130,11,34,36,4,36,130,11,34,37,4,37,130,11,34,38,4,38,130,11,34,39,4,39,130,11,34,40,4,40,130,11,34,41,4,41,130,95,34,42,4,42,130,11,34,43,4,43,130,11,34,44,4,44,130,11,
    34,45,4,45,130,11,34,46,4,46,130,11,34,47,4,47,130,47,34,48,4,48,130,11,34,49,4,49,130,23,46,50,4,50,0,10,4,51,4,51,0,14,4,52,4,52,130,11,34,53,4,53,130,11,34,55,4,55,130,5,34,57,4,
    57,130,5,34,59,4,59,130,5,34,61,4,61,130,5,34,63,4,63,130,5,46,64,4,64,0,13,4,65,4,65,0,15,4,66,4,66,130,11,34,67,4,67,130,11,34,68,4,68,130,11,34,69,4,69,130,11,40,73,4,73,0,5,4,75,
    4,75,130,5,8,32,76,4,76,0,9,4,78,4,78,0,23,4,79,4,79,0,28,4,80,4,80,0,18,4,81,4,81,0,20,4,82,4,82,130,11,34,83,4,83,130,11,34,85,4,85,130,47,34,86,4,86,130,41,34,87,4,87,130,41,34,
    98,4,98,130,17,34,100,4,100,130,5,34,102,4,102,130,5,46,103,4,103,0,17,4,104,4,104,0,19,4,105,4,105,130,113,52,111,4,111,0,25,4,169,4,169,0,8,0,1,0,0,0,10,2,6,8,16,24,89,11,12,46,72,
    103,114,101,107,0,118,108,97,116,110,0,164,0,4,130,35,36,0,255,255,0,18,130,7,8,32,10,0,20,0,30,0,40,0,52,0,65,0,75,0,85,0,95,0,105,0,115,0,125,0,135,0,145,0,155,0,165,0,175,138,45,
    8,34,1,0,11,0,21,0,31,0,41,0,53,0,66,0,76,0,86,0,96,0,106,0,116,0,126,0,136,0,146,0,156,0,166,0,176,138,45,8,34,2,0,12,0,22,0,32,0,42,0,54,0,67,0,77,0,87,0,97,0,107,0,117,0,127,0,137,
    0,147,0,157,0,167,0,177,130,119,8,37,6,65,90,69,32,0,84,67,82,84,32,0,126,77,79,76,32,0,168,78,65,86,32,0,212,82,79,77,32,1,0,84,85,82,32,1,44,0,131,173,8,38,19,0,3,0,13,0,23,0,33,
    0,43,0,50,0,55,0,68,0,78,0,88,0,98,0,108,0,118,0,128,0,138,0,148,0,158,0,168,0,178,132,43,32,18,130,135,8,32,14,0,24,0,34,0,44,0,56,0,69,0,79,0,89,0,99,0,109,0,119,0,129,0,139,0,149,
    0,159,0,169,0,179,134,41,8,34,5,0,15,0,25,0,35,0,45,0,57,0,70,0,80,0,90,0,100,0,110,0,120,0,130,0,140,0,150,0,160,0,170,0,180,132,41,8,38,19,0,6,0,16,0,26,0,36,0,46,0,58,0,62,0,71,
    0,81,0,91,0,101,0,111,0,121,0,131,0,141,0,151,0,161,0,171,0,181,134,43,8,36,7,0,17,0,27,0,37,0,47,0,59,0,63,0,72,0,82,0,92,0,102,0,112,0,122,0,132,0,142,0,152,0,162,0,172,0,182,134,
    43,8,36,8,0,18,0,28,0,38,0,48,0,60,0,64,0,73,0,83,0,93,0,103,0,113,0,123,0,133,0,143,0,153,0,163,0,173,0,183,134,43,8,44,9,0,19,0,29,0,39,0,49,0,51,0,61,0,74,0,84,0,94,0,104,0,114,
    0,124,0,134,0,144,0,154,0,164,0,174,0,184,0,185,99,50,115,99,4,88,132,5,32,94,132,5,32,100,132,5,32,106,164,5,37,99,109,112,4,112,99,180,5,37,100,108,105,103,4,120,132,5,32,126,132,
    5,32,132,132,5,32,138,164,5,37,110,111,109,4,144,100,131,5,32,150,132,5,32,156,132,5,32,162,163,5,37,102,114,97,99,4,168,181,5,130,178,34,97,4,178,132,5,38,186,108,110,117,109,4,192,
    132,5,32,198,132,5,32,204,132,5,32,210,164,5,37,111,99,108,4,216,108,131,5,32,222,132,5,32,228,130,76,34,114,4,234,132,5,32,240,132,5,32,246,132,5,32,252,163,5,32,111,130,36,33,5,2,
    132,5,32,8,132,5,32,14,132,5,32,20,163,5,32,112,131,59,32,26,132,5,32,32,132,5,32,38,132,5,32,44,163,5,37,115,109,99,112,5,50,132,5,32,56,132,5,32,62,132,5,32,68,164,5,37,115,48,49,
    5,74,115,131,5,32,80,132,5,32,86,132,5,32,92,166,5,34,50,5,98,130,41,34,50,5,104,132,5,32,110,132,5,32,116,166,5,34,51,5,122,130,41,34,51,5,128,132,5,32,134,132,5,32,140,166,5,34,52,
    5,146,130,41,34,52,5,152,132,5,32,158,132,5,32,164,166,5,34,53,5,170,130,41,34,53,5,176,132,5,32,182,132,5,32,188,166,5,34,54,5,194,130,41,34,54,5,200,132,5,32,206,132,5,32,212,166,
    5,34,55,5,218,130,41,34,55,5,224,132,5,32,230,132,5,32,236,163,5,37,116,110,117,109,5,242,132,5,32,248,132,5,32,254,131,5,33,6,4,163,5,34,0,0,0,24,94,219,23,36,2,0,8,0,9,132,31,32,
    14,132,5,32,16,132,5,32,15,132,5,32,13,132,5,32,67,132,5,32,69,132,5,32,68,132,5,32,66,130,5,38,3,0,63,0,64,0,65,130,9,36,2,0,17,0,18,132,23,133,5,32,60,132,11,32,62,132,5,32,61,132,
    5,32,59,132,5,32,10,132,5,32,12,132,5,32,11,132,5,32,71,132,5,32,73,132,5,32,72,132,5,32,70,132,5,32,48,132,5,32,50,132,5,32,49,132,5,32,47,132,5,32,56,132,5,32,58,132,5,32,57,132,
    5,32,55,132,5,32,5,132,5,32,7,132,5,32,6,132,5,32,4,132,5,32,20,132,5,32,22,132,5,32,21,132,5,32,19,132,5,32,24,132,5,32,26,132,5,32,25,132,5,32,23,132,5,32,28,132,5,32,30,132,5,32,
    29,132,5,32,27,132,5,32,32,132,5,32,34,132,5,32,33,132,5,32,31,132,5,32,36,132,5,32,38,132,5,32,37,132,5,32,35,132,5,32,40,132,5,32,42,132,5,32,41,132,5,32,39,132,5,32,44,132,5,32,
    46,132,5,32,45,132,5,32,43,132,5,32,52,132,5,32,54,132,5,32,53,132,5,36,51,0,75,0,152,133,1,33,4,38,133,1,37,7,20,7,192,14,80,130,1,34,102,14,136,134,1,36,190,14,228,15,18,134,1,33,
    38,15,133,1,33,58,15,133,1,33,78,15,133,1,33,96,15,133,1,33,122,15,133,1,33,188,15,133,1,33,218,15,133,1,33,248,15,132,1,33,16,42,134,1,33,92,16,133,1,38,142,16,162,16,218,16,204,134,
    1,32,218,130,11,130,3,35,17,6,0,1,132,159,8,111,8,0,2,1,196,0,223,1,231,1,186,1,187,1,188,1,189,1,190,1,191,1,192,1,193,1,194,1,195,1,196,1,197,1,198,1,199,1,200,1,201,1,202,1,203,
    1,204,1,205,1,206,1,207,1,208,1,209,1,210,1,211,1,232,1,233,2,67,2,59,1,234,1,235,1,236,1,237,1,238,1,239,1,240,1,241,1,242,1,243,1,244,1,245,1,246,1,247,1,248,1,249,1,250,1,251,1,
    252,1,253,1,254,2,130,110,9,85,4,220,2,2,2,3,2,4,2,5,2,6,2,7,2,8,2,9,2,10,2,11,2,47,2,15,2,16,2,17,2,20,2,21,2,22,2,23,2,24,2,25,2,27,2,28,2,30,2,29,2,251,2,252,2,253,2,254,2,255,3,
    0,3,1,3,2,3,3,3,4,3,5,3,6,3,7,3,8,3,9,3,10,3,11,3,12,3,13,3,14,3,15,3,16,3,17,3,18,3,19,3,20,3,21,3,22,3,23,3,24,3,25,3,26,3,27,3,28,3,29,3,30,3,31,3,32,3,33,3,34,3,35,3,36,3,37,3,
    38,3,39,3,40,3,41,3,42,3,43,3,44,3,45,3,46,3,47,3,48,3,49,3,50,3,51,3,52,3,53,3,54,3,55,3,56,3,57,3,58,3,59,3,60,3,61,3,62,3,63,3,64,3,65,3,66,3,67,3,69,3,68,3,70,3,71,3,72,3,73,3,
    74,3,75,3,76,3,77,3,78,3,79,3,80,3,81,4,170,4,171,4,172,4,173,4,174,4,175,4,176,4,177,4,178,4,179,4,180,4,181,4,182,4,183,4,184,4,185,4,186,4,187,4,188,4,189,4,190,4,191,4,192,4,193,
    4,194,4,195,4,196,4,197,1,255,4,198,4,199,4,200,4,201,4,202,4,203,4,204,4,205,4,206,4,207,4,208,4,209,4,210,4,211,4,212,4,213,4,215,4,216,4,218,2,26,4,219,2,14,4,214,2,19,2,13,4,217,
    2,12,2,18,0,1,0,223,24,98,181,58,34,133,0,146,16,98,183,1,128,67,141,9,34,116,0,183,67,139,51,43,2,252,3,47,2,59,1,250,4,201,4,202,67,109,7,43,1,255,2,0,4,205,4,206,4,208,4,211,67,
    117,21,67,165,11,67,129,12,34,23,2,25,67,113,56,32,78,67,115,44,67,113,59,67,111,5,46,200,4,203,4,204,4,207,4,209,4,210,2,1,4,212,67,83,11,35,4,198,4,199,67,55,5,33,2,24,67,57,6,32,
    251,67,59,10,32,22,67,61,6,9,130,183,0,69,0,70,0,71,0,72,0,73,0,74,0,75,0,76,0,77,0,78,0,79,0,80,0,81,0,82,0,83,0,84,0,85,0,86,0,87,0,88,0,89,0,90,0,91,0,92,0,93,0,94,0,135,0,140,0,
    147,0,233,0,234,0,235,0,236,0,237,0,238,0,239,0,240,0,241,0,242,0,243,0,244,0,245,0,246,0,247,0,248,0,249,0,250,0,251,0,252,0,253,0,254,0,255,1,0,1,1,1,2,1,3,1,4,1,5,1,6,1,45,1,49,
    1,51,1,57,1,59,1,61,1,64,1,71,2,74,2,102,2,103,2,104,2,105,2,106,2,107,2,108,2,109,2,110,2,111,2,112,2,113,2,114,2,115,2,116,2,117,2,118,2,119,2,120,2,121,2,122,2,123,2,124,2,125,2,
    126,2,127,2,128,2,129,2,131,2,133,2,135,2,137,2,139,2,141,2,143,2,145,2,147,2,149,2,151,2,153,2,155,2,157,2,159,2,161,2,163,2,165,2,167,2,169,2,171,2,173,2,178,2,180,2,182,2,184,2,
    186,2,188,2,190,2,192,2,194,2,197,2,199,2,201,2,203,2,205,2,207,2,209,2,211,2,213,2,217,2,219,2,221,2,223,2,225,2,227,2,229,2,231,2,233,2,235,2,237,2,239,2,242,2,244,2,246,3,143,3,
    144,3,145,3,146,3,147,3,148,3,149,3,150,3,151,3,152,3,153,3,154,3,155,3,156,3,157,3,158,3,187,3,189,3,191,3,205,3,212,3,218,3,224,4,70,4,73,4,75,4,79,4,87,4,89,4,90,4,94,4,106,0,6,
    0,0,0,6,0,18,0,42,0,66,0,90,0,114,0,138,0,3,130,17,32,1,130,17,36,1,0,144,0,1,130,11,32,74,130,5,34,1,0,77,138,23,32,120,138,23,32,78,138,23,32,96,137,23,33,2,173,138,23,32,72,137,
    23,33,3,154,138,23,32,48,138,23,32,156,138,23,32,24,137,23,35,4,25,0,2,130,13,34,167,0,171,130,137,32,4,130,3,40,1,0,8,0,1,6,30,0,54,130,171,77,223,5,8,114,202,0,252,1,14,1,24,1,74,
    1,100,1,126,1,144,1,186,1,236,1,246,2,24,2,50,2,68,2,118,2,136,2,162,2,204,2,222,3,16,3,26,3,36,3,54,3,104,3,114,3,124,3,134,3,160,3,186,3,204,3,246,4,40,4,50,4,84,4,110,4,128,4,178,
    4,196,4,222,5,8,5,26,5,36,5,46,5,56,5,66,5,108,5,150,5,192,5,234,6,20,0,6,0,14,0,20,0,26,0,32,0,38,0,44,2,75,130,147,34,167,2,76,130,5,34,168,2,78,130,5,34,169,3,240,130,5,34,170,4,
    122,130,5,34,171,3,238,130,5,38,172,0,1,0,4,4,135,135,9,33,2,136,130,9,32,168,130,3,36,6,0,12,4,137,130,7,38,172,4,139,0,2,1,162,142,87,32,83,132,87,32,84,130,5,34,168,4,10,130,5,34,
    169,4,8,132,87,32,124,130,5,34,171,4,6,132,77,32,2,130,51,34,12,4,118,132,123,32,162,132,67,131,105,32,141,132,27,141,165,32,87,132,77,32,88,132,41,32,166,132,77,32,22,132,77,32,126,
    132,77,32,24,132,49,34,3,0,8,131,217,33,4,143,132,115,32,145,130,5,34,172,2,179,132,85,134,25,33,2,181,132,25,32,147,132,25,32,183,132,25,132,129,33,3,172,132,23,32,149,132,69,44,5,
    0,12,0,18,0,24,0,30,0,36,4,120,132,117,32,189,132,117,32,91,132,117,32,151,132,59,32,191,146,239,32,92,132,43,32,93,132,43,32,95,132,43,32,28,132,161,32,128,132,161,32,26,65,71,8,32,
    153,130,9,44,168,0,4,0,10,0,16,0,22,0,28,2,202,132,123,32,130,132,37,32,155,132,93,32,204,140,179,32,208,132,31,32,157,132,25,32,214,137,179,33,4,159,132,17,32,218,146,137,32,97,132,
    137,32,98,132,137,32,224,132,137,32,52,132,137,32,132,132,99,32,50,65,121,10,32,161,132,29,32,163,65,61,11,32,3,131,87,34,167,3,161,132,117,32,165,65,17,15,32,3,131,17,34,167,2,101,
    132,29,32,68,132,61,32,66,132,91,32,64,137,85,33,2,241,132,29,32,167,65,197,18,32,102,132,153,32,103,132,153,32,105,130,5,33,169,3,131,43,34,170,4,123,130,11,34,171,3,239,65,35,8,32,
    136,66,107,8,32,137,66,107,10,32,138,130,11,34,172,4,140,146,241,32,110,132,87,32,111,132,119,32,11,132,149,32,9,132,149,32,125,132,241,32,7,136,87,32,119,132,77,36,1,0,4,4,142,136,
    19,32,25,66,59,12,32,144,132,67,32,146,130,5,34,172,2,180,65,135,12,32,182,132,25,32,148,132,25,32,184,65,135,9,33,3,173,132,23,32,150,65,41,15,33,4,121,132,147,32,190,132,235,32,118,
    132,147,32,152,132,59,32,192,146,191,131,151,34,167,2,120,132,43,32,122,132,43,32,29,132,191,32,129,132,191,32,27,136,171,32,154,66,59,14,32,203,132,123,32,131,132,37,32,156,132,93,
    32,205,140,179,32,209,132,31,32,158,132,25,32,215,137,179,33,4,160,132,17,32,219,146,137,32,124,132,181,32,125,132,137,32,225,132,137,32,53,132,137,32,133,132,99,32,51,65,229,9,33,
    4,162,132,29,32,164,66,59,12,32,160,130,13,34,167,3,162,132,117,32,166,65,17,15,32,3,131,17,34,167,2,128,132,29,32,69,132,61,32,67,132,91,32,65,137,85,33,2,242,132,29,32,168,65,255,
    8,32,247,132,241,36,1,0,4,2,249,136,9,32,248,136,9,32,250,132,9,67,177,10,33,2,114,132,191,32,115,132,191,32,167,132,99,32,23,132,99,32,127,130,5,33,171,0,138,41,33,4,42,130,17,34,
    167,4,40,132,111,32,46,132,41,32,44,132,41,32,48,143,183,33,4,43,132,41,32,41,132,41,32,47,132,41,32,45,132,41,32,49,144,41,32,56,132,41,32,54,132,41,32,60,132,41,32,58,132,41,32,62,
    144,41,32,57,132,41,32,55,132,41,32,61,132,41,32,59,132,41,32,63,65,235,8,32,134,132,219,44,2,0,17,0,37,0,41,0,0,0,43,0,45,130,191,8,48,47,0,52,0,8,0,54,0,59,0,14,0,61,0,62,0,20,0,
    69,0,73,0,22,0,75,0,77,0,27,0,79,0,84,0,30,0,86,0,91,0,36,0,93,0,94,0,42,0,129,130,1,34,44,0,131,130,1,34,45,0,134,130,1,34,46,0,137,130,1,34,47,0,140,130,1,40,48,0,151,0,154,0,49,
    0,207,130,1,33,53,0,74,41,7,34,1,0,6,130,121,38,1,0,2,2,212,2,213,136,21,32,2,130,111,40,4,4,221,4,222,4,223,4,224,130,21,41,4,2,134,2,135,2,152,2,153,0,70,199,8,33,0,38,130,55,34,
    10,0,28,70,25,5,33,1,163,130,7,34,74,1,168,130,5,38,88,0,1,0,4,1,169,134,9,36,2,0,74,0,87,138,53,32,68,132,53,32,20,132,35,32,164,130,11,32,77,132,9,32,166,132,9,137,91,32,30,138,37,
    32,165,130,11,32,80,132,37,32,167,134,9,130,83,130,111,138,193,33,1,149,130,31,34,1,0,75,136,191,130,213,33,1,39,132,19,32,186,140,19,32,172,132,19,32,54,136,19,32,2,131,191,35,1,227,
    1,228,138,17,38,10,0,2,1,229,1,230,130,17,36,2,0,47,0,79,138,25,58,30,0,12,2,40,2,42,2,41,2,43,2,44,2,31,2,32,2,33,1,174,2,35,2,36,2,37,130,37,42,12,0,39,0,40,0,43,0,51,0,53,74,5,6,
    33,75,0,73,243,5,75,149,8,41,0,12,0,3,2,38,2,39,2,39,130,47,38,3,0,73,0,75,1,174,138,95,50,102,0,8,2,61,2,45,2,46,2,48,2,49,2,57,2,58,2,60,138,29,50,22,0,8,0,27,0,21,0,22,0,23,0,24,
    0,25,0,29,0,20,130,29,48,8,1,173,2,34,4,112,4,113,4,114,4,115,4,116,4,117,141,49,33,4,117,137,29,35,1,173,4,116,130,29,32,8,130,55,137,69,32,27,130,71,137,159,131,99,141,27,132,99,
    141,147,33,2,61,65,83,12,32,105,130,13,40,1,0,19,0,6,0,0,0,1,130,163,32,3,130,5,32,18,130,3,32,82,132,15,42,0,0,74,0,2,0,2,1,124,1,124,130,30,34,212,1,221,82,121,8,32,8,130,9,34,40,
    1,192,136,75,56,2,0,26,0,10,2,50,0,122,0,115,0,116,2,51,2,52,2,53,2,54,2,55,2,56,130,63,32,1,130,179,81,129,5,130,68,130,99,8,36,2,0,38,0,16,1,212,1,213,1,214,1,215,1,216,1,217,1,218,
    1,219,1,220,1,221,2,64,2,62,2,65,2,66,2,63,4,225,130,89,32,16,140,235,36,26,0,27,0,28,130,239,42,77,0,78,2,173,3,154,3,156,4,25,5,250,88,209,75,52,
};


static const char* GetDefaultCompressedFontDataTTF(int* out_size)
{
    *out_size = roboto_compressed_size;
    return (const char*)roboto_compressed_data;
}
#endif // #ifndef IMGUI_DISABLE_DEFAULT_FONT


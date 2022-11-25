#pragma once
#include "EasyVKStart.h"

//Vulkan SDK's vulkan_format_traits.hpp offers more detailed information.
struct formatInfo {
    enum rawDataType :uint8_t {
        other,
        integer,
        floatingPoint
    };
    uint8_t componentCount;
    uint8_t sizePerComponent;	//0 means 'compressed' or 'not even' or 'less than 1'
    uint8_t sizePerPixel;		//0 means 'compressed', this value is the aligned size, may different from vk::blockSize(...)
    uint8_t rawDataType;
};
constexpr formatInfo formatInfos_v1_0[] = {
    { 0, 0, 0, 0 },//VK_FORMAT_UNDEFINED = 0,

    { 2, 0, 1, 1 },//VK_FORMAT_R4G4_UNORM_PACK8 = 1,

    { 4, 0, 2, 1 },//VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
    { 4, 0, 2, 1 },//VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,

    { 3, 0, 2, 1 },//VK_FORMAT_R5G6B5_UNORM_PACK16 = 4,
    { 3, 0, 2, 1 },//VK_FORMAT_B5G6R5_UNORM_PACK16 = 5,

    { 4, 0, 2, 1 },//VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
    { 4, 0, 2, 1 },//VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
    { 4, 0, 2, 1 },//VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,

    { 1, 1, 1, 1 },//VK_FORMAT_R8_UNORM = 9,
    { 1, 1, 1, 1 },//VK_FORMAT_R8_SNORM = 10,
    { 1, 1, 1, 1 },//VK_FORMAT_R8_USCALED = 11,
    { 1, 1, 1, 1 },//VK_FORMAT_R8_SSCALED = 12,
    { 1, 1, 1, 1 },//VK_FORMAT_R8_UINT = 13,
    { 1, 1, 1, 1 },//VK_FORMAT_R8_SINT = 14,
    { 1, 1, 1, 1 },//VK_FORMAT_R8_SRGB = 15,

    { 2, 1, 2, 1 },//VK_FORMAT_R8G8_UNORM = 16,
    { 2, 1, 2, 1 },//VK_FORMAT_R8G8_SNORM = 17,
    { 2, 1, 2, 1 },//VK_FORMAT_R8G8_USCALED = 18,
    { 2, 1, 2, 1 },//VK_FORMAT_R8G8_SSCALED = 19,
    { 2, 1, 2, 1 },//VK_FORMAT_R8G8_UINT = 20,
    { 2, 1, 2, 1 },//VK_FORMAT_R8G8_SINT = 21,
    { 2, 1, 2, 1 },//VK_FORMAT_R8G8_SRGB = 22,

    { 3, 1, 3, 1 },//VK_FORMAT_R8G8B8_UNORM = 23,
    { 3, 1, 3, 1 },//VK_FORMAT_R8G8B8_SNORM = 24,
    { 3, 1, 3, 1 },//VK_FORMAT_R8G8B8_USCALED = 25,
    { 3, 1, 3, 1 },//VK_FORMAT_R8G8B8_SSCALED = 26,
    { 3, 1, 3, 1 },//VK_FORMAT_R8G8B8_UINT = 27,
    { 3, 1, 3, 1 },//VK_FORMAT_R8G8B8_SINT = 28,
    { 3, 1, 3, 1 },//VK_FORMAT_R8G8B8_SRGB = 29,
    { 3, 1, 3, 1 },//VK_FORMAT_B8G8R8_UNORM = 30,
    { 3, 1, 3, 1 },//VK_FORMAT_B8G8R8_SNORM = 31,
    { 3, 1, 3, 1 },//VK_FORMAT_B8G8R8_USCALED = 32,
    { 3, 1, 3, 1 },//VK_FORMAT_B8G8R8_SSCALED = 33,
    { 3, 1, 3, 1 },//VK_FORMAT_B8G8R8_UINT = 34,
    { 3, 1, 3, 1 },//VK_FORMAT_B8G8R8_SINT = 35,
    { 3, 1, 3, 1 },//VK_FORMAT_B8G8R8_SRGB = 36,

    { 4, 1, 4, 1 },//VK_FORMAT_R8G8B8A8_UNORM = 37,
    { 4, 1, 4, 1 },//VK_FORMAT_R8G8B8A8_SNORM = 38,
    { 4, 1, 4, 1 },//VK_FORMAT_R8G8B8A8_USCALED = 39,
    { 4, 1, 4, 1 },//VK_FORMAT_R8G8B8A8_SSCALED = 40,
    { 4, 1, 4, 1 },//VK_FORMAT_R8G8B8A8_UINT = 41,
    { 4, 1, 4, 1 },//VK_FORMAT_R8G8B8A8_SINT = 42,
    { 4, 1, 4, 1 },//VK_FORMAT_R8G8B8A8_SRGB = 43,
    { 4, 1, 4, 1 },//VK_FORMAT_B8G8R8A8_UNORM = 44,
    { 4, 1, 4, 1 },//VK_FORMAT_B8G8R8A8_SNORM = 45,
    { 4, 1, 4, 1 },//VK_FORMAT_B8G8R8A8_USCALED = 46,
    { 4, 1, 4, 1 },//VK_FORMAT_B8G8R8A8_SSCALED = 47,
    { 4, 1, 4, 1 },//VK_FORMAT_B8G8R8A8_UINT = 48,
    { 4, 1, 4, 1 },//VK_FORMAT_B8G8R8A8_SINT = 49,
    { 4, 1, 4, 1 },//VK_FORMAT_B8G8R8A8_SRGB = 50,
    { 4, 1, 4, 1 },//VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
    { 4, 1, 4, 1 },//VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
    { 4, 1, 4, 1 },//VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
    { 4, 1, 4, 1 },//VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
    { 4, 1, 4, 1 },//VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
    { 4, 1, 4, 1 },//VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
    { 4, 1, 4, 1 },//VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,

    { 4, 0, 4, 1 },//VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
    { 4, 0, 4, 1 },//VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
    { 4, 0, 4, 1 },//VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
    { 4, 0, 4, 1 },//VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
    { 4, 0, 4, 1 },//VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
    { 4, 0, 4, 1 },//VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
    { 4, 0, 4, 1 },//VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
    { 4, 0, 4, 1 },//VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
    { 4, 0, 4, 1 },//VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
    { 4, 0, 4, 1 },//VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
    { 4, 0, 4, 1 },//VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
    { 4, 0, 4, 1 },//VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69,

    { 1, 2, 2, 1 },//VK_FORMAT_R16_UNORM = 70,
    { 1, 2, 2, 1 },//VK_FORMAT_R16_SNORM = 71,
    { 1, 2, 2, 1 },//VK_FORMAT_R16_USCALED = 72,
    { 1, 2, 2, 1 },//VK_FORMAT_R16_SSCALED = 73,
    { 1, 2, 2, 1 },//VK_FORMAT_R16_UINT = 74,
    { 1, 2, 2, 1 },//VK_FORMAT_R16_SINT = 75,
    { 1, 2, 2, 2 },//VK_FORMAT_R16_SFLOAT = 76,

    { 2, 2, 4, 1 },//VK_FORMAT_R16G16_UNORM = 77,
    { 2, 2, 4, 1 },//VK_FORMAT_R16G16_SNORM = 78,
    { 2, 2, 4, 1 },//VK_FORMAT_R16G16_USCALED = 79,
    { 2, 2, 4, 1 },//VK_FORMAT_R16G16_SSCALED = 80,
    { 2, 2, 4, 1 },//VK_FORMAT_R16G16_UINT = 81,
    { 2, 2, 4, 1 },//VK_FORMAT_R16G16_SINT = 82,
    { 2, 2, 4, 2 },//VK_FORMAT_R16G16_SFLOAT = 83,

    { 3, 2, 6, 1 },//VK_FORMAT_R16G16B16_UNORM = 84,
    { 3, 2, 6, 1 },//VK_FORMAT_R16G16B16_SNORM = 85,
    { 3, 2, 6, 1 },//VK_FORMAT_R16G16B16_USCALED = 86,
    { 3, 2, 6, 1 },//VK_FORMAT_R16G16B16_SSCALED = 87,
    { 3, 2, 6, 1 },//VK_FORMAT_R16G16B16_UINT = 88,
    { 3, 2, 6, 1 },//VK_FORMAT_R16G16B16_SINT = 89,
    { 3, 2, 6, 2 },//VK_FORMAT_R16G16B16_SFLOAT = 90,

    { 4, 2, 8, 1 },//VK_FORMAT_R16G16B16A16_UNORM = 91,
    { 4, 2, 8, 1 },//VK_FORMAT_R16G16B16A16_SNORM = 92,
    { 4, 2, 8, 1 },//VK_FORMAT_R16G16B16A16_USCALED = 93,
    { 4, 2, 8, 1 },//VK_FORMAT_R16G16B16A16_SSCALED = 94,
    { 4, 2, 8, 1 },//VK_FORMAT_R16G16B16A16_UINT = 95,
    { 4, 2, 8, 1 },//VK_FORMAT_R16G16B16A16_SINT = 96,
    { 4, 2, 8, 2 },//VK_FORMAT_R16G16B16A16_SFLOAT = 97,

    { 1, 4, 4, 1 },//VK_FORMAT_R32_UINT = 98,
    { 1, 4, 4, 1 },//VK_FORMAT_R32_SINT = 99,
    { 1, 4, 4, 2 },//VK_FORMAT_R32_SFLOAT = 100,

    { 2, 4, 8, 1 },//VK_FORMAT_R32G32_UINT = 101,
    { 2, 4, 8, 1 },//VK_FORMAT_R32G32_SINT = 102,
    { 2, 4, 8, 2 },//VK_FORMAT_R32G32_SFLOAT = 103,

    { 3, 4, 12, 1 },//VK_FORMAT_R32G32B32_UINT = 104,
    { 3, 4, 12, 1 },//VK_FORMAT_R32G32B32_SINT = 105,
    { 3, 4, 12, 2 },//VK_FORMAT_R32G32B32_SFLOAT = 106,

    { 4, 4, 16, 1 },//VK_FORMAT_R32G32B32A32_UINT = 107,
    { 4, 4, 16, 1 },//VK_FORMAT_R32G32B32A32_SINT = 108,
    { 4, 4, 16, 2 },//VK_FORMAT_R32G32B32A32_SFLOAT = 109,

    { 1, 8, 8, 1 },//VK_FORMAT_R64_UINT = 110,
    { 1, 8, 8, 1 },//VK_FORMAT_R64_SINT = 111,
    { 1, 8, 8, 2 },//VK_FORMAT_R64_SFLOAT = 112,

    { 2, 8, 16, 1 },//VK_FORMAT_R64G64_UINT = 113,
    { 2, 8, 16, 1 },//VK_FORMAT_R64G64_SINT = 114,
    { 2, 8, 16, 2 },//VK_FORMAT_R64G64_SFLOAT = 115,

    { 3, 8, 24, 1 },//VK_FORMAT_R64G64B64_UINT = 116,
    { 3, 8, 24, 1 },//VK_FORMAT_R64G64B64_SINT = 117,
    { 3, 8, 24, 2 },//VK_FORMAT_R64G64B64_SFLOAT = 118,

    { 4, 8, 32, 1 },//VK_FORMAT_R64G64B64A64_UINT = 119,
    { 4, 8, 32, 1 },//VK_FORMAT_R64G64B64A64_SINT = 120,
    { 4, 8, 32, 2 },//VK_FORMAT_R64G64B64A64_SFLOAT = 121,

    { 3, 0, 4, 2 },//VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
    { 3, 0, 4, 2 },//VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,//'E' is a 5-bit shared exponent

    { 1, 2, 2, 1 },//VK_FORMAT_D16_UNORM = 124,
    { 1, 3, 4, 1 },//VK_FORMAT_X8_D24_UNORM_PACK32 = 125,//8 bits are unused therefore componentCount is 1, sizePerComponent is 3
    { 1, 4, 4, 2 },//VK_FORMAT_D32_SFLOAT = 126,
    { 1, 1, 1, 1 },//VK_FORMAT_S8_UINT = 127,
    { 2, 0, 3, 1 },//VK_FORMAT_D16_UNORM_S8_UINT = 128,
    { 2, 0, 4, 1 },//VK_FORMAT_D24_UNORM_S8_UINT = 129,
    { 2, 0, 8, 0 },//VK_FORMAT_D32_SFLOAT_S8_UINT = 130,//24 bits are unused if data is of linear tiling therefore sizePerPixel is 8

    { 3, 0, 0, 1 },//VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131,
    { 3, 0, 0, 1 },//VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132,
    { 4, 0, 0, 1 },//VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133,
    { 4, 0, 0, 1 },//VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134,

    { 4, 0, 0, 1 },//VK_FORMAT_BC2_UNORM_BLOCK = 135,
    { 4, 0, 0, 1 },//VK_FORMAT_BC2_SRGB_BLOCK = 136,
    { 4, 0, 0, 1 },//VK_FORMAT_BC3_UNORM_BLOCK = 137,
    { 4, 0, 0, 1 },//VK_FORMAT_BC3_SRGB_BLOCK = 138,

    { 1, 0, 0, 1 },//VK_FORMAT_BC4_UNORM_BLOCK = 139,
    { 1, 0, 0, 1 },//VK_FORMAT_BC4_SNORM_BLOCK = 140,
    { 2, 0, 0, 1 },//VK_FORMAT_BC5_UNORM_BLOCK = 141,
    { 2, 0, 0, 1 },//VK_FORMAT_BC5_SNORM_BLOCK = 142,
    { 3, 0, 0, 2 },//VK_FORMAT_BC6H_UFLOAT_BLOCK = 143,
    { 3, 0, 0, 2 },//VK_FORMAT_BC6H_SFLOAT_BLOCK = 144,
    { 4, 0, 0, 1 },//VK_FORMAT_BC7_UNORM_BLOCK = 145,
    { 4, 0, 0, 1 },//VK_FORMAT_BC7_SRGB_BLOCK = 146,

    { 3, 0, 0, 1 },//VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147,
    { 3, 0, 0, 1 },//VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148,
    { 4, 0, 0, 1 },//VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149,
    { 4, 0, 0, 1 },//VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150,
    { 4, 0, 0, 1 },//VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151,
    { 4, 0, 0, 1 },//VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152,

    { 1, 0, 0, 1 },//VK_FORMAT_EAC_R11_UNORM_BLOCK = 153,
    { 1, 0, 0, 1 },//VK_FORMAT_EAC_R11_SNORM_BLOCK = 154,
    { 2, 0, 0, 1 },//VK_FORMAT_EAC_R11G11_UNORM_BLOCK = 155,
    { 2, 0, 0, 1 },//VK_FORMAT_EAC_R11G11_SNORM_BLOCK = 156,

    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 157,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_4x4_SRGB_BLOCK = 158,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_5x4_UNORM_BLOCK = 159,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_5x4_SRGB_BLOCK = 160,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_5x5_UNORM_BLOCK = 161,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_5x5_SRGB_BLOCK = 162,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_6x5_UNORM_BLOCK = 163,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_6x5_SRGB_BLOCK = 164,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_6x6_UNORM_BLOCK = 165,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_6x6_SRGB_BLOCK = 166,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_8x5_UNORM_BLOCK = 167,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_8x5_SRGB_BLOCK = 168,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_8x6_UNORM_BLOCK = 169,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_8x6_SRGB_BLOCK = 170,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_8x8_UNORM_BLOCK = 171,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_8x8_SRGB_BLOCK = 172,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_10x5_UNORM_BLOCK = 173,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_10x5_SRGB_BLOCK = 174,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_10x6_UNORM_BLOCK = 175,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_10x6_SRGB_BLOCK = 176,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_10x8_UNORM_BLOCK = 177,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_10x8_SRGB_BLOCK = 178,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_10x10_UNORM_BLOCK = 179,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_10x10_SRGB_BLOCK = 180,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_12x10_UNORM_BLOCK = 181,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_12x10_SRGB_BLOCK = 182,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_12x12_UNORM_BLOCK = 183,
    { 4, 0, 0, 1 },//VK_FORMAT_ASTC_12x12_SRGB_BLOCK = 184,
};
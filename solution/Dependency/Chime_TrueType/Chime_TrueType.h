// Chime_TrueType
// 
// Usage: https://github.com/EasyVulkan/Chime_TrueType
// 
// --------------------------------------------------------------------------------
// 
// MIT License
// Copyright (c) 2026 Citrus Qiao
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to use
// copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
// Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// --------------------------------------------------------------------------------

#pragma once
#include <memory>
#include <bit>
#include <map>
#include <span>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <iostream>
#include <format>
//#pragma warning(disable:4244)

#pragma region Macro
#ifndef CHIME_TRUETYPE_PRINT_ERROR
#define CHIME_TRUETYPE_PRINT_ERROR(...) (std::cout << std::format("[ chime::trueType ] ERROR\n" __VA_ARGS__) << '\n')
#endif
#ifndef CHIME_TRUETYPE_PRINT_INFO
#define CHIME_TRUETYPE_PRINT_INFO(...)  (std::cout << std::format("[ chime::trueType ] INFO\n" __VA_ARGS__) << '\n')
#endif
#ifndef CHIME_TRUETYPE_PRINT_EXTRA_INFO
#define CHIME_TRUETYPE_PRINT_EXTRA_INFO // To print extra info: #define CHIME_TRUETYPE_PRINT_EXTRA_INFO CHIME_TRUETYPE_PRINT_INFO
#endif
#ifndef CHIME_TRUETYPE_NAMESPACE
#define CHIME_TRUETYPE_NAMESPACE chime::trueType
#endif
#define CHIME_TRUETYPE_BEGIN namespace CHIME_TRUETYPE_NAMESPACE {
#define CHIME_TRUETYPE_END   }
#pragma endregion

CHIME_TRUETYPE_BEGIN
using int8 = int8_t;
using int16 = int8_t[2];
using int32 = int8_t[4];
using uint8 = uint8_t;
using uint16 = uint8_t[2];
using uint32 = uint8_t[4];
inline int8_t GetI8(const uint8_t* pData) {
	return pData[0];
}
inline int16_t GetI16(const uint8_t* pData) {
	return int16_t(pData[0] << 8) + pData[1];
}
inline uint8_t GetU8(const uint8_t* pData) {
	return pData[0];
}
inline uint16_t GetU16(const uint8_t* pData) {
	return (pData[0] << 8) + pData[1];
}
inline uint32_t GetU32(const uint8_t* pData) {
	return (pData[0] << 24) + (pData[1] << 16) + (pData[2] << 8) + pData[3];
}
inline float GetF2Dot14(const uint8_t* pData) {
	return GetI16(pData) / float(1 << 14);
}

inline std::unique_ptr<uint8_t[]> LoadFile(const char* filepath, size_t& fileSize) {
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	if (!file) {
		CHIME_TRUETYPE_PRINT_ERROR("Failed to open the file: {}", filepath);
		return {};
	}
	fileSize = size_t(file.tellg());
	std::unique_ptr pData = std::make_unique<uint8_t[]>(fileSize);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(pData.get()), fileSize);
	file.close();
	return pData;
}

#include "ttcHeaderParser.h"
#include "tableDirectoryParser.h"
#include "cmapParser.h"
#include "headParser.h"
#include "hheaParser.h"
#include "hmtxParser.h"
#include "maxpParser.h"
#include "nameParser.h"
#include "glyfParser.h"
#include "locaParser.h"
#include "kernParser.h"
#include "GPOSParser.h"
#include "OS_2Parser.h"
#include "rasterizer.h"

class ttfLoader {
public:
	using indexMapping = cmapParser::indexMapping;
	struct glyphRenderingInfo {
		float advanceWidth;
		float width;
		float leftSideBearing;
		float leftmostPixelBearing;
		uint16_t layerIndex;
		uint16_t offsetU;
		uint16_t sizeU;
	};
	struct kerning {
		uint16_t glyph0;
		uint16_t glyph1;
		float advance;
	};
protected:
	uint32_t sfntVersion = 0;
	float scale = 0;
	float fontHeight = 0;
	float fontAscent = 0;
	float fontLineSpacing = 0;
	float pixelDistanceScale = 0; // 0 if isn't SDF
	uint16_t glyphCount = 0;
	uint16_t imageLayerCount = 0;
	uint16_t imageLayerWidth = 0;
	uint16_t imageLayerHeight = 0;
	int16_t imageAscent = 0; // Include padding
	uint8_t imagePadding = 0;
	bool fontNameEncodingIsU16 = false;
	alignas(8) [[no_unique_address]] std::monostate endOfFixedLengthFields;
	decltype(nameParser::result_t::fullName) fontName;
	std::vector<indexMapping> glyphIndexMappings;
	std::vector<glyphRenderingInfo> glyphRenderingInfos;
	std::vector<kerning> kernings;
	std::unique_ptr<uint8_t[]> pImageData;
	/* Static */
	static inline uint32_t maxImageLayerCount = 2048;
	static inline uint32_t maxImageLayerWidth = 16384;
	/* Const Function */
	bool SaveLoaded_Ttfl(std::ofstream& file) const {
		// Compress image data
		auto RleEncode = [](const uint8_t* pData, size_t dataSize) {
			std::pair<std::vector<uint8_t>, std::vector<uint8_t>> result;
			auto& [counts, values] = result;
			uint8_t count = 0;
			uint8_t value = pData[0];
			for (size_t i = 1; i < dataSize; i++)
				if (value == pData[i] &&
					count != 255)
					count++;
				else
					counts.push_back(count),
					values.push_back(value),
					count = 0,
					value = pData[i];
			counts.push_back(count);
			values.push_back(value);
			return result;
		};
		uint64_t imageDataSize = ImageDataSize();
		auto sequences_imageData = RleEncode(pImageData.get(), imageDataSize);
		auto sequences_counts = RleEncode(sequences_imageData.first.data(), sequences_imageData.first.size());
		// Write file signature & compressed image data
		char reserved[4] = {}; // Reserved for future use
		uint64_t sequenceCount_counts = sequences_counts.first.size();
		uint64_t sequenceCount_imageData = sequences_imageData.first.size();
		file.
			write("ttfL", 4).
			write(reserved, 4).
			write(reinterpret_cast<char*>(&sequenceCount_counts), 8).
			write(reinterpret_cast<char*>(&sequenceCount_imageData), 8).
			write(reinterpret_cast<char*>(&imageDataSize), 8).
			write(reinterpret_cast<char*>(sequences_counts.first.data()), sequenceCount_counts).
			write(reinterpret_cast<char*>(sequences_counts.second.data()), sequenceCount_counts).
			write(reinterpret_cast<char*>(sequences_imageData.second.data()), sequenceCount_imageData).
			seekp((size_t(file.tellp()) + 3) & ~3);
		// Write everything loaded except image data
		uint32_t elementCounts[] = {
			uint32_t(fontName.size()),
			uint32_t(glyphIndexMappings.size()),
			uint32_t(glyphRenderingInfos.size()),
			uint32_t(kernings.size()),
		};
		uint32_t dataSizes[] = {
			uint32_t(elementCounts[0] * sizeof(char)),
			uint32_t(elementCounts[1] * sizeof(indexMapping)),
			uint32_t(elementCounts[2] * sizeof(glyphRenderingInfo)),
			uint32_t(elementCounts[3] * sizeof(kerning)),
		};
		file.
			write(reinterpret_cast<const char*>(this), offsetof(ttfLoader, endOfFixedLengthFields)).
			write(reinterpret_cast<char*>(&elementCounts[0]), 4).
			write(reinterpret_cast<const char*>(fontName.data()), dataSizes[0]).
			write(reinterpret_cast<char*>(&elementCounts[1]), 4).
			write(reinterpret_cast<const char*>(glyphIndexMappings.data()), dataSizes[1]).
			write(reinterpret_cast<char*>(&elementCounts[2]), 4).
			write(reinterpret_cast<const char*>(glyphRenderingInfos.data()), dataSizes[2]).
			write(reinterpret_cast<char*>(&elementCounts[3]), 4).
			write(reinterpret_cast<const char*>(kernings.data()), dataSizes[3]);
		return true;
	}
	bool SaveLoaded_Png(std::ofstream& file) const {
	#ifndef INCLUDE_STB_IMAGE_WRITE_H
		CHIME_TRUETYPE_PRINT_ERROR("If you want to save loaded data to a PNG file, stb_image_write.h must be included before this header!");
		return false;
		uint8_t* (*stbi_write_png_to_mem)(const uint8_t*, int, int, int, int, int*);
	#endif
		// Create PNG data
		int pngSize;
		std::unique_ptr<uint8_t[]> pPngData(stbi_write_png_to_mem(pImageData.get(), 0, imageLayerWidth, imageLayerCount * imageLayerHeight, 1, &pngSize));
		if (!pPngData) {
			CHIME_TRUETYPE_PRINT_ERROR("Failed to create a PNG!");
			return false;
		}
		// Prepare private chunk
		uint32_t elementCounts[] = {
			uint32_t(fontName.size()),
			uint32_t(glyphIndexMappings.size()),
			uint32_t(glyphRenderingInfos.size()),
			uint32_t(kernings.size()),
		};
		uint32_t dataSizes[] = {
			uint32_t(elementCounts[0] * sizeof(char)),
			uint32_t(elementCounts[1] * sizeof(indexMapping)),
			uint32_t(elementCounts[2] * sizeof(glyphRenderingInfo)),
			uint32_t(elementCounts[3] * sizeof(kerning)),
		};
		uint32_t chunkDataSize = offsetof(ttfLoader, endOfFixedLengthFields) + dataSizes[0] + dataSizes[1] + dataSizes[2] + +dataSizes[3] + sizeof(elementCounts) + 3;
		uint32_t crc = 0; // Ignore
		if (uint64_t(chunkDataSize) + 15 + pngSize > INT32_MAX) {
			CHIME_TRUETYPE_PRINT_ERROR("Failed to save the PNG file! File size must be less than 2G!");
			return false;
		}
		// Write PNG file signature & IHDR chunk
		file.write(reinterpret_cast<char*>(pPngData.get()), 33);
		// Write everything loaded except image data
		char paddingBytes[3] = {}; // Reserved for future use
		file.
			write(reinterpret_cast<char*>(&chunkDataSize) + 3, 1).
			write(reinterpret_cast<char*>(&chunkDataSize) + 2, 1).
			write(reinterpret_cast<char*>(&chunkDataSize) + 1, 1).
			write(reinterpret_cast<char*>(&chunkDataSize) + 0, 1).
			write("ttFM", 4).
			write(paddingBytes, 3).
			write(reinterpret_cast<const char*>(this), offsetof(ttfLoader, endOfFixedLengthFields)).
			write(reinterpret_cast<char*>(&elementCounts[0]), 4).
			write(reinterpret_cast<const char*>(fontName.data()), dataSizes[0]).
			write(reinterpret_cast<char*>(&elementCounts[1]), 4).
			write(reinterpret_cast<const char*>(glyphIndexMappings.data()), dataSizes[1]).
			write(reinterpret_cast<char*>(&elementCounts[2]), 4).
			write(reinterpret_cast<const char*>(glyphRenderingInfos.data()), dataSizes[2]).
			write(reinterpret_cast<char*>(&elementCounts[3]), 4).
			write(reinterpret_cast<const char*>(kernings.data()), dataSizes[3]).
			write(reinterpret_cast<char*>(&crc), 4);
		// Write rest of PNG
		file.write(reinterpret_cast<char*>(pPngData.get() + 33), pngSize - 33);
		return true;
	}
	/* Non-const Function */
	void RasterizeGlyphs(glyfParser& glyfParser, const uint8_t* pTable_glyf, const std::vector<uint32_t>& glyphDescriptionLengths, const rasterizer::bounds* glyphBoundingBoxes, int8_t sdfPadding) {
		static constexpr float squarePrecision = 0.35f * 0.35f;
		auto RasterizeGlyphs_Inner = [this](auto& glyfParser, const std::vector<uint32_t>& glyphDescriptionLengths, const rasterizer::bounds* glyphBoundingBoxes, auto... sdfPadding) {
			std::vector<glyfParser::pointF32> points;
			std::vector<rasterizer::point> tessellatedPoints;
			rasterizer rasterizer;
			pImageData = std::make_unique<uint8_t[]>(imageLayerCount * imageLayerWidth * imageLayerHeight);
			for (auto& i : glyfParser.Result().glyphRecords) {
				size_t glyphIndex = &i - glyfParser.Result().glyphRecords.data();
				if (glyphIndex % 100 == 0)
					CHIME_TRUETYPE_PRINT_EXTRA_INFO("Rendering glyphs: {}/{}", glyphIndex, glyphCount);
				if (glyphRenderingInfos[glyphIndex].sizeU == 0)
					continue;
				const glyfParser::pointF32* pFirstPoint;
				points.clear();
				tessellatedPoints.clear();
				if (i.indexIntoSimpleGlyphs != UINT16_MAX)
					glyfParser.GetSimpleGlyphPoints(i.indexIntoSimpleGlyphs, points, scale),
					TessellateSimpleGlyphContour(glyfParser.Result(), i.indexIntoSimpleGlyphs, pFirstPoint = points.data(), squarePrecision, tessellatedPoints);
				else
					glyfParser.GetCompoundGlyphPoints(i.indexIntoCompoundGlyphs, /*sfntVersion,*/ points, scale),
					TessellateCompoundGlyphContour(glyfParser.Result(), i.indexIntoCompoundGlyphs, pFirstPoint = points.data(), squarePrecision, tessellatedPoints);
				size_t offset = glyphRenderingInfos[glyphIndex].offsetU + imagePadding + size_t(glyphRenderingInfos[glyphIndex].layerIndex * imageLayerHeight + imageAscent - glyphBoundingBoxes[glyphIndex].top) * imageLayerWidth;
				if constexpr (sizeof...(sdfPadding))
					offset -= std::get<0>(std::tie(sdfPadding...)) * (1 + imageLayerWidth),
					rasterizer.RasterizeSdf(tessellatedPoints, glyphBoundingBoxes[glyphIndex], &pImageData[offset], imageLayerWidth, imageLayerHeight, imageAscent, std::get<0>(std::tie(sdfPadding...)));
				else
					rasterizer.Rasterize(tessellatedPoints, glyphBoundingBoxes[glyphIndex], &pImageData[offset], imageLayerWidth, imageLayerHeight, imageAscent);
			}
		};
		if (glyfParser.Result().points.empty())
			glyfParser = std::decay_t<decltype(glyfParser)>(pTable_glyf, glyphDescriptionLengths);
		if (sdfPadding > 0)
			pixelDistanceScale = 128.f / sdfPadding,
			RasterizeGlyphs_Inner(glyfParser, glyphDescriptionLengths, glyphBoundingBoxes, sdfPadding);
		else
			pixelDistanceScale = (sdfPadding == 0) * 255.f,
			RasterizeGlyphs_Inner(glyfParser, glyphDescriptionLengths, glyphBoundingBoxes);
	}
	void RasterizeGlyphs_Stb(const uint8_t* fileBytes, uint32_t fontOffset, const rasterizer::bounds* glyphBoundingBoxes, int8_t sdfPadding, const char* filepath) {
	#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
		stbtt_fontinfo info;
		if (!stbtt_InitFont(&info, fileBytes, fontOffset)) {
			filepath ?
				CHIME_TRUETYPE_PRINT_ERROR("Failed to parse the file: {}", filepath) :
				CHIME_TRUETYPE_PRINT_ERROR("Failed to parse TTF data from the given address!");
			return;
		}
		pImageData = std::make_unique<uint8_t[]>(imageLayerCount * imageLayerWidth * imageLayerHeight);
		if (sdfPadding > 0) {
			pixelDistanceScale = 128.f / sdfPadding;
			for (uint16_t i = 0; i < glyphCount; i++) {
				if (i % 100 == 0)
					CHIME_TRUETYPE_PRINT_EXTRA_INFO("Rendering glyphs: {}/{}", i, glyphCount);
				if (glyphRenderingInfos[i].sizeU == 0)
					continue;
				size_t offset = glyphRenderingInfos[i].offsetU + imagePadding - sdfPadding + size_t(glyphRenderingInfos[i].layerIndex * imageLayerHeight + imageAscent - glyphBoundingBoxes[i].top - sdfPadding) * imageLayerWidth;
				int width, height;
				if (uint8_t* pImageData_src = stbtt_GetGlyphSDF(&info, scale, i, sdfPadding, 128, pixelDistanceScale, &width, &height, nullptr, nullptr)) {
					uint8_t* pImageData_dst = &pImageData[offset];
					for (int i = 0; i < height; i++)
						memcpy(pImageData_dst + i * imageLayerWidth, pImageData_src + i * width, width);
					stbtt_FreeSDF(pImageData_src, nullptr);
				}
			}
		}
		else {
			pixelDistanceScale = (sdfPadding == 0) * 255.f;
			for (uint16_t i = 0; i < glyphCount; i++) {
				if (i % 100 == 0)
					CHIME_TRUETYPE_PRINT_EXTRA_INFO("Rendering glyphs: {}/{}", i, glyphCount);
				if (glyphRenderingInfos[i].sizeU == 0)
					continue;
				auto& [left, bottom, right, top] = glyphBoundingBoxes[i];
				size_t offset = glyphRenderingInfos[i].offsetU + imagePadding + size_t(glyphRenderingInfos[i].layerIndex * imageLayerHeight + imageAscent - top) * imageLayerWidth;
				stbtt_MakeGlyphBitmap(&info, &pImageData[offset], right - left, top - bottom, imageLayerWidth, scale, scale, i);
			}
		}
	#endif
	}
	bool LoadLoaded_Internal(const uint8_t* fileBytes, size_t fileSize, const char* filepath = nullptr) {
		const uint8_t* pData_src = fileBytes;
		// Determine file type, load image data
		if (GetU32(pData_src) == 'ttfL') {
			auto RleDecode = [](const uint8_t* counts, const uint8_t* values, size_t sequenceCount, size_t resultDataSize) {
				std::unique_ptr pResult = std::make_unique<uint8_t[]>(resultDataSize);
				uint8_t* pData = pResult.get();
				for (size_t i = 0; i < sequenceCount; i++)
					std::fill(pData, pData + counts[i] + 1, values[i]),
					pData += counts[i] + 1;
				return pResult;
			};
			uint64_t sequenceCount_counts = *reinterpret_cast<const uint64_t*>(pData_src += 8);
			uint64_t sequenceCount_imageData = *reinterpret_cast<const uint64_t*>(pData_src += 8);
			uint64_t imageDataSize = *reinterpret_cast<const uint64_t*>(pData_src += 8);
			pData_src += 8;
			std::unique_ptr counts = RleDecode(pData_src, pData_src + sequenceCount_counts, sequenceCount_counts, sequenceCount_imageData);
			pImageData = RleDecode(counts.get(), pData_src + sequenceCount_counts * 2, sequenceCount_imageData, imageDataSize);
			pData_src += (3 + sequenceCount_counts * 2 + sequenceCount_imageData) & ~3;
		}
		else if (GetU32(pData_src) == '\x89PNG' &&
			GetU32(pData_src + 37) == 'ttFM') {
		#ifndef STBI_INCLUDE_STB_IMAGE_H
			CHIME_TRUETYPE_PRINT_ERROR("If you want to load a PNG file, stb_image.h must be included before this header!");
			return false;
			uint8_t* (*stbi_load_from_memory)(const uint8_t*, int, int*, int*, int*, int);
		#endif
			if (fileSize > INT32_MAX) {
				filepath ?
					CHIME_TRUETYPE_PRINT_ERROR("Failed to load the PNG file! File size must be less than 2G!\nFile: {}", filepath) :
					CHIME_TRUETYPE_PRINT_ERROR("Failed to load PNG data from the given address! Data size must be less than 2G!");
				return false;
			}
			int width, length, channelCount;
			pImageData = std::unique_ptr<uint8_t[]>(stbi_load_from_memory(fileBytes, int(fileSize), &width, &length, &channelCount, 1));
			if (!pImageData) {
				filepath ?
					CHIME_TRUETYPE_PRINT_ERROR("Failed to load PNG data from the file: {}", filepath) :
					CHIME_TRUETYPE_PRINT_ERROR("Failed to load PNG data from the given address!");
				return false;
			}
			pData_src += 44;
		}
		else
			return false;
		// Load everything except image data
		memcpy(this, pData_src, offsetof(ttfLoader, endOfFixedLengthFields));
		pData_src += offsetof(ttfLoader, endOfFixedLengthFields);

		uint32_t elementCount;
		uint32_t dataSize;
		memcpy(&elementCount, pData_src, 4);
		pData_src += 4;
		fontName.resize(elementCount);
		memcpy(fontName.data(), pData_src, dataSize = elementCount * sizeof(char));
		pData_src += dataSize;

		memcpy(&elementCount, pData_src, 4);
		pData_src += 4;
		glyphIndexMappings.resize(elementCount);
		memcpy(glyphIndexMappings.data(), pData_src, dataSize = elementCount * sizeof(indexMapping));
		pData_src += dataSize;

		memcpy(&elementCount, pData_src, 4);
		pData_src += 4;
		glyphRenderingInfos.resize(elementCount);
		memcpy(glyphRenderingInfos.data(), pData_src, dataSize = elementCount * sizeof(glyphRenderingInfo));
		pData_src += dataSize;

		memcpy(&elementCount, pData_src, 4);
		pData_src += 4;
		kernings.resize(elementCount);
		memcpy(kernings.data(), pData_src, elementCount * sizeof(kerning));
		return true;
	}
	/* Static Function */
	static void SortUint32(std::vector<uint32_t>& values) {
		auto CountingSort = [](const uint8_t* pData, std::vector<uint32_t>& indices, uint32_t* indices_temp) {
			size_t count = indices.size();
			uint32_t sums[256]{};
			for (size_t i = 0; i < count; i++)
				sums[pData[sizeof(uint32_t) * indices[i]]]++;
			for (size_t i = 1; i < 256; i++)
				sums[i] += sums[i - 1];
			std::memcpy(indices_temp, indices.data(), sizeof(uint32_t) * count);
			for (int32_t i = count - 1; i >= 0; i--)
				indices[--sums[pData[sizeof(uint32_t) * indices_temp[i]]]] = indices_temp[i];
		};
		size_t count = values.size();
		std::vector<uint32_t> indices(values.size());
		std::vector<uint32_t> temp(values.size());
		std::iota(indices.begin(), indices.end(), 0);
		if constexpr (std::endian::native == std::endian::little)
			for (size_t i = 0; i < sizeof(int32_t); i++)
				CountingSort(reinterpret_cast<const uint8_t*>(values.data()) + i, indices, temp.data());
		else
			for (int32_t i = sizeof(int32_t) - 1; i >= 0; i--)
				CountingSort(reinterpret_cast<const uint8_t*>(values.data()) + i, indices, temp.data());
		values.resize(values.size() * 2); // Not reserve(...), Avoid UB
		uint32_t* glyphIndices_copy = &values[count];
		std::memcpy(glyphIndices_copy, values.data(), sizeof(uint32_t) * count);
		for (size_t i = 0; i < count; i++)
			values[i] = glyphIndices_copy[indices[i]];
		values.resize(count);
	}
	static void TessellateSimpleGlyphContour(const glyfParser::result_t& glyfParseResult, uint16_t indexIntoSimpleGlyphs, const glyfParser::pointF32*& pFirstPoint, float squarePrecision, std::vector<rasterizer::point>& points_out) {
		for (auto& i : glyfParseResult.simpleGlyphs[indexIntoSimpleGlyphs].contourPointCounts) {
			rasterizer::TessellateContour(std::span{ pFirstPoint, i }, points_out, squarePrecision);
			pFirstPoint += i;
		}
	}
	static void TessellateCompoundGlyphContour(const glyfParser::result_t& glyfParseResult, uint16_t indexIntoCompoundGlyphs, const glyfParser::pointF32*& pFirstPoint, float squarePrecision, std::vector<rasterizer::point>& points_out) {
		for (auto& i : glyfParseResult.compoundGlyphs[indexIntoCompoundGlyphs].components)
			if (const glyfParser::glyphRecord& glyphRecord = glyfParseResult.glyphRecords[i.glyphIndex];
				glyphRecord.indexIntoSimpleGlyphs != UINT16_MAX)
				TessellateSimpleGlyphContour(glyfParseResult, glyphRecord.indexIntoSimpleGlyphs, pFirstPoint, squarePrecision, points_out);
			else
				TessellateCompoundGlyphContour(glyfParseResult, glyphRecord.indexIntoCompoundGlyphs, pFirstPoint, squarePrecision, points_out);
	}
	static bool LoadTtf_Internal(auto& self, const uint8_t* fileBytes, uint32_t fontOffset, float fontHeight, int8_t sdfPadding, const char* filepath = nullptr) {
		static constexpr uint32_t maxFontHeight = 7200;
		static constexpr bool rasterize = !requires{ self.glyfData; };
		if (self.sfntVersion)
			self.Reset();
		// Check font height
		if constexpr (rasterize)
			if (fontHeight <= 0) {
				CHIME_TRUETYPE_PRINT_ERROR("Font height must be greater than 0!");
				return false;
			}
			else if (fontHeight > maxFontHeight) {
				CHIME_TRUETYPE_PRINT_ERROR("Font height must be less than or equal to {}!", maxFontHeight);
				return false;
			}
		// If it's a TTC file, select the first font
		if (fontOffset == UINT32_MAX) {
			ttcHeaderParser ttcHeaderParser(fileBytes);
			fontOffset = ttcHeaderParser.Result().fontOffsets.empty() ?
				0 :
				ttcHeaderParser.Result().fontOffsets[0];
		}
		// Parse table directory
		tableDirectoryParser tableDirectoryParser(fileBytes + fontOffset, filepath);
		if (tableDirectoryParser.Result().tableRecords.empty())
			return false;
		self.sfntVersion = tableDirectoryParser.Result().sfntVersion;
		auto
			pTable_cmap = tableDirectoryParser.FindTable(fileBytes, 'cmap', "cmap", filepath),
			pTable_head = tableDirectoryParser.FindTable(fileBytes, 'head', "head", filepath),
			pTable_hhea = tableDirectoryParser.FindTable(fileBytes, 'hhea', "hhea", filepath),
			pTable_hmtx = tableDirectoryParser.FindTable(fileBytes, 'hmtx', "hmtx", filepath),
			pTable_maxp = tableDirectoryParser.FindTable(fileBytes, 'maxp', "maxp", filepath),
			pTable_name = tableDirectoryParser.FindTable(fileBytes, 'name', "name", filepath),
			pTable_glyf = tableDirectoryParser.FindTable(fileBytes, 'glyf', "glyf", filepath),
			pTable_loca = tableDirectoryParser.FindTable(fileBytes, 'loca', "loca", filepath),
			pTable_kern = tableDirectoryParser.FindTable(fileBytes, 'kern'),
			pTable_GPOS = tableDirectoryParser.FindTable(fileBytes, 'GPOS'),
			pTable_OS_2 = tableDirectoryParser.FindTable(fileBytes, 'OS/2');
		if (!pTable_cmap) return false;
		if (!pTable_head) return false;
		if (!pTable_hhea) return false;
		if (!pTable_hmtx) return false;
		if (!pTable_maxp) return false;
		if (!pTable_name) return false;
		if (!pTable_glyf) return false;
		if (!pTable_loca) return false;
		// Parse 'name' table
		nameParser nameParser(pTable_name);
		self.fontName = std::move(nameParser.Result().fullName);
		self.fontNameEncodingIsU16 = nameParser.Result().nameEncodingIsU16;
		// Parse 'cmap' table
		cmapParser cmapParser(pTable_cmap);
		if (cmapParser.Result().indexMappings.empty()) {
			filepath ?
				CHIME_TRUETYPE_PRINT_ERROR("Failed to parse the 'cmap' table!\nFile: {}", filepath) :
				CHIME_TRUETYPE_PRINT_ERROR("Failed to parse the 'cmap' table!");
			return false;
		}
		self.glyphIndexMappings = std::move(cmapParser.Result().indexMappings);
		// Parse 'hhea' table
		hheaParser hheaParser(pTable_hhea);
		auto [ascent, descent, lineGap, hMetricCount] = hheaParser.Result();
		// Parse 'OS/2' table if it exists
		if (pTable_OS_2) {
			OS_2Parser OS_2Parser(pTable_OS_2);
			ascent = OS_2Parser.Result().typoAscender;
			descent = OS_2Parser.Result().typoDescender;
			lineGap = OS_2Parser.Result().typoLineGap;
		}
		// Parse 'head' table
		headParser headParser(pTable_head);
		// Parse 'maxp' table
		maxpParser maxpParser(pTable_maxp);
		self.glyphCount = maxpParser.Result().glyphCount;
		CHIME_TRUETYPE_PRINT_EXTRA_INFO("Glyph count: {}", self.glyphCount);
		// Parse 'loca' table
		locaParser locaParser(pTable_loca, headParser.Result().locaFormat, self.glyphCount);
		if (locaParser.Result().glyphDescriptionLengths.empty()) {
			filepath ?
				CHIME_TRUETYPE_PRINT_ERROR("Failed to parse the 'loca' table!\nFile: {}", filepath) :
				CHIME_TRUETYPE_PRINT_ERROR("Failed to parse the 'loca' table!");
			return false;
		}
		// Parse 'glyf' table
		glyfParser glyfParser(pTable_glyf, locaParser.Result().glyphDescriptionLengths
		#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
			, rasterize
		#endif
		);
		// Parse 'hmtx' table
		hmtxParser hmtxParser(pTable_hmtx, hMetricCount, self.glyphCount);
		if (hmtxParser.Result().horizontalMetricPairs.empty()) {
			filepath ?
				CHIME_TRUETYPE_PRINT_ERROR("Failed to parse the 'hmtx' table!\nFile: {}", filepath) :
				CHIME_TRUETYPE_PRINT_ERROR("Failed to parse the 'hmtx' table!");
			return false;
		}
		if constexpr (rasterize) {
			// Calculate scale, ascent and line spacing
			self.scale = fontHeight / (ascent - descent);
			self.fontHeight = fontHeight;
			self.fontAscent = ascent * self.scale;
			self.fontLineSpacing = fontHeight + lineGap * self.scale;
			// Calculate image padding
			self.imagePadding = std::max(int32_t(sdfPadding), 0) + std::clamp(int32_t(fontHeight / 16), 1, 8);
			// Get kernings
			if (pTable_GPOS) {
				GPOSParser GPOSParser(pTable_GPOS);
				self.kernings.reserve(GPOSParser.Result().kerningPairs.size());
				for (auto& i : GPOSParser.Result().kerningPairs)
					self.kernings.emplace_back(i.glyph0, i.glyph1, i.value * self.scale);
			}
			if (pTable_kern && self.kernings.empty()) {
				kernParser kernParser(pTable_kern);
				self.kernings.reserve(kernParser.Result().kerningPairs.size());
				for (auto& i : kernParser.Result().kerningPairs)
					self.kernings.emplace_back(i.glyph0, i.glyph1, i.value * self.scale);
			}
			// Get indices of glyphs to be rasterized
			std::vector<uint32_t> glyphIndices(self.glyphIndexMappings.size() + 1);
			uint32_t* pGlyphIndex = &glyphIndices[1];
			for (auto& i : self.glyphIndexMappings)
				*pGlyphIndex++ = i.glyphIndex;
			SortUint32(glyphIndices);
			// Get all glyph infos
			std::vector<rasterizer::bounds> glyphBoundingBoxes(self.glyphCount);
			self.glyphRenderingInfos.resize(self.glyphCount);
			pGlyphIndex = glyphIndices.data();
			uint32_t* pGlyphIndexBack = &glyphIndices.back();
			uint32_t maxGlyphUnitWidth = 0;
			for (uint16_t i = 0; i < self.glyphCount; i++) {
				if (i == *pGlyphIndex) {
					// Get bouding box
					auto& [xMin, yMin, xMax, yMax] = glyfParser.Result().glyphRecords[i].boundingBox;
					glyphBoundingBoxes[i] = {
						int16_t(std::floor(xMin * self.scale)),
						int16_t(std::floor(yMin * self.scale)),
						int16_t(std::ceil(xMax * self.scale)),
						int16_t(std::ceil(yMax * self.scale)) // May be more than fontAscent
					};
					// Get horizontal metrics
					auto& [advanceWidth, leftSideBearing] = hmtxParser.Result().horizontalMetricPairs[i];
					self.glyphRenderingInfos[i] = {
						advanceWidth * self.scale,
						(xMax - xMin) * self.scale,
						leftSideBearing * self.scale,
						(leftSideBearing - xMin) * self.scale + glyphBoundingBoxes[i].left
					};
					self.glyphRenderingInfos[i].sizeU = glyphBoundingBoxes[i].right - glyphBoundingBoxes[i].left + self.imagePadding * 2;
					if (maxGlyphUnitWidth < self.glyphRenderingInfos[i].sizeU)
						maxGlyphUnitWidth = self.glyphRenderingInfos[i].sizeU;
				}
				while (*pGlyphIndex == i && pGlyphIndex < pGlyphIndexBack)
					pGlyphIndex++;
			}
			// Determine image width
			if (maxGlyphUnitWidth > maxImageLayerWidth ||
				self.glyphCount * maxGlyphUnitWidth > maxImageLayerCount * maxImageLayerWidth) {
				float maxFontHeight = fontHeight * (maxImageLayerWidth - self.imagePadding * 2) / (maxGlyphUnitWidth - self.imagePadding * 2);
				maxFontHeight = maxImageLayerCount < self.glyphCount ?
					std::floor(maxFontHeight * maxImageLayerCount / self.glyphCount) :
					std::floor(maxFontHeight);
				filepath ?
					CHIME_TRUETYPE_PRINT_ERROR("For the font being loaded, font height must be less than or equal to {}!", maxFontHeight) :
					CHIME_TRUETYPE_PRINT_ERROR("For the font being loaded, font height must be less than or equal to {}!\nFile: {}", maxFontHeight, filepath);
				return false;
			}
			self.imageLayerWidth = maxGlyphUnitWidth * std::min(maxImageLayerWidth / maxGlyphUnitWidth, 32u);
			// Calculate image vertical metrics
			int32_t maxTop = int32_t(std::ceil(headParser.Result().yMax * self.scale));
			int32_t minBottom = int32_t(std::floor(headParser.Result().yMin * self.scale));
			self.imageLayerHeight = maxTop - minBottom + self.imagePadding * 2;
			self.imageAscent = maxTop + self.imagePadding;
			// Calculate glyph positions in the image
			uint32_t offsetU = 0;
			for (uint16_t i = 0; i < self.glyphCount; i++) {
				if (offsetU + self.glyphRenderingInfos[i].sizeU > self.imageLayerWidth)
					self.imageLayerCount++,
					offsetU = 0;
				self.glyphRenderingInfos[i].layerIndex = self.imageLayerCount;
				self.glyphRenderingInfos[i].offsetU = offsetU;
				offsetU += self.glyphRenderingInfos[i].sizeU;
			}
			self.imageLayerCount++;
			// Rasterize all glyphs
			self.pImageData = nullptr;
			self.RasterizeGlyphs_Stb(fileBytes, fontOffset, glyphBoundingBoxes.data(), sdfPadding, filepath);
			if (self.pImageData) return true;
			self.RasterizeGlyphs(glyfParser, pTable_glyf, locaParser.Result().glyphDescriptionLengths, glyphBoundingBoxes.data(), sdfPadding);
			if (self.pImageData) return true;
			CHIME_TRUETYPE_PRINT_ERROR("Failed to render glyphs!");
			return false;
		}
		else {
			self.fontHeight = ascent - descent;
			self.fontAscent = ascent;
			self.fontLineSpacing = self.fontHeight + lineGap;
			self.maxGlyphTop = headParser.Result().yMax;
			// Get kernings
			if (pTable_GPOS) {
				GPOSParser GPOSParser(pTable_GPOS);
				self.kernings.reserve(GPOSParser.Result().kerningPairs.size());
				for (auto& i : GPOSParser.Result().kerningPairs)
					self.kernings.emplace_back(i.glyph0, i.glyph1, i.value);
			}
			if (pTable_kern && self.kernings.empty()) {
				kernParser kernParser(pTable_kern);
				self.kernings.reserve(kernParser.Result().kerningPairs.size());
				for (auto& i : kernParser.Result().kerningPairs)
					self.kernings.emplace_back(i.glyph0, i.glyph1, i.value);
			}
			// Get all glyph infos
			self.glyphRenderingInfos.resize(self.glyphCount);
			for (uint16_t i = 0; i < self.glyphCount; i++) {
				auto& [xMin, yMin, xMax, yMax] = glyfParser.Result().glyphRecords[i].boundingBox;
				auto& [advanceWidth, leftSideBearing] = hmtxParser.Result().horizontalMetricPairs[i];
				self.glyphRenderingInfos[i] = {
					float(advanceWidth),
					float(xMax - xMin),
					float(leftSideBearing),
					float(leftSideBearing)
				};
			}
			self.glyfData = std::move(glyfParser);
			return true;
		}
	}
	static bool LoadTtc_Internal(auto& self, const uint8_t* fileBytes, uint32_t fontIndex, float fontHeight, int8_t sdfPadding, const char* filepath = nullptr) {
		ttcHeaderParser ttcHeaderParser(fileBytes);
		if (ttcHeaderParser.Result().fontOffsets.empty())
			return LoadTtf_Internal(self, fileBytes, 0, fontHeight, sdfPadding, filepath);
		else if (fontIndex < ttcHeaderParser.Result().fontOffsets.size())
			return LoadTtf_Internal(self, fileBytes, ttcHeaderParser.Result().fontOffsets[fontIndex], fontHeight, sdfPadding, filepath);
		return false;
	}
	static auto GetFontNames_Internal(const uint8_t* fileBytes, const char* filepath = nullptr) {
		std::vector<nameParser::result_t> fontNames;
		ttcHeaderParser ttcHeaderParser(fileBytes);
		if (ttcHeaderParser.Result().fontOffsets.empty()) {
			tableDirectoryParser tableDirectoryParser(fileBytes, filepath);
			if (tableDirectoryParser.Result().tableRecords.empty())
				return fontNames;
			if (const uint8_t* pTable_name = tableDirectoryParser.FindTable(fileBytes, 'name', "name", filepath))
				fontNames.push_back(nameParser(pTable_name).Result());
		}
		else
			for (auto& i : ttcHeaderParser.Result().fontOffsets)
				if (const uint8_t* pTable_name = tableDirectoryParser(fileBytes + i, filepath).FindTable(fileBytes, 'name', "name", filepath))
					fontNames.push_back(nameParser(pTable_name).Result());
		return fontNames;
	}
public:
	ttfLoader() = default;
	ttfLoader(const char* filepath_ttf, float fontHeight, int8_t sdfPadding = -1) {
		LoadTtf(filepath_ttf, fontHeight, sdfPadding);
	}
	ttfLoader(const uint8_t* fileBytes_ttf, float fontHeight, int8_t sdfPadding = -1) {
		LoadTtf(fileBytes_ttf, fontHeight, sdfPadding);
	}
	ttfLoader(const char* filepath_preLoaded) {
		LoadLoaded(filepath_preLoaded);
	}
	ttfLoader(const uint8_t* fileBytes_preLoaded, size_t fileSize) {
		LoadLoaded(fileBytes_preLoaded, fileSize);
	}
	ttfLoader(ttfLoader&&) = default;
	/* Getter */
	uint32_t SfntVersion() const { return sfntVersion; }
	float Scale() const { return scale; }
	float FontHeight() const { return fontHeight; }
	float FontAscent() const { return fontAscent; }
	float FontLineSpacing() const { return fontLineSpacing; }
	float PixelDistanceScale() const { return pixelDistanceScale; }
	uint16_t GlyphCount() const { return glyphCount; }
	uint16_t ImageLayerCount() const { return imageLayerCount; }
	uint16_t ImageLayerWidth() const { return imageLayerWidth; }
	uint16_t ImageLayerHeight() const { return imageLayerHeight; }
	int16_t ImageAscent() const { return imageAscent; }
	uint8_t ImagePadding() const { return imagePadding; }
	bool FontNameEncodingIsU16() const { return fontNameEncodingIsU16; }
	const auto& FontName() const { return fontName; }
	const auto& GlyphIndexMappings() const { return glyphIndexMappings; }
	const auto& GlyphRenderingInfos() const { return glyphRenderingInfos; }
	const auto& Kernings() const { return kernings; }
	const uint8_t* PImageData() const { return pImageData.get(); }
	size_t ImageDataSize() const { return size_t(imageLayerCount) * imageLayerWidth * imageLayerHeight; }
	/* Const Function */
	bool SaveLoaded(const char* filepath) const {
		if (glyphRenderingInfos.empty()) {
			CHIME_TRUETYPE_PRINT_ERROR("Call LoadTtf(...) first, then you may call SaveLoaded(...) before calling MoveGlyphRenderingInfosTo(...).");
			return false;
		}
		std::ofstream file(filepath, std::ios::binary);
		if (!file) {
			CHIME_TRUETYPE_PRINT_ERROR("Failed to create an ofstream!\nFile: {}", filepath);
			return false;
		}
		std::string _filepath(filepath);
		bool result = _filepath.rfind(".png") == _filepath.size() - 4 ?
			SaveLoaded_Png(file) :
			SaveLoaded_Ttfl(file);
		if (result)
			CHIME_TRUETYPE_PRINT_INFO("Save loaded data to the file: {}", filepath);
		file.close();
		return result;
	}
	void StoreGlyphIndexMappingsTo(std::map<uint32_t, uint16_t>& glyphIndexMappings) const {
		for (auto& i : this->glyphIndexMappings)
			glyphIndexMappings.emplace(i.codePoint, i.glyphIndex);
	}
	void StoreGlyphIndexMappingsTo(std::unordered_map<uint32_t, uint16_t>& glyphIndexMappings) const {
		for (auto& i : this->glyphIndexMappings)
			glyphIndexMappings.emplace(i.codePoint, i.glyphIndex);
	}
	void StoreGlyphRenderingInfosTo(std::vector<glyphRenderingInfo>& glyphRenderingInfos) const {
		glyphRenderingInfos = this->glyphRenderingInfos;
	}
	void StoreKerningsTo(std::map<uint16_t, std::map<uint16_t, float>>& kernings) const {
		for (auto& i : this->kernings)
			kernings.try_emplace(i.glyph0).first->second.emplace(i.glyph1, i.advance);
	}
	void StoreKerningsTo(std::unordered_map<uint16_t, std::unordered_map<uint16_t, float>>& kernings) const {
		for (auto& i : this->kernings)
			kernings.try_emplace(i.glyph0).first->second.emplace(i.glyph1, i.advance);
	}
	/* Non-const Function */
	bool LoadTtf(const char* filepath, float fontHeight, int8_t sdfPadding = -1) {
		size_t fileSize;
		if (std::unique_ptr bytes = LoadFile(filepath, fileSize))
			return LoadTtf_Internal(*this, bytes.get(), -1, fontHeight, sdfPadding, filepath);
		return false;
	}
	bool LoadTtf(const uint8_t* fileBytes, float fontHeight, int8_t sdfPadding = -1) {
		return LoadTtf_Internal(*this, fileBytes, -1, fontHeight, sdfPadding);
	}
	bool LoadTtc(const char* filepath, uint32_t fontIndex, float fontHeight, int8_t sdfPadding = -1) {
		size_t fileSize;
		if (std::unique_ptr bytes = LoadFile(filepath, fileSize))
			return LoadTtc_Internal(*this, bytes.get(), fontIndex, fontHeight, sdfPadding, filepath);
		return false;
	}
	bool LoadTtc(const uint8_t* fileBytes, uint32_t fontIndex, float fontHeight, int8_t sdfPadding = -1) {
		return LoadTtc_Internal(*this, fileBytes, fontIndex, fontHeight, sdfPadding);
	}
	bool LoadLoaded(const char* filepath) {
		size_t fileSize;
		if (std::unique_ptr bytes = LoadFile(filepath, fileSize))
			return LoadLoaded_Internal(bytes.get(), fileSize, filepath);
		return false;
	}
	bool LoadLoaded(const uint8_t* fileBytes, size_t fileSize) {
		return LoadLoaded_Internal(fileBytes, fileSize);
	}
	void StoreGlyphRenderingInfosTo(std::vector<glyphRenderingInfo>& glyphRenderingInfos) {
		glyphRenderingInfos = std::move(this->glyphRenderingInfos);
	}
	void Reset() {
		this->~ttfLoader();
		new(this) ttfLoader;
	}
	/* Static Function */
	static auto GetFontNames(const char* filepath, std::unique_ptr<uint8_t[]>& fileBytes) {
		size_t fileSize;
		if (fileBytes = LoadFile(filepath, fileSize))
			return GetFontNames_Internal(fileBytes.get(), filepath);
		return std::vector<nameParser::result_t>{};
	}
	static auto GetFontNames(const uint8_t* fileBytes) {
		return GetFontNames_Internal(fileBytes);
	}
	static void MaxImageLayerCount(uint32_t maxImageLayerCount) { ttfLoader::maxImageLayerCount = maxImageLayerCount; }
	static void MaxImageLayerWidth(uint32_t maxImageLayerWidth) { ttfLoader::maxImageLayerWidth = maxImageLayerWidth; }
};

class ttfLoader_imageless {
	friend class ttfLoader;
public:
	using indexMapping = cmapParser::indexMapping;
	using glyphRenderingInfo = ttfLoader::glyphRenderingInfo;
	using kerning = ttfLoader::kerning;
protected:
	uint32_t sfntVersion = 0;
	int32_t fontHeight = 0;
	int32_t fontAscent = 0;
	int32_t fontLineSpacing = 0;
	uint16_t glyphCount = 0;
	int16_t maxGlyphTop = 0;
	bool fontNameEncodingIsU16 = false;
	decltype(nameParser::result_t::fullName) fontName;
	std::vector<indexMapping> glyphIndexMappings;
	std::vector<glyphRenderingInfo> glyphRenderingInfos;
	std::vector<kerning> kernings;
	glyfParser glyfData;
	/* Non-const Function */
	bool LoadTtf_Internal(const uint8_t* fileBytes, uint32_t fontOffset, const char* filepath = nullptr) {
		struct _ : ttfLoader { using ttfLoader::LoadTtf_Internal; };
		return _::LoadTtf_Internal(*this, fileBytes, fontOffset, 0, 0, filepath);
	}
	bool LoadTtc_Internal(const uint8_t* fileBytes, uint32_t fontIndex, const char* filepath = nullptr) {
		struct _ : ttfLoader { using ttfLoader::LoadTtc_Internal; };
		return _::LoadTtc_Internal(*this, fileBytes, fontIndex, 0, 0, filepath);
	}
public:
	ttfLoader_imageless() = default;
	ttfLoader_imageless(const char* filepath_ttf) {
		LoadTtf(filepath_ttf);
	}
	ttfLoader_imageless(const uint8_t* fileBytes_ttf) {
		LoadTtf(fileBytes_ttf);
	}
	ttfLoader_imageless(ttfLoader_imageless&&) = default;
	/* Getter */
	uint32_t SfntVersion() const { return sfntVersion; }
	int32_t FontHeight() const { return fontHeight; }
	int32_t FontAscent() const { return fontAscent; }
	int32_t FontLineSpacing() const { return fontLineSpacing; }
	uint16_t GlyphCount() const { return glyphCount; }
	int16_t MaxGlyphTop() const { return maxGlyphTop; }
	bool FontNameEncodingIsU16() const { return fontNameEncodingIsU16; }
	const auto& FontName() const { return fontName; }
	const auto& GlyphIndexMappings() const { return glyphIndexMappings; }
	const auto& GlyphRenderingInfos() const { return glyphRenderingInfos; }
	const auto& Kernings() const { return kernings; }
	const glyfParser& GlyfData() const { return glyfData; }
	/* Const Function */
	void StoreGlyphIndexMappingsTo(std::map<uint32_t, uint16_t>& glyphIndexMappings) const {
		for (auto& i : this->glyphIndexMappings)
			glyphIndexMappings.emplace(i.codePoint, i.glyphIndex);
	}
	void StoreGlyphIndexMappingsTo(std::unordered_map<uint32_t, uint16_t>& glyphIndexMappings) const {
		for (auto& i : this->glyphIndexMappings)
			glyphIndexMappings.emplace(i.codePoint, i.glyphIndex);
	}
	void StoreGlyphRenderingInfosTo(std::vector<glyphRenderingInfo>& glyphRenderingInfos) const {
		glyphRenderingInfos = this->glyphRenderingInfos;
	}
	void StoreKerningsTo(std::map<uint16_t, std::map<uint16_t, float>>& kernings) const {
		for (auto& i : this->kernings)
			kernings.try_emplace(i.glyph0).first->second.emplace(i.glyph1, i.advance);
	}
	void StoreKerningsTo(std::unordered_map<uint16_t, std::unordered_map<uint16_t, float>>& kernings) const {
		for (auto& i : this->kernings)
			kernings.try_emplace(i.glyph0).first->second.emplace(i.glyph1, i.advance);
	}
	void StoreGlyfDataTo(glyfParser& glyfData) const {
		glyfData = this->glyfData;
	}
	/* Non-const Function */
	bool LoadTtf(const char* filepath) {
		size_t fileSize;
		if (std::unique_ptr bytes = LoadFile(filepath, fileSize))
			return LoadTtf_Internal(bytes.get(), -1, filepath);
		return false;
	}
	bool LoadTtf(const uint8_t* fileBytes) {
		return LoadTtf_Internal(fileBytes, -1);
	}
	bool LoadTtc(const char* filepath, uint32_t fontIndex) {
		size_t fileSize;
		if (std::unique_ptr bytes = LoadFile(filepath, fileSize))
			return LoadTtc_Internal(bytes.get(), fontIndex, filepath);
		return false;
	}
	bool LoadTtc(const uint8_t* fileBytes, uint32_t fontIndex) {
		return LoadTtc_Internal(fileBytes, fontIndex);
	}
	void StoreGlyphRenderingInfosTo(std::vector<glyphRenderingInfo>& glyphRenderingInfos) {
		glyphRenderingInfos = std::move(this->glyphRenderingInfos);
	}
	void StoreGlyfDataTo(glyfParser& glyfData) {
		glyfData = std::move(this->glyfData);
	}
	void Reset() {
		this->~ttfLoader_imageless();
		new(this) ttfLoader_imageless;
	}
	/* Static Function */
	static void RasterizeGlyph(const glyfParser& glyfData, uint32_t glyphIndex,
		float scale, float fontHeight, float subpixelPositionX, float subpixelPositionY, // 0 <= subpixelPositionX < 1; 0 <= subpixelPositionY < 1
		std::vector<glyfParser::pointF32>& points, std::vector<rasterizer::point>& tessellatedPoints, rasterizer& rasterizer,
		std::vector<uint8_t>& pixels, uint16_t& imageWidth, uint16_t& imageHeight) {
		static constexpr float squarePrecision = 0.35f * 0.35f;
		struct _ : ttfLoader {
			using ttfLoader::TessellateSimpleGlyphContour;
			using ttfLoader::TessellateCompoundGlyphContour;
		};
		auto AddOffset = [](std::vector<glyfParser::pointF32>& points, float offsetX, float offsetY) {
			for (auto& i : points)
				i.x += offsetX,
				i.y += offsetY;
		};
		points.clear();
		tessellatedPoints.clear();
		const glyfParser::glyphRecord& glyphRecord = glyfData.Result().glyphRecords[glyphIndex];
		const glyfParser::pointF32* pFirstPoint;
		float scaledXMin = glyphRecord.boundingBox.xMin * scale;
		float scaledYMax = glyphRecord.boundingBox.yMax * scale;
		float subpixelOffsetX = subpixelPositionX - (scaledXMin - std::floor(scaledXMin));
		float subpixelOffsetY = subpixelPositionY - (scaledYMax - std::floor(scaledYMax));
		if (glyphRecord.indexIntoSimpleGlyphs != UINT16_MAX)
			glyfData.GetSimpleGlyphPoints(glyphRecord.indexIntoSimpleGlyphs, points, scale),
			AddOffset(points, subpixelOffsetX, subpixelOffsetY),
			_::TessellateSimpleGlyphContour(glyfData.Result(), glyphRecord.indexIntoSimpleGlyphs, pFirstPoint = points.data(), squarePrecision, tessellatedPoints);
		else
			glyfData.GetCompoundGlyphPoints(glyphRecord.indexIntoCompoundGlyphs, /*sfntVersion,*/ points, scale),
			AddOffset(points, subpixelOffsetX, subpixelOffsetY),
			_::TessellateCompoundGlyphContour(glyfData.Result(), glyphRecord.indexIntoCompoundGlyphs, pFirstPoint = points.data(), squarePrecision, tessellatedPoints);
		rasterizer::bounds bounds = {
			int16_t(std::floor(scaledXMin)), // Same result with or without '+ subpixelOffsetX'
			int16_t(std::floor(glyphRecord.boundingBox.yMin * scale + subpixelOffsetY)),
			int16_t(std::ceil(glyphRecord.boundingBox.xMax * scale + subpixelOffsetX)),
			int16_t(std::ceil(scaledYMax + subpixelOffsetY))
		};
		imageWidth = bounds.right - bounds.left;
		imageHeight = bounds.top - bounds.bottom;
		pixels.clear();                          // Zeroize
		pixels.resize(imageWidth * imageHeight); //
		rasterizer.Rasterize(tessellatedPoints, bounds, pixels.data(), imageWidth);
	}
};
CHIME_TRUETYPE_END
// Chime_TextPrinter
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
#include <optional>
#include "Chime_TrueType/Chime_TrueType.h"

#pragma region Macro
#ifndef API_SPECIFIC
#define API_SPECIFIC
#endif
#ifndef CHIME_NAMESPACE
#define CHIME_NAMESPACE chime
#endif
#ifndef CHIME_NAMESPACE_BEGIN
#define CHIME_NAMESPACE_BEGIN namespace CHIME_NAMESPACE {
#endif
#ifndef CHIME_NAMESPACE_END
#define CHIME_NAMESPACE_END   }
#endif
#pragma endregion

#pragma region Type.h
#ifndef CHIME_TYPEHEADER
CHIME_NAMESPACE_BEGIN
#ifndef GLM_SETUP_INCLUDED
struct vec2 {
	float x, y;
};
struct uvec2 {
	uint32_t x, y;
};
#else
using vec2 = glm::vec2;
using uvec2 = glm::uvec2;
#endif
#if __cpp_lib_constexpr_memory >= 202202L
template<typename T>
struct default_delete {
	constexpr default_delete() noexcept = default;
	template<typename U>
	default_delete(const default_delete<U>&) noexcept requires(std::convertible_to<U*, T*>) {}
	void operator()(T* ptr) const noexcept { delete ptr; }
};
template<typename T>
using pImpl = std::unique_ptr<T, default_delete<T>>;
#else
template<typename T>
using pImpl = std::unique_ptr<T, std::default_delete<T>>;
#endif
CHIME_NAMESPACE_END
#endif
#pragma endregion

CHIME_NAMESPACE_BEGIN
API_SPECIFIC class apiData_font;
API_SPECIFIC class apiData_textPrinter;

API_SPECIFIC void InitializeText();

#ifdef CHIME_TRUETYPE_NAMESPACE
using namespace CHIME_TRUETYPE_NAMESPACE;
#endif

class font {
protected:
	pImpl<apiData_font> pApiData;
	std::unique_ptr<glyfParser> pGlyfData; // Written by font_textureless
	float fontHeight = 0;
	float fontAscent = 0;
	float fontLineSpacing = 0;
	float pixelDistanceScale = 0;
	uint16_t glyphCount = 0;
	uint16_t imageLayerCount = 0;
	uint16_t imageLayerWidth = 0;
	uint16_t imageLayerHeight = 0;
	int16_t imageAscent = 0;
	uint8_t imagePadding = 0;
	bool fontNameEncodingIsU16 = false;
	decltype(nameParser::result_t::fullName) fontName;
	std::unordered_map<uint32_t, uint16_t> glyphIndexMappings;
	std::vector<ttfLoader::glyphRenderingInfo> glyphRenderingInfos;
	std::unordered_map<uint16_t, std::unordered_map<uint16_t, float>> kernings;
	/* Constructor */
	font() = default;
	/* Non-const Function */
	API_SPECIFIC void CreateApiData(const uint8_t* pImageData, bool generateMipmap);
	void LoadData(auto& loader, bool generateMipmap) {
		if (!loader.PImageData())
			return;
		fontHeight = loader.FontHeight();
		fontAscent = loader.FontAscent();
		fontLineSpacing = loader.FontLineSpacing();
		pixelDistanceScale = loader.PixelDistanceScale();
		glyphCount = loader.GlyphCount();
		imageLayerCount = loader.ImageLayerCount();
		imageLayerWidth = loader.ImageLayerWidth();
		imageLayerHeight = loader.ImageLayerHeight();
		imageAscent = loader.ImageAscent();
		imagePadding = loader.ImagePadding();
		fontNameEncodingIsU16 = loader.FontNameEncodingIsU16();
		fontName = loader.FontName();
		loader.StoreGlyphIndexMappingsTo(glyphIndexMappings);
		loader.StoreGlyphRenderingInfosTo(glyphRenderingInfos);
		loader.StoreKerningsTo(kernings);
		CreateApiData(loader.PImageData(), generateMipmap);
	}
public:
	font(const char* filepath_ttf, float fontHeight, int8_t sdfPadding = -1, bool generateMipmap = true) {
		ttfLoader loader(filepath_ttf, fontHeight, sdfPadding);
		LoadData(loader, generateMipmap);
	}
	font(const char* filepath_preLoaded, bool generateMipmap = true) {
		ttfLoader loader(filepath_preLoaded);
		LoadData(loader, generateMipmap);
	}
	font(ttfLoader& loader, bool generateMipmap = true) {
		LoadData(loader, generateMipmap);
	}
	font(const ttfLoader& loader, bool generateMipmap = true) {
		LoadData(loader, generateMipmap);
	}
	font(font&&) = default;
	/* Getter */
	float FontHeight() const { return fontHeight; }
	float FontAscent() const { return fontAscent; }
	float FontLineSpacing() const { return fontLineSpacing; }
	float PixelDistanceScale() const { return pixelDistanceScale; }
	uint16_t GlyphCount() const { return glyphCount; }
	uint16_t ImageLayerCount() const { return imageLayerCount; }
	uint16_t ImageLayerWidth() const { return imageLayerWidth; }
	uint16_t ImageLayerHeight() const { return imageLayerHeight; }
	uvec2 ImageLayerSize() const { return { imageLayerWidth, imageLayerHeight }; }
	int16_t ImageAscent() const { return imageAscent; }
	uint8_t ImagePadding() const { return imagePadding; }
	bool FontNameEncodingIsU16() const { return fontNameEncodingIsU16; }
	const auto& FontName() const { return fontName; }
	uint16_t GlyphIndex(uint32_t character) const {
		// Character codes that do not correspond to any glyph in the font should be mapped to glyph index 0.
		auto mapping = glyphIndexMappings.find(character);
		return mapping == glyphIndexMappings.end() ? 0 : mapping->second;
	}
	ttfLoader::glyphRenderingInfo GlyphRenderingInfo(uint16_t glyphIndex) const {
		return glyphIndex < glyphCount ? glyphRenderingInfos[glyphIndex] : ttfLoader::glyphRenderingInfo{};
	}
	float Kerning(uint16_t glyphIndex_current, uint16_t glyphIndex_next) const {
		if (auto i = kernings.find(glyphIndex_current); i != kernings.end())
			if (auto j = i->second.find(glyphIndex_next); j != i->second.end())
				return j->second;
		return 0;
	}
	/* Const Function */
	const bool IsTextureless() const { return pGlyfData.get(); }
	const apiData_font* operator->() const { return pApiData.get(); }
};
class font_textureless : public font {
	using font::ImageLayerCount;
	using font::ImageLayerWidth;
	using font::ImageLayerHeight;
	using font::ImageLayerSize;
	using font::ImageAscent;
	using font::ImagePadding;
protected:
	/* Constructor */
	font_textureless() = default;
	/* Non-const Function */
	void LoadData(auto& loader) {
		pGlyfData = std::make_unique<glyfParser>();
		fontHeight = float(loader.FontHeight());
		fontAscent = float(loader.FontAscent());
		fontLineSpacing = float(loader.FontLineSpacing());
		glyphCount = loader.GlyphCount();
		imageAscent = loader.MaxGlyphTop();
		fontNameEncodingIsU16 = loader.FontNameEncodingIsU16();
		fontName = loader.FontName();
		loader.StoreGlyphIndexMappingsTo(glyphIndexMappings);
		loader.StoreGlyphRenderingInfosTo(glyphRenderingInfos);
		loader.StoreKerningsTo(kernings);
		loader.StoreGlyfDataTo(*pGlyfData);
	}
public:
	font_textureless(const char* filepath_ttf) {
		ttfLoader_imageless loader(filepath_ttf);
		LoadData(loader);
	}
	font_textureless(ttfLoader_imageless& loader) {
		LoadData(loader);
	}
	font_textureless(const ttfLoader_imageless& loader) {
		LoadData(loader);
	}
	font_textureless(font_textureless&&) = default;
	/* Getter */
	int16_t MaxGlyphTop() const { return imageAscent; }
	const glyfParser& GlyfData() const { return *pGlyfData; }
};

class textPrinter {
public:
	using fnNextCharacter_t = std::optional<uint32_t>(*)(const void* text, uint32_t textLength, uint8_t characterStride, uint32_t& currentIndex);
	using fnPerformBlending_t = void(*)(uint8_t* pPixel, uint8_t monochromeValue, uint8_t canvasChannelCount, const uint8_t color_rgba[4]);
	class fnPrint {
	protected:
		textPrinter* pTextPrinter = nullptr;
		const font* pFont = nullptr;
		uint32_t vertexCount = 0;
		uint32_t firstVertexIndex = 0;
	public:
		fnPrint() = default;
		fnPrint(textPrinter& printer, const font& font) :
			pTextPrinter(&printer), pFont(&font), vertexCount(printer.vertexCount_toDraw), firstVertexIndex(printer.vertices.size() - printer.vertexCount_toDraw) {
			printer.vertexCount_toDraw = 0;
			if (vertexCount)
				printer.deferredPrintingCount++;
		}
		fnPrint(fnPrint&& other) noexcept :
			pTextPrinter(other.pTextPrinter), pFont(other.pFont), vertexCount(other.vertexCount), firstVertexIndex(other.firstVertexIndex) {
			other.vertexCount = 0;
		}
		~fnPrint() {
			if (vertexCount) {
				pTextPrinter->deferredPrintingCount--;
				if (pTextPrinter->deferredPrintingCount == 0 &&
					!pTextPrinter->updateApiData)
					pTextPrinter->vertices.clear();
			}
		}
		fnPrint& operator=(fnPrint&& other) noexcept {
			this->~fnPrint();
			new(this) fnPrint(std::move(other));
			return *this;
		}
		void operator()(vec2 view) const {
			pTextPrinter->Print_Internal(view, *pFont, vertexCount, firstVertexIndex);
		}
	};
	struct characterVertex {
		vec2 position;
		float scale;
		uint32_t color; // RGBA8
		uint16_t glyphIndex;
		uint16_t layerIndex;
		uint16_t offsetU;
		uint16_t sizeU;
	};
protected:
	pImpl<apiData_textPrinter> pApiData;
	float fontHeight = 0;
	float lineSpacing = 0;
	float extraCharacterSpacing = 0;
	float maxLineWidth = 0;
	uint32_t color = 0xffffffff; // Straight alpha
	bool roundVertexPosition = false;
	bool printLineFeed = false;
	std::vector<characterVertex> vertices;
	uint32_t vertexCount_toDraw = 0;
	uint16_t deferredPrintingCount = 0;
	uint16_t previousGlyphIndex = UINT16_MAX;
	float previousScale = 0;
	vec2 currentPosition = {};
	vec2 printAreaSize = {};
	float printAreaTop = 0;
	uint8_t* pCanvas = nullptr;
	uvec2 canvasSize = {};
	uint8_t canvasPixelStride = 0;
	bool updateApiData = false;
	fnNextCharacter_t fnNextCharacter = nullptr;
	fnPerformBlending_t fnPerformBlending = nullptr;
	/* Constructor */
	textPrinter() = default;
	/* Const Function */
	API_SPECIFIC void Draw(vec2 view, const font& font, uint32_t vertexCount, uint32_t firstVertexIndex) const;
	API_SPECIFIC void UpdateApiData_Internal() const;
	/* Non-const Function */
	API_SPECIFIC void CreateApiData(uint32_t capacity); // No API call if capacity is 0
	template<typename T>
	void GenerateVertices(std::basic_string_view<T> text, const font& font) {
		if (text.empty())
			return;
		// Calculate current scale
		float scale = fontHeight / font.FontHeight();
		// Calculate scaled metrics
		float fontAscent = font.FontAscent() * scale;
		float imageAscent = font.ImageAscent() * scale;
		float imagePadding = font.ImagePadding() * scale;
		// Calculate print area top and bottom position
		float lineTop = currentPosition.y - fontAscent;
		float lineBottom = lineTop + fontHeight;
		this->printAreaTop = std::min(lineTop, std::min(lineBottom, printAreaTop));
		float printAreaBottom = std::max(lineTop, std::max(lineBottom, printAreaTop + printAreaSize.y));

		vec2 position = { currentPosition.x - imagePadding, currentPosition.y - imageAscent };
		characterVertex vertex = {
			.scale = scale
		};
		size_t previousVertexCount = vertices.size();
		uint32_t textLength = text.length();
		uint32_t characterCount = 0;
		for (uint32_t i = 0; i < textLength; i++, characterCount++) {
			uint32_t character;
			if (fnNextCharacter)
				if (auto optional = fnNextCharacter(text.data(), textLength, sizeof(T), i))
					character = *optional;
				else
					break;
			else
				character = text[i];
			if (character == '\n' &&
				!printLineFeed) {
				vertex.glyphIndex = UINT16_MAX;
				position.x = -imagePadding;
				position.y += lineSpacing;
				continue;
			}
			if (characterCount)
				previousGlyphIndex = vertex.glyphIndex,
				position.x += font.Kerning(previousGlyphIndex, vertex.glyphIndex = font.GlyphIndex(character)) * scale;
			else
				position.x += font.Kerning(previousGlyphIndex, vertex.glyphIndex = font.GlyphIndex(character)) * previousScale;
			auto [advanceWidth, _0, _1, leftmostPixelBearing, layerIndex, offsetU, sizeU] = font.GlyphRenderingInfo(vertex.glyphIndex);
			advanceWidth *= scale;
			leftmostPixelBearing *= scale;
			if (maxLineWidth < position.x + imagePadding + advanceWidth) // maxLineWidth < lineRight
				if (position.x != -imagePadding)
					position.x = -imagePadding,
					position.y += lineSpacing;
			vertex.position.x = position.x + leftmostPixelBearing;
			vertex.position.y = position.y;
			position.x += std::max(advanceWidth + extraCharacterSpacing, 0.f);
			if (roundVertexPosition)
				vertex.position.x = std::round(vertex.position.x),
				vertex.position.y = std::round(vertex.position.y);
			vertex.color = color;
			vertex.layerIndex = layerIndex;
			vertex.offsetU = offsetU;
			vertex.sizeU = sizeU;
			vertices.push_back(vertex);
			float lineRight = vertex.position.x + imagePadding - leftmostPixelBearing + advanceWidth;
			if (printAreaSize.x < lineRight)
				printAreaSize.x = lineRight;
		}
		currentPosition.x = position.x + imagePadding;
		currentPosition.y = position.y + imageAscent;

		// Calculate print area height
		lineTop = currentPosition.y - fontAscent;
		lineBottom = lineTop + fontHeight;
		printAreaTop = std::min(lineTop, std::min(lineBottom, printAreaTop));
		printAreaBottom = std::max(lineTop, std::max(lineBottom, printAreaBottom));
		printAreaSize.y = printAreaBottom - printAreaTop;

		if (characterCount)
			previousGlyphIndex = vertex.glyphIndex;

		if (size_t newVertexCount = vertices.size() - previousVertexCount)
			previousScale = scale,
			vertexCount_toDraw += newVertexCount;
	}
	template<typename T>
	float GenerateVertices_FirstLine(std::basic_string_view<T>& text, const font& font, float& strictLineLeft, float& strictLineRight) {
		if (text.empty())
			return 0;
		// Calculate current scale
		float scale = fontHeight / font.FontHeight();
		// Calculate scaled metrics
		float fontAscent = font.FontAscent() * scale;
		float imageAscent = font.ImageAscent() * scale;
		float imagePadding = font.ImagePadding() * scale;
		// Calculate print area top and bottom position
		float lineTop = currentPosition.y - fontAscent;
		float lineBottom = lineTop + fontHeight;
		this->printAreaTop = std::min(lineTop, std::min(lineBottom, printAreaTop));
		float printAreaBottom = std::max(lineTop, std::max(lineBottom, printAreaTop + printAreaSize.y));

		float lineRight = 0;
		strictLineLeft = maxLineWidth;
		strictLineRight = 0;

		vec2 position = { -imagePadding, currentPosition.y - imageAscent };
		characterVertex vertex = {
			.position = { 0, roundVertexPosition ? std::round(position.y) : position.y },
			.scale = scale
		};
		size_t previousVertexCount = vertices.size();
		size_t textLength = text.length();
		uint32_t i = 0, characterCount = 0;
		for (; i < textLength; i++, characterCount++) {
			uint32_t character;
			if (fnNextCharacter)
				if (auto optional = fnNextCharacter(text.data(), textLength, sizeof(T), i))
					character = *optional;
				else {
					i++;
					break;
				}
			else
				character = text[i];
			if (character == '\n' &&
				!printLineFeed) {
				i++;
				characterCount++;
				break;
			}
			if (characterCount)
				previousGlyphIndex = vertex.glyphIndex,
				position.x += font.Kerning(previousGlyphIndex, vertex.glyphIndex = font.GlyphIndex(character)) * scale;
			else
				vertex.glyphIndex = font.GlyphIndex(character);
			auto [advanceWidth, width, leftSideBearing, leftmostPixelBearing, layerIndex, offsetU, sizeU] = font.GlyphRenderingInfo(vertex.glyphIndex);
			advanceWidth *= scale;
			leftmostPixelBearing *= scale;
			if (maxLineWidth < position.x + imagePadding + advanceWidth) // maxLineWidth < lineRight
				if (position.x != -imagePadding)
					break;
			vertex.position.x = position.x + leftmostPixelBearing;
			position.x += std::max(advanceWidth + extraCharacterSpacing, 0.f);
			if (roundVertexPosition)
				vertex.position.x = std::round(vertex.position.x);
			vertex.color = color;
			vertex.layerIndex = layerIndex;
			vertex.offsetU = offsetU;
			vertex.sizeU = sizeU;
			vertices.push_back(vertex);
			float positionX = vertex.position.x + imagePadding - leftmostPixelBearing;
			float _lineRight = positionX + advanceWidth;
			if (lineRight < _lineRight)
				lineRight = _lineRight;
			float leftEdgePosition = positionX + leftSideBearing * scale;
			float rightEdgePosition = leftEdgePosition + width * scale;
			if (strictLineLeft > leftEdgePosition)
				strictLineLeft = leftEdgePosition;
			if (strictLineRight < rightEdgePosition)
				strictLineRight = rightEdgePosition;
		}
		currentPosition.x = 0;

		// Calculate print area height
		lineTop = currentPosition.y - fontAscent;
		lineBottom = lineTop + fontHeight;
		printAreaTop = std::min(lineTop, std::min(lineBottom, printAreaTop));
		printAreaBottom = std::max(lineTop, std::max(lineBottom, printAreaBottom));
		printAreaSize.y = printAreaBottom - printAreaTop;
		// New line
		if (characterCount)
			currentPosition.y += lineSpacing,
			previousGlyphIndex = UINT16_MAX;

		if (size_t newVertexCount = vertices.size() - previousVertexCount)
			previousScale = scale,
			vertexCount_toDraw += newVertexCount;

		text = text.substr(i, textLength - i);
		return lineRight;
	}
	void Print_Internal(vec2 view, const font& font, uint32_t vertexCount, uint32_t firstVertexIndex) {
		if (vertexCount)
			if (font.operator->()) {
				Draw(view, font, vertexCount, firstVertexIndex);
				updateApiData = true;
			}
			else if (font.IsTextureless()) {
				if (!pCanvas)
					return;
				std::vector<glyfParser::pointF32> points;
				std::vector<rasterizer::point> tessellatedPoints;
				rasterizer rasterizer;
				std::vector<uint8_t> pixels;
				uint16_t imageWidth, imageHeight;
				uint16_t maxGlyphTop = static_cast<const font_textureless&>(font).MaxGlyphTop();
				const glyfParser& glyfData = static_cast<const font_textureless&>(font).GlyfData();
				const characterVertex* pVertex = &vertices[firstVertexIndex];
				for (size_t i = 0; i < vertexCount; i++, pVertex++) {
					auto& [xMin, yMin, xMax, yMax] = glyfData.Result().glyphRecords[pVertex->glyphIndex].boundingBox;
					float left = view.x + pVertex->position.x;
					float right = left + (xMax - xMin) * pVertex->scale;
					float top = view.y + pVertex->position.y + (maxGlyphTop - yMax) * pVertex->scale;
					float bottom = top + (yMax - yMin) * pVertex->scale;
					if (left >= canvasSize.x)
						continue;
					if (right <= 0)
						continue;
					if (top >= canvasSize.y)
						continue;
					if (bottom <= 0)
						continue;
					float subpixelPositionX = left - std::floor(left);
					float subpixelPositionY = -top - std::floor(-top);
					ttfLoader_imageless::RasterizeGlyph(
						glyfData, pVertex->glyphIndex, pVertex->scale, fontHeight, subpixelPositionX, subpixelPositionY,
						points, tessellatedPoints, rasterizer, pixels, imageWidth, imageHeight);
					int32_t _left = int32_t(std::floor(left));
					int32_t _top = int32_t(std::floor(top));
					uint32_t offsetX = std::max(_left, 0);
					uint32_t offsetY = std::max(_top, 0);
					uint32_t copyWidth = std::min(uint32_t(std::ceil(right)), canvasSize.x) - offsetX;
					uint32_t copyHeight = std::min(uint32_t(std::ceil(bottom)), canvasSize.y) - offsetY;
					uint8_t* pImageData_src = &pixels[(offsetX - _left) + (offsetY - _top) * imageWidth];
					uint8_t* pImageData_dst = pCanvas + (offsetX + offsetY * canvasSize.x) * canvasPixelStride;
					if (fnPerformBlending)
						for (size_t j = 0; j < copyHeight; j++, pImageData_src += imageWidth, pImageData_dst += canvasSize.x * canvasPixelStride)
							for (size_t i = 0; i < copyWidth; i++)
								fnPerformBlending(pImageData_dst + i * canvasPixelStride, pImageData_src[i], canvasPixelStride, reinterpret_cast<uint8_t*>(&color));
					else
						if (canvasPixelStride == 1)
							for (size_t j = 0; j < copyHeight; j++, pImageData_src += imageWidth, pImageData_dst += canvasSize.x)
								std::memcpy(pImageData_dst, pImageData_src, copyWidth);
						else
							for (size_t j = 0; j < copyHeight; j++, pImageData_src += imageWidth, pImageData_dst += canvasSize.x * canvasPixelStride)
								for (size_t i = 0; i < copyWidth; i++)
									pImageData_dst[i * canvasPixelStride] = pImageData_src[i];
				}
			}
	}
	/* Static Function */
	static uint32_t EndiannessCast(uint32_t value) {
		if constexpr (std::endian::native == std::endian::little)
			return trueType::GetU32(reinterpret_cast<uint8_t*>(&value));
		return value;
	}
public:
	textPrinter(uint32_t initialCapacity, float fontHeight, float lineSpacing, float extraCharacterSpacing, float maxLineWidth, uint32_t color_rgba = 0xffffffff) :
		fontHeight(fontHeight), lineSpacing(lineSpacing), extraCharacterSpacing(extraCharacterSpacing), maxLineWidth(maxLineWidth), color(EndiannessCast(color_rgba)) {
		if (initialCapacity)
			vertices.reserve(initialCapacity),
			CreateApiData(vertices.capacity());
		else
			CreateApiData(0);
	}
	textPrinter(textPrinter&&) = default;
	/* Getter */
	float FontHeight() const { return fontHeight; }
	float LineSpacing() const { return lineSpacing; }
	float ExtraCharacterSpacing() const { return extraCharacterSpacing; }
	float MaxLineWidth() const { return maxLineWidth; }
	uint32_t Color() const { return EndiannessCast(color); };
	bool RoundVertexPosition() const { return roundVertexPosition; }
	bool PrintLineFeed() const { return printLineFeed; }
	const characterVertex& Vertex(uint32_t index) const { return vertices[index]; }
	uint32_t VertexCount() const { return uint32_t(vertices.size()); }
	vec2 CurrentPosition() const { return currentPosition; }
	vec2 PrintAreaSize() const { return printAreaSize; }
	float PrintAreaTop() const { return printAreaTop; }
	/* Setter */
	void FontHeight(float fontHeight) { this->fontHeight = fontHeight; }
	void LineSpacing(float lineSpacing) { this->lineSpacing = lineSpacing; }
	void ExtraCharacterSpacing(float extraCharacterSpacing) { this->extraCharacterSpacing = extraCharacterSpacing; }
	void MaxLineWidth(float maxLineWidth) { this->maxLineWidth = maxLineWidth; }
	void Color(uint32_t color_rgba) { this->color = EndiannessCast(color_rgba); };
	void RoundVertexPosition(bool roundVertexPosition) { this->roundVertexPosition = roundVertexPosition; }
	void PrintLineFeed(bool printLineFeed) { this->printLineFeed = printLineFeed; }
	void FnNextCharacter(fnNextCharacter_t fnNextCharacter) { this->fnNextCharacter = fnNextCharacter; }
	void FnPerformBlending(fnPerformBlending_t fnPerformBlending) { this->fnPerformBlending = fnPerformBlending; } // For textureless printing
	/* Const Function */
	const apiData_textPrinter* operator->() const { return pApiData.get(); }
	float GetBaseline(float textTop, const font& font) const {
		return textTop + font.FontAscent() * fontHeight / font.FontHeight();
	}
	/* Non-const Function */
	void SetTexturelessPrintingCanvas(uint8_t* pCanvas, uvec2 canvasSize, uint8_t pixelStrideInBytes) {
		this->pCanvas = pCanvas;
		this->canvasSize = canvasSize;
		canvasPixelStride = pixelStrideInBytes;
	}
	void ResetPrintArea() {
		previousGlyphIndex = UINT16_MAX;
		previousScale = 0;
		currentPosition = {};
		printAreaSize = {};
		printAreaTop = 0;
	}
	// Print(...) increases both printAreaSize.x and printAreaSize.y.
	vec2 Print(fnPrint& deferredPrinting, const auto& text, const font& font)
		requires (requires{ std::basic_string(text); }) {
		using T = decltype(std::basic_string(text))::value_type;
		GenerateVertices(std::basic_string_view<T>(text), font);
		deferredPrinting = { *this, font };
		return printAreaSize;
	}
	vec2 Print(vec2 view, const auto& text, const font& font) {
		fnPrint fnPrint;
		Print(fnPrint, text, font);
		fnPrint(view);
		return printAreaSize;
	}
	// Print_HorizontallyCentered(...) and Print_RightAligned(...) only increases printAreaSize.y.
	float Print_HorizontallyCentered(fnPrint& deferredPrinting, const auto& text, const font& font, float& printAreaWidth, bool isStrict = false)
		requires (requires{ std::basic_string(text); }) {
		using T = decltype(std::basic_string(text))::value_type;
		printAreaWidth = 0;
		std::basic_string_view<T> subText = text;
		while (subText.length()) {
			size_t i = vertices.size();
			float strictLineLeft, strictLineRight;
			float lineRight = GenerateVertices_FirstLine(subText, font, strictLineLeft, strictLineRight);
			float offsetX = (isStrict ? strictLineLeft + strictLineRight : lineRight) / 2;
			if (roundVertexPosition)
				offsetX = std::round(offsetX);
			for (size_t vertexCount = vertices.size(); i < vertexCount; i++)
				vertices[i].position.x -= offsetX;
			float lineWidth = isStrict ? strictLineRight - strictLineLeft : lineRight;
			if (printAreaWidth < lineWidth)
				printAreaWidth = lineWidth;
		}
		deferredPrinting = { *this, font };
		return printAreaSize.y;
	}
	float Print_HorizontallyCentered(fnPrint& deferredPrinting, const auto& text, const font& font, bool isStrict = false) {
		float printAreaWidth;
		return Print_HorizontallyCentered(deferredPrinting, text, font, printAreaWidth, isStrict);
	}
	float Print_HorizontallyCentered(vec2 view, const auto& text, const font& font, float& printAreaWidth, bool isStrict = false) {
		fnPrint fnPrint;
		Print_HorizontallyCentered(fnPrint, text, font, printAreaWidth, isStrict);
		fnPrint(view);
		return printAreaSize.y;
	}
	float Print_HorizontallyCentered(vec2 view, const auto& text, const font& font, bool isStrict = false) {
		float printAreaWidth;
		return Print_HorizontallyCentered(view, text, font, printAreaWidth, isStrict);
	}
	float Print_RightAligned(fnPrint& deferredPrinting, const auto& text, const font& font, float& printAreaWidth)
		requires (requires{ std::basic_string(text); }) {
		using T = decltype(std::basic_string(text))::value_type;
		printAreaWidth = 0;
		std::basic_string_view<T> subText = text;
		while (subText.length()) {
			size_t i = vertices.size();
			float strictLineLeft, strictLineRight;
			float lineWidth = GenerateVertices_FirstLine(subText, font, strictLineLeft, strictLineRight);
			for (size_t vertexCount = vertices.size(); i < vertexCount; i++)
				vertices[i].position.x -= lineWidth;
			if (printAreaWidth < lineWidth)
				printAreaWidth = lineWidth;
		}
		deferredPrinting = { *this, font };
		return printAreaSize.y;
	}
	float Print_RightAligned(fnPrint& deferredPrinting, const auto& text, const font& font) {
		float printAreaWidth;
		return Print_RightAligned(deferredPrinting, text, font, printAreaWidth);
	}
	float Print_RightAligned(vec2 view, const auto& text, const font& font, float& printAreaWidth) {
		fnPrint fnPrint;
		Print_RightAligned(fnPrint, text, font, printAreaWidth);
		fnPrint(view);
		return printAreaSize.y;
	}
	float Print_RightAligned(vec2 view, const auto& text, const font& font) {
		float printAreaWidth;
		return Print_RightAligned(view, text, font, printAreaWidth);
	}
	// Call UpdateApiData() before submitting API commands, this invalidates all generated vertices.
	void UpdateApiData(bool resetPrintArea = true) {
		if (vertices.size())
			UpdateApiData_Internal(),
			vertices.clear();
		if (resetPrintArea)
			ResetPrintArea();
		updateApiData = false;
	}
};
CHIME_NAMESPACE_END
class glyfParser {
public:
	struct glyphBoundingBox {
		int16_t xMin;
		int16_t yMin;
		int16_t xMax;
		int16_t yMax;
	};
	struct glyphRecord {
		glyphBoundingBox boundingBox;
		uint16_t indexIntoSimpleGlyphs = UINT16_MAX;
		uint16_t indexIntoCompoundGlyphs = UINT16_MAX;
	};
	struct point {
		int16_t x;
		int16_t y;
		bool isOnCurve;
	};
	struct pointF32 {
		float x;
		float y;
		bool isOnCurve;
	};
	struct simpleGlyph {
		uint32_t firstPointIndex;
		uint32_t pointCount;
		std::vector<uint16_t> contourPointCounts;
	};
	struct component {
		uint16_t glyphIndex;
		int16_t offsetX;
		int16_t offsetY;
		bool hasTransformation;
		bool applyTransformationToOffset; // Should be ignored if the font is produced for OS X or iOS
		float scaleX;
		float scale01;
		float scale10;
		float scaleY;
	};
	struct compoundGlyph {
		std::vector<component> components;
	};
	struct result_t {
		std::vector<glyphRecord> glyphRecords;
		std::vector<point> points;
		std::vector<simpleGlyph> simpleGlyphs;
		std::vector<compoundGlyph> compoundGlyphs;
	};
	glyfParser() = default;
	glyfParser(const uint8_t* pTable, const std::vector<uint32_t>& glyphDescriptionLengths, bool boundingBoxesOnly = false) { boundingBoxesOnly ? ParseTable<true>(pTable, glyphDescriptionLengths) : ParseTable(pTable, glyphDescriptionLengths); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
	void GetSimpleGlyphPoints(uint16_t indexIntoSimpleGlyphs, std::vector<pointF32>& points_out, float scale) const {
		if (result.simpleGlyphs[indexIntoSimpleGlyphs].pointCount == 0)
			return;
		GetSimpleGlyphPoints_Internal(result.simpleGlyphs[indexIntoSimpleGlyphs], points_out, scale);
	}
	void GetCompoundGlyphPoints(uint16_t indexIntoCompoundGlyphs, /*uint32_t sfntVersion,*/ std::vector<pointF32>& points_out, float scale) const {
		GetCompoundGlyphPoints_Internal(result.compoundGlyphs[indexIntoCompoundGlyphs], /*sfntVersion,*/ points_out, scale);
	}
protected:
	result_t result;
	template<bool boundingBoxesOnly = false>
	void ParseTable(const uint8_t* pTable, const std::vector<uint32_t>& glyphDescriptionLengths) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/glyf
		struct glyphDescription {
			struct header {
				int16 numberOfContours;
				int16 xMin;
				int16 yMin;
				int16 xMax;
				int16 yMax;
			};
			const uint8_t* pData;
			int16_t NumberOfContours() const { return GetI16(pData + offsetof(header, numberOfContours)); }
			glyphBoundingBox BoundingBox() const {
				return {
					GetI16(pData + offsetof(header, xMin)),
					GetI16(pData + offsetof(header, yMin)),
					GetI16(pData + offsetof(header, xMax)),
					GetI16(pData + offsetof(header, yMax)) };
			}
		};
		result.glyphRecords.reserve(glyphDescriptionLengths.size());
		result.simpleGlyphs.emplace_back();
		for (auto& i : glyphDescriptionLengths)
			if (i == 0)
				result.glyphRecords.emplace_back().indexIntoSimpleGlyphs = 0;
			else {
				glyphDescription glyphDescription(pTable);
				result.glyphRecords.emplace_back(glyphDescription.BoundingBox());
				if constexpr (!boundingBoxesOnly)
					if (glyphDescription.NumberOfContours() > 0)
						result.glyphRecords.back().indexIntoSimpleGlyphs = result.simpleGlyphs.size(),
						result.simpleGlyphs.emplace_back(),
						ParseSimpleGlyphDescription(pTable + sizeof(glyphDescription::header), glyphDescription.NumberOfContours());
					else if (glyphDescription.NumberOfContours() < 0)
						result.glyphRecords.back().indexIntoCompoundGlyphs = result.compoundGlyphs.size(),
						result.compoundGlyphs.emplace_back(),
						ParseCompoundGlyphDescription(pTable + sizeof(glyphDescription::header));
					else
						result.glyphRecords.back().indexIntoSimpleGlyphs = 0;
				pTable += i;
			}
	}
	void ParseSimpleGlyphDescription(const uint8_t* pSimpleGlyphDescription, uint16_t contourCount) {
		enum flag {
			ON_CURVE_POINT = 1 << 0,
			X_SHORT_VECTOR = 1 << 1,
			Y_SHORT_VECTOR = 1 << 2,
			REPEAT = 1 << 3,
			X_IS_SAME = 1 << 4,
			Y_IS_SAME = 1 << 5,
			POSITIVE_X_SHORT_VECTOR = X_IS_SAME,
			POSITIVE_Y_SHORT_VECTOR = Y_IS_SAME,
			OVERLAP_SIMPLE = 1 << 6
		};
		if (contourCount == 0)
			return;
		const uint8_t* endPtsOfContours = pSimpleGlyphDescription;
		const uint8_t* pInstructionLength = endPtsOfContours + sizeof(uint16_t) * contourCount;
		const uint8_t* pData = pInstructionLength + sizeof(uint16_t) + GetU16(pInstructionLength);
		auto& [firstPointIndex, pointCount, contourPointCounts] = result.simpleGlyphs.back();
		// Get point counts of each contour
		contourPointCounts.resize(contourCount);
		contourPointCounts[0] = GetU16(endPtsOfContours) + 1;
		for (size_t i = 1; i < contourCount; i++)
			contourPointCounts[i] = GetU16(endPtsOfContours + sizeof(uint16_t) * i) - GetU16(endPtsOfContours + sizeof(uint16_t) * (i - 1));
		// Get flags
		pointCount = GetU16(pInstructionLength - sizeof(uint16_t)) + 1;
		std::unique_ptr flagBytes = std::make_unique<uint8_t[]>(pointCount);
		for (size_t i = 0; i < pointCount;)
			if (uint8_t flagByte = flagBytes[i++] = GetU8(pData++);
				flagByte & REPEAT) {
				for (size_t j = 0; j < GetU8(pData); j++)
					flagBytes[i++] = flagByte;
				pData++;
			}
		// Get point coordinates
		firstPointIndex = result.points.size();
		result.points.resize(firstPointIndex + pointCount);
		point* points = &result.points[firstPointIndex];
		for (size_t i = 0; i < pointCount; i++)
			if (points[i].isOnCurve = flagBytes[i] & ON_CURVE_POINT;
				flagBytes[i] & X_SHORT_VECTOR)
				points[i].x = points[i - bool(i)].x + GetU8(pData++) * (flagBytes[i] & POSITIVE_X_SHORT_VECTOR ? 1 : -1);
			else
				points[i].x = points[i - bool(i)].x + (flagBytes[i] & X_IS_SAME ? 0 : GetI16(pData)),
				pData += sizeof(int16_t) * !bool(flagBytes[i] & X_IS_SAME);
		for (size_t i = 0; i < pointCount; i++)
			if (flagBytes[i] & Y_SHORT_VECTOR)
				points[i].y = points[i - bool(i)].y + GetU8(pData++) * (flagBytes[i] & POSITIVE_Y_SHORT_VECTOR ? 1 : -1);
			else
				points[i].y = points[i - bool(i)].y + (flagBytes[i] & Y_IS_SAME ? 0 : GetI16(pData)),
				pData += sizeof(int16_t) * !bool(flagBytes[i] & Y_IS_SAME);
	}
	void ParseCompoundGlyphDescription(const uint8_t* componentDescriptions) {
		enum flag {
			ARG_1_AND_2_ARE_WORDS = 1 << 0,
			ARGS_ARE_XY_VALUES = 1 << 1,
			ROUND_XY_TO_GRID = 1 << 2,
			WE_HAVE_A_SCALE = 1 << 3,
			MORE_COMPONENTS = 1 << 5,
			WE_HAVE_AN_X_AND_Y_SCALE = 1 << 6,
			WE_HAVE_A_TWO_BY_TWO = 1 << 7,
			WE_HAVE_INSTRUCTIONS = 1 << 8,
			USE_MY_METRICS = 1 << 9,
			OVERLAP_COMPOUND = 1 << 10,
			SCALED_COMPONENT_OFFSET = 1 << 11,
			UNSCALED_COMPONENT_OFFSET = 1 << 12,
		};
		const uint8_t* pData = componentDescriptions;
		uint16_t flags;
		do {
			component& component = result.compoundGlyphs.back().components.emplace_back();
			flags = GetU16(pData);
			component.glyphIndex = GetU16(pData += sizeof(int16_t));
			pData += sizeof(int16_t);
			if (flags & ARGS_ARE_XY_VALUES)
				if (flags & ARG_1_AND_2_ARE_WORDS)
					component.offsetX = GetI16(pData),
					component.offsetY = GetI16(pData += sizeof(int16_t)),
					pData += sizeof(int16_t);
				else
					component.offsetX = GetI8(pData),
					component.offsetY = GetI8(pData += sizeof(int8_t)),
					pData += sizeof(int8_t);
			else {
				CHIME_TRUETYPE_PRINT_ERROR("Component offset is specified by matching points, which is not supported!");
				result.compoundGlyphs.back() = {};
				return;
			}
			if (component.hasTransformation = flags & (WE_HAVE_A_SCALE | WE_HAVE_AN_X_AND_Y_SCALE | WE_HAVE_A_TWO_BY_TWO)) {
				if (flags & WE_HAVE_A_SCALE)
					component.scaleX = component.scaleY = GetF2Dot14(pData);
				else if (flags & WE_HAVE_AN_X_AND_Y_SCALE)
					component.scaleX = GetF2Dot14(pData),
					component.scaleY = GetF2Dot14(pData += sizeof(int16_t));
				else
					component.scaleX = GetF2Dot14(pData),
					component.scale01 = GetF2Dot14(pData += sizeof(int16_t)),
					component.scale10 = GetF2Dot14(pData += sizeof(int16_t)),
					component.scaleY = GetF2Dot14(pData += sizeof(int16_t));
				pData += sizeof(int16_t);
				// If the SCALED_COMPONENT_OFFSET flag is set, then the x and y offset values are deemed to be in the component glyph’s coordinate system,
				// and the scale transformation is applied to both values.
				component.applyTransformationToOffset = flags & SCALED_COMPONENT_OFFSET;
			}
		} while (flags & MORE_COMPONENTS);
	}
	void GetSimpleGlyphPoints_Internal(const simpleGlyph& glyph, std::vector<pointF32>& points_out, auto... scale) const {
		float _scale;
		if constexpr (sizeof...(scale))
			_scale = std::get<0>(std::tie(scale...));
		points_out.resize(points_out.size() + glyph.pointCount);
		pointF32* pBack_out = &points_out.back();
		pointF32* pPoint_out = pBack_out - glyph.pointCount + 1;
		const point* pPoint = &result.points[glyph.firstPointIndex];
		for (; pPoint_out <= pBack_out; pPoint++, pPoint_out++) {
			if constexpr (sizeof...(scale))
				pPoint_out->x = _scale * pPoint->x,
				pPoint_out->y = _scale * pPoint->y;
			else
				pPoint_out->x = pPoint->x,
				pPoint_out->y = pPoint->y;
			pPoint_out->isOnCurve = pPoint->isOnCurve;
		}
	}
	void GetCompoundGlyphPoints_Internal(const compoundGlyph& glyph, /*uint32_t sfntVersion,*/ std::vector<pointF32>& points_out, auto... scale) const {
		float _scale;
		if constexpr (sizeof...(scale))
			_scale = std::get<0>(std::tie(scale...));
		for (auto& i : glyph.components) {
			size_t firstPointIndex = points_out.size();
			const glyphRecord& glyphRecord = result.glyphRecords[i.glyphIndex];
			if (glyphRecord.indexIntoSimpleGlyphs != UINT16_MAX)
				GetSimpleGlyphPoints_Internal(result.simpleGlyphs[glyphRecord.indexIntoSimpleGlyphs], points_out);
			else
				GetCompoundGlyphPoints_Internal(result.compoundGlyphs[glyphRecord.indexIntoCompoundGlyphs], /*sfntVersion,*/ points_out);
			pointF32* pPoint = &points_out[firstPointIndex];
			pointF32* pBack = &points_out.back();
			if (!i.hasTransformation)
				for (; pPoint <= pBack; pPoint++)
					if constexpr (sizeof...(scale))
						pPoint->x = _scale * (pPoint->x + i.offsetX),
						pPoint->y = _scale * (pPoint->y + i.offsetY);
					else
						pPoint->x += i.offsetX,
						pPoint->y += i.offsetY;
			else
				/*if (sfntVersion == 'true') {
					// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6glyf.html#COMPOUNDGLYPHS
					float m = std::max(std::abs(i.scaleX), std::abs(i.scale01));
					float n = std::max(std::abs(i.scale10), std::abs(i.scaleY));
					if (std::abs(std::abs(i.scaleX) - std::abs(i.scale10)) <= 33.f / 65536)
						m *= 2;
					if (std::abs(std::abs(i.scale01) - std::abs(i.scaleY)) <= 33.f / 65536)
						n *= 2;
					for (; pPoint <= pBack; pPoint++) {
						float x = pPoint->x;
						if constexpr (sizeof...(scale))
							pPoint->x = _scale * (i.scaleX * x + i.scale10 * pPoint->y + m * i.offsetX),
							pPoint->y = _scale * (i.scale01 * x + i.scaleY * pPoint->y + n * i.offsetY);
						else
							pPoint->x = i.scaleX * x + i.scale10 * pPoint->y + m * i.offsetX,
							pPoint->y = i.scale01 * x + i.scaleY * pPoint->y + n * i.offsetY;
					}
				}
				else*/ {
					float offsetX = i.offsetX;
					float offsetY = i.offsetY;
					if (i.applyTransformationToOffset)
						offsetX = i.scaleX * i.offsetX + i.scale10 * i.offsetY,
						offsetY = i.scale01 * i.offsetX + i.scaleY * i.offsetY;
					for (; pPoint <= pBack; pPoint++) {
						float x = pPoint->x;
						if constexpr (sizeof...(scale))
							pPoint->x = _scale * (i.scaleX * x + i.scale10 * pPoint->y + offsetX),
							pPoint->y = _scale * (i.scale01 * x + i.scaleY * pPoint->y + offsetY);
						else
							pPoint->x = i.scaleX * x + i.scale10 * pPoint->y + offsetX,
							pPoint->y = i.scale01 * x + i.scaleY * pPoint->y + offsetY;
					}
				}
		}
	}
};
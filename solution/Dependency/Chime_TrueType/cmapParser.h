class cmapParser {
public:
	struct indexMapping {
		uint32_t codePoint;
		uint16_t glyphIndex;
	};
	struct result_t {
		std::vector<indexMapping> indexMappings;
	};
	cmapParser() = default;
	cmapParser(const uint8_t* pTable) { ParseTable(pTable); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseTable(const uint8_t* pTable) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/cmap
		struct header {
			uint16 version;
			uint16 numTables;
		};
		struct encodingRecord {
			uint16 platformID;
			uint16 encodingID;
			uint32 subtableOffset;
		};
		// Find subtable
		uint16_t format = 0;
		const uint8_t* pSubtable = nullptr;
		const uint8_t* pEncodingRecord = pTable + sizeof(header);
		uint16_t encodingRecordCount = GetU16(pTable + offsetof(header, numTables));
		for (size_t i = 0; i < encodingRecordCount; i++, pEncodingRecord += sizeof(encodingRecord)) {
			const uint8_t* _pSubtable = nullptr;
			switch (GetU16(pEncodingRecord + offsetof(encodingRecord, platformID))) {
			case 0: // Unicode
				_pSubtable = pTable + GetU32(pEncodingRecord + offsetof(encodingRecord, subtableOffset));
				CHIME_TRUETYPE_PRINT_EXTRA_INFO("Cmap subtable format (Platform ID: Unicode): {}", GetU16(_pSubtable));
				break;
			case 1: // Macintosh
				if (GetU16(pEncodingRecord + offsetof(encodingRecord, encodingID)) == 0) { // Mac OS Roman
					_pSubtable = pTable + GetU32(pEncodingRecord + offsetof(encodingRecord, subtableOffset));
					CHIME_TRUETYPE_PRINT_EXTRA_INFO("Cmap subtable format (Platform ID: Macintosh): {}", GetU16(_pSubtable));
				}
				break;
			case 3: // Windows
				switch (GetU16(pEncodingRecord + offsetof(encodingRecord, encodingID))) {
				case 0:  // Non-Standard symbol
				case 1:  // Unicode BMP
				case 10: // Unicode full repertoire
					_pSubtable = pTable + GetU32(pEncodingRecord + offsetof(encodingRecord, subtableOffset));
					CHIME_TRUETYPE_PRINT_EXTRA_INFO("Cmap subtable format (Platform ID: Windows): {}", GetU16(_pSubtable));
				}
			}
			if (_pSubtable)
				switch (uint16_t _format = GetU16(_pSubtable)) {
				case 0:
				case 6:
					if (format == 0)
						format = _format,
						pSubtable = _pSubtable;
					break;
				case 10:
					if (format == 0 ||
						format == 6)
						format = _format,
						pSubtable = _pSubtable;
					break;
				case 4:
				case 12:
				case 13:
					if (format < _format ||
						format == 6 ||
						format == 10)
						format = _format,
						pSubtable = _pSubtable;
				}
		}
		// Parse subtable
		if (pSubtable)
			switch (format) {
				//case 2:  // For non-Unicode encoding, isn't commonly used today
				//case 8:  // Haven't seen any particular use
				//case 14: // Unicode Variation Sequences
			case 0:
				ParseSubtable_Format0(pSubtable); break;
			case 4:
				ParseSubtable_Format4(pSubtable); break;
			case 6:
				ParseSubtable_Format6(pSubtable); break;
			case 10:
				ParseSubtable_Format10(pSubtable); break;
			case 12:
				ParseSubtable_Format12(pSubtable); break;
			case 13:
				ParseSubtable_Format13(pSubtable); break;
			default:
				CHIME_TRUETYPE_PRINT_ERROR("Cmap subtable format {} is not supported!", format);
			}
	}
	void ParseSubtable_Format0(const uint8_t* pSubtable) {
		struct {
			struct _ {
				uint16 format;
				uint16 length;
				uint16 language;
			};
			const uint8_t* pData;
			uint8_t GlyphId(uint8_t codePoint) const {
				return GetU8(pData + sizeof(_) + codePoint);
			}
		} table(pSubtable);
		result.indexMappings.reserve(256);
		for (uint32_t codePoint = 0; codePoint < 256; codePoint++)
			result.indexMappings.emplace_back(codePoint, table.GlyphId(codePoint));
	}
	void ParseSubtable_Format4(const uint8_t* pSubtable) {
		struct {
			struct _ {
				uint16 format;
				uint16 length;
				uint16 language;
				uint16 segCountX2;
				uint16 searchRange;
				uint16 entrySelector;
				uint16 rangeShift;
			};
			const uint8_t* pData;
			uint16_t SegCountX2() const {
				return GetU16(pData + offsetof(_, segCountX2));
			}
			uint16_t EndCode(uint16_t segmentIndex) const {
				return GetU16(pData + sizeof(_) + sizeof(uint16_t) * segmentIndex);
			}
			uint16_t StartCode(uint16_t segmentIndex) const {
				return GetU16(pData + sizeof(_) + sizeof(uint16_t) * (1 + segmentIndex) + SegCountX2());
			}
			uint16_t GlyphId(uint16_t segmentIndex, uint16_t codePoint) const {
				uint16_t segmentCountX2 = SegCountX2();
				const uint8_t* pIdRangeOffset = pData + sizeof(_) + sizeof(uint16_t) * (1 + segmentIndex) + segmentCountX2 * 3;
				if (uint16_t idRangeOffset = GetU16(pIdRangeOffset))
					if (uint16_t glyphIndex = GetU16(pIdRangeOffset + sizeof(uint16_t) * (codePoint - StartCode(segmentIndex)) + idRangeOffset))
						return glyphIndex + GetU16(pIdRangeOffset - segmentCountX2);
					else
						return 0;
				// If the idRangeOffset is 0, the idDelta value is added directly to the character code to get the corresponding glyph index.
				return codePoint + GetU16(pIdRangeOffset - segmentCountX2);
			}
		} table(pSubtable);
		uint16_t segmentCount = table.SegCountX2() / 2;
		for (uint16_t i = 0; i < segmentCount; i++) {
			uint32_t startCode = table.StartCode(i);
			uint32_t endCode = table.EndCode(i);
			for (uint32_t codePoint = startCode; codePoint <= endCode; codePoint++)
				result.indexMappings.emplace_back(codePoint, table.GlyphId(i, codePoint));
		}
	}
	void ParseSubtable_Format6(const uint8_t* pSubtable) {
		struct {
			struct _ {
				uint16 format;
				uint16 length;
				uint16 language;
				uint16 firstCode;
				uint16 entryCount;
			};
			const uint8_t* pData;
			uint16_t FirstCode() const {
				return GetU16(pData + offsetof(_, firstCode));
			}
			uint16_t EntryCount() const {
				return GetU16(pData + offsetof(_, entryCount));
			}
			uint16_t GlyphId(uint16_t glyphIdIndex) const {
				return GetU16(pData + sizeof(_) + sizeof(uint16_t) * glyphIdIndex);
			}
		} table(pSubtable);
		uint16_t codePoint = table.FirstCode();
		uint16_t codeCount = table.EntryCount();
		result.indexMappings.reserve(codeCount);
		for (uint16_t i = 0; i < codeCount; i++, codePoint++)
			result.indexMappings.emplace_back(codePoint, table.GlyphId(i));
	}
	void ParseSubtable_Format10(const uint8_t* pSubtable) {
		struct {
			struct _ {
				uint16 format;
				uint16 reserved;
				uint32 length;
				uint32 language;
				uint32 startCharCode;
				uint32 numChars;
			};
			const uint8_t* pData;
			uint32_t StartCharCode() const {
				return GetU16(pData + offsetof(_, startCharCode));
			}
			uint32_t NumChars() const {
				return GetU16(pData + offsetof(_, numChars));
			}
			uint16_t GlyphId(uint32_t glyphIdIndex) const {
				return GetU16(pData + sizeof(_) + sizeof(uint16_t) * glyphIdIndex);
			}
		} table(pSubtable);
		uint32_t codePoint = table.StartCharCode();
		uint32_t codeCount = table.NumChars();
		result.indexMappings.reserve(codeCount);
		for (uint32_t i = 0; i < codeCount; i++, codePoint++)
			result.indexMappings.emplace_back(codePoint, table.GlyphId(i));
	}
	void ParseSubtable_Format12(const uint8_t* pSubtable) {
		struct {
			struct _ {
				uint16 format;
				uint16 reserved;
				uint32 length;
				uint32 language;
				uint32 numGroups;
			};
			struct sequentialMapGroup {
				uint32_t startCharCode;
				uint32_t endCharCode;
				uint32_t startGlyphID;
			};
			const uint8_t* pData;
			uint32_t NumGroups() const {
				return GetU32(pData + offsetof(_, numGroups));
			}
			sequentialMapGroup Group(uint32_t groupIndex) const {
				const uint8_t* pGroup = pData + sizeof(_) + sizeof(sequentialMapGroup) * groupIndex;
				return {
					GetU32(pGroup + offsetof(sequentialMapGroup, startCharCode)),
					GetU32(pGroup + offsetof(sequentialMapGroup, endCharCode)),
					GetU32(pGroup + offsetof(sequentialMapGroup, startGlyphID)) };
			}
		} table(pSubtable);
		uint32_t groupCount = table.NumGroups();
		for (uint16_t i = 0; i < groupCount; i++) {
			auto [startCode, endCode, glyphIndex] = table.Group(i);
			for (uint32_t codePoint = startCode; codePoint <= endCode; codePoint++, glyphIndex++)
				result.indexMappings.emplace_back(codePoint, glyphIndex);
		}
	}
	void ParseSubtable_Format13(const uint8_t* pSubtable) {
		struct {
			struct _ {
				uint16 format;
				uint16 reserved;
				uint32 length;
				uint32 language;
				uint32 numGroups;
			};
			struct constantMapGroup {
				uint32_t startCharCode;
				uint32_t endCharCode;
				uint32_t glyphID;
			};
			const uint8_t* pData;
			uint32_t NumGroups() const {
				return GetU32(pData + offsetof(_, numGroups));
			}
			constantMapGroup Group(uint32_t groupIndex) const {
				const uint8_t* pGroup = pData + sizeof(_) + sizeof(constantMapGroup) * groupIndex;
				return {
					GetU32(pGroup + offsetof(constantMapGroup, startCharCode)),
					GetU32(pGroup + offsetof(constantMapGroup, endCharCode)),
					GetU32(pGroup + offsetof(constantMapGroup, glyphID)) };
			}
		} table(pSubtable);
		uint32_t groupCount = table.NumGroups();
		for (uint16_t i = 0; i < groupCount; i++) {
			auto [startCode, endCode, glyphIndex] = table.Group(i);
			for (uint32_t codePoint = startCode; codePoint <= endCode; codePoint++)
				result.indexMappings.emplace_back(codePoint, glyphIndex);
		}
	}
};
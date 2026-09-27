class GPOSParser {
public:
	struct result_t {
		std::vector<kernParser::kerningPair> kerningPairs;
	};
	GPOSParser() = default;
	GPOSParser(const uint8_t* pTable) { ParseTable(pTable); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseTable(const uint8_t* pTable) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/gpos
		struct {
			struct _ {
				uint16 majorVersion;
				uint16 minorVersion;
				uint16 scriptListOffset;
				uint16 featureListOffset;
				uint16 lookupListOffset;
				uint32 featureVariationsOffset;
			};
			const uint8_t* pData;
			uint16_t LookupListOffset() const {
				return GetU16(pData + offsetof(_, lookupListOffset));
			}
		} table(pTable);
		ParseLookupListTable(pTable + table.LookupListOffset());
	}
	void ParseLookupListTable(const uint8_t* pLookupListTable) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#lookuplist-table
		struct {
			struct _ {
				uint16 lookupCount;
			};
			const uint8_t* pData;
			uint16_t LookupCount() const {
				return GetU16(pData + offsetof(_, lookupCount));
			}
			uint16_t LookupOffset(uint16_t lookupIndex) const {
				return GetU16(pData + sizeof(_) + sizeof(uint16_t) * lookupIndex);
			}
		} table(pLookupListTable);
		uint16_t lookupCount = table.LookupCount();
		for (uint16_t i = 0; i < lookupCount; i++)
			ParseLookupTable(pLookupListTable + table.LookupOffset(i));
	}
	void ParseLookupTable(const uint8_t* pLookupTable) {
		struct {
			struct _ {
				uint16 lookupType;
				uint16 lookupFlag;
				uint16 subTableCount;
			};
			const uint8_t* pData;
			uint16_t LookupType() const {
				return GetU16(pData + offsetof(_, lookupType));
			}
			uint16_t SubTableCount() const {
				return GetU16(pData + offsetof(_, subTableCount));
			}
			uint16_t SubtableOffset(uint16_t subtableIndex) const {
				return GetU16(pData + sizeof(_) + sizeof(uint16_t) * subtableIndex);
			}
		} table(pLookupTable);
		uint16_t subtableCount = table.SubTableCount();
		switch (table.LookupType()) {
		case 2:
			for (uint16_t i = 0; i < subtableCount; i++)
				ParseLookupSubtable_PairPos(pLookupTable + table.SubtableOffset(i));
			break;
		case 9:
			for (uint16_t i = 0; i < subtableCount; i++)
				ParseLookupSubtable_PosExtension(pLookupTable + table.SubtableOffset(i));
		}
	}
	void ParseLookupSubtable_PairPos(const uint8_t* pLookupSubtable) {
		enum valueFormat {
			X_PLACEMENT = 1 << 0,
			Y_PLACEMENT = 1 << 1,
			X_ADVANCE = 1 << 2,
			Y_ADVANCE = 1 << 3,
			X_PLACEMENT_DEVICE = 1 << 4,
			Y_PLACEMENT_DEVICE = 1 << 5,
			X_ADVANCE_DEVICE = 1 << 6,
			Y_ADVANCE_DEVICE = 1 << 7
		};
		auto ParseLookupSubtable_PairPosFormat1 = [&](const uint8_t* pLookupSubtable) {
			struct {
				struct _ {
					uint16 format;
					uint16 coverageOffset;
					uint16 valueFormat1;
					uint16 valueFormat2;
					uint16 pairSetCount;
				};
				const uint8_t* pData;
				uint16_t CoverageOffset() const {
					return GetU16(pData + offsetof(_, coverageOffset));
				}
				uint16_t ValueFormat1() const {
					return GetU16(pData + offsetof(_, valueFormat1));
				}
				uint16_t ValueFormat2() const {
					return GetU16(pData + offsetof(_, valueFormat2));
				}
				uint16_t PairSetCount() const {
					return GetU16(pData + offsetof(_, pairSetCount));
				}
				uint16_t PairSetOffset(uint16_t pairSetIndex) const {
					return GetU16(pData + sizeof(_) + sizeof(uint16_t) * pairSetIndex);
				}
			} table(pLookupSubtable);
			if (table.ValueFormat1() != X_ADVANCE &&
				table.ValueFormat1() != (X_ADVANCE | X_ADVANCE_DEVICE) ||
				table.ValueFormat2())
				return;
			std::vector<uint16_t> glyphIndices;
			ParseCoverageTable(pLookupSubtable + table.CoverageOffset(), glyphIndices);
			struct pairValue {
				uint16_t secondGlyph;
				int16_t xAdvance;
			};
			size_t recordSize = sizeof(pairValue) + sizeof(uint16_t) * bool(table.ValueFormat1() & X_ADVANCE_DEVICE);
			uint16_t pairSetCount = table.PairSetCount();
			for (uint16_t i = 0; i < pairSetCount; i++) {
				const uint8_t* pPairSetTable = pLookupSubtable + table.PairSetOffset(i);
				uint16_t pairValueCount = GetU16(pPairSetTable);
				uint16_t firstGlyph = glyphIndices[i];
				for (uint16_t i = 0; i < pairValueCount; i++) {
					const uint8_t* pPairValue = pPairSetTable + sizeof(uint16_t) + recordSize * i;
					result.kerningPairs.emplace_back(
						firstGlyph,
						GetU16(pPairValue + offsetof(pairValue, secondGlyph)),
						GetI16(pPairValue + offsetof(pairValue, xAdvance)));
				}
			}
		};
		auto ParseLookupSubtable_PairPosFormat2 = [&](const uint8_t* pLookupSubtable) {
			struct {
				struct _ {
					uint16 format;
					uint16 coverageOffset;
					uint16 valueFormat1;
					uint16 valueFormat2;
					uint16 classDef1Offset;
					uint16 classDef2Offset;
					uint16 class1Count;
					uint16 class2Count;
				};
				const uint8_t* pData;
				uint16_t CoverageOffset() const {
					return GetU16(pData + offsetof(_, coverageOffset));
				}
				uint16_t ValueFormat1() const {
					return GetU16(pData + offsetof(_, valueFormat1));
				}
				uint16_t ValueFormat2() const {
					return GetU16(pData + offsetof(_, valueFormat2));
				}
				uint16_t ClassDef1Offset() const {
					return GetU16(pData + offsetof(_, classDef1Offset));
				}
				uint16_t ClassDef2Offset() const {
					return GetU16(pData + offsetof(_, classDef2Offset));
				}
				uint16_t Class1Count() const {
					return GetU16(pData + offsetof(_, class1Count));
				}
				uint16_t Class2Count() const {
					return GetU16(pData + offsetof(_, class2Count));
				}
			} table(pLookupSubtable);
			if (table.ValueFormat1() != X_ADVANCE &&
				table.ValueFormat1() != (X_ADVANCE | X_ADVANCE_DEVICE) ||
				table.ValueFormat2())
				return;
			std::vector<uint16_t> glyphIndices;
			ParseCoverageTable(pLookupSubtable + table.CoverageOffset(), glyphIndices);
			std::vector<std::pair<uint16_t, uint16_t>> glyphIndexAndClassValuePairs1;
			std::vector<std::pair<uint16_t, uint16_t>> glyphIndexAndClassValuePairs2;
			ParseClassDefTable(pLookupSubtable + table.ClassDef1Offset(), glyphIndexAndClassValuePairs1);
			ParseClassDefTable(pLookupSubtable + table.ClassDef2Offset(), glyphIndexAndClassValuePairs2);
			if (glyphIndexAndClassValuePairs1.empty())
				glyphIndexAndClassValuePairs1.push_back({});
			std::pair<uint16_t, uint16_t>* pGlyphIndexAndClassValuePair1 = glyphIndexAndClassValuePairs1.data();
			std::pair<uint16_t, uint16_t>* pGlyphIndexAndClassValuePair1Back = &glyphIndexAndClassValuePairs1.back();
			const uint8_t* valueRecords = pLookupSubtable + sizeof(decltype(table)::_);
			size_t recordSize = sizeof(int16_t) + sizeof(uint16_t) * bool(table.ValueFormat1() & X_ADVANCE_DEVICE);
			uint16_t class2Count = table.Class2Count();
			for (auto& i : glyphIndices) {
				uint16_t class1 = i == pGlyphIndexAndClassValuePair1->first ?
					pGlyphIndexAndClassValuePair1->second :
					0;
				// Because PairPos subtables are used to adjust the positioning of specific glyph pairs rather than common situations,
				// in pratice, if the second glyph is not covered by the ClassDef2 table, xAdvance is very likely set to 0.
				// Here, simply ignores every glyph not covered by the ClassDef2 table.
				for (auto& j : glyphIndexAndClassValuePairs2)
					if (int16_t xAdvance = GetI16(valueRecords + recordSize * (class1 * class2Count + j.second)))
						result.kerningPairs.emplace_back(
							i,
							j.first,
							xAdvance);
				if (pGlyphIndexAndClassValuePair1->first == i &&
					pGlyphIndexAndClassValuePair1 < pGlyphIndexAndClassValuePair1Back)
					pGlyphIndexAndClassValuePair1++;
			}
		};
		switch (GetU16(pLookupSubtable)) {
		case 1:
			ParseLookupSubtable_PairPosFormat1(pLookupSubtable); break;
		case 2:
			ParseLookupSubtable_PairPosFormat2(pLookupSubtable);
		}
	}
	void ParseLookupSubtable_PosExtension(const uint8_t* pLookupSubtable) {
		struct {
			struct _ {
				uint16 format;
				uint16 extensionLookupType;
				uint32 extensionOffset;
			};
			const uint8_t* pData;
			uint16_t ExtensionLookupType() const {
				return GetU16(pData + offsetof(_, extensionLookupType));
			}
			uint16_t ExtensionOffset() const {
				return GetU32(pData + offsetof(_, extensionOffset));
			}
		} table(pLookupSubtable);
		if (table.ExtensionLookupType() == 2)
			ParseLookupSubtable_PairPos(pLookupSubtable + table.ExtensionOffset());
	}
	static void ParseCoverageTable(const uint8_t* pCoverageTable, std::vector<uint16_t>& glyphIndices) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#coverage-table
		auto ParseCoverageTable_Format1 = [&](const uint8_t* pCoverageTable) {
			struct {
				struct _ {
					uint16 format;
					uint16 glyphCount;
				};
				const uint8_t* pData;
				uint16_t GlyphCount() const {
					return GetU16(pData + offsetof(_, glyphCount));
				}
				uint16_t GlyphId(uint16_t glyphIdIndex) const {
					return GetU16(pData + sizeof(_) + sizeof(uint16_t) * glyphIdIndex);
				}
			} table(pCoverageTable);
			uint16_t glyphCount = table.GlyphCount();
			glyphIndices.reserve(glyphCount);
			for (uint16_t i = 0; i < glyphCount; i++)
				glyphIndices.push_back(table.GlyphId(i));
		};
		auto ParseCoverageTable_Format2 = [&](const uint8_t* pCoverageTable) {
			struct {
				struct _ {
					uint16 format;
					uint16 rangeCount;
				};
				struct rangeRecord {
					uint16 startGlyphID;
					uint16 endGlyphID;
					uint16 startCoverageIndex;
				};
				const uint8_t* pData;
				uint16_t RangeCount() const {
					return GetU16(pData + offsetof(_, rangeCount));
				}
				uint16_t StartGlyphId(uint16_t rangeIndex) const {
					return GetU16(pData + sizeof(_) + sizeof(rangeRecord) * rangeIndex + offsetof(rangeRecord, startGlyphID));
				}
				uint16_t EndGlyphId(uint16_t rangeIndex) const {
					return GetU16(pData + sizeof(_) + sizeof(rangeRecord) * rangeIndex + offsetof(rangeRecord, endGlyphID));
				}
				uint16_t StartCoverageIndex(uint16_t rangeIndex) const {
					return GetU16(pData + sizeof(_) + sizeof(rangeRecord) * rangeIndex + offsetof(rangeRecord, startCoverageIndex));
				}
			} table(pCoverageTable);
			uint16_t rangeCountMinusOne = table.RangeCount() - 1;
			glyphIndices.reserve(table.StartCoverageIndex(rangeCountMinusOne) + table.EndGlyphId(rangeCountMinusOne) - table.StartGlyphId(rangeCountMinusOne) + 1);
			for (uint16_t i = 0; i <= rangeCountMinusOne; i++) {
				uint16_t startGlyphIndex = table.StartGlyphId(i);
				uint16_t endGlyphIndex = table.EndGlyphId(i);
				for (uint16_t i = startGlyphIndex; i <= endGlyphIndex; i++)
					glyphIndices.push_back(i);
			}
		};
		switch (GetU16(pCoverageTable)) {
		case 1:
			ParseCoverageTable_Format1(pCoverageTable); break;
		case 2:
			ParseCoverageTable_Format2(pCoverageTable);
		}
	}
	static void ParseClassDefTable(const uint8_t* pClassDefTable, std::vector<std::pair<uint16_t, uint16_t>>& glyphIndexAndClassValuePairs) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#class-definition-table
		auto ParseClassDefTable_Format1 = [&](const uint8_t* pClassDefTable) {
			struct {
				struct _ {
					uint16 format;
					uint16 startGlyphID;
					uint16 glyphCount;
				};
				const uint8_t* pData;
				uint16_t StartGlyphId() const {
					return GetU16(pData + offsetof(_, startGlyphID));
				}
				uint16_t GlyphCount() const {
					return GetU16(pData + offsetof(_, glyphCount));
				}
				uint16_t ClassValue(uint16_t classValueIndex) {
					return GetU16(pData + sizeof(_) + sizeof(uint16_t) * classValueIndex);
				}
			} table(pClassDefTable);
			uint16_t glyphIndex = table.StartGlyphId();
			uint16_t glyphCount = table.GlyphCount();
			glyphIndexAndClassValuePairs.reserve(glyphCount);
			for (uint16_t i = 0; i < glyphCount; i++, glyphIndex++)
				glyphIndexAndClassValuePairs.emplace_back(glyphIndex, table.ClassValue(i));
		};
		auto ParseClassDefTable_Format2 = [&](const uint8_t* pClassDefTable) {
			struct {
				struct _ {
					uint16 format;
					uint16 classRangeCount;
				};
				struct classRange {
					uint16_t startGlyphID;
					uint16_t endGlyphID;
					uint16_t classValue;
				};
				const uint8_t* pData;
				uint16_t ClassRangeCount() const {
					return GetU16(pData + offsetof(_, classRangeCount));
				}
				classRange ClassRange(uint16_t classRangeIndex) const {
					const uint8_t* pClassRange = pData + sizeof(_) + sizeof(classRange) * classRangeIndex;
					return {
						GetU16(pClassRange + offsetof(classRange, startGlyphID)),
						GetU16(pClassRange + offsetof(classRange, endGlyphID)),
						GetU16(pClassRange + offsetof(classRange, classValue)) };
				}
			} table(pClassDefTable);
			uint16_t classRangeCount = table.ClassRangeCount();
			for (uint16_t i = 0; i < classRangeCount; i++) {
				auto [startGlyphIndex, endGlyphIndex, classValue] = table.ClassRange(i);
				for (uint32_t glyphIndex = startGlyphIndex; glyphIndex <= endGlyphIndex; glyphIndex++)
					glyphIndexAndClassValuePairs.emplace_back(glyphIndex, classValue);
			}
		};
		switch (GetU16(pClassDefTable)) {
		case 1:
			ParseClassDefTable_Format1(pClassDefTable); break;
		case 2:
			ParseClassDefTable_Format2(pClassDefTable);
		}
	}
};
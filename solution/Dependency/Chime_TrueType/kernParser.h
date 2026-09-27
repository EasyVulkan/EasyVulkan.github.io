class kernParser {
public:
	struct kerningPair {
		uint16_t glyph0;
		uint16_t glyph1;
		int16_t value;
	};
	struct result_t {
		std::vector<kerningPair> kerningPairs;
	};
	kernParser() = default;
	kernParser(const uint8_t* pTable) { ParseTable(pTable); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseTable(const uint8_t* pTable) {
		if (uint16_t version = GetU16(pTable))
			/*if (version == 1)
				return ParseTable_V1(pTable);
			else*/ {
				CHIME_TRUETYPE_PRINT_ERROR("Kern table version {} is not supported!", version);
				return;
			}
		// https://learn.microsoft.com/en-us/typography/opentype/spec/kern
		struct header {
			uint16 version;
			uint16 nTables;
		};
		struct subtableHeader {
			uint16 version;
			uint16 length;
			uint16 coverage;
		};
		const uint8_t* pSubtable = pTable + sizeof(header);
		uint16_t subtableCount = GetU16(pTable + offsetof(header, nTables));
		for (size_t i = 0; i < subtableCount; i++) {
			if (uint8_t format = GetU8(pSubtable + offsetof(subtableHeader, coverage)))
				CHIME_TRUETYPE_PRINT_ERROR("Kern subtable format {} is not supported!", format);
			else
				ParseSubtable_Format0(pSubtable);
			pSubtable += GetU16(pSubtable + offsetof(subtableHeader, length));
		}
	}
	void ParseTable_V1(const uint8_t* pTable) {
		// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6kern.html
		struct header {
			uint32 version;
			uint32 nTables;
		};
		struct subtableHeader {
			uint32 length;
			uint16 coverage;
			uint16 tupleIndex;
		};
		const uint8_t* pSubtable = pTable + sizeof(header);
		uint32_t subtableCount = GetU32(pTable + offsetof(header, nTables));
		for (size_t i = 0; i < subtableCount; i++) {
			if (uint8_t format = GetU8(pSubtable + offsetof(subtableHeader, coverage) + 1))
				CHIME_TRUETYPE_PRINT_ERROR("Kern subtable format {} is not supported!", format);
			else
				ParseSubtable_V1Format0(pSubtable);
			pSubtable += GetU32(pSubtable + offsetof(subtableHeader, length));
		}
	}
	void ParseSubtable_Format0(const uint8_t* pSubtable) {
		struct {
			struct _ {
				uint16 version;
				uint16 length;
				uint16 coverage;
				uint16 nPairs;
				uint16 searchRange;
				uint16 entrySelector;
				uint16 rangeShift;
			};
			const uint8_t* pData;
			uint16_t Coverage() const {
				return GetU16(pData + offsetof(_, coverage));
			}
			bool HasVerticalData() const {    // TODO
				return !(GetU8(pData + offsetof(_, coverage) + 1) & 1 << 0);
			}
			bool HasMinimumData() const {     // TODO
				return GetU8(pData + offsetof(_, coverage) + 1) & 1 << 1;
			}
			bool HasCrossStreamData() const { // TODO
				return GetU8(pData + offsetof(_, coverage) + 1) & 1 << 2;
			}
			bool Override() const {           // TODO
				return GetU8(pData + offsetof(_, coverage) + 1) & 1 << 3;
			}
			uint16_t NPairs() const {
				return GetU16(pData + offsetof(_, nPairs));
			}
			kerningPair KernPair(uint16_t kernPairIndex) const {
				const uint8_t* pKernPair = pData + sizeof(_) + sizeof(kerningPair) * kernPairIndex;
				return {
					GetU16(pKernPair + offsetof(kerningPair, glyph0)),
					GetU16(pKernPair + offsetof(kerningPair, glyph1)),
					GetI16(pKernPair + offsetof(kerningPair, value)) };
			}
		} table(pSubtable);
		if (table.Coverage() != 1) {
			CHIME_TRUETYPE_PRINT_ERROR("Kern subtable coverage mask 0b{:b} is not supported!", table.Coverage());
			return;
		}
		result.kerningPairs.reserve(result.kerningPairs.size() + table.NPairs());
		uint16_t kerningCount = table.NPairs();
		for (uint16_t i = 0; i < kerningCount; i++)
			result.kerningPairs.push_back(table.KernPair(i));
	}
	void ParseSubtable_V1Format0(const uint8_t* pSubtable) {
		struct {
			struct _ {
				uint32 length;
				uint16 coverage;
				uint16 tupleIndex;
				uint16 nPairs;
				uint16 searchRange;
				uint16 entrySelector;
				uint16 rangeShift;
			};
			const uint8_t* pData;
			uint16_t Coverage() const {
				return GetU16(pData + offsetof(_, coverage));
			}
			bool HasVerticalData() const {    // TODO
				return GetU8(pData + offsetof(_, coverage)) == 0x80;
			}
			bool HasCrossStreamData() const { // TODO
				return GetU8(pData + offsetof(_, coverage)) == 0x40;
			}
			bool HasVariationData() const {   // TODO
				return GetU8(pData + offsetof(_, coverage)) == 0x20;
			}
			uint16_t NPairs() const {
				return GetU16(pData + offsetof(_, nPairs));
			}
			kerningPair KernPair(uint16_t kernPairIndex) const {
				const uint8_t* pKernPair = pData + sizeof(_) + sizeof(kerningPair) * kernPairIndex;
				return {
					GetU16(pKernPair + offsetof(kerningPair, glyph0)),
					GetU16(pKernPair + offsetof(kerningPair, glyph1)),
					GetI16(pKernPair + offsetof(kerningPair, value)) };
			}
		} table(pSubtable);
		if (table.Coverage()) {
			CHIME_TRUETYPE_PRINT_ERROR("Kern subtable coverage mask 0b{:b} is not supported!", table.Coverage());
			return;
		}
		result.kerningPairs.reserve(result.kerningPairs.size() + table.NPairs());
		uint16_t kerningCount = table.NPairs();
		for (uint16_t i = 0; i < kerningCount; i++)
			result.kerningPairs.push_back(table.KernPair(i));
	}
};
class maxpParser {
public:
	struct result_t {
		uint16_t glyphCount;
	};
	maxpParser() = default;
	maxpParser(const uint8_t* pTable) { ParseTable(pTable); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseTable(const uint8_t* pTable) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/maxp
		struct {
			struct _ {
				uint32 version;
				uint16 numGlyphs;
				uint16 maxPoints;
				uint16 maxContours;
				uint16 maxCompositePoints;
				uint16 maxCompositeContours;
				uint16 maxZones;
				uint16 maxTwilightPoints;
				uint16 maxStorage;
				uint16 maxFunctionDefs;
				uint16 maxInstructionDefs;
				uint16 maxStackElements;
				uint16 maxSizeOfInstructions;
				uint16 maxComponentElements;
				uint16 maxComponentDepth;
			};
			const uint8_t* pData;
			uint16_t NumGlyphs() const { return GetU16(pData + offsetof(_, numGlyphs)); }
		} table(pTable);
		result.glyphCount = table.NumGlyphs();
	}
};
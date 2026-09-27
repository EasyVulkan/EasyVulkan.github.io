class locaParser {
public:
	struct result_t {
		std::vector<uint32_t> glyphDescriptionLengths;
	};
	locaParser() = default;
	locaParser(const uint8_t* pTable, int16_t locaFormat, uint16_t glyphCount) { ParseTable(pTable, locaFormat, glyphCount); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseTable(const uint8_t* pTable, int16_t locaFormat, uint16_t glyphCount) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/loca
		result.glyphDescriptionLengths.reserve(glyphCount);
		const uint8_t* pData = pTable;
		if (locaFormat == 0)
			for (size_t i = 0; i < glyphCount; i++, pData += sizeof(uint16_t))
				result.glyphDescriptionLengths.push_back((GetU16(pData + sizeof(uint16_t)) - GetU16(pData)) * 2);
		else
			for (size_t i = 0; i < glyphCount; i++, pData += sizeof(uint32_t))
				result.glyphDescriptionLengths.push_back(GetU32(pData + sizeof(uint32_t)) - GetU32(pData));
	}
};
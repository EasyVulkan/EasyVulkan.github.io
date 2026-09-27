class hmtxParser {
public:
	struct horizontalMetricPair {
		uint16_t advanceWidth;
		int16_t leftSideBearing;
	};
	struct result_t {
		std::vector<horizontalMetricPair> horizontalMetricPairs;
	};
	hmtxParser() = default;
	hmtxParser(const uint8_t* pTable, uint16_t hMetricCount, uint16_t glyphCount) { ParseTable(pTable, hMetricCount, glyphCount); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseTable(const uint8_t* pTable, uint16_t hMetricCount, uint16_t glyphCount) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/hmtx
		result.horizontalMetricPairs.reserve(glyphCount);
		for (size_t i = 0; i < hMetricCount; i++) {
			result.horizontalMetricPairs.emplace_back(
				GetU16(pTable + offsetof(horizontalMetricPair, advanceWidth)),
				GetI16(pTable + offsetof(horizontalMetricPair, leftSideBearing)));
			pTable += sizeof(horizontalMetricPair);
		}
		uint16_t lastAdvanceWidth = result.horizontalMetricPairs.back().advanceWidth;
		for (size_t i = hMetricCount; i < glyphCount; i++) {
			result.horizontalMetricPairs.emplace_back(lastAdvanceWidth, GetI16(pTable));
			pTable += sizeof(int16_t);
		}
	}
};
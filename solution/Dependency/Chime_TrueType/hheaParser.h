class hheaParser {
public:
	struct result_t {
		int16_t ascender;
		int16_t descender;
		int16_t lineGap;
		uint16_t hMetricCount;
	};
	hheaParser() = default;
	hheaParser(const uint8_t* pTable) { ParseTable(pTable); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseTable(const uint8_t* pTable) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/hhea
		struct {
			struct _ {
				uint16 majorVersion;
				uint16 minorVersion;
				int16 ascender;
				int16 descender;
				int16 lineGap;
				uint16 advanceWidthMax;
				int16 minLeftSideBearing;
				int16 minRightSideBearing;
				int16 xMaxExtent;
				int16 caretSlopeRise;
				int16 caretSlopeRun;
				int16 caretOffset;
				int16 reserved[4];
				int16 metricDataFormat;
				uint16 numberOfHMetrics;
			};
			const uint8_t* pData;
			int16_t Ascender() const { return GetI16(pData + offsetof(_, ascender)); }
			int16_t Descender() const { return GetI16(pData + offsetof(_, descender)); }
			int16_t LineGap() const { return GetI16(pData + offsetof(_, lineGap)); }
			uint16_t NumberOfHMetrics() const { return GetU16(pData + offsetof(_, numberOfHMetrics)); }
		} table(pTable);
		result.ascender = table.Ascender();
		result.descender = table.Descender();
		result.lineGap = table.LineGap();
		result.hMetricCount = table.NumberOfHMetrics();
	}
};
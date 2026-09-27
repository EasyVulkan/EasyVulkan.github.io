class headParser {
public:
	struct result_t {
		int16_t xMin;
		int16_t yMin;
		int16_t xMax;
		int16_t yMax;
		int16_t locaFormat;
	};
	headParser() = default;
	headParser(const uint8_t* pTable) { ParseTable(pTable); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseTable(const uint8_t* pTable) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/head
		struct {
			using LONGDATETIME = uint8_t[8];
			struct _ {
				uint16 majorVersion;
				uint16 minorVersion;
				uint32 fontRevision;
				uint32 checksumAdjustment;
				uint32 magicNumber;
				uint16 flags;      // TODO: bit 0~3
				uint16 unitsPerEm;
				LONGDATETIME created;
				LONGDATETIME modified;
				int16 xMin;
				int16 yMin;
				int16 xMax;
				int16 yMax;
				uint16 macStyle;
				uint16 lowestRecPPEM;
				int16 fontDirectionHint;
				int16 indexToLocFormat;
				int16 glyphDataFormat;
			};
			const uint8_t* pData;
			int16_t XMin() const { return GetI16(pData + offsetof(_, xMin)); }
			int16_t YMin() const { return GetI16(pData + offsetof(_, yMin)); }
			int16_t XMax() const { return GetI16(pData + offsetof(_, xMax)); }
			int16_t YMax() const { return GetI16(pData + offsetof(_, yMax)); }
			int16_t IndexToLocFormat() const { return GetI16(pData + offsetof(_, indexToLocFormat)); }
		} table(pTable);
		result.xMin = table.XMin();
		result.yMin = table.YMin();
		result.xMax = table.XMax();
		result.yMax = table.YMax();
		result.locaFormat = table.IndexToLocFormat();
	}
};
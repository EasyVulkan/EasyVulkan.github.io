class OS_2Parser {
public:
	struct result_t {
		int16_t typoAscender;
		int16_t typoDescender;
		int16_t typoLineGap;
	};
	OS_2Parser() = default;
	OS_2Parser(const uint8_t* pTable) { ParseTable(pTable); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseTable(const uint8_t* pTable) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/os2
		struct {
			struct _ {
				uint16 version;
				int16 xAvgCharWidth;
				uint16 usWeightClass;
				uint16 usWidthClass;
				uint16 fsType;
				int16 ySubscriptXSize;
				int16 ySubscriptYSize;
				int16 ySubscriptXOffset;
				int16 ySubscriptYOffset;
				int16 ySuperscriptXSize;
				int16 ySuperscriptYSize;
				int16 ySuperscriptXOffset;
				int16 ySuperscriptYOffset;
				int16 yStrikeoutSize;
				int16 yStrikeoutPosition;
				int16 sFamilyClass;
				uint8 panose[10];
				uint32 ulUnicodeRange[4];
				uint32 achVendID;
				uint16 fsSelection;
				uint16 usFirstCharIndex;
				uint16 usLastCharIndex;
				int16 sTypoAscender;
				int16 sTypoDescender;
				int16 sTypoLineGap;
				uint16 usWinAscent;
				uint16 usWinDescent;
				uint32 ulCodePageRange1;
				uint32 ulCodePageRange2;
				int16 sxHeight;
				int16 sCapHeight;
				uint16 usDefaultChar;
				uint16 usBreakChar;
				uint16 usMaxContext;
				uint16 usLowerOpticalPointSize;
				uint16 usUpperOpticalPointSize;
			};
			const uint8_t* pData;
			int16_t TypoAscender() const { return GetI16(pData + offsetof(_, sTypoAscender)); }
			int16_t TypoDescender() const { return GetI16(pData + offsetof(_, sTypoDescender)); }
			int16_t TypoLineGap() const { return GetI16(pData + offsetof(_, sTypoLineGap)); }
		} table(pTable);
		result.typoAscender = table.TypoAscender();
		result.typoDescender = table.TypoDescender();
		result.typoLineGap = table.TypoLineGap();
	}
};
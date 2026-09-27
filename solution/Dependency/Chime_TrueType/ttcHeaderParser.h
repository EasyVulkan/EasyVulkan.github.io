class ttcHeaderParser {
public:
	struct result_t {
		std::vector<uint32_t> fontOffsets;
	};
	ttcHeaderParser() = default;
	ttcHeaderParser(const uint8_t* fileBytes) { ParseHeader(fileBytes); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseHeader(const uint8_t* fileBytes) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/otff#ttc-header
		struct {
			struct _ {
				uint32 ttcTag;
				uint16 majorVersion;
				uint16 minorVersion;
				uint32 numFonts;
			};
			const uint8_t* pData;
			bool IsTtc() const { return GetU32(pData + offsetof(_, ttcTag)) == 'ttcf'; }
			uint32_t NumFonts() const { return GetU32(pData + offsetof(_, numFonts)); }
			uint32_t TableDirectoryOffset(uint16_t fontIndex) const {
				return GetU32(pData + sizeof(_) + sizeof(uint32_t) * fontIndex);
			}
		} ttcHeader(fileBytes);
		if (!ttcHeader.IsTtc())
			return;
		uint32_t fontCount = ttcHeader.NumFonts();
		for (size_t i = 0; i < fontCount; i++)
			result.fontOffsets.push_back(ttcHeader.TableDirectoryOffset(i));
	}
};
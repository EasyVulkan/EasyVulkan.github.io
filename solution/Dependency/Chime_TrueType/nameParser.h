class nameParser {
public:
	struct result_t {
		struct : std::string {
			std::u16string_view U16StringView() const { return { reinterpret_cast<const char16_t*>(data()), size() / 2 }; }
			bool Contains(std::string_view string) const {
				return find(string) != std::string::npos;
			}
			bool Contains(std::u16string_view string) const {
				return U16StringView().find(string) != std::string::npos;
			}
		} fullName;
		bool nameEncodingIsU16;
	};
	nameParser() = default;
	nameParser(const uint8_t* pTable) { ParseTable(pTable); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
protected:
	result_t result;
	void ParseTable(const uint8_t* pTable) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/name
		struct header {
			uint16 version;
			uint16 count;
			uint16 storageOffset;
		};
		struct nameRecord {
			uint16 platformID;
			uint16 encodingID;
			uint16 languageID;
			uint16 nameID;
			uint16 length;
			uint16 stringOffset;
		};
		const uint8_t* pNameRecord = pTable + sizeof(header);
		uint16_t nameRecordCount = GetU16(pTable + offsetof(header, count));
		for (size_t i = 0; i < nameRecordCount; i++, pNameRecord += sizeof(nameRecord))
			if (GetU16(pNameRecord + offsetof(nameRecord, nameID)) == 4) {
				result.nameEncodingIsU16 = false;
				switch (GetU16(pNameRecord + offsetof(nameRecord, platformID))) {
				case 0: // Unicode
					result.nameEncodingIsU16 = true;
					break;
				case 1: // Macintosh
					break;
				case 3: // Microsoft
					switch (GetU16(pNameRecord + offsetof(nameRecord, encodingID))) {
					case 3: // PRC
					case 4: // Big5
					case 5: // Wansung
					default:
						result.nameEncodingIsU16 = true;
					}
				}
				const uint8_t* fullName = pTable +
					GetU16(pTable + offsetof(header, storageOffset)) +
					GetU16(pNameRecord + offsetof(nameRecord, stringOffset));
				uint16_t stringLength = GetU16(pNameRecord + offsetof(nameRecord, length));
				result.fullName.resize(stringLength);
				if (result.nameEncodingIsU16)
					for (size_t i = 0; i < stringLength; i += 2) {
						uint16_t character = GetU16(fullName + i);
						memcpy(&result.fullName[i], &character, sizeof(uint16_t));
					}
				else
					memcpy(result.fullName.data(), fullName, stringLength);
				return;
			}
	}
};
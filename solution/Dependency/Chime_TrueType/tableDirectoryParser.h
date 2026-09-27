class tableDirectoryParser {
public:
	struct tableTagAndOffset {
		uint32_t tag;
		uint32_t offset;
	};
	struct result_t {
		uint32_t sfntVersion;
		std::vector<tableTagAndOffset> tableRecords;
	};
	tableDirectoryParser() = default;
	tableDirectoryParser(const uint8_t* pTableDirectory, const char* filepath = nullptr) { ParseTableDirectory(pTableDirectory, filepath); }
	const result_t& Result() const { return result; }
	result_t& Result() { return result; }
	const uint8_t* FindTable(const uint8_t* fileBytes, uint32_t tableTag, const char* tableTagString = nullptr, const char* filepath = nullptr) const {
		for (auto& i : result.tableRecords)
			if (i.tag == tableTag)
				return fileBytes + i.offset;
		if (tableTagString)
			filepath ?
				CHIME_TRUETYPE_PRINT_ERROR("The '{}' table is required but not found!\nFile: {}", tableTagString, filepath) :
				CHIME_TRUETYPE_PRINT_ERROR("The '{}' table is required but not found!", tableTagString);
		return nullptr;
	}
protected:
	result_t result;
	void ParseTableDirectory(const uint8_t* pTableDirectory, const char* filepath) {
		// https://learn.microsoft.com/en-us/typography/opentype/spec/otff#table-directory
		struct {
			struct _ {
				uint32 sfntVersion;
				uint16 numTables;
				uint16 searchRange;
				uint16 entrySelector;
				uint16 rangeShift;
			};
			struct tableRecord {
				uint32 tableTag;
				uint32 checksum;
				uint32 offset;
				uint32 length;
			};
			const uint8_t* pData;
			uint32_t SfntVersion() const { return GetU32(pData + offsetof(_, sfntVersion)); }
			uint16_t NumTables() const { return GetU16(pData + offsetof(_, numTables)); }
			tableTagAndOffset TableTagAndOffset(uint16_t tableIndex) const {
				const uint8_t* pTableRecord = pData + sizeof(_) + sizeof(tableRecord) * tableIndex;
				return {
					GetU32(pTableRecord + offsetof(tableRecord, tableTag)),
					GetU32(pTableRecord + offsetof(tableRecord, offset)) };
			}
		} tableDirectory(pTableDirectory);
		result.sfntVersion = tableDirectory.SfntVersion();
		if (result.sfntVersion != 0x00010000 &&
			//result.sfntVersion != 'true' &&
			//result.sfntVersion != 'typ1' &&
			result.sfntVersion != 'OTTO') {
			filepath ?
				CHIME_TRUETYPE_PRINT_ERROR("File format is not supported!\nFile: {}", filepath) :
				CHIME_TRUETYPE_PRINT_ERROR("File format is not supported!");
			return;
		}
		uint16_t tableCount = tableDirectory.NumTables();
		for (uint16_t i = 0; i < tableCount; i++)
			result.tableRecords.push_back(tableDirectory.TableTagAndOffset(i));
	}
};
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string_view>
#include "halley/file_formats/csv_file.h"

using namespace Halley;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	try {
		CSVFile csv;
		csv.load(std::string_view(reinterpret_cast<const char*>(data), size));
		if (csv.getColumns().empty()) {
			return -1;
		}
		for (size_t i = 0; i < csv.getNumRows() && i < 256; ++i) {
			csv.getRow(i);
		}
		csv.save();
	} catch (const std::exception&) {
		return -1;
	}
	return 0;
}

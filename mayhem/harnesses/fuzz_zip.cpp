#include <cstddef>
#include <cstdint>
#include <exception>
#include "halley/file_formats/zip_file.h"

using namespace Halley;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	try {
		ZipFile zip;
		if (!zip.open(Bytes(reinterpret_cast<const Byte*>(data), reinterpret_cast<const Byte*>(data) + size))) {
			return -1;
		}
		const size_t n = zip.getNumFiles();
		for (size_t i = 0; i < n && i < 64; ++i) {
			zip.getFileName(i);
			if (zip.getFileSize(i) < (1u << 24)) {
				zip.extractFile(i);
			}
		}
	} catch (const std::exception&) {
		return -1;
	}
	return 0;
}

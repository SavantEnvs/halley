#include <cstddef>
#include <cstdint>
#include <exception>
#include <gsl/gsl>
#include "halley/file_formats/image.h"

using namespace Halley;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	try {
		const gsl::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(data), size);
		const Image img(bytes);
		img.getTrimRect();
		img.countColours();
	} catch (const std::exception&) {
		return -1;
	}
	return 0;
}

#include <cstddef>
#include <cstdint>
#include <exception>
#include <gsl/gsl>
#include "halley/file_formats/json_convert.h"

using namespace Halley;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	try {
		const ConfigNode node = JSONConvert::parseConfig(gsl::span<const std::byte>(reinterpret_cast<const std::byte*>(data), size));
		JSONConvert::generateJSON(node);
	} catch (const std::exception&) {
		return -1;
	}
	return 0;
}

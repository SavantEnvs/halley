#include <cstddef>
#include <cstdint>
#include <exception>
#include "halley/file_formats/yaml_convert.h"
#include "halley/text/halleystring.h"

using namespace Halley;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	try {
		const String str(reinterpret_cast<const char*>(data), size);
		const ConfigNode node = YAMLConvert::parseConfig(str);
		YAMLConvert::generateYAML(node);
	} catch (const std::exception&) {
		return -1;
	}
	return 0;
}

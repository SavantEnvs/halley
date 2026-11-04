#include <cstddef>
#include <cstdint>
#include <exception>
#include <gsl/gsl>
#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"

using namespace Halley;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	try {
		const gsl::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(data), size);
		ConfigNode node;
		Deserializer::fromBytes(node, bytes, SerializerOptions(SerializerOptions::maxVersion));
		Serializer::toBytes(node, SerializerOptions(SerializerOptions::maxVersion));
	} catch (const std::exception&) {
		return -1;
	}
	return 0;
}

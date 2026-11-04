#include <cstddef>
#include <cstdint>
#include <exception>
#include <vector>
#include <gsl/gsl>
#include "halley/bytes/compression.h"

using namespace Halley;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	if (size < 1) {
		return -1;
	}
	const gsl::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(data + 1), size - 1);
	try {
		switch (data[0] % 4) {
		case 0:
			Compression::decompress(bytes, 1 << 24);
			break;
		case 1: {
			std::byte header[16];
			Compression::lz4DecompressFile(bytes, gsl::span<std::byte>(header, sizeof(header)));
			break;
		}
		case 2: {
			std::vector<std::byte> dst(1 << 16);
			Compression::lz4Decompress(bytes, gsl::span<std::byte>(dst.data(), dst.size()));
			break;
		}
		case 3:
			CompressedBytes::fromSerializedBytes(bytes).decompressBytes();
			break;
		}
	} catch (const std::exception&) {
		return -1;
	}
	return 0;
}

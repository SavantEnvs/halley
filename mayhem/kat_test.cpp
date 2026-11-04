// Known-answer tests for the file-format code the fuzz targets exercise (upstream's tests don't cover it).
#include <gtest/gtest.h>
#include <halley.hpp>
#include <cstring>
#include <string>
using namespace Halley;

static gsl::span<const std::byte> asBytes(const std::string& s)
{
	return gsl::span<const std::byte>(reinterpret_cast<const std::byte*>(s.data()), s.size());
}

TEST(MayhemKat, YamlMap)
{
	const ConfigNode node = YAMLConvert::parseConfig(String("name: halley\nversion: 3\nratio: 0.5\nitems:\n  - a\n  - b\n"));
	EXPECT_EQ(node["name"].asString(), "halley");
	EXPECT_EQ(node["version"].asInt(), 3);
	EXPECT_FLOAT_EQ(node["ratio"].asFloat(), 0.5f);
	ASSERT_EQ(node["items"].asSequence().size(), 2u);
	EXPECT_EQ(node["items"][1].asString(), "b");
}

TEST(MayhemKat, YamlRoundTrip)
{
	const ConfigNode a = YAMLConvert::parseConfig(String("x: 1\ny: [2, 3]\n"));
	const ConfigNode b = YAMLConvert::parseConfig(YAMLConvert::generateYAML(a));
	EXPECT_EQ(b["x"].asInt(), 1);
	EXPECT_EQ(b["y"][1].asInt(), 3);
}

TEST(MayhemKat, JsonObject)
{
	const std::string json = R"({"n":42,"s":"hi","arr":[1,2,3],"ok":true})";
	const ConfigNode node = JSONConvert::parseConfig(asBytes(json));
	EXPECT_EQ(node["n"].asInt(), 42);
	EXPECT_EQ(node["s"].asString(), "hi");
	EXPECT_EQ(node["arr"].asSequence().size(), 3u);
	EXPECT_EQ(node["arr"][2].asInt(), 3);
	EXPECT_TRUE(node["ok"].asBool());
}

TEST(MayhemKat, ConfigBinaryRoundTrip)
{
	ConfigNode n = ConfigNode::MapType();
	n["k"] = 7;
	n["s"] = "str";
	const auto bytes = Serializer::toBytes(n, SerializerOptions(SerializerOptions::maxVersion));
	ConfigNode back;
	Deserializer::fromBytes(back, bytes.const_byte_span(), SerializerOptions(SerializerOptions::maxVersion));
	EXPECT_EQ(back["k"].asInt(), 7);
	EXPECT_EQ(back["s"].asString(), "str");
}

TEST(MayhemKat, Lz4RoundTrip)
{
	Bytes src;
	for (int i = 0; i < 1000; ++i) {
		src.push_back(static_cast<Byte>(i % 7));
	}
	const Bytes packed = Compression::lz4Compress(src.const_byte_span());
	EXPECT_LT(packed.size(), src.size());
	Bytes dst(src.size());
	const auto n = Compression::lz4Decompress(packed.const_byte_span(), dst.byte_span());
	ASSERT_TRUE(n.has_value());
	EXPECT_EQ(*n, src.size());
	EXPECT_EQ(std::memcmp(dst.data(), src.data(), src.size()), 0);
}

TEST(MayhemKat, ZlibRoundTrip)
{
	const std::string text = "halley halley halley halley";
	const Bytes packed = Compression::compress(asBytes(text));
	const Bytes back = Compression::decompress(packed);
	ASSERT_EQ(back.size(), text.size());
	EXPECT_EQ(std::memcmp(back.data(), text.data(), text.size()), 0);
}

TEST(MayhemKat, ImageFormats)
{
	Image img(Image::Format::RGBA, Vector2i(4, 3), true);
	auto px = img.getPixels4BPP();
	for (size_t i = 0; i < px.size(); ++i) {
		px[i] = Image::convertRGBAToInt(static_cast<unsigned>(i * 20), static_cast<unsigned>(255 - i * 20), 7, 255);
	}
	const Bytes variants[] = { img.savePNGToBytes(), img.saveQOIToBytes(), img.saveHLIFToBytes("kat") };
	for (const auto& bytes : variants) {
		const Image back(bytes.const_byte_span());
		ASSERT_EQ(back.getSize(), Vector2i(4, 3));
		const auto bpx = back.getPixels4BPP();
		for (size_t i = 0; i < px.size(); ++i) {
			EXPECT_EQ(bpx[i], px[i]) << "pixel " << i;
		}
	}
}

TEST(MayhemKat, ZipExtract)
{
	// stored-entry zip containing "a.txt" = "hello zip"
	static const unsigned char zip[] = {
		0x50,0x4b,0x03,0x04,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0x00,0x8b,0x73,0x95,0xac,0x09,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x61,0x2e,0x74,0x78,0x74,0x68,0x65,0x6c,0x6c,0x6f,0x20,0x7a,0x69,0x70,0x50,0x4b,0x01,0x02,0x14,0x03,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0x00,0x8b,0x73,0x95,0xac,0x09,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x01,0x00,0x00,0x00,0x00,0x61,0x2e,0x74,0x78,0x74,0x50,0x4b,0x05,0x06,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x33,0x00,0x00,0x00,0x2c,0x00,0x00,0x00,0x00,0x00
	};
	ZipFile z;
	ASSERT_TRUE(z.open(Bytes(zip, zip + sizeof(zip))));
	ASSERT_EQ(z.getNumFiles(), 1u);
	EXPECT_EQ(z.getFileName(0), "a.txt");
	const Bytes data = z.extractFile(0);
	EXPECT_EQ(std::string(reinterpret_cast<const char*>(data.data()), data.size()), "hello zip");
}

TEST(MayhemKat, Csv)
{
	CSVFile csv;
	csv.load(std::string_view("id,name\n1,alice\n2,\"bo,b\"\n"));
	EXPECT_EQ(csv.getNumRows(), 2u);
	EXPECT_EQ(csv.getRow(1)[1], "bo,b");
	EXPECT_EQ(csv.getRow(0)[1], "alice");
}

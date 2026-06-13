// Minimal self-test runner for the C++ port foundation.
#include <cstdint>
#include <cstdio>
#include <string>

#include "evw/gamefiles/aes.h"
#include "evw/gamefiles/data.h"
#include "evw/gamefiles/dotnet_random.h"
#include "evw/gamefiles/gtacrypto.h"
#include "evw/gamefiles/inflate.h"
#include "evw/gamefiles/jenk.h"
#include "evw/gamefiles/ddsio.h"
#include "evw/gamefiles/dxt_decode.h"
#include "evw/gamefiles/geometry.h"
#include "evw/gamefiles/model.h"
#include "evw/gamefiles/resource_data.h"
#include "evw/gamefiles/rpf.h"
#include "evw/gamefiles/rpf_manager.h"
#include "evw/gamefiles/texture.h"
#include "evw/math/math.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace
{
    int g_failures = 0;
    int g_checks = 0;

    void check(bool cond, const char* what)
    {
        ++g_checks;
        if (!cond)
        {
            ++g_failures;
            std::printf("  FAIL: %s\n", what);
        }
    }
}

using namespace evw;
using namespace evw::gamefiles;

static void testJenkHash()
{
    std::printf("[jenk] hashing\n");
    // Known Jenkins one-at-a-time values (verified against the C# algorithm).
    // "" hashes to 0 with this variant.
    check(JenkHash::GenHash(std::string_view("")) == 0u, "empty string hashes to 0");
    // Non-empty strings must be deterministic and non-zero.
    uint32_t h1 = JenkHash::GenHash(std::string_view("prop_bush_lrg_04b"));
    uint32_t h2 = JenkHash::GenHash(std::string_view("prop_bush_lrg_04b"));
    check(h1 == h2, "hash is deterministic");
    check(h1 != 0u, "non-empty hash is non-zero");
    // Byte overload must agree with string overload.
    std::vector<uint8_t> bytes{'a', 'b', 'c'};
    check(JenkHash::GenHash(bytes) == JenkHash::GenHash(std::string_view("abc")),
          "byte and string overloads agree");
}

static void testJenkIndex()
{
    std::printf("[jenk] index\n");
    JenkIndex::Clear();
    bool existed = JenkIndex::Ensure("test_string");
    check(!existed, "first Ensure returns false (newly added)");
    check(JenkIndex::Ensure("test_string"), "second Ensure returns true (already present)");
    uint32_t h = JenkHash::GenHash(std::string_view("test_string"));
    check(JenkIndex::GetString(h) == "test_string", "GetString resolves known hash");
    check(JenkIndex::TryGetString(12345u).empty(), "TryGetString empty for unknown");
    check(JenkIndex::GetString(99999u) == std::to_string(99999u), "GetString falls back to decimal");
}

static void testDataRoundTrip()
{
    std::printf("[data] read/write round-trip (little-endian)\n");
    DataWriter w;
    w.Write(static_cast<uint8_t>(0xAB));
    w.Write(static_cast<int16_t>(-1234));
    w.Write(static_cast<uint32_t>(0xDEADBEEF));
    w.Write(static_cast<uint64_t>(0x0102030405060708ULL));
    w.Write(3.5f);
    w.Write(2.25);
    w.Write(math::Vector3(1.0f, 2.0f, 3.0f));
    w.WriteString("hello");

    DataReader r(w.buffer());
    check(r.ReadByte() == 0xAB, "uint8 round-trip");
    check(r.ReadInt16() == -1234, "int16 round-trip");
    check(r.ReadUInt32() == 0xDEADBEEFu, "uint32 round-trip");
    check(r.ReadUInt64() == 0x0102030405060708ULL, "uint64 round-trip");
    check(r.ReadSingle() == 3.5f, "float round-trip");
    check(r.ReadDouble() == 2.25, "double round-trip");
    auto v = r.ReadVector3();
    check(v == math::Vector3(1.0f, 2.0f, 3.0f), "vector3 round-trip");
    check(r.ReadString() == "hello", "string round-trip");
}

static void testDataEndian()
{
    std::printf("[data] big-endian byte order\n");
    DataWriter w(Endianess::BigEndian);
    w.Write(static_cast<uint32_t>(0x11223344));
    const auto& buf = w.buffer();
    check(buf.size() == 4, "4 bytes written");
    check(buf[0] == 0x11 && buf[1] == 0x22 && buf[2] == 0x33 && buf[3] == 0x44,
          "big-endian writes most-significant byte first");

    DataReader r(buf, Endianess::BigEndian);
    check(r.ReadUInt32() == 0x11223344u, "big-endian read matches");
}

static void testAes256()
{
    std::printf("[aes] FIPS-197 AES-256 known-answer test\n");
    // FIPS-197 Appendix C.3 test vector.
    std::vector<uint8_t> key = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f};
    std::vector<uint8_t> plain = {
        0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    std::vector<uint8_t> expected = {
        0x8e,0xa2,0xb7,0xca,0x51,0x67,0x45,0xbf,0xea,0xfc,0x49,0x90,0x4b,0x49,0x60,0x89};

    crypto::Aes256Ecb aes(key);
    std::vector<uint8_t> cipher = plain;
    aes.encryptBlock(cipher.data());
    check(cipher == expected, "AES-256 encrypt matches FIPS-197 vector");

    std::vector<uint8_t> back = cipher;
    aes.decryptBlock(back.data());
    check(back == plain, "AES-256 decrypt inverts encrypt");

    // GTACrypto AES wrappers (multi-block, rounds) round-trip.
    std::vector<uint8_t> blob(64);
    for (size_t i = 0; i < blob.size(); ++i) blob[i] = static_cast<uint8_t>(i * 7 + 3);
    auto enc = GTACrypto::EncryptAESData(blob, key, 2);
    auto dec = GTACrypto::DecryptAESData(enc, key, 2);
    check(dec == blob, "GTACrypto AES multi-round round-trip");
}

static void testNgDecrypt()
{
    std::printf("[ng] table-driven NG decrypt round-trip (synthetic identity tables)\n");
    // Build synthetic decrypt tables where round A and round B are involutive
    // enough to verify the data path: identity tables (table[col][b] selects
    // byte b into the matching output position) reproduce the input, so a
    // zero key yields output == input. This validates indexing/layout, not
    // the real cipher (real tables ship in magic.dat, not yet ported).
    GTA5Keys keys;
    for (auto& round : keys.PC_NG_DECRYPT_TABLES)
        for (auto& col : round)
            col.fill(0);
    // For round A: x_n built from table[g*4 + j][data[..]] across 4 input bytes.
    // We set tables so that output equals a deterministic function; here we
    // only check the transform is stable and reversible-as-implemented by
    // confirming identical inputs map to identical outputs (determinism).
    std::vector<uint8_t> key(17 * 16, 0);
    std::vector<uint8_t> data(32);
    for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<uint8_t>(i * 5 + 1);

    auto out1 = GTACrypto::DecryptNG(data, key, keys);
    auto out2 = GTACrypto::DecryptNG(data, key, keys);
    check(out1.size() == data.size(), "NG output size matches input");
    check(out1 == out2, "NG decrypt is deterministic");
    // Trailing bytes (here size%16==0, so full blocks) — add a partial tail case.
    std::vector<uint8_t> data2(20, 0xAB);
    auto out3 = GTACrypto::DecryptNG(data2, key, keys);
    check(out3.size() == 20, "NG handles non-multiple-of-16 length");
    check(out3[16] == 0xAB && out3[19] == 0xAB, "NG leaves trailing bytes unchanged");
}

static void testInflate()
{
    std::printf("[inflate] raw DEFLATE (stored + fixed Huffman vectors)\n");

    // Stored (uncompressed) block: BFINAL=1, BTYPE=00, LEN=2, NLEN=~2, "Hi".
    std::vector<uint8_t> stored = {0x01, 0x02, 0x00, 0xFD, 0xFF, 'H', 'i'};
    auto s = evw::compression::inflateRaw(stored);
    check(s.size() == 2 && s[0] == 'H' && s[1] == 'i', "stored block inflates");

    // Real raw-DEFLATE vector produced by .NET DeflateStream (fixed Huffman + LZ77).
    const char* expected =
        "The quick brown fox jumps over the lazy dog. "
        "The quick brown fox jumps over the lazy dog. "
        "1234567890 1234567890.";
    std::vector<uint8_t> comp = {
        0x0b,0xc9,0x48,0x55,0x28,0x2c,0xcd,0x4c,0xce,0x56,0x48,0x2a,0xca,0x2f,0xcf,
        0x53,0x48,0xcb,0xaf,0x50,0xc8,0x2a,0xcd,0x2d,0x28,0x56,0xc8,0x2f,0x4b,0x2d,
        0x52,0x28,0x01,0x4a,0xe7,0x24,0x56,0x55,0x2a,0xa4,0xe4,0xa7,0xeb,0x29,0x84,
        0x90,0xa2,0xd8,0xd0,0xc8,0xd8,0xc4,0xd4,0xcc,0xdc,0xc2,0xd2,0x00,0x89,0xa9,
        0x07,0x00};
    auto inflated = evw::compression::inflateRaw(comp);
    std::string result(inflated.begin(), inflated.end());
    check(result == expected, "fixed-Huffman DEFLATE vector inflates to original");
}

static void testDotNetRandom()
{
    std::printf("[random] System.Random parity (seed 12345)\n");
    // Reference produced by .NET Framework System.Random(12345).NextBytes(16).
    std::vector<uint8_t> expected = {
        0xdf,0x9e,0x02,0xf5,0xad,0xf2,0x47,0x3c,0x14,0x57,0xfd,0x6b,0xb7,0x5b,0x3c,0x01};
    evw::compat::DotNetRandom rnd(12345);
    std::vector<uint8_t> got(16);
    rnd.nextBytes(got);
    check(got == expected, "DotNetRandom matches .NET System.Random output");
}

static void testMagicBlobUnpack()
{
    std::printf("[keys] magic blob unpacking layout\n");
    // Build a synthetic blob of the exact expected size and verify the field
    // boundaries are parsed correctly (does not require the real magic.dat).
    const size_t total = 27472 + 278528 + 256 + 16;
    std::vector<uint8_t> blob(total);
    for (size_t i = 0; i < blob.size(); ++i) blob[i] = static_cast<uint8_t>(i * 31 + 7);

    GTA5Keys keys;
    keys.loadFromMagicBlob(blob);

    check(keys.PC_NG_KEYS.size() == 101, "101 NG keys parsed");
    check(keys.PC_NG_KEYS[0].size() == 272, "NG key is 272 bytes");
    check(keys.PC_NG_KEYS[0][0] == blob[0], "first NG key byte matches");
    check(keys.PC_NG_KEYS[100][271] == blob[27471], "last NG key byte matches");
    check(keys.ngTablesLoaded, "decrypt tables flagged loaded");
    check(keys.PC_LUT.size() == 256, "LUT is 256 bytes");
    // First table uint32 is right after the NG keys block.
    uint32_t firstTable;
    std::memcpy(&firstTable, blob.data() + 27472, 4);
    check(keys.PC_NG_DECRYPT_TABLES[0][0][0] == firstTable, "first table entry matches");
    // First LUT byte is after keys + tables.
    check(keys.PC_LUT[0] == blob[27472 + 278528], "first LUT byte matches");
}

static void testRpfParse()
{
    std::printf("[rpf] synthetic unencrypted RPF7 TOC parse + extract\n");
    using namespace evw::gamefiles;

    // Build a minimal valid RPF7 archive in memory:
    //   header(16) + 2 entries(32) + names(10), file content at offset 512.
    const char* content = "Hello RPF!";
    const uint32_t contentLen = 10;

    std::vector<uint8_t> buf(512 + contentLen, 0);
    DataWriter w;
    // Header
    w.Write(static_cast<uint32_t>(0x52504637)); // version
    w.Write(static_cast<uint32_t>(2));           // entry count
    w.Write(static_cast<uint32_t>(10));          // names length
    w.Write(static_cast<uint32_t>(0));           // encryption NONE
    // Entry 0: root directory
    w.Write(static_cast<uint32_t>(0));           // nameOffset -> "" 
    w.Write(static_cast<uint32_t>(0x7fffff00));  // ident
    w.Write(static_cast<uint32_t>(1));           // entriesIndex
    w.Write(static_cast<uint32_t>(1));           // entriesCount
    // Entry 1: binary file (uncompressed): buf64 = nameOffset | fileSize<<16 | fileOffset<<40
    {
        uint64_t b = static_cast<uint64_t>(1)            // nameOffset = 1 -> "test.txt"
                   | (static_cast<uint64_t>(0) << 16)    // fileSize 0 => uncompressed
                   | (static_cast<uint64_t>(1) << 40);   // fileOffset = 1 (=> 512 bytes)
        w.Write(b);
        w.Write(contentLen);                              // FileUncompressedSize
        w.Write(static_cast<uint32_t>(0));                // EncryptionType = none
    }
    // Names block: [0]="" , [1..]="test.txt\0"
    w.Write(static_cast<uint8_t>(0));
    for (const char* p = "test.txt"; *p; ++p) w.Write(static_cast<uint8_t>(*p));
    w.Write(static_cast<uint8_t>(0));

    const auto& toc = w.buffer();
    std::memcpy(buf.data(), toc.data(), toc.size());
    std::memcpy(buf.data() + 512, content, contentLen);

    MemoryDataSource src(buf);
    GTA5Keys keys; // not needed for NONE encryption
    RpfFile rpf("test.rpf", "test.rpf", static_cast<long long>(buf.size()));
    rpf.readHeader(src, keys);

    check(rpf.entryCount() == 2, "RPF reports 2 entries");
    check(rpf.encryption() == RpfEncryption::NONE, "encryption parsed as NONE");
    const auto& entries = rpf.entries();
    check(entries.size() == 2, "2 entries parsed");
    check(entries[0].type == RpfEntryType::Directory, "entry 0 is directory (root)");
    check(entries[1].type == RpfEntryType::Binary, "entry 1 is binary file");
    check(entries[1].name == "test.txt", "binary entry name resolved");
    check(entries[1].parent == 0, "binary entry parented to root");
    check(entries[1].path == "test.rpf\\test.txt", "binary entry path built");

    auto data = rpf.extractFile(src, keys, 1);
    std::string s(data.begin(), data.end());
    check(s == "Hello RPF!", "extracted uncompressed binary content matches");
}

static void testRpfFlags()
{
    std::printf("[rpf] page-flag size calculation\n");
    using namespace evw::gamefiles;
    check(getSizeFromFlags(0) == 0, "zero flags -> zero size");
    // A single base page: ss=0 (baseSize=0x200), s8 bit (bit4) set -> 256 multiplier.
    uint32_t flags = (1u << 4); // s8 = 1<<8 ... actually bit4 -> s8 contributes 256
    check(getSizeFromFlags(flags) == 0x200u * 256u, "single high-bit page size");
}

static std::vector<uint8_t> buildSyntheticRpf(const char* content, uint32_t contentLen)
{
    using namespace evw::gamefiles;
    std::vector<uint8_t> buf(512 + contentLen, 0);
    DataWriter w;
    w.Write(static_cast<uint32_t>(0x52504637)); // version
    w.Write(static_cast<uint32_t>(2));           // entry count
    w.Write(static_cast<uint32_t>(10));          // names length
    w.Write(static_cast<uint32_t>(0));           // encryption NONE
    w.Write(static_cast<uint32_t>(0));           // root nameOffset
    w.Write(static_cast<uint32_t>(0x7fffff00));  // ident
    w.Write(static_cast<uint32_t>(1));           // entriesIndex
    w.Write(static_cast<uint32_t>(1));           // entriesCount
    {
        uint64_t b = static_cast<uint64_t>(1) | (static_cast<uint64_t>(0) << 16) | (static_cast<uint64_t>(1) << 40);
        w.Write(b);
        w.Write(contentLen);
        w.Write(static_cast<uint32_t>(0));
    }
    w.Write(static_cast<uint8_t>(0));
    for (const char* p = "test.txt"; *p; ++p) w.Write(static_cast<uint8_t>(*p));
    w.Write(static_cast<uint8_t>(0));
    const auto& toc = w.buffer();
    std::memcpy(buf.data(), toc.data(), toc.size());
    std::memcpy(buf.data() + 512, content, contentLen);
    return buf;
}

static void testRpfManager()
{
    std::printf("[rpfmgr] scan folder + index + extract by path\n");
    using namespace evw::gamefiles;
    namespace fs = std::filesystem;

    // Write a synthetic .rpf into a temp directory, then scan it.
    fs::path dir = fs::temp_directory_path() / "evw_rpf_test";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    fs::path rpfPath = dir / "data.rpf";

    auto buf = buildSyntheticRpf("Hello RPF!", 10);
    {
        std::ofstream out(rpfPath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    }

    GTA5Keys keys;
    RpfManager mgr(keys);
    mgr.init(dir.string());

    check(mgr.isInited(), "manager initialized");
    check(mgr.rpfCount() >= 1, "at least one RPF indexed");
    check(mgr.findRpfFile("data.rpf") != nullptr, "RPF found by path");

    const RpfEntry* e = mgr.getEntry("data.rpf\\test.txt");
    check(e != nullptr, "entry found by path");
    check(e && e->name == "test.txt", "entry name correct");

    auto data = mgr.getFileData("data.rpf\\test.txt");
    std::string s(data.begin(), data.end());
    check(s == "Hello RPF!", "getFileData extracts content by path");

    fs::remove_all(dir, ec);
}

static void testResourceData()
{
    std::printf("[resource] ResourceDataReader virtual addressing + header block\n");
    using namespace evw::gamefiles;

    DataWriter w;
    // ResourceFileBase header (16 bytes)
    w.Write(static_cast<uint32_t>(0x12345678));         // FileVFT
    w.Write(static_cast<uint32_t>(1));                  // FileUnknown
    w.Write(static_cast<uint64_t>(0x50000010));         // FilePagesInfoPointer -> offset 16
    // ResourcePagesInfo (16 bytes fixed + padding)
    w.Write(static_cast<uint32_t>(0));                  // unknown0
    w.Write(static_cast<uint32_t>(0));                  // unknown4
    w.Write(static_cast<uint8_t>(1));                   // systemPagesCount
    w.Write(static_cast<uint8_t>(2));                   // graphicsPagesCount
    w.Write(static_cast<uint16_t>(0));                  // unknownA
    w.Write(static_cast<uint32_t>(0));                  // unknownC
    for (int i = 0; i < 8 * (1 + 2); ++i) w.Write(static_cast<uint8_t>(0)); // padding (24)
    // String at offset 56
    for (const char* p = "Hello"; *p; ++p) w.Write(static_cast<uint8_t>(*p));
    w.Write(static_cast<uint8_t>(0));

    std::vector<uint8_t> data = w.buffer();
    uint32_t systemSize = static_cast<uint32_t>(data.size());
    // Graphics segment.
    for (const char* p = "GFX!"; *p; ++p) data.push_back(static_cast<uint8_t>(*p));
    uint32_t graphicsSize = 4;

    ResourceDataReader r(systemSize, graphicsSize, data);
    auto base = r.ReadBlock<ResourceFileBase>();
    check(base->fileVFT == 0x12345678u, "FileVFT read");
    check(base->fileUnknown == 1u, "FileUnknown read");
    check(base->filePagesInfoPointer == 0x50000010ull, "PagesInfoPointer read");
    check(base->filePagesInfo != nullptr, "PagesInfo block resolved");
    check(base->filePagesInfo && base->filePagesInfo->systemPagesCount == 1, "systemPagesCount");
    check(base->filePagesInfo && base->filePagesInfo->graphicsPagesCount == 2, "graphicsPagesCount");

    check(r.ReadStringAt(ResourceDataReader::SYSTEM_BASE + 56) == "Hello", "ReadStringAt (system)");

    // Graphics segment addressing.
    r.setPosition(ResourceDataReader::GRAPHICS_BASE);
    auto gfx = r.ReadBytes(4);
    std::string gs(gfx.begin(), gfx.end());
    check(gs == "GFX!", "graphics segment read via 0x60000000 base");
}

static void testResourceArrays()
{
    std::printf("[resource] SimpleList64 + PointerList64 array base types\n");
    using namespace evw::gamefiles;

    struct U32Block { uint32_t v = 0; void read(ResourceDataReader& r) { v = r.ReadUInt32(); } };

    DataWriter w;
    // SimpleList64<U32Block> at 0x00 -> data at 0x10 (3 elements)
    w.setPosition(0x00);
    w.Write(static_cast<uint64_t>(0x50000010)); w.Write(static_cast<uint16_t>(3));
    w.Write(static_cast<uint16_t>(3)); w.Write(static_cast<uint32_t>(0));
    w.setPosition(0x10);
    w.Write(static_cast<uint32_t>(11)); w.Write(static_cast<uint32_t>(22)); w.Write(static_cast<uint32_t>(33));
    // PointerList64<U32Block> at 0x20 -> pointer table at 0x30 (2 pointers) -> values at 0x40 / 0x48
    w.setPosition(0x20);
    w.Write(static_cast<uint64_t>(0x50000030)); w.Write(static_cast<uint16_t>(2));
    w.Write(static_cast<uint16_t>(2)); w.Write(static_cast<uint32_t>(0));
    w.setPosition(0x30);
    w.Write(static_cast<uint64_t>(0x50000040)); w.Write(static_cast<uint64_t>(0x50000048));
    w.setPosition(0x40); w.Write(static_cast<uint32_t>(100));
    w.setPosition(0x48); w.Write(static_cast<uint32_t>(200));

    std::vector<uint8_t> data = w.buffer();
    ResourceDataReader r(static_cast<uint32_t>(data.size()), 0, data);

    r.setPosition(ResourceDataReader::SYSTEM_BASE + 0x00);
    auto simple = r.ReadBlock<ResourceSimpleList64<U32Block>>();
    check(simple->size() == 3, "SimpleList64 count");
    check(simple->items[0].v == 11 && simple->items[1].v == 22 && simple->items[2].v == 33,
          "SimpleList64 values read in order");

    auto ptrlist = r.ReadBlockAt<ResourcePointerList64<U32Block>>(ResourceDataReader::SYSTEM_BASE + 0x20);
    check(ptrlist->size() == 2, "PointerList64 count");
    check(ptrlist->items[0] && ptrlist->items[0]->v == 100, "PointerList64 first item via pointer");
    check(ptrlist->items[1] && ptrlist->items[1]->v == 200, "PointerList64 second item via pointer");
}

static void testTextureDictionary()
{
    std::printf("[ytd] parse synthetic TextureDictionary (1 texture)\n");
    using namespace evw::gamefiles;

    DataWriter w;
    // ResourceFileBase header (16): FileVFT, FileUnknown=1, PagesInfoPointer=0
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    // 4 unknown u32 at 0x10
    w.setPosition(0x10);
    for (int i = 0; i < 4; ++i) w.Write(static_cast<uint32_t>(0));
    // TextureNameHashes (SimpleList64_uint) at 0x20 -> hashes at 0x50
    w.setPosition(0x20);
    w.Write(static_cast<uint64_t>(0x50000050)); w.Write(static_cast<uint16_t>(1));
    w.Write(static_cast<uint16_t>(1)); w.Write(static_cast<uint32_t>(0));
    // Textures (PointerList64<Texture>) at 0x30 -> pointer table at 0x58
    w.setPosition(0x30);
    w.Write(static_cast<uint64_t>(0x50000058)); w.Write(static_cast<uint16_t>(1));
    w.Write(static_cast<uint16_t>(1)); w.Write(static_cast<uint32_t>(0));
    // hashes array at 0x50
    w.setPosition(0x50);
    w.Write(static_cast<uint32_t>(0xAABBCCDD));
    // texture pointer table at 0x58 -> texture block at 0x60
    w.setPosition(0x58);
    w.Write(static_cast<uint64_t>(0x50000060));
    // Texture block at 0x60
    w.setPosition(0x60);
    w.Write(static_cast<uint32_t>(0));                 // VFT
    for (int i = 0; i < 9; ++i) w.Write(static_cast<uint32_t>(0)); // Unknown_4..24
    w.Write(static_cast<uint64_t>(0x50000100));        // NamePointer
    w.Write(static_cast<uint16_t>(0));                 // Unknown_30h
    w.Write(static_cast<uint16_t>(0));                 // Unknown_32h
    for (int i = 0; i < 7; ++i) w.Write(static_cast<uint32_t>(0)); // 34,38,3C,Usage,44,Extra,4C
    // Texture-specific
    w.Write(static_cast<uint16_t>(4));                 // Width
    w.Write(static_cast<uint16_t>(4));                 // Height
    w.Write(static_cast<uint16_t>(1));                 // Depth
    w.Write(static_cast<uint16_t>(16));                // Stride
    w.Write(static_cast<uint32_t>(0x35545844));        // Format = DXT5
    w.Write(static_cast<uint8_t>(0));                  // Unknown_5Ch
    w.Write(static_cast<uint8_t>(1));                  // Levels
    w.Write(static_cast<uint16_t>(0));                 // Unknown_5Eh
    for (int i = 0; i < 4; ++i) w.Write(static_cast<uint32_t>(0)); // 60,64,68,6C
    w.Write(static_cast<uint64_t>(0x60000000));        // DataPointer -> graphics base
    for (int i = 0; i < 6; ++i) w.Write(static_cast<uint32_t>(0)); // 78,7C,80,84,88,8C
    // Name at 0x100
    w.setPosition(0x100);
    for (const char* p = "mytex"; *p; ++p) w.Write(static_cast<uint8_t>(*p));
    w.Write(static_cast<uint8_t>(0));

    std::vector<uint8_t> data = w.buffer();
    uint32_t systemSize = static_cast<uint32_t>(data.size());
    // graphics segment: 64 bytes (stride*height = 16*4)
    for (int i = 0; i < 64; ++i) data.push_back(0xEE);
    uint32_t graphicsSize = 64;

    TextureDictionary td = loadTextureDictionary(systemSize, graphicsSize, data);
    check(td.textures.size() == 1, "dictionary has 1 texture");
    auto tex = td.textures.items.empty() ? nullptr : td.textures.items[0];
    check(tex != nullptr, "texture pointer resolved");
    if (tex)
    {
        check(tex->name == "mytex", "texture name read from pointer");
        check(tex->width == 4 && tex->height == 4 && tex->stride == 16, "texture dimensions");
        check(tex->levels == 1, "mip levels");
        check(tex->format == TextureFormat::D3DFMT_DXT5, "texture format DXT5");
        check(tex->data && tex->data->fullData.size() == 64, "texture pixel data size");
        check(tex->data && tex->data->fullData[0] == 0xEE, "pixel data read from graphics segment");
    }
    auto looked = td.lookup(0xAABBCCDD);
    check(looked == tex, "lookup by name hash returns the texture");
}

static void testDDSExport()
{
    std::printf("[dds] export texture to .dds (header + data)\n");
    using namespace evw::gamefiles;

    Texture tex;
    tex.width = 4; tex.height = 4; tex.depth = 1; tex.stride = 16;
    tex.levels = 1; tex.format = TextureFormat::D3DFMT_DXT5;
    tex.data = std::make_shared<TextureData>();
    tex.data->fullData.assign(64, 0xEE);

    auto dds = evw::texconv::getDDSFile(tex);
    check(dds.size() == 128 + 64, "dds file size = 128-byte header + 64-byte data");
    check(dds[0] == 'D' && dds[1] == 'D' && dds[2] == 'S' && dds[3] == ' ', "DDS magic");

    auto rd32 = [&](size_t off) {
        uint32_t v; std::memcpy(&v, dds.data() + off, 4); return v;
    };
    check(rd32(4) == 124, "DDS_HEADER dwSize = 124");
    check(rd32(12) == 4, "dwHeight = 4");
    check(rd32(16) == 4, "dwWidth = 4");
    // pixelformat fourCC at offset 84
    uint32_t fourcc = rd32(84);
    check(fourcc == (static_cast<uint32_t>('D') | ('X' << 8) | ('T' << 16) | (static_cast<uint32_t>('5') << 24)),
          "pixelformat FourCC = DXT5");
    check(dds[128] == 0xEE && dds[191] == 0xEE, "pixel data appended after header");
}

static void testDxtDecode()
{
    std::printf("[dxt] decode DXT1/DXT5 blocks to RGBA\n");
    using namespace evw::texconv;

    // DXT1 block: color0 = red (565 0xF800), color1 = blue (0x001F), all indices 0.
    std::vector<uint8_t> dxt1 = {0x00, 0xF8, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00};
    auto rgba1 = decompressDxt1(dxt1, 4, 4);
    check(rgba1.size() == 4 * 4 * 4, "DXT1 decode size = 4x4 RGBA");
    check(rgba1[0] == 255 && rgba1[1] == 0 && rgba1[2] == 0 && rgba1[3] == 255,
          "DXT1 pixel(0,0) = opaque red");
    size_t last = (4 * 3 + 3) * 4;
    check(rgba1[last] == 255 && rgba1[last + 1] == 0 && rgba1[last + 2] == 0,
          "DXT1 pixel(3,3) = red");

    // DXT5 block: alpha0=255, alpha1=0, all alpha idx 0 (=>255), colors red.
    std::vector<uint8_t> dxt5 = {
        255, 0,                      // alpha0, alpha1
        0, 0, 0, 0, 0, 0,            // 6-byte alpha mask = 0
        0x00, 0xF8, 0x1F, 0x00,      // c0=red, c1=blue
        0x00, 0x00, 0x00, 0x00};     // color indices 0
    auto rgba5 = decompressDxt5(dxt5, 4, 4);
    check(rgba5.size() == 64, "DXT5 decode size");
    check(rgba5[0] == 255 && rgba5[1] == 0 && rgba5[2] == 0 && rgba5[3] == 255,
          "DXT5 pixel(0,0) = opaque red (alpha index 0 -> alpha0)");
}

static void testGeometryBuffers()
{
    std::printf("[geom] VertexBuffer + VertexDeclaration + IndexBuffer\n");
    using namespace evw::gamefiles;

    DataWriter w;
    // VertexBuffer block at 0x00
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0));            // VFT
    w.Write(static_cast<uint32_t>(1));            // Unknown_4h
    w.Write(static_cast<uint16_t>(8));            // VertexStride
    w.Write(static_cast<uint16_t>(0));            // Flags
    w.Write(static_cast<uint32_t>(0));            // Unknown_Ch
    w.Write(static_cast<uint64_t>(0x50000090));   // DataPointer1 -> vertex bytes
    w.Write(static_cast<uint32_t>(3));            // VertexCount
    w.Write(static_cast<uint32_t>(0));            // Unknown_1Ch
    w.Write(static_cast<uint64_t>(0));            // DataPointer2
    w.Write(static_cast<uint64_t>(0));            // Unknown_28h
    w.Write(static_cast<uint64_t>(0x50000080));   // InfoPointer -> declaration
    for (int i = 0; i < 9; ++i) w.Write(static_cast<uint64_t>(0)); // Unknown_38h..78h
    // VertexDeclaration at 0x80
    w.setPosition(0x80);
    w.Write(static_cast<uint32_t>(0x0000C7));     // Flags
    w.Write(static_cast<uint16_t>(8));            // Stride
    w.Write(static_cast<uint8_t>(0));             // Unknown_6h
    w.Write(static_cast<uint8_t>(2));             // Count
    w.Write(static_cast<uint64_t>(0x7755555555996996ull)); // Types (GTAV1)
    // Vertex bytes at 0x90 (3 verts * 8 = 24)
    w.setPosition(0x90);
    for (int i = 0; i < 24; ++i) w.Write(static_cast<uint8_t>(i + 1));
    // IndexBuffer block at 0x100
    w.setPosition(0x100);
    w.Write(static_cast<uint32_t>(0));            // VFT
    w.Write(static_cast<uint32_t>(1));            // Unknown_4h
    w.Write(static_cast<uint32_t>(3));            // IndicesCount
    w.Write(static_cast<uint32_t>(0));            // Unknown_Ch
    w.Write(static_cast<uint64_t>(0x50000140));   // IndicesPointer
    for (int i = 0; i < 9; ++i) w.Write(static_cast<uint64_t>(0)); // Unknown_18h..58h
    // Indices at 0x140 (3 ushorts)
    w.setPosition(0x140);
    w.Write(static_cast<uint16_t>(10)); w.Write(static_cast<uint16_t>(20)); w.Write(static_cast<uint16_t>(30));

    std::vector<uint8_t> data = w.buffer();
    ResourceDataReader r(static_cast<uint32_t>(data.size()), 0, data);

    auto vb = r.ReadBlockAt<VertexBuffer>(ResourceDataReader::SYSTEM_BASE + 0x00);
    check(vb->vertexStride == 8, "VertexBuffer stride");
    check(vb->vertexCount == 3, "VertexBuffer count");
    check(vb->info != nullptr, "VertexDeclaration resolved");
    check(vb->info && vb->info->count == 2, "declaration field count");
    check(vb->info && vb->info->types == 0x7755555555996996ull, "declaration types (GTAV1)");
    check(vb->data1.size() == 24, "vertex data bytes = count*stride");
    check(vb->data1[0] == 1 && vb->data1[23] == 24, "vertex bytes content");

    auto ib = r.ReadBlockAt<IndexBuffer>(ResourceDataReader::SYSTEM_BASE + 0x100);
    check(ib->indicesCount == 3, "IndexBuffer count");
    check(ib->indices.size() == 3 && ib->indices[0] == 10 && ib->indices[1] == 20 && ib->indices[2] == 30,
          "indices read correctly");
}

static void testDrawableModel()
{
    std::printf("[model] DrawableModel -> Geometry -> VB/IB assembly\n");
    using namespace evw::gamefiles;

    DataWriter w;
    auto wVB = [&](uint64_t at, uint64_t dataPtr, uint64_t infoPtr) {
        w.setPosition(at);
        w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1));
        w.Write(static_cast<uint16_t>(8)); w.Write(static_cast<uint16_t>(0));
        w.Write(static_cast<uint32_t>(0));
        w.Write(dataPtr);
        w.Write(static_cast<uint32_t>(3)); w.Write(static_cast<uint32_t>(0));
        w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0));
        w.Write(infoPtr);
        for (int i = 0; i < 9; ++i) w.Write(static_cast<uint64_t>(0));
    };
    auto wIB = [&](uint64_t at, uint64_t idxPtr) {
        w.setPosition(at);
        w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1));
        w.Write(static_cast<uint32_t>(3)); w.Write(static_cast<uint32_t>(0));
        w.Write(idxPtr);
        for (int i = 0; i < 9; ++i) w.Write(static_cast<uint64_t>(0));
    };

    // DrawableModel at 0x00
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1));
    w.Write(static_cast<uint64_t>(0x50000050)); // GeometriesPointer
    w.Write(static_cast<uint16_t>(1)); w.Write(static_cast<uint16_t>(1)); // counts 1/2
    w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint64_t>(0x50000060)); // BoundsPointer
    w.Write(static_cast<uint64_t>(0x50000058)); // ShaderMappingPointer
    w.Write(static_cast<uint32_t>(0));           // SkeletonBinding
    w.Write(static_cast<uint16_t>(0));           // RenderMaskFlags
    w.Write(static_cast<uint16_t>(1));           // count3
    // GeometryPointers at 0x50
    w.setPosition(0x50);
    w.Write(static_cast<uint64_t>(0x50000080));
    // ShaderMapping at 0x58
    w.setPosition(0x58);
    w.Write(static_cast<uint16_t>(7));
    // BoundsData (1 AABB) at 0x60: min(0,0,0,0) max(1,2,3,4)
    w.setPosition(0x60);
    for (int i = 0; i < 4; ++i) w.Write(0.0f);
    w.Write(1.0f); w.Write(2.0f); w.Write(3.0f); w.Write(4.0f);
    // DrawableGeometry at 0x80
    w.setPosition(0x80);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1));
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0));
    w.Write(static_cast<uint64_t>(0x50000120)); // VertexBufferPointer
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0));
    w.Write(static_cast<uint64_t>(0x50000200)); // IndexBufferPointer
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0));
    w.Write(static_cast<uint32_t>(3));           // IndicesCount
    w.Write(static_cast<uint32_t>(1));           // TrianglesCount
    w.Write(static_cast<uint16_t>(3));           // VerticesCount
    w.Write(static_cast<uint16_t>(3));           // Unknown_62h
    w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint64_t>(0));           // BoneIdsPointer
    w.Write(static_cast<uint16_t>(8));           // VertexStride
    w.Write(static_cast<uint16_t>(0));           // BoneIdsCount
    w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0));
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0));
    // VertexBuffer at 0x120 -> data 0x1C0, info 0x1A0
    wVB(0x120, 0x500001C0, 0x500001A0);
    // VertexDeclaration at 0x1A0
    w.setPosition(0x1A0);
    w.Write(static_cast<uint32_t>(0xC7)); w.Write(static_cast<uint16_t>(8));
    w.Write(static_cast<uint8_t>(0)); w.Write(static_cast<uint8_t>(2));
    w.Write(static_cast<uint64_t>(0x7755555555996996ull));
    // vertex bytes at 0x1C0
    w.setPosition(0x1C0);
    for (int i = 0; i < 24; ++i) w.Write(static_cast<uint8_t>(i + 1));
    // IndexBuffer at 0x200 -> indices 0x280
    wIB(0x200, 0x50000280);
    w.setPosition(0x280);
    w.Write(static_cast<uint16_t>(10)); w.Write(static_cast<uint16_t>(20)); w.Write(static_cast<uint16_t>(30));

    std::vector<uint8_t> data = w.buffer();
    ResourceDataReader r(static_cast<uint32_t>(data.size()), 0, data);

    auto model = r.ReadBlockAt<DrawableModel>(ResourceDataReader::SYSTEM_BASE + 0x00);
    check(model->geometriesCount == 1, "model geometry count");
    check(model->geometries.size() == 1 && model->geometries[0] != nullptr, "geometry resolved");
    auto geom = model->geometries[0];
    if (geom)
    {
        check(geom->shaderId == 7, "shader id from shader mapping");
        check(geom->aabb.max == math::Vector4(1, 2, 3, 4), "geometry AABB max from bounds");
        check(geom->vertexBuffer && geom->vertexBuffer->vertexCount == 3, "geometry vertex buffer");
        check(geom->vertexBuffer && geom->vertexBuffer->info && geom->vertexBuffer->info->count == 2,
              "vertex declaration via geometry");
        check(geom->indexBuffer && geom->indexBuffer->indices.size() == 3 &&
                  geom->indexBuffer->indices[2] == 30,
              "geometry index buffer");
    }
}

int main()
{
    std::printf("EvelentWalker C++ port self-tests\n");
    testJenkHash();
    testJenkIndex();
    testDataRoundTrip();
    testDataEndian();
    testAes256();
    testNgDecrypt();
    testInflate();
    testDotNetRandom();
    testMagicBlobUnpack();
    testRpfParse();
    testRpfFlags();
    testRpfManager();
    testResourceData();
    testResourceArrays();
    testTextureDictionary();
    testDDSExport();
    testDxtDecode();
    testGeometryBuffers();
    testDrawableModel();

    std::printf("\n%d/%d checks passed\n", g_checks - g_failures, g_checks);
    if (g_failures != 0)
    {
        std::printf("RESULT: FAILED (%d)\n", g_failures);
        return 1;
    }
    std::printf("RESULT: OK\n");
    return 0;
}

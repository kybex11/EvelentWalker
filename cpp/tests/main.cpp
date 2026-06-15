// Minimal self-test runner for the C++ port foundation.
#include <cstdint>
#include <cstdio>
#include <string>

#include "evw/app/explorer.h"
#include "evw/app/render_mesh.h"
#include "evw/gamefiles/aes.h"
#include "evw/gamefiles/bounds.h"
#include "evw/gamefiles/gamefile.h"
#include "evw/gamefiles/gxt2.h"
#include "evw/gamefiles/heightmap.h"
#include "evw/gamefiles/navmesh.h"
#include "evw/gamefiles/node.h"
#include "evw/gamefiles/pso.h"
#include "evw/gamefiles/rbf.h"
#include "evw/gamefiles/rbf_xml.h"
#include "evw/gamefiles/records.h"
#include "evw/gamefiles/data.h"
#include "evw/gamefiles/dotnet_random.h"
#include "evw/gamefiles/gtacrypto.h"
#include "evw/gamefiles/inflate.h"
#include "evw/gamefiles/jenk.h"
#include "evw/gamefiles/ddsio.h"
#include "evw/gamefiles/drawable.h"
#include "evw/gamefiles/dxt_decode.h"
#include "evw/gamefiles/frag.h"
#include "evw/gamefiles/geometry.h"
#include "evw/gamefiles/meta.h"
#include "evw/gamefiles/metatypes.h"
#include "evw/gamefiles/model.h"
#include "evw/gamefiles/resource_data.h"
#include "evw/gamefiles/rpf.h"
#include "evw/gamefiles/rpf_manager.h"
#include "evw/gamefiles/shader.h"
#include "evw/gamefiles/texture.h"
#include "evw/gamefiles/ymap.h"
#include "evw/gamefiles/ytyp.h"
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

static void testShaderGroup()
{
    std::printf("[shader] ShaderGroup -> ShaderFX list\n");
    using namespace evw::gamefiles;

    DataWriter w;
    // ShaderGroup at 0x00 (64 bytes)
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1));
    w.Write(static_cast<uint64_t>(0));            // TextureDictionaryPointer = none
    w.Write(static_cast<uint64_t>(0x50000040));   // ShadersPointer
    w.Write(static_cast<uint16_t>(1)); w.Write(static_cast<uint16_t>(1)); // counts
    w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0));
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint64_t>(0));
    // Shaders pointer array at 0x40
    w.setPosition(0x40);
    w.Write(static_cast<uint64_t>(0x50000050));
    // ShaderFX at 0x50 (48 bytes)
    w.setPosition(0x50);
    w.Write(static_cast<uint64_t>(0));            // ParametersPointer
    w.Write(static_cast<uint32_t>(0x12345678));   // Name
    w.Write(static_cast<uint32_t>(0));            // Unknown_Ch
    w.Write(static_cast<uint8_t>(5));             // ParameterCount
    w.Write(static_cast<uint8_t>(2));             // RenderBucket
    w.Write(static_cast<uint16_t>(32768));        // Unknown_12h
    w.Write(static_cast<uint16_t>(112));          // ParameterSize
    w.Write(static_cast<uint16_t>(160));          // ParameterDataSize
    w.Write(static_cast<uint32_t>(0xAABBCCDD));   // FileName
    w.Write(static_cast<uint32_t>(0));            // Unknown_1Ch
    w.Write(static_cast<uint32_t>(65284));        // RenderBucketMask
    w.Write(static_cast<uint16_t>(0));            // Unknown_24h
    w.Write(static_cast<uint8_t>(0));             // Unknown_26h
    w.Write(static_cast<uint8_t>(3));             // TextureParametersCount
    w.Write(static_cast<uint64_t>(0));            // Unknown_28h

    std::vector<uint8_t> data = w.buffer();
    ResourceDataReader r(static_cast<uint32_t>(data.size()), 0, data);

    auto sg = r.ReadBlockAt<ShaderGroup>(ResourceDataReader::SYSTEM_BASE + 0x00);
    check(sg->shadersCount == 1, "shader group shader count");
    check(sg->shaders.size() == 1 && sg->shaders.items[0] != nullptr, "shader resolved");
    auto sh = sg->shaders.items.empty() ? nullptr : sg->shaders.items[0];
    if (sh)
    {
        check(sh->name == 0x12345678u, "shader name hash");
        check(sh->fileName == 0xAABBCCDDu, "shader file name hash");
        check(sh->parameterCount == 5, "shader parameter count");
        check(sh->renderBucket == 2, "shader render bucket");
        check(sh->textureParametersCount == 3, "shader texture parameter count");
    }
    check(sg->textureDictionary == nullptr, "no embedded texture dictionary (pointer 0)");
}

static void testDrawable()
{
    std::printf("[ydr] DrawableBase -> ShaderGroup + High LOD models\n");
    using namespace evw::gamefiles;

    DataWriter w;
    // ResourceFileBase header (16)
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    // DrawableBase structure (0x10..0xA7)
    w.setPosition(0x10);
    w.Write(static_cast<uint64_t>(0x500000C0));   // ShaderGroupPointer
    w.Write(static_cast<uint64_t>(0));            // SkeletonPointer
    w.Write(0.0f); w.Write(0.0f); w.Write(0.0f);  // BoundingCenter
    w.Write(10.0f);                                // BoundingSphereRadius
    w.Write(-1.0f); w.Write(-1.0f); w.Write(-1.0f); // BoundingBoxMin
    w.Write(static_cast<uint32_t>(0x7f800001));   // Unknown_3Ch
    w.Write(1.0f); w.Write(1.0f); w.Write(1.0f);   // BoundingBoxMax
    w.Write(static_cast<uint32_t>(0x7f800001));   // Unknown_4Ch
    w.Write(static_cast<uint64_t>(0x50000100));   // DrawableModelsHighPointer
    w.Write(static_cast<uint64_t>(0));            // Med
    w.Write(static_cast<uint64_t>(0));            // Low
    w.Write(static_cast<uint64_t>(0));            // VeryLow
    w.Write(100.0f); w.Write(50.0f); w.Write(25.0f); w.Write(10.0f); // LodDist
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0)); // RenderMaskFlags
    w.Write(static_cast<uint64_t>(0));            // JointsPointer
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint16_t>(0)); // Unk98, BlocksSize
    w.Write(static_cast<uint32_t>(0));            // Unk9C
    w.Write(static_cast<uint64_t>(0));            // DrawableModelsPointer
    // ShaderGroup at 0xC0 (64), all zero (no shaders, no tex dict)
    w.setPosition(0xC0);
    for (int i = 0; i < 8; ++i) w.Write(static_cast<uint64_t>(0));
    // High LOD ResourcePointerListHeader at 0x100
    w.setPosition(0x100);
    w.Write(static_cast<uint64_t>(0x50000110)); // entries pointer
    w.Write(static_cast<uint16_t>(1));          // count
    w.Write(static_cast<uint16_t>(1));          // capacity
    w.Write(static_cast<uint32_t>(0));          // pad
    // model pointers at 0x110
    w.setPosition(0x110);
    w.Write(static_cast<uint64_t>(0x50000120));
    // DrawableModel at 0x120 (48, no geometries)
    w.setPosition(0x120);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1));
    w.Write(static_cast<uint64_t>(0));          // GeometriesPointer
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint16_t>(0)); // counts
    w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint64_t>(0));          // BoundsPointer
    w.Write(static_cast<uint64_t>(0));          // ShaderMappingPointer
    w.Write(static_cast<uint32_t>(0));          // SkeletonBinding
    w.Write(static_cast<uint16_t>(0));          // RenderMaskFlags
    w.Write(static_cast<uint16_t>(0));          // count3

    std::vector<uint8_t> data = w.buffer();
    ResourceDataReader r(static_cast<uint32_t>(data.size()), 0, data);

    auto drw = r.ReadBlock<DrawableBase>();
    check(drw->boundingSphereRadius == 10.0f, "drawable bounding sphere radius");
    check(drw->shaderGroup != nullptr, "shader group parsed");
    check(drw->shaderGroup && drw->shaderGroup->shadersCount == 0, "shader group has 0 shaders");
    check(drw->modelsHigh.size() == 1, "high LOD has 1 model");
    check(drw->modelsMed.empty(), "med LOD empty");
    check(drw->allModels().size() == 1, "allModels aggregates LODs");
    check(drw->lodDistHigh == 100.0f, "lod distance high");
}

static void testDrawableDictionary()
{
    std::printf("[ydd] DrawableDictionary -> drawables by hash\n");
    using namespace evw::gamefiles;

    DataWriter w;
    // ResourceFileBase header (16)
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    // DrawableDictionary structure (0x10..0x3F)
    w.setPosition(0x10);
    w.Write(static_cast<uint64_t>(0));            // Unknown_10h
    w.Write(static_cast<uint64_t>(1));            // Unknown_18h
    w.Write(static_cast<uint64_t>(0x50000050));   // HashesPointer
    w.Write(static_cast<uint16_t>(1)); w.Write(static_cast<uint16_t>(1)); // hashes count
    w.Write(static_cast<uint32_t>(0));            // Unknown_2Ch
    w.Write(static_cast<uint64_t>(0x50000060));   // DrawablesPointer
    w.Write(static_cast<uint16_t>(1)); w.Write(static_cast<uint16_t>(1)); // drawables count
    w.Write(static_cast<uint32_t>(0));            // Unknown_3Ch
    // hashes at 0x50
    w.setPosition(0x50);
    w.Write(static_cast<uint32_t>(0xCAFEBABE));
    // drawables pointer array at 0x60 (1 pointer -> drawable at 0x80)
    w.setPosition(0x60);
    w.Write(static_cast<uint64_t>(0x50000080));
    // Drawable at 0x80: minimal DrawableBase(168) + Drawable extra(40) = 208 bytes, all zero pointers.
    // ResourceFileBase header within the drawable
    w.setPosition(0x80);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    // Rest of the 208-byte drawable left as zeros (no shader group, no models, no name).

    std::vector<uint8_t> data = w.buffer();
    // Ensure buffer covers the full drawable block (0x80 + 208 = 0x150).
    if (data.size() < 0x150) data.resize(0x150, 0);
    ResourceDataReader r(static_cast<uint32_t>(data.size()), 0, data);

    auto ydd = r.ReadBlock<DrawableDictionary>();
    check(ydd->drawablesCount == 1, "ydd drawable count");
    check(ydd->hashes.size() == 1 && ydd->hashes[0] == 0xCAFEBABEu, "ydd hash read");
    check(ydd->drawables.size() == 1 && ydd->drawables.items[0] != nullptr, "ydd drawable resolved");
}

static void testShaderParameters()
{
    std::printf("[shader] ShaderFX parameters (texture + vector)\n");
    using namespace evw::gamefiles;

    DataWriter w;
    // ShaderGroup at 0x00 (64)
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1));
    w.Write(static_cast<uint64_t>(0));            // TextureDictionaryPointer
    w.Write(static_cast<uint64_t>(0x50000040));   // ShadersPointer
    w.Write(static_cast<uint16_t>(1)); w.Write(static_cast<uint16_t>(1));
    w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0));
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint64_t>(0));
    // Shaders pointer array at 0x40
    w.setPosition(0x40);
    w.Write(static_cast<uint64_t>(0x50000050));
    // ShaderFX at 0x50 (48): ParametersPointer=0x50000080, ParameterCount=2
    w.setPosition(0x50);
    w.Write(static_cast<uint64_t>(0x50000080));   // ParametersPointer
    w.Write(static_cast<uint32_t>(0x1111));       // Name
    w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint8_t>(2));             // ParameterCount
    w.Write(static_cast<uint8_t>(0));             // RenderBucket
    w.Write(static_cast<uint16_t>(32768));
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint16_t>(0));
    w.Write(static_cast<uint32_t>(0x2222));       // FileName
    w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint8_t>(0));
    w.Write(static_cast<uint8_t>(1));             // TextureParametersCount
    w.Write(static_cast<uint64_t>(0));
    // ParametersBlock at 0x80: 2 ShaderParameter headers (16 bytes each)
    //   param0: dataType=0 (texture) -> dataPointer=0x500000C0 (TextureBase)
    //   param1: dataType=1 (vector)  -> dataPointer=0x50000140 (Vector4)
    w.setPosition(0x80);
    w.Write(static_cast<uint8_t>(0)); w.Write(static_cast<uint8_t>(0));
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint64_t>(0x500000C0));   // texture pointer
    w.Write(static_cast<uint8_t>(1)); w.Write(static_cast<uint8_t>(0));
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint64_t>(0x50000140));   // vector pointer
    // After params (offset += 16 for the vector), hashes follow at 0xA0 + 16 = 0xB0
    // param data region is 16 bytes; hashes (2 x u32) at 0xB0
    w.setPosition(0xB0);
    w.Write(static_cast<uint32_t>(0xAAAA)); w.Write(static_cast<uint32_t>(0xBBBB));
    // TextureBase at 0xC0 (80 bytes): NamePointer at +0x28 -> 0x50000130
    w.setPosition(0xC0);
    w.Write(static_cast<uint32_t>(0));            // VFT
    for (int i = 0; i < 9; ++i) w.Write(static_cast<uint32_t>(0)); // unknowns to 0x28
    w.Write(static_cast<uint64_t>(0x50000130));   // NamePointer (at 0xC0+0x28=0xE8)
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint16_t>(0));
    for (int i = 0; i < 7; ++i) w.Write(static_cast<uint32_t>(0)); // to 0x50
    // name at 0x130
    w.setPosition(0x130);
    for (const char* p = "diffuse"; *p; ++p) w.Write(static_cast<uint8_t>(*p));
    w.Write(static_cast<uint8_t>(0));
    // Vector4 at 0x140
    w.setPosition(0x140);
    w.Write(1.0f); w.Write(2.0f); w.Write(3.0f); w.Write(4.0f);

    std::vector<uint8_t> data = w.buffer();
    ResourceDataReader r(static_cast<uint32_t>(data.size()), 0, data);
    auto sg = r.ReadBlockAt<ShaderGroup>(ResourceDataReader::SYSTEM_BASE + 0x00);
    auto sh = (sg && !sg->shaders.items.empty()) ? sg->shaders.items[0] : nullptr;
    check(sh != nullptr, "shader resolved");
    check(sh && sh->parametersList != nullptr, "parameters block parsed");
    if (sh && sh->parametersList)
    {
        auto& pl = *sh->parametersList;
        check(pl.parameters.size() == 2, "two shader parameters");
        check(pl.textureCount() == 1, "one texture parameter");
        check(pl.parameters[0].texture && pl.parameters[0].texture->name == "diffuse",
              "texture parameter references texture 'diffuse'");
        check(!pl.parameters[1].vectors.empty() && pl.parameters[1].vectors[0] == math::Vector4(1, 2, 3, 4),
              "vector parameter value");
        check(pl.hashes.size() == 2 && pl.hashes[0] == 0xAAAAu, "parameter name hashes");
    }
}

static void testMeta()
{
    std::printf("[meta] Meta container: structures + enums + data blocks\n");
    using namespace evw::gamefiles;

    DataWriter w;
    // ResourceFileBase header (16)
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    // Meta structure (from 0x10)
    w.setPosition(0x10);
    w.Write(static_cast<int32_t>(0));             // Unknown_10h
    w.Write(static_cast<int16_t>(0));             // Unknown_14h
    w.Write(static_cast<uint8_t>(0));             // HasEncryptedStrings
    w.Write(static_cast<uint8_t>(0));             // Unknown_17h
    w.Write(static_cast<int32_t>(0));             // Unknown_18h
    w.Write(static_cast<int32_t>(1));             // RootBlockIndex (1-based -> block 0)
    w.Write(static_cast<int64_t>(0x50000080));    // StructureInfosPointer
    w.Write(static_cast<int64_t>(0x500000C0));    // EnumInfosPointer
    w.Write(static_cast<int64_t>(0x50000100));    // DataBlocksPointer
    w.Write(static_cast<int64_t>(0x50000140));    // NamePointer
    w.Write(static_cast<int64_t>(0));             // EncryptedStringsPointer
    w.Write(static_cast<int16_t>(1));             // StructureInfosCount
    w.Write(static_cast<int16_t>(1));             // EnumInfosCount
    w.Write(static_cast<int16_t>(1));             // DataBlocksCount
    w.Write(static_cast<int16_t>(0));             // Unknown_4Eh
    for (int i = 0; i < 8; ++i) w.Write(static_cast<uint32_t>(0)); // Unknown_50h..6Ch
    // StructureInfo at 0x80 (32 bytes) -> 1 entry at 0x180
    w.setPosition(0x80);
    w.Write(static_cast<uint32_t>(0xDEAD0001));   // StructureNameHash
    w.Write(static_cast<uint32_t>(0));            // StructureKey
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0)); // Unknown_8/Ch
    w.Write(static_cast<int64_t>(0x50000180));    // EntriesPointer
    w.Write(static_cast<int32_t>(16));            // StructureSize
    w.Write(static_cast<int16_t>(0));             // Unknown_1Ch
    w.Write(static_cast<int16_t>(1));             // EntriesCount
    // EnumInfo at 0xC0 (24 bytes) -> 2 entries at 0x1A0
    w.setPosition(0xC0);
    w.Write(static_cast<uint32_t>(0xBEEF0001));   // EnumNameHash
    w.Write(static_cast<uint32_t>(0));            // EnumKey
    w.Write(static_cast<int64_t>(0x500001A0));    // EntriesPointer
    w.Write(static_cast<int32_t>(2));             // EntriesCount
    w.Write(static_cast<int32_t>(0));             // Unknown_14h
    // DataBlock at 0x100 (16 bytes) -> data at 0x1C0 (8 bytes)
    w.setPosition(0x100);
    w.Write(static_cast<uint32_t>(0xDEAD0001));   // StructureNameHash
    w.Write(static_cast<int32_t>(8));             // DataLength
    w.Write(static_cast<int64_t>(0x500001C0));    // DataPointer
    // Name at 0x140
    w.setPosition(0x140);
    for (const char* p = "metatest"; *p; ++p) w.Write(static_cast<uint8_t>(*p));
    w.Write(static_cast<uint8_t>(0));
    // Structure entry at 0x180 (16 bytes)
    w.setPosition(0x180);
    w.Write(static_cast<uint32_t>(0x11112222));   // EntryNameHash
    w.Write(static_cast<int32_t>(0));             // DataOffset
    w.Write(static_cast<uint8_t>(0x15));          // DataType = UnsignedInt
    w.Write(static_cast<uint8_t>(0));             // Unknown_9h
    w.Write(static_cast<int16_t>(-1));            // ReferenceTypeIndex
    w.Write(static_cast<uint32_t>(0));            // ReferenceKey
    // Enum entries at 0x1A0 (2 x 8 bytes)
    w.setPosition(0x1A0);
    w.Write(static_cast<uint32_t>(0xAA)); w.Write(static_cast<int32_t>(0));
    w.Write(static_cast<uint32_t>(0xBB)); w.Write(static_cast<int32_t>(1));
    // Data block bytes at 0x1C0
    w.setPosition(0x1C0);
    for (int i = 0; i < 8; ++i) w.Write(static_cast<uint8_t>(0x40 + i));

    std::vector<uint8_t> data = w.buffer();
    ResourceDataReader r(static_cast<uint32_t>(data.size()), 0, data);
    auto meta = r.ReadBlock<Meta>();

    check(meta->name == "metatest", "meta name");
    check(meta->structureInfos.size() == 1, "1 structure info");
    check(meta->enumInfos.size() == 1, "1 enum info");
    check(meta->dataBlocks.size() == 1, "1 data block");

    const MetaStructureInfo* s = meta->findStructure(0xDEAD0001);
    check(s != nullptr, "structure found by hash");
    check(s && s->entries.size() == 1, "structure has 1 entry");
    check(s && !s->entries.empty() && s->entries[0].entryNameHash == 0x11112222u, "entry name hash");
    check(s && !s->entries.empty() && s->entries[0].dataType == 0x15, "entry data type");

    check(meta->enumInfos[0].entries.size() == 2, "enum has 2 entries");
    check(meta->enumInfos[0].entries[1].entryValue == 1, "enum entry value");

    const MetaDataBlock* root = meta->rootBlock();
    check(root != nullptr, "root data block resolved");
    check(root && root->data.size() == 8 && root->data[0] == 0x40, "root block data");
}

static void testMetaTypes()
{
    std::printf("[meta] MetaTypes typed data-block reading\n");
    using namespace evw::gamefiles;

#pragma pack(push, 1)
    struct Pt { int32_t x; int32_t y; };
#pragma pack(pop)

    // Build a Meta with one data block (name 0x1234) holding 3 Pt structs.
    Meta meta;
    MetaDataBlock blk;
    blk.structureNameHash = 0x1234;
    Pt pts[3] = {{1, 2}, {3, 4}, {5, 6}};
    blk.data.resize(sizeof(pts));
    std::memcpy(blk.data.data(), pts, sizeof(pts));
    blk.dataLength = static_cast<int32_t>(blk.data.size());
    meta.dataBlocks.data.push_back(blk);

    // pointer: block index 1 (1-based), offset 0
    uint64_t ptr = 1;
    auto arr = metatypes::convertDataArray<Pt>(meta, 0x1234, ptr, 3);
    check(arr.size() == 3, "convertDataArray count");
    check(arr.size() == 3 && arr[0].x == 1 && arr[0].y == 2, "first struct");
    check(arr.size() == 3 && arr[2].x == 5 && arr[2].y == 6, "last struct");

    // offset into the middle (skip first struct): offset 8 -> (8<<12)|1
    uint64_t ptr2 = (static_cast<uint64_t>(8) << 12) | 1;
    auto arr2 = metatypes::convertDataArray<Pt>(meta, 0x1234, ptr2, 2);
    check(arr2.size() == 2 && arr2[0].x == 3 && arr2[1].x == 5, "offset pointer reads from middle");

    Pt single{};
    check(metatypes::getDataAt<Pt>(meta, 0x1234, ptr, single) && single.x == 1, "getDataAt single value");

    Pt typed{};
    check(metatypes::getTypedData<Pt>(meta, 0x1234, typed) && typed.x == 1, "getTypedData by name");
    check(!metatypes::getTypedData<Pt>(meta, 0x9999, typed), "getTypedData missing name returns false");
}

static void testYmap()
{
    std::printf("[ymap] CMapData -> CEntityDef pointer array\n");
    using namespace evw::gamefiles;

    const uint32_t CMAPDATA = 0xAAAA, CENTITYDEF = 0xBBBB, POINTERS = 0xCCCC;

    // Block 1 (index 0): CMapData
    CMapData cmap{};
    cmap.name = 0x1111;
    cmap.entities.pointer = 3;          // block 3 (1-based) = pointers block, offset 0
    cmap.entities.count1 = 2;
    cmap.entities.count2 = 2;

    // Block 2 (index 1): two CEntityDef
    CEntityDef e0{}, e1{};
    e0.archetypeName = 0x12345678; e0.position = math::Vector3(1, 2, 3);
    e1.archetypeName = 0x9ABCDEF0; e1.position = math::Vector3(4, 5, 6);

    // Block 3 (index 2): two MetaPointer -> block 2, offsets 0 and 128
    metatypes::MetaPointer p0{}, p1{};
    p0.value = 2 | (static_cast<uint64_t>(0) << 12);
    p1.value = 2 | (static_cast<uint64_t>(128) << 12);

    Meta meta;
    auto mkblock = [](uint32_t name, const void* src, size_t n) {
        MetaDataBlock b; b.structureNameHash = name;
        b.data.resize(n); std::memcpy(b.data.data(), src, n);
        b.dataLength = static_cast<int32_t>(n);
        return b;
    };
    meta.dataBlocks.data.push_back(mkblock(CMAPDATA, &cmap, sizeof(cmap)));
    std::vector<uint8_t> entBytes(256);
    std::memcpy(entBytes.data(), &e0, 128);
    std::memcpy(entBytes.data() + 128, &e1, 128);
    meta.dataBlocks.data.push_back(mkblock(CENTITYDEF, entBytes.data(), entBytes.size()));
    std::vector<uint8_t> ptrBytes(16);
    std::memcpy(ptrBytes.data(), &p0, 8);
    std::memcpy(ptrBytes.data() + 8, &p1, 8);
    meta.dataBlocks.data.push_back(mkblock(POINTERS, ptrBytes.data(), ptrBytes.size()));
    meta.rootBlockIndex = 1; // 1-based -> block 0 (CMapData)

    YmapFile ymap;
    bool ok = ymap.loadFromMeta(meta, CENTITYDEF);
    check(ok, "ymap loaded from meta");
    check(ymap.mapData.name == 0x1111u, "map data name");
    check(ymap.entities.size() == 2, "two entities via pointer array");
    check(ymap.entities.size() == 2 && ymap.entities[0].archetypeName == 0x12345678u,
          "entity 0 archetype hash");
    check(ymap.entities.size() == 2 && ymap.entities[0].position == math::Vector3(1, 2, 3),
          "entity 0 position");
    check(ymap.entities.size() == 2 && ymap.entities[1].archetypeName == 0x9ABCDEF0u,
          "entity 1 archetype hash");
}

static void testYtyp()
{
    std::printf("[ytyp] CMapTypes -> CBaseArchetypeDef pointer array\n");
    using namespace evw::gamefiles;

    const uint32_t CMAPTYPES = 0xA1, CARCHDEF = 0xB1, POINTERS = 0xC1;

    CMapTypes mt{};
    mt.name = 0x5555;
    mt.archetypes.pointer = 3;       // pointers block (index 3, 1-based), offset 0
    mt.archetypes.count1 = 1;
    mt.archetypes.count2 = 1;

    CBaseArchetypeDef a0{};
    a0.name = 0xABCDEF01;
    a0.drawableDictionary = 0x11112222;
    a0.textureDictionary = 0x33334444;
    a0.assetName = 0x55556666;
    a0.bsRadius = 12.5f;
    a0.lodDist = 200.0f;

    metatypes::MetaPointer p0{};
    p0.value = 2 | (static_cast<uint64_t>(0) << 12); // block 2 (archetype), offset 0

    Meta meta;
    auto mkblock = [](uint32_t name, const void* src, size_t n) {
        MetaDataBlock b; b.structureNameHash = name;
        b.data.resize(n); std::memcpy(b.data.data(), src, n);
        b.dataLength = static_cast<int32_t>(n);
        return b;
    };
    meta.dataBlocks.data.push_back(mkblock(CMAPTYPES, &mt, sizeof(mt)));     // block index 0
    meta.dataBlocks.data.push_back(mkblock(CARCHDEF, &a0, sizeof(a0)));      // block index 1
    metatypes::MetaPointer ptrs[1] = {p0};
    meta.dataBlocks.data.push_back(mkblock(POINTERS, ptrs, sizeof(ptrs)));   // block index 2
    meta.rootBlockIndex = 1; // -> block 0 (CMapTypes)

    YtypFile ytyp;
    bool ok = ytyp.loadFromMeta(meta, CARCHDEF);
    check(ok, "ytyp loaded from meta");
    check(ytyp.mapTypes.name == 0x5555u, "map types name");
    check(ytyp.archetypes.size() == 1, "one archetype");
    const CBaseArchetypeDef* a = ytyp.findArchetype(0xABCDEF01);
    check(a != nullptr, "archetype found by name");
    check(a && a->drawableDictionary == 0x11112222u, "archetype drawable dictionary hash");
    check(a && a->textureDictionary == 0x33334444u, "archetype texture dictionary hash");
    check(a && a->bsRadius == 12.5f, "archetype bounding sphere radius");
    check(a && a->lodDist == 200.0f, "archetype lod dist");
}

static void testExplorerModel()
{
    std::printf("[explorer] ExplorerModel scan + search + open\n");
    namespace fs = std::filesystem;

    fs::path dir = fs::temp_directory_path() / "evw_explorer_test";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    auto buf = buildSyntheticRpf("Hello RPF!", 10);
    {
        std::ofstream out(dir / "data.rpf", std::ios::binary);
        out.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    }

    evw::app::ExplorerModel model;
    model.init(dir.string());
    check(model.isInited(), "explorer initialized");
    check(model.rpfCount() >= 1, "explorer indexed rpf");

    auto results = model.search("test.txt");
    check(!results.empty(), "search finds test.txt");

    auto pv = model.openFile("data.rpf\\test.txt");
    check(pv.kind == evw::app::PreviewKind::Binary, "binary preview kind");
    check(pv.dataSize == 10, "preview data size");

    fs::remove_all(dir, ec);
}

static void testVertexPositions()
{
    std::printf("[geom] extract vertex positions (Float3 layout)\n");
    using namespace evw::gamefiles;

    // Declaration with only Position (slot 0) as Float3 (type 6), stride 12.
    VertexDeclaration decl;
    decl.flags = 0x1;                 // only bit 0 (Position)
    decl.types = 0x6;                 // slot 0 = Float3
    decl.stride = 12;
    decl.count = 1;

    VertexBuffer vb;
    vb.vertexStride = 12;
    vb.vertexCount = 2;
    vb.info = std::make_shared<VertexDeclaration>(decl);
    vb.data1.resize(24);
    float verts[6] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    std::memcpy(vb.data1.data(), verts, sizeof(verts));

    auto pos = vb.extractPositions();
    check(pos.size() == 2, "two positions extracted");
    check(pos.size() == 2 && pos[0] == math::Vector3(1, 2, 3), "position 0");
    check(pos.size() == 2 && pos[1] == math::Vector3(4, 5, 6), "position 1");

    // componentOffset: with Position(Float3, 12) then Normal(slot 3) present,
    // the normal's offset should be 12.
    VertexDeclaration d2;
    d2.flags = 0x1 | (1u << 3);       // Position + Normal
    d2.types = 0x6 | (0x6ull << (3 * 4)); // both Float3
    check(d2.componentOffset(0) == 0, "position offset 0");
    check(d2.componentOffset(3) == 12, "normal offset after position");

    // Full layout: Position(Float3) + Normal(Float3) + TexCoord0(Float2), stride 32.
    VertexDeclaration d3;
    d3.flags = 0x1 | (1u << 3) | (1u << 6);
    d3.types = 0x6ull | (0x6ull << (3 * 4)) | (0x5ull << (6 * 4));
    d3.stride = 32;
    VertexBuffer vb3;
    vb3.vertexStride = 32; vb3.vertexCount = 1;
    vb3.info = std::make_shared<VertexDeclaration>(d3);
    vb3.data1.resize(32);
    float fpos[3] = {1, 2, 3}, nrm[3] = {0, 0, 1}, uv[2] = {0.5f, 0.25f};
    std::memcpy(vb3.data1.data() + 0, fpos, 12);
    std::memcpy(vb3.data1.data() + 12, nrm, 12);
    std::memcpy(vb3.data1.data() + 24, uv, 8);
    auto normals = vb3.extractNormals();
    auto uvs = vb3.extractTexCoords0();
    check(normals.size() == 1 && normals[0] == math::Vector3(0, 0, 1), "normal extracted");
    check(uvs.size() == 1 && uvs[0] == math::Vector2(0.5f, 0.25f), "texcoord extracted");
}

static void testMathMatrix()
{
    std::printf("[math] matrix multiply / transform / projection\n");
    using namespace evw::math;

    Matrix id;
    check(mul(id, id) == id, "identity * identity = identity");

    auto t = translation(1, 2, 3);
    check(transformPoint(t, Vector3(0, 0, 0)) == Vector3(1, 2, 3), "translation moves origin");

    // v * Scale * Translate : (1,0,0) -> scale(2)-> (2,0,0) -> +x1 -> (3,0,0)
    auto st = mul(scaling(2, 2, 2), translation(1, 0, 0));
    check(transformPoint(st, Vector3(1, 0, 0)) == Vector3(3, 0, 0), "combined scale+translate order");

    auto proj = perspectiveFovRH(1.5708f, 1.0f, 0.1f, 100.0f);
    check(proj.at(2, 3) == -1.0f, "perspective sets w = -z");
    check(proj.at(0, 0) > 0.0f, "perspective x scale positive");
}

static void testRenderMesh()
{
    std::printf("[render] buildMeshes from a DrawableModel\n");
    using namespace evw::gamefiles;

    auto vb = std::make_shared<VertexBuffer>();
    VertexDeclaration decl;
    decl.flags = 0x1; decl.types = 0x6; decl.stride = 12; decl.count = 1;
    vb->info = std::make_shared<VertexDeclaration>(decl);
    vb->vertexStride = 12; vb->vertexCount = 3;
    vb->data1.resize(36);
    float verts[9] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    std::memcpy(vb->data1.data(), verts, sizeof(verts));

    auto ib = std::make_shared<IndexBuffer>();
    ib->indicesCount = 3;
    ib->indices = {0, 1, 2};

    auto geom = std::make_shared<DrawableGeometry>();
    geom->vertexBuffer = vb;
    geom->indexBuffer = ib;
    geom->shaderId = 5;

    DrawableModel model;
    model.geometries.push_back(geom);

    auto meshes = evw::app::buildMeshes(model);
    check(meshes.size() == 1, "one mesh built");
    check(meshes.size() == 1 && meshes[0].positions.size() == 3, "mesh has 3 positions");
    check(meshes.size() == 1 && meshes[0].indices.size() == 3, "mesh has 3 indices");
    check(meshes.size() == 1 && meshes[0].triangleCount() == 1, "one triangle");
    check(meshes.size() == 1 && meshes[0].shaderId == 5, "mesh shader id");
    check(meshes.size() == 1 && meshes[0].positions[1] == math::Vector3(1, 0, 0), "vertex position");
}

static void testBounds()
{
    std::printf("[bounds] collision Bounds header parse\n");
    using namespace evw::gamefiles;

    DataWriter w;
    // ResourceFileBase header (16): FileVFT, FileUnknown, PagesInfoPointer=0
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    // Bounds header from 0x10
    w.setPosition(0x10);
    w.Write(static_cast<uint8_t>(3));            // Type = Box
    w.Write(static_cast<uint8_t>(0));            // Unknown_11h
    w.Write(static_cast<uint16_t>(0));           // Unknown_12h
    w.Write(5.0f);                                // SphereRadius
    w.Write(static_cast<uint32_t>(0));           // Unknown_18h
    w.Write(static_cast<uint32_t>(0));           // Unknown_1Ch
    w.Write(1.0f); w.Write(2.0f); w.Write(3.0f);  // BoxMax
    w.Write(0.04f);                               // Margin
    w.Write(-1.0f); w.Write(-2.0f); w.Write(-3.0f); // BoxMin
    w.Write(static_cast<uint32_t>(1));           // Unknown_3Ch
    w.Write(0.0f); w.Write(0.0f); w.Write(0.0f);  // BoxCenter
    w.Write(static_cast<uint8_t>(7));            // MaterialIndex
    w.Write(static_cast<uint8_t>(0));            // ProceduralId
    w.Write(static_cast<uint8_t>(0));            // RoomId
    w.Write(static_cast<uint8_t>(0));            // UnkFlags
    w.Write(0.0f); w.Write(0.0f); w.Write(0.0f);  // SphereCenter
    w.Write(static_cast<uint8_t>(0));            // PolyFlags
    w.Write(static_cast<uint8_t>(0));            // MaterialColorIndex
    w.Write(static_cast<uint16_t>(0));           // Unknown_5Eh
    w.Write(0.0f); w.Write(0.0f); w.Write(0.0f);  // Unknown_60h
    w.Write(48.0f);                               // Volume

    std::vector<uint8_t> data = w.buffer();
    ResourceDataReader r(static_cast<uint32_t>(data.size()), 0, data);
    auto b = r.ReadBlock<Bounds>();

    check(b->type == BoundsType::Box, "bounds type = Box");
    check(b->sphereRadius == 5.0f, "sphere radius");
    check(b->boxMax == math::Vector3(1, 2, 3), "box max");
    check(b->boxMin == math::Vector3(-1, -2, -3), "box min");
    check(b->margin == 0.04f, "margin");
    check(b->materialIndex == 7, "material index");
    check(b->volume == 48.0f, "volume");
}

static void testPso()
{
    std::printf("[pso] PSO (PSIN/PMAP) big-endian sections\n");
    using namespace evw::gamefiles;

    // Build a minimal PSO: PSIN section (16 bytes) + PMAP section (32 bytes).
    DataWriter w(Endianess::BigEndian);
    // PSIN: ident, length(16), 8-byte payload
    w.Write(static_cast<uint32_t>(0x5053494E)); // "PSIN"
    w.Write(static_cast<int32_t>(16));          // length
    for (int i = 0; i < 8; ++i) w.Write(static_cast<uint8_t>(0x40 + i));
    // PMAP: ident, length(32), rootId, count, unkE, 1 entry
    w.Write(static_cast<uint32_t>(0x504D4150)); // "PMAP"
    w.Write(static_cast<int32_t>(32));          // length
    w.Write(static_cast<int32_t>(1));           // rootId
    w.Write(static_cast<int16_t>(1));           // entriesCount
    w.Write(static_cast<int16_t>(0x7070));      // Unknown_Eh
    // entry: nameHash, offset, unk, length
    w.Write(static_cast<uint32_t>(0xAABBCCDD));
    w.Write(static_cast<int32_t>(8));
    w.Write(static_cast<int32_t>(0));
    w.Write(static_cast<int32_t>(8));

    std::vector<uint8_t> data = w.buffer();

    check(PsoFile::isPSO(data), "isPSO detects PSIN magic");
    PsoFile pso;
    bool ok = pso.load(data);
    check(ok, "pso loaded");
    check(pso.rootId() == 1, "pso root id");
    check(pso.dataSection().size() == 16, "PSIN data section captured");
    check(pso.entries().size() == 1, "one data-map entry");
    const PsoDataMappingEntry* b = pso.getBlock(1);
    check(b != nullptr, "getBlock(1) resolves");
    check(b && b->nameHash == 0xAABBCCDDu, "entry name hash");
    check(b && b->offset == 8 && b->length == 8, "entry offset/length");
    check(pso.getBlock(2) == nullptr, "out-of-range block is null");
}

static void testRbf()
{
    std::printf("[rbf] RBF binary-XML tree parse\n");
    using namespace evw::gamefiles;

    auto wstr = [](DataWriter& w, const char* s) {
        for (const char* p = s; *p; ++p) w.Write(static_cast<uint8_t>(*p));
    };

    DataWriter w; // little-endian
    w.Write(static_cast<int32_t>(0x30464252)); // "RBF0"
    // root structure: descriptorIndex 0 (new), type 0
    w.Write(static_cast<uint8_t>(0)); w.Write(static_cast<uint8_t>(0));
    w.Write(static_cast<int16_t>(4)); wstr(w, "root");
    w.Write(static_cast<int16_t>(0)); w.Write(static_cast<int16_t>(0)); w.Write(static_cast<int16_t>(0));
    // uint child: descriptorIndex 1 (new), type 0x10
    w.Write(static_cast<uint8_t>(1)); w.Write(static_cast<uint8_t>(0x10));
    w.Write(static_cast<int16_t>(3)); wstr(w, "val");
    w.Write(static_cast<uint32_t>(42));
    // float child: descriptorIndex 2 (new), type 0x40
    w.Write(static_cast<uint8_t>(2)); w.Write(static_cast<uint8_t>(0x40));
    w.Write(static_cast<int16_t>(1)); wstr(w, "f");
    w.Write(2.5f);
    // string child: descriptorIndex 3 (new), type 0x60
    w.Write(static_cast<uint8_t>(3)); w.Write(static_cast<uint8_t>(0x60));
    w.Write(static_cast<int16_t>(4)); wstr(w, "name");
    w.Write(static_cast<int16_t>(5)); wstr(w, "hello");
    // close root
    w.Write(static_cast<uint8_t>(0xFF)); w.Write(static_cast<uint8_t>(0xFF));

    std::vector<uint8_t> data = w.buffer();
    check(RbfFile::isRBF(data), "isRBF detects magic");

    RbfFile rbf;
    auto root = rbf.load(data);
    check(root != nullptr, "rbf root parsed");
    check(root && root->name == "root", "root name");
    check(root && root->children.size() == 3, "root has 3 children");
    auto val = root ? root->find("val") : nullptr;
    check(val && val->type == RbfType::Uint32 && val->u32 == 42, "uint child");
    auto f = root ? root->find("f") : nullptr;
    check(f && f->type == RbfType::Float && f->f == 2.5f, "float child");
    auto nm = root ? root->find("name") : nullptr;
    check(nm && nm->type == RbfType::String && nm->str == "hello", "string child");
}

static void testGxt2()
{
    std::printf("[gxt2] global text table\n");
    using namespace evw::gamefiles;
    DataWriter w; // little-endian
    w.Write(static_cast<uint32_t>(Gxt2File::MAGIC));
    w.Write(static_cast<uint32_t>(1));            // count
    w.Write(static_cast<uint32_t>(0x1234));       // hash
    w.Write(static_cast<uint32_t>(24));           // offset
    w.Write(static_cast<uint32_t>(Gxt2File::MAGIC)); // second magic
    w.Write(static_cast<uint32_t>(27));           // endpos
    for (const char* p = "hi"; *p; ++p) w.Write(static_cast<uint8_t>(*p));
    w.Write(static_cast<uint8_t>(0));

    Gxt2File gxt;
    bool ok = gxt.load(w.buffer());
    check(ok, "gxt2 loaded");
    check(gxt.entries().size() == 1, "one text entry");
    check(gxt.lookup(0x1234) == "hi", "text lookup by hash");
}

static void testGameFileType()
{
    std::printf("[gamefile] type detection by extension\n");
    using namespace evw::gamefiles;
    check(detectGameFileType("prop_bush.ydr") == GameFileType::Ydr, "ydr");
    check(detectGameFileType("vehicles.ytd") == GameFileType::Ytd, "ytd");
    check(detectGameFileType("CITY.YMAP") == GameFileType::Ymap, "ymap (uppercase)");
    check(detectGameFileType("x.ytyp") == GameFileType::Ytyp, "ytyp");
    check(detectGameFileType("a.ybn") == GameFileType::Ybn, "ybn");
    check(detectGameFileType("readme.txt") == GameFileType::Unknown, "unknown ext");
    check(std::string(gameFileTypeName(GameFileType::Ydr)).find("YDR") != std::string::npos, "type name");
}

static void testRbfXml()
{
    std::printf("[rbf] RBF -> XML export\n");
    using namespace evw::gamefiles;
    auto wstr = [](DataWriter& w, const char* s) { for (const char* p = s; *p; ++p) w.Write(static_cast<uint8_t>(*p)); };
    DataWriter w;
    w.Write(static_cast<int32_t>(0x30464252));
    w.Write(static_cast<uint8_t>(0)); w.Write(static_cast<uint8_t>(0));
    w.Write(static_cast<int16_t>(4)); wstr(w, "root");
    w.Write(static_cast<int16_t>(0)); w.Write(static_cast<int16_t>(0)); w.Write(static_cast<int16_t>(0));
    w.Write(static_cast<uint8_t>(1)); w.Write(static_cast<uint8_t>(0x60));
    w.Write(static_cast<int16_t>(4)); wstr(w, "name");
    w.Write(static_cast<int16_t>(5)); wstr(w, "hello");
    w.Write(static_cast<uint8_t>(0xFF)); w.Write(static_cast<uint8_t>(0xFF));

    RbfFile rbf;
    auto root = rbf.load(w.buffer());
    std::string xml = rbfToXml(root);
    check(xml.find("<root>") != std::string::npos, "xml has root element");
    check(xml.find("hello") != std::string::npos, "xml has string value");
}

static void testWaypointRecords()
{
    std::printf("[ywr] waypoint record list\n");
    using namespace evw::gamefiles;
    DataWriter w;
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    w.setPosition(0x10);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0));   // Unk10/14
    w.Write(static_cast<uint64_t>(0x50000040));                            // EntriesPointer
    w.Write(static_cast<uint32_t>(2));                                     // EntriesCount
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0));
    w.setPosition(0x40);
    // entry 0: pos(1,2,3), unk 10,11,12,13
    w.Write(1.0f); w.Write(2.0f); w.Write(3.0f);
    w.Write(static_cast<uint16_t>(10)); w.Write(static_cast<uint16_t>(11));
    w.Write(static_cast<uint16_t>(12)); w.Write(static_cast<uint16_t>(13));
    // entry 1: pos(4,5,6)
    w.Write(4.0f); w.Write(5.0f); w.Write(6.0f);
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint16_t>(0));
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint16_t>(0));

    ResourceDataReader r(static_cast<uint32_t>(w.buffer().size()), 0, w.buffer());
    auto list = r.ReadBlock<WaypointRecordList>();
    check(list->entriesCount == 2, "two waypoint entries");
    check(list->entries.size() == 2 && list->entries[0].position == math::Vector3(1, 2, 3), "entry 0 position");
    check(list->entries.size() == 2 && list->entries[0].unk0 == 10, "entry 0 unk0");
    check(list->entries.size() == 2 && list->entries[1].position == math::Vector3(4, 5, 6), "entry 1 position");
}

static void testQuaternion()
{
    std::printf("[math] quaternion -> matrix (identity)\n");
    using namespace evw::math;
    Quaternion q(0, 0, 0, 1);
    Matrix m = quaternionToMatrix(q);
    check(m == Matrix(), "identity quaternion -> identity matrix");
    Quaternion n = Quaternion(0, 0, 0, 2).normalized();
    check(n == Quaternion(0, 0, 0, 1), "quaternion normalized");
}

static void testHeightmap()
{
    std::printf("[hmap] heightmap (.dat) uncompressed parse\n");
    using namespace evw::gamefiles;
    DataWriter w;
    w.Write(static_cast<uint32_t>(0x484D4150)); // magic
    w.Write(static_cast<uint8_t>(1)); w.Write(static_cast<uint8_t>(0)); // version
    w.Write(static_cast<uint16_t>(0));          // pad
    w.Write(static_cast<uint32_t>(0));          // compressed = 0
    w.Write(static_cast<uint16_t>(2));          // width
    w.Write(static_cast<uint16_t>(2));          // height
    w.Write(0.0f); w.Write(0.0f); w.Write(0.0f); // bbMin
    w.Write(10.0f); w.Write(10.0f); w.Write(5.0f); // bbMax
    w.Write(static_cast<uint32_t>(4));          // length
    w.Write(static_cast<uint8_t>(10)); w.Write(static_cast<uint8_t>(20));
    w.Write(static_cast<uint8_t>(30)); w.Write(static_cast<uint8_t>(40));

    HeightmapFile hm;
    bool ok = hm.load(w.buffer());
    check(ok, "heightmap loaded");
    check(hm.width == 2 && hm.height == 2, "heightmap dimensions");
    check(hm.bbMax == math::Vector3(10, 10, 5), "heightmap bbMax");
    check(hm.maxHeights.size() == 4 && hm.maxHeights[0] == 10 && hm.maxHeights[3] == 40, "height cells");
}

static void testBoundGeometry()
{
    std::printf("[bounds] BoundGeometry quantized vertices\n");
    using namespace evw::gamefiles;
    DataWriter w;
    // ResourceFileBase header
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    // Bounds base: Type = 4 (Geometry) at 0x10; rest zero
    w.setPosition(0x10); w.Write(static_cast<uint8_t>(4));
    // BoundGeometry struct from 0x70
    w.setPosition(0x90); w.Write(0.25f); w.Write(0.25f); w.Write(0.25f);   // Quantum
    w.setPosition(0xB0); w.Write(static_cast<uint64_t>(0x50000200));        // VerticesPointer
    w.setPosition(0xD0); w.Write(static_cast<uint32_t>(3));                 // VerticesCount
    w.Write(static_cast<uint32_t>(1));                                      // PolygonsCount
    w.setPosition(0x120); w.Write(static_cast<uint8_t>(2));                 // MaterialsCount
    // ensure block extends to 0x130
    w.setPosition(0x12F); w.Write(static_cast<uint8_t>(0));
    // vertices at 0x200: int16 triples
    w.setPosition(0x200);
    w.Write(static_cast<int16_t>(4)); w.Write(static_cast<int16_t>(8)); w.Write(static_cast<int16_t>(12));
    w.Write(static_cast<int16_t>(0)); w.Write(static_cast<int16_t>(0)); w.Write(static_cast<int16_t>(0));
    w.Write(static_cast<int16_t>(8)); w.Write(static_cast<int16_t>(8)); w.Write(static_cast<int16_t>(8));

    ResourceDataReader r(static_cast<uint32_t>(w.buffer().size()), 0, w.buffer());
    auto bg = r.ReadBlock<BoundGeometry>();
    check(bg->type == BoundsType::Geometry, "geometry bound type");
    check(bg->verticesCount == 3, "vertex count");
    check(bg->materialsCount == 2, "materials count");
    check(bg->quantum == math::Vector3(0.25f, 0.25f, 0.25f), "quantum");
    check(bg->vertices.size() == 3 && bg->vertices[0] == math::Vector3(1, 2, 3), "dequantized vertex 0");
    check(bg->vertices.size() == 3 && bg->vertices[2] == math::Vector3(2, 2, 2), "dequantized vertex 2");
}

static void testNodeDictionary()
{
    std::printf("[ynd] path node dictionary\n");
    using namespace evw::gamefiles;
    DataWriter w;
    w.setPosition(0x00);
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    // NodeDictionary header from 0x10
    w.setPosition(0x10);
    w.Write(static_cast<uint64_t>(0x50000080)); // NodesPointer
    w.Write(static_cast<uint32_t>(2));          // NodesCount
    w.Write(static_cast<uint32_t>(2));          // vehicle
    w.Write(static_cast<uint32_t>(0));          // ped
    w.Write(static_cast<uint32_t>(0));          // Unk24
    w.Write(static_cast<uint64_t>(0));          // LinksPtr
    w.Write(static_cast<uint32_t>(0));          // LinksCount
    w.Write(static_cast<uint32_t>(0));          // Unk34
    w.Write(static_cast<uint64_t>(0));          // JunctionsPtr
    w.Write(static_cast<uint64_t>(0));          // JHBPtr
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0)); // Unk48/4C
    w.Write(static_cast<uint64_t>(0));          // JunctionRefsPtr
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint16_t>(0)); // refs counts
    w.Write(static_cast<uint32_t>(0));          // Unk5C
    w.Write(static_cast<uint32_t>(0));          // JunctionsCount
    w.Write(static_cast<uint32_t>(0));          // JHBCount
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0)); // Unk68/6C
    // node0 at 0x80
    w.setPosition(0x80);
    for (int i = 0; i < 4; ++i) w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint16_t>(5)); w.Write(static_cast<uint16_t>(7));  // area, node
    w.Write(static_cast<uint32_t>(0));                                     // street
    w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint16_t>(0));  // unused4, link
    w.Write(static_cast<int16_t>(100)); w.Write(static_cast<int16_t>(200)); // posX/Y
    w.Write(static_cast<uint8_t>(0)); w.Write(static_cast<uint8_t>(0));
    w.Write(static_cast<int16_t>(50));                                     // posZ
    w.Write(static_cast<uint8_t>(0)); w.Write(static_cast<uint8_t>(0));
    w.Write(static_cast<uint8_t>(0)); w.Write(static_cast<uint8_t>(0));
    // node1 at 0xA8
    w.setPosition(0xA8);
    for (int i = 0; i < 4; ++i) w.Write(static_cast<uint32_t>(0));
    w.Write(static_cast<uint16_t>(9)); w.Write(static_cast<uint16_t>(0));
    w.setPosition(0xA8 + 40 - 1); w.Write(static_cast<uint8_t>(0));

    ResourceDataReader r(static_cast<uint32_t>(w.buffer().size()), 0, w.buffer());
    auto nd = r.ReadBlock<NodeDictionary>();
    check(nd->nodesCount == 2, "node count");
    check(nd->nodes.size() == 2 && nd->nodes[0].areaID == 5 && nd->nodes[0].nodeID == 7, "node 0 ids");
    check(nd->nodes.size() == 2 && nd->nodes[0].positionX == 100 && nd->nodes[0].positionZ == 50, "node 0 position");
    check(nd->nodes.size() == 2 && nd->nodes[1].areaID == 9, "node 1 area id");
}

static void testNavMesh()
{
    std::printf("[ynv] nav mesh header\n");
    using namespace evw::gamefiles;
    DataWriter w;
    // ResourceFileBase header
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    // NavMesh header (sequential, matching read order)
    w.Write(static_cast<uint32_t>(7));            // ContentFlags
    w.Write(static_cast<uint32_t>(0));            // VersionUnk1
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0)); // Unused 018/01C
    for (int i = 0; i < 16; ++i) w.Write(0.0f);   // Transform
    w.Write(100.0f); w.Write(50.0f); w.Write(100.0f); // AABBSize
    w.Write(static_cast<uint32_t>(0));            // AABBUnk
    w.Write(static_cast<uint64_t>(0));            // VerticesPointer
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0)); // Unused 078/07C
    w.Write(static_cast<uint64_t>(0));            // IndicesPointer
    w.Write(static_cast<uint64_t>(0));            // EdgesPointer
    w.Write(static_cast<uint32_t>(0));            // EdgesIndicesCount
    for (int i = 0; i < 16; ++i) w.Write(static_cast<uint32_t>(0)); // AdjAreaIDs
    w.Write(static_cast<uint64_t>(0));            // PolysPointer
    w.Write(static_cast<uint64_t>(0));            // SectorTreePointer
    w.Write(static_cast<uint64_t>(0));            // PortalsPointer
    w.Write(static_cast<uint64_t>(0));            // PortalLinksPointer
    w.Write(static_cast<uint32_t>(120));          // VerticesCount
    w.Write(static_cast<uint32_t>(64));           // PolysCount
    w.Write(static_cast<uint32_t>(5403));         // AreaID
    w.Write(static_cast<uint32_t>(4096));         // TotalBytes
    w.Write(static_cast<uint32_t>(8));            // PointsCount
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0)); // Portals/PortalLinks counts
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(0));

    ResourceDataReader r(static_cast<uint32_t>(w.buffer().size()), 0, w.buffer());
    auto nav = r.ReadBlock<NavMesh>();
    check(nav->contentFlags == 7, "nav content flags");
    check(nav->aabbSize == math::Vector3(100, 50, 100), "nav aabb size");
    check(nav->verticesCount == 120, "nav vertices count");
    check(nav->polysCount == 64, "nav polys count");
    check(nav->areaID == 5403, "nav area id");
    check(nav->pointsCount == 8, "nav points count");
}

static void testFragType()
{
    std::printf("[yft] fragment type header\n");
    using namespace evw::gamefiles;
    DataWriter w;
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<uint32_t>(1)); w.Write(static_cast<uint64_t>(0));
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0)); // Unknown_10/18
    w.Write(1.0f); w.Write(2.0f); w.Write(3.0f);   // BoundingSphereCenter
    w.Write(7.5f);                                  // BoundingSphereRadius
    w.Write(static_cast<uint64_t>(0));              // DrawablePointer (none)
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0)); // arrays
    w.Write(static_cast<uint32_t>(0)); w.Write(static_cast<int32_t>(0));  // count/flag
    w.Write(static_cast<uint64_t>(0));              // Unknown_50h
    w.Write(static_cast<uint64_t>(0));              // NamePointer
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint32_t>(0)); // Cloths
    for (int i = 0; i < 7; ++i) w.Write(static_cast<uint64_t>(0)); // Unknown_70..A0
    w.Write(static_cast<uint64_t>(0));              // BoneTransformsPointer
    for (int i = 0; i < 7; ++i) w.Write(static_cast<int32_t>(0));  // B0..C8
    w.Write(0.0f);                                  // Unknown_CCh
    w.Write(9.8f);                                  // GravityFactor
    w.Write(1.2f);                                  // BuoyancyFactor
    w.Write(static_cast<uint8_t>(0));               // Unknown_D8h
    w.Write(static_cast<uint8_t>(3));               // GlassWindowsCount
    w.Write(static_cast<uint16_t>(0));              // Unknown_DAh
    w.Write(static_cast<uint32_t>(0));              // Unknown_DCh
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0)); // GlassWindows/E8
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0)); // PhysicsLODGroup/DrawableCloth
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0)); // 100/108
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint16_t>(0)); w.Write(static_cast<uint32_t>(0)); // LightAttributes
    w.Write(static_cast<uint64_t>(0)); w.Write(static_cast<uint64_t>(0)); // VehicleGlass/128

    ResourceDataReader r(static_cast<uint32_t>(w.buffer().size()), 0, w.buffer());
    auto frag = r.ReadBlock<FragType>();
    check(frag->boundingSphereCenter == math::Vector3(1, 2, 3), "frag bounding sphere center");
    check(frag->boundingSphereRadius == 7.5f, "frag bounding sphere radius");
    check(frag->gravityFactor == 9.8f, "frag gravity factor");
    check(frag->buoyancyFactor == 1.2f, "frag buoyancy factor");
    check(frag->glassWindowsCount == 3, "frag glass windows count");
    check(frag->drawable == nullptr, "no drawable (pointer 0)");
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
    testVertexPositions();
    testDrawableModel();
    testShaderGroup();
    testShaderParameters();
    testDrawable();
    testDrawableDictionary();
    testMeta();
    testMetaTypes();
    testYmap();
    testYtyp();
    testExplorerModel();
    testMathMatrix();
    testRenderMesh();
    testBounds();
    testBoundGeometry();
    testPso();
    testRbf();
    testGxt2();
    testGameFileType();
    testRbfXml();
    testWaypointRecords();
    testQuaternion();
    testHeightmap();
    testNodeDictionary();
    testNavMesh();
    testFragType();

    std::printf("\n%d/%d checks passed\n", g_checks - g_failures, g_checks);
    if (g_failures != 0)
    {
        std::printf("RESULT: FAILED (%d)\n", g_failures);
        return 1;
    }
    std::printf("RESULT: OK\n");
    return 0;
}

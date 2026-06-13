#include "evw/gamefiles/inflate.h"

#include <array>
#include <stdexcept>

namespace evw::compression
{
    namespace
    {
        class BitReader
        {
        public:
            BitReader(const uint8_t* data, size_t length) : data_(data), length_(length) {}

            uint32_t getBit()
            {
                if (bitCount_ == 0)
                {
                    if (bytePos_ >= length_) throw std::runtime_error("inflate: out of input");
                    bitBuffer_ = data_[bytePos_++];
                    bitCount_ = 8;
                }
                uint32_t bit = bitBuffer_ & 1u;
                bitBuffer_ >>= 1;
                --bitCount_;
                return bit;
            }

            uint32_t getBits(int count)
            {
                uint32_t value = 0;
                for (int i = 0; i < count; ++i) value |= (getBit() << i);
                return value;
            }

            void alignToByte() { bitCount_ = 0; }

            uint8_t readByte()
            {
                if (bytePos_ >= length_) throw std::runtime_error("inflate: out of input");
                return data_[bytePos_++];
            }

        private:
            const uint8_t* data_;
            size_t length_;
            size_t bytePos_ = 0;
            uint32_t bitBuffer_ = 0;
            int bitCount_ = 0;
        };

        // Canonical Huffman decoder built from a list of code lengths.
        struct HuffmanTree
        {
            std::array<int, 16> counts{};   // number of codes of each length
            std::vector<int> symbols;       // symbols sorted by code

            void build(const std::vector<int>& lengths)
            {
                counts.fill(0);
                for (int len : lengths) counts[len]++;
                counts[0] = 0;

                std::array<int, 16> offsets{};
                offsets[1] = 0;
                for (int i = 1; i < 15; ++i) offsets[i + 1] = offsets[i] + counts[i];

                symbols.assign(lengths.size(), 0);
                for (size_t sym = 0; sym < lengths.size(); ++sym)
                    if (lengths[sym] != 0)
                        symbols[offsets[lengths[sym]]++] = static_cast<int>(sym);
            }
        };

        int decodeSymbol(BitReader& br, const HuffmanTree& tree)
        {
            int code = 0;
            int first = 0;
            int index = 0;
            for (int len = 1; len <= 15; ++len)
            {
                code |= static_cast<int>(br.getBit());
                int count = tree.counts[len];
                if (code - first < count)
                    return tree.symbols[index + (code - first)];
                index += count;
                first += count;
                first <<= 1;
                code <<= 1;
            }
            throw std::runtime_error("inflate: invalid Huffman code");
        }

        // RFC 1951 length/distance base tables.
        const int kLengthBase[29] = {
            3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
        const int kLengthExtra[29] = {
            0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
        const int kDistBase[30] = {
            1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,
            1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
        const int kDistExtra[30] = {
            0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

        void inflateBlockData(BitReader& br, std::vector<uint8_t>& out,
                              const HuffmanTree& litTree, const HuffmanTree& distTree)
        {
            for (;;)
            {
                int sym = decodeSymbol(br, litTree);
                if (sym == 256) break;          // end of block
                if (sym < 256)
                {
                    out.push_back(static_cast<uint8_t>(sym));
                    continue;
                }
                sym -= 257;
                if (sym >= 29) throw std::runtime_error("inflate: invalid length symbol");
                int length = kLengthBase[sym] + static_cast<int>(br.getBits(kLengthExtra[sym]));

                int dsym = decodeSymbol(br, distTree);
                if (dsym >= 30) throw std::runtime_error("inflate: invalid distance symbol");
                int distance = kDistBase[dsym] + static_cast<int>(br.getBits(kDistExtra[dsym]));

                if (static_cast<size_t>(distance) > out.size())
                    throw std::runtime_error("inflate: distance too far back");

                size_t start = out.size() - distance;
                for (int i = 0; i < length; ++i)
                    out.push_back(out[start + i]);
            }
        }

        void buildFixedTrees(HuffmanTree& litTree, HuffmanTree& distTree)
        {
            std::vector<int> litLengths(288);
            for (int i = 0; i < 144; ++i) litLengths[i] = 8;
            for (int i = 144; i < 256; ++i) litLengths[i] = 9;
            for (int i = 256; i < 280; ++i) litLengths[i] = 7;
            for (int i = 280; i < 288; ++i) litLengths[i] = 8;
            litTree.build(litLengths);

            std::vector<int> distLengths(30, 5);
            distTree.build(distLengths);
        }

        void readDynamicTrees(BitReader& br, HuffmanTree& litTree, HuffmanTree& distTree)
        {
            int hlit = static_cast<int>(br.getBits(5)) + 257;
            int hdist = static_cast<int>(br.getBits(5)) + 1;
            int hclen = static_cast<int>(br.getBits(4)) + 4;

            static const int order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
            std::vector<int> clLengths(19, 0);
            for (int i = 0; i < hclen; ++i)
                clLengths[order[i]] = static_cast<int>(br.getBits(3));

            HuffmanTree clTree;
            clTree.build(clLengths);

            std::vector<int> lengths;
            lengths.reserve(hlit + hdist);
            while (static_cast<int>(lengths.size()) < hlit + hdist)
            {
                int sym = decodeSymbol(br, clTree);
                if (sym < 16)
                {
                    lengths.push_back(sym);
                }
                else if (sym == 16)
                {
                    if (lengths.empty()) throw std::runtime_error("inflate: bad repeat");
                    int repeat = 3 + static_cast<int>(br.getBits(2));
                    int prev = lengths.back();
                    for (int i = 0; i < repeat; ++i) lengths.push_back(prev);
                }
                else if (sym == 17)
                {
                    int repeat = 3 + static_cast<int>(br.getBits(3));
                    for (int i = 0; i < repeat; ++i) lengths.push_back(0);
                }
                else // sym == 18
                {
                    int repeat = 11 + static_cast<int>(br.getBits(7));
                    for (int i = 0; i < repeat; ++i) lengths.push_back(0);
                }
            }

            std::vector<int> litLengths(lengths.begin(), lengths.begin() + hlit);
            std::vector<int> distLengths(lengths.begin() + hlit, lengths.begin() + hlit + hdist);
            litTree.build(litLengths);
            distTree.build(distLengths);
        }
    }

    std::vector<uint8_t> inflateRaw(const uint8_t* data, size_t length, size_t expectedSize)
    {
        BitReader br(data, length);
        std::vector<uint8_t> out;
        if (expectedSize) out.reserve(expectedSize);

        bool finalBlock = false;
        while (!finalBlock)
        {
            finalBlock = br.getBit() != 0;
            uint32_t type = br.getBits(2);
            if (type == 0)
            {
                // Stored (uncompressed) block.
                br.alignToByte();
                uint8_t l0 = br.readByte();
                uint8_t l1 = br.readByte();
                br.readByte(); // nlen lo
                br.readByte(); // nlen hi
                int len = l0 | (l1 << 8);
                for (int i = 0; i < len; ++i) out.push_back(br.readByte());
            }
            else if (type == 1)
            {
                HuffmanTree litTree, distTree;
                buildFixedTrees(litTree, distTree);
                inflateBlockData(br, out, litTree, distTree);
            }
            else if (type == 2)
            {
                HuffmanTree litTree, distTree;
                readDynamicTrees(br, litTree, distTree);
                inflateBlockData(br, out, litTree, distTree);
            }
            else
            {
                throw std::runtime_error("inflate: invalid block type");
            }
        }
        return out;
    }

    std::vector<uint8_t> inflateRaw(const std::vector<uint8_t>& input, size_t expectedSize)
    {
        return inflateRaw(input.data(), input.size(), expectedSize);
    }
}

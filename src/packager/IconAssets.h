#pragma once

#include "ArchiveBuilders.h"
#include "../platform/MemeNativeApis.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace MemePackager {

struct IconImage {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgba;
};

namespace icon_detail {

inline void pushU32BE(std::vector<uint8_t> &out, uint32_t v) {
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(v & 0xFF));
}

inline void pushU16LE(std::vector<uint8_t> &out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

inline void pushU32LE(std::vector<uint8_t> &out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

inline uint32_t readU32BE(const uint8_t *p) {
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) |
           static_cast<uint32_t>(p[3]);
}

inline uint32_t adler32Bytes(const uint8_t *data, size_t len) {
    uint32_t a = 1, b = 0;
    constexpr uint32_t MOD = 65521;
    for (size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % MOD;
        b = (b + a) % MOD;
    }
    return (b << 16) | a;
}

inline std::vector<uint8_t> zlibCompress(const std::vector<uint8_t> &raw) {
    // Reuse gzipBytes from ArchiveBuilders to extract compressed RFC-1951 DEFLATE payload when available.
    auto gz = gzipBytes(raw);
    if (gz.size() >= 18 && gz[0] == 0x1F && gz[1] == 0x8B && gz[2] == 0x08 && gz[3] == 0x00) {
        std::vector<uint8_t> z;
        z.reserve(2 + (gz.size() - 18) + 4);
        z.push_back(0x78);
        z.push_back(0x9C);
        z.insert(z.end(), gz.begin() + 10, gz.end() - 8);
        pushU32BE(z, adler32Bytes(raw.data(), raw.size()));
        return z;
    }
    // Stored RFC-1950 fallback
    std::vector<uint8_t> z = {0x78, 0x01};
    size_t pos = 0;
    do {
        size_t rem = raw.size() - pos;
        uint16_t chunk = static_cast<uint16_t>(std::min<size_t>(65535, rem));
        bool last = (pos + chunk >= raw.size());
        z.push_back(last ? 0x01 : 0x00);
        pushU16LE(z, chunk);
        pushU16LE(z, static_cast<uint16_t>(~chunk));
        if (chunk > 0) z.insert(z.end(), raw.begin() + static_cast<ptrdiff_t>(pos), raw.begin() + static_cast<ptrdiff_t>(pos + chunk));
        pos += chunk;
    } while (pos < raw.size());
    pushU32BE(z, adler32Bytes(raw.data(), raw.size()));
    return z;
}

// Minimal RFC-1951 DEFLATE + zlib inflater for pure-C++ custom PNG decoding
struct BitReader {
    const uint8_t *data = nullptr;
    size_t size = 0;
    size_t bytePos = 0;
    uint32_t bitBuf = 0;
    int bitCount = 0;

    void fill(int n) {
        while (bitCount < n && bytePos < size) {
            bitBuf |= static_cast<uint32_t>(data[bytePos++]) << bitCount;
            bitCount += 8;
        }
    }
    int readBits(int n) {
        if (n == 0) return 0;
        fill(n);
        if (bitCount < n) return -1;
        int val = static_cast<int>(bitBuf & ((1u << n) - 1u));
        bitBuf >>= n;
        bitCount -= n;
        return val;
    }
    void alignToByte() {
        int drop = bitCount & 7;
        bitBuf >>= drop;
        bitCount -= drop;
    }
};

struct HuffTree {
    static constexpr int MAX_BITS = 15;
    std::array<int, MAX_BITS + 1> count{};
    std::vector<int> symbols;
    std::array<int, MAX_BITS + 1> offsets{};

    bool build(const std::vector<int> &lengths) {
        count.fill(0);
        for (int len : lengths) {
            if (len > MAX_BITS) return false;
            if (len > 0) count[static_cast<size_t>(len)]++;
        }
        int left = 1;
        for (int len = 1; len <= MAX_BITS; ++len) {
            left = (left << 1) - count[static_cast<size_t>(len)];
            if (left < 0) return false;
        }
        offsets.fill(0);
        for (int len = 1; len < MAX_BITS; ++len) {
            offsets[static_cast<size_t>(len + 1)] = offsets[static_cast<size_t>(len)] + count[static_cast<size_t>(len)];
        }
        int total = offsets[MAX_BITS] + count[MAX_BITS];
        symbols.assign(static_cast<size_t>(total), 0);
        std::array<int, MAX_BITS + 1> cur = offsets;
        for (size_t i = 0; i < lengths.size(); ++i) {
            int len = lengths[i];
            if (len > 0) symbols[static_cast<size_t>(cur[static_cast<size_t>(len)]++)] = static_cast<int>(i);
        }
        return true;
    }

    int decode(BitReader &br) const {
        int code = 0, first = 0, index = 0;
        for (int len = 1; len <= MAX_BITS; ++len) {
            int b = br.readBits(1);
            if (b < 0) return -1;
            code |= b;
            int cnt = count[static_cast<size_t>(len)];
            if (code - cnt < first) {
                return symbols[static_cast<size_t>(index + (code - first))];
            }
            index += cnt;
            first += cnt;
            first <<= 1;
            code <<= 1;
        }
        return -1;
    }
};

inline bool inflateZlib(const std::vector<uint8_t> &zdata, std::vector<uint8_t> &out) {
    if (zdata.size() < 6) return false;
    uint8_t cmf = zdata[0], flg = zdata[1];
    if ((cmf & 0x0F) != 8 || (((static_cast<uint16_t>(cmf) << 8) | flg) % 31) != 0) return false;
    size_t start = (flg & 0x20) ? 6 : 2;
    if (start >= zdata.size()) return false;

    BitReader br{zdata.data() + start, zdata.size() - start, 0, 0, 0};
    out.clear();

    static constexpr int lenBase[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
    static constexpr int lenExtra[29] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static constexpr int distBase[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static constexpr int distExtra[30] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
    static constexpr int clOrder[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};

    while (true) {
        int bfinal = br.readBits(1);
        int btype = br.readBits(2);
        if (bfinal < 0 || btype < 0) return false;
        if (btype == 0) {
            br.alignToByte();
            int l0 = br.readBits(8), l1 = br.readBits(8), n0 = br.readBits(8), n1 = br.readBits(8);
            if (l0 < 0 || l1 < 0 || n0 < 0 || n1 < 0) return false;
            uint16_t len = static_cast<uint16_t>(l0 | (l1 << 8));
            for (uint16_t i = 0; i < len; ++i) {
                int c = br.readBits(8);
                if (c < 0) return false;
                out.push_back(static_cast<uint8_t>(c));
            }
        } else if (btype == 1 || btype == 2) {
            HuffTree litTree, distTree;
            if (btype == 1) {
                std::vector<int> ll(288, 0), dl(32, 5);
                for (int i = 0; i <= 143; ++i) ll[static_cast<size_t>(i)] = 8;
                for (int i = 144; i <= 255; ++i) ll[static_cast<size_t>(i)] = 9;
                for (int i = 256; i <= 279; ++i) ll[static_cast<size_t>(i)] = 7;
                for (int i = 280; i <= 287; ++i) ll[static_cast<size_t>(i)] = 8;
                if (!litTree.build(ll) || !distTree.build(dl)) return false;
            } else {
                int hlit = br.readBits(5) + 257;
                int hdist = br.readBits(5) + 1;
                int hclen = br.readBits(4) + 4;
                if (hlit < 257 || hdist < 1 || hclen < 4) return false;
                std::vector<int> clens(19, 0);
                for (int i = 0; i < hclen; ++i) clens[static_cast<size_t>(clOrder[i])] = br.readBits(3);
                HuffTree clTree;
                if (!clTree.build(clens)) return false;
                std::vector<int> both(static_cast<size_t>(hlit + hdist), 0);
                int idx = 0;
                while (idx < hlit + hdist) {
                    int sym = clTree.decode(br);
                    if (sym < 0) return false;
                    if (sym < 16) {
                        both[static_cast<size_t>(idx++)] = sym;
                    } else {
                        int rep = 0, val = 0;
                        if (sym == 16) {
                            if (idx == 0) return false;
                            val = both[static_cast<size_t>(idx - 1)];
                            rep = 3 + br.readBits(2);
                        } else if (sym == 17) {
                            rep = 3 + br.readBits(3);
                        } else {
                            rep = 11 + br.readBits(7);
                        }
                        while (rep-- > 0 && idx < hlit + hdist) both[static_cast<size_t>(idx++)] = val;
                    }
                }
                std::vector<int> ll(both.begin(), both.begin() + hlit);
                std::vector<int> dl(both.begin() + hlit, both.end());
                if (!litTree.build(ll) || !distTree.build(dl)) return false;
            }
            while (true) {
                int sym = litTree.decode(br);
                if (sym < 0) return false;
                if (sym < 256) {
                    out.push_back(static_cast<uint8_t>(sym));
                } else if (sym == 256) {
                    break;
                } else {
                    int li = sym - 257;
                    if (li < 0 || li >= 29) return false;
                    int matchLen = lenBase[li] + br.readBits(lenExtra[li]);
                    int ds = distTree.decode(br);
                    if (ds < 0 || ds >= 30) return false;
                    int dist = distBase[ds] + br.readBits(distExtra[ds]);
                    if (dist <= 0 || static_cast<size_t>(dist) > out.size()) return false;
                    for (int k = 0; k < matchLen; ++k) {
                        out.push_back(out[out.size() - static_cast<size_t>(dist)]);
                    }
                }
            }
        } else {
            return false;
        }
        if (bfinal) break;
    }
    return true;
}

inline int paethPredictor(int a, int b, int c) {
    int p = a + b - c;
    int pa = std::abs(p - a), pb = std::abs(p - b), pc = std::abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

} // namespace icon_detail

inline IconImage resizeBilinearRgba(const IconImage &src, int dstW, int dstH) {
    IconImage dst;
    dst.width = std::max(1, dstW);
    dst.height = std::max(1, dstH);
    dst.rgba.assign(static_cast<size_t>(dst.width) * static_cast<size_t>(dst.height) * 4u, 0);
    if (src.width <= 0 || src.height <= 0 || src.rgba.empty()) return dst;
    if (src.width == dst.width && src.height == dst.height) {
        dst.rgba = src.rgba;
        return dst;
    }
    for (int y = 0; y < dst.height; ++y) {
        float fy = (dst.height > 1) ? (static_cast<float>(y) + 0.5f) * static_cast<float>(src.height) / static_cast<float>(dst.height) - 0.5f : 0.0f;
        int y0 = std::clamp(static_cast<int>(std::floor(fy)), 0, src.height - 1);
        int y1 = std::clamp(y0 + 1, 0, src.height - 1);
        float ty = std::clamp(fy - static_cast<float>(y0), 0.0f, 1.0f);
        for (int x = 0; x < dst.width; ++x) {
            float fx = (dst.width > 1) ? (static_cast<float>(x) + 0.5f) * static_cast<float>(src.width) / static_cast<float>(dst.width) - 0.5f : 0.0f;
            int x0 = std::clamp(static_cast<int>(std::floor(fx)), 0, src.width - 1);
            int x1 = std::clamp(x0 + 1, 0, src.width - 1);
            float tx = std::clamp(fx - static_cast<float>(x0), 0.0f, 1.0f);
            for (int c = 0; c < 4; ++c) {
                float p00 = src.rgba[(static_cast<size_t>(y0) * static_cast<size_t>(src.width) + static_cast<size_t>(x0)) * 4u + static_cast<size_t>(c)];
                float p10 = src.rgba[(static_cast<size_t>(y0) * static_cast<size_t>(src.width) + static_cast<size_t>(x1)) * 4u + static_cast<size_t>(c)];
                float p01 = src.rgba[(static_cast<size_t>(y1) * static_cast<size_t>(src.width) + static_cast<size_t>(x0)) * 4u + static_cast<size_t>(c)];
                float p11 = src.rgba[(static_cast<size_t>(y1) * static_cast<size_t>(src.width) + static_cast<size_t>(x1)) * 4u + static_cast<size_t>(c)];
                float v = (p00 * (1.0f - tx) + p10 * tx) * (1.0f - ty) + (p01 * (1.0f - tx) + p11 * tx) * ty;
                dst.rgba[(static_cast<size_t>(y) * static_cast<size_t>(dst.width) + static_cast<size_t>(x)) * 4u + static_cast<size_t>(c)] =
                    static_cast<uint8_t>(std::clamp(std::lround(v), 0L, 255L));
            }
        }
    }
    return dst;
}

inline std::vector<uint8_t> encodePngRgba(const IconImage &img) {
    if (img.width <= 0 || img.height <= 0 || img.rgba.size() < static_cast<size_t>(img.width) * static_cast<size_t>(img.height) * 4u) return {};
    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    auto addChunk = [&](const char tag[4], const std::vector<uint8_t> &payload) {
        icon_detail::pushU32BE(png, static_cast<uint32_t>(payload.size()));
        size_t crcStart = png.size();
        png.insert(png.end(), tag, tag + 4);
        png.insert(png.end(), payload.begin(), payload.end());
        uint32_t c = crc32Bytes(png.data() + crcStart, png.size() - crcStart);
        icon_detail::pushU32BE(png, c);
    };

    std::vector<uint8_t> ihdr;
    icon_detail::pushU32BE(ihdr, static_cast<uint32_t>(img.width));
    icon_detail::pushU32BE(ihdr, static_cast<uint32_t>(img.height));
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(6);  // RGBA
    ihdr.push_back(0);  // compression
    ihdr.push_back(0);  // filter
    ihdr.push_back(0);  // non-interlaced
    addChunk("IHDR", ihdr);

    const size_t stride = static_cast<size_t>(img.width) * 4u;
    std::vector<uint8_t> rawLines;
    rawLines.reserve((stride + 1) * static_cast<size_t>(img.height));
    for (int y = 0; y < img.height; ++y) {
        rawLines.push_back(0); // filter type 0
        const uint8_t *row = img.rgba.data() + static_cast<size_t>(y) * stride;
        rawLines.insert(rawLines.end(), row, row + stride);
    }
    addChunk("IDAT", icon_detail::zlibCompress(rawLines));
    addChunk("IEND", {});
    return png;
}

inline bool decodePngRgba(const std::vector<uint8_t> &pngBytes, IconImage &out) {
    static constexpr uint8_t kSig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (pngBytes.size() < 33 || std::memcmp(pngBytes.data(), kSig, 8) != 0) return false;
    int w = 0, h = 0;
    uint8_t depth = 0, colorType = 0, interlace = 0;
    std::vector<uint8_t> idat, plte, trns;
    size_t pos = 8;
    while (pos + 12 <= pngBytes.size()) {
        uint32_t len = icon_detail::readU32BE(pngBytes.data() + pos);
        if (pos + 12 + len > pngBytes.size()) break;
        const char *tp = reinterpret_cast<const char *>(pngBytes.data() + pos + 4);
        const uint8_t *dat = pngBytes.data() + pos + 8;
        if (std::memcmp(tp, "IHDR", 4) == 0 && len >= 13) {
            w = static_cast<int>(icon_detail::readU32BE(dat));
            h = static_cast<int>(icon_detail::readU32BE(dat + 4));
            depth = dat[8]; colorType = dat[9]; interlace = dat[12];
        } else if (std::memcmp(tp, "PLTE", 4) == 0) {
            plte.assign(dat, dat + len);
        } else if (std::memcmp(tp, "tRNS", 4) == 0) {
            trns.assign(dat, dat + len);
        } else if (std::memcmp(tp, "IDAT", 4) == 0) {
            idat.insert(idat.end(), dat, dat + len);
        } else if (std::memcmp(tp, "IEND", 4) == 0) {
            break;
        }
        pos += 12 + len;
    }
    if (w <= 0 || h <= 0 || depth != 8 || interlace != 0 || idat.empty()) return false;
    int spp = 0;
    if (colorType == 6) spp = 4;
    else if (colorType == 2) spp = 3;
    else if (colorType == 0) spp = 1;
    else if (colorType == 4) spp = 2;
    else if (colorType == 3) spp = 1;
    else return false;

    std::vector<uint8_t> raw;
    if (!icon_detail::inflateZlib(idat, raw)) return false;
    size_t rowBytes = static_cast<size_t>(w) * static_cast<size_t>(spp);
    if (raw.size() < (rowBytes + 1) * static_cast<size_t>(h)) return false;

    std::vector<uint8_t> recon(rowBytes * static_cast<size_t>(h), 0);
    for (int y = 0; y < h; ++y) {
        uint8_t f = raw[static_cast<size_t>(y) * (rowBytes + 1)];
        const uint8_t *srcRow = raw.data() + static_cast<size_t>(y) * (rowBytes + 1) + 1;
        uint8_t *dstRow = recon.data() + static_cast<size_t>(y) * rowBytes;
        const uint8_t *prevRow = (y > 0) ? (recon.data() + static_cast<size_t>(y - 1) * rowBytes) : nullptr;
        for (size_t x = 0; x < rowBytes; ++x) {
            int a = (x >= static_cast<size_t>(spp)) ? dstRow[x - static_cast<size_t>(spp)] : 0;
            int b = prevRow ? prevRow[x] : 0;
            int c = (prevRow && x >= static_cast<size_t>(spp)) ? prevRow[x - static_cast<size_t>(spp)] : 0;
            int val = srcRow[x];
            if (f == 1) val = (val + a) & 0xFF;
            else if (f == 2) val = (val + b) & 0xFF;
            else if (f == 3) val = (val + ((a + b) >> 1)) & 0xFF;
            else if (f == 4) val = (val + icon_detail::paethPredictor(a, b, c)) & 0xFF;
            dstRow[x] = static_cast<uint8_t>(val);
        }
    }

    out.width = w;
    out.height = h;
    out.rgba.assign(static_cast<size_t>(w) * static_cast<size_t>(h) * 4u, 255);
    for (size_t i = 0; i < static_cast<size_t>(w) * static_cast<size_t>(h); ++i) {
        size_t o = i * 4u;
        const uint8_t *s = recon.data() + i * static_cast<size_t>(spp);
        if (colorType == 6) {
            out.rgba[o + 0] = s[0]; out.rgba[o + 1] = s[1]; out.rgba[o + 2] = s[2]; out.rgba[o + 3] = s[3];
        } else if (colorType == 2) {
            out.rgba[o + 0] = s[0]; out.rgba[o + 1] = s[1]; out.rgba[o + 2] = s[2]; out.rgba[o + 3] = 255;
        } else if (colorType == 0) {
            out.rgba[o + 0] = s[0]; out.rgba[o + 1] = s[0]; out.rgba[o + 2] = s[0]; out.rgba[o + 3] = 255;
        } else if (colorType == 4) {
            out.rgba[o + 0] = s[0]; out.rgba[o + 1] = s[0]; out.rgba[o + 2] = s[0]; out.rgba[o + 3] = s[1];
        } else if (colorType == 3) {
            size_t pidx = static_cast<size_t>(s[0]) * 3u;
            if (pidx + 2 < plte.size()) {
                out.rgba[o + 0] = plte[pidx + 0]; out.rgba[o + 1] = plte[pidx + 1]; out.rgba[o + 2] = plte[pidx + 2];
                out.rgba[o + 3] = (s[0] < trns.size()) ? trns[s[0]] : 255;
            }
        }
    }
    return true;
}

inline std::vector<uint8_t> buildIcnsFromRgba(const IconImage &baseIcon) {
    struct IcnsSlot { const char type[4]; int size; };
    static constexpr IcnsSlot kSlots[] = {
        {{'i','c','p','4'}, 16},
        {{'i','c','p','5'}, 32},
        {{'i','c','p','6'}, 64},
        {{'i','c','0','7'}, 128},
        {{'i','c','0','8'}, 256},
        {{'i','c','0','9'}, 512},
        {{'i','c','1','0'}, 1024}
    };
    std::vector<uint8_t> body;
    for (const auto &slot : kSlots) {
        IconImage scaled = resizeBilinearRgba(baseIcon, slot.size, slot.size);
        auto png = encodePngRgba(scaled);
        if (png.empty()) continue;
        body.insert(body.end(), slot.type, slot.type + 4);
        icon_detail::pushU32BE(body, static_cast<uint32_t>(8u + png.size()));
        body.insert(body.end(), png.begin(), png.end());
    }
    std::vector<uint8_t> icns = {'i', 'c', 'n', 's'};
    icon_detail::pushU32BE(icns, static_cast<uint32_t>(8u + body.size()));
    icns.insert(icns.end(), body.begin(), body.end());
    return icns;
}

inline std::vector<uint8_t> buildIcoFromRgba(const IconImage &baseIcon) {
    static constexpr int kSizes[4] = {16, 32, 48, 256};
    std::vector<std::vector<uint8_t>> payloads;
    for (int sz : kSizes) {
        payloads.push_back(encodePngRgba(resizeBilinearRgba(baseIcon, sz, sz)));
    }
    std::vector<uint8_t> ico;
    icon_detail::pushU16LE(ico, 0); // reserved
    icon_detail::pushU16LE(ico, 1); // type ICON
    icon_detail::pushU16LE(ico, 4); // count
    uint32_t offset = 6 + 4 * 16;
    for (size_t i = 0; i < 4; ++i) {
        int sz = kSizes[i];
        uint8_t dim = (sz >= 256) ? 0 : static_cast<uint8_t>(sz);
        ico.push_back(dim); // width
        ico.push_back(dim); // height
        ico.push_back(0);   // color count
        ico.push_back(0);   // reserved
        icon_detail::pushU16LE(ico, 1);  // planes
        icon_detail::pushU16LE(ico, 32); // bpp
        icon_detail::pushU32LE(ico, static_cast<uint32_t>(payloads[i].size()));
        icon_detail::pushU32LE(ico, offset);
        offset += static_cast<uint32_t>(payloads[i].size());
    }
    for (const auto &p : payloads) ico.insert(ico.end(), p.begin(), p.end());
    return ico;
}

inline IconImage loadOrDefaultIcon(const std::string &iconPath, int defaultSize = 512) {
    if (!iconPath.empty() && std::filesystem::exists(iconPath)) {
        std::ifstream ifs(iconPath, std::ios::binary);
        if (ifs.is_open()) {
            std::vector<uint8_t> raw((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            IconImage img;
            if (decodePngRgba(raw, img)) {
                return (img.width == defaultSize && img.height == defaultSize) ? img : resizeBilinearRgba(img, defaultSize, defaultSize);
            }
        }
#if defined(__APPLE__)
        std::filesystem::path tmpPng = std::filesystem::temp_directory_path() / "meme_icon_conv.png";
        std::string cmd = "/usr/bin/sips -s format png \"" + iconPath + "\" --out \"" + tmpPng.string() + "\" >/dev/null 2>&1";
        if (std::system(cmd.c_str()) == 0 && std::filesystem::exists(tmpPng)) {
            std::ifstream tf(tmpPng, std::ios::binary);
            std::vector<uint8_t> png((std::istreambuf_iterator<char>(tf)), std::istreambuf_iterator<char>());
            std::error_code ec;
            std::filesystem::remove(tmpPng, ec);
            IconImage img;
            if (decodePngRgba(png, img)) {
                return (img.width == defaultSize && img.height == defaultSize) ? img : resizeBilinearRgba(img, defaultSize, defaultSize);
            }
        }
#endif
    }
    IconImage defImg;
    defImg.width = defaultSize;
    defImg.height = defaultSize;
    defImg.rgba = MemeNative::buildDefaultSquircleIconRgba(defaultSize);
    return defImg;
}

} // namespace MemePackager

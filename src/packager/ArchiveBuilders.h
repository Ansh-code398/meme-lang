#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace MemePackager {

struct ArchiveFileEntry {
    std::string path;      // e.g. "./usr/local/bin/myapp" or "/usr/local/bin/myapp"
    std::vector<uint8_t> data;
    uint32_t mode = 0644;
    bool isDir = false;
};

inline uint32_t crc32Bytes(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

inline std::vector<uint8_t> gzipBytes(const std::vector<uint8_t> &raw) {
    // Prefer system gzip for true compressed archives; fallback to RFC-1952 stored DEFLATE
    std::filesystem::path tmpIn = std::filesystem::temp_directory_path() / "meme_pkg_gz_in.bin";
    {
        std::ofstream os(tmpIn, std::ios::binary);
        if (os.is_open() && !raw.empty()) {
            os.write(reinterpret_cast<const char *>(raw.data()), static_cast<std::streamsize>(raw.size()));
        }
    }
    std::string cmd = "gzip -n -c \"" + tmpIn.string() + "\" 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        std::vector<uint8_t> out;
        uint8_t buf[8192];
        size_t n = 0;
        while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0) {
            out.insert(out.end(), buf, buf + n);
        }
        int status = pclose(pipe);
        std::error_code ec;
        std::filesystem::remove(tmpIn, ec);
        if (status == 0 && out.size() >= 18) return out;
    }

    // Stored RFC-1952 fallback
    std::vector<uint8_t> gz = {0x1F, 0x8B, 0x08, 0x00, 0, 0, 0, 0, 0x02, 0x03};
    size_t pos = 0;
    do {
        size_t rem = raw.size() - pos;
        uint16_t chunk = static_cast<uint16_t>(std::min<size_t>(65535, rem));
        bool last = (pos + chunk >= raw.size());
        gz.push_back(last ? 0x01 : 0x00);
        gz.push_back(static_cast<uint8_t>(chunk & 0xFF));
        gz.push_back(static_cast<uint8_t>((chunk >> 8) & 0xFF));
        uint16_t nlen = static_cast<uint16_t>(~chunk);
        gz.push_back(static_cast<uint8_t>(nlen & 0xFF));
        gz.push_back(static_cast<uint8_t>((nlen >> 8) & 0xFF));
        if (chunk > 0) gz.insert(gz.end(), raw.begin() + static_cast<ptrdiff_t>(pos), raw.begin() + static_cast<ptrdiff_t>(pos + chunk));
        pos += chunk;
    } while (pos < raw.size());

    uint32_t crc = crc32Bytes(raw.data(), raw.size());
    uint32_t isz = static_cast<uint32_t>(raw.size());
    for (int i = 0; i < 4; ++i) gz.push_back(static_cast<uint8_t>((crc >> (i * 8)) & 0xFF));
    for (int i = 0; i < 4; ++i) gz.push_back(static_cast<uint8_t>((isz >> (i * 8)) & 0xFF));
    return gz;
}

inline std::vector<uint8_t> buildTar(const std::vector<ArchiveFileEntry> &entries) {
    std::vector<uint8_t> out;
    auto writeOctal = [](uint8_t *dst, size_t width, uint64_t val) {
        std::string s(width, '0');
        s[width - 1] = '\0';
        for (int i = static_cast<int>(width) - 2; i >= 0; --i) {
            s[static_cast<size_t>(i)] = static_cast<char>('0' + (val & 7));
            val >>= 3;
        }
        std::memcpy(dst, s.data(), width);
    };

    for (const auto &e : entries) {
        std::array<uint8_t, 512> hdr{};
        hdr.fill(0);
        std::string path = e.path;
        if (e.isDir && !path.empty() && path.back() != '/') path += '/';
        std::string namePart = path, prefixPart;
        if (path.size() > 100) {
            size_t slash = path.rfind('/', 154);
            if (slash != std::string::npos && path.size() - slash - 1 <= 100) {
                prefixPart = path.substr(0, slash);
                namePart = path.substr(slash + 1);
            } else {
                namePart = path.substr(0, 100);
            }
        }
        std::memcpy(hdr.data() + 0, namePart.data(), std::min<size_t>(100, namePart.size()));
        writeOctal(hdr.data() + 100, 8, e.mode);
        writeOctal(hdr.data() + 108, 8, 0);
        writeOctal(hdr.data() + 116, 8, 0);
        writeOctal(hdr.data() + 124, 12, e.isDir ? 0 : e.data.size());
        writeOctal(hdr.data() + 136, 12, 1730000000ULL);
        std::memset(hdr.data() + 148, ' ', 8);
        hdr[156] = e.isDir ? '5' : '0';
        std::memcpy(hdr.data() + 257, "ustar\0", 6);
        std::memcpy(hdr.data() + 263, "00", 2);
        std::memcpy(hdr.data() + 265, "root", 4);
        std::memcpy(hdr.data() + 297, "root", 4);
        if (!prefixPart.empty()) std::memcpy(hdr.data() + 345, prefixPart.data(), std::min<size_t>(155, prefixPart.size()));

        uint32_t sum = 0;
        for (uint8_t b : hdr) sum += b;
        writeOctal(hdr.data() + 148, 7, sum);
        hdr[155] = ' ';

        out.insert(out.end(), hdr.begin(), hdr.end());
        if (!e.isDir && !e.data.empty()) {
            out.insert(out.end(), e.data.begin(), e.data.end());
            size_t rem = e.data.size() % 512;
            if (rem != 0) out.insert(out.end(), 512 - rem, 0);
        }
    }
    out.insert(out.end(), 1024, 0);
    return out;
}

inline std::vector<uint8_t> buildArArchive(const std::vector<std::pair<std::string, std::vector<uint8_t>>> &members) {
    std::vector<uint8_t> out;
    const char magic[] = "!<arch>\n";
    out.insert(out.end(), magic, magic + 8);
    for (const auto &[name, data] : members) {
        char hdr[61];
        std::snprintf(hdr, sizeof(hdr), "%-16.16s%-12d%-6d%-6d%-8o%-10zu`\n",
                      name.c_str(), 1730000000, 0, 0, 0100644, data.size());
        out.insert(out.end(), hdr, hdr + 60);
        out.insert(out.end(), data.begin(), data.end());
        if ((data.size() % 2) != 0) out.push_back('\n');
    }
    return out;
}

inline std::vector<uint8_t> buildCpioNewc(const std::vector<ArchiveFileEntry> &entries) {
    std::vector<uint8_t> out;
    auto pushPad4 = [&]() {
        while ((out.size() % 4) != 0) out.push_back(0);
    };
    uint32_t ino = 1;
    for (const auto &e : entries) {
        std::string p = e.path;
        while (!p.empty() && p.front() == '/') p.erase(p.begin());
        if (p.empty()) p = ".";
        uint32_t mode = e.isDir ? (0040000u | (e.mode & 0777u)) : (0100000u | (e.mode & 0777u));
        uint32_t sz = e.isDir ? 0u : static_cast<uint32_t>(e.data.size());
        uint32_t namesz = static_cast<uint32_t>(p.size() + 1);
        char hdr[111];
        std::snprintf(hdr, sizeof(hdr),
                      "070701%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X",
                      ino++, mode, 0u, 0u, e.isDir ? 2u : 1u, 1730000000u, sz,
                      0u, 0u, 0u, 0u, namesz, 0u);
        out.insert(out.end(), hdr, hdr + 110);
        out.insert(out.end(), p.begin(), p.end());
        out.push_back(0);
        pushPad4();
        if (!e.isDir && sz > 0) {
            out.insert(out.end(), e.data.begin(), e.data.end());
            pushPad4();
        }
    }
    const char trailer[] = "TRAILER!!!";
    char thdr[111];
    std::snprintf(thdr, sizeof(thdr),
                  "070701%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X",
                  0u, 0u, 0u, 0u, 1u, 0u, 0u, 0u, 0u, 0u, 0u, static_cast<uint32_t>(sizeof(trailer)), 0u);
    out.insert(out.end(), thdr, thdr + 110);
    out.insert(out.end(), trailer, trailer + sizeof(trailer));
    pushPad4();
    return out;
}

class RpmHeaderBuilder {
    struct Entry {
        uint32_t tag;
        uint32_t type;
        uint32_t offset;
        uint32_t count;
    };
    std::vector<Entry> entries_;
    std::vector<uint8_t> store_;

    void align(size_t a) {
        while ((store_.size() % a) != 0) store_.push_back(0);
    }
    static void pushBE16(std::vector<uint8_t> &v, uint16_t val) {
        v.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        v.push_back(static_cast<uint8_t>(val & 0xFF));
    }
    static void pushBE32(std::vector<uint8_t> &v, uint32_t val) {
        v.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
        v.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        v.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        v.push_back(static_cast<uint8_t>(val & 0xFF));
    }
public:
    void addInt16Array(uint32_t tag, const std::vector<uint16_t> &vals) {
        align(2);
        uint32_t off = static_cast<uint32_t>(store_.size());
        for (uint16_t x : vals) pushBE16(store_, x);
        entries_.push_back({tag, 3, off, static_cast<uint32_t>(vals.size())});
    }
    void addInt32(uint32_t tag, uint32_t val) {
        addInt32Array(tag, {val});
    }
    void addInt32Array(uint32_t tag, const std::vector<uint32_t> &vals) {
        align(4);
        uint32_t off = static_cast<uint32_t>(store_.size());
        for (uint32_t x : vals) pushBE32(store_, x);
        entries_.push_back({tag, 4, off, static_cast<uint32_t>(vals.size())});
    }
    void addString(uint32_t tag, const std::string &s) {
        uint32_t off = static_cast<uint32_t>(store_.size());
        store_.insert(store_.end(), s.begin(), s.end());
        store_.push_back(0);
        entries_.push_back({tag, 6, off, 1});
    }
    void addStringArray(uint32_t tag, const std::vector<std::string> &strs) {
        uint32_t off = static_cast<uint32_t>(store_.size());
        for (const auto &s : strs) {
            store_.insert(store_.end(), s.begin(), s.end());
            store_.push_back(0);
        }
        entries_.push_back({tag, 8, off, static_cast<uint32_t>(strs.size())});
    }
    std::vector<uint8_t> build(bool pad8 = false) {
        std::sort(entries_.begin(), entries_.end(), [](const Entry &a, const Entry &b) { return a.tag < b.tag; });
        std::vector<uint8_t> out = {0x8E, 0xAD, 0xE8, 0x01, 0x00, 0x00, 0x00, 0x00};
        pushBE32(out, static_cast<uint32_t>(entries_.size()));
        pushBE32(out, static_cast<uint32_t>(store_.size()));
        for (const auto &e : entries_) {
            pushBE32(out, e.tag);
            pushBE32(out, e.type);
            pushBE32(out, e.offset);
            pushBE32(out, e.count);
        }
        out.insert(out.end(), store_.begin(), store_.end());
        if (pad8) {
            while ((out.size() % 8) != 0) out.push_back(0);
        }
        return out;
    }
};

inline std::vector<uint8_t> buildRpmPackage(
    const std::string &pkgName,
    const std::string &version,
    const std::string &release,
    const std::string &arch,
    const std::string &summary,
    const std::vector<ArchiveFileEntry> &files)
{
    std::vector<uint8_t> cpioRaw = buildCpioNewc(files);
    std::vector<uint8_t> payloadGz = gzipBytes(cpioRaw);

    std::vector<uint32_t> fileSizes, fileFlags, dirIndexes;
    std::vector<uint16_t> fileModes;
    std::vector<std::string> baseNames, dirNames, fileMd5s, fileUsers, fileGroups;
    uint32_t totalSize = 0;

    for (const auto &f : files) {
        if (f.isDir) continue;
        std::string full = f.path;
        if (full.empty() || full.front() != '/') full = "/" + full;
        size_t sl = full.rfind('/');
        std::string dir = (sl == std::string::npos) ? "/" : full.substr(0, sl + 1);
        std::string base = (sl == std::string::npos) ? full : full.substr(sl + 1);
        uint32_t dIdx = 0;
        auto dit = std::find(dirNames.begin(), dirNames.end(), dir);
        if (dit == dirNames.end()) {
            dIdx = static_cast<uint32_t>(dirNames.size());
            dirNames.push_back(dir);
        } else {
            dIdx = static_cast<uint32_t>(dit - dirNames.begin());
        }
        uint32_t sz = static_cast<uint32_t>(f.data.size());
        totalSize += sz;
        fileSizes.push_back(sz);
        fileModes.push_back(static_cast<uint16_t>(0100000u | (f.mode & 0777u)));
        fileFlags.push_back(0);
        dirIndexes.push_back(dIdx);
        baseNames.push_back(base);
        fileMd5s.emplace_back("");
        fileUsers.emplace_back("root");
        fileGroups.emplace_back("root");
    }

    RpmHeaderBuilder mainHdr;
    mainHdr.addString(1000, pkgName);
    mainHdr.addString(1001, version);
    mainHdr.addString(1002, release);
    mainHdr.addString(1004, summary);
    mainHdr.addString(1005, summary + " — Built with MEME-UI Native Compiler");
    mainHdr.addInt32(1009, totalSize);
    mainHdr.addString(1014, "MIT");
    mainHdr.addString(1016, "Applications/System");
    mainHdr.addString(1021, "linux");
    mainHdr.addString(1022, arch);
    mainHdr.addInt32Array(1028, fileSizes);
    mainHdr.addInt16Array(1030, fileModes);
    mainHdr.addStringArray(1035, fileMd5s);
    mainHdr.addInt32Array(1036, fileFlags);
    mainHdr.addStringArray(1039, fileUsers);
    mainHdr.addStringArray(1040, fileGroups);
    mainHdr.addInt32Array(1116, dirIndexes);
    mainHdr.addStringArray(1117, baseNames);
    mainHdr.addStringArray(1118, dirNames);
    mainHdr.addString(1124, "cpio");
    mainHdr.addString(1125, "gzip");
    mainHdr.addString(1126, "9");
    std::vector<uint8_t> mainBytes = mainHdr.build(false);

    RpmHeaderBuilder sigHdr;
    sigHdr.addInt32(1000, static_cast<uint32_t>(mainBytes.size() + payloadGz.size()));
    sigHdr.addInt32(1007, static_cast<uint32_t>(cpioRaw.size()));
    std::vector<uint8_t> sigBytes = sigHdr.build(true);

    std::array<uint8_t, 96> lead{};
    lead.fill(0);
    lead[0] = 0xED; lead[1] = 0xAB; lead[2] = 0xEE; lead[3] = 0xDB;
    lead[4] = 3; lead[5] = 0; // RPM v3 lead
    lead[6] = 0; lead[7] = 0; // Binary package
    uint16_t archNum = (arch == "aarch64") ? 19 : 1;
    lead[8] = static_cast<uint8_t>((archNum >> 8) & 0xFF);
    lead[9] = static_cast<uint8_t>(archNum & 0xFF);
    std::string nevr = pkgName + "-" + version + "-" + release;
    std::memcpy(lead.data() + 10, nevr.data(), std::min<size_t>(65, nevr.size()));
    lead[76] = 0; lead[77] = 1; // Linux
    lead[78] = 0; lead[79] = 5; // Header-style signature

    std::vector<uint8_t> rpm;
    rpm.reserve(lead.size() + sigBytes.size() + mainBytes.size() + payloadGz.size());
    rpm.insert(rpm.end(), lead.begin(), lead.end());
    rpm.insert(rpm.end(), sigBytes.begin(), sigBytes.end());
    rpm.insert(rpm.end(), mainBytes.begin(), mainBytes.end());
    rpm.insert(rpm.end(), payloadGz.begin(), payloadGz.end());
    return rpm;
}

} // namespace MemePackager

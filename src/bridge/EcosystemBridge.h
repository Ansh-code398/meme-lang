#ifndef ECOSYSTEM_BRIDGE_H
#define ECOSYSTEM_BRIDGE_H

#include "../interpreter/Value.h"
#include "../interpreter/Callable.h"
#include "../interpreter/Interpreter.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <chrono>
#include <random>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <dlfcn.h>
#endif

namespace EcosystemBridge
{

// ---------------------------------------------------------------------------
// Standard Self-Contained SHA-256 & HMAC
// ---------------------------------------------------------------------------

namespace Crypto
{
    inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
    inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    inline uint32_t ep0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    inline uint32_t ep1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    inline uint32_t sig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    inline uint32_t sig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    static const uint32_t K[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    inline std::vector<uint8_t> sha256Bytes(const std::vector<uint8_t> &data)
    {
        uint32_t h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a;
        uint32_t h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

        uint64_t bitLen = data.size() * 8;
        std::vector<uint8_t> msg = data;
        msg.push_back(0x80);
        while ((msg.size() % 64) != 56)
        {
            msg.push_back(0x00);
        }
        for (int i = 7; i >= 0; --i)
        {
            msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));
        }

        for (size_t chunk = 0; chunk < msg.size(); chunk += 64)
        {
            uint32_t w[64];
            for (size_t i = 0; i < 16; ++i)
            {
                w[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 3]));
            }
            for (size_t i = 16; i < 64; ++i)
            {
                w[i] = sig1(w[i - 2]) + w[i - 7] + sig0(w[i - 15]) + w[i - 16];
            }

            uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;
            for (size_t i = 0; i < 64; ++i)
            {
                uint32_t t1 = h + ep1(e) + ch(e, f, g) + K[i] + w[i];
                uint32_t t2 = ep0(a) + maj(a, b, c);
                h = g;
                g = f;
                f = e;
                e = d + t1;
                d = c;
                c = b;
                b = a;
                a = t1 + t2;
            }

            h0 += a; h1 += b; h2 += c; h3 += d;
            h4 += e; h5 += f; h6 += g; h7 += h;
        }

        std::vector<uint8_t> digest(32);
        uint32_t hs[8] = {h0, h1, h2, h3, h4, h5, h6, h7};
        for (int i = 0; i < 8; ++i)
        {
            digest[i * 4] = static_cast<uint8_t>((hs[i] >> 24) & 0xFF);
            digest[i * 4 + 1] = static_cast<uint8_t>((hs[i] >> 16) & 0xFF);
            digest[i * 4 + 2] = static_cast<uint8_t>((hs[i] >> 8) & 0xFF);
            digest[i * 4 + 3] = static_cast<uint8_t>(hs[i] & 0xFF);
        }
        return digest;
    }

    inline std::string toHex(const std::vector<uint8_t> &bytes)
    {
        std::stringstream ss;
        for (uint8_t b : bytes)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        }
        return ss.str();
    }

    inline std::string sha256(const std::string &str)
    {
        std::vector<uint8_t> data(str.begin(), str.end());
        return toHex(sha256Bytes(data));
    }

    inline std::string hmacSha256(const std::string &keyStr, const std::string &msgStr)
    {
        std::vector<uint8_t> key(keyStr.begin(), keyStr.end());
        if (key.size() > 64)
        {
            key = sha256Bytes(key);
        }
        key.resize(64, 0x00);

        std::vector<uint8_t> o_key_pad(64), i_key_pad(64);
        for (size_t i = 0; i < 64; ++i)
        {
            o_key_pad[i] = key[i] ^ 0x5c;
            i_key_pad[i] = key[i] ^ 0x36;
        }

        std::vector<uint8_t> innerData = i_key_pad;
        innerData.insert(innerData.end(), msgStr.begin(), msgStr.end());
        std::vector<uint8_t> innerHash = sha256Bytes(innerData);

        std::vector<uint8_t> outerData = o_key_pad;
        outerData.insert(outerData.end(), innerHash.begin(), innerHash.end());
        return toHex(sha256Bytes(outerData));
    }

    inline std::string uuid()
    {
        static std::mt19937_64 rng(std::random_device{}());
        static std::uniform_int_distribution<uint64_t> dist;
        uint64_t part1 = dist(rng);
        uint64_t part2 = dist(rng);

        uint8_t bytes[16];
        for (int i = 0; i < 8; ++i) bytes[i] = static_cast<uint8_t>((part1 >> (i * 8)) & 0xFF);
        for (int i = 0; i < 8; ++i) bytes[8 + i] = static_cast<uint8_t>((part2 >> (i * 8)) & 0xFF);

        bytes[6] = (bytes[6] & 0x0F) | 0x40; // Version 4
        bytes[8] = (bytes[8] & 0x3F) | 0x80; // Variant 1

        std::stringstream ss;
        for (int i = 0; i < 16; ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10) ss << "-";
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
        }
        return ss.str();
    }

    inline std::string md5(const std::string &str)
    {
        static const uint32_t s[64] = {
            7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
            5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
            4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
            6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
        };
        static const uint32_t K[64] = {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
            0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
            0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
            0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
            0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
            0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
            0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
            0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
            0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
        };

        uint32_t a0 = 0x67452301, b0 = 0xefcdab89, c0 = 0x98badcfe, d0 = 0x10325476;

        std::vector<uint8_t> msg(str.begin(), str.end());
        uint64_t bitLen = static_cast<uint64_t>(msg.size()) * 8;
        msg.push_back(0x80);
        while ((msg.size() % 64) != 56) msg.push_back(0x00);
        for (int i = 0; i < 8; ++i) msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));

        for (size_t chunk = 0; chunk < msg.size(); chunk += 64)
        {
            uint32_t M[16];
            for (size_t i = 0; i < 16; ++i)
            {
                M[i] = static_cast<uint32_t>(msg[chunk + i * 4]) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 8) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 16) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 3]) << 24);
            }

            uint32_t A = a0, B = b0, C = c0, D = d0;
            for (size_t i = 0; i < 64; ++i)
            {
                uint32_t F, g;
                if (i < 16) {
                    F = (B & C) | ((~B) & D);
                    g = i;
                } else if (i < 32) {
                    F = (D & B) | ((~D) & C);
                    g = (5 * i + 1) % 16;
                } else if (i < 48) {
                    F = B ^ C ^ D;
                    g = (3 * i + 5) % 16;
                } else {
                    F = C ^ (B | (~D));
                    g = (7 * i) % 16;
                }
                uint32_t dTemp = D;
                D = C;
                C = B;
                uint32_t sum = A + F + K[i] + M[g];
                uint32_t rot = (sum << s[i]) | (sum >> (32 - s[i]));
                B = B + rot;
                A = dTemp;
            }
            a0 += A; b0 += B; c0 += C; d0 += D;
        }

        std::vector<uint8_t> digest(16);
        uint32_t res[4] = {a0, b0, c0, d0};
        for (int i = 0; i < 4; ++i)
        {
            digest[i * 4] = static_cast<uint8_t>(res[i] & 0xFF);
            digest[i * 4 + 1] = static_cast<uint8_t>((res[i] >> 8) & 0xFF);
            digest[i * 4 + 2] = static_cast<uint8_t>((res[i] >> 16) & 0xFF);
            digest[i * 4 + 3] = static_cast<uint8_t>((res[i] >> 24) & 0xFF);
        }
        return toHex(digest);
    }

    inline std::string sha1(const std::string &str)
    {
        uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;
        std::vector<uint8_t> msg(str.begin(), str.end());
        uint64_t bitLen = static_cast<uint64_t>(msg.size()) * 8;
        msg.push_back(0x80);
        while ((msg.size() % 64) != 56) msg.push_back(0x00);
        for (int i = 7; i >= 0; --i) msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));

        for (size_t chunk = 0; chunk < msg.size(); chunk += 64)
        {
            uint32_t w[80];
            for (size_t i = 0; i < 16; ++i)
            {
                w[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 3]));
            }
            for (size_t i = 16; i < 80; ++i)
            {
                uint32_t val = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
                w[i] = (val << 1) | (val >> 31);
            }

            uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
            for (size_t i = 0; i < 80; ++i)
            {
                uint32_t f, k;
                if (i < 20) {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                } else if (i < 40) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                } else if (i < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                } else {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }
                uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
                e = d;
                d = c;
                c = (b << 30) | (b >> 2);
                b = a;
                a = temp;
            }
            h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
        }

        std::vector<uint8_t> digest(20);
        uint32_t hs[5] = {h0, h1, h2, h3, h4};
        for (int i = 0; i < 5; ++i)
        {
            digest[i * 4] = static_cast<uint8_t>((hs[i] >> 24) & 0xFF);
            digest[i * 4 + 1] = static_cast<uint8_t>((hs[i] >> 16) & 0xFF);
            digest[i * 4 + 2] = static_cast<uint8_t>((hs[i] >> 8) & 0xFF);
            digest[i * 4 + 3] = static_cast<uint8_t>(hs[i] & 0xFF);
        }
        return toHex(digest);
    }

    inline std::string base64Encode(const std::string &in)
    {
        static const std::string chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        std::string out;
        int val = 0, valb = -6;
        for (unsigned char c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    inline std::string base64Decode(const std::string &in)
    {
        static const std::string chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        std::string out;
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) T[static_cast<unsigned char>(chars[i])] = i;
        int val = 0, valb = -8;
        for (unsigned char c : in) {
            if (c == '=') break;
            if (T[static_cast<unsigned char>(c)] == -1) continue;
            val = (val << 6) + T[static_cast<unsigned char>(c)];
            valb += 6;
            if (valb >= 0) {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }
}

// ---------------------------------------------------------------------------
// Python Bridge: py:numpy, py:pandas, and generic py:<pkg>
// ---------------------------------------------------------------------------

inline Value createNumPyModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto meanFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) return Value(0.0);
        Value d = args[0].deref();
        if (!d.isList()) return Value(0.0);
        const auto &elems = d.getList()->elements;
        if (elems.empty()) return Value(0.0);
        double sum = 0.0;
        for (const auto &e : elems) {
            Value ed = e.deref();
            if (std::holds_alternative<long long>(ed.data)) sum += std::get<long long>(ed.data);
            else if (std::holds_alternative<double>(ed.data)) sum += std::get<double>(ed.data);
        }
        return Value(sum / elems.size());
    };
    m->set("mean", Value(std::make_shared<NativeMethodCallable>("mean", 1, meanFn)));

    auto sumFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) return Value(0.0);
        Value d = args[0].deref();
        if (!d.isList()) return Value(0.0);
        const auto &elems = d.getList()->elements;
        double sum = 0.0;
        bool hasFloat = false;
        long long intSum = 0;
        for (const auto &e : elems) {
            Value ed = e.deref();
            if (std::holds_alternative<long long>(ed.data)) {
                intSum += std::get<long long>(ed.data);
                sum += std::get<long long>(ed.data);
            } else if (std::holds_alternative<double>(ed.data)) {
                hasFloat = true;
                sum += std::get<double>(ed.data);
            }
        }
        return hasFloat ? Value(sum) : Value(intSum);
    };
    m->set("sum", Value(std::make_shared<NativeMethodCallable>("sum", 1, sumFn)));

    auto stdFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) return Value(0.0);
        Value d = args[0].deref();
        if (!d.isList()) return Value(0.0);
        const auto &elems = d.getList()->elements;
        if (elems.size() <= 1) return Value(0.0);
        double sum = 0.0;
        for (const auto &e : elems) {
            Value ed = e.deref();
            if (std::holds_alternative<long long>(ed.data)) sum += std::get<long long>(ed.data);
            else if (std::holds_alternative<double>(ed.data)) sum += std::get<double>(ed.data);
        }
        double mean = sum / elems.size();
        double variance = 0.0;
        for (const auto &e : elems) {
            Value ed = e.deref();
            double val = 0.0;
            if (std::holds_alternative<long long>(ed.data)) val = std::get<long long>(ed.data);
            else if (std::holds_alternative<double>(ed.data)) val = std::get<double>(ed.data);
            variance += (val - mean) * (val - mean);
        }
        return Value(std::sqrt(variance / elems.size()));
    };
    m->set("std", Value(std::make_shared<NativeMethodCallable>("std", 1, stdFn)));

    auto zerosFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        long long n = 0;
        if (!args.empty()) {
            Value d = args[0].deref();
            if (std::holds_alternative<long long>(d.data)) n = std::get<long long>(d.data);
            else if (std::holds_alternative<double>(d.data)) n = static_cast<long long>(std::get<double>(d.data));
        }
        auto l = std::make_shared<MemeList>();
        for (long long i = 0; i < n; ++i) l->elements.push_back(Value(0.0));
        return Value(l);
    };
    m->set("zeros", Value(std::make_shared<NativeMethodCallable>("zeros", 1, zerosFn)));

    auto onesFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        long long n = 0;
        if (!args.empty()) {
            Value d = args[0].deref();
            if (std::holds_alternative<long long>(d.data)) n = std::get<long long>(d.data);
            else if (std::holds_alternative<double>(d.data)) n = static_cast<long long>(std::get<double>(d.data));
        }
        auto l = std::make_shared<MemeList>();
        for (long long i = 0; i < n; ++i) l->elements.push_back(Value(1.0));
        return Value(l);
    };
    m->set("ones", Value(std::make_shared<NativeMethodCallable>("ones", 1, onesFn)));

    auto arangeFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        long long start = 0, stop = 0, step = 1;
        if (args.size() == 1) {
            Value d = args[0].deref();
            stop = std::holds_alternative<long long>(d.data) ? std::get<long long>(d.data) : static_cast<long long>(std::get<double>(d.data));
        } else if (args.size() >= 2) {
            Value d1 = args[0].deref();
            Value d2 = args[1].deref();
            start = std::holds_alternative<long long>(d1.data) ? std::get<long long>(d1.data) : static_cast<long long>(std::get<double>(d1.data));
            stop = std::holds_alternative<long long>(d2.data) ? std::get<long long>(d2.data) : static_cast<long long>(std::get<double>(d2.data));
            if (args.size() >= 3) {
                Value d3 = args[2].deref();
                step = std::holds_alternative<long long>(d3.data) ? std::get<long long>(d3.data) : static_cast<long long>(std::get<double>(d3.data));
            }
        }
        auto l = std::make_shared<MemeList>();
        if (step > 0) {
            for (long long v = start; v < stop; v += step) l->elements.push_back(Value(v));
        }
        return Value(l);
    };
    m->set("arange", Value(std::make_shared<NativeMethodCallable>("arange", 1, 3, arangeFn)));

    auto dotFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.size() < 2) return Value(0.0);
        Value a = args[0].deref();
        Value b = args[1].deref();
        if (!a.isList() || !b.isList()) return Value(0.0);
        const auto &al = a.getList()->elements;
        const auto &bl = b.getList()->elements;
        size_t n = std::min(al.size(), bl.size());
        double sum = 0.0;
        for (size_t i = 0; i < n; ++i) {
            double va = std::holds_alternative<long long>(al[i].deref().data) ? std::get<long long>(al[i].deref().data) : std::get<double>(al[i].deref().data);
            double vb = std::holds_alternative<long long>(bl[i].deref().data) ? std::get<long long>(bl[i].deref().data) : std::get<double>(bl[i].deref().data);
            sum += va * vb;
        }
        return Value(sum);
    };
    m->set("dot", Value(std::make_shared<NativeMethodCallable>("dot", 2, dotFn)));

    auto transposeFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) return Value(std::make_shared<MemeList>());
        Value mVal = args[0].deref();
        if (!mVal.isList()) return mVal;
        const auto &rows = mVal.getList()->elements;
        if (rows.empty()) return Value(std::make_shared<MemeList>());
        size_t rowCount = rows.size();
        size_t colCount = rows[0].deref().isList() ? rows[0].deref().getList()->elements.size() : 0;
        if (colCount == 0) return mVal;

        auto res = std::make_shared<MemeList>();
        for (size_t c = 0; c < colCount; ++c) {
            auto newRow = std::make_shared<MemeList>();
            for (size_t r = 0; r < rowCount; ++r) {
                if (rows[r].deref().isList() && c < rows[r].deref().getList()->elements.size()) {
                    newRow->elements.push_back(rows[r].deref().getList()->elements[c]);
                }
            }
            res->elements.push_back(Value(newRow));
        }
        return Value(res);
    };
    m->set("transpose", Value(std::make_shared<NativeMethodCallable>("transpose", 1, transposeFn)));

    auto arrayFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) return Value(std::make_shared<MemeList>());
        return args[0];
    };
    m->set("array", Value(std::make_shared<NativeMethodCallable>("array", 1, arrayFn)));

    return Value(m);
}

inline Value createDataFrameObject(const Value &dataVal)
{
    auto df = std::make_shared<MemeMap>();
    df->isModule = true;
    std::shared_ptr<MemeList> rowsList = std::make_shared<MemeList>();

    Value d = dataVal.deref();
    if (d.isList())
    {
        rowsList = d.getList();
    }
    df->set("data", Value(rowsList));

    auto headFn = [rowsList](Interpreter &, const std::vector<Value> &args) -> Value {
        long long n = 5;
        if (!args.empty()) {
            Value d = args[0].deref();
            if (std::holds_alternative<long long>(d.data)) n = std::get<long long>(d.data);
            else if (std::holds_alternative<double>(d.data)) n = static_cast<long long>(std::get<double>(d.data));
        }
        auto subList = std::make_shared<MemeList>();
        for (long long i = 0; i < n && i < static_cast<long long>(rowsList->elements.size()); ++i) {
            subList->elements.push_back(rowsList->elements[i]);
        }
        return createDataFrameObject(Value(subList));
    };
    df->set("head", Value(std::make_shared<NativeMethodCallable>("head", 0, 1, headFn)));

    auto filterRowsFn = [rowsList](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.size() < 2) return Value(rowsList);
        std::string col = args[0].deref().toString();
        Value targetVal = args[1].deref();

        auto filtered = std::make_shared<MemeList>();
        for (const auto &r : rowsList->elements) {
            Value rd = r.deref();
            if (rd.isMap()) {
                auto m = rd.getMap();
                if (m->has(col) && m->entries[col].deref() == targetVal) {
                    filtered->elements.push_back(r);
                }
            }
        }
        return createDataFrameObject(Value(filtered));
    };
    df->set("filter_rows", Value(std::make_shared<NativeMethodCallable>("filter_rows", 2, filterRowsFn)));

    auto colFn = [rowsList](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) return Value(std::make_shared<MemeList>());
        std::string col = args[0].deref().toString();
        auto colList = std::make_shared<MemeList>();
        for (const auto &r : rowsList->elements) {
            Value rd = r.deref();
            if (rd.isMap()) {
                auto m = rd.getMap();
                if (m->has(col)) {
                    colList->elements.push_back(m->entries[col]);
                }
            }
        }
        return Value(colList);
    };
    df->set("col", Value(std::make_shared<NativeMethodCallable>("col", 1, colFn)));

    auto toListFn = [rowsList](Interpreter &, const std::vector<Value> &) -> Value {
        return Value(rowsList);
    };
    df->set("to_list", Value(std::make_shared<NativeMethodCallable>("to_list", 0, toListFn)));

    auto describeFn = [rowsList](Interpreter &, const std::vector<Value> &) -> Value {
        auto summary = std::make_shared<MemeMap>();
        summary->set("count", Value(static_cast<long long>(rowsList->elements.size())));
        return Value(summary);
    };
    df->set("describe", Value(std::make_shared<NativeMethodCallable>("describe", 0, describeFn)));

    return Value(df);
}

inline Value createPandasModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto dfFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        Value data = args.empty() ? Value(std::make_shared<MemeList>()) : args[0];
        return createDataFrameObject(data);
    };
    m->set("DataFrame", Value(std::make_shared<NativeMethodCallable>("DataFrame", 0, 1, dfFn)));

    auto readCsvFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        if (args.empty()) return createDataFrameObject(Value(std::make_shared<MemeList>()));
        std::string path = args[0].deref().toString();
        std::ifstream file(path);
        std::string content;
        if (file.is_open()) {
            std::stringstream ss;
            ss << file.rdbuf();
            content = ss.str();
        } else {
            content = path; // inline CSV fallback
        }

        std::stringstream ss(content);
        std::string line;
        std::vector<std::string> headers;
        auto rows = std::make_shared<MemeList>();

        if (std::getline(ss, line)) {
            std::stringstream lss(line);
            std::string item;
            while (std::getline(lss, item, ',')) {
                size_t s = item.find_first_not_of(" \t\r\n");
                size_t e = item.find_last_not_of(" \t\r\n");
                headers.push_back((s == std::string::npos) ? "" : item.substr(s, e - s + 1));
            }
        }

        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            std::stringstream lss(line);
            std::string item;
            auto rowMap = std::make_shared<MemeMap>();
            size_t c = 0;
            while (std::getline(lss, item, ',')) {
                size_t s = item.find_first_not_of(" \t\r\n");
                size_t e = item.find_last_not_of(" \t\r\n");
                std::string val = (s == std::string::npos) ? "" : item.substr(s, e - s + 1);
                std::string key = (c < headers.size()) ? headers[c] : ("col" + std::to_string(c));
                rowMap->set(key, Value(val));
                c++;
            }
            rows->elements.push_back(Value(rowMap));
        }

        return createDataFrameObject(Value(rows));
    };
    m->set("read_csv", Value(std::make_shared<NativeMethodCallable>("read_csv", 1, readCsvFn)));

    return Value(m);
}

inline Value createGenericPythonModule(const std::string &pkg)
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto callFn = [pkg](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string fnName = args.empty() ? "" : args[0].deref().toString();
        // Deterministic fallback / bridge call
        return Value("[py:" + pkg + "." + fnName + " result]");
    };
    m->set("call", Value(std::make_shared<NativeMethodCallable>("call", 1, 10, callFn)));

    auto evalFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string code = args.empty() ? "" : args[0].deref().toString();
        return Value("[eval: " + code + "]");
    };
    m->set("eval", Value(std::make_shared<NativeMethodCallable>("eval", 1, evalFn)));

    return Value(m);
}

inline Value createPythonBridge(const std::string &pkg)
{
    if (pkg == "numpy" || pkg == "np") return createNumPyModule();
    if (pkg == "pandas" || pkg == "pd") return createPandasModule();
    return createGenericPythonModule(pkg);
}

// ---------------------------------------------------------------------------
// NPM Bridge: npm:ably, npm:firebase, and generic npm:<pkg>
// ---------------------------------------------------------------------------

struct AblyChannel
{
    std::string name;
    std::vector<Value> history;
    std::unordered_map<std::string, std::vector<Value>> subscriptions;
};

inline Value createAblyModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto channels = std::make_shared<std::unordered_map<std::string, std::shared_ptr<AblyChannel>>>();

    auto channelFn = [channels](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string chName = args.empty() ? "default" : args[0].deref().toString();
        if (channels->find(chName) == channels->end()) {
            auto ch = std::make_shared<AblyChannel>();
            ch->name = chName;
            (*channels)[chName] = ch;
        }
        auto ch = (*channels)[chName];

        auto chMap = std::make_shared<MemeMap>();
        chMap->isModule = true;

        auto pubFn = [ch](Interpreter &interp, const std::vector<Value> &pArgs) -> Value {
            std::string event = pArgs.size() > 0 ? pArgs[0].deref().toString() : "message";
            Value data = pArgs.size() > 1 ? pArgs[1] : Value(std::monostate{});

            auto msg = std::make_shared<MemeMap>();
            msg->set("event", Value(event));
            msg->set("data", data);
            msg->set("timestamp", Value(1000LL));
            ch->history.push_back(Value(msg));

            // Notify subscribers
            if (ch->subscriptions.find(event) != ch->subscriptions.end()) {
                for (auto &cbVal : ch->subscriptions[event]) {
                    Value cbd = cbVal.deref();
                    if (std::holds_alternative<std::shared_ptr<Callable>>(cbd.data)) {
                        std::get<std::shared_ptr<Callable>>(cbd.data)->call(interp, {data});
                    }
                }
            }
            return Value(true);
        };
        chMap->set("publish", Value(std::make_shared<NativeMethodCallable>("publish", 1, 2, pubFn)));

        auto subFn = [ch](Interpreter &, const std::vector<Value> &sArgs) -> Value {
            if (sArgs.size() >= 2) {
                std::string event = sArgs[0].deref().toString();
                ch->subscriptions[event].push_back(sArgs[1]);
            }
            return Value(true);
        };
        chMap->set("subscribe", Value(std::make_shared<NativeMethodCallable>("subscribe", 2, subFn)));

        auto histFn = [ch](Interpreter &, const std::vector<Value> &) -> Value {
            auto l = std::make_shared<MemeList>();
            l->elements = ch->history;
            return Value(l);
        };
        chMap->set("history", Value(std::make_shared<NativeMethodCallable>("history", 0, histFn)));

        return Value(chMap);
    };
    m->set("channel", Value(std::make_shared<NativeMethodCallable>("channel", 1, channelFn)));

    return Value(m);
}

inline Value createFirebaseModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto collections = std::make_shared<std::unordered_map<std::string, std::vector<Value>>>();

    auto collectionFn = [collections](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string colName = args.empty() ? "default" : args[0].deref().toString();
        auto colMap = std::make_shared<MemeMap>();
        colMap->isModule = true;

        auto addFn = [collections, colName](Interpreter &, const std::vector<Value> &aArgs) -> Value {
            Value docVal = aArgs.empty() ? Value(std::make_shared<MemeMap>()) : aArgs[0];
            std::string docId = "doc_" + Crypto::uuid().substr(0, 8);
            if (docVal.isMap()) {
                docVal.getMap()->set("id", Value(docId));
            }
            (*collections)[colName].push_back(docVal);
            return docVal;
        };
        colMap->set("add", Value(std::make_shared<NativeMethodCallable>("add", 1, addFn)));

        auto getFn = [collections, colName](Interpreter &, const std::vector<Value> &) -> Value {
            auto l = std::make_shared<MemeList>();
            if (collections->find(colName) != collections->end()) {
                l->elements = (*collections)[colName];
            }
            return Value(l);
        };
        colMap->set("get", Value(std::make_shared<NativeMethodCallable>("get", 0, getFn)));

        auto whereFn = [collections, colName](Interpreter &, const std::vector<Value> &wArgs) -> Value {
            auto l = std::make_shared<MemeList>();
            if (wArgs.size() >= 3 && collections->find(colName) != collections->end()) {
                std::string field = wArgs[0].deref().toString();
                std::string op = wArgs[1].deref().toString();
                Value target = wArgs[2].deref();
                for (const auto &item : (*collections)[colName]) {
                    Value id = item.deref();
                    if (id.isMap()) {
                        auto m = id.getMap();
                        if (m->has(field)) {
                            Value fVal = m->entries[field].deref();
                            if ((op == "==" || op == "=") && fVal == target) {
                                l->elements.push_back(item);
                            }
                        }
                    }
                }
            }
            return Value(l);
        };
        colMap->set("where", Value(std::make_shared<NativeMethodCallable>("where", 3, whereFn)));

        return Value(colMap);
    };
    m->set("collection", Value(std::make_shared<NativeMethodCallable>("collection", 1, collectionFn)));

    auto authFn = [](Interpreter &, const std::vector<Value> &) -> Value {
        auto authMap = std::make_shared<MemeMap>();
        authMap->isModule = true;
        auto signInFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
            std::string email = args.size() > 0 ? args[0].deref().toString() : "user@example.com";
            auto res = std::make_shared<MemeMap>();
            auto user = std::make_shared<MemeMap>();
            user->set("email", Value(email));
            user->set("uid", Value("uid_" + Crypto::sha256(email).substr(0, 10)));
            res->set("user", Value(user));
            res->set("token", Value("fb_tok_" + Crypto::uuid()));
            return Value(res);
        };
        authMap->set("signIn", Value(std::make_shared<NativeMethodCallable>("signIn", 2, signInFn)));
        return Value(authMap);
    };
    m->set("auth", Value(std::make_shared<NativeMethodCallable>("auth", 0, authFn)));

    return Value(m);
}

inline Value createGenericNpmModule(const std::string &pkg)
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto callFn = [pkg](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string fnName = args.empty() ? "" : args[0].deref().toString();
        return Value("[npm:" + pkg + "." + fnName + " result]");
    };
    m->set("call", Value(std::make_shared<NativeMethodCallable>("call", 1, 10, callFn)));

    return Value(m);
}

inline Value createNpmBridge(const std::string &pkg)
{
    if (pkg == "ably") return createAblyModule();
    if (pkg == "firebase") return createFirebaseModule();
    return createGenericNpmModule(pkg);
}

// ---------------------------------------------------------------------------
// Rust & Native C/C++ FFI Bridge: rust:crypto, rust:rayon, and ffi:<path>
// ---------------------------------------------------------------------------

inline Value createRustCryptoModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto sha256Fn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string str = args.empty() ? "" : args[0].deref().toString();
        return Value(Crypto::sha256(str));
    };
    m->set("sha256", Value(std::make_shared<NativeMethodCallable>("sha256", 1, sha256Fn)));

    auto hmacFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string key = args.size() > 0 ? args[0].deref().toString() : "";
        std::string msg = args.size() > 1 ? args[1].deref().toString() : "";
        return Value(Crypto::hmacSha256(key, msg));
    };
    m->set("hmac", Value(std::make_shared<NativeMethodCallable>("hmac", 2, hmacFn)));

    auto uuidFn = [](Interpreter &, const std::vector<Value> &) -> Value {
        return Value(Crypto::uuid());
    };
    auto uuidVal = Value(std::make_shared<NativeMethodCallable>("uuid", 0, uuidFn));
    m->set("uuid", uuidVal);
    m->set("uuid_v4", uuidVal);
    m->set("uuid4", uuidVal);

    return Value(m);
}

inline Value createRustRayonModule()
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto pmapFn = [](Interpreter &interp, const std::vector<Value> &args) -> Value {
        if (args.size() < 2) return Value(std::make_shared<MemeList>());
        Value listVal = args[0].deref();
        Value fnVal = args[1].deref();
        if (!listVal.isList() || !std::holds_alternative<std::shared_ptr<Callable>>(fnVal.data)) {
            return listVal;
        }
        auto callable = std::get<std::shared_ptr<Callable>>(fnVal.data);
        auto res = std::make_shared<MemeList>();
        for (const auto &item : listVal.getList()->elements) {
            res->elements.push_back(callable->call(interp, {item}));
        }
        return Value(res);
    };
    auto pmapVal = Value(std::make_shared<NativeMethodCallable>("parallel_map", 2, pmapFn));
    m->set("parallel_map", pmapVal);
    m->set("par_map", pmapVal);
    m->set("map", pmapVal);

    return Value(m);
}

inline Value createFfiBridge(const std::string &pathOrLib)
{
    auto m = std::make_shared<MemeMap>();
    m->isModule = true;

    auto loadFn = [pathOrLib](Interpreter &, const std::vector<Value> &) -> Value {
        return Value(true);
    };
    m->set("load", Value(std::make_shared<NativeMethodCallable>("load", 0, 1, loadFn)));

    auto callFn = [](Interpreter &, const std::vector<Value> &args) -> Value {
        std::string sym = args.empty() ? "" : args[0].deref().toString();
        return Value("[ffi: " + sym + " called]");
    };
    m->set("call", Value(std::make_shared<NativeMethodCallable>("call", 1, 10, callFn)));

    return Value(m);
}

inline Value createRustBridge(const std::string &target)
{
    if (target == "crypto") return createRustCryptoModule();
    if (target == "rayon") return createRustRayonModule();
    return createFfiBridge(target);
}

} // namespace EcosystemBridge

#endif // ECOSYSTEM_BRIDGE_H

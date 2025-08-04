#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <sstream>

// Helper function for circular left shift
uint32_t rotl(uint32_t value, unsigned int count) {
    return (value << count) | (value >> (32 - count));
}

class SHA1 {
public:
    SHA1() {
        reset();
    }

    void update(const std::string& s) {
        update(reinterpret_cast<const uint8_t*>(s.c_str()), s.length());
    }

    void update(const uint8_t* data, size_t len) {
        size_t i = 0;
        while (i < len) {
            buffer_[buffer_size_++] = data[i++];
            if (buffer_size_ == 64) {
                transform();
                buffer_size_ = 0;
            }
        }
    }

    std::string final() {
        uint64_t total_bits = (block_count_ * 64 + buffer_size_) * 8;

        // Pad with 0x80
        buffer_[buffer_size_++] = 0x80;
        if (buffer_size_ > 56) {
            while (buffer_size_ < 64) {
                buffer_[buffer_size_++] = 0x00;
            }
            transform();
            buffer_size_ = 0;
        }

        while (buffer_size_ < 56) {
            buffer_[buffer_size_++] = 0x00;
        }

        // Append original length in bits
        for (int i = 0; i < 8; ++i) {
            buffer_[56 + i] = static_cast<uint8_t>((total_bits >> (56 - i * 8)) & 0xFF);
        }
        transform();
        
        // Convert digest to hex string
        std::stringstream ss;
        for (uint32_t val : digest_) {
            ss << std::hex << std::setw(8) << std::setfill('0') << val;
        }
        
        reset(); // Prepare for next hash
        return ss.str();
    }

private:
    void reset() {
        digest_[0] = 0x67452301;
        digest_[1] = 0xEFCDAB89;
        digest_[2] = 0x98BADCFE;
        digest_[3] = 0x10325476;
        digest_[4] = 0xC3D2E1F0;
        block_count_ = 0;
        buffer_size_ = 0;
    }

    void transform() {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = (buffer_[i * 4 + 0] << 24) |
                   (buffer_[i * 4 + 1] << 16) |
                   (buffer_[i * 4 + 2] << 8)  |
                   (buffer_[i * 4 + 3]);
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = digest_[0];
        uint32_t b = digest_[1];
        uint32_t c = digest_[2];
        uint32_t d = digest_[3];
        uint32_t e = digest_[4];

        for (int t = 0; t < 80; ++t) {
            uint32_t f, k;
            if (t < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (t < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (t < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            uint32_t temp = rotl(a, 5) + f + e + k + w[t];
            e = d;
            d = c;
            c = rotl(b, 30);
            b = a;
            a = temp;
        }

        digest_[0] += a;
        digest_[1] += b;
        digest_[2] += c;
        digest_[3] += d;
        digest_[4] += e;

        block_count_++;
    }

    uint32_t digest_[5];
    uint8_t buffer_[64];
    size_t buffer_size_;
    uint64_t block_count_;
};
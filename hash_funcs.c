#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define sym_x16(num4) ((num4) < 10 ? '0' + (num4) : 'a' + (num4) - 10)


// ------
// SHA256
// ------

#define rcshift(num32, i) ((num32) >> (i) | (num32) << (32-(i))) //circular rshift

const uint32_t sha256_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

void sha256_main_loop(int32_t h[8], const uint8_t * chunk_str, uint64_t chunks)
{
    uint32_t w[64];

    for (uint64_t chunk = 0; chunk < chunks; ++chunk)
    {
        for (uint8_t i = 0; i < 16; ++i)
        {
            w[i] = (uint32_t)chunk_str[0] << 24 |
                   (uint32_t)chunk_str[1] << 16 |
                   (uint32_t)chunk_str[2] << 8  |
                   (uint32_t)chunk_str[3];
            chunk_str += 4;
        }
        for (uint8_t i = 16; i < 64; ++i)
        {
            uint32_t s0 = (w[i-15] >> 3) ^ rcshift(w[i-15], 7) ^ rcshift(w[i-15], 18);
            uint32_t s1 = (w[i-2] >> 10) ^ rcshift(w[i-2], 17) ^ rcshift(w[i-2], 19);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }

        uint32_t A = h[0], B = h[1], C = h[2], D = h[3],
                 E = h[4], F = h[5], G = h[6], H = h[7];

        for (uint8_t i = 0; i < 64; ++i)
        {
            uint32_t t1 = sha256_K[i] + w[i] + H + ((E & F) ^ ((~E) & G)) +
                          (rcshift(E, 6) ^ rcshift(E, 11) ^ rcshift(E, 25));
            uint32_t t0 = t1 + ((A & B) ^ (A & C) ^ (B & C)) +
                          (rcshift(A, 2) ^ rcshift(A, 13) ^ rcshift(A, 22));

            H = G; G = F; F = E; E = D + t1;
            D = C; C = B; B = A; A = t0;
        }

        h[0] += A; h[1] += B; h[2] += C; h[3] += D;
        h[4] += E; h[5] += F; h[6] += G; h[7] += H;
    }
}

char * sha256(const char *str, uint64_t siz)
{
    uint64_t chunks = siz >> 6;
    uint64_t extra_siz = siz - (chunks << 6);
    uint8_t extra_chunks = (extra_siz + 72) >> 6;
    uint64_t extra_mem = extra_chunks << 6;

    char extra[1024];
    memcpy(extra, str + (chunks << 6), extra_siz);
    extra[extra_siz] = (char)(1 << 7);
    memset(extra + extra_siz + 1, 0, extra_mem - extra_siz - 1);
    siz <<= 3;
    for (uint64_t i = 0; i < 8; ++i) {
        uint8_t shift = i << 3;
        extra[extra_mem - 1 - i] = (uint8_t)(siz >> shift);
    }
    siz >>= 3;

    int32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    };
    sha256_main_loop(h, (uint8_t *)str, chunks);
    sha256_main_loop(h, (uint8_t *)extra, extra_chunks);

    char * hash = malloc(65);
    // free `hash` memory from outside
    for (uint8_t i = 0; i < 8; ++i) {
        for (uint8_t j = 0; j < 8; ++j) {
            hash[(i<<3)+j] = sym_x16((h[i] >> ((7-j)<<2)) & 15);
        }
    }
    hash[64] = '\0';
    return hash;
}


// ----------
// STREEBOG256
// ----------

const uint8_t sb256_PI[256] = {
    252, 238, 221, 17,  207, 110, 49,  22,  251, 196, 250, 218, 35,  197, 4,   77,
    233, 119, 240, 219, 147, 46,  153, 186, 23,  54,  241, 187, 20,  205, 95,  193,
    249, 24,  101, 90,  226, 92,  239, 33,  129, 28,  60,  66,  139, 1,   142, 79,
    5,   132, 2,   174, 227, 106, 143, 160, 6,   11,  237, 152, 127, 212, 211, 31,
    235, 52,  44,  81,  234, 200, 72,  171, 242, 42,  104, 162, 253, 58,  206, 204,
    181, 112, 14,  86,  8,   12,  118, 18,  191, 114, 19,  71,  156, 183, 93,  135,
    21,  161, 150, 41,  16,  123, 154, 199, 243, 145, 120, 111, 157, 158, 178, 177,
    50,  117, 25,  61,  255, 53,  138, 126, 109, 84,  198, 128, 195, 189, 13,  87,
    223, 245, 36,  169, 62,  168, 67,  201, 215, 121, 214, 246, 124, 34,  185, 3,
    224, 15,  236, 222, 122, 148, 176, 188, 220, 232, 40,  80,  78,  51,  10,  74,
    167, 151, 96,  115, 30,  0,   98,  68,  26,  184, 56,  130, 100, 159, 38,  65,
    173, 69,  70,  146, 39,  94,  85,  47,  140, 163, 165, 125, 105, 213, 149, 59,
    7,   88,  179, 64,  134, 172, 29,  247, 48,  55,  107, 228, 136, 217, 231, 137,
    225, 27,  131, 73,  76,  63,  248, 254, 141, 83,  170, 144, 202, 216, 133, 97,
    32,  113, 103, 164, 45,  43,  9,   91,  203, 155, 37,  208, 190, 229, 108, 82,
    89,  166, 116, 210, 230, 244, 180, 192, 209, 102, 175, 194, 57,  75,  99,  182,
};
const uint8_t sb256_TAU[64] = {
    0, 8,  16, 24, 32, 40, 48, 56, 1, 9,  17, 25, 33, 41, 49, 57,
    2, 10, 18, 26, 34, 42, 50, 58, 3, 11, 19, 27, 35, 43, 51, 59,
    4, 12, 20, 28, 36, 44, 52, 60, 5, 13, 21, 29, 37, 45, 53, 61,
    6, 14, 22, 30, 38, 46, 54, 62, 7, 15, 23, 31, 39, 47, 55, 63,
};
const uint64_t sb256_A[64] = {
    0x8e20faa72ba0b470, 0x47107ddd9b505a38, 0xad08b0e0c3282d1c, 0xd8045870ef14980e,
    0x6c022c38f90a4c07, 0x3601161cf205268d, 0x1b8e0b0e798c13c8, 0x83478b07b2468764,
    0xa011d380818e8f40, 0x5086e740ce47c920, 0x2843fd2067adea10, 0x14aff010bdd87508,
    0x0ad97808d06cb404, 0x05e23c0468365a02, 0x8c711e02341b2d01, 0x46b60f011a83988e,
    0x90dab52a387ae76f, 0x486dd4151c3dfdb9, 0x24b86a840e90f0d2, 0x125c354207487869,
    0x092e94218d243cba, 0x8a174a9ec8121e5d, 0x4585254f64090fa0, 0xaccc9ca9328a8950,
    0x9d4df05d5f661451, 0xc0a878a0a1330aa6, 0x60543c50de970553, 0x302a1e286fc58ca7,
    0x18150f14b9ec46dd, 0x0c84890ad27623e0, 0x0642ca05693b9f70, 0x0321658cba93c138,
    0x86275df09ce8aaa8, 0x439da0784e745554, 0xafc0503c273aa42a, 0xd960281e9d1d5215,
    0xe230140fc0802984, 0x71180a8960409a42, 0xb60c05ca30204d21, 0x5b068c651810a89e,
    0x456c34887a3805b9, 0xac361a443d1c8cd2, 0x561b0d22900e4669, 0x2b838811480723ba,
    0x9bcf4486248d9f5d, 0xc3e9224312c8c1a0, 0xeffa11af0964ee50, 0xf97d86d98a327728,
    0xe4fa2054a80b329c, 0x727d102a548b194e, 0x39b008152acb8227, 0x9258048415eb419d,
    0x492c024284fbaec0, 0xaa16012142f35760, 0x550b8e9e21f7a530, 0xa48b474f9ef5dc18,
    0x70a6a56e2440598e, 0x3853dc371220a247, 0x1ca76e95091051ad, 0x0edd37c48a08a6d8,
    0x07e095624504536c, 0x8d70c431ac02a736, 0xc83862965601dd1b, 0x641c314b2b8ee083,
};

const uint8_t sb256_C[12][64] = {
    {
        0xb1, 0x08, 0x5b, 0xda, 0x1e, 0xca, 0xda, 0xe9,
        0xeb, 0xcb, 0x2f, 0x81, 0xc0, 0x65, 0x7c, 0x1f,
        0x2f, 0x6a, 0x76, 0x43, 0x2e, 0x45, 0xd0, 0x16,
        0x71, 0x4e, 0xb8, 0x8d, 0x75, 0x85, 0xc4, 0xfc,
        0x4b, 0x7c, 0xe0, 0x91, 0x92, 0x67, 0x69, 0x01,
        0xa2, 0x42, 0x2a, 0x08, 0xa4, 0x60, 0xd3, 0x15,
        0x05, 0x76, 0x74, 0x36, 0xcc, 0x74, 0x4d, 0x23,
        0xdd, 0x80, 0x65, 0x59, 0xf2, 0xa6, 0x45, 0x07,
    }, {
        0x6f, 0xa3, 0xb5, 0x8a, 0xa9, 0x9d, 0x2f, 0x1a,
        0x4f, 0xe3, 0x9d, 0x46, 0x0f, 0x70, 0xb5, 0xd7,
        0xf3, 0xfe, 0xea, 0x72, 0x0a, 0x23, 0x2b, 0x98,
        0x61, 0xd5, 0x5e, 0x0f, 0x16, 0xb5, 0x01, 0x31,
        0x9a, 0xb5, 0x17, 0x6b, 0x12, 0xd6, 0x99, 0x58,
        0x5c, 0xb5, 0x61, 0xc2, 0xdb, 0x0a, 0xa7, 0xca,
        0x55, 0xdd, 0xa2, 0x1b, 0xd7, 0xcb, 0xcd, 0x56,
        0xe6, 0x79, 0x04, 0x70, 0x21, 0xb1, 0x9b, 0xb7,
    }, {
        0xf5, 0x74, 0xdc, 0xac, 0x2b, 0xce, 0x2f, 0xc7,
        0x0a, 0x39, 0xfc, 0x28, 0x6a, 0x3d, 0x84, 0x35,
        0x06, 0xf1, 0x5e, 0x5f, 0x52, 0x9c, 0x1f, 0x8b,
        0xf2, 0xea, 0x75, 0x14, 0xb1, 0x29, 0x7b, 0x7b,
        0xd3, 0xe2, 0x0f, 0xe4, 0x90, 0x35, 0x9e, 0xb1,
        0xc1, 0xc9, 0x3a, 0x37, 0x60, 0x62, 0xdb, 0x09,
        0xc2, 0xb6, 0xf4, 0x43, 0x86, 0x7a, 0xdb, 0x31,
        0x99, 0x1e, 0x96, 0xf5, 0x0a, 0xba, 0x0a, 0xb2,
    }, {
        0xef, 0x1f, 0xdf, 0xb3, 0xe8, 0x15, 0x66, 0xd2,
        0xf9, 0x48, 0xe1, 0xa0, 0x5d, 0x71, 0xe4, 0xdd,
        0x48, 0x8e, 0x85, 0x7e, 0x33, 0x5c, 0x3c, 0x7d,
        0x9d, 0x72, 0x1c, 0xad, 0x68, 0x5e, 0x35, 0x3f,
        0xa9, 0xd7, 0x2c, 0x82, 0xed, 0x03, 0xd6, 0x75,
        0xd8, 0xb7, 0x13, 0x33, 0x93, 0x52, 0x03, 0xbe,
        0x34, 0x53, 0xea, 0xa1, 0x93, 0xe8, 0x37, 0xf1,
        0x22, 0x0c, 0xbe, 0xbc, 0x84, 0xe3, 0xd1, 0x2e,
    }, {
        0x4b, 0xea, 0x6b, 0xac, 0xad, 0x47, 0x47, 0x99,
        0x9a, 0x3f, 0x41, 0x0c, 0x6c, 0xa9, 0x23, 0x63,
        0x7f, 0x15, 0x1c, 0x1f, 0x16, 0x86, 0x10, 0x4a,
        0x35, 0x9e, 0x35, 0xd7, 0x80, 0x0f, 0xff, 0xbd,
        0xbf, 0xcd, 0x17, 0x47, 0x25, 0x3a, 0xf5, 0xa3,
        0xdf, 0xff, 0x00, 0xb7, 0x23, 0x27, 0x1a, 0x16,
        0x7a, 0x56, 0xa2, 0x7e, 0xa9, 0xea, 0x63, 0xf5,
        0x60, 0x17, 0x58, 0xfd, 0x7c, 0x6c, 0xfe, 0x57,
    }, {
        0xae, 0x4f, 0xae, 0xae, 0x1d, 0x3a, 0xd3, 0xd9,
        0x6f, 0xa4, 0xc3, 0x3b, 0x7a, 0x30, 0x39, 0xc0,
        0x2d, 0x66, 0xc4, 0xf9, 0x51, 0x42, 0xa4, 0x6c,
        0x18, 0x7f, 0x9a, 0xb4, 0x9a, 0xf0, 0x8e, 0xc6,
        0xcf, 0xfa, 0xa6, 0xb7, 0x1c, 0x9a, 0xb7, 0xb4,
        0x0a, 0xf2, 0x1f, 0x66, 0xc2, 0xbe, 0xc6, 0xb6,
        0xbf, 0x71, 0xc5, 0x72, 0x36, 0x90, 0x4f, 0x35,
        0xfa, 0x68, 0x40, 0x7a, 0x46, 0x64, 0x7d, 0x6e,
    }, {
        0xf4, 0xc7, 0x0e, 0x16, 0xee, 0xaa, 0xc5, 0xec,
        0x51, 0xac, 0x86, 0xfe, 0xbf, 0x24, 0x09, 0x54,
        0x39, 0x9e, 0xc6, 0xc7, 0xe6, 0xbf, 0x87, 0xc9,
        0xd3, 0x47, 0x3e, 0x33, 0x19, 0x7a, 0x93, 0xc9,
        0x09, 0x92, 0xab, 0xc5, 0x2d, 0x82, 0x2c, 0x37,
        0x06, 0x47, 0x69, 0x83, 0x28, 0x4a, 0x05, 0x04,
        0x35, 0x17, 0x45, 0x4c, 0xa2, 0x3c, 0x4a, 0xf3,
        0x88, 0x86, 0x56, 0x4d, 0x3a, 0x14, 0xd4, 0x93,
    }, {
        0x9b, 0x1f, 0x5b, 0x42, 0x4d, 0x93, 0xc9, 0xa7,
        0x03, 0xe7, 0xaa, 0x02, 0x0c, 0x6e, 0x41, 0x41,
        0x4e, 0xb7, 0xf8, 0x71, 0x9c, 0x36, 0xde, 0x1e,
        0x89, 0xb4, 0x44, 0x3b, 0x4d, 0xdb, 0xc4, 0x9a,
        0xf4, 0x89, 0x2b, 0xcb, 0x92, 0x9b, 0x06, 0x90,
        0x69, 0xd1, 0x8d, 0x2b, 0xd1, 0xa5, 0xc4, 0x2f,
        0x36, 0xac, 0xc2, 0x35, 0x59, 0x51, 0xa8, 0xd9,
        0xa4, 0x7f, 0x0d, 0xd4, 0xbf, 0x02, 0xe7, 0x1e,
    }, {
        0x37, 0x8f, 0x5a, 0x54, 0x16, 0x31, 0x22, 0x9b,
        0x94, 0x4c, 0x9a, 0xd8, 0xec, 0x16, 0x5f, 0xde,
        0x3a, 0x7d, 0x3a, 0x1b, 0x25, 0x89, 0x42, 0x24,
        0x3c, 0xd9, 0x55, 0xb7, 0xe0, 0x0d, 0x09, 0x84,
        0x80, 0x0a, 0x44, 0x0b, 0xdb, 0xb2, 0xce, 0xb1,
        0x7b, 0x2b, 0x8a, 0x9a, 0xa6, 0x07, 0x9c, 0x54,
        0x0e, 0x38, 0xdc, 0x92, 0xcb, 0x1f, 0x2a, 0x60,
        0x72, 0x61, 0x44, 0x51, 0x83, 0x23, 0x5a, 0xdb,
    }, {
        0xab, 0xbe, 0xde, 0xa6, 0x80, 0x05, 0x6f, 0x52,
        0x38, 0x2a, 0xe5, 0x48, 0xb2, 0xe4, 0xf3, 0xf3,
        0x89, 0x41, 0xe7, 0x1c, 0xff, 0x8a, 0x78, 0xdb,
        0x1f, 0xff, 0xe1, 0x8a, 0x1b, 0x33, 0x61, 0x03,
        0x9f, 0xe7, 0x67, 0x02, 0xaf, 0x69, 0x33, 0x4b,
        0x7a, 0x1e, 0x6c, 0x30, 0x3b, 0x76, 0x52, 0xf4,
        0x36, 0x98, 0xfa, 0xd1, 0x15, 0x3b, 0xb6, 0xc3,
        0x74, 0xb4, 0xc7, 0xfb, 0x98, 0x45, 0x9c, 0xed,
    }, {
        0x7b, 0xcd, 0x9e, 0xd0, 0xef, 0xc8, 0x89, 0xfb,
        0x30, 0x02, 0xc6, 0xcd, 0x63, 0x5a, 0xfe, 0x94,
        0xd8, 0xfa, 0x6b, 0xbb, 0xeb, 0xab, 0x07, 0x61,
        0x20, 0x01, 0x80, 0x21, 0x14, 0x84, 0x66, 0x79,
        0x8a, 0x1d, 0x71, 0xef, 0xea, 0x48, 0xb9, 0xca,
        0xef, 0xba, 0xcd, 0x1d, 0x7d, 0x47, 0x6e, 0x98,
        0xde, 0xa2, 0x59, 0x4a, 0xc0, 0x6f, 0xd8, 0x5d,
        0x6b, 0xca, 0xa4, 0xcd, 0x81, 0xf3, 0x2d, 0x1b,
    }, {
        0x37, 0x8e, 0xe7, 0x67, 0xf1, 0x16, 0x31, 0xba,
        0xd2, 0x13, 0x80, 0xb0, 0x04, 0x49, 0xb1, 0x7a,
        0xcd, 0xa4, 0x3c, 0x32, 0xbc, 0xdf, 0x1d, 0x77,
        0xf8, 0x20, 0x12, 0xd4, 0x30, 0x21, 0x9f, 0x9b,
        0x5d, 0x80, 0xef, 0x9d, 0x18, 0x91, 0xcc, 0x86,
        0xe7, 0x1d, 0xa4, 0xaa, 0x88, 0xe1, 0x28, 0x52,
        0xfa, 0xf4, 0x17, 0xd5, 0xd9, 0xb2, 0x1b, 0x99,
        0x48, 0xbc, 0x92, 0x4a, 0xf1, 0x1b, 0xd7, 0x20,
    }
};

void sb256_X(uint8_t dst[64], const uint8_t src[64])
{
    for (uint8_t i = 0; i < 64; ++i) {
        dst[i] ^= src[i];
    }
}

void sb256_LPS(uint8_t vec[64])
{
    uint8_t tmp[64];

    for (uint8_t i = 0; i < 8; ++i)
    {
        i <<= 3;

        for (uint8_t j = 0; j < 8; ++j) {
            tmp[i+j] = sb256_PI[vec[sb256_TAU[i+j]]];
        }

        uint64_t c = 0;
        for (uint8_t j = 0; j < 64; ++j) {
            if (tmp[i+(j>>3)] & (1 << ((7-j)&7))) {
                c ^= sb256_A[j];
            }
        }
        for (uint8_t j = 0; j < 8; ++j) {
            tmp[i+j] = c >> ((7-j)<<3);
        }

        i >>= 3;
    }
    memcpy(vec, tmp, 64);
}

void sb256_E(uint8_t K[64], const uint8_t m[64])
{
    uint8_t Ki[64];
    memcpy(Ki, K, 64);

    sb256_X(K, m);

    for (uint8_t i = 0; i < 12; ++i) {
        sb256_LPS(K);
        sb256_X(Ki, sb256_C[i]);
        sb256_LPS(Ki);
        sb256_X(K, Ki);
    }
}

void sb256_gN(uint8_t h[64], const uint8_t m[64], const uint8_t N[64])
{
    uint8_t E[64];
    memcpy(E, h, 64);
    if (N != NULL) { sb256_X(E, N); }
    sb256_LPS(E);
    sb256_E(E, m);
    sb256_X(h, E);
    sb256_X(h, m);
}

void sb256_add(uint8_t dst[64], const uint8_t src[64])
{
    uint16_t r = 0;
    for (uint8_t i = 63; i < 64; --i) {
        r += src[i] + dst[i];
        dst[i] = r;
        r >>= 8;
    }
}

void sb256_add_u16(uint8_t dst[64], uint16_t num)
{
    for (uint8_t i = 63; i < 64 && num; --i) {
        num += dst[i];
        dst[i] = num;
        num >>= 8;
    }
}

char * streebog256(const char *str, uint64_t siz)
{
    uint8_t h[64] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };

    uint8_t N[64] = {0};
    uint8_t Sigma[64] = {0};

    while (siz >= 64) {
        siz -= 64;
        const uint8_t *m = (uint8_t *)str + siz;
        sb256_gN(h, m, N);
        sb256_add_u16(N, 512);
        sb256_add(Sigma, m);
    }

    uint8_t shift = 63 - siz;
    uint8_t m[64];
    memset(m, 0, shift);
    m[shift] = 1;
    memcpy(m + shift + 1, str, siz);

    sb256_gN(h, m, N);
    sb256_add_u16(N, siz << 3);
    sb256_add(Sigma, m);
    sb256_gN(h, N, NULL);
    sb256_gN(h, Sigma, NULL);

    char * hash = malloc(65);
    // free `hash` memory from outside
    for (uint8_t i = 0; i < 32; ++i) {
        hash[(i<<1)]   = sym_x16(h[i] >> 4);
        hash[(i<<1)+1] = sym_x16(h[i] & 15);
    }
    hash[64] = '\0';
    return hash;
}

void freestr(void *str) {
    free(str);
}

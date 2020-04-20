#include <stdio.h>
#include <stdint.h>

uint64_t mask(int x)
{
    uint64_t r = 0;
    for(int i = 0; i < 8; ++i) {
        uint64_t m = 0x0f;
        if((x & (1 << i)) != 0) {
            m = 0x01;
        }
        r |= m << ((i) * 5);
    }
    uint64_t s = 0;
    for(int i = 0; i < 5; ++i) {
        s |= ((r >> (i * 8)) & 0xff) << ((4 - i) * 8);
    }
    return s;
}

uint64_t five_1[256];

void stash_5(uint32_t *dst, uint32_t s)
{
    uint32_t p1 = s & 0xff;
    uint32_t p2 = (s >> 8) & 0xff;
    uint32_t p3 = (s >> 16) & 0xff;
    uint32_t p4 = (s >> 24) & 0xff;

    uint64_t a = five_1[p1];
    uint64_t b = five_1[p2];
    uint64_t c = five_1[p3];
    uint64_t d = five_1[p4];

    dst[0] = (uint32_t)(a);
    dst[1] = (uint32_t)((a >> 32) | (b << 8));
    dst[2] = (uint32_t)((b >> 24) | (c << 16));
    dst[3] = (uint32_t)((c >> 16) | (d << 24));
    dst[4] = (uint32_t)(d >> 8);
}

int main()
{
    char const *sep = "unsigned long long const five_1[256] = {\n    ";
    for(int i = 0; i < 256; ++i) {
        uint64_t m = mask(i);
        five_1[i] = m;
        printf("%s0x%10llx", sep, m);
        sep = ",\n    ";
    }
    printf("\n};\n\n");

    uint32_t b[10];
    stash_5(b, 0xffffffff);
    stash_5(b + 5, 0xffffffff);
}
#ifndef RGB_PIXEL_H
#define RGB_PIXEL_H

#include <cstdint>

/* Had to abandon pragma pack for now,
 * because my ide goes crazy when I try to use it :/ */
struct rgb_pixel {
    uint8_t r:8 = 0;
    uint8_t g:8 = 0;
    uint8_t b:8 = 0;
};

constexpr int pixel_size = sizeof(rgb_pixel);

inline rgb_pixel operator + (const rgb_pixel& left,
                             const rgb_pixel& right)
{
    return {(uint8_t)(left.r + right.r),
            (uint8_t)(left.g + right.g),
            (uint8_t)(left.b + right.b)};
}

inline rgb_pixel operator * (float coef, const rgb_pixel& p)
{
    return {(uint8_t)(coef * p.r),
            (uint8_t)(coef * p.g),
            (uint8_t)(coef * p.b)};
}


#endif // RGB_PIXEL_H

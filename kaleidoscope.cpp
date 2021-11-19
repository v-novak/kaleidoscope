#include "kaleidoscope.h"

#include <cmath>
#include <iostream>
#include <algorithm>
#include <limits>

struct point2d
{
    float x;
    float y;
};

point2d rotate2d(const point2d& src, const point2d& around,
                 float angle)
{
    float cosa = cosf(angle);
    float sina = sinf(angle);

    return {
        cosa * (src.x - around.x) - sina * (src.y - around.y) + around.x,
        sina * (src.x - around.x) + cosa * (src.y - around.y) + around.y
    };
}

inline bool point_in_triangle(const point2d triangle[3], const point2d& p)
{
    const float& ax = triangle[0].x;
    const float& ay = triangle[0].y;
    const float& bx = triangle[1].x;
    const float& by = triangle[1].y;
    const float& cx = triangle[2].x;
    const float& cy = triangle[2].y;

    float det = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);

    return  det * ((bx - ax) * (p.y - ay) - (by - ay) * (p.x - ax)) >= 0 &&
            det * ((cx - bx) * (p.y - by) - (cy - by) * (p.x - bx)) >= 0 &&
            det * ((ax - cx) * (p.y - cy) - (ay - cy) * (p.x - cx)) >= 0;
}

void kaleidoscope(rgb_image& img, int sectors)
{
    if (sectors < 4) sectors = 6;
    if (sectors % 2) sectors += 1;

    /*      ______
     *     /\    /\
     *    /  \  /  \
     *   /____\/____\
     *   \    /\    /
     *    \  /  \  /
     *     \/____\/
     */
    rgb_image scaled_img;
    img.scale(scaled_img, 50);
    img.gamma(1.2f);
    img.dim(50);
    img.blur(2); // bonus

    float sector_angle = 2 * M_PI / sectors;

    // start with the lower central sector:
    point2d triangle[3] = {
        {(float)img.w() / 2, (float) img.h() / 2},
        {(float)img.w() / 2 + tan(sector_angle / 2) * scaled_img.h(), (float)(img.h()) - 1},
        {(float)img.w() / 2 - tan(sector_angle / 2) * scaled_img.h(), (float)(img.h()) - 1}
    };

    for (int sector = 0; sector < sectors; ++sector) {
        float angle_offset = sector * sector_angle;

        // determine the window for impainting
        float xmin = std::numeric_limits<float>::max();
        float xmax = std::numeric_limits<float>::min();
        float ymin = std::numeric_limits<float>::max();
        float ymax = std::numeric_limits<float>::min();

        for (auto& pt: triangle) {
            xmin = fmin(pt.x, xmin);
            xmax = fmax(pt.x, xmax);
            ymin = fmin(pt.y, ymin);
            ymax = fmax(pt.y, ymax);
        }

        for (int row = ymin; row <= ymax; ++row) {
            for (int col = xmin; col <= xmax; ++col) {
                point2d pt;
                pt.x = col;
                pt.y = row;

                if (point_in_triangle(triangle, pt)) {
                    point2d coords_translated = rotate2d(point2d{float(col), float(row)}, triangle[0], -angle_offset);
                    coords_translated.x = (coords_translated.x - triangle[0].x + scaled_img.w() / 2);
                    coords_translated.y = (coords_translated.y - triangle[0].y);

                    img.set(col, row, scaled_img.at(coords_translated.x, coords_translated.y));
                }
            }
        }

        triangle[1] = triangle[2];
        triangle[2] = rotate2d(triangle[2], triangle[0], sector_angle);
    }
}

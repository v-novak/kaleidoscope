#ifndef RGBIMAGE_H
#define RGBIMAGE_H


#include "rgb_pixel.h"
#include "util.h"

#include <memory>
#include <stdexcept>
#include <string>

class rgb_image
{ 
public:
    rgb_image() noexcept;
    rgb_image(const rgb_image&) noexcept;
    rgb_image(rgb_image&&) noexcept;
    ~rgb_image();

    const rgb_image& operator= (const rgb_image& src);

    void load(const std::string& filename, int scale_percent = 100) noexcept(false);
    void save(const std::string& filename, int quality_percent = 90) noexcept(false);
    void scale(rgb_image& output, int scale_percent = 100) const;

    int w() const;
    int h() const;

    rgb_pixel at(int col, int row) const;
    rgb_pixel at(float col, float row) const; // linearly interpolated
    void set(int col, int row, const rgb_pixel& val) const;

    void dim(int percent); // make the image (percent/100) bright
    void gamma(float g); // gamma correction
    void blur(int r); // gaussian blur

private:
    struct impl;
    std::unique_ptr<impl> _impl;
};

#endif // RGBIMAGE_H

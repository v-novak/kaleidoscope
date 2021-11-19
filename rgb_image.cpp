#include "rgb_image.h"
#include "util.h"

#include <turbojpeg.h>
#include <jconfig.h>
#include <cmath>
#include <fstream>
#include <iostream>


struct rgb_image::impl
{
    tjhandle compressor = nullptr;
    tjhandle decompressor = nullptr;

    unsigned char* buffer = nullptr;
    size_t bufsize = 0;

    int w = 0, h = 0;
    const int pix_fmt = TJPF_RGB;

    inline size_t imgsize(int W, int H) const {
        return W * H * tjPixelSize[pix_fmt];
    }

    inline uint8_t* pixaddr(int col, int row)
    {
        return buffer + 3 * (row * w + col);
    }

    void resize(int new_w, int new_h) {
        if (new_w * new_h != w * h) {
            if (buffer) {
                tjFree(buffer);
                buffer = nullptr;
            }
            bufsize = imgsize(new_w, new_h);
            if (bufsize > 0) {
                buffer = tjAlloc(bufsize);
                assert(buffer);
            }
        }

        w = new_w;
        h = new_h;
    }

    inline rgb_pixel at(int col, int row) {
        if (!buffer) return {0, 0, 255};

        if (row < 0) row = 0;
        else if (row >= h) row = h - 1;

        if (col < 0) col = 0;
        else if (col >= w) col = w - 1;

        uint8_t* offset = pixaddr(col, row);
        return {*offset, *(offset + 1), *(offset + 2)};
    }

    inline rgb_pixel at(float col, float row) {
        if (col < 0.0f) col = 0.0f;
        else if (col >= w) col = w - 1;

        if (row < 0.0f) row = 0.0f;
        else if (row >= h) row = h - 1;

        float col_int, col_frac; // integral and fractional parts
        col_frac = modf(col, &col_int);

        float row_int, row_frac;
        row_frac = modf(row, &row_int);

        /* Bilinear interpolation:
         *
         *       AB
         *  A •------• B
         *    |  |   |
         *    |--x---|
         *    |  |   |
         *  C •------• D
         *       CD
         */
        rgb_pixel A = at(int(col_int),     int(row_int)),
                  B = at(int(col_int) + 1, int(row_int)),
                  C = at(int(col_int),     int(row_int) + 1),
                  D = at(int(col_int) + 1, int(row_int) + 1);

        rgb_pixel AB = (1.0f - col_frac) * A + col_frac * B;
        rgb_pixel CD = (1.0f - col_frac) * C + col_frac * D;

        return (1.0f - row_frac) * AB + row_frac * CD;
    }

    inline void set(int col, int row, const rgb_pixel& val) {
        if (row < 0 || row >= h || col < 0 || col >= w) return;
        uint8_t* offset = pixaddr(col, row);
        *offset = val.r;
        *(offset + 1) = val.g;
        *(offset + 2) = val.b;
    }

    ~impl() {
        if (buffer) {
            tjFree(buffer);
            buffer = nullptr;
            bufsize = 0;
        }

        if (compressor) {
            tjDestroy(compressor);
            compressor = nullptr;
        }

        if (decompressor) {
            tjDestroy(decompressor);
            decompressor = nullptr;
        }
    }
};

rgb_image::rgb_image() noexcept
{
    _impl.reset(new impl());
}

rgb_image::rgb_image(const rgb_image& src) noexcept
{
    _impl.reset(new impl());
    if (src._impl->bufsize != 0) {
        _impl->resize(src.w(), src.h());
        memcpy(_impl->buffer, src._impl->buffer, _impl->bufsize);
    }
}

rgb_image::rgb_image(rgb_image&& other) noexcept
{
    _impl = std::move(other._impl);
}

rgb_image::~rgb_image()
{
    _impl.reset();
}

const rgb_image& rgb_image::operator=(const rgb_image& src)
{
    src.scale(*this, 100);
    return *this;
}

void rgb_image::load(const std::string &filename, int scale_percent) noexcept(false)
{
    assert(_impl);

    if (!_impl->decompressor) {
        _impl->decompressor = tjInitDecompress();
        if (!_impl->decompressor) {
            throw io_exception("Cannot initialise decompressor");
        }
    }

    std::ifstream input_file(filename.c_str());
    if (!input_file) {
        throw io_exception("Could not load " + filename);
    }
    input_file.seekg(0, std::ios::end);

    size_t input_size = input_file.tellg();
    input_file.seekg(0);

    unsigned char* input_buffer = tjAlloc(input_size);
    scope_guard imageDataDeleter([&]() {
        tjFree(input_buffer);
    });
    input_file.read(reinterpret_cast<char*>(input_buffer), input_size);

    int width;
    int height;
    int subsamp;
    int colorspace;
    tjDecompressHeader3(_impl->decompressor, input_buffer, input_size, &width, &height, &subsamp, &colorspace);

    if (scale_percent != 100) {
        tjscalingfactor scaling{scale_percent, 100};
        width = TJSCALED(width, scaling);
        height = TJSCALED(height, scaling);
    }

    _impl->resize(width, height);

    if (tjDecompress2(_impl->decompressor, input_buffer, input_size,
                      _impl->buffer, width, 0, height, _impl->pix_fmt, /*flags*/ 0) < 0) {
        throw io_exception(std::string("Decompressing JPEG image failed: ") + tjGetErrorStr2(_impl->decompressor));
    }
}

void rgb_image::save(const std::string &filename, int quality_percent) noexcept(false)
{
    if (quality_percent < 0) quality_percent = 0;
    if (quality_percent > 100) quality_percent = 100;

    if (!_impl->compressor) {
        _impl->compressor = tjInitCompress();
        if (!_impl->compressor ) {
            throw io_exception(std::string("Cannot initialise the compressor: ")
                               + tjGetErrorStr2(_impl->compressor));
        }
    }

    unsigned char* output_buffer = nullptr;
    size_t output_size;

    int subsamp = TJ_420;
    if (tjCompress2(_impl->compressor, _impl->buffer, _impl->w, 0, _impl->h,
                    _impl->pix_fmt, &output_buffer, &output_size, subsamp, quality_percent, 0) != 0) {
        throw(std::string("Failed to compress the image: ")
                + tjGetErrorStr2(_impl->compressor));

    }
    scope_guard outputBufDeleter([&]() {
        tjFree(output_buffer);
    });

    std::ofstream ofs("output.jpg");
    ofs.write(reinterpret_cast<char*>(output_buffer), output_size);
    ofs.close();
}

void rgb_image::scale(rgb_image &output, int scale_percent) const
{
    if (scale_percent <= 0) {
        // scale_percent = 0;
        output._impl->resize(0, 0);
    } else if (scale_percent == 100) {
        // scale_percent = 100;
        output._impl->resize(_impl->w, _impl->h);
        memcpy(output._impl->buffer, _impl->buffer, _impl->bufsize);
    }

    float scaling_f = 100.0f / scale_percent; // for coordinates translation
    tjscalingfactor scaling{scale_percent, 100};
    int w = TJSCALED(_impl->w, scaling);
    int h = TJSCALED(_impl->h, scaling);
    output._impl->resize(w, h);

    for (int c = 0; c < w; ++c) {
        for (int r = 0; r < h; ++r) {
            output.set(c, r, at(scaling_f * c, scaling_f * r));
        }
    }
}

int rgb_image::w() const
{
    return _impl->w;
}

int rgb_image::h() const
{
    return _impl->h;
}

rgb_pixel rgb_image::at(int col, int row) const
{
    return _impl->at(col, row);
}

rgb_pixel rgb_image::at(float col, float row) const
{
    return _impl->at(col, row);
}

void rgb_image::set(int col, int row, const rgb_pixel &val) const
{
    _impl->set(col, row, val);
}

void rgb_image::dim(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 99) return; // nothing to do

    unsigned char* buffer = _impl->buffer;
    size_t size = _impl->bufsize;

    if (percent == 0) {
        for (size_t i = 0; i < _impl->bufsize; ++i)
            buffer[i] = 0;
    } else {
        tjscalingfactor scaling{percent, 100};
        for (size_t i = 0; i < size; ++i)
            buffer[i] = TJSCALED(buffer[i], scaling);
    }
}

void rgb_image::gamma(float g)
{
    if (g < 0.0f)
        g = 0.01f;

    unsigned char* buffer = _impl->buffer;
    size_t size = _impl->bufsize;
    for (size_t i = 0; i < size; ++i) {
        unsigned char c = buffer[i];
        float fc = float(c) / 255;
        fc = pow(fc, g) * 255.0f;
        c = round(fc);
        buffer[i] = c;
    }
}

void rgb_image::blur(int r)
{
    unsigned window_width = 2 * r + 1;
    float* gaussian_window = new float[window_width * window_width];
    scope_guard guard([&](){
        delete[] gaussian_window;
    });

    auto window_offset = [&](int col, int row) {
        return row * window_width + col;
    };

    for (int col = 0; col < window_width; ++col) {
        for (int row = 0; row <= col; ++row) {
            float d_square = (col - r) * (col - r) + (row - r) * (row - r);
            float value = expf(-d_square / 2.0f);

            gaussian_window[window_offset(col, row)] = value;
            // the window is symmetric, so reuse the value
            gaussian_window[window_offset(window_width - col - 1, window_width - row - 1)] = value;
        }
    }

    // normalise the window
    float sum = 0.0f;
    for (int col = 0; col < window_width; ++col)
        for (int row = 0; row < window_width; ++row)
            sum += gaussian_window[window_offset(col, row)];

    for (int col = 0; col < window_width; ++col) {
        for (int row = 0; row < window_width; ++row) {
            float& value = gaussian_window[window_offset(col, row)];
            value /= sum;
        }
    }

    // convolution operation
    // might take some time
    // ...and cpu
    for (int col = 0; col < _impl->w; ++col) {
        for (int row = 0; row < _impl->h; ++row) {
            rgb_pixel acc;
            for (int wc = -r; wc <= r; ++wc) {
                for (int wr = -r; wr <= r; ++wr) {
                    rgb_pixel pix = _impl->at(int(col + wc), int(row + wr));
                    float coef = gaussian_window[window_offset(wc + r, wr + r)];
                    acc.r += pix.r * coef;
                    acc.g += pix.g * coef;
                    acc.b += pix.b * coef;
                }
            }
            _impl->set(col, row, acc);
        }
    }
}


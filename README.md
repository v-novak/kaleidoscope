# Kaleidoscope filter
Built with no additional libraries, other than libjpeg-turbo

## Features:
* interpolation, resizing of the image
* `rgb_image` class is error-proof: when accessing individual pixels, it checks the indices and returns the edge pixels, or ignores the write operation with incorrect coodrinates
* color operations: dimming, gamma correction, Gaussian blur
* kaleidoscope effect with arbitrary number of "mirrors"

## Building and using

```
git clone https://github.com/libjpeg-turbo/libjpeg-turbo
mkdir build
cd build
cmake ..
make -j
./kaleidoscope input.jpg output.jpg
```


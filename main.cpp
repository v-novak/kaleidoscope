#include "kaleidoscope.h"
#include <iostream>
#include <string>
#include <fstream>

void usage(const char* executable)
{
    std::cout << "Usage: " << executable
              << " input_file.jpg output_file.jpg" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    try {
        rgb_image src;
        src.load(argv[1]);
        kaleidoscope(src);
        src.save(argv[2]);
        return 0;
    }  catch (io_exception& e) {
        std::cerr << "IO exception: " << e.what() << std::endl;
        return 2;
    }
}

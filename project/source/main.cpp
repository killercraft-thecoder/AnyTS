#include "setup.h"
#include "iostream_virt.h"
#include <cstdlib>

int main(int argc, char *argv[])
{
    Setup::initialize();

    if (argc < 2)
    {
        std::printf("Usage: path/to/built/runtime <script.ts>");
        return 1;
    }

    if (!Setup::runFile(argv[1]))
    {
        return 1;
    }

    return 0;
}
#include <iostream>

#include "lib.h"
#include "game_engine.h"

int main(int argc, char** argv)
{
    int a {3};
    int b {4};
    std::cout << a << 'x' << b << '=' << multiply(a, b) << '\n';
    return 0;
}
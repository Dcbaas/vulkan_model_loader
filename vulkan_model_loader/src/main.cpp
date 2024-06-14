#include <iostream>

#include "game_engine.h"

int main(int argc, char** argv)
{
    try
    {
        baas::game_engine::GameEngine engine = baas::game_engine::GameEngine();
    }
    catch (std::runtime_error re)
    {
        std::cerr << re.what() << '\n';
    }
    std::cout << "Hello Vulkan" << '\n';

    return 0;
}
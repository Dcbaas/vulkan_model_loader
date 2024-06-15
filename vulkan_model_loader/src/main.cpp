#include <iostream>

#include "game_engine.h"

int main(int argc, char** argv)
{
    std::cout << "Hello Vulkan" << '\n';
    try
    {
        baas::game_engine::GameEngine engine = baas::game_engine::GameEngine();
    }
    catch (std::runtime_error re)
    {
        std::cout << "Error: " << re.what() << '\n';
        return 1;
    }
    std::cout << "Hello Vulkan" << '\n';

    return 0;
}
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
    catch (std::exception any_ex)
    {
        std::cout << "Error: " << any_ex.what() << '\n';
        return 1;
    }

    return 0;
}
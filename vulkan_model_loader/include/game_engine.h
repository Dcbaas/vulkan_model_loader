#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <bitset>
namespace baas::game_engine
{

    // TODO Put this in a nicer place later
    constexpr uint32_t WIDTH = 800;
    constexpr uint32_t HEIGHT = 600;
    
    namespace engine_state_bit
    {
        constexpr std::size_t WINDOW_BIT {0};
        constexpr std::size_t VK_INSTANCE_BIT {1};
    }

    class GameEngine
    {
    public:
        GameEngine();
        ~GameEngine();
    private:
        GLFWwindow* window;

        vk::UniqueInstance vk_instance;
        vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> debug_messenger;
        vk::UniqueSurfaceKHR surface;

        std::bitset<2> engine_state;


        void init_window();
        void init_vulkan();

        void create_instance();
        
        void setup_debug_messenger();

        bool window_enabled() const 
        {
            return engine_state.test(engine_state_bit::WINDOW_BIT);
        }

        bool vk_instance_enabled() const
        {
            return engine_state.test(engine_state_bit::VK_INSTANCE_BIT);
        }
        
    };
}


#endif
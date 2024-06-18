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
        void main_loop();
    private:
        GLFWwindow* window;

        vk::UniqueInstance vk_instance;
        vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> debug_messenger;
        vk::UniqueSurfaceKHR surface;
        vk::PhysicalDevice physical_device;
        vk::UniqueDevice device;

        vk::Queue graphics_queue;
        vk::Queue present_queue;

        vk::UniqueSwapchainKHR swap_chain;
        std::vector<vk::Image> swap_chain_images;
        std::vector<vk::UniqueImageView> image_views;

        vk::UniqueRenderPass render_pass;

        std::bitset<2> engine_state;


        void init_window();
        void init_vulkan();

        void create_instance();

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
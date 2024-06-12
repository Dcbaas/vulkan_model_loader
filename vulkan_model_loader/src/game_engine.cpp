#include "game_engine.h"

#include <vector>
#include <cstdint>

namespace baas::game_engine
{

    void GameEngine::init_window()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Model Loader", nullptr, nullptr);
        
        engine_state.set(engine_state_bit::WINDOW_BIT);
        glfwSetWindowUserPointer(window, this);

        // TODO Add Frame Buffer Size callback
    }

    void GameEngine::create_instance()
    {
        // TODO Add Validation Layers Support

        vk::ApplicationInfo app_info{};
        app_info.pApplicationName = "Vulkan Model Loader";
        app_info.applicationVersion = vk::makeVersion(1, 0, 0);
        app_info.pEngineName = "Vulkan HPP";
        app_info.engineVersion = vk::makeVersion(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_3;

        vk::InstanceCreateInfo create_info{};
        create_info.pApplicationInfo = &app_info;

        auto extensions = get_required_extensions();
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        // TODO Add validation layers check

        // TODO handle exceptions
        vk_instance = vk::createInstance(create_info);
    }

    std::vector<const char*> get_required_extensions()
    {
        uint32_t glfw_ext_count{0};
        const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

        std::vector<const char*> extensions(glfw_ext, glfw_ext + glfw_ext_count);

        return extensions;
    }
}
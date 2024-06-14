#include "game_engine.h"

#include <algorithm>
#include <vector>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>

namespace baas::game_engine
{

    #ifdef NDEBUG
    constexpr bool enable_validation_layers = false;
    #else
    constexpr bool enable_validation_layers = true;
    #endif

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };    

    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    std::vector<const char*> get_required_extensions();

    GameEngine::GameEngine()
    {
        init_window();
        init_vulkan();
    }

    GameEngine::~GameEngine()
    {
        // if (enable_validation_layers)
        // {
        //     vk_instance->destroyDebugUtilsMessengerEXT();
        // }
        
        // if (vk_instance_enabled())
        // {
        //     vk_instance->destroy();
        // }

        if (window_enabled())
        {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }


    void GameEngine::init_window()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Model Loader", nullptr, nullptr);
        
        engine_state.set(engine_state_bit::WINDOW_BIT);
        glfwSetWindowUserPointer(window, this);

        // TODO Add Frame Buffer Size callback
    }

    void GameEngine::init_vulkan()
    {
        // Create Instance
        vk::ApplicationInfo app_info{};
        app_info.pApplicationName = "Vulkan Model Loader";
        app_info.applicationVersion = vk::makeVersion(1, 0, 0);
        app_info.pEngineName = "Vulkan HPP";
        app_info.engineVersion = vk::makeVersion(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_2;

        vk::InstanceCreateInfo create_info{};
        create_info.pApplicationInfo = &app_info;

        auto extensions = get_required_extensions();
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        if (enable_validation_layers)
        {
            create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            create_info.ppEnabledLayerNames = validationLayers.data();
        }
        // TODO handle exceptions
        vk_instance = vk::createInstanceUnique(create_info);
        
        setup_debug_messenger();

        // Setup Surface 
        VkSurfaceKHR surface_temp;
        auto result = glfwCreateWindowSurface(*vk_instance, window, nullptr, &surface_temp);
        if (result != VK_SUCCESS)
        {
            /* code */
            throw std::runtime_error("Failed to create surface");
        }
        surface = vk::UniqueSurfaceKHR(surface_temp, *vk_instance);

        auto physical_devices = vk_instance->enumeratePhysicalDevices();
        auto physicalDevice = physical_devices[std::distance(physical_devices.begin(),
        std::find_if(physical_devices.begin(), physical_devices.end(), [](const vk::PhysicalDevice& physical_device) {
            auto available_extensions = physical_device.enumerateDeviceExtensionProperties();
            std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
            for (auto &&extension : available_extensions)
            {
                required_extensions.erase(extension.extensionName);
            }
            
        }))];
        
    }

    void GameEngine::setup_debug_messenger()
    {

        // TODO Add validation layers check
        if (enable_validation_layers)
        {
            auto dynamic_dispatch_loader = vk::DispatchLoaderDynamic(*vk_instance, vkGetInstanceProcAddr);

            using debug_util_bits = vk::DebugUtilsMessageSeverityFlagBitsEXT;
            auto severity_flags = debug_util_bits::eVerbose | debug_util_bits::eWarning | debug_util_bits::eError;
            using message_type_bits = vk::DebugUtilsMessageTypeFlagBitsEXT;
            auto message_type_flags = message_type_bits::eGeneral | message_type_bits::eValidation | message_type_bits::ePerformance;

            using DebugCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT;
            DebugCreateInfo debug_create_info = DebugCreateInfo({}, severity_flags, message_type_flags, debug_callback);
            debug_messenger = vk_instance->createDebugUtilsMessengerEXTUnique(debug_create_info, nullptr, dynamic_dispatch_loader);
        }
    }

    std::vector<const char*> get_required_extensions()
    {
        uint32_t glfw_ext_count{0};
        const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

        std::vector<const char*> extensions(glfw_ext, glfw_ext + glfw_ext_count);

        return extensions;
    }

    // This is ripped from the tutorial. Eventually figure out how to do this in a cpp sort of way
    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';

        return VK_FALSE;
    } 
}
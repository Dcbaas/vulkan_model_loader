#include "game_engine.h"

#include <vector>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace baas::game_engine
{

    #ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
    #else
    constexpr bool enableValidationLayers = true;
    #endif

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };


    // // These are ripped straight from vulkan hpp. Idk why they're here. Need to find out later
    PFN_vkCreateDebugUtilsMessengerEXT  pfnVkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

    ///////// These two functions are ripped straight from the samples page on vulken hpp
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT( VkInstance                                 instance,
                                                                const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo,
                                                                const VkAllocationCallbacks *              pAllocator,
                                                                VkDebugUtilsMessengerEXT *                 pMessenger )
    {
        PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        if (!pfnVkCreateDebugUtilsMessengerEXT)
        {
            throw std::runtime_error("GetInstanceProcAddr: Unable to find pfnVkCreateDebugUtilsMessengerEXT function.");
        }
        return pfnVkCreateDebugUtilsMessengerEXT( instance, pCreateInfo, pAllocator, pMessenger );
    }

    VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks const * pAllocator )
    {
            PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
            if (!pfnVkDestroyDebugUtilsMessengerEXT)
            {
                throw std::runtime_error("GetInstanceProcAddr: Unable to find pfnVkDestroyDebugUtilsMessengerEXT function.");
            }
        return pfnVkDestroyDebugUtilsMessengerEXT( instance, messenger, pAllocator );
    }
    // !end debug samples
    

    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    std::vector<const char*> get_required_extensions();

    GameEngine::GameEngine()
    {
        init_window();
        init_vulkan();
    }

    GameEngine::~GameEngine()
    {
        if (enableValidationLayers)
        {
            vk_instance.destroyDebugUtilsMessengerEXT();
        }
        
        if (vk_instance_enabled())
        {
            vk_instance.destroy();
        }

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
        create_instance();
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

        // if (enableValidationLayers)
        // {
        //     create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        //     create_info.ppEnabledLayerNames = validationLayers.data();
        // }
        // TODO handle exceptions
        vk_instance = vk::createInstance(create_info);
    }

    void GameEngine::setup_debug_messenger()
    {
        // TODO Add validation layers check
        if (enableValidationLayers)
        {
            // pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vk_instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
            // if (!pfnVkDestroyDebugUtilsMessengerEXT)
            // {
            //     throw std::runtime_error("GetInstanceProcAddr: Unable to find pfnVkDestroyDebugUtilsMessengerEXT function.");
            // }

            using debug_util_bits = vk::DebugUtilsMessageSeverityFlagBitsEXT;
            auto severity_flags = debug_util_bits::eVerbose | debug_util_bits::eWarning | debug_util_bits::eError;
            using message_type_bits = vk::DebugUtilsMessageTypeFlagBitsEXT;
            auto message_type_flags = message_type_bits::eGeneral | message_type_bits::eValidation | message_type_bits::ePerformance;

            using DebugCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT;
            DebugCreateInfo debug_create_info = DebugCreateInfo({}, severity_flags, message_type_flags, &debug_callback);
            debug_messenger = vk_instance.createDebugUtilsMessengerEXT(debug_create_info);
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
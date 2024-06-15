#include "game_engine.h"

#include <algorithm>
#include <vector>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

namespace baas::game_engine
{

    #ifdef NDEBUG
    constexpr bool enable_validation_layers = false;
    #else
    constexpr bool enable_validation_layers = true;
    #endif

    // Utility Struct to determine queue families 
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    // Utility Struct to determine swap chain support
    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

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
        app_info.apiVersion = vk::ApiVersion10;

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

        // Choose PhysicalDevice
        auto physical_devices = vk_instance->enumeratePhysicalDevices();
        auto find_queue_families = [this](vk::PhysicalDevice& physical_device)
        {
            // Check Queue Families
            QueueFamilyIndices indicies;
            auto queue_family_properties = physical_device.getQueueFamilyProperties();
            uint32_t i {0};
            for (auto &&queue_family : queue_family_properties)
            {
                if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
                {
                    indicies.graphicsFamily = i;
                }
                VkBool32 present_support = physical_device.getSurfaceSupportKHR(i, *this->surface);
                if (present_support)
                {
                    indicies.presentFamily = i;
                }

                if (indicies.isComplete())
                {
                    break;
                }
                ++i;
            }

            return indicies;
        };

        auto all_device_ext_present = [](vk::PhysicalDevice& physical_device)
        {
            // Check extension support
            auto available_extensions = physical_device.enumerateDeviceExtensionProperties();
            const char* required_extensions = vk::KHRSwapchainExtensionName;
            return std::any_of(available_extensions.begin(), available_extensions.end(), 
            [](vk::ExtensionProperties& extension)
            {
                std::string extension_name = std::string(extension.extensionName.data());
                std::string expected_ext = std::string(vk::KHRSwapchainExtensionName);
                return expected_ext == extension_name;
            });
        };

        auto get_swap_chain_support_info = [this](vk::PhysicalDevice& physical_device)
        {
            SwapChainSupportDetails details;
            details.capabilities = physical_device.getSurfaceCapabilitiesKHR(*this->surface);
            details.formats = physical_device.getSurfaceFormatsKHR(*this->surface);
            details.presentModes = physical_device.getSurfacePresentModesKHR(*this->surface);
            return details;
        };

        std::optional<vk::PhysicalDevice> chosen_device; // This is probably dumb TODO. Find a way to do this better
        QueueFamilyIndices indicies;
        SwapChainSupportDetails swap_chain_details;
        for (auto &&tmp_physical_device : physical_devices)
        {
            indicies = find_queue_families(tmp_physical_device);
            bool extensions_supported = all_device_ext_present(tmp_physical_device);
            swap_chain_details = get_swap_chain_support_info(tmp_physical_device);
            bool swap_chains_adequate = !swap_chain_details.formats.empty() && !swap_chain_details.presentModes.empty();
            if (indicies.isComplete() && extensions_supported && swap_chains_adequate)
            {
                chosen_device = tmp_physical_device;
                break;
            }
        }

        if (!chosen_device.has_value())
        {
            throw std::runtime_error("Failed to find a suitable GPU");
        }
        else
        {
            physical_device = chosen_device.value();
        }
    }

    void GameEngine::setup_debug_messenger()
    {
        // TODO Add validation layers check
        if (enable_validation_layers)
        {
            auto dynamic_dispatch_loader = vk::DispatchLoaderDynamic(*vk_instance, vkGetInstanceProcAddr);

            auto create_function = dynamic_dispatch_loader.vkCreateDebugUtilsMessengerEXT;
            if (create_function == nullptr)
            {
                throw std::runtime_error("vkCreateDebugUtilsMessengerEXT is null");
            }
            auto destroy_function = dynamic_dispatch_loader.vkDestroyDebugUtilsMessengerEXT;
            if (destroy_function == nullptr)
            {
                throw std::runtime_error("vkDestroyDebugUtilsMessengerEXT is null");
            }

            using debug_util_bits = vk::DebugUtilsMessageSeverityFlagBitsEXT;
            auto severity_flags = debug_util_bits::eVerbose | debug_util_bits::eWarning | debug_util_bits::eError;
            using message_type_bits = vk::DebugUtilsMessageTypeFlagBitsEXT;
            auto message_type_flags = message_type_bits::eGeneral | message_type_bits::eValidation | message_type_bits::ePerformance;

            using DebugCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT;
            DebugCreateInfo debug_create_info = DebugCreateInfo({}, severity_flags, message_type_flags, debug_callback);
            // TODO: It seems that the function for creating the messenger is null causing seg fault. Find out why this is. 
            debug_messenger = vk_instance->createDebugUtilsMessengerEXTUnique(debug_create_info, nullptr, dynamic_dispatch_loader);
        }
    }

    std::vector<const char*> get_required_extensions()
    {
        uint32_t glfw_ext_count{0};
        const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

        std::vector<const char*> extensions(glfw_ext, glfw_ext + glfw_ext_count);
        if (enable_validation_layers)
        {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }
        return extensions;
    }

    // This is ripped from the tutorial. Eventually figure out how to do this in a cpp sort of way
    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';

        return VK_FALSE;
    } 
}
#include "game_engine.h"

#include <algorithm>
#include <iterator>
#include <vector>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

#include "file_ops.h"

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

    void glfw_key_press_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        }
        
    }

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
        glfwSetKeyCallback(window, glfw_key_press_callback);

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
        vk_instance = vk::createInstanceUnique(create_info);
        
        // Setup Debugger
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

        // Create Logical Device
        std::vector<uint32_t> unique_queue_families;
        if (indicies.graphicsFamily == indicies.presentFamily)
        {
            unique_queue_families.push_back(indicies.graphicsFamily.value());
        }
        else
        {
            unique_queue_families.push_back(indicies.graphicsFamily.value());
            unique_queue_families.push_back(indicies.presentFamily.value());
        }

        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        float queue_priority = 1.0f;
        for (auto &&queue_family_index : unique_queue_families)
        {
            // vk::DeviceQueueCreateInfo queue_create_info(vk::DeviceQueueCreateFlags(), queue_family_index, 1.0f); // This constructor didn't work. Why? 
            vk::DeviceQueueCreateInfo queue_create_info(vk::DeviceQueueCreateFlags(),queue_family_index, 1, &queue_priority);
            queue_create_infos.push_back(queue_create_info);
        }

        std::vector<const char*> enabled_layers;
        if (enable_validation_layers)
        {
            std::copy(validationLayers.begin(), validationLayers.end(), std::back_inserter(enabled_layers));
        }
        
        vk::DeviceCreateInfo device_create_info(vk::DeviceCreateFlags(),queue_create_infos, enabled_layers, device_extensions); // TODO this might not be right
        
        device = physical_device.createDeviceUnique(device_create_info);

        graphics_queue = device->getQueue(indicies.graphicsFamily.value(), 0);
        present_queue = device->getQueue(indicies.presentFamily.value(), 0);

        // Create Swap Chains
        vk::Format chosen_format;
        vk::ColorSpaceKHR chosen_color_space;
        for (auto &&available_format : swap_chain_details.formats)
        {
            if (available_format.format == vk::Format::eB8G8R8A8Srgb && available_format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear)
            {
                chosen_format = available_format.format;
                chosen_color_space = available_format.colorSpace;
                break;
            }
        }

        vk::PresentModeKHR chosen_present_mode{vk::PresentModeKHR::eFifo}; // Fallback to fifo if mailbox doesn't exist.
        for (auto &&present_mode : swap_chain_details.presentModes)
        {
            if (present_mode == vk::PresentModeKHR::eMailbox)
            {
                chosen_present_mode = present_mode;
                break;
            }
        }

        vk::Extent2D chosen_extent;
        if (swap_chain_details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            chosen_extent = swap_chain_details.capabilities.currentExtent;
        }
        else
        {
            int width;
            int height;
            auto capabilities = swap_chain_details.capabilities; // temp declaration here for simpler referencing.
            glfwGetFramebufferSize(window, &width, &height);

            chosen_extent = vk::Extent2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            chosen_extent.width = std::clamp(chosen_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            chosen_extent.height = std::clamp(chosen_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        uint32_t image_count = swap_chain_details.capabilities.minImageCount + 1;
        if (swap_chain_details.capabilities.maxImageCount > 0 && image_count > swap_chain_details.capabilities.maxImageCount) {
            image_count = swap_chain_details.capabilities.maxImageCount;
        }
        
        vk::ImageUsageFlags image_usage_flags(vk::ImageUsageFlagBits::eColorAttachment);
        std::vector<uint32_t> swap_info_queue_indicies{indicies.graphicsFamily.value(), indicies.presentFamily.value()};
        
        vk::SwapchainCreateFlagsKHR swap_flags{};
        uint32_t image_array_layers{1};
        vk::SwapchainCreateInfoKHR swap_chain_info(swap_flags, surface.get(), image_count, chosen_format, chosen_color_space, chosen_extent, image_array_layers, image_usage_flags, vk::SharingMode::eExclusive, swap_info_queue_indicies, vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, chosen_present_mode, true, nullptr);
        
        swap_chain = device->createSwapchainKHRUnique(swap_chain_info);
        swap_chain_images = device->getSwapchainImagesKHR(swap_chain.get());
        
        // Create ImageViews
        image_views.reserve(swap_chain_images.size());
        for (auto &&image : swap_chain_images)
        {
            vk::ImageViewType image_view_type = vk::ImageViewType::e2D;
            vk::ComponentMapping component_mapping{vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA};
            vk::ImageSubresourceRange sub_resource_range{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
            vk::ImageViewCreateInfo image_view_create_info(vk::ImageViewCreateFlags(), image, image_view_type, chosen_format, component_mapping, sub_resource_range);
            image_views.push_back(device->createImageViewUnique(image_view_create_info));
        }

        // Create Render Pass
        auto samples = vk::SampleCountFlagBits::e1;
        auto load_op = vk::AttachmentLoadOp::eClear;
        auto store_op = vk::AttachmentStoreOp::eStore;
        auto stencil_load_op = vk::AttachmentLoadOp::eDontCare;
        auto stencil_store_op = vk::AttachmentStoreOp::eDontCare;
        auto init_layout = vk::ImageLayout::eUndefined;
        auto final_layout = vk::ImageLayout::ePresentSrcKHR;
        auto color_attachment = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(), chosen_format, samples, load_op, store_op, stencil_load_op, stencil_store_op, init_layout, final_layout);
        auto color_attachment_ref = vk::AttachmentReference(0U, vk::ImageLayout::eColorAttachmentOptimal);

        auto pipeline_bind_point = vk::PipelineBindPoint::eGraphics;
        auto subpass_desc = vk::SubpassDescription(vk::SubpassDescriptionFlags(), pipeline_bind_point, 0U, nullptr, 1U, &color_attachment_ref);

        auto pipeline_stage_flags= vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        auto src_access_flags = vk::AccessFlags();
        auto dest_access_flags = vk::AccessFlags(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
        auto subpass_dependency = vk::SubpassDependency(VK_SUBPASS_EXTERNAL, 0U, pipeline_stage_flags, pipeline_stage_flags, src_access_flags, dest_access_flags);

        auto render_pass_create_info = vk::RenderPassCreateInfo(vk::RenderPassCreateFlags(), 1U, &color_attachment, 1U, &subpass_desc, 1U, &subpass_dependency);
        render_pass = device->createRenderPassUnique(render_pass_create_info);

        // Create Graphics Pipeline
    }

    void GameEngine::main_loop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
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
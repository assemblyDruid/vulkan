#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <assert.h>

const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers =
{
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
const bool enableDebugInfoMessages = false; // Change to disable Debug [ Info ] messages.
#endif


VkResult
CreateDebugUtilsMessengerEXT(VkInstance                                instance,
                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                             const VkAllocationCallbacks*              pAllocator,
                             VkDebugUtilsMessengerEXT*                 pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                          "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}


void
DestroyDebugUtilsMessengerEXT(VkInstance               instance,
                              VkDebugUtilsMessengerEXT debugMessenger,
                              VkAllocationCallbacks*   pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                            "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}


class HelloTriangleApplication
{
public:
    void
    Run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }


private:
    void
    InitWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "HelloTriangleApplication", nullptr, nullptr);
    }


    bool
    CheckValidationLayerSupport()
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }


    static VKAPI_ATTR VkBool32 VKAPI_CALL
    DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT             messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void*                                       pUserData)
    {
        if (pUserData && messageType) {} // Silence unused arguments warning
        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            std::cout << "[ INFO ] Validation layer: " << pCallbackData->pMessage << std::endl;
        }
        else
        {
            std::cerr << "[ ERROR ] Validation layer: " << pCallbackData->pMessage << std::endl;
        }
        return VK_FALSE; // Determines if the calling Vulkan function should be aborted
    }


    void
    PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        if (enableDebugInfoMessages)
        {
            std::cout << "[ INFO ] Info messages [ enabled ]." << std::endl;
            createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        }

        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;
    }


    void
    SetupDebugMessenger()
    {
        if (!enableValidationLayers) return;
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        PopulateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(vulkanInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("[ ERROR ] Failed to set up the debug messenger");
        }
    }


    void
    CreateVulkanInstance()
    {
        // Specify application info
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // Specify instance info
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

        // Check to see if validation layers are turned on. If so, validate that they are available and add them to the
        // createInfo struct.
        std::vector<const char*> extensions;
        if (enableValidationLayers)
        {
            if (!CheckValidationLayerSupport())
            {
                throw std::runtime_error("[ ERROR ] Validation layers requested, but not available.");
            }
            std::cout << "[ INFO ] Validation layers [ enabled ]." << std::endl;

            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

            // [ cfarvin::NOTE ] In the example, this was in it's own function, getRequiredExtensions()
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            extensions = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

            // We're adding these optional extensions. The GLFW ones are always required.
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
        }
        else
        {
            std::cout << "Validation layers [ disabled ]." << std::endl;
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &vulkanInstance) != VK_SUCCESS)
        {
            throw std::runtime_error("[ ERROR ] Failed to create a Vulkan instance.");
        }
    }


    // [ cfarvin::NOTE ] Physical devices are cleaned up when the instance is destroyed by default,
    // and do not need to be manually cleaned up.
    // [ cfarvin::DEVIATION ] We are going to deviate from the tutorial here and require that the
    // same physical device be capable of *both* graphics and present functionality, understanding that
    // this is poor production practice and should be taken into account at a later time.
    void
    FindGraphicsCompatibleDevice()
    {
        // Determine devices quantity
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);

        if (!deviceCount)
        {
            throw std::runtime_error("[ ERROR ] No Vulkan compatible GPUs found.");
        }

        // Store all devices
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices.data());

        // Find and use the first graphics compatible device found
        VkBool32 foundGraphicsCapableDevice = VK_FALSE;
        VkBool32 foundPresentCapableDevice = VK_TRUE;
        for (const auto& thisDevice : devices)
        {
            if (foundGraphicsCapableDevice && foundPresentCapableDevice) { break; }

            // Determine queue family quantity per device
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(thisDevice, &queueFamilyCount, nullptr);

            // Store all queue family properties per queue family
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(thisDevice,
                                                     &queueFamilyCount,
                                                     queueFamilies.data());

            // Determine if any queue in any queue family is both graphics and present compatible
            uint32_t queueFamilyIndex = 0;
            for (const auto& queueFamily: queueFamilies)
            {
                // [ cfarvin::NOTE ] foundCapablePresentDevice is set in vkGetPhysicalDeviceSurfaceSupportKHR
                vkGetPhysicalDeviceSurfaceSupportKHR(thisDevice,
                                                     queueFamilyIndex,
                                                     surface,
                                                     &foundPresentCapableDevice);

                if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && foundPresentCapableDevice)
                {
                    physicalDevice = thisDevice;
                    foundGraphicsCapableDevice = VK_TRUE;
                    GRAPHICAL_AND_PRESENT_QUEUE_FAMILY_INDEX = queueFamilyIndex;
                    break;
                }
                queueFamilyIndex++;
            }
        }

        // Ensure that a suitable physical device was successfuly aquired.
        if (physicalDevice == VK_NULL_HANDLE)
        {
            if (!foundGraphicsCapableDevice)
            {
                std::cerr << "[ ERROR ] No graphics capable device found." << std::endl;
            }

            if (!foundPresentCapableDevice)
            {
                std::cerr << "[ ERROR ] No present capable device found." << std::endl;
            }
            throw std::runtime_error("[ ERROR ] No suitible  GPUs found.");
        }
    }


    void
    CreateLogicalDevice()
    {
        // Specify the queues to be created
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = GRAPHICAL_AND_PRESENT_QUEUE_FAMILY_INDEX;
        queueCreateInfo.queueCount = 1;

        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // Specify device features
        // [ cfarvin::TODO ] Come back to this when we want to do more than
        // the initial setup.
        VkPhysicalDeviceFeatures deviceFeatures = {};

        // Specify logical device properties
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        // Create the logical device
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("[ ERROR ] Failed to create logical deivce.");
        }

        vkGetDeviceQueue(device, GRAPHICAL_AND_PRESENT_QUEUE_FAMILY_INDEX, 0, &graphicsQueue);
    }


    void
    CreateSurface()
    {
        if (glfwCreateWindowSurface(vulkanInstance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("[ ERROR ] Failed to create window surface.");
        }
    }


    void
    InitVulkan()
    {
        CreateVulkanInstance();

        if (enableValidationLayers)
        {
            SetupDebugMessenger();
        }

        CreateSurface();
        FindGraphicsCompatibleDevice();
        CreateLogicalDevice();
    }


    void
    MainLoop()
    {
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }


    void
    Cleanup()
    {
        // Vulkan cleanup
        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
        }

        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
        vkDestroyInstance(vulkanInstance, nullptr);

        // GLFW Cleanup
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLFWwindow*              window = nullptr;
    VkInstance               vulkanInstance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
    VkDevice                 device; // [ cfarvin::NOTE ] Think "logcial device"
    uint32_t                 GRAPHICAL_AND_PRESENT_QUEUE_FAMILY_INDEX;
    uint32_t                 PRESENT_QUEUE_FMAILY_INDEX;
    VkQueue                  graphicsQueue;
    VkSurfaceKHR             surface;
};


int
main(int argc, char** argv)
{
    if (argc && argv) {} // silence unused argument error/warnings

    HelloTriangleApplication app;

    try
    {
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

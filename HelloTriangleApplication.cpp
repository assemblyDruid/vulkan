#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <cstring>

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
const bool enableDebugInfoMessages = true; // Change to disable Debug [ Info ] messages.
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance                                instance,
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

class HelloTriangleApplication
{
public:
    void
    run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }


private:
    void
    initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "HelloTriangleApplication", nullptr, nullptr);
    }


    bool
    checkValidationLayerSupport()
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
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
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


    void setupDebugMessenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        if (enableDebugInfoMessages)
        {
            std::cout << "[ INFO ] Info messages enabled." << std::endl;
            createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        }

        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        if (CreateDebugUtilsMessengerEXT(vulkanInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("[ ERROR ] Failed to set up the debug messenger");
        }
    }


    void
    createVulkanInstance()
    {
        // Vulkan Struct Documentation:
        //
        // typedef struct VkApplicationInfo {
        //     VkStructureType    sType;              // The type of the structure
        //     const void*        pNext;              // NULL or a pointer to an extension-specific structure
        //     const char*        pApplicationName;   // NULL or a NULLTERM UTF8 string naming the application
        //     uint32_t           applicationVersion; // Unsigned integer containing developer version of app
        //     const char*        pEngineName;        // NULL or poitner to NULLTERM UTF8 with name of engine creating the app
        //     uint32_t           engineVersion;      // Unsigned integer of engine version creating the app
        //     uint32_t           apiVersion;         // *Must* be highest possible Vulkan version number
        // } VkApplicationInfo;
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // Vulkan Struct Documentation:
        //
        // typedef struct VkInstanceCreateInfo {
        //     VkStructureType             sType;                   // The type of the structure
        //     const void*                 pNext;                   // NULL or poitner to an extension-specific structure
        //     VkInstanceCreateFlags       flags;                   // Reserved for future use
        //     const VkApplicationInfo*    pApplicationInfo;        // NULL or VkApplicationInfo*
        //     uint32_t                    enabledLayerCount;       // Number of global layers to enable
        //     const char* const*          ppEnabledLayerNames;     // enabledLayerCount** NULLTERM UTF8 layer names
        //     uint32_t                    enabledExtensionCount;   // Number of global extensions to enable
        //     const char* const*          ppEnabledExtensionNames; // enabledExtensionCount NULLTERM UTF8 extension names
        // } VkInstanceCreateInfo;
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Check to see if validation layers are turned on. If so, validate that they are available and add them to the
        // createInfo struct.
        std::vector<const char*> extensions;
        if (enableValidationLayers)
        {
            if (!checkValidationLayerSupport())
            {
                throw std::runtime_error("[ ERROR ] Validation layers requested, but not available.");
            }
            std::cout << "[ INFO ] Validation layers [ enabled ]." << std::endl;

            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

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

    void
    initVulkan()
    {
        createVulkanInstance();

        if (!enableValidationLayers)
        {
            setupDebugMessenger();
        }
    }


    void
    mainLoop()
    {
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }


    void
    cleanup()
    {
        vkDestroyInstance(vulkanInstance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }


    GLFWwindow* window;
    VkInstance vulkanInstance;
    VkDebugUtilsMessengerEXT debugMessenger;
};


int
main(int argc, char** argv)
{
    if (argc && argv) {} // silence unused argument error/warnings

    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

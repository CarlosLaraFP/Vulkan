#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> // GLFW will include its own definitions and automatically load <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // provides the EXIT_SUCCESS and EXIT_FAILURE macros
#include <vector>
#include <algorithm> //std::find_if; C++20 has the more concise std::ranges::find in <ranges>
#include <sstream> // std::ostringstream; C++20 has std::format in <format>
#include <cstring> // strcmp compares two strings character by character. If the strings are equal, the function returns 0.

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// All of the useful standard validation is bundled into a layer included in the SDK:
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

// The NDEBUG macro is part of the C++ standard and means "not debug"
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

// A lot of information in Vulkan is passed through structs instead of function parameters.
class HelloTriangleApplication 
{
public:
    void run() 
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;
    VkInstance instance;

    void initWindow() 
    {
        if (!glfwInit())
        {
            throw std::runtime_error("GLFW could not be initialized.");
        }

        // Because GLFW was originally designed to create an OpenGL context, we need to tell it to not create an OpenGL context.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Because handling resized windows takes special care that we’ll look into later, disable it for now.
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // The 4th parameter allows you to optionally specify a monitor to open the window on and the last parameter is only relevant to OpenGL.
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

        if (window == nullptr)
        {
            glfwTerminate();

            throw std::runtime_error("Window could not be created.");
        }
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        /*
            Returns an array of names of Vulkan instance extensions required by GLFW for creating Vulkan surfaces for GLFW windows.
            If successful, the list will always contain VK_KHR_surface, so if you don't require any
            additional extensions you can pass this list directly to the VkInstanceCreateInfo struct.
        */
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> requiredExtensions { glfwExtensions, glfwExtensions + glfwExtensionCount };

        // The extensions specified by GLFW are always required, but the debug messenger extension is conditionally added.
        if (enableValidationLayers)
        {
            requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Using this macro rather than the literal helps avoid typos.
        }

        return requiredExtensions;
    }

    void checkVulkanExtensions(const std::vector<const char*>& requiredExtensions)
    {
        // 1. To allocate an array to hold the extension details, we first need to know how many there are:
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        // 2. Allocate an array to hold the extension details. Each VkExtensionProperties struct contains the name and version of an extension.
        std::vector<VkExtensionProperties> vkExtensions{ extensionCount };

        // 3. Query the extension details:
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vkExtensions.data());

        std::cout << "Available Vulkan extensions:" << std::endl;

        for (const auto& vkExtension : vkExtensions) 
        {
            std::cout << '\t' << vkExtension.extensionName << std::endl;
        }

        for (const auto& extension : requiredExtensions)
        {
            auto iterator = std::find_if(
                vkExtensions.begin(),
                vkExtensions.end(),
                [&extension](const VkExtensionProperties& vkExtension) 
                {
                    // strcmp needed for C-style string comparison
                    return strcmp(vkExtension.extensionName, extension) == 0;
                    //return vkExtension.extensionName == extension;
                }
            );

            if (iterator == vkExtensions.end())
            {
                std::ostringstream output;
                output << "Required extension " << extension << " is not supported by Vulkan.";

                throw std::runtime_error(output.str());
            }
        }
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers { layerCount };
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        std::cout << "Validation Layers supported by Vulkan:" << std::endl;

        for (const char* layerName : validationLayers) 
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) 
            {
                std::cout << '\t' << layerProperties.layerName << std::endl;

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

    /*
        The instance is the connection between your application and the Vulkan library and 
        creating it involves specifying some details about your application to the driver.
    */
    void createInstance()
    {
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("Validation layers requested, but not available.");
        }

        /*
            This data is technically optional, but it may provide some useful information to the driver in order to optimize 
            your specific application (e.g. because it uses a well-known graphics engine with certain special behavior).
        */
        VkApplicationInfo appInfo {};

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        /*
            This is not optional and tells the Vulkan driver which global extensions and validation layers we want to use. 
            Global here means that they apply to the entire program and not a specific device.
        */
        auto extensions = getRequiredExtensions();

        /*
            Per the vkCreateInstance documentation, one of the possible error codes is VK_ERROR_EXTENSION_NOT_PRESENT.
            We could simply specify the extensions we require and terminate if that error code comes back.
            That makes sense for essential extensions like the window system interface, but what if we want to check for optional functionality?
        */
        checkVulkanExtensions(extensions);

        VkInstanceCreateInfo createInfo {};
         
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        /*
            Specify the desired global extensions; since Vulkan is a platform agnostic API, we need an extension to interface with the window 
            system. GLFW has a handy built-in function that returns the extension(s) it needs to do that - which we can pass to the struct.
        */
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        // The last two members determine the global validation layers to enable (for debug mode).
        if (enableValidationLayers) 
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else 
        {
            createInfo.enabledLayerCount = 0;
        }

        /*
            This is the general pattern followed by object creation function parameters in Vulkan:

            Pointer to struct with creation info.
            Pointer to custom allocator callbacks, always nullptr in this tutorial.
            Pointer to the variable that stores the handle to the new object.
        
            Nearly all Vulkan functions return a value of type VkResult that is either VK_SUCCESS or an error code. 
            To check if the instance was created successfully, we can just check for the success value:
        */
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) 
        {
            throw std::runtime_error("Failed to create VkInstance.");
        }
    }

    void initVulkan() 
    {
        createInstance();
    }

    void mainLoop() 
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup() 
    {
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() 
{
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

/*
    Object/Model Space: Coordinates are defined relative to the local origin of the geometric object. This is where vertices of a model typically start.

    World Space: After applying the model transformation, coordinates are in a common space shared by all objects in the scene.

    View/Camera Space: Applying the view transformation positions objects relative to the camera's viewpoint.

    Clip Space: After the projection transformation, coordinates are in a space where they can be clipped against the view frustum. Vertices are then perspective-divided to go from clip space to NDC.

    NDC (Normalized Device Coordinates): In this space, the x, y, and z coordinates are normalized to be within a standard cubic volume (in Vulkan, x and y range from -1.0 to 1.0, and z from 0.0 to 1.0 for the default depth range). This space is what the viewport transformation uses to map to framebuffer coordinates.

    Framebuffer/Window Space: Finally, the viewport transformation maps NDC to the actual pixels in the framebuffer or the window surface being rendered to.

    NDC is a crucial step in the pipeline that standardizes how geometry is represented before it’s mapped onto the screen or the target framebuffer, ensuring consistent rendering across different hardware and graphics APIs.
*/
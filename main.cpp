#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> // GLFW will include its own definitions and automatically load <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // provides the EXIT_SUCCESS and EXIT_FAILURE macros
#include <vector>
#include <algorithm> //std::clamp and std::find_if; C++20 has the more concise std::ranges::find in <ranges>
#include <sstream> // std::ostringstream; C++20 has std::format in <format>
#include <cstring> // strcmp compares two strings character by character. If the strings are equal, the function returns 0.
#include <optional> // C++17
#include <set>
#include <cstdint> // uint32_t
#include <limits> // std::numeric_limits
#include <fstream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// All of the useful standard validation is bundled into a layer included in the SDK:
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

// Required device extensions in addition to instance extensions (GLFW and debug)
/*
    Not all graphics cards are capable of presenting images directly to a screen for various reasons, for example because 
    they are designed for servers and don’t have any display outputs. Secondly, since image presentation is heavily tied 
    into the window system and the surfaces associated with windows, it is not actually part of the Vulkan core. 
    You have to enable the VK_KHR_swapchain device extension after querying for its support.
*/
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME }; // macro helps the compiler catch misspellings

// The NDEBUG macro is part of the C++ standard and means "not debug"
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

/*
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message like the creation of a resource
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Message about behavior that is not necessarily an error, but very likely a bug in your application
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Message about behavior that is invalid and may cause crashes

    The values of this enumeration are set up in such a way that you can use a comparison operation to check 
    if a message is equal or worse compared to some level of severity, for example:

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) 
    {
        // Message is important enough to show
    }

    The messageType parameter can have the following values:

    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Some event has happened that is unrelated to the specification or performance
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something has happened that violates the specification or indicates a possible mistake
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkan
*/
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData // specified during the setup of the callback and allows you to pass your own data to it
) {
    std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;

    /*
        The callback returns a boolean that indicates if the Vulkan call that triggered the validation layer message should be aborted. 
        If the callback returns true, then the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error. 
        This is normally only used to test the validation layers themselves, so you should always return VK_FALSE.
    */
    return VK_FALSE;
}

/*
    Extensions in Vulkan, such as VkDebugUtilsMessengerEXT, play a critical role in extending the core functionality of Vulkan. Vulkan is designed to be a lean and efficient graphics and compute API, offering minimal features in its core specification to cover a wide range of hardware. Extensions allow developers to access additional features and tools that are not part of the core specification, enabling them to use new hardware capabilities, debugging tools, and other functionalities that may be vendor-specific or not universally needed.

    Understanding 

    The VkDebugUtilsMessengerEXT extension is a part of the Vulkan API that provides a way to receive callbacks from the Vulkan driver for various events, such as errors, warnings, and performance issues. It is extremely useful during development for debugging and validating applications' Vulkan usage.

    Why Extensions Need to be Loaded Dynamically

    Extensions like VkDebugUtilsMessengerEXT need to be loaded dynamically for several reasons:

    Version and Vendor Independence: Vulkan aims to be a cross-platform API, supporting a wide range of hardware and software environments. Extensions can be vendor-specific or only available in certain versions of a driver or operating system. Dynamically loading extensions allows applications to query and use these features only when they are available, making the application more portable and robust.

    Avoiding Bloat: Including all possible extensions and their functionalities directly in the Vulkan library would significantly increase its size and complexity. By requiring explicit loading of extensions, Vulkan keeps the core library lean and efficient. Developers only use and load what is necessary for their application.

    Forward Compatibility: Dynamically loading extensions ensures that applications can run on a wide variety of hardware and software configurations, including future ones. An application can check for the presence of an extension and adapt its behavior accordingly, whether the extension is available or not.

    How Extensions are Loaded

    The methods below for loading/destroying the VkDebugUtilsMessengerEXT extension is a common pattern for working with Vulkan extensions:

    vkGetInstanceProcAddr: This function retrieves the address of the extension function from the Vulkan loader or driver. By passing the instance and the name of the function, it returns a function pointer (PFN_vkCreateDebugUtilsMessengerEXT in this case) that can be cast to the appropriate type.

    Function Pointer Call: If the function pointer is not nullptr, the extension is present, and the application can call the function to use the extension's features. Otherwise, it indicates that the extension is not available (VK_ERROR_EXTENSION_NOT_PRESENT), and the application can handle this situation, usually by disabling the functionality associated with the extension or falling back to a different implementation.

    This dynamic loading mechanism allows Vulkan applications to be both forward-compatible and adaptable to the wide range of hardware and software environments where they might run.

    Vulkan extensions can be thought of as additional features or functionalities that are not part of the Vulkan core specification. They can be provided by various sources:

    GPU Vendors (Hardware): Many Vulkan extensions are specific to hardware from certain GPU vendors, such as NVIDIA, AMD, or Intel. These extensions allow developers to take advantage of unique features and capabilities of these vendors' GPUs. For example, an extension might expose a new rendering technique or optimization that is only possible on a particular vendor's hardware.

    Khronos Group (Vulkan's Governing Body): Some extensions are standardized by the Khronos Group, the consortium behind Vulkan. These extensions may add features that are useful across a wide range of hardware but were not included in the core Vulkan specification for various reasons, possibly including the need for further experimentation or because they were developed after the core specification was finalized.

    Platform-Specific: There are also extensions that are specific to certain operating systems or platforms. These extensions allow Vulkan applications to better integrate with the underlying system, offering functionalities like better window system integration or specific optimizations for a given platform.

    When an extension is available on your machine, it means that the combination of your GPU hardware, its driver, and possibly the operating system supports this extension. The Vulkan driver for your GPU implements these extensions to expose additional features beyond what is available in the Vulkan core. This is why not all extensions are available on all devices; their availability can depend on the hardware capability of the GPU, the driver version installed, and sometimes the operating system version.

    The process of dynamically querying and loading extensions, as described earlier, allows a Vulkan application to check at runtime which extensions are available on the user's machine and adapt its behavior accordingly. This ensures that the application can take advantage of these extended features when available, while still being able to run on systems where those features are not supported.
*/

/*
    Because this function is an extension function, it is not automatically loaded. 
    We have to look up its address ourselves using vkGetInstanceProcAddr via a proxy function that handles this in the background.
*/
static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) 
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else 
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

/*
    The VkDebugUtilsMessengerEXT object also needs to be cleaned up with a call to vkDestroyDebugUtilsMessengerEXT. 
    Similarly to vkCreateDebugUtilsMessengerEXT the function needs to be explicitly loaded
*/
static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, 
    VkDebugUtilsMessengerEXT debugMessenger, 
    const VkAllocationCallbacks* pAllocator
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) 
    {
        func(instance, debugMessenger, pAllocator);
    }
}

// Reads all of the bytes from the specified file and returns them in a byte array managed by a vector.
static std::vector<char> readFile(const std::string& filename)
{
    // ate: Start reading at the end of the file
    // binary: Read the file as a binary file (avoid text transformations)
    std::ifstream file { filename, std::ios::ate | std::ios::binary };

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file.");
    }

    // The advantage of starting to read at the end of the file is that we can use 
    // the read position to determine the size of the file and allocate a buffer:
    size_t fileSize = (size_t)file.tellg(); // returns the current position of the file pointer (end, which = file size)
    // the distinction between an initializer list and a single size argument matters for std::vector (be careful)
    std::vector<char> buffer(fileSize);

    file.seekg(0); // Resets the file pointer to the beginning of the file, preparing it for reading from the start.
    file.read(buffer.data(), fileSize); // reads fileSize bytes from the file into the buffer
    file.close();

    return buffer;
}

struct QueueFamilyIndices
{
    /*
        It’s possible that the queue families supporting drawing commands and the ones supporting presentation do not overlap. 
        Therefore we have to take into account that there could be a distinct presentation queue.
    */
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

/*
    There are basically three kinds of properties we need to check:

    - Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
    - Surface formats (pixel format, color space)
    - Available presentation modes
*/
struct SwapChainSupportDetails
{
    /*
        The VkSurfaceCapabilitiesKHR structure includes minImageExtent and maxImageExtent fields that define 
        the minimum and maximum dimensions of swap chain images that the device supports for the surface in question.
    */
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

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
    VkDebugUtilsMessengerEXT debugMessenger; // The debug callback is managed with a handle that needs to be explicitly created and destroyed.
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // Implicitly destroyed when the VkInstance is destroyed
    VkDevice device; // logical device
    /*
        The queues are automatically created along with the logical device, but we need a handle to interface with them.
        Device queues are implicitly cleaned up when the logical device is destroyed.
    */
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    // Each created by the implementation for the swap chain and will be automatically cleaned up once the swap chain has been destroyed.
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews; // describes how to access the image and which part of the image to access
    VkFormat swapChainImageFormat; // required for VkImageViewCreateInfo
    VkExtent2D swapChainExtent;

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

    // Instance extensions add new functionalities that are global to the application and not specific to any particular GPU (device).
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

        std::cout << "Required extensions:" << std::endl;

        for (const auto& extension : requiredExtensions)
        {
            std::cout << '\t' << extension << std::endl;

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

        /* 
            The last members determine the global validation layers to enable(for debug mode).

            The vkCreateDebugUtilsMessengerEXT call requires a valid VkInstance to have been created 
            and vkDestroyDebugUtilsMessengerEXT must be called before the VkInstance is destroyed. 
            This leaves us unable to debug any issues in the vkCreateInstance and vkDestroyInstance calls.
            However, per the extension documentation, there is a way to create a separate debug utils messenger 
            specifically for these two function calls. It requires us to pass a pointer to a 
            VkDebugUtilsMessengerCreateInfoEXT struct in the pNext extension field of VkInstanceCreateInfo.

            The debugCreateInfo variable is placed outside the if statement to ensure that it is not destroyed 
            before the vkCreateInstance call. By creating an additional debug messenger this way it will 
            automatically be used during vkCreateInstance and vkDestroyInstance and cleaned up after that.
        */
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};

        if (enableValidationLayers) 
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);

            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else 
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
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

    void createSurface()
    {
        // Under the hood, GLFW performs platform-specific operations (i.e. vkCreateWin32SurfaceKHR)
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("GLFW failed to create window surface.");
        }
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies { queueFamilyCount };
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        /*
            The VkQueueFamilyProperties struct contains some details about the queue family, including 
            the type of operations that are supported and the number of queues that can be created based 
            on that family. We need to find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT.
        */

        int i = 0;

        for (const auto& queueFamily : queueFamilies)
        {
            // bitwise AND operation between the queueFlags bitmask and the VK_QUEUE_GRAPHICS_BIT constant
            // We could prefer a physical device that supports drawing and presentation in the same queue for improved performance.
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && !indices.graphicsFamily.has_value())
            {
                indices.graphicsFamily = i;
            }

            /*
                Since the presentation is a queue-specific feature, the problem is actually about finding 
                a queue family that supports presenting to the surface we created.
            */
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport)
            {
                indices.presentFamily = i; // may or may not be the same as the graphics queue family
            }

            if (indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
    }

    // Just checking if a swap chain is available is not sufficient because it may not be compatible with our window surface.
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        // All of the support querying functions have these two as first parameters because they are the core components of the swap chain.
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount > 0)
        {
            details.formats.resize(formatCount); // Make sure the vector is resized to hold all the available formats
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount > 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    /*
        Device extensions extend the capabilities of a specific physical device (GPU). 
        Vulkan treats each GPU as a separate device, and device extensions allow you to use 
        additional features or optimizations offered by that GPU beyond what the core Vulkan specification supports.
        Another example of a device extension is VK_NV_ray_tracing, which adds support for hardware-accelerated ray tracing on NVIDIA GPUs.
    */
    bool checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions { extensionCount };
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        //std::cout << "Available device extensions:" << std::endl;

        for (const auto& extension : availableExtensions)
        {
            //std::cout << '\t' << extension.extensionName << std::endl;

            requiredExtensions.erase(extension.extensionName);
        }

        /*
            Verify that the graphics card is capable of creating a swap chain. It should be noted that the availability of a
            presentation queue, as we checked in the previous chapter, implies that the swap chain extension must be supported.
            However, it’s still good to be explicit about things, and the extension does have to be explicitly enabled.
        */
        return requiredExtensions.empty();
    }

    /*
        We need to evaluate each GPU and check if they are suitable for the operations we want to perform, 
        because not all graphics cards are created equal.
    */
    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        // Basic device properties like the name, type, and supported Vulkan version can be queried using vkGetPhysicalDeviceProperties:
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        /*
            typedef enum VkPhysicalDeviceType {
                VK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
                VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
                VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
                VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
                VK_PHYSICAL_DEVICE_TYPE_CPU = 4,
            } VkPhysicalDeviceType;
        */
        std::cout << deviceProperties.deviceName << " | " << deviceProperties.deviceType << std::endl;

        // Querying support for optional features like texture compression, 64 bit floats, multi viewport rendering (useful for VR), etc.
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        auto vr = deviceFeatures.multiViewport ? "VR supported" : "VR unsupported";

        std::cout << vr << std::endl;

        QueueFamilyIndices indices = findQueueFamilies(device);

        bool swapChainAdequate = false; // true implies device extensions are supported

        // Only try to query for swap chain support after verifying that the extension is available.
        if (checkDeviceExtensionSupport(device))
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            // Checks whether the swap chain is compatible with the window surface
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }
        
        // Discrete GPUs have a significant performance advantage.
        return indices.isComplete() && swapChainAdequate && deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    }

    /*
        Instead of just checking if a device is suitable or not and going with the first one, you could also give each 
        device a score and pick the highest one. That way you could favor a dedicated graphics card by giving it a 
        higher score, but fall back to an integrated GPU if that’s the only available one.
    */
    void selectPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) 
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support.");
        }

        std::vector<VkPhysicalDevice> devices { deviceCount };
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        std::cout << "Found " << deviceCount << " GPU(s) with Vulkan support." << std::endl;

        // We can select any number of graphics cards and use them simultaneously; using only 1 in this tutorial.
        for (const auto& device : devices)
        {
            if (isDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU.");
        }
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        // Using a set in case it's the same queue family for both capabilities
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0f;

        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo {};

            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            /*
                We don’t really need more than one queue per family because we can create all of the command buffers
                on multiple threads and then submit them all at once on the main thread with a single low-overhead call.
            */
            queueCreateInfo.queueCount = 1;
            /*
                Vulkan lets us assign priorities to queues to influence the scheduling of command buffer execution
                using floating point numbers between 0.0 and 1.0. This is required even if there is only a single queue.
            */
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures {}; // nothing special for now

        VkDeviceCreateInfo createInfo {};

        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        /* 
            The remainder of the information is similar to the VkInstanceCreateInfo struct and requires us to
            specify extensions and validation layers. The difference is that these are device specific this time.
            An example of a device specific extension is VK_KHR_swapchain, which allows us to present rendered images 
            from that device to windows. It is possible that there are Vulkan devices in the system that lack this ability, 
            for example because they only support compute operations.
        */
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        /*
            Previous implementations of Vulkan made a distinction between instance and device specific validation layers, but 
            this is no longer the case. The enabledLayerCount and ppEnabledLayerNames fields of VkDeviceCreateInfo are ignored 
            by up-to-date implementations. However, it is still a good idea to set them anyway to be compatible with older implementations.
        */
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        // Similarly to the instance creation function, this call can return errors based on enabling 
        // non-existent extensions or specifying the desired usage of unsupported features.
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device.");
        }
        
        // Queue index 0 for both because each queue family only has a single queue.
        // In case the queue families are the same, the two handles will most likely have the same value.
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    /*
        Each VkSurfaceFormatKHR entry contains a format and a colorSpace member. The format member specifies the color 
        channels and types. For example, VK_FORMAT_B8G8R8A8_SRGB means that we store the B, G, R and alpha channels in 
        that order with an 8 bit unsigned integer for a total of 32 bits per pixel. The colorSpace member indicates if 
        the SRGB color space is supported or not using the VK_COLOR_SPACE_SRGB_NONLINEAR_KHR flag.
    */
    VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector< VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            /*
                For the color space we’ll use SRGB if it is available, because it results in more accurate perceived colors. 
                It is also the standard color space for images, like the textures we’ll use later on. 
                Because of that we should also use an SRGB color format, of which one of the most common ones is VK_FORMAT_B8G8R8A8_SRGB.
            */
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        // We could start rank the available formats based on how "good" they are, 
        // but in most cases it’s okay to just settle with the first format that is specified.
        return availableFormats[0];
    }

    /*
        The presentation mode is the most important setting for the swap chain because it represents the actual conditions for showing images 
        to the screen. There are four possible modes available in Vulkan:

        VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away, which may result in tearing.

        VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the queue when the display is refreshed and the program inserts rendered images at the back of the queue. If the queue is full then the program has to wait. This is most similar to vertical sync as found in modern games. The moment that the display is refreshed is known as "vertical blank".

        VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late and the queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives. This may result in visible tearing.

        VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application when the queue is full, the images that are already queued are simply replaced with the newer ones. This mode can be used to render frames as fast as possible while still avoiding tearing, resulting in fewer latency issues than standard vertical sync. This is commonly known as "triple buffering", although the existence of three buffers alone does not necessarily mean that the framerate is unlocked.
    */
    VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            /*
                VK_PRESENT_MODE_MAILBOX_KHR (triple buffering) is a nice trade-off if energy usage is not a concern. 
                It allows us to avoid tearing while still maintaining a fairly low latency by rendering new images 
                that are as up-to-date as possible right until the vertical blank. 
                On mobile devices, where energy usage is more important, use VK_PRESENT_MODE_FIFO_KHR instead.
            */
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        // VK_PRESENT_MODE_FIFO_KHR (v-sync) mode is guaranteed to be available
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    /*
        The swap extent is the resolution of the swap chain images and it’s almost always exactly equal to the resolution of the window 
        that we’re drawing to in pixels. The range of the possible resolutions is defined in the VkSurfaceCapabilitiesKHR structure. 
        Vulkan tells us to match the resolution of the window by setting the width and height in the currentExtent member. 
        However, some window managers do allow us to differ here and this is indicated by setting the width and height in currentExtent 
        to a special value: the maximum value of uint32_t. In that case we’ll pick the resolution that best matches the window within 
        the minImageExtent and maxImageExtent bounds. But we must specify the resolution in the correct unit.
    */
    VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        /*
            GLFW uses two units when measuring sizes: pixels and screen coordinates. For example, the resolution {WIDTH, HEIGHT} 
            that we specified when creating the window is measured in screen coordinates. But Vulkan works with pixels, 
            so the swap chain extent must be specified in pixels as well. Unfortunately, if we are using a high DPI display 
            (like Apple’s Retina display), screen coordinates don’t correspond to pixels. Instead, due to the higher pixel density, 
            the resolution of the window in pixels will be larger than the resolution in screen coordinates. So if Vulkan doesn’t 
            fix the swap extent for us, we can’t just use the original {WIDTH, HEIGHT}. Instead, we must use glfwGetFramebufferSize 
            to query the resolution of the window in pixels before matching it against the minimum and maximum image extent.
        */
        if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
        {
            // Required by Vulkan since currentExtent width and height do not match the window resolution. A value of 
            // std::numeric_limits<uint32_t>::max() indicates that the surface size will be determined by the extent of the swap chain images.
            int width, height;
            /*
                Retrieves the resolution of the window in pixels. GLFW, or any windowing system, may allow the creation of windows with 
                dimensions outside the bounds supported by the Vulkan implementation on the device for a given surface (i.e. a window might 
                be resized by the user or the system to dimensions larger or smaller than what the device can handle for rendering).
            */
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent
            {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            /*
                The clamp function is used here to bound the values of width and height between the allowed minimum and maximum extents 
                that are supported by the implementation. Clamping is a way to gracefully handle cases where the window size doesn't match 
                the GPU's supported sizes exactly, allowing for flexible window management while still adhering to the device's constraints.
            */
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
        else
        {
            // currentExtent width and height already match the window resolution
            return capabilities.currentExtent;
        }
    }

    /*
        Find the settings for the best possible swap chain. If the swapChainAdequate conditions 
        were met then the support is definitely sufficient, but there may still be many 
        different modes of varying optimality. There are three types of settings to determine:

        1. Surface format (color depth)
        2. Presentation mode (conditions for "swapping" images to the screen)
        3. Swap extent (resolution of images in swap chain)

        For each of these settings we’ll have an ideal value in mind that we’ll go with if 
        it’s available and otherwise we’ll create some logic to find the next best thing.
    */
    void createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = selectSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = selectSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = selectSwapExtent(swapChainSupport.capabilities);

        /*
            Decide how many images we would like to have in the swap chain. Selecting the minimum means that we may sometimes have 
            to wait on the driver to complete internal operations before we can acquire another image to render to. Therefore it is 
            recommended to request at least one more image than the minimum. We should also make sure to not exceed the maximum 
            number of images, where 0 is a special value that means that there is no maximum.
        */
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo {};

        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        /*
            Single Layer: For the majority of applications, each image in the swap chain will consist of a single layer. 
            This is the case for standard 2D applications, where you're rendering a single image to be displayed on the screen. 
            Therefore, imageArrayLayers is always set to 1.

            Stereoscopic 3D Applications: In the context of stereoscopic 3D applications, the application needs to render a separate image 
            for each eye to create a 3D effect. This requires two layers per image in the swap chain—one for the left eye and one for the 
            right eye. For such applications, imageArrayLayers would be set to 2 to accommodate both layers within each swap chain image.
            Vulkan supports not only standard 2D rendering but also more complex scenarios like stereoscopic 3D, virtual reality (VR), 
            and augmented reality (AR), where multiple views (layers) are required to be rendered and presented simultaneously.
        */
        createInfo.imageArrayLayers = 1;
        /*
            We are going to render directly to them, which means that they’re used as color attachment. It is also possible to render 
            images to a separate image first to perform operations like post-processing. In that case you may use a value like 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer the rendered image to a swap chain image.
        */
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) 
        {
            // Images can be used across multiple queue families without explicit ownership transfers.
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else 
        {
            /*
                An image is owned by one queue family at a time and ownership must be explicitly transferred 
                before using it in another queue family. This option offers the best performance.
                The graphics queue family and presentation queue family are the same on most GPUs.
            */
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        /*
            We can specify that a certain transform should be applied to images in the swap chain if it is supported 
            (supportedTransforms in capabilities), like a 90 degree clockwise rotation or horizontal flip. 
            To specify that you do not want any transformation, simply specify the current transformation.
        */
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        /*
            specifies if the alpha channel should be used for blending with other windows in the window system.
            We almost always want to simply ignore the alpha channel, hence VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
        */ 
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        /*
            If clipped is set to VK_TRUE then that means that we don’t care about the color of pixels that are obscured, 
            for example because another window is in front of them. Unless you really need to be able to read these 
            pixels back and get predictable results, you’ll get the best performance by enabling clipping.
        */
        createInfo.clipped = VK_TRUE;
        /*
            With Vulkan it’s possible that your swap chain becomes invalid or unoptimized while your application is running, 
            for example because the window was resized. In that case the swap chain actually needs to be recreated from scratch and a 
            reference to the old one must be specified in this field. For now we assume that we will only ever create one swap chain.
        */
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swap chain.");
        }

        // We now have a set of images that can be drawn onto and can be presented to the window.

        // We will reference swap chain images during rendering operations later
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        // These will be needed later
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    // Creates a basic image view for every image in the swap chain so that we can use them as color targets.
    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo {};

            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            // The viewType and format fields specify how the image data should be interpreted. 
            // The viewType parameter allows you to treat images as 1D textures, 2D textures, 3D textures and cube maps.
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            /*
                The components field allows US to swizzle the color channels around. 
                For example, you can map all of the channels to the red channel for a monochrome texture. 
                We can also map constant values of 0 and 1 to a channel. In our case we’ll stick to the default mapping.
            */
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            /*
                The subresourceRange field describes what is the purpose of the image and which part of the image should be accessed. 
                Our images will be used as color targets without any mipmapping levels or multiple layers.
                If we were working on a stereographic 3D application, then we would create a swap chain with multiple layers. We could then 
                create multiple image views for each image representing the views for the left and right eyes by accessing different layers.
            */
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create image view.");
            }
        }
    }

    void createGraphicsPipeline()
    {
        auto vertexShaderCode = readFile("shaders/vert.spv");
        auto fragmentShaderCode = readFile("shaders/frag.spv");

        std::cout << vertexShaderCode.size() << std::endl;
        std::cout << fragmentShaderCode.size() << std::endl;
    }

    void initVulkan() 
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        selectPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback; // pointer to the callback function
        createInfo.pUserData = nullptr; // Optional, such as a pointer to the HelloTriangleApplication class
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers) return;

        // Fill in a struct with details about the messenger and its callback.
        VkDebugUtilsMessengerCreateInfoEXT createInfo;

        populateDebugMessengerCreateInfo(createInfo);

        /*
            Since the debug messenger is specific to our Vulkan instance and its layers, it needs to be explicitly specified as first argument. 
            You will also see this pattern with other child objects later on.
        */
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) 
        {
            throw std::runtime_error("Failed to set up debug messenger.");
        }
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
        // Note the reverse order of deletions based on dependencies

        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr); // Unlike images, the image views were explicitly.
        }
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        // Logical devices don’t interact directly with instances, which is why instance is not included as a parameter.
        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) 
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
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
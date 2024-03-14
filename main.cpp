#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> // GLFW will include its own definitions and automatically load <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // provides the EXIT_SUCCESS and EXIT_FAILURE macros
#include <vector>
#include <algorithm> //std::find_if; C++20 has the more concise std::ranges::find in <ranges>
#include <sstream> // std::ostringstream; C++20 has std::format in <format>
#include <cstring> // strcmp compares two strings character by character. If the strings are equal, the function returns 0.
#include <optional> // C++17

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

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value();
    }
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
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // Implicitly destroyed when the VkInstance is destroyed.

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
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
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
            The support for optional features like texture compression, 64 bit floats and 
            multi viewport rendering (useful for VR) can be queried using vkGetPhysicalDeviceFeatures:
        */
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        auto vr = deviceFeatures.multiViewport ? "VR supported" : "VR unsupported";

        std::cout << vr << std::endl;

        QueueFamilyIndices indices = findQueueFamilies(device);
        
        return indices.isComplete();
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

    void initVulkan() 
    {
        createInstance();
        setupDebugMessenger();
        selectPhysicalDevice();
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
        if (enableValidationLayers) 
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
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
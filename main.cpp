#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> // GLFW will include its own definitions and automatically load <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // provides the EXIT_SUCCESS and EXIT_FAILURE macros
#include <vector>
#include <algorithm> //std::find_if; C++20 has the more concise std::ranges::find in <ranges>
#include <sstream> // std::ostringstream; C++20 has std::format in <format>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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
    std::vector<std::string> glfwRequiredExtensions;
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

    void checkVulkanExtensions()
    {
        // 1. To allocate an array to hold the extension details, we first need to know how many there are:
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        // 2. Allocate an array to hold the extension details. Each VkExtensionProperties struct contains the name and version of an extension.
        std::vector<VkExtensionProperties> extensions { extensionCount };

        // 3. Query the extension details:
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "Available Vulkan extensions:" << std::endl;

        for (const auto& extension : extensions) 
        {
            std::cout << '\t' << extension.extensionName << std::endl;
        }

        for (const auto& glfwExtension : glfwRequiredExtensions)
        {
            auto iterator = std::find_if(
                extensions.begin(), 
                extensions.end(), 
                [&glfwExtension](const VkExtensionProperties& vkExtension) 
                {
                    return vkExtension.extensionName == glfwExtension;
                }
            );

            if (iterator == extensions.end())
            {
                std::ostringstream output;
                output << "GLFW extension " << glfwExtension << " is not supported by Vulkan.";

                throw std::runtime_error(output.str());
            }
        }

        // check if all of the extensions returned by glfwGetRequiredInstanceExtensions are included in the supported extensions list
    }

    /*
        The instance is the connection between your application and the Vulkan library and 
        creating it involves specifying some details about your application to the driver.
    */
    void createInstance()
    {
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
        uint32_t glfwExtensionCount = 0;
        /*
            Returns an array of names of Vulkan instance extensions required by GLFW for creating Vulkan surfaces for GLFW windows.
            If successful, the list will always contain VK_KHR_surface, so if you don't require any
            additional extensions you can pass this list directly to the VkInstanceCreateInfo struct.
        */
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        glfwRequiredExtensions = { glfwExtensions, glfwExtensions + sizeof(glfwExtensions) / sizeof(char*) };

        VkInstanceCreateInfo createInfo {};
         
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        /*
            Specify the desired global extensions; since Vulkan is a platform agnostic API, we need an extension to interface with the window 
            system. GLFW has a handy built-in function that returns the extension(s) it needs to do that - which we can pass to the struct.
        */
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        // The last two members determine the global validation layers to enable.
        createInfo.enabledLayerCount = 0;

        /*
            If you look at the vkCreateInstance documentation then you’ll see that one of the possible error codes is 
            VK_ERROR_EXTENSION_NOT_PRESENT. We could simply specify the extensions we require and terminate if that 
            error code comes back. That makes sense for essential extensions like the window system interface, 
            but what if we want to check for optional functionality?
        */
        checkVulkanExtensions();

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

/*
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>


int main() 
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
*/
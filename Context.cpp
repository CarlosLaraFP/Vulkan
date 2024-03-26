#include "Context.hpp"

#include <stdexcept>

Context::Context(int width, int height, void* app)
{
    if (!glfwInit())
    {
        throw std::runtime_error("GLFW could not be initialized.");
    }

    // Because GLFW was originally designed to create an OpenGL context, we need to tell it to not create an OpenGL context.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Handling resized windows takes special care
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // The 4th parameter allows you to optionally specify a monitor to open the window on and the last parameter is only relevant to OpenGL.
    m_Window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

    if (m_Window == nullptr)
    {
        glfwTerminate();

        throw std::runtime_error("Window could not be created.");
    }

    glfwSetWindowUserPointer(m_Window, this); // this instance can now be retrieved from within the callback
    glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
}

Context::~Context()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

/*
    The reason that we’re creating a static function as a callback is because GLFW doesn't know how to
    properly call a member function with the right this pointer to our HelloTriangleApplication instance.
    However, we do get a reference to the GLFWwindow in the callback and there is another GLFW function
    that allows you to store an arbitrary pointer inside of it: glfwSetWindowUserPointer
*/
void Context::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto context = reinterpret_cast<Context*>(glfwGetWindowUserPointer(window));

    context->framebufferResized = true;
}

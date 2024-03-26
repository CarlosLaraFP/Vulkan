#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> // GLFW will include its own definitions and automatically load <vulkan/vulkan.h>

class Context
{
public:
	Context(int width, int height, void* app);
	~Context();

	inline GLFWwindow* getWindow() const { return m_Window; }

	bool framebufferResized = false; // window resized

private:
	GLFWwindow* m_Window;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};
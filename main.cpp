#include "VulkanApp.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

int main() 
{
    VulkanApp app { WIDTH, HEIGHT };

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
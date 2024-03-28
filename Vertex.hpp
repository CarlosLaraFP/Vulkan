#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

// hash functions for the GLM types need to be included
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <array>

// Interleaving vertex attributes
struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texture;

    /*
        Tells Vulkan how to pass this data format to the vertex shader once it’s been uploaded into GPU memory.
        A vertex binding describes at which rate to load data from memory throughout the vertices. It specifies the number
        of bytes between data entries and whether to move to the next data entry after each vertex or after each instance.
    */
    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        // All of our per-vertex data is packed together in one array, so we’re only going to have one binding.
        // The binding parameter specifies the index of the binding in the array of bindings
        bindingDescription.binding = 0;
        // specifies the number of bytes from one entry to the next
        bindingDescription.stride = sizeof(Vertex);
        /*
            Specifies whether vertex attribute addressing is a function of the vertex index or of the instance index.

            VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
            VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance

            We’re not going to use instanced rendering, so we’ll stick to per-vertex data.
        */
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    /*
        Describes how to handle vertex input.
        Describes how to extract a vertex attribute from a chunk of vertex data originating from a binding description.
        The format parameter describes the type of data for the attribute. They are specified using the same enumeration as color formats.
        The format parameter implicitly defines the byte size of attribute data and
        the offset parameter specifies the number of bytes since the start of the per-vertex data to read from.
    */
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        // This array type is basically a vector with a known size at compile time.
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        // layout(location = 0) in vertex shader
        attributeDescriptions[0].location = 0;
        // the binding number which this attribute takes its data from
        attributeDescriptions[0].binding = 0;
        // the size and type of the vertex attribute data
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        // byte offset of this attribute relative to the start of an element in the vertex input binding
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        // layout(location = 1) in vertex shader
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // only integers can be unsigned
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        // layout(location = 2) in vertex shader
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texture);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const
    {
        return position == other.position && color == other.color && texture == other.texture;
    }
};

/*
    A hash function for Vertex is implemented by specifying a template specialization for std::hash<T>.
    Hash functions are a complex topic, but cppreference.com recommends the following approach combining
    the fields of a struct to create a decent quality hash function:
*/
namespace std
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.position) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texture) << 1);
        }
    };
}

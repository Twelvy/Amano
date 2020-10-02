#pragma once

#include "Vertex.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace Amano {

class Device;

class Mesh
{
public:
	Mesh(Device* device);
	~Mesh();

	// Loads the model at the given file path
	// Creates all the necessary buffers
	bool create(const std::string& filename);

	VkBuffer getVertexBuffer() { return m_vertexBuffer; }
	VkBuffer getIndexBuffer() { return m_indexBuffer; }
	uint32_t getVertexCount() { return static_cast<uint32_t>(m_vertices.size()); }
	uint32_t getIndexCount() { return static_cast<uint32_t>(m_indices.size()); }

private:
	bool load(const std::string& filename);
	bool createVertexBuffer();
	bool createIndexBuffer();

private:
	Device* m_device;
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;
};

}

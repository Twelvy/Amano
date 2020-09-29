#pragma once

#include "Config.h"

#include <string>
#include <vector>

namespace Amano {

class Device;

class Model
{
public:
	Model(Device* device);
	~Model();

	bool create(const std::string& filename);

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
#include "Mesh.h"
#include "Device.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>

namespace std {
	// hash method for Vertex
	// This is necessary when using Vertex as a key in a map
	// TODO: revise this method
	template<> struct hash<Amano::Vertex> {
		size_t operator()(Amano::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1);
		}
	};
}

namespace {
	VkDeviceAddress getBufferAddress(VkDevice device, VkBuffer buffer) {
		VkBufferDeviceAddressInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		info.pNext = nullptr;
		info.buffer = buffer;

		return vkGetBufferDeviceAddress(device, &info);
	}
}

namespace Amano {

Mesh::Mesh(Device* device)
	: m_device{ device }
	, m_vertices()
	, m_indices()
	, m_vertexBuffer{ VK_NULL_HANDLE }
	, m_vertexBufferMemory{ VK_NULL_HANDLE }
	, m_indexBuffer{ VK_NULL_HANDLE }
	, m_indexBufferMemory{ VK_NULL_HANDLE }
{
}

Mesh::~Mesh() {
	m_device->destroyBuffer(m_vertexBuffer);
	m_device->destroyBuffer(m_indexBuffer);
	m_device->freeDeviceMemory(m_vertexBufferMemory);
	m_device->freeDeviceMemory(m_indexBufferMemory);
}

bool Mesh::create(const std::string& filename) {
	return load(filename)
		&& createVertexBuffer()
		&& createIndexBuffer();
}

VkDeviceAddress Mesh::getVertexBufferAddress() const {
	return getBufferAddress(m_device->handle(), m_vertexBuffer);
}

VkDeviceAddress Mesh::getIndexBufferAddress() const {
	return getBufferAddress(m_device->handle(), m_indexBuffer);
}

bool Mesh::load(const std::string& filename) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
		std::cerr << (warn + err) << std::endl;
		return false;
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};


			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // fixing the gl coordinates
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
				m_vertices.push_back(vertex);
			}

			m_indices.push_back(uniqueVertices[vertex]);
		}
	}

	return true;
}

bool Mesh::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	if (!m_device->createBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory))
		return false;

	void* data;
	vkMapMemory(m_device->handle(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_device->handle(), stagingBufferMemory);

	if (!m_device->createBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vertexBuffer,
		m_vertexBufferMemory))
		return false;

	m_device->copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize, QueueType::eGraphics);

	m_device->destroyBuffer(stagingBuffer);
	m_device->freeDeviceMemory(stagingBufferMemory);

	return true;
}

bool Mesh::createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	if (!m_device->createBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory))
		return false;

	void* data;
	vkMapMemory(m_device->handle(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_indices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_device->handle(), stagingBufferMemory);

	if (!m_device->createBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_indexBuffer,
		m_indexBufferMemory))
		return false;

	m_device->copyBuffer(stagingBuffer, m_indexBuffer, bufferSize, QueueType::eGraphics);

	m_device->destroyBuffer(stagingBuffer);
	m_device->freeDeviceMemory(stagingBufferMemory);

	return true;
}

}

#include "Model.h"
#include "Device.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>

namespace Amano {

Model::Model(Device* device)
	: m_device{ device }
	, m_vertices()
	, m_indices()
	, m_vertexBuffer{ VK_NULL_HANDLE }
	, m_vertexBufferMemory{ VK_NULL_HANDLE }
	, m_indexBuffer{ VK_NULL_HANDLE }
	, m_indexBufferMemory{ VK_NULL_HANDLE }
{
}

Model::~Model() {
	m_device->destroyBuffer(m_vertexBuffer);
	m_device->destroyBuffer(m_indexBuffer);
	m_device->freeDeviceMemory(m_vertexBufferMemory);
	m_device->freeDeviceMemory(m_indexBufferMemory);
}

bool Model::create(const std::string& filename) {
	return load(filename)
		&& createVertexBuffer()
		&& createIndexBuffer();
}

bool Model::load(const std::string& filename) {
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

bool Model::createVertexBuffer() {
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

bool Model::createIndexBuffer() {
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

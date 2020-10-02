#pragma once

#include "Device.h"

namespace Amano {

template<typename DESC>
class UniformBuffer
{
public:
	UniformBuffer(Device* device)
		: m_device{ device }
		, m_buffer{ VK_NULL_HANDLE }
		, m_bufferMemory{ VK_NULL_HANDLE }
	{
		VkDeviceSize bufferSize = sizeof(DESC);
		m_device->createBufferAndMemory(bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_buffer,
			m_bufferMemory);
	}

	~UniformBuffer() {
		m_device->destroyBuffer(m_buffer);
		m_device->freeDeviceMemory(m_bufferMemory);
	}

	VkBuffer getBuffer() { return m_buffer; }
	size_t getSize() { return sizeof(DESC); }

	void update(DESC& desc) {
		void* data;
		vkMapMemory(m_device->handle(), m_bufferMemory, 0, sizeof(DESC), 0, &data);
		memcpy(data, &desc, sizeof(DESC));
		vkUnmapMemory(m_device->handle(), m_bufferMemory);
	}

private:
	Device* m_device;
	VkBuffer m_buffer;
	VkDeviceMemory m_bufferMemory;
};

}

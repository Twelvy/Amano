#pragma once

#include <vulkan/vulkan.h>

namespace Amano {

class Device;

// Queue abstraction
// Used to generate and submit command buffers
class Queue
{
public:
	Queue(Device* device, uint32_t familyIndex);
	~Queue();

	uint32_t familyIndex() const { return m_familyIndex; }
	VkQueue handle() { return m_queue; }

	// Generates a command buffer that will be deleted once it has been submitted
	// Use endSingleTimeCommands with the generated command buffer
	// There is no need to call freeCommandBuffer afterwards
	VkCommandBuffer beginSingleTimeCommands();

	// Ends and submits the command buffer.
	// this call is blocking until the command is executed.
	// After this call, the passed command buffer isn't usable anymore
	// There is no need to call freeCommandBuffer afterwards
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	// Generates a command buffer that will be used multiple times
	// Use endCommands with the generated command buffer
	VkCommandBuffer beginCommands();

	// Ends the command buffer but doesn't execute it
	bool endCommands(VkCommandBuffer commandBuffer);

	// Free the command buffer.
	// After this call, the command buffer in't usable anymore
	void freeCommandBuffer(VkCommandBuffer commandBuffer);

	// Submits the whole queue
	bool submit(VkSubmitInfo* submitInfo, VkFence fence);

private:
	Device* m_device;
	uint32_t m_familyIndex;
	VkQueue m_queue;
	VkCommandPool m_commandPool;
};

}

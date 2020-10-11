#pragma once

#include "../Device.h"

#include <vector>

namespace Amano {

class Pass {
public:
	Pass(Device* device, VkPipelineStageFlags pipelineStage);
	virtual ~Pass();

	VkSemaphore signalSemaphore() const { return m_signalSemaphore; }
	VkPipelineStageFlags pipelineStage() const { return m_pipelineStage; }

	void addWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags pipelineStage);
	
protected:
	Device* m_device;
	VkSemaphore m_signalSemaphore;
	VkPipelineStageFlags m_pipelineStage;
	std::vector<VkSemaphore> m_waitSemaphores;
	std::vector<VkPipelineStageFlags> m_waitPipelineStages;
};

}

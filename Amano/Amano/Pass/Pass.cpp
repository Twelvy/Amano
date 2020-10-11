#include "Pass.h"

#include <iostream>

namespace Amano {

Pass::Pass(Device* device, VkPipelineStageFlags pipelineStage)
	: m_device{ device }
	, m_signalSemaphore{ VK_NULL_HANDLE }
	, m_pipelineStage{ pipelineStage }
	, m_waitSemaphores()
	, m_waitPipelineStages()
{
	// create signal semaphore
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_signalSemaphore) != VK_SUCCESS) {
		std::cerr << "failed to create semaphores for a frame!" << std::endl;
	}
}

Pass::~Pass() {
	vkDestroySemaphore(m_device->handle(), m_signalSemaphore, nullptr);
}

void Pass::addWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags pipelineStage) {
	m_waitSemaphores.push_back(semaphore);
	m_waitPipelineStages.push_back(pipelineStage);
}

}

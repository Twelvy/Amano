#pragma once

#include "../Device.h"

namespace Amano {

class SamplerBuilder {
public:
	SamplerBuilder();

	SamplerBuilder& setMaxLoad(float maxLoad);

	VkSampler build(Device& device);

private:
	VkSamplerCreateInfo m_samplerInfo;
};

}

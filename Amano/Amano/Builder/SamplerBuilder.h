#pragma once

#include "../Device.h"

namespace Amano {

// Default values:
//   - min and mag filter are LINEAR
class SamplerBuilder {
public:
	SamplerBuilder();

	SamplerBuilder& setMaxLod(float maxLod);
	SamplerBuilder& setFilter(VkFilter magFilter, VkFilter minFilter);

	VkSampler build(Device& device);

private:
	VkSamplerCreateInfo m_samplerInfo;
};

}

#pragma once

#include "../Config.h"
#include "../Device.h"

namespace Amano {

struct ShaderGroupBindingTable {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
	uint32_t groupSize = 0;

	void clean(Device* device);
};

struct ShaderBindingTables {
	ShaderGroupBindingTable rgenShaderBindingTable;
	ShaderGroupBindingTable missShaderBindingTable;
	ShaderGroupBindingTable chitShaderBindingTable;
	//ShaderGroupBindingTable ahitShaderBindingTable;
	void clean(Device* device);
};

class ShaderBindingTableBuilder
{
public:
	enum class Stage {
		eRayGen,
		eMiss,
		eClosestHit,
		eAnyHit
	};

public:
	ShaderBindingTableBuilder(Device* device, VkPipeline pipeline);

	ShaderBindingTableBuilder& addShader(Stage stage, uint32_t index);

	ShaderBindingTables build();

private:
	void setupShaderBindingTable(ShaderGroupBindingTable& table, uint32_t index);

private:
	Device* m_device;
	VkPipeline m_pipeline;
	ShaderBindingTables m_bindingTables;
};

}

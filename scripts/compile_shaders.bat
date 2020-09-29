SET VULKAN_SDK=C:/VulkanSDK/1.2.135.0
SET VULKAN_SDK_SPV_COMPILER=%VULKAN_SDK%/Bin/glslc.exe

%VULKAN_SDK_SPV_COMPILER% ../shaders/gbuffer.vert -o ../compiled_shaders/gbufferv.spv
%VULKAN_SDK_SPV_COMPILER% ../shaders/gbuffer.frag -o ../compiled_shaders/gbufferf.spv

pause
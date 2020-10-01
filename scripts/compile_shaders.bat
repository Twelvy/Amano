SET VULKAN_SDK=C:/VulkanSDK/1.2.135.0
SET VULKAN_SDK_SPV_COMPILER=%VULKAN_SDK%/Bin/glslc.exe
SET VULKAN_SDK_GLSLLANG=%VULKAN_SDK%/Bin/glslangValidator.exe

%VULKAN_SDK_SPV_COMPILER% ../shaders/gbuffer.vert -o ../compiled_shaders/gbufferv.spv
%VULKAN_SDK_SPV_COMPILER% ../shaders/gbuffer.frag -o ../compiled_shaders/gbufferf.spv

%VULKAN_SDK_GLSLLANG% -V ../shaders/raygen.rgen      -o ../compiled_shaders/raygen.spv
%VULKAN_SDK_GLSLLANG% -V ../shaders/closesthit.rchit -o ../compiled_shaders/closesthit.spv
%VULKAN_SDK_GLSLLANG% -V ../shaders/miss.rmiss       -o ../compiled_shaders/miss.spv

%VULKAN_SDK_GLSLLANG% -V ../shaders/shadow_raygen.rgen      -o ../compiled_shaders/shadow_raygen.spv
%VULKAN_SDK_GLSLLANG% -V ../shaders/shadow_closesthit.rchit -o ../compiled_shaders/shadow_closesthit.spv
%VULKAN_SDK_GLSLLANG% -V ../shaders/shadow_miss.rmiss       -o ../compiled_shaders/shadow_miss.spv

pause
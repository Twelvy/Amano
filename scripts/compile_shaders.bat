@echo off

SET VULKAN_SDK=C:/VulkanSDK/1.2.135.0
SET VULKAN_SDK_SPV_COMPILER=%VULKAN_SDK%/Bin/glslc.exe
SET OUTPUT_DIR=../compiled_shaders

cd ../shaders/

FOR /R %%i IN (*) DO (
    IF %%~xi==.vert (
        echo compiling vertex shader %%i
        %VULKAN_SDK_SPV_COMPILER% %%i -o %OUTPUT_DIR%/%%~niv.spv
    ) ELSE IF %%~xi==.frag (
        echo compiling fragment shader %%i
        %VULKAN_SDK_SPV_COMPILER% %%i -o %OUTPUT_DIR%/%%~nif.spv
    ) ELSE IF %%~xi==.comp (
        echo compiling compute shader %%i
        %VULKAN_SDK_SPV_COMPILER% %%i -o %OUTPUT_DIR%/%%~ni.spv
    ) ELSE IF %%~xi==.rgen (
        echo compiling raygen shader %%i
        %VULKAN_SDK_SPV_COMPILER% %%i -o %OUTPUT_DIR%/%%~ni.spv
    ) ELSE IF %%~xi==.rchit (
        echo compiling closest hit shader %%i
        %VULKAN_SDK_SPV_COMPILER% %%i -o %OUTPUT_DIR%/%%~ni.spv
    ) ELSE IF %%~xi==.rahit (
        echo compiling any hit shader %%i
        %VULKAN_SDK_SPV_COMPILER% %%i -o %OUTPUT_DIR%/%%~ni.spv
    ) ELSE IF %%~xi==.rmiss (
        echo compiling miss shader %%i
        %VULKAN_SDK_SPV_COMPILER% %%i -o %OUTPUT_DIR%/%%~ni.spv
    ) ELSE (
        echo shader %%i is from an unknown/unsupported type
    )
)

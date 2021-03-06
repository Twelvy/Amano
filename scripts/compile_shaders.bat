@echo off
setlocal enabledelayedexpansion

SET VULKAN_SDK=C:/VulkanSDK/1.2.170.0
SET VULKAN_SDK_SPV_COMPILER=%VULKAN_SDK%/Bin/glslc.exe
SET OUTPUT_DIR=../compiled_shaders

cd ../shaders/

REM mkdir if necessary

FOR /R %%i IN (*) DO (
    SET SUPPORTED_SHADER=1
    IF %%~xi==.vert (
        echo compiling vertex shader %%i
    ) ELSE IF %%~xi==.frag (
        echo compiling fragment shader %%i
    ) ELSE IF %%~xi==.comp (
        echo compiling compute shader %%i
    ) ELSE IF %%~xi==.rgen (
        echo compiling raygen shader %%i
    ) ELSE IF %%~xi==.rchit (
        echo compiling closest hit shader %%i
    ) ELSE IF %%~xi==.rahit (
        echo compiling any hit shader %%i
    ) ELSE IF %%~xi==.rmiss (
        echo compiling miss shader %%i
    ) ELSE IF %%~xi==.glsl (
        REM safely ignore this one
        SET SUPPORTED_SHADER=0
    ) ELSE (
        echo shader %%i is from an unknown/unsupported type
        SET SUPPORTED_SHADER=0
    )
    IF !SUPPORTED_SHADER! == 1 (
        %VULKAN_SDK_SPV_COMPILER% --target-env=vulkan1.2 %%i -o %OUTPUT_DIR%/%%~nxi.spv
    )
)

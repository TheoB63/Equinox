@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

if "%VULKAN_SDK%"=="" (
    echo [ERROR] VULKAN_SDK is not set. Install the Vulkan SDK first.
    exit /b 1
)

set GLSLC="%VULKAN_SDK%\Bin\glslc.exe"
if not exist spv mkdir spv
if not exist spv\gl mkdir spv\gl

for %%f in (*.vert *.frag) do (
    echo [Vulkan] %%f
    %GLSLC% --target-env=vulkan1.3 -O "%%f" -o "spv\%%f.spv"
    if errorlevel 1 exit /b 1

    set NAME=%%~nf
    if /I not "!NAME:~0,3!"=="vk_" (
        echo [OpenGL] %%f
        %GLSLC% -G -O "%%f" -o "spv\gl\%%f.spv"
        if errorlevel 1 exit /b 1
    )
)

echo SPIR-V generated in assets\shaders\spv
endlocal

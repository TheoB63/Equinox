@echo off
REM ============================================================================
REM  Compiles all the GLSL shaders (.vert / .frag) of this folder to SPIR-V,
REM  TWICE, from the very same source file:
REM
REM    glslc -V  -> assets\shaders\spv\<name>.spv      Vulkan  (GfxVulkanBackend)
REM    glslc -G  -> assets\shaders\spv\gl\<name>.spv   OpenGL  (GfxOpenGLBackend,
REM                                                    loaded through ARB_gl_spirv)
REM
REM  That is how Vulkan and OpenGL run exactly the same shader.
REM  Run it again whenever you modify a .vert or a .frag.
REM  The pre-compiled .spv files are provided: this script is only needed if
REM  you change a shader.
REM
REM  Rules for a shader to compile for BOTH targets:
REM    * no push constant  (OpenGL SPIR-V does not allow them)
REM    * "set" must be 0
REM    * the per-object data goes through a std140 block (binding 2)
REM ============================================================================
setlocal
cd /d "%~dp0"

if "%VULKAN_SDK%"=="" (
    echo [ERROR] The VULKAN_SDK environment variable was not found. Install the Vulkan SDK.
    pause
    exit /b 1
)

set GLSLC="%VULKAN_SDK%\Bin\glslc.exe"
if not exist spv mkdir spv
if not exist spv\gl mkdir spv\gl

for %%f in (*.vert *.frag) do (
    echo [Vulkan ] %%f
    %GLSLC% -V "%%f" -o "spv\%%f.spv"
    if errorlevel 1 (
        echo [ERROR] Vulkan compilation failed for %%f
        pause
        exit /b 1
    )

    echo [OpenGL ] %%f
    %GLSLC% -G "%%f" -o "spv\gl\%%f.spv"
    if errorlevel 1 (
        echo [ERROR] OpenGL compilation failed for %%f
        pause
        exit /b 1
    )
)

echo.
echo Done:
echo   assets\shaders\spv     (Vulkan)
echo   assets\shaders\spv\gl  (OpenGL / ARB_gl_spirv)
pause

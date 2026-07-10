@echo off
REM Compile all GLSL shaders to SPIR-V on Windows

setlocal enabledelayedexpansion

set SHADER_DIR=src\main\resources\shaders
set OUTPUT_DIR=build\shaders

REM Check if Vulkan SDK is installed
if not defined VULKAN_SDK (
    echo Error: VULKAN_SDK environment variable not set.
    echo Please install Vulkan SDK from https://vulkan.lunarg.com/sdk/home
    exit /b 1
)

set GLSLANG=%VULKAN_SDK%\Bin\glslangValidator.exe

if not exist "%GLSLANG%" (
    echo Error: glslangValidator not found at %GLSLANG%
    exit /b 1
)

REM Create output directory
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo Compiling vertex shaders...
for %%f in ("%SHADER_DIR%\*.vert") do (
    set filename=%%~nxf
    set output=%OUTPUT_DIR%\!filename!.spv
    echo   Compiling %%f -^> !output!
    "%GLSLANG%" -V "%%f" -o "!output!"
)

echo Compiling fragment shaders...
for %%f in ("%SHADER_DIR%\*.frag") do (
    set filename=%%~nxf
    set output=%OUTPUT_DIR%\!filename!.spv
    echo   Compiling %%f -^> !output!
    "%GLSLANG%" -V "%%f" -o "!output!"
)

echo Shader compilation complete!
echo SPIR-V files are in: %OUTPUT_DIR%

endlocal
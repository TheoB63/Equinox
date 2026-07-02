workspace "Equinox"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Equinox/vendor/GLFW/include"
IncludeDir["Glad"] = "Equinox/vendor/Glad/include"
IncludeDir["ImGui"] = "Equinox/vendor/imgui"
IncludeDir["glm"] = "Equinox/vendor/glm"

include "Equinox/vendor/GLFW"
include "Equinox/vendor/Glad"
include "Equinox/vendor/imgui"

project "Equinox"
	location "Equinox"
	kind "SharedLib"
	language "C++"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "eqnpch.h"
	pchsource "Equinox/src/eqnpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}"	
	}

	links 
	{ 
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

    buildoptions
		{
			"/utf-8"
		}

		defines
		{
			"EQN_PLATFORM_WINDOWS",
			"EQN_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
		}

	filter "configurations:Debug"
		defines "EQN_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "EQN_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "EQN_DIST"
		runtime "Release"
		optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Equinox/vendor/spdlog/include",
		"Equinox/src",
		"%{IncludeDir.glm}"
	}

	links
	{
		"Equinox"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

    buildoptions
		{
			"/utf-8"
		}
    
		defines
		{
			"EQN_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "EQN_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "EQN_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "EQN_DIST"
		runtime "Release"
		optimize "On"
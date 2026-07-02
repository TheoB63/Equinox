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

-- Include directories relative to root folder (solution directory)
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
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

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

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
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
		systemversion "latest"

		defines
		{
			"EQN_PLATFORM_WINDOWS",
			"EQN_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines "EQN_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "EQN_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "EQN_DIST"
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

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
		"Equinox/vendor",
		"%{IncludeDir.glm}"
	}

	links
	{
		"Equinox"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"EQN_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "EQN_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "EQN_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "EQN_DIST"
		runtime "Release"
		optimize "on"

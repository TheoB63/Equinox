workspace "Equinox"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Equinox"
	location "Equinox"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

    buildoptions
		{
			"/utf-8"
		}

		defines
		{
			"EQN_PLATFORM_WINDOWS",
			"EQN_BUILD_DLL"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
		}

	filter "configurations:Debug"
		defines "EQN_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "EQN_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "EQN_DIST"
		optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"

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
		"Equinox/src"
	}

	links
	{
		"Equinox"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
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
		symbols "On"

	filter "configurations:Release"
		defines "EQN_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "EQN_DIST"
		optimize "On"
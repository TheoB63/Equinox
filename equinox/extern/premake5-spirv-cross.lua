-- ============================================================================
--  equinox/extern/premake5-spirv-cross.lua
--
--  Calqué sur luth/extern/premake5-spirv-cross.lua de Hekbas/Luth
--  (commit 235669436b, 17 mars 2026 : "fix(renderer): enable SPIRV-Cross
--   reflection via source build"), adapté aux chemins d'Equinox.
--
--  A placer dans equinox/extern/ et à référencer dans le premake5.lua racine :
--      include "equinox/extern/premake5-spirv-cross"
--
--  Les sources SPIRV-Cross doivent être dans :
--      equinox/extern/source/spirv-cross/       (sous-module git, tag = celui
--   du SDK Vulkan installé ; voir FIX-SPIRV-CROSS.md)
--
--  IMPORTANT : les .cpp ci-dessous compilent les headers SITUÉS DANS LE MÊME
--  ARBRE (includedirs "source/spirv-cross"). Le code d'Equinox doit donc
--  inclure <spirv_cross.hpp> et <spirv_glsl.hpp> (forme plate), et NON
--  <spirv_cross/spirv_cross.hpp> (les headers vendus dans
--  vulkan/include/spirv_cross/ resteraient figés en 1.4.304 => mismatch
--  header/lib => LNK2019 à nouveau).
-- ============================================================================

project "spirv-cross"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	architecture "x86_64"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"source/spirv-cross/spirv_cross.cpp",             -- Compiler::get_member_name() etc.
		"source/spirv-cross/spirv_cfg.cpp",
		"source/spirv-cross/spirv_cross_parsed_ir.cpp",
		"source/spirv-cross/spirv_parser.cpp",
		"source/spirv-cross/spirv_glsl.cpp",              -- CompilerGLSL (spirv_glsl.hpp)
		"source/spirv-cross/spirv_cross_util.cpp",
	}

	includedirs { "source/spirv-cross" }

	buildoptions { "/utf-8" }
	defines { "SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS" }

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"

	filter {}

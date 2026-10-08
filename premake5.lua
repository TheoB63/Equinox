include "dependencies.lua"

workspace "Equinox"
   architecture "x86_64"
   startproject "EquinoxApp"

   buildoptions { "/utf-8" }

   configurations
   {
      "Debug",
      "Release",
      "Dist"
   }

   flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "Equinox"
   include "Equinox"
group ""

group "Equinox/Extern"
   include "equinox/extern/premake5-assimp"
   include "equinox/extern/premake5-glad"
   include "equinox/extern/premake5-glfw"
   include "equinox/extern/premake5-glm"
   include "equinox/extern/premake5-imgui"
   include "equinox/extern/premake5-imguizmo"
   include "equinox/extern/premake5-tracy"
   include "equinox/extern/premake5-spirv-cross"
group ""

group "EquinoxApp"
   include "EquinoxApp"
group ""

group "EquinoxApp/Extern"
      include "equinoxApp/extern"
group ""

group "Sandbox"
   include "Sandbox"
group ""

group "Tools"
   include "extern/premake"
group ""
#include "eqnpch.h"
#include "equinox/graphics/GfxBackend.h"
#include "equinox/graphics/GfxVulkanBackend.h"
#include "equinox/graphics/GfxOpenGLBackend.h"
#include "equinox/graphics/GfxPipeline.h"   // VulkanBackend owns a std::unique_ptr<GfxPipeline>
#include "equinox/core/Log.h"

#include <cassert>

namespace Equinox::Gfx
{
	namespace
	{
		IBackend* s_Backend = nullptr;
	}

	namespace Backend
	{
		bool Create(BackendType type, void* windowHandle, u32 width, u32 height, bool vsync)
		{
			if (s_Backend)
			{
				EQN_CORE_WARN("Gfx::Backend::Create called twice: keeping the current backend ({0})", s_Backend->GetName());
				return true;
			}

			// ---- the API -> implementation mapping ----
			IBackend* backend = nullptr;
			switch (type)
			{
			case BackendType::Vulkan:  backend = new VulkanBackend();  break;
			case BackendType::OpenGL:  backend = new OpenGLBackend();  break;
			default:
				EQN_CORE_CRITICAL("Unknown graphics backend");
				return false;
			}

			backend->Init(windowHandle, width, height, vsync);
			s_Backend = backend;

			EQN_CORE_INFO("Graphics backend: {0}", backend->GetName());
			return true;
		}

		bool Exists()
		{
			return s_Backend != nullptr;
		}

		IBackend& Get()
		{
			assert(s_Backend && "Gfx::Backend::Get() called before Backend::Create()");
			return *s_Backend;
		}

		void Destroy()
		{
			if (!s_Backend) return;

			s_Backend->Shutdown();
			delete s_Backend;
			s_Backend = nullptr;
		}
	}
}

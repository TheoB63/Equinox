#pragma once

#include <memory>

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Equinox {

	class EQUINOX_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

// Core log macros
#define EQN_CORE_TRACE(...)    ::Equinox::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define EQN_CORE_INFO(...)     ::Equinox::Log::GetCoreLogger()->info(__VA_ARGS__)
#define EQN_CORE_WARN(...)     ::Equinox::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define EQN_CORE_ERROR(...)    ::Equinox::Log::GetCoreLogger()->error(__VA_ARGS__)
#define EQN_CORE_FATAL(...)    ::Equinox::Log::GetCoreLogger()->fatal(__VA_ARGS__)

// Client log macros
#define EQN_TRACE(...)	      ::Equinox::Log::GetClientLogger()->trace(__VA_ARGS__)
#define EQN_INFO(...)	      ::Equinox::Log::GetClientLogger()->info(__VA_ARGS__)
#define EQN_WARN(...)	      ::Equinox::Log::GetClientLogger()->warn(__VA_ARGS__)
#define EQN_ERROR(...)	      ::Equinox::Log::GetClientLogger()->error(__VA_ARGS__)
#define EQN_FATAL(...)	      ::Equinox::Log::GetClientLogger()->fatal(__VA_ARGS__)
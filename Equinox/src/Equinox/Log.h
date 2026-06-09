#pragma once

#include <memory>

#include "Core.h"
#include "spdlog/spdlog.h"

namespace Equinox {

	class EQUINOX_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
	};

}

// Core log macros
#define EQN_CORE_TRACE(...)    ::Equinox::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define EQN_CORE_INFO(...)     ::Equinox::Log::GetCoreLogger()->info(__VA_ARGS__)
#define EQN_CORE_WARN(...)     ::Equinox::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define EQN_CORE_ERROR(...)    ::Equinox::Log::GetCoreLogger()->error(__VA_ARGS__)
#define EQN_CORE_CRITICAL(...) ::Equinox::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define EQN_TRACE(...)         ::Equinox::Log::GetClientLogger()->trace(__VA_ARGS__)
#define EQN_INFO(...)          ::Equinox::Log::GetClientLogger()->info(__VA_ARGS__)
#define EQN_WARN(...)          ::Equinox::Log::GetClientLogger()->warn(__VA_ARGS__)
#define EQN_ERROR(...)         ::Equinox::Log::GetClientLogger()->error(__VA_ARGS__)
#define EQN_CRITICAL(...)      ::Equinox::Log::GetClientLogger()->critical(__VA_ARGS__)
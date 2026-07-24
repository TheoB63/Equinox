#include <Equinox.h>
#include <Equinox/core/EntryPoint.h>

#include "VulkanApp.h"
#include "RTShaderApp.h"

#include <vector>
#include <string>
#include <cstring>
#include <utility>

namespace EquinoxTest
{
	void CreateTestArgs(const std::vector<std::string>& args, int& argc, char**& argv)
	{
		argc = static_cast<int>(args.size());
		argv = new char* [argc + 1];

		for (int i = 0; i < argc; ++i) {
			argv[i] = new char[args[i].size() + 1];
			std::strcpy(argv[i], args[i].c_str());
		}
		argv[argc] = nullptr;
	}

	void FreeTestArgs(int argc, char** argv)
	{
		for (int i = 0; i < argc; ++i) {
			delete[] argv[i];
		}
		delete[] argv;
	}
}

namespace Equinox
{
	App* CreateApp(int argc, char** argv)
	{
		int testArgc = argc;
		char** testArgv = argv;

		if (argc < 2) { // No argument
			std::vector<std::string> args = { "Sandbox", "--opengl" };
			EquinoxTest::CreateTestArgs(args, testArgc, testArgv);

			EQN_CORE_WARN("No arguments provided");
			EQN_CORE_WARN("Valid Args: {} [--opengl|--vulkan]", argv[0]);
			EQN_CORE_WARN("Initializing default {}", args[1]);
		}

		std::string appType = testArgv[1];
		if (appType == "--opengl") {
			return new RTShaderApp(testArgc, testArgv);
		}
		else if (appType == "--vulkan") {
			return new VulkanApp(testArgc, testArgv);
		}
		else {  // Invalid argument
			std::vector<std::string> args = { "Sandbox", "--vulkan" };
			EquinoxTest::CreateTestArgs(args, testArgc, testArgv);
			EQN_CORE_WARN("Unknown argument: '{}'. Valid Args: [--opengl|--vulkan]", appType);
			EQN_CORE_WARN("Initializing default [--vulkan]");
			return new VulkanApp(testArgc, testArgv);
		}
	}
}
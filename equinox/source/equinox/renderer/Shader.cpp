#include "eqnpch.h"
#include "equinox/renderer/Shader.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/openGL/GLShader.h"

namespace Equinox
{
	std::shared_ptr<Shader> Shader::Create(const std::string& filePath)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return std::make_shared<GLShader>(filePath);
		default:
			EQN_CORE_ASSERT(false, "Unknown RendererAPI!");
			return nullptr;
		}
	}

	std::shared_ptr<Shader> Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return std::make_shared<GLShader>(vertexSrc, fragmentSrc);
		default:
			EQN_CORE_ASSERT(false, "Unknown RendererAPI!");
			return nullptr;
		}
	}

	std::string Shader::Load(const std::string& filePath)
	{
		std::ifstream in(filePath, std::ios::in | std::ios::binary);
		if (!in)
		{
			EQN_CORE_ERROR("Could not open shader file: {0}", filePath);
			return "";
		}

		std::stringstream buffer;
		buffer << in.rdbuf();
		return buffer.str();
	}
}
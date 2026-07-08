#include "eqnpch.h"
#include "equinox/core/App.h"
#include "equinox/core/Log.h"
#include "equinox/core/Timestep.h"
#include "equinox/window/Window.h"
#include "equinox/input/Input.h"
#include "equinox/editor/Editor.h"
#include "equinox/events/Event.h"
#include "equinox/events/AppEvent.h"
#include "equinox/events/KeyEvent.h"
#include "equinox/resources/ShaderLibrary.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/Shader.h"

// TEST need to be removed later
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

namespace Equinox
{
	App::App()
	{
		WindowSpec spec;
		spec.Title = "Equinox Engine";

		m_Window = std::make_unique<Window>(spec);
		Input::SetWindow(m_Window->GetNativeWindow());
		Editor::Init(m_Window->GetNativeWindow());

		// TEST need to be removed later
		float vertices[] =
		{
			// Positions   // TexCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};

		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);

		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// Position attribute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

		// Texture coordinate attribute
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		glBindVertexArray(0);
	}

	App::~App()
	{
	}

	void App::Run()
	{
		OnInit();

		while (m_Running)
		{
			f32 time = static_cast<f32>(glfwGetTime());
			f32 dt = time - m_LastFrameTime;
			m_LastFrameTime = time;

			// Update window first
			m_Window->OnUpdate();

			// User-defined update
			OnUpdate(dt);


			// TEST need to be removed later
			auto shader = Shader::Create("C:/Users/theob/Documents/BlanchardTheo/Documents/Cours/3emeAnnee/Projets Persos/Equinox/equinoxApp/resources/test.glsl");

			shader->Bind();
			shader->SetFloat("u_Time", time);
			//shader->SetVec2("u_Resolution",  glm::vec2((f32)m_Window->GetWidth(), (f32)m_Window->GetHeight()));
			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			shader->Unbind();
			// TEST END


			// Render UI
			Editor::BeginFrame();
			OnUIRender();
			Editor::EndFrame();

			m_Window->SwapBuffers();
		}
		OnShutdown();
	}

	void App::Close()
	{
		m_Running = false;
	}
}
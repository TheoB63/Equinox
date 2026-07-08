#include "Equinox.h"

#include <imgui.h>

//TEST need to be removed later
#include "equinox/resources/ShaderLibrary.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/Shader.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory> 

namespace Equinox
{
	class EquinoxApp : public App
	{
	public:
		EquinoxApp() {}

		~EquinoxApp() override = default;

	protected:
		void OnInit() override
		{
			InitTestOpenGL();
		}

		void OnUpdate(f32 dt) override
		{
			static float time = 0;
			time += dt;

			TestOpenGL(time);
		}

		void OnUIRender() override
		{
			ImGui::Begin("Equinox Dashboard");
			//ImGui::Text("Welcome to Equinox!");
			ImGui::End();

			// Demo Window
			static bool showDemo = true;
			if (showDemo)
			{
				ImGui::ShowDemoWindow(&showDemo);
			}
		}

		void OnShutdown() override {}


		// TEST OPENGL need to be removed later

		GLuint quadVAO, quadVBO;

		void InitTestOpenGL()
		{
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

		void TestOpenGL(float time)
		{
			auto shader = Shader::Create("C:/Users/theob/Documents/BlanchardTheo/Documents/Cours/3emeAnnee/Projets Persos/Equinox/equinoxApp/resources/test.glsl");

			shader->Bind();
			shader->SetFloat("u_time", time);
			//shader->SetVec2("u_resolution",  glm::vec2(1280.0, 720.0));
			//shader->SetVec2("u_Resolution",  glm::vec2((f32)m_Window->GetWidth(), (f32)m_Window->GetHeight()));
			//shader->SetFloat("u_playerJump", Input::IsMouseButtonPressed(0));
			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			shader->Unbind();
		}

	};

	App* createApp()
	{
		return new EquinoxApp();
	}
}

int main()
{
	Equinox::Log::Init();
	Equinox::App* app = Equinox::createApp();
	app->Run();
	delete app;
	return 0;
}
#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <cassert>
#include <vector>
#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include "GL_framework.h"
#include <iostream>

#pragma region Global Variables
static int mode = 1;
//CHICKEN
std::vector< glm::vec3 > chickenVertices;
std::vector< glm::vec2 > chickenUvs;
std::vector< glm::vec3 > chickenNormals;

glm::vec3 chickenPosition = { 0.f,0.f,0.f };

//TRUMP
std::vector< glm::vec3 > trumpVertices;
std::vector< glm::vec2 > trumpUvs;
std::vector< glm::vec3 > trumpNormals;

glm::vec3 trumpPosition = { 0.f,0.f,0.f };

#pragma endregion


#pragma region Namespaces
namespace Model
{
	void setupModels();
	void setupSpecificModel(GLuint &vao, GLuint vbo[], std::vector < glm::vec3 > &vertices, std::vector < glm::vec3 > &normals);

	void cleanupModels();
	void cleanupSpecificModel(GLuint &vao, GLuint vbo[]);

	void updateModel(glm::mat4 &objMat, glm::mat4& transform);

	void updateModels(double time);
	void drawTrump(double time);
	void drawChicken(double time);
	void drawSpecificModel(GLuint &vao, glm::mat4 &objMat, std::vector < glm::vec3 > &vertices, glm::vec4 color, float time);

}
namespace ImGui {
	void Render();
}
namespace RenderVars
{
	const float FOV = glm::radians(65.f);
	const float zNear = 1.f;
	const float zFar = 10000.f;

	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;

	struct prevMouse
	{
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	float panv[3] = { 0.f, 10.f, -10.f };
	float rota[2] = { 0.f, 0.f };
}
namespace RV = RenderVars;
#pragma endregion


#pragma region Functions

extern bool loadOBJ(const char * path, std::vector < glm::vec3 > & out_vertices, std::vector < glm::vec2 > & out_uvs, std::vector < glm::vec3 > & out_normals);

void loadAllModels()
{
	bool res = loadOBJ("trump.obj", trumpVertices, trumpUvs, trumpNormals);
	res = loadOBJ("chicken.obj", chickenVertices, chickenUvs, chickenNormals);
	Model::setupModels();
}

void drawLoop(double currentTime)
{
	bool isTrump = false;
	chickenPosition = { 0.f,1.2f,0.f };
	trumpPosition = { 0.f,0.f,0.f };

	for (int i = 0; i <= 10; i++)//columnes
	{
		trumpPosition.x = 0.f;
		chickenPosition.x = 0.f;

		for (int j = 0; j <= 10; j++)//files
		{
			trumpPosition.x += 1.0;
			chickenPosition.x += 1.0;
			Model::updateModels(currentTime);
			if (isTrump)
			{
				Model::drawTrump(currentTime);
			}
			else
			{
				Model::drawChicken(currentTime);
			}
			isTrump = !isTrump;
		}
		trumpPosition.y -= 1.0;
		chickenPosition.y -= 1.0;
		//isTrump = !isTrump;
	}
}


#pragma region defaultFunctions
void GUI()
{
	bool show = true;
	ImGui::Begin("Simulation Parameters", &show, 0);

	// Do your GUI code here....
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate

		ImGui::RadioButton("Loop", &mode, 1);
		ImGui::RadioButton("Instancing", &mode, 2);
		ImGui::RadioButton("MultiDraw", &mode, 3);
	}
	// .........................

	ImGui::End();
}
void GLResize(int width, int height)
{
	glViewport(0, 0, width, height);
	if (height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev)
{
	if (RV::prevMouse.waspressed && RV::prevMouse.button == ev.button)
	{
		float diffx = ev.posx - RV::prevMouse.lastx;
		float diffy = ev.posy - RV::prevMouse.lasty;
		switch (ev.button)
		{
		case MouseEvent::Button::Left: // ROTATE
			RV::rota[0] += diffx * 0.005f;
			RV::rota[1] += diffy * 0.005f;
			break;
		case MouseEvent::Button::Right: // MOVE XY
			RV::panv[0] += diffx * 0.03f;
			RV::panv[1] -= diffy * 0.03f;
			break;
		case MouseEvent::Button::Middle: // MOVE Z
			RV::panv[2] += diffy * 0.05f;
			break;
		default: break;
		}
	}
	else
	{
		RV::prevMouse.button = ev.button;
		RV::prevMouse.waspressed = true;
	}
	RV::prevMouse.lastx = ev.posx;
	RV::prevMouse.lasty = ev.posy;
}
//////////////////////////////////// COMPILE AND LINK
GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name = "")
{
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);
	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (res == GL_FALSE)
	{
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetShaderInfoLog(shader, res, &res, buff);
		fprintf(stderr, "Error Shader %s: %s", name, buff);
		delete[] buff;
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
void linkProgram(GLuint program)
{
	glLinkProgram(program);
	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (res == GL_FALSE)
	{
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetProgramInfoLog(program, res, &res, buff);
		fprintf(stderr, "Error Link: %s", buff);
		delete[] buff;
	}
}
#pragma endregion
#pragma endregion

//////////////////////////////////////////////////////////////////////////////////////////////////// INIT
void GLinit(int width, int height) {
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	RV::_projection = glm::perspective(RV::FOV, (float)width/(float)height, RV::zNear, RV::zFar);

	if (mode == 1)
	{
		loadAllModels();
		Model::setupModels();
	}
	else if (mode == 2)
	{

	}
	else if (mode == 3)
	{

	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////// RENDER
void GLrender(double currentTime) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//CAMERA
	RV::_modelView = glm::mat4(1.f);
	RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));

	if (mode == 1)
	{
		drawLoop(currentTime);
	}
	else if (mode == 2)
	{

	}
	else if (mode == 3)
	{

	}



	RV::_MVP = RV::_projection * RV::_modelView;

	ImGui::Render();
}

//////////////////////////////////////////////////////////////////////////////////////////////////// CLEANUP
void GLcleanup()
{

	Model::cleanupModels();
}

//////////////////////////////////////////////////////////////////////////////////////////////////// MODEL
namespace Model
{
#pragma region model variables
	//VAOs
	GLuint trumpVao;
	GLuint chickenVao;

	//VBOs
	GLuint trumpVbo[3];
	GLuint chickenVbo[3];

	GLuint modelShaders[2];
	GLuint modelProgram;

	//OBJECT MATRICES
	glm::mat4 chickenObjMat = glm::mat4(1.f);
	glm::mat4 trumpObjMat = glm::mat4(1.f);


	//SCALE MATRICES
	glm::mat4 trumpScale = glm::scale(glm::mat4(), glm::vec3(0.1, 0.1, 0.1));
	glm::mat4 chickenScale = glm::scale(glm::mat4(), glm::vec3(0.01, 0.01, 0.01));

	//COLORS:
	glm::vec4 trumpColor = { 0.f,0.f,1.f,1.f };
	glm::vec4 chickenColor = { 1.f,1.f,0.f,1.f };


#pragma endregion

	//SHADERS
#pragma region shader
	const char* model_vertShader =
		"#version 330\n\
		in vec3 in_Position;\n\
		in vec3 in_Normal;\n\
		out vec4 vert_Normal;\n\
		uniform mat4 objMat;\n\
		uniform mat4 mv_Mat;\n\
		uniform mat4 mvpMat;\n\
		void main() {\n\
			gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
			vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
		}";


	const char* model_fragShader =
		"#version 330\n\
		in vec4 vert_Normal;\n\
		out vec4 out_Color;\n\
		uniform mat4 mv_Mat;\n\
		uniform vec4 color;\n\
		void main() {\n\
			out_Color = vec4(color.xyz * dot(normalize(vert_Normal), mv_Mat*vec4(0.0, 1.0, 0.0, 0.0)) + color.xyz * 0.3, 1.0 );\n\
		}";

#pragma endregion
	//SETUP
	void setupModels()
	{
		//Trump
		setupSpecificModel(trumpVao, trumpVbo, trumpVertices, trumpNormals);

		//Chicken
		setupSpecificModel(chickenVao, chickenVbo, chickenVertices, chickenNormals);
	}


	void setupSpecificModel(GLuint &vao, GLuint vbo[], std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals)
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(3, vbo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		modelShaders[0] = compileShader(model_vertShader, GL_VERTEX_SHADER, "modelVert");
		modelShaders[1] = compileShader(model_fragShader, GL_FRAGMENT_SHADER, "modelFrag");

		modelProgram = glCreateProgram();
		glAttachShader(modelProgram, modelShaders[0]);
		glAttachShader(modelProgram, modelShaders[1]);
		glBindAttribLocation(modelProgram, 0, "in_Position");
		glBindAttribLocation(modelProgram, 1, "in_Normal");
		linkProgram(modelProgram);
	}

	//CLEANUP
	void cleanupModels()
	{
		//Trump
		cleanupSpecificModel(trumpVao, trumpVbo);

		//Chicken
		cleanupSpecificModel(chickenVao, chickenVbo);
	}

	void cleanupSpecificModel(GLuint &vao, GLuint vbo[])
	{
		glDeleteBuffers(2, vbo);
		glDeleteVertexArrays(1, &vao);

		glDeleteProgram(modelProgram);
		glDeleteShader(modelShaders[0]);
		glDeleteShader(modelShaders[1]);
	}

	//UPDATE AND DRAW
	void updateModel(glm::mat4 &objMat, glm::mat4& transform)
	{
		objMat = transform;
	}

	void updateModels(double time)
	{
		//TRUMP
		updateModel(trumpObjMat, glm::translate(glm::mat4(), trumpPosition) * trumpScale);

		//CHICKEN
		updateModel(chickenObjMat, glm::translate(glm::mat4(), chickenPosition) * chickenScale);;
	}

	void drawTrump(double time)
	{
		drawSpecificModel(trumpVao, trumpObjMat, trumpVertices, trumpColor, time);

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}

	void drawChicken(double time)
	{
		drawSpecificModel(chickenVao, chickenObjMat, chickenVertices, chickenColor, time);

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}


	void drawSpecificModel(GLuint &vao, glm::mat4 &objMat, std::vector<glm::vec3> &vertices, glm::vec4 color, float time)
	{
		glBindVertexArray(vao);
		glUseProgram(modelProgram);

		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform1f(glGetUniformLocation(modelProgram, "time"), time);
		glUniform4f(glGetUniformLocation(modelProgram, "color"), color.x, color.y, color.z, color.t);

		glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 3);
	}
}

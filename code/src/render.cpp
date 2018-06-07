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
extern int mode;

const int NUM_ELEMENTS = 10000;
int nRows = NUM_ELEMENTS/100;
int nCols = NUM_ELEMENTS/100;

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

//MultiDraw
std::vector<glm::vec3> vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

#pragma endregion


#pragma region Namespaces
namespace Model
{
	//basic draw loop
	void setupModels();
	void setupSpecificModel(GLuint &vao, GLuint vbo[], std::vector < glm::vec3 > &vertices, std::vector < glm::vec3 > &normals);

	void cleanupModels();
	void cleanupSpecificModel(GLuint &vao, GLuint vbo[]);

	void updateModel(glm::mat4 &objMat, glm::mat4& transform);
	void updateTrump(double time);
	void updateChicken(double time);
	void drawTrump(double time);
	void drawChicken(double time);
	void drawSpecificModel(GLuint &vao, glm::mat4 &objMat, std::vector < glm::vec3 > &vertices, glm::vec4 color, float time);

	//instancing v1
	void instancingSetupModels();
	void instancingSetup(GLuint &vao, GLuint vbo[], std::vector < glm::vec3 > &vertices, std::vector < glm::vec3 > &normals);

	void instancingDrawModels(double time);
	void instancingDraw(GLuint &vao, glm::mat4 &objMat, std::vector < glm::vec3 > &vertices, glm::vec4 color, float time, glm::vec3 offset);

	//instancing v2
	void updateModelMatrices(double time);
	void instancingSetupModels2();
	void instancingSetup2(GLuint &vao, GLuint vbo[], std::vector < glm::vec3 > &vertices, std::vector < glm::vec3 > &normals);

	void instancingDrawModels2(double time);
	void instancingDraw2(GLuint &vao, glm::mat4 &objMat, std::vector < glm::vec3 > &vertices, glm::vec4 color, float time);

	//MultiDrawIndirect
	void setupMultiDraw();
	void cleanupMultiDraw();
	void renderMultiDraw();
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

	float panv[3] = { 0.f, 0.f, -10.f };
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

	//MultiDrawIndirect
	res = loadOBJ("trump.obj", vertices, uvs, normals);
	vertices.insert(vertices.end(), chickenVertices.begin(), chickenVertices.end());
	normals.insert(normals.end(), chickenNormals.begin(), chickenNormals.end());

	Model::setupModels();	// ¿?¿?
}
//Wave:
const float amplitude = 0.5f;
const float frequency = 4.0f;
const glm::vec3 waveDirection = { 0.f, -1.f, 0.f };
const float lambda = 0.3f;
const float phi = 1.0f;
const float k = (lambda / (2 * glm::pi<float>()));

void gerstnerWave(glm::vec3 &pos, glm::vec3 x0, float time)
{
	pos -= waveDirection * k* amplitude * sin(glm::dot(waveDirection, x0) - frequency * time + phi);
	pos.z += amplitude * cos(glm::dot(waveDirection, x0) - frequency* time + phi);
}

void drawLoop(double currentTime)
{
	glm::vec3 chickenOffset = { 0.f,1.2f,0.f };

	for (int i = 1; i <= NUM_ELEMENTS/2; i++)//columnes
	{
		trumpPosition = glm::vec3((i % 50)*2.f, -i / 50 * 2.6f, 0.f);
		Model::updateTrump(currentTime);
		Model::drawTrump(currentTime);
		chickenPosition = glm::vec3((i % 50)*2.f, -i / 50 * 2.6f, 0.f) + chickenOffset;
		Model::updateChicken(currentTime);
		Model::drawChicken(currentTime);
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

	//camera
	RV::_projection = glm::perspective(RV::FOV, (float)width/(float)height, RV::zNear, RV::zFar);
	RV::panv[0] = 0.f;
	RV::panv[1] = 0.f;
	//RV::panv[0] = -50.f;
	//RV::panv[1] = 50.f;
	RV::panv[2] = -50.f;
	RV::rota[0] = 0.f;
	RV::rota[1] = 0.f;

	loadAllModels();

	if (mode == 1)
	{
		Model::setupModels();
	}
	else if (mode == 2)
	{
		Model::instancingSetupModels();
	}
	else if (mode == 3)
	{
		Model::setupMultiDraw();
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
		Model::instancingDrawModels(currentTime);
	}
	else if (mode == 3)
	{
		Model::renderMultiDraw();
	}

	RV::_MVP = RV::_projection * RV::_modelView;

	ImGui::Render();
}

//////////////////////////////////////////////////////////////////////////////////////////////////// CLEANUP
void GLcleanup()
{
	Model::cleanupModels();
	Model::cleanupMultiDraw();
}

//////////////////////////////////////////////////////////////////////////////////////////////////// MODEL
namespace Model
{
#pragma region model variables
	//VAOs
	GLuint trumpVao;
	GLuint chickenVao;
	GLuint multiVao;

	//VBOs
	GLuint trumpVbo[3];
	GLuint chickenVbo[3];
	GLuint multiVbo[3];

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

	//INSTANCING:
	glm::mat4 modelMatrices[NUM_ELEMENTS/2];

	//MultiDrawIndirect
	typedef  struct {
		GLuint  count;
		GLuint  primCount;
		GLuint  first;
		GLuint  baseInstance;
	} DrawArraysIndirectCommand;
	DrawArraysIndirectCommand cmd[2];

	glm::mat4 multiobjMat = glm::mat4(1.f);
	

#pragma endregion
	
#pragma region basicDrawLoop

	//SHADERS
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
		glGenBuffers(2, vbo);

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

	void updateTrump(double time)
	{
		glm::vec3 aux = trumpPosition;
		gerstnerWave(aux, trumpPosition, time);
		//TRUMP
		updateModel(trumpObjMat, glm::translate(glm::mat4(), aux) * trumpScale);
	}
	void updateChicken(double time)
	{
		glm::vec3 aux = chickenPosition;
		gerstnerWave(aux, chickenPosition, time);
		//CHICKEN
		updateModel(chickenObjMat, glm::translate(glm::mat4(), aux) * chickenScale);
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
#pragma endregion

#pragma region Instancing v1
	const char* instancing_vertShader =
		"#version 330\n\
		in vec3 in_Position;\n\
		in vec3 in_Normal;\n\
		\n\
		out vec4 vert_Normal;\n\
		uniform mat4 scale;\n\
		uniform mat4 mv_Mat;\n\
		uniform mat4 mvpMat;\n\
		uniform float time;\n\
		uniform vec3 offset;\n\
		\n\
		//Wave:\n\
		float amplitude = 0.5f;\n\
		float frequency = 4.0f;\n\
		vec3 waveDirection = vec3( 0.f, -1.f, 0.f );\n\
		float lambda = 0.3f;\n\
		float phi = 1.0f;\n\
		uniform float k;\n\
		\n\
		vec3 gerstnerWave(vec3 pos, vec3 x0, float time)\n\
		{\n\
			pos -= waveDirection * k* amplitude * sin(dot(waveDirection, x0) - frequency * time + phi);\n\
			pos.z += amplitude * cos(dot(waveDirection, x0) - frequency * time + phi);\n\
			return pos;\n\
		}\n\
		\n\
		\n\
		void main() {\n\
			vec3 position = vec3((gl_InstanceID%50)*2.f, -gl_InstanceID/50*2.6f,0.f) + offset;\n\
			position = gerstnerWave(position, position, time);\n\
			mat4 translation = mat4(1.0, 0.0, 0.0, 0.0,    0.0, 1.0, 0.0, 0.0,    0.0, 0.0, 1.0, 0.0,    position.x, position.y, 0.0, 1.0); \n\
			gl_Position = mvpMat *translation * scale * vec4(in_Position, 1.0);\n\
			vert_Normal = mv_Mat * translation* scale * vec4(in_Normal, 0.0);\n\
		}";
	const char* instancing_fragShader =
		"#version 330\n\
		in vec4 vert_Normal;\n\
		out vec4 out_Color;\n\
		uniform mat4 mv_Mat;\n\
		uniform vec4 color;\n\
		void main() {\n\
			out_Color = vec4(color.xyz * dot(normalize(vert_Normal), mv_Mat*vec4(0.0, 1.0, 0.0, 0.0)) + color.xyz * 0.3, 1.0 );\n\
		}";

	void instancingSetupModels()
	{
		//Trump
		instancingSetup(trumpVao, trumpVbo, trumpVertices, trumpNormals);

		//Chicken
		instancingSetup(chickenVao, chickenVbo, chickenVertices, chickenNormals);
	}
	void instancingSetup(GLuint &vao, GLuint vbo[], std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals)
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(2, vbo);

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

		modelShaders[0] = compileShader(instancing_vertShader, GL_VERTEX_SHADER, "instancing_modelVert");
		modelShaders[1] = compileShader(instancing_fragShader, GL_FRAGMENT_SHADER, "instancing_modelFrag");

		modelProgram = glCreateProgram();
		glAttachShader(modelProgram, modelShaders[0]);
		glAttachShader(modelProgram, modelShaders[1]);
		glBindAttribLocation(modelProgram, 0, "in_Position");
		glBindAttribLocation(modelProgram, 1, "in_Normal");
		linkProgram(modelProgram);
	}

	void instancingDrawModels(double time)
	{
		instancingDraw(trumpVao, trumpScale, trumpVertices, trumpColor, time, glm::vec3 { 0.f,1.2f,0.f });
		instancingDraw(chickenVao, chickenScale, chickenVertices, chickenColor, time, glm::vec3 { 0.f,0.f,0.f });

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}
	
	void instancingDraw(GLuint &vao, glm::mat4 &scale, std::vector<glm::vec3> &vertices, glm::vec4 color, float time, glm::vec3 offset)
	{
		glBindVertexArray(vao);
		glUseProgram(modelProgram);

		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "scale"), 1, GL_FALSE, glm::value_ptr(scale));
		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform1f(glGetUniformLocation(modelProgram, "time"), time);
		glUniform1f(glGetUniformLocation(modelProgram, "k"), k);
		glUniform3f(glGetUniformLocation(modelProgram, "offset"), offset.x, offset.y, offset.z);
		glUniform4f(glGetUniformLocation(modelProgram, "color"), color.x, color.y, color.z, color.t);

		glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size() * 3, NUM_ELEMENTS/2);
	}
#pragma endregion

#pragma region Instancing v2
	const char* instancing_vertShader2 =
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

	void instancingSetupModels2()
	{
		//Trump
		instancingSetup2(trumpVao, trumpVbo, trumpVertices, trumpNormals);

		//Chicken
		instancingSetup2(chickenVao, chickenVbo, chickenVertices, chickenNormals);
	}
	void instancingSetup2(GLuint &vao, GLuint vbo[], std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals)
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(2, vbo);

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

		modelShaders[0] = compileShader(instancing_vertShader2, GL_VERTEX_SHADER, "instancing_modelVert");
		modelShaders[1] = compileShader(instancing_fragShader, GL_FRAGMENT_SHADER, "modelFrag");

		modelProgram = glCreateProgram();
		glAttachShader(modelProgram, modelShaders[0]);
		glAttachShader(modelProgram, modelShaders[1]);
		glBindAttribLocation(modelProgram, 0, "in_Position");
		glBindAttribLocation(modelProgram, 1, "in_Normal");
		linkProgram(modelProgram);
	}
	void updateModelMatrices(double currentTime)
	{
		bool isTrump = false;
		chickenPosition = { 0.f,1.2f,0.f };
		trumpPosition = { 0.f,0.f,0.f };

		for (int i = 0; i <= nCols; i++)//columnes
		{
			trumpPosition.x = 0.f;
			chickenPosition.x = 0.f;

			for (int j = 0; j <= nRows; j++)//files
			{
				trumpPosition.x += 1.f;
				chickenPosition.x += 1.0f;

				if (isTrump)
				{
					glm::vec3 aux = trumpPosition;
					//gerstnerWave(aux, trumpPosition, time);
					//TRUMP
					updateModel(trumpObjMat, glm::translate(glm::mat4(), aux) * trumpScale);
				}
				else
				{
					Model::updateChicken(currentTime);
					Model::drawChicken(currentTime);
				}
				isTrump = !isTrump;
			}
			trumpPosition.y -= 1.3f;
			chickenPosition.y -= 1.3f;
		}
	}

	void instancingDrawModels2(double time)
	{
		instancingDraw2(trumpVao, trumpScale, trumpVertices, trumpColor, time);
		instancingDraw2(chickenVao, chickenScale, chickenVertices, chickenColor, time);

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}

	void instancingDraw2(GLuint &vao, glm::mat4 &scale, std::vector<glm::vec3> &vertices, glm::vec4 color, float time)
	{
		glBindVertexArray(vao);
		glUseProgram(modelProgram);

		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(scale));
		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));

		glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size() * 3, NUM_ELEMENTS / 2);
	}
#pragma endregion

#pragma region MultiDrawIndirect

	const char* multi_vertShader =
		"#version 330\n\
	in vec3 in_Position;\n\
	in vec3 in_Normal;\n\
	out vec4 vert_Normal;\n\
	uniform mat4 objMat;\n\
	uniform mat4 mv_Mat;\n\
	uniform mat4 mvpMat;\n\
	uniform vec3 offset;\n\
	void main() {\n\
		gl_Position = mvpMat * objMat * vec4(in_Position + vec3(gl_InstanceID*50, 0.0, 0.0), 1.0);\n\
		vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
	}";


	const char* multi_fragShader =
		"#version 330\n\
	in vec4 vert_Normal;\n\
	out vec4 out_Color;\n\
	uniform mat4 mv_Mat;\n\
	uniform vec4 color;\n\
	void main() {\n\
		out_Color = vec4(color.xyz * dot(vert_Normal, mv_Mat*vec4(0.0, 1.0, 0.0, 0.0)) + color.xyz * 0.3, 1.0 );\n\
	}";

	void setupMultiDraw() {
		//Trump
		cmd[0].count = vertices.size() - chickenVertices.size();
		cmd[0].primCount = 5000;
		cmd[0].first = 0;
		cmd[0].baseInstance = 0;

		//Chicken
		cmd[1].count = chickenVertices.size();
		cmd[1].primCount = 5000;
		cmd[1].first = vertices.size() - chickenVertices.size();
		cmd[1].baseInstance = 1;

		glGenVertexArrays(1, &multiVao);
		glBindVertexArray(multiVao);
		glGenBuffers(3, multiVbo);

		glBindBuffer(GL_ARRAY_BUFFER, multiVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, multiVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		modelShaders[0] = compileShader(multi_vertShader, GL_VERTEX_SHADER, "multiVertexShader");
		modelShaders[1] = compileShader(multi_fragShader, GL_FRAGMENT_SHADER, "multiFragmentShader");

		modelProgram = glCreateProgram();
		glAttachShader(modelProgram, modelShaders[0]);
		glAttachShader(modelProgram, modelShaders[1]);
		glBindAttribLocation(modelProgram, 0, "in_Position");
		glBindAttribLocation(modelProgram, 1, "in_Normal");
		linkProgram(modelProgram);


		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, multiVbo[2]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(cmd), &cmd, GL_STATIC_DRAW);
	}

	void cleanupMultiDraw() {
		glDeleteBuffers(2, multiVbo);
		glDeleteVertexArrays(1, &multiVao);

		glDeleteProgram(modelProgram);
		glDeleteShader(modelShaders[0]);
		glDeleteShader(modelShaders[1]);
	}

	void renderMultiDraw() {
		glBindVertexArray(multiVao);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, multiVbo[2]);
		glUseProgram(modelProgram);
		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(multiobjMat));
		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(modelProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(modelProgram, "color"), 0.1f, 1.f, 1.f, 0.f);

		glMultiDrawArraysIndirect(GL_TRIANGLES, 0, 2, 0);
		std::cout << glewGetErrorString(glGetError()) << std::endl;

		glUseProgram(0);
		glBindVertexArray(0);
	}

#pragma endregion
}

#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <cassert>
#include <vector>

#include "GL_framework.h"


//CHICKEN
std::vector< glm::vec3 > chickenVertices;
std::vector< glm::vec2 > chickenUvs;
std::vector< glm::vec3 > chickenNormals;

//TRUMP
std::vector< glm::vec3 > trumpVertices;
std::vector< glm::vec2 > trumpUvs;
std::vector< glm::vec3 > trumpNormals;


///////// fw decl
namespace ImGui {
	void Render();
}


namespace Model
{
	void setupModels();
	void setupSpecificModel(GLuint &vao, GLuint vbo[], std::vector < glm::vec3 > &vertices, std::vector < glm::vec3 > &normals);

	void cleanupModels();
	void cleanupSpecificModel(GLuint &vao, GLuint vbo[]);

	void updateModel(glm::mat4 &objMat, glm::mat4& transform);

	void updateModels(double time);
	void drawModels(double time);
	void drawSpecificModel(GLuint &vao, glm::mat4 &objMat, std::vector < glm::vec3 > &vertices, glm::vec4 color, float time);

	//CAMERA VARIABLES
	glm::vec3 trumpPosition;
	glm::vec3 chickenPosition;

}


////////////////

namespace RenderVars {
	const float FOV = glm::radians(65.f);
	const float zNear = 1.f;
	const float zFar = 50.f;

	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;

	struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	float panv[3] = { 0.f, -5.f, -15.f };
	float rota[2] = { 0.f, 0.f };
}
namespace RV = RenderVars;

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);
	if(height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev) {
	if(RV::prevMouse.waspressed && RV::prevMouse.button == ev.button) {
		float diffx = ev.posx - RV::prevMouse.lastx;
		float diffy = ev.posy - RV::prevMouse.lasty;
		switch(ev.button) {
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
	} else {
		RV::prevMouse.button = ev.button;
		RV::prevMouse.waspressed = true;
	}
	RV::prevMouse.lastx = ev.posx;
	RV::prevMouse.lasty = ev.posy;
}

void GLinit(int width, int height) {
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	RV::_projection = glm::perspective(RV::FOV, (float)width/(float)height, RV::zNear, RV::zFar);

	// Setup shaders & geometry
	/*Box::setupCube();
	Axis::setupAxis();
	Cube::setupCube();*/

}

void GLcleanup() {
	/*Box::cleanupCube();
	Axis::cleanupAxis();
	Cube::cleanupCube();
*/
}

void GLrender(double currentTime) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RV::_modelView = glm::mat4(1.f);
	RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));

	RV::_MVP = RV::_projection * RV::_modelView;

	// render code
	/*Box::drawCube();
	Axis::drawAxis();
	Cube::drawCube();*/



	ImGui::Render();
}


//////////////////////////////////// COMPILE AND LINK
GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name="") {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);
	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (res == GL_FALSE) {
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
void linkProgram(GLuint program) {
	glLinkProgram(program);
	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (res == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetProgramInfoLog(program, res, &res, buff);
		fprintf(stderr, "Error Link: %s", buff);
		delete[] buff;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////// MODEL
namespace Model
{
#pragma region model variables
	//VAOs
	GLuint trumpVao;
	GLuint chickenVao;
	GLuint cabinVao;
	GLuint wheelVao;
	GLuint feetVao;
	GLuint cabinVao2;
	GLuint wheelVao2;
	GLuint feetVao2;

	//VBOs
	GLuint trumpVbo[3];
	GLuint chickenVbo[3];
	GLuint cabinVbo[3];
	GLuint wheelVbo[3];
	GLuint feetVbo[3];
	GLuint cabinVbo2[3];
	GLuint wheelVbo2[3];
	GLuint feetVbo2[3];

	GLuint modelShaders[2];
	GLuint modelProgram;

	//OBJECT MATRICES
	glm::mat4 chickenObjMat = glm::mat4(1.f);
	glm::mat4 trumpObjMat = glm::mat4(1.f);
	glm::mat4 cabinObjMat = glm::mat4(1.f);
	glm::mat4 cabinsObjMats[Ncubes];
	glm::mat4 cabinsObjMats2[Ncubes];
	glm::vec3 cabinPosition1;
	glm::mat4 wheelObjMat = glm::mat4(1.f);
	glm::mat4 feetObjMat = glm::mat4(1.f);
	glm::mat4 cabinObjMat2 = glm::mat4(1.f);
	glm::vec3 cabinPosition2;
	glm::mat4 wheelObjMat2 = glm::mat4(1.f);
	glm::mat4 feetObjMat2 = glm::mat4(1.f);


	//SCALE MATRICES
	glm::mat4 cabinScale = glm::scale(glm::mat4(), glm::vec3(0.015, 0.015, 0.015));
	glm::mat4 wheelScale = glm::scale(glm::mat4(), glm::vec3(0.065, 0.065, 0.065));
	glm::mat4 feetScale = glm::scale(glm::mat4(), glm::vec3(0.15, 0.15, 0.15));
	glm::mat4 trumpScale = glm::scale(glm::mat4(), glm::vec3(0.045, 0.045, 0.045));
	glm::mat4 chickenScale = glm::scale(glm::mat4(), glm::vec3(0.006, 0.006, 0.006));

	//COLORS:
	glm::vec4 cabinColor = { 0.5f,1.f,1.f,1.f };
	glm::vec4 wheelColor = { 1.f,0.f,0.f,1.f };
	glm::vec4 feetColor = { 0.f,1.f,0.f,1.f };
	glm::vec4 trumpColor = { 0.f,0.f,1.f,1.f };
	glm::vec4 chickenColor = { 1.f,1.f,0.f,1.f };

	//TRANSLATIONS
	glm::vec3 trumpDisplacement = glm::vec3(-0.2f, -0.58f, 0.f);
	glm::vec3 chickenDisplacement = glm::vec3(0.2f, -0.58f, 0.f);

	//ROTATE MATRICES
	glm::mat4 trumpRotate = glm::rotate(glm::mat4(), glm::radians(90.f), glm::vec3(0, 1, 0));
	glm::mat4 chickenRotate = glm::rotate(glm::mat4(), glm::radians(-90.f), glm::vec3(0, 1, 0));

	//CABINS
	bool cabin1 = true;
	int cabinNum = 1;
	glm::vec3 cabinPos[Ncubes];
	glm::vec3 cabinPos2[Ncubes];
#pragma endregion

	//SHADERS
#pragma region shader
	const char* model_vertShader =
		"#version 330\n\
		uniform int numExercise;\n\
		in vec3 in_Position;\n\
		in vec3 in_Normal;\n\
		out vec4 vert_Normal;\n\
		uniform mat4 objMat;\n\
		uniform mat4 mv_Mat;\n\
		uniform mat4 mvpMat;\n\
		out vec3 sunDir;\n\
		out vec3 moonDir;\n\
		out vec3 bulbDir;\n\
		uniform vec3 sunPos;\n\
		uniform vec3 moonPos;\n\
		uniform vec3 bulbPos;\n\
		\n\
		void main() {\n\
			gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
			vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
			\n\
			if(numExercise >=5){\n\
			sunDir = normalize(sunPos - (objMat * vec4(in_Position, 1.0)).xyz);\n\
			moonDir = normalize(moonPos - (objMat * vec4(in_Position, 1.0)).xyz);\n\
			}\n\
			if(numExercise >=6){\n\
			bulbDir = normalize(bulbPos - (objMat * vec4(in_Position, 1.0)).xyz);\n\
			}\n\
		}";


	const char* model_fragShader =
		"#version 330\n\
		in vec4 vert_Normal;\n\
		in vec3 sunDir;\n\
		in vec3 moonDir;\n\
		in vec3 bulbDir;\n\
		out vec4 out_Color;\n\
		uniform int numExercise;\n\
		uniform mat4 mv_Mat;\n\
		uniform bool dayNight;\n\
		uniform int lightBulbMode;\n\
		uniform int toonShader;\n\
		uniform float time;\n\
		uniform vec4 color;\n\
		uniform bool contour;\n\
		vec4 ambientColor;\n\
		vec4 moonColor = vec4(102.0/255.0, 160.0/255.0, 255.0/255.0, 0.0);\n\
		vec4 sunColor;\n\
		vec4 lightBulbColor = vec4(117.0/255.0, 1.0, 155.0/255.0, 0.0);\n\
		vec4 contourColor = vec4(220.0/255.0, 0.0, 1.0, 0.0);\n\
		float U;\n\
		out vec4 FragColor;\n\
		float near = 0.1;\n\
		float far= 1000.0f;\n\
		float linearizeDepth(float depth)\n\
		{\n\
			float z = depth*2.0-1.0;\n\
			return(2.0*near*far)/ (far+near - z* ( far - near));\n\
		}\n\
		\n\
		void toonShading()\n\
		{\n\
			//float radiantPower = 1.0;//phi\n\
			//float reflectionCoefficient = 1.0;//kd\n\
			\n\
			//U = reflectionCoefficient * dot(n, l)*radiantPower / (4.f * glm::pi<float>()*d*d);\n\
			\n\
			if (U < 0.2)\n\
				U = 0.0;\n\
			else if (U >= 0.2 && U < 0.4)\n\
				U = 0.2;\n\
			else if (U >= 0.4 && U < 0.5)\n\
				U = 0.4;\n\
			else if (U >= 0.5)\n\
				U = 1.0;\n\
		}\n\
		\n\
		void main() {\n\
			out_Color = 0.4 * vec4(color.xyz * dot(normalize(vert_Normal), mv_Mat*vec4(0.0, 1.0, 0.0, 0.0)) + color.xyz * 0.3, 1.0 );\n\
			if(numExercise >= 5)\n\
			{\n\
				if(lightBulbMode > 1){\n\
					U = dot(normalize(vert_Normal), mv_Mat*vec4(bulbDir.x, bulbDir.y, bulbDir.z, 0.0));\n\
					out_Color += 1.3 * vec4(lightBulbColor.xyz * U, 1.0 );\n\
				}\n\
				if(!dayNight){\n\
					ambientColor = vec4(8.0/255.0, 8.0/255.0, 135.0/255.0, 0.0);\n\
					U = dot(normalize(vert_Normal), mv_Mat*vec4(moonDir.x, moonDir.y, moonDir.z, 0.0));\n\
					out_Color += 0.5 * vec4(moonColor.xyz * U + ambientColor.xyz * 0.3, 1.0 );\n\
				} else{\n\
					U = dot(normalize(vert_Normal), mv_Mat*vec4(moonDir.x, moonDir.y, moonDir.z, 0.0));\n\
					out_Color += 0.5 * vec4(moonColor.xyz * U, 1.0 );\n\
					if((int(time) % 20 <= 12 && int(time) % 20 >= 10) || (int(time) % 20 >= 18 && int(time) % 20 <= 20)){\n\
						sunColor = vec4(237.0/255.0, 82.0/255.0, 21.0/255.0, 0.0);\n\
						ambientColor = vec4(0.0/255.0, 0.0/255.0, 0.0/255.0, 0.0);\n\
					} else if(int(time) % 20 > 12 && int(time) % 20 < 18){\n\
						sunColor = vec4(239.0/255.0, 255.0/255.0, 96.0/255.0, 0.0);\n\
						ambientColor = vec4(0.0/255.0, 0.0/255.0, 0.0/255.0, 0.0);\n\
					} else if(int(time) % 20 > 0 && int(time) % 20 < 10){\n\
						sunColor = vec4(0.0/255.0, 0.0/255.0, 0.0/255.0, 0.0);\n\
						ambientColor = vec4(29.0/255.0, 76.0/255.0, 153.0/255.0, 0.0);\n\
					}\n\
					U = dot(normalize(vert_Normal), mv_Mat*vec4(sunDir.x, sunDir.y, sunDir.z, 0.0));\n\
					out_Color += 0.6 * vec4(sunColor.xyz * U + ambientColor.xyz * 0.2, 1.0 );\n\
				}\n\
			}\n\
			if(numExercise >= 9)\n\
			{\n\
				if(toonShader == 1)\n\
				{\n\
					U = dot(normalize(vert_Normal), mv_Mat*vec4(sunDir.x, sunDir.y, sunDir.z, 0.0));\n\
					toonShading();\n\
					out_Color += 0.6 * vec4(sunColor.xyz * U + ambientColor.xyz * 0.2, 1.0);\n\
				}\n\
				if(toonShader == 2)\n\
				{\n\
					U = dot(normalize(vert_Normal), mv_Mat*vec4(sunDir.x, sunDir.y, sunDir.z, 0.0));\n\
					toonShading();\n\
					out_Color += vec4(sunColor.xyz * U + ambientColor.xyz * 0.2, 1.0);\n\
					U = dot(normalize(vert_Normal), mv_Mat*vec4(moonDir.x, moonDir.y, moonDir.z, 0.0));\n\
					toonShading();\n\
					out_Color += 0.6 * vec4(moonColor.xyz * U, 1.0);\n\
				}\n\
				if(toonShader == 3)\n\
				{\n\
					U = dot(normalize(vert_Normal), mv_Mat*vec4(moonDir.x, moonDir.y, moonDir.z, 0.0));\n\
					toonShading();\n\
					out_Color += 0.6 * vec4(moonColor.xyz * U, 1.0);\n\
					U = dot(normalize(vert_Normal), mv_Mat*vec4(bulbDir.x, bulbDir.y, bulbDir.z, 0.0));\n\
					toonShading();\n\
					out_Color += 2.0 * vec4(lightBulbColor.xyz * U, 1.0);\n\
				}\n\
			}\n\
			if(numExercise >= 12)\n\
			{\n\
				if(contour)\n\
				{\n\
					out_Color = vec4(contourColor.xyz * dot(normalize(vert_Normal), mv_Mat*vec4(0.0, 1.0, 0.0, 0.0)) + contourColor.xyz * 0.3, 1.0 );\n\
				}\n\
			}\n\
		}";

	const char* stencil_fragShader =
		"#version 330\n\
			\n\
			int main()\n\
			{\n\
				\n\
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
		glGenBuffers(3, vbo);//2?

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

		//NORMALIZE
		//normals = glm::normalize(normals);
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
				trumpPosition = cabinPosition1 + trumpDisplacement;
				updateModel(trumpObjMat, glm::translate(glm::mat4(), trumpPosition) * trumpRotate * trumpScale);

				//CHICKEN
				chickenPosition = cabinPosition1 + chickenDisplacement;
				updateModel(chickenObjMat, glm::translate(glm::mat4(), chickenPosition) * chickenRotate * chickenScale);;
	}

	void drawModels(double time)
	{

		drawSpecificModel(trumpVao, trumpObjMat, trumpVertices, trumpColor, time);
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

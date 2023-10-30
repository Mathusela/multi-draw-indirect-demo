#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <random>

#include <extern/glad/glad.h>
#include <extern/GLFW/glfw3.h>
#include <extern/glm/glm.hpp>
#include <extern/glm/gtc/type_ptr.hpp>

constexpr const unsigned int WIDTH = 1400;
constexpr const unsigned int HEIGHT = 900;
const std::string TITLE = "OpenGL 4 Testing";

// ========================================
// Shader

std::string loadShaderSource(std::string inputPath) {
    std::ifstream input(inputPath);
	if (!input) {
		std::cout << "Failed to import shader source from file: " + inputPath + "\n";
	}

    std::string inputText = "";

    for (std::string line; getline(input, line);) {
        inputText = inputText + "\n" + line;
    }

    inputText += "\0";

    input.close();
    input.clear();

    return inputText;
}

class Shader {
	void getCompilationErrors(unsigned int shader, std::string type) {
		int success;
		char infoLog[512];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shader, 512, NULL, infoLog);
			std::cout << type + " shader compilation failed\n" + std::string(infoLog);
    	}
	}

public:
	unsigned int id;

	Shader() {}

	Shader(std::string vertexSource, std::string fragmentSource) {
		auto vs = vertexSource.c_str();
		auto fs = fragmentSource.c_str();

		unsigned int vert = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vert, 1, &vs, NULL);
		glCompileShader(vert);
		getCompilationErrors(vert, "Vertex");

		unsigned int frag = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag, 1, &fs, NULL);
		glCompileShader(frag);
		getCompilationErrors(frag, "Fragment");
		
		id = glCreateProgram();
		glAttachShader(id, vert);
		glAttachShader(id, frag);
		glLinkProgram(id);

		glDeleteShader(frag);
		glDeleteShader(vert);
	}

	void bind() {
		glUseProgram(id);
	} 

	static void unbind() {
		glUseProgram(NULL);
	}

	void setUniform(std::string location, int x);
	void setUniform(std::string location, float x);
	void setUniform(std::string location, glm::vec2 x);
	void setUniform(std::string location, glm::vec3 x);
	void setUniform(std::string location, glm::mat3 x);
	void setUniform(std::string location, glm::mat4 x);
};

void Shader::setUniform(std::string location, int x) {
	glUniform1i(glGetUniformLocation(id, location.c_str()), x);
}
void Shader::setUniform(std::string location, float x) {
	glUniform1f(glGetUniformLocation(id, location.c_str()), x);
}
void Shader::setUniform(std::string location, glm::vec2 x) {
	glUniform2fv(glGetUniformLocation(id, location.c_str()), 1, glm::value_ptr(x));
}
void Shader::setUniform(std::string location, glm::vec3 x) {
	glUniform3fv(glGetUniformLocation(id, location.c_str()), 1, glm::value_ptr(x));
}
void Shader::setUniform(std::string location, glm::mat3 x) {
	glUniformMatrix3fv(glGetUniformLocation(id, location.c_str()), 1, GL_FALSE, glm::value_ptr(x));
}
void Shader::setUniform(std::string location, glm::mat4 x) {
	glUniformMatrix4fv(glGetUniformLocation(id, location.c_str()), 1, GL_FALSE, glm::value_ptr(x));
}

// ========================================
// Camera

class Camera {
	float fov;
	glm::vec3 position;
	glm::vec3 forward;
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	
	// const glm::vec3 correctCoords {-1.0, 1.0, 1.0};

	void updateViewMatrix() {
		// viewMatrix = glm::lookAt(position*correctCoords, (position+forward)*correctCoords, {0.0, 1.0, 0.0});
		viewMatrix = glm::lookAt(position, position+forward, {0.0, 1.0, 0.0});
	}

public:
	glm::mat4 getViewMatrix() {
		return viewMatrix;
	}

	glm::mat4 getProjectionMatrix() {
		return projectionMatrix;
	}

	glm::vec3 getPosition() {
		return position;
	}

	void setPosition(glm::vec3 newPosition) {
		position = newPosition;
		updateViewMatrix();
	}

	glm::vec3 getForward() {
		return forward;
	}

	void setForward(glm::vec3 newForward) {
		forward = glm::normalize(newForward);
		updateViewMatrix();
	}

	Camera(glm::vec3 position, glm::vec3 forward, float fov, float near, float far) {
		setPosition(position);
		setForward(forward);
		projectionMatrix = glm::perspective<float>(fov, (float)WIDTH/HEIGHT, near, far);
	}

};

// ========================================
// Drawable

class Drawable {
protected:
	unsigned int vertexCount;
	std::vector<float> verts;

public:
	Drawable(std::vector<float> verts): verts(verts) {
		vertexCount = (unsigned int)verts.size()/6;
	}

	virtual void draw() = 0;
};

// ========================================
// Object Data

struct ObjectData {
	glm::vec3 position;
	glm::vec3 color;
};

// ========================================
// Object

class Object : public Drawable {
	ObjectData data;

public:
	Object(std::vector<float> verts, ObjectData data): data(data), Drawable(verts) {

	}

	void draw();

	friend void drawObjects(std::vector<Object*> objects, Camera& camera);
};

// ========================================
// ObjectInstanced

// class ObjectInstanced : public Drawable {
// 	std::vector<ObjectData> data;

// public:
// 	ObjectInstanced(std::vector<float> verts): Drawable(verts) {

// 	}

// 	void draw();

// 	friend void drawInstanced(const ObjectInstanced& object, Camera& camera);
// };

// ========================================
// Draw definitions

std::vector<Object*> g_objects {};
// std::vector<ObjectInstanced*> g_instancedObjects {};

void Object::draw() {
	g_objects.push_back(this);
}

// void ObjectInstanced::draw() {
// 	g_instancedObjects.push_back(this);
// }

// ========================================
// Draw indirect command declarations

struct DrawArraysIndirectCommand {
	unsigned int count;
    unsigned int instanceCount;
    unsigned int firstVertex;
    unsigned int baseInstance;
};

// ========================================
// Draw (MDI)

Shader shaderGBuffer;
Shader shaderDeferred;

namespace drawObjectBuffers {
	unsigned int VAO;
	unsigned int VertexBuffer;
	unsigned int UniformsBuffer;
	unsigned int IndirectDrawBuffer;
	
	unsigned int GBuffer;
	unsigned int ScreenQuadVAO;
	unsigned int GBufferRBO;

	unsigned int LightsUBO;
}

namespace drawObjectTextures {
	unsigned int GPosition;
	unsigned int GNormal;
	unsigned int GColor;
}

std::vector<glm::vec3> lightPositions {};
int lightCount = 200;

void attachTextureToFramebuffer(unsigned int FBO, unsigned int Texture, unsigned int InternalFormat, unsigned int DataType, unsigned int attachmentId) {
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glBindTexture(GL_TEXTURE_2D, Texture);

	glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, WIDTH, HEIGHT, 0, GL_RGBA, DataType, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentId, GL_TEXTURE_2D, Texture, 0);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setupDrawObjects() {
	// Draw data buffers
	glGenVertexArrays(1, &drawObjectBuffers::VAO);
	glGenBuffers(1, &drawObjectBuffers::VertexBuffer);
	glGenBuffers(1, &drawObjectBuffers::UniformsBuffer);
	glGenBuffers(1, &drawObjectBuffers::IndirectDrawBuffer);
	
	// Screen quad
	std::vector<float> screenQuadVerts {
		-1.0f, -1.0f, 0.0f,		0.0f, 0.0f,
		1.0f, -1.0f, 0.0f,		1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,		0.0f, 1.0f,

		-1.0f, 1.0f, 0.0f,		0.0f, 1.0f,
		1.0f, -1.0f, 0.0f,		1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,		1.0f, 1.0f
	};
	glGenVertexArrays(1, &drawObjectBuffers::ScreenQuadVAO);
	glBindVertexArray(drawObjectBuffers::ScreenQuadVAO);

	unsigned int ScreenQuadVBO;
	glGenBuffers(1, &ScreenQuadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, ScreenQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * screenQuadVerts.size(), &screenQuadVerts[0], GL_STATIC_DRAW);

	// Positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
	glEnableVertexAttribArray(0);

	// UVs
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float)*3));
	glEnableVertexAttribArray(1);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glDeleteBuffers(1, &ScreenQuadVBO);

	// GBuffer FBO
	glGenFramebuffers(1, &drawObjectBuffers::GBuffer);
	glGenTextures(1, &drawObjectTextures::GPosition);
	glGenTextures(1, &drawObjectTextures::GNormal);
	glGenTextures(1, &drawObjectTextures::GColor);

	attachTextureToFramebuffer(drawObjectBuffers::GBuffer, drawObjectTextures::GPosition, GL_RGBA16F, GL_FLOAT, GL_COLOR_ATTACHMENT0);
	attachTextureToFramebuffer(drawObjectBuffers::GBuffer, drawObjectTextures::GNormal, GL_RGBA16F, GL_FLOAT,  GL_COLOR_ATTACHMENT1);
	attachTextureToFramebuffer(drawObjectBuffers::GBuffer, drawObjectTextures::GColor, GL_RGBA16, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT2);
	
	glBindFramebuffer(GL_FRAMEBUFFER, drawObjectBuffers::GBuffer);
	unsigned int attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, attachments);

	glGenRenderbuffers(1, &drawObjectBuffers::GBufferRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, drawObjectBuffers::GBufferRBO); 
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH, HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, drawObjectBuffers::GBufferRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Light positions
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0, 50*2.0);

	for (int i=0; i<lightCount; i++)
		lightPositions.push_back(glm::vec3(dis(gen), dis(gen), dis(gen)+10));

	// Lights UBO
	// Padded to align with std140 layout rules: https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
	struct LightData {			
		//							Base alignment		aligned offset
		float position[3];		// 	16 					0
		float paddingPos;		// 						(12 [+4])
		float color[3];			//	16 					16
		float power;			//	4 					28
	};

	std::vector<LightData> lights;
	for (int i=0; i<lightCount; i++) {
		auto position = lightPositions[i];
		LightData light {
			{position.x, position.y, position.z},
			0.0f,
			{1.0f, 1.0f, 1.0f},
			1.0f,
		};
		lights.push_back(light);
	}

	glGenBuffers(1, &drawObjectBuffers::LightsUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, drawObjectBuffers::LightsUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightData)*lights.size(), &lights[0], GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, drawObjectBuffers::LightsUBO);
	glUniformBlockBinding(shaderDeferred.id, glGetUniformBlockIndex(shaderDeferred.id, "LightsBlock"), 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// ========================================
// DRAW OPTIONS:
// FIXME: ifndef NO_REGENERATING_DRAW_CALLS only draws tris

#define STATIC_DRAW

#ifdef STATIC_DRAW
#define NO_REGENERATING_DRAW_CALLS
#endif
// ========================================

void drawObjects(std::vector<Object*> objects, Camera& camera) {
	#ifdef STATIC_DRAW
	static bool firstRun = true;
	#endif

	unsigned int VAO = drawObjectBuffers::VAO;
	glBindVertexArray(VAO);

	#ifdef NO_REGENERATING_DRAW_CALLS
	static std::vector<DrawArraysIndirectCommand> drawCommands;
	#else
	std::vector<DrawArraysIndirectCommand> drawCommands;
	#endif

	std::vector<float> verts;
	unsigned int numVerts = 0;
	std::vector<float> uniforms;
	
	unsigned int count = 0;
	for (auto object : objects) {
		#ifdef NO_REGENERATING_DRAW_CALLS
		if (firstRun) {
		#endif

		// Generate draw calls
		DrawArraysIndirectCommand command {};
		command.count = object->vertexCount;
		command.instanceCount = 1;
		command.firstVertex = numVerts;
		command.baseInstance = count++;		// Offset so you call the right data from buffers with divisor (instance [1] / divisor [1]) + baseInstance)
		drawCommands.push_back(command);

		#ifndef NO_REGENERATING_DRAW_CALLS
		#ifdef STATIC_DRAW
		if (firstRun) {
		#endif
		#endif

		// Aggregate data
		numVerts += object->vertexCount;
		for (auto num : object->verts) verts.push_back(num);
		auto pos = object->data.position;
		auto col = object->data.color;
		uniforms.push_back(pos.x); uniforms.push_back(pos.y); uniforms.push_back(pos.z);
		uniforms.push_back(col.x); uniforms.push_back(col.y); uniforms.push_back(col.z);

		#ifdef STATIC_DRAW
		}	// firstRun
		#endif
	}

	#ifdef STATIC_DRAW
	if (firstRun) {
	#endif

	// Verts
	unsigned int VertexBuffer = drawObjectBuffers::VertexBuffer;
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verts.size(), &verts[0], GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);

	// Uniforms
	unsigned int UniformsBuffer = drawObjectBuffers::UniformsBuffer;
	glBindBuffer(GL_ARRAY_BUFFER, UniformsBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * uniforms.size(), &uniforms[0], GL_DYNAMIC_DRAW);

	// Positions
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);	// Change once per instance / draw call

	// Color
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	#ifdef STATIC_DRAW
	} // firstRun
	firstRun = false;
	#endif

	// Draw calls (MDI)
	unsigned int IndirectDrawBuffer = drawObjectBuffers::IndirectDrawBuffer;
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, IndirectDrawBuffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand) * drawCommands.size(), &drawCommands[0], GL_DYNAMIC_DRAW);

	// Uniforms (object unspecific)
	shaderGBuffer.setUniform("viewMatrix", camera.getViewMatrix());
	shaderGBuffer.setUniform("projectionMatrix", camera.getProjectionMatrix());

	// GBuffer
	unsigned int GBuffer = drawObjectBuffers::GBuffer;
	glBindFramebuffer(GL_FRAMEBUFFER, GBuffer);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//  Draw
	glMultiDrawArraysIndirect(GL_TRIANGLES, (const void*)0, drawCommands.size(), 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	shaderDeferred.bind();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, drawObjectTextures::GPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, drawObjectTextures::GNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, drawObjectTextures::GColor);
	
	shaderDeferred.setUniform("positionTexture", 0);
	shaderDeferred.setUniform("normalTexture", 1);
	shaderDeferred.setUniform("colorTexture", 2);

	// Uniforms (object unspecific)
	shaderDeferred.setUniform("cameraPos", camera.getPosition());
	
	shaderDeferred.setUniform("lightCount", lightCount);

	// Draw
	glBindVertexArray(drawObjectBuffers::ScreenQuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	shaderDeferred.unbind();

	// Cleanup
	glBindVertexArray(0);
}

// ========================================
// Draw instanced

// // TODO: Unimplemented
// void drawInstanced(const ObjectInstanced& object, Camera& camera) {
// 	auto instances = object.data;

// 	// glDrawArraysInstanced()
// }

// ========================================
// Draw objects dispatched to be rendered

Camera mainCamera({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 45.0, 0.1, 300.0);

void drawDispatched() {
	shaderGBuffer.bind();

	drawObjects(g_objects, mainCamera);
	// for (auto iObject : g_instancedObjects) {
	// 	drawInstanced(*iObject, mainCamera);
	// }

	shaderGBuffer.unbind();

	g_objects.clear();
	// g_instancedObjects.clear();
}

// ========================================
// Camera controller

void cameraController(Camera& camera, GLFWwindow* window, double deltaTime) {
	glm::vec3 offset = {0.0, 0.0, 0.0};

	float speed;
	if (glfwGetKey(window, GLFW_KEY_F)) speed = 8.0f * deltaTime;
	else speed = 30.0f * deltaTime;
	
	glm::vec3 forwardNoVertRot = glm::normalize(camera.getForward() * glm::vec3(1.0, 0.0, 1.0));
	if (glfwGetKey(window, GLFW_KEY_W)) offset += forwardNoVertRot;
	if (glfwGetKey(window, GLFW_KEY_S)) offset += -forwardNoVertRot;
	glm::vec3 up = {0.0, 1.0, 0.0};
	glm::vec3 right = glm::cross(camera.getForward(), up);
	if (glfwGetKey(window, GLFW_KEY_A)) offset += -right;
	if (glfwGetKey(window, GLFW_KEY_D)) offset += right;
	
	if (glfwGetKey(window, GLFW_KEY_SPACE)) offset += up;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) offset += -up;

	auto newPos = camera.getPosition() + (offset.x*offset.x + offset.y*offset.y + offset.z*offset.z != 0 ? glm::normalize(offset) : offset)*speed;
	camera.setPosition(newPos);

	// FIXME: some rotations dont work when 2 movement buttons are pressed e.g. space and shift stop up and down
	float rotSpeed = 1.7f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_LEFT)) camera.setForward(glm::normalize(camera.getForward() + right*-rotSpeed));
	if (glfwGetKey(window, GLFW_KEY_RIGHT)) camera.setForward(glm::normalize(camera.getForward() + right*rotSpeed));
	if (glfwGetKey(window, GLFW_KEY_UP)) camera.setForward(glm::normalize(camera.getForward() + up*rotSpeed));
	if (glfwGetKey(window, GLFW_KEY_DOWN)) camera.setForward(glm::normalize(camera.getForward() + up*-rotSpeed));
}

// ========================================
// Main

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto window = glfwCreateWindow(WIDTH, HEIGHT, TITLE.c_str(), NULL, NULL);

	glfwMakeContextCurrent(window);
	// VSYNC OFF
	glfwSwapInterval(0);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// shaderGBuffer = Shader(loadShaderSource("../../resources/shaders/gBuffer.vs"), loadShaderSource("../../resources/shaders/gBuffer.fs"));
	// shaderDeferred = Shader(loadShaderSource("../../resources/shaders/deferred.vs"), loadShaderSource("../../resources/shaders/deferred.fs"));

	shaderGBuffer = Shader(loadShaderSource("D:/Programming/C++/code/OpenGL4Testing/resources/shaders/gBuffer.vs"), loadShaderSource("D:/Programming/C++/code/OpenGL4Testing/resources/shaders/gBuffer.fs"));
	shaderDeferred = Shader(loadShaderSource("D:/Programming/C++/code/OpenGL4Testing/resources/shaders/deferred.vs"), loadShaderSource("D:/Programming/C++/code/OpenGL4Testing/resources/shaders/deferred.fs"));


	// ========================================
	// Setup

	std::vector<float> tri {
		// Front
		-0.5f, -0.5f, -0.5f,	0.0f, 0.5f, -1.0f,
		0.0f, 0.5f, 0.0f,		0.0f, 0.5f, -1.0f,
		0.5f, -0.5f, -0.5f,		0.0f, 0.5f, -1.0f,

		// Back
		-0.5f, -0.5f, 0.5f,		0.0f, -0.5f, 1.0f,
		0.5f, -0.5f, 0.5f,		0.0f, -0.5f, 1.0f,
		0.0f, 0.5f, 0.0f,		0.0f, -0.5f, 1.0f,

		// Right
		-0.5f, -0.5f, -0.5f,	-1.0f, 0.5f, 0.0f,
		-0.5f, -0.5f, 0.5f,		-1.0f, 0.5f, 0.0f,
		0.0f, 0.5f, 0.0f,		-1.0f, 0.5f, 0.0f,

		// Left
		0.5f, -0.5f, -0.5f,		1.0f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f,		1.0f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.5f,		1.0f, -0.5f, 0.0f,

		// Bottom
		-0.5f, -0.5f, -0.5f,	0.0f, -1.0f, 0.0f,
		0.5f, -0.5f, -0.5f,		0.0f, -1.0f, 0.0f,
		-0.5f, -0.5f, 0.5f,		0.0f, -1.0f, 0.0f,

		0.5f, -0.5f, -0.5f,		0.0f, -1.0f, 0.0f,
		0.5f, -0.5f, 0.5f,		0.0f, -1.0f, 0.0f,
		-0.5f, -0.5f, 0.5f,		0.0f, -1.0f, 0.0f
	};

	std::vector<float> quad {
		// Front
		-0.5f, -0.5f, -0.5f,	0.0f, 0.0f, -1.0f,
		-0.5f, 0.5f, -0.5f,		0.0f, 0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,		0.0f, 0.0f, -1.0f,

		-0.5f, 0.5f, -0.5f,		0.0f, 0.0f, -1.0f,
		0.5f, 0.5f, -0.5f,		0.0f, 0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,		0.0f, 0.0f, -1.0f,

		// Back
		-0.5f, -0.5f, 0.5f,		0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.5f,		0.0f, 0.0f, 1.0f,
		-0.5f, 0.5f, 0.5f,		0.0f, 0.0f, 1.0f,

		-0.5f, 0.5f, 0.5f,		0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.5f,		0.0f, 0.0f, 1.0f,
		0.5f, 0.5f, 0.5f,		0.0f, 0.0f, 1.0f,

		// Right
		-0.5f, -0.5f, -0.5f,	-1.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, 0.5f,		-1.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, -0.5f,		-1.0f, 0.0f, 0.0f,

		-0.5f, 0.5f, -0.5f,		-1.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, 0.5f,		-1.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, 0.5f,		-1.0f, 0.0f, 0.0f,

		// Left
		0.5f, -0.5f, -0.5f,		1.0f, 0.0f, 0.0f,
		0.5f, 0.5f, -0.5f,		1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.5f,		1.0f, 0.0f, 0.0f,

		0.5f, 0.5f, -0.5f,		1.0f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.5f,		1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.5f,		1.0f, 0.0f, 0.0f,

		// Top
		-0.5f, 0.5f, -0.5f,		0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.5f,		0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f,		0.0f, 1.0f, 0.0f,

		0.5f, 0.5f, -0.5f,		0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.5f,		0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f,		0.0f, 1.0f, 0.0f,

		// Bottom
		-0.5f, -0.5f, -0.5f,	0.0f, -1.0f, 0.0f,
		0.5f, -0.5f, -0.5f,		0.0f, -1.0f, 0.0f,
		-0.5f, -0.5f, 0.5f,		0.0f, -1.0f, 0.0f,

		0.5f, -0.5f, -0.5f,		0.0f, -1.0f, 0.0f,
		0.5f, -0.5f, 0.5f,		0.0f, -1.0f, 0.0f,
		-0.5f, -0.5f, 0.5f,		0.0f, -1.0f, 0.0f
	};

	std::vector<Object*> objects = {};
	unsigned int x, y, z;
	float spread = 2.0;
	// float spread = 3.0;
	x = 50; y = 50; z = 50;
	for (int i=0; i<x; i++)
		for (int j=0; j<y; j++)
			for (int k=0; k<z; k++) {
				auto color = glm::vec3(i%2 == 0, j%2 == 0, k%2 == 0)*glm::vec3(0.7, 0.7, 0.7) + glm::vec3(0.3, 0.3, 0.3);
				objects.push_back(new Object(i%2 == 0 ? tri : quad, {{i*spread, j*spread, k*spread + 10.0}, color}));
			}

	// Object obj1(tri, {glm::vec3(-0.5, 0.0, 5.0), glm::vec3(1.0, 0.0, 0.0)});
	// Object obj2(quad, {glm::vec3(0.5, 0.0, 5.0), glm::vec3(0.0, 1.0, 0.0)});

	// ========================================

	setupDrawObjects();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	double deltaTime = 0.0;
	double prevTime = 0.0;

	while (!glfwWindowShouldClose(window)) {
		auto currentTime = glfwGetTime();
		deltaTime = currentTime - prevTime;
		prevTime = currentTime;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cameraController(mainCamera, window, deltaTime);

		// ========================================
		// Draw

		// obj1.draw();
		// obj2.draw();
		for (auto object : objects) object->draw();
		drawDispatched();

		// ========================================

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// ========================================
	// Cleanup

	for (auto object : objects) delete object;

	unsigned int buffers[] = {drawObjectBuffers::VertexBuffer, drawObjectBuffers::UniformsBuffer, drawObjectBuffers::IndirectDrawBuffer, drawObjectBuffers::LightsUBO};
	glDeleteBuffers(4, buffers);
	unsigned int vertexArrays[] = {drawObjectBuffers::VAO, drawObjectBuffers::ScreenQuadVAO};
	glDeleteVertexArrays(2, vertexArrays);
	glDeleteFramebuffers(1, &drawObjectBuffers::GBuffer);
	glDeleteRenderbuffers(1, &drawObjectBuffers::GBufferRBO);

	// ========================================

	return 0;
}
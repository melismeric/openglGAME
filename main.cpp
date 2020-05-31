#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "camera.h"
#include "model.h"
#include "AI.h"
#include <time.h>
#include<chrono>

#include "GLTexture.h"
#include "ImageLoader.h"
#include <random>
#include <vector>
#include <string>
#include<windows.h>

#include <iostream>

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H



struct Character {
	GLuint TextureID;   // ID handle of the glyph texture
	glm::ivec2 Size;    // Size of glyph
	glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
	GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
GLuint VAOText, VBOText;

void RenderText(Shader &shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void renderScene(const Shader &shader, GLTexture monster);
void renderCube();
void renderQuad();
glm::vec3 workThatAI(int random);

int velocities[8] = { 0.5, 0.2, 1, 0.4, 0.2, 0.3, 0.7, 0.1 };

// THE KEY :)

bool showTheKey = true;

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(1.0f, 0.0f, 5.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

//AI
AI ourAI(glm::vec3(9.0f, 0.0f, -10.0f));

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// meshes
unsigned int planeVAO;

glm::vec3 lightPos(1.0f, 2.0f, 2.0f);

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef _APPLE_
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	// Set OpenGL options
	//glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);
	// Compile and setup the shader
	Shader textShader("shader-text.vs", "shader-text.frag");
	glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(SCR_WIDTH), 0.0f, static_cast<GLfloat>(SCR_HEIGHT));
	textShader.use();
	glUniformMatrix4fv(glGetUniformLocation(textShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	// FreeType
	FT_Library ft;
	// All functions return a value different than 0 whenever an error occurred
	if (FT_Init_FreeType(&ft))
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

	// Load font as face
	FT_Face face;
	if (FT_New_Face(ft, "arial.ttf", 0, &face))
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

	// Set size to load glyphs as
	FT_Set_Pixel_Sizes(face, 0, 48);

	// Disable byte-alignment restriction
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load first 128 characters of ASCII set
	for (GLubyte c = 0; c < 128; c++)
	{
		// Load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// Generate texture
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.insert(std::pair<GLchar, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	// Destroy FreeType once we're finished
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// Configure VAO/VBO for texture quads
	glGenVertexArrays(1, &VAOText);
	glGenBuffers(1, &VBOText);
	glBindVertexArray(VAOText);
	glBindBuffer(GL_ARRAY_BUFFER, VBOText);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	// build and compile shaders
	// -------------------------
	Shader lightingShader("2.2.basic_lighting.vs", "2.2.basic_lighting.fs");
	//Shader simpleShader("6.1.cubemaps.vs", "6.1.cubemaps.fs");
	Shader shader("3.1.3.shadow_mapping.vs", "3.1.3.shadow_mapping.fs");
	Shader simpleDepthShader("3.1.3.shadow_mapping_depth.vs", "3.1.3.shadow_mapping_depth.fs");
	Shader debugDepthQuad("3.1.3.debug_quad.vs", "3.1.3.debug_quad_depth.fs");
	Shader lampShader("2.2.lamp.vs", "2.2.lamp.fs");


	float vertices[] = {
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

	   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
	   -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
	   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

	   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

	   -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	   -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

	   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	   -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};

	// positions all containers
	glm::vec3 cubePositions[] = {
		//front wall
		glm::vec3(0.0f,  0.0f,  0.0f),
		glm::vec3(2.0f,  0.0f,  0.0f),
		glm::vec3(3.0f,  0.0f,  0.0f),
		glm::vec3(4.0f,  0.0f,  0.0f),
		glm::vec3(5.0f,  0.0f,  0.0f),
		glm::vec3(6.0f,  0.0f,  0.0f),
		glm::vec3(7.0f,  0.0f,  0.0f),
		glm::vec3(8.0f,  0.0f,  0.0f),
		glm::vec3(9.0f,  0.0f,  0.0f),
		glm::vec3(10.0f,  0.0f,  0.0f),
		glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3(-2.0f,  0.0f,  0.0f),
		glm::vec3(-3.0f,  0.0f,  0.0f),
		glm::vec3(-4.0f,  0.0f,  0.0f),
		glm::vec3(-5.0f,  0.0f,  0.0f),
		glm::vec3(-6.0f,  0.0f,  0.0f),
		glm::vec3(-7.0f,  0.0f,  0.0f),
		glm::vec3(-8.0f,  0.0f,  0.0f),
		glm::vec3(-9.0f,  0.0f,  0.0f),
		glm::vec3(-10.0f,  0.0f,  0.0f),

		//left wall
		glm::vec3(-10.0f,  0.0f,  -1.0f),
		glm::vec3(-10.0f,  0.0f,  -2.0f),
		glm::vec3(-10.0f,  0.0f,  -3.0f),
		glm::vec3(-10.0f,  0.0f,  -4.0f),
		glm::vec3(-10.0f,  0.0f,  -5.0f),
		glm::vec3(-10.0f,  0.0f,  -6.0f),
		glm::vec3(-10.0f,  0.0f,  -7.0f),
		glm::vec3(-10.0f,  0.0f,  -8.0f),
		glm::vec3(-10.0f,  0.0f,  -9.0f),
		glm::vec3(-10.0f,  0.0f,  -10.0f),
		//back wall
		glm::vec3(1.0f,  0.0f,  -10.0f),
		glm::vec3(2.0f,  0.0f,  -10.0f),
		glm::vec3(3.0f,  0.0f,  -10.0f),
		glm::vec3(4.0f,  0.0f,  -10.0f),
		glm::vec3(5.0f,  0.0f,  -10.0f),
		glm::vec3(6.0f,  0.0f,  -10.0f),
		glm::vec3(7.0f,  0.0f,  -10.0f),
		glm::vec3(8.0f,  0.0f,  -10.0f),
		glm::vec3(0.0f,  0.0f,  -10.0f),
		glm::vec3(10.0f,  0.0f,  -10.0f),
		glm::vec3(-1.0f,  0.0f,  -10.0f),
		glm::vec3(-2.0f,  0.0f,  -10.0f),
		glm::vec3(-3.0f,  0.0f,  -10.0f),
		glm::vec3(-4.0f,  0.0f,  -10.0f),
		glm::vec3(-5.0f,  0.0f,  -10.0f),
		glm::vec3(-6.0f,  0.0f,  -10.0f),
		glm::vec3(-7.0f,  0.0f,  -10.0f),
		glm::vec3(-8.0f,  0.0f,  -10.0f),
		glm::vec3(-9.0f,  0.0f,  -10.0f),

		//right wall
		glm::vec3(10.0f,  0.0f,  -1.0f),
		glm::vec3(10.0f,  0.0f,  -2.0f),
		glm::vec3(10.0f,  0.0f,  -3.0f),
		glm::vec3(10.0f,  0.0f,  -4.0f),
		glm::vec3(10.0f,  0.0f,  -5.0f),
		glm::vec3(10.0f,  0.0f,  -6.0f),
		glm::vec3(10.0f,  0.0f,  -7.0f),
		glm::vec3(10.0f,  0.0f,  -8.0f),
		glm::vec3(10.0f,  0.0f,  -9.0f),
		glm::vec3(10.0f,  0.0f,  -10.0f),

		//labyrinth Rigth side
		glm::vec3(2.0f,  0.0f,  -1.0f),
		glm::vec3(2.0f,  0.0f,  -3.0f),
		glm::vec3(2.0f,  0.0f,  -4.0f),
		glm::vec3(2.0f,  0.0f,  -5.0f),
		glm::vec3(2.0f,  0.0f,  -7.0f),
		glm::vec3(2.0f,  0.0f,  -8.0f),

		glm::vec3(2.0f,  0.0f,  -6.0f),
		glm::vec3(4.0f,  0.0f,  -6.0f),
		glm::vec3(4.0f,  0.0f,  -7.0f),
		glm::vec3(5.0f,  0.0f,  -7.0f),
		glm::vec3(5.0f,  0.0f,  -8.0f),
		glm::vec3(5.0f,  0.0f,  -9.0f),
		glm::vec3(5.0f,  0.0f,  -10.0f),

		glm::vec3(8.0f,  0.0f,  -9.0f),
		glm::vec3(8.0f,  0.0f,  -8.0f),
		glm::vec3(7.0f,  0.0f,  -8.0f),
		glm::vec3(7.0f,  0.0f,  -7.0f),
		glm::vec3(7.0f,  0.0f,  -6.0f),
		glm::vec3(8.0f,  0.0f,  -4.0f),
		glm::vec3(9.0f,  0.0f,  -6.0f),
		glm::vec3(9.0f,  0.0f,  -3.0f),
		glm::vec3(8.0f,  0.0f,  -3.0f),

		glm::vec3(4.0f,  0.0f,  -2.0f),
		glm::vec3(5.0f,  0.0f,  -2.0f),
		glm::vec3(4.0f,  0.0f,  -3.0f),
		glm::vec3(4.0f,  0.0f,  -4.0f),

		glm::vec3(8.0f,  0.0f,  -2.0f),
		glm::vec3(7.0f,  0.0f,  -2.0f),
		glm::vec3(7.0f,  0.0f,  -4.0f),
		glm::vec3(6.0f,  0.0f,  -4.0f),

		//labyrinth left side
		glm::vec3(0.0f, 0.0f, -2.0f),
		glm::vec3(-1.0f, 0.0f, -2.0f),
		glm::vec3(-2.0f, 0.0f, -2.0f),
		glm::vec3(-3.0f, 0.0f, -2.0f),
		glm::vec3(-3.0f, 0.0f, -3.0f),
		glm::vec3(-3.0f, 0.0f, -4.0f),

		glm::vec3(-5.0f, 0.0f, -2.0f),
		glm::vec3(-6.0f, 0.0f, -2.0f),
		glm::vec3(-7.0f, 0.0f, -2.0f),
		glm::vec3(-7.0f, 0.0f, -3.0f),
		glm::vec3(-7.0f, 0.0f, -4.0f),
		glm::vec3(-7.0f, 0.0f, -5.0f),
		glm::vec3(-4.0f, 0.0f, -4.0f),
		glm::vec3(-1.0f, 0.0f, -3.0f),
		glm::vec3(-1.0f, 0.0f, -4.0f),
		//glm::vec3(-1.0f, 0.0f, -5.0f),
		glm::vec3(-1.0f, 0.0f, -6.0f),
		glm::vec3(-7.0f, 0.0f, -6.0f),

		glm::vec3(-5.0f, 0.0f, -4.0f),
		//glm::vec3(-2.0f, 0.0f, -6.0f),
		glm::vec3(-3.0f, 0.0f, -6.0f),
		glm::vec3(-4.0f, 0.0f, -6.0f),
		glm::vec3(-8.0f, 0.0f, -2.0f),
		glm::vec3(-6.0f, 0.0f, -6.0f),
		glm::vec3(-9.0f, 0.0f, -4.0f),
		glm::vec3(-8.0f, 0.0f, -6.0f),
		glm::vec3(-8.0f, 0.0f, -8.0f),
		glm::vec3(-9.0f, 0.0f, -8.0f),
		glm::vec3(-6.0f, 0.0f, -8.0f),
		glm::vec3(-6.0f, 0.0f, -9.0f),

		glm::vec3(-4.0f, 0.0f, -8.0f),
		glm::vec3(-4.0f, 0.0f, -7.0f),

		//glm::vec3(0.0f, 0.0f, -3.0f),
		glm::vec3(0.0f, 0.0f, -4.0f),
		glm::vec3(0.0f, 0.0f, -6.0f),
		glm::vec3(0.0f, 0.0f, -7.0f),
		glm::vec3(1.0f, 0.0f, -6.0f),
		glm::vec3(-2.0f, 0.0f, -9.0f),
		glm::vec3(-2.0f, 0.0f, -8.0f),

		//second layer
		glm::vec3(-2.0f, 1.0f, -1.0f),
		glm::vec3(-1.0f, 1.0f,  -1.0f),
		glm::vec3(0.0f, 1.0f, -1.0f),
		glm::vec3(-3.0f, 1.0f, -1.0f),
		glm::vec3(-5.0f, 1.0f, -1.0f),
		glm::vec3(-6.0f, 1.0f, -1.0f),
		glm::vec3(-7.0f, 1.0f, -1.0f),
		glm::vec3(-8.0f, 1.0f, -1.0f),
		glm::vec3(-8.0f, 1.0f, -5.0f),
		glm::vec3(-9.0f, 1.0f, -5.0f),
		glm::vec3(-9.0f, 1.0f, -3.0f),
		glm::vec3(-8.0f, 1.0f, -4.0f),
		glm::vec3(-8.0f, 1.0f, -3.0f),
		glm::vec3(-8.0f, 1.0f, -7.0f),
		glm::vec3(-8.0f, 1.0f, -9.0f),
		glm::vec3(-5.0f, 1.0f, -5.0f),
		glm::vec3(-6.0f, 1.0f, -4.0f),
		glm::vec3(-5.0f, 1.0f, -3.0f),
		glm::vec3(-4.0f, 1.0f, -2.0f),
		glm::vec3(-2.0f, 1.0f, -5.0f),
		glm::vec3(-2.0f, 1.0f, -4.0f),
		glm::vec3(0.0f, 1.0f, -9.0f),
		glm::vec3(0.0f, 1.0f, -8.0f),
		glm::vec3(-1.0f, 1.0f, -7.0f),
		glm::vec3(9.0f, 1.0f, -9.0f),
		glm::vec3(9.0f, 1.0f, -10.0f),
		glm::vec3(9.0f, 1.0f, -8.0f),
		glm::vec3(9.0f, 1.0f, -7.0f),
		glm::vec3(8.0f, 1.0f, -7.0f),
		glm::vec3(8.0f, 1.0f, -6.0f),

		glm::vec3(3.0f, 1.0f, -2.0f),
		glm::vec3(3.0f, 1.0f, -1.0f),
		glm::vec3(4.0f, 1.0f, -1.0f),
		glm::vec3(5.0f, 1.0f, -1.0f),
		glm::vec3(6.0f, 1.0f, -1.0f),
		glm::vec3(7.0f, 1.0f, -1.0f),
		glm::vec3(8.0f, 1.0f, -1.0f),
			glm::vec3(3.0f, 1.0f, -3.0f),
			glm::vec3(3.0f, 1.0f, -4.0f),
			glm::vec3(3.0f, 1.0f, -5.0f),
			glm::vec3(3.0f, 1.0f, -6.0f),
			glm::vec3(3.0f, 1.0f, -7.0f),
			glm::vec3(4.0f, 1.0f, -8.0f),
			glm::vec3(4.0f, 1.0f, -9.0f),
			glm::vec3(1.0f, 1.0f, -7.0f),
			glm::vec3(2.0f, 1.0f, -9.0f),
			glm::vec3(1.0f, 1.0f, -8.0f),
			glm::vec3(6.0f, 1.0f, -9.0f),
			glm::vec3(6.0f, 1.0f, -8.0f),
			glm::vec3(6.0f, 1.0f, -3.0f),
			glm::vec3(5.0f, 1.0f, -4.0f),
			glm::vec3(5.0f, 1.0f, -5.0f),
			glm::vec3(5.0f, 1.0f, -6.0f),
			glm::vec3(1.0f, 1.0f, -1.0f),
			glm::vec3(1.0f, 1.0f, -2.0f),
			glm::vec3(1.0f, 1.0f, -3.0f),
			glm::vec3(1.0f, 1.0f, -4.0f),
			glm::vec3(6.0f, 1.0f, -4.0f),
			glm::vec3(-4.0f, 1.0f, -9.0f),
	};

	unsigned int  cubeVBO;
	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	unsigned int VBOLight;
	glGenBuffers(1, &VBOLight);


	// second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float planeVertices[] = {
		// positions            // normals         // texcoords
		 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
		-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

		 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
		 25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
	};
	// plane VAO
	unsigned int planeVBO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glBindVertexArray(0);

	// load textures
	// -------------
	GLTexture iceTexture = ImageLoader::loadPNG("ice.png");
	GLTexture keyTexture = ImageLoader::loadPNG("key.png");
	GLTexture doorTexture = ImageLoader::loadPNG("door.png");
	GLTexture monsterTexture = ImageLoader::loadPNG("monster.png");

	// configure depth map FBO
	// -----------------------
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// shader configuration
	// --------------------
	shader.use();
	shader.setInt("diffuseTexture", 0);
	shader.setInt("shadowMap", 1);
	debugDepthQuad.use();
	debugDepthQuad.setInt("depthMap", 0);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{

		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);
		int random = rand() % 7 + 0;
		workThatAI(velocities[random]);

		// change light position over time
		lightPos.x = sin(glfwGetTime()) * 3.0f;
		lightPos.z = cos(glfwGetTime()) * 2.0f;
		lightPos.y = 5.0 + cos(glfwGetTime()) * 1.0f;

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// be sure to activate shader when setting uniforms/drawing objects
		lightingShader.use();
		lightingShader.setVec3("objectColor", 0.6f, 0.256f, 2.0f);
		lightingShader.setVec3("lightColor", 2.0f, 2.0f, 1.0f);
		lightingShader.setVec3("lightPos", lightPos);
		lightingShader.setVec3("viewPos", camera.Position);

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		lightingShader.setMat4("projection", projection);
		lightingShader.setMat4("view", view);

		// world transformation
		glm::mat4 model = glm::mat4(1.0f);

		// 1. render depth of scene to texture (from light's perspective)
		// --------------------------------------------------------------
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		float near_plane = 1.0f, far_plane = 7.5f;
		//lightProjection = glm::perspective(glm::radians(45.0f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane); // note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene
		lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		lightSpaceMatrix = lightProjection * lightView;
		// render scene from light's point of view
		simpleDepthShader.use();
		simpleDepthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, monsterTexture.id);
		renderScene(simpleDepthShader, monsterTexture);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// reset viewport
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 2. render scene as normal using the generated depth/shadow map  
		// --------------------------------------------------------------
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shader.use();
		projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		view = camera.GetViewMatrix();
		shader.setMat4("projection", projection);
		shader.setMat4("view", view);
		// set light uniforms
		shader.setVec3("viewPos", camera.Position);
		shader.setVec3("lightPos", lightPos);
		shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, iceTexture.id);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		renderScene(shader, monsterTexture);
		if (showTheKey == true)
		{
			glm::mat4 model1 = glm::mat4(1.0f);
			model1 = glm::mat4(1.0f);
			model1 = glm::translate(model1, glm::vec3(-9.0f, 0.0f, -9.0f));
			model1 = glm::scale(model1, glm::vec3(0.2f));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, keyTexture.id);
			shader.setMat4("model", model1);
			renderCube();

			model1 = glm::mat4(1.0f);
			model1 = glm::translate(model1, glm::vec3(9.0f, 0.0f, -10.0f));
			model1 = glm::scale(model1, glm::vec3(0.5f));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, doorTexture.id);
			shader.setMat4("model", model1);
			renderCube();
		}


		lightingShader.use();
		// render the cube
		glBindVertexArray(cubeVAO);
		for (unsigned int i = 0; i < 186; i++)
		{
			// calculate the model matrix for each object and pass it to shader before drawing
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, cubePositions[i]);
			//float angle = 20.0f * i;
			//model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
			lightingShader.setMat4("model", model);


			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		glBindVertexArray(0);
		lightingShader.setMat4("view", view);
		lightingShader.setMat4("projection", projection);

		// also draw the lamp object
		lampShader.use();
		lampShader.setMat4("projection", projection);
		lampShader.setMat4("view", view);
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3(0.5f)); // a smaller cube
		lampShader.setMat4("model", model);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// render Depth map to quad for visual debugging
		// ---------------------------------------------
		debugDepthQuad.use();
		debugDepthQuad.setFloat("near_plane", near_plane);
		debugDepthQuad.setFloat("far_plane", far_plane);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		// COLLUSION DETECTION

		glm::vec3 Position = camera.Position;
		glm::vec3 NewPos = { camera.Position[0], camera.Position[1], camera.Position[2] };
		glm::vec3 oldPos = camera.Before;
		glm::vec3 oldAI = ourAI.BeforePos;

		glm::vec3 AIPos = ourAI.Position;

		// AABB for all the none moving cubes
		for (int i = 0; i < 186; i++)
		{
			glm::vec3 cube1 = { cubePositions[i].x, cubePositions[i].y, cubePositions[i].z };

			float dx = Position[0] - cube1.x;
			float dy = Position[1] - cube1.y;
			float dz = Position[2] - cube1.z;

			float Idistance = sqrt(dx*dx + dy * dy + dz * dz);
			if (Idistance <= 0.8)
			{
				camera.Position = oldPos;
			}

		
		}

		float Ax = Position[0] - AIPos[0];
		float Ay = Position[1] - AIPos[1];
		float Az = Position[2] - AIPos[2];

		float AI_distance = sqrt(Ax*Ax + Ay * Ay + Az * Az);
		bool warningMonster = false;
		if (AI_distance <= 5 && AI_distance > 0.8)
		{
			RenderText(textShader, "MONSTER IS NEAR", 400.0f, 300.0f, 1.0f, glm::vec3(1.0F, 0.0f, 0.0f));
		}
		if (AI_distance <= 0.8)
		{

			RenderText(textShader, "GAME OVER", 500.0f, 400.0f, 1.0f, glm::vec3(1.0F, 0.0f, 0.0f));
			RenderText(textShader, "Press Enter to close the game...", 300.0f, 300.0f, 1.0f, glm::vec3(1.0F, 0.0f, 0.0f));

			if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);

		}


		glm::vec3 Key(-9.0f, 0.0f, -9.0f);
		float dx = camera.Position[0] - Key.x;
		float dy = camera.Position[1] - Key.y;
		float dz = camera.Position[2] - Key.z;
		float distance = sqrt(dx*dx + dy * dy + dz * dz);
		if (distance <= 0.8)
		{
			showTheKey = false;
			RenderText(textShader, "You got the key, You can jump!", 300.0f, 10.0f, 1.0f, glm::vec3(1.0, 6.0f, 6.0f));
		}
		glfwPollEvents();

		//std::cout << camera.Position[0] <<","<< camera.Position[1] << ","<< camera.Position[2] << std::endl;

		if (showTheKey == false && camera.Position[0] > 7 && camera.Position[0] < 10 && camera.Position[1] == 0 && camera.Position[2] <= -10) {
			ourAI.Position = glm::vec3(0.0f, 2.0f, 0.0f);
			RenderText(textShader, "YOU WON", 500.0f, 400.0f, 1.0f, glm::vec3(0.5f, 1.0f, 0.0f));
			RenderText(textShader, "Press Enter to close the game...", 300.0f, 300.0f, 1.0f, glm::vec3(0.5f, 1.0f, 0.0f));

			if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);
		}

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);

	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeVBO);

	glutMainLoop();
	glfwTerminate();
	return 0;
}

// renders the 3D scene
// --------------------
void renderScene(const Shader &shader, GLTexture monster)
{
	// floor
	glm::mat4 model = glm::mat4(1.0f);
	shader.setMat4("model", model);
	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	// cubes
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(1.0f, 1.5f, 0.0));
	model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.setMat4("model", model);
	renderCube();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 2.0));
	model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	model = glm::scale(model, glm::vec3(0.25f));
	shader.setMat4("model", model);
	renderCube();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(3.0f, 0.0f, 2.0));
	model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	model = glm::scale(model, glm::vec3(0.25f));
	shader.setMat4("model", model);
	renderCube();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 2.0));
	model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	model = glm::scale(model, glm::vec3(0.25));
	shader.setMat4("model", model);
	renderCube();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 2.0));
	model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	model = glm::scale(model, glm::vec3(0.25));
	shader.setMat4("model", model);
	renderCube();

	int random = rand() % 7 + 0;
	ourAI.Position = workThatAI(velocities[random]);

	model = glm::mat4(1.0f);
	model = glm::translate(model, ourAI.Position);
	model = glm::scale(model, glm::vec3(0.3f));
	shader.setMat4("model", model);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, monster.id);
	renderCube();
}



// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	camera.Before = camera.Position;
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.ProcessKeyboard(JUMP, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}
glm::vec3 workThatAI(int random)
{
	glm::vec3 Pos = camera.Position;
	ourAI.BeforePos = ourAI.Position;

	if (Pos[0] > ourAI.Position[0])
		ourAI.AIKeyboard(SAG, deltaTime, random);
	if (Pos[0] < ourAI.Position[0])
		ourAI.AIKeyboard(SOL, deltaTime, random);
	if (Pos[2] > ourAI.Position[2])
		ourAI.AIKeyboard(ARKA, deltaTime, random);
	if (Pos[2] < ourAI.Position[2])
		ourAI.AIKeyboard(ON, deltaTime, random);

	return(ourAI.Position);
}

void RenderText(Shader &shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
	// Activate corresponding render state	
	shader.use();
	glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAOText);

	// Iterate through all characters
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		Character ch = Characters[*c];

		GLfloat xpos = x + ch.Bearing.x * scale;
		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;


		GLfloat w = ch.Size.x * scale;
		GLfloat h = ch.Size.y * scale;
		// Update VBO for each character
		GLfloat vertices[6][4] = {
			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w, ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w, ypos,       1.0, 1.0 },
			{ xpos + w, ypos + h,   1.0, 0.0 }
		};
		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// Update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, VBOText);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
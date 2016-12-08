#define GLFW_DLL 1

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>

#include "linmath.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
  float Position[2];
  float TexCoord[2];
} Vertex;

Vertex vertexes[] = {
  {{1, -1}, {0.99999, 0}},
  {{1, 1},  {0.99999, 0.99999}},
  {{-1, 1}, {0, 0.99999}},
  {{-1, -1}, {0, 0}},
  {{1, -1}, {0.99999, 0}}
};

static const char* vertex_shader_text =
"uniform mat4 MVP;\n"
"attribute vec2 TexCoordIn;\n"
"attribute vec2 vPos;\n"
"varying vec2 TexCoordOut;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    TexCoordOut = TexCoordIn;\n"
"}\n";

static const char* fragment_shader_text =
"varying lowp vec2 TexCoordOut;\n"
"uniform sampler2D Texture;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
"}\n";

float rotation = 3.1415;

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    } else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
      rotation -= 1.5708;
    } else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
      rotation += 1.5708;
    }
}

void glCompileShaderOrDie(GLuint shader) {
  GLint compiled;
  glCompileShader(shader);
  glGetShaderiv(shader,
		GL_COMPILE_STATUS,
		&compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader,
		  GL_INFO_LOG_LENGTH,
		  &infoLen);
    char* info = malloc(infoLen+1);
    GLint done;
    glGetShaderInfoLog(shader, infoLen, &done, info);
    printf("Unable to compile shader: %s\n", info);
    exit(1);
  }
}

int ppmFileType;
int maxColorValue;

int image_width;
int image_height;

unsigned char* image;

void getPPMFileType(FILE* fh) {
	char PPMFileType [4];

	if (fgets(PPMFileType, 4, fh) != NULL) {
		if (!strcmp(PPMFileType, "P3\n")) {
			ppmFileType = '3';
		} else if (!strcmp(PPMFileType, "P6\n")) {
			ppmFileType = '6';
		} else {
			fprintf(stderr, "Error: Not a PPM file. Incompatible file type.\n");
			exit(1);
		}
	}
}

void getWidthAndHeight(FILE* fh) {
	char value[20];

	while ((value[0] = fgetc(fh)) == '#') {	// Line is a comment
		while (fgetc(fh) != '\n');	// Ignore the line
	}

	if (!isdigit(value[0])) {
		fprintf(stderr, "Error: Image width is not valid.\n");
		exit(1);
	}

	int i = 1;
	while ((value[i] = fgetc(fh)) != ' ') {
		if (!isdigit(value[i])) {
			fprintf(stderr, "Error: Image width is not valid.\n");
			exit(1);
		}
		i++;
	}
	value[i] = '\0';
	image_width = atoi(value);

	i = 0;
	while ((value[i] = fgetc(fh)) != '\n') {
		if (!isdigit(value[i])) {
			fprintf(stderr, "Error: Image height is not valid.\n");
			exit(1);
		}
		i++;
	}
	value[i] = '\0';
	image_height = atoi(value);
}

void getMaxColorValue(FILE* fh) {
	char value[4];

	for (int i = 0; i < 4; i++) {
		value[i] = fgetc(fh);
		if (value[i] == '\n') {
			value[i] = '\0';
			break;
		} else if (!isdigit(value[i])) {
			fprintf(stderr, "Error: Value must be a digit.\n");
			exit(1);
		}
	}

	maxColorValue = atoi(value);

	if (maxColorValue > 255) {
		fprintf(stderr, "Error: Not a PPM file. Max color value too high.\n");
		exit(1);
	} else if (maxColorValue < 1) {
		fprintf(stderr, "Error: Not a PPM file. Max color value too low.\n");
		exit(1);
	}
}

void getP3Value(FILE* fh, unsigned char* outValue) {
	unsigned char value[4];
	int rgbValue;

	for (int i = 0; i < 4; i++) {
		value[i] = fgetc(fh);
		if (value[i] == '\n') {
			value[i] = '\0';
			break;
		} else if (!isdigit(value[i])) {
			fprintf(stderr, "Error: Value must be a digit.\n");
			exit(1);
		}
	}

	rgbValue = atoi(value);
	if (rgbValue > maxColorValue) {
		fprintf(stderr, "Error: Color value exceeding max.\n");
		exit(1);
	}

	*outValue = rgbValue;
}

void parseP3(FILE* fh) {
	for (int i = 0; i < image_width * image_height * 3; i += 3) {
		getP3Value(fh, &image[i]);
		getP3Value(fh, &image[i+1]);
		getP3Value(fh, &image[i+2]);
	}
}

void parseP6(FILE* fh) {
	fread(image, sizeof(unsigned char), image_width * image_height * 3, fh);
}

void loadPPM(FILE* fh) {
	getWidthAndHeight(fh);
	image = malloc(sizeof(unsigned char) * image_width * image_height * 3);
	getMaxColorValue(fh);
	if (ppmFileType == '3') {
		parseP3(fh);
	} else if (ppmFileType == '6') {
		parseP6(fh);
	}
}

int main(int argc, char* argv[])
{
  FILE* fh;

  fh = fopen(argv[1], "rb");
  if (fh == NULL) {
    fprintf(stderr, "Error: Input file not found.\n");
    return 1;
  }

  getPPMFileType(fh);

  loadPPM(fh);
  fclose(fh);

    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    int windowWidth = 640;
    int windowHeight = 480;
    window = glfwCreateWindow(windowWidth, windowHeight, "ezview", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    // gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE: OpenGL error checks have been omitted for brevity

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShaderOrDie(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShaderOrDie(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    // more error checking! glLinkProgramOrDie!

    mvp_location = glGetUniformLocation(program, "MVP");
    assert(mvp_location != -1);

    vpos_location = glGetAttribLocation(program, "vPos");
    assert(vpos_location != -1);

    GLint texcoord_location = glGetAttribLocation(program, "TexCoordIn");
    assert(texcoord_location != -1);

    GLint tex_location = glGetUniformLocation(program, "Texture");
    assert(tex_location != -1);

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location,
			  2,
			  GL_FLOAT,
			  GL_FALSE,
                          sizeof(Vertex),
			  (void*) 0);

    glEnableVertexAttribArray(texcoord_location);
    glVertexAttribPointer(texcoord_location,
			  2,
			  GL_FLOAT,
			  GL_FALSE,
                          sizeof(Vertex),
			  (void*) (sizeof(float) * 2));

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB,
		 GL_UNSIGNED_BYTE, image);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(tex_location, 0);

    while (!glfwWindowShouldClose(window))
    {
        float ratio;
        int width, height;
        mat4x4 m, p, mvp;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        mat4x4_identity(m);
        mat4x4_rotate_Z(m, m, rotation);
        mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        mat4x4_mul(mvp, p, m);

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawArrays(GL_TRIANGLES, 2, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    free(image);
    exit(EXIT_SUCCESS);
}

//! [code]

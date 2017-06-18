#define NOMINMAX
#include "GLRenderer.h"

#include <algorithm>

#include <vector>
#include <stdlib.h>

namespace
{

struct Vertex final
{
	Vertex() = delete;
	float x;
	float y;
	float z;
};

std::vector<Vertex> generate_vertices()
{
	std::vector<Vertex> vertices;

	const int x_count = 5;
	const int y_count = 4;
	
	for (int x_counter = 0; x_counter < x_count; ++x_counter)
	{
		for (int y_counter = 0; y_counter < y_count; ++y_counter)
		{
			vertices.push_back({-1.f + x_counter * (2.0f / (x_count - 1)), -1.f + y_counter * (2.0f / (y_count - 1)), 0.f});
		}
	}
	return vertices;
}

std::vector<unsigned short> generate_indices(const unsigned short vertex_count, unsigned index_count)
{
	std::vector<unsigned short> indices;
	indices.reserve(index_count);

	for (unsigned i = 0; i < index_count; ++i)
	{
		indices.push_back(rand() % vertex_count);
	}

	return indices;
}

GLuint LoadShader(GLenum type, const char *shaderSrc)
{
	GLuint shader;
	GLint compiled;

	shader = glCreateShader(type);

	if (shader == 0)
		return 0;

	glShaderSource(shader, 1, &shaderSrc, NULL);

	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled)
	{
		GLint infoLen = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint create_program()
{
	const char * vShaderStr =
		R"(
attribute vec4 vPosition;
void main()
{
	gl_Position = vPosition;
}
)";

	const char * fShaderStr =
		R"(
void main()
{
	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;

	vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);

	programObject = glCreateProgram();

	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);

	glBindAttribLocation(programObject, 0, "vPosition");

	glLinkProgram(programObject);

	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		GLint infoLen = 0;

		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
		glDeleteProgram(programObject);
	}

	return programObject;
}

} // namespace

GLRenderer::GLRenderer(int width, int height, const std::function<void()> &create_context, const std::function<void()> destroy_context)
	: width_(width)
	, height_(height)
	, create_context_(create_context)
	, destroy_context_(destroy_context)
{
	worker_stopped_ = true;
	requested_ = 0;
}

void GLRenderer::Setup(const BufferUploadingPolicy policy)
{
	glDeleteBuffers(1, &vbo_);
	glDeleteBuffers(1, &ibo_);

	policy_ = policy;
	program_object_ = create_program();

	const auto vertices = generate_vertices();
	indices_ = generate_indices((unsigned short)vertices.size(), 128 * 1024 * 1024 / sizeof(unsigned short));

	glGenBuffers(1, &vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	glGenBuffers(1, &ibo_);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices_.size(), nullptr, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices_.size(), indices_.data(), GL_STATIC_DRAW);
}

void GLRenderer::Draw()
{
	glViewport(0, 0, width_, height_);

	glClearColor((rand() % 255) / 255.f, (rand() % 255) / 255.f, (rand() % 255) / 255.f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program_object_);

	++requested_;

	if (policy_ == BufferUploadingPolicy::MainThreadUploading)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices_.size(), nullptr, GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices_.size(), indices_.data(), GL_STATIC_DRAW);
	}

// 	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, (void*)10002);
}

void GLRenderer::StartWorker()
{
	worker_stopped_ = false;
	worker_thread_ = std::thread([this]()
	{
		create_context_();
		while (!worker_stopped_)
		{
			if (requested_ > 0)
			{
				--requested_;
				if (policy_ == BufferUploadingPolicy::WorkerThreadUploading)
				{
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices_.size(), nullptr, GL_STATIC_DRAW);
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices_.size(), indices_.data(), GL_STATIC_DRAW);
				}
			}
		}
		destroy_context_();
	});
}

void GLRenderer::StopWorker()
{
	worker_stopped_ = true;
	if (worker_thread_.joinable())
	{
		worker_thread_.join();
	}
}

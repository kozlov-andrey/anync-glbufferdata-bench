#pragma once

#ifdef _WIN32
#include <glad/glad.h>
#else
#include <GLES2/gl2.h>
#endif

#include <vector>
#include <functional>
#include <atomic>
#include <thread>

enum class BufferUploadingPolicy
{
	NoUploading,
	MainThreadUploading,
	WorkerThreadUploading,
};

class GLRenderer final
{
public:
	GLRenderer(int width, int height, const std::function<void()> &create_context, const std::function<void()> destroy_context);

	void Setup(BufferUploadingPolicy policy);
	void Draw();
	void StartWorker();
	void StopWorker();

private:
	int width_ = 0;
	int height_ = 0;
	GLuint program_object_ = 0;
	GLuint vbo_ = 0;
	GLuint ibo_ = 0;
	std::vector<unsigned short> indices_;
	std::function<void()> create_context_;
	std::function<void()> destroy_context_;
	std::atomic<bool> worker_stopped_;
	std::atomic<int> requested_;
	std::thread worker_thread_;
	BufferUploadingPolicy policy_;
};
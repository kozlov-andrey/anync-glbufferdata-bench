#include "CollectStatistics.h"

#include "Time.h"
#include "GLRenderer.h"

#include <iostream>
#include <fstream>
#include <map>

void collect_statistics(
	std::function<void()> swap_func,
	std::function<void()> create_context,
	std::function<void()> destroy_context,
	int width,
	int height,
	const char *output_path)
{
	GLRenderer renderer(width, height, create_context, destroy_context);

	std::vector<double> fps_statistics;

	std::ofstream file(std::string(output_path) + "statistics.txt");

	const auto policies = {
		BufferUploadingPolicy::NoUploading,
		BufferUploadingPolicy::WorkerThreadUploading,
		BufferUploadingPolicy::MainThreadUploading
	};

	for (const auto policy : policies)
	{
		renderer.Setup(policy);
		renderer.StartWorker();

		const auto time_start = time_now();

		const auto frame_count = 100;

		for (int i = 0; i < frame_count; ++i)
		{
			renderer.Draw();
			swap_func();
		}
		renderer.StopWorker();
		const auto time_end = time_now();

		fps_statistics.push_back(frame_count / ((time_end - time_start) / 1000000000.));
	}


	for (const auto fps : fps_statistics)
	{
		file << fps << std::endl;
	}
}

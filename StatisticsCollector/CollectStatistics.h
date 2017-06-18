#pragma once

#include <functional>

void collect_statistics(
	std::function<void()> swap_func,
	std::function<void()> create_context,
	std::function<void()> destroy_context,
	int width, int height, const char *output_path);
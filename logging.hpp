#pragma once

#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdarg>

enum : uint32_t {
	eLogInfo = 0,
	eLogWarning,
	eLogError,
};

static inline std::string time_str()
{
	auto t = std::time(0);
	auto tm = *std::localtime(&t);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%H:%M:%S");
	return oss.str();
}

inline void logf(uint32_t mode, const char* format, ...)
{
	// Bold and color
	static const char *modifiers[] = {
		"\033[1m\033[34m",
		"\033[1m\033[33m",
		"\033[1m\033[31m",
	};

	static const char *end = "\033[0m";

	static const char *headers[] = {
		"INFO",
		"WARNING",
		"ERROR",
	};

	const std::string time = time_str();

	va_list args;
	va_start(args, format);
	fprintf(stderr, "%s[%s %s]%s ", modifiers[mode], time.c_str(), headers[mode], end);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

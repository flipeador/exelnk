#pragma once

/***************************************************
 * WINDOWS
***************************************************/

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#undef GetCurrentDirectory
#undef SetCurrentDirectory
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable

/***************************************************
 * STD LIBRARY
***************************************************/

#include <format>
#include <vector>
#include <array>
#include <ranges>
#include <regex>
#include <string>
#include <string_view>
#include <iostream>
#include <optional>
#include <algorithm>
#include <functional>

using String = std::wstring;
using StrView = std::wstring_view;

template <typename T> using Vector = std::vector<T>;
template <typename T> using Optional = std::optional<T>;
template <typename T> using Function = std::function<T>;

/***************************************************
 * PROJECT
***************************************************/

#include "lib/path.hpp"
#include "lib/file.hpp"
#include "lib/util.hpp"

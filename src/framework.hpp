#pragma once

/***************************************************
 * WINDOWS
***************************************************/

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#define LONG_MAXPATH UNICODE_STRING_MAX_CHARS // 32767

/***************************************************
 * STD LIBRARY
***************************************************/

#include <optional>
#include <iostream>
#include <filesystem>

using String = std::wstring;
using StrView = std::wstring_view;
using Path = std::filesystem::path;

/***************************************************
 * PROJECT
***************************************************/

#include "util.hpp"

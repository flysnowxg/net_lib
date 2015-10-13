#pragma once

#ifndef WINVER				// 允许使用特定于 Windows XP 或更高版本的功能。
#define WINVER 0x0501		// 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif

#ifndef _WIN32_WINNT		// 允许使用特定于 Windows XP 或更高版本的功能。
#define _WIN32_WINNT 0x0501	// 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif						

#ifndef _WIN32_WINDOWS		// 允许使用特定于 Windows 98 或更高版本的功能。
#define _WIN32_WINDOWS 0x0410 // 将此值更改为适当的值，以指定将 Windows Me 或更高版本作为目标。
#endif

#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include <windows.h>

#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <strstream>
#include <list>
#include <map>
#include <hash_map>
#include <set>
#include <vector>
#include <algorithm>
using namespace std;
using namespace stdext;
using namespace std::tr1;

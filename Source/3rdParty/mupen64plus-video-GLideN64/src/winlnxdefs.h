/**
 * Mupen64 - winlnxdefs.h
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#ifndef WINLNXDEFS_H
#define WINLNXDEFS_H

#include <limits.h> // PATH_MAX
#include <stdlib.h> // malloc(), srand()
#include <stdio.h>
#include <string.h>
#include <time.h>   // time()
#include <unistd.h> // readlink()

#define timeGetTime() time( 0 )

typedef unsigned int BOOL, BOOLEAN;
typedef unsigned int DWORD;
typedef unsigned long long DWORD64, QWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE, byte;
typedef unsigned int UINT;

typedef char CHAR;
typedef short SHORT;
typedef long LONG;
typedef float FLOAT;

typedef long __int32;
typedef int HINSTANCE;
typedef int HWND;
typedef int WPARAM;
typedef int LPARAM;
typedef void* LPVOID;

typedef const char 	*LPCSTR;

// types
/*#define BOOL	unsigned int
#define BOOLEAN	unsigned int
#define DWORD	unsigned long
#define WORD	unsigned short
#define BYTE	unsigned char*/

#define __declspec(dllexport)
#define _cdecl
#define WINAPI
//#define APIENTRY
//#define EXPORT
//#define CALL

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

#endif

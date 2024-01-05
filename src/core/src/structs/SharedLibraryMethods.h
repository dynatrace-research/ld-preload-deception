// Copyright 2024 Dynatrace LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Portions of this code, as identified in remarks, are provided under the
// Creative Commons BY-SA 4.0 or the MIT license, and are provided without
// any warranty. In each of the remarks, we have provided attribution to the
// original creators and other attribution parties, along with the title of
// the code (if known) a copyright notice and a link to the license, and a
// statement indicating whether or not we have modified the code.

#pragma once

#include <sys/socket.h>

#ifndef __USE_GNU
#	define __USE_GNU
#endif

typedef int (*func_libc_start_main_t)(
		int (*)(int, char**, char**), int, char**, int (*)(int, char**, char**), void (*)(void), void (*)(void), void*);

typedef int (*func_bind_t)(int, const struct sockaddr*, socklen_t);
typedef int (*func_accept_t)(int, struct sockaddr*, socklen_t*);
typedef int (*func_accept4_t)(int, struct sockaddr*, socklen_t*, int);
typedef int (*func_getsockname_t)(int, struct sockaddr*, socklen_t* restrict);
typedef ssize_t (*func_recv_t)(int, void*, size_t, int);
typedef ssize_t (*func_send_t)(int, const void*, size_t, int);
typedef ssize_t (*func_read_t)(int, void*, size_t);
typedef ssize_t (*func_write_t)(int, const void*, size_t);
typedef int (*func_close_t)(int);

typedef struct {
	func_libc_start_main_t main_global;

	func_bind_t bind_global;
	func_accept_t accept_global;
	func_accept4_t accept4_global;
	func_getsockname_t getsockname_global;
	func_recv_t recv_global;
	func_send_t send_global;
	func_read_t read_global;
	func_write_t write_global;
	func_close_t close_global;
} SharedLibraryMethods;

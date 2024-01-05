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

#include "./structs/SharedLibraryMethods.h"

#include <sys/socket.h>
#include <sys/types.h>

/**
 * Overwritten shared library methods.
 */
int bind(int sockfd, const struct sockaddr* address, socklen_t address_len);
int accept(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len);
int accept4(int sockfd, struct sockaddr* address, socklen_t* addrlen, int flags);
int getsockname(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len);
ssize_t read(int fd, void* buf, size_t count);
ssize_t write(int fd, const void* buf, size_t count);
ssize_t recv(int sockfd, void* buf, size_t len, int flags);
ssize_t send(int sockfd, const void* buf, size_t len, int flags);
int close(int fd);

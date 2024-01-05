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

// prepared for quick tracing output during development
int bind_dev(int sockfd, const struct sockaddr* address, socklen_t address_len);
int accept_dev(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len);
int accept4_dev(int sockfd, struct sockaddr* address, socklen_t* addrlen, int flags);
int getsockname_dev(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len);
ssize_t read_dev(int fd, void* buf, size_t count);
ssize_t write_dev(int fd, const void* buf, size_t count);
ssize_t recv_dev(int sockfd, void* buf, size_t len, int flags);
ssize_t send_dev(int sockfd, const void* buf, size_t len, int flags);
int close_dev(int fd);

// --------------- not globaly implemented function
typedef int (*func_socket_t)(int, int, int);
typedef int (*func_getpeername_t)(int, struct sockaddr* restrict, socklen_t* restrict);
typedef int (*func_getsockopt_t)(int, int, int, void* restrict, socklen_t* restrict);
typedef int (*func_connect_t)(int, const struct sockaddr*, socklen_t);
typedef int (*func_listen_t)(int, int);
typedef int (*func_openat_t)(int, const char*, int, mode_t);

typedef int (*func_recvfrom_t)(int, void*, size_t, int, struct sockaddr*, socklen_t*);
typedef int (*func_recvmsg_t)(int, struct msghdr*, int);

typedef ssize_t (*func_first_t)(int, char**, char**);
/*
int socket(int domain, int type, int protocol);
int getpeername(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len);
int getsockopt(int socket, int level, int option_name, void* restrict option_value, socklen_t* restrict option_len);
int connect(int socket, const struct sockaddr* address, socklen_t address_len);
int listen(int sockfd, int backlog);

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags);
//*/

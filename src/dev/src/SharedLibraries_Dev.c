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


#include "SharedLibraries_Dev.h"
#include "../../core/src/Utils.h"
#include "../../core/src/structs/GlobalVariables.h"
#include "../../default/src/SharedLibraries_Default.h"
#include "DevUtils.h"

#include <arpa/inet.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int bind_dev(int sockfd, const struct sockaddr* address, socklen_t address_len) {
	int success = globals.originalSharedLibraryMethods.bind_global(sockfd, address, address_len);

	// filter out non-ipv4 request
	if (globals.honeywiresBook != NULL && sockfd < SOCKET_FD_LIMIT && success == 0 && address != NULL && isIp(address->sa_family)) {
		struct sockaddr_in* address_in = (struct sockaddr_in*)address;
		unsigned short port = htons(address_in->sin_port);
		char inetStringBufferv4[INET_ADDRSTRLEN];
		char inetStringBufferV6[INET6_ADDRSTRLEN ];

		// Convert the network address to char[]
		if (address->sa_family == AF_INET) {
			inet_ntop(AF_INET, &(address_in->sin_addr), inetStringBufferv4, sizeof(inetStringBufferv4));
		} else if(address->sa_family == AF_INET6) {
			inet_ntop(AF_INET6, &(address_in->sin_addr), inetStringBufferV6, sizeof(inetStringBufferV6));
		}

		simpleLogger(LoggerPriority__INFO, " [-] bind_dev(socketFd: %d, port %d) \n", sockfd, port);

		if (globals.socketFdTracing == -1 && isSupportedPort(port)) {
			simpleLogger(LoggerPriority__INFO, "  |- bind_dev -> socketFdTracing is set to: %d\n", sockfd);

			globals.socketFdTracing = sockfd;
		}
	}

	return success;
}

int accept_dev(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len) {
	simpleLogger(
			LoggerPriority__INFO,
			" [-] accept_dev(socket %d, address_len %jd) + saved sockedID: %d\n",
			socket,
			(intmax_t)address_len,
			globals.socketFdTracing);

	return accept4_default(socket, address, address_len, 0);
}

int accept4_dev(int sockfd, struct sockaddr* address, socklen_t* addrlen, int flags) {
	int newSockfd = globals.originalSharedLibraryMethods.accept4_global(sockfd, address, addrlen, flags);

	simpleLogger(
			LoggerPriority__INFO,
			" [-] accept4_dev(sockfd: %d, newSockfd: %d, addrlen: %d, flags: %d); return %d\n",
			sockfd,
			newSockfd,
			sockfd,
			flags);

	return newSockfd;
}

int getsockname_dev(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len) {
	int success = globals.originalSharedLibraryMethods.getsockname_global(socket, address, address_len);

	if (socket < SOCKET_FD_LIMIT && success == 0 && isIp(address->sa_family)) {
		struct sockaddr_in* address_in = (struct sockaddr_in*)address;
		unsigned short port = htons(address_in->sin_port);

		simpleLogger(
				LoggerPriority__INFO,
				" [-] getsockname(socket: %d, port %d, address_len %d)\n",
				socket,
				port,
				address_len);
	}

	return success;
}

ssize_t read_dev(int fd, void* buf, size_t count) {
	ssize_t bytesRead = globals.originalSharedLibraryMethods.read_global(fd, buf, count);

	// without restriction container might generate timeout due to to many log lines
	if (fd < SOCKET_FD_LIMIT && globals.socketTracedToPort[fd] == true) {
		simpleLogger(LoggerPriority__INFO, " [-] read_dev(fd %d, buf %p, count %zu); return %zd\n", fd, buf, count, bytesRead);
	}

	return bytesRead;
}

ssize_t write_dev(int fd, const void* buf, size_t count) {
	ssize_t bytesWritten = globals.originalSharedLibraryMethods.write_global(fd, buf, count);

	// without restriction container might generate timeout due to to many log lines
	if (fd < SOCKET_FD_LIMIT && globals.socketTracedToPort[fd] == true) {
		simpleLogger(LoggerPriority__INFO, " [-] write_dev(fd %d, buf %p, count %zu); return %zd\n", fd, buf, count, bytesWritten);
	}

	return bytesWritten;
}

ssize_t recv_dev(int sockfd, void* buf, size_t len, int flags) {
	ssize_t ret = globals.originalSharedLibraryMethods.recv_global(sockfd, buf, len, flags);

	// without restriction container might generate timeout due to to many log lines
	if (sockfd < SOCKET_FD_LIMIT) {
		simpleLogger(LoggerPriority__INFO, " [-] recv_dev(sockfd: %d, length: %zu, ret: %zd, flags: %d)\n", sockfd, len, ret, flags);
	}

	return ret;
}

ssize_t send_dev(int sockfd, const void* buf, size_t len, int flags) {
	// without restriction container might generate timeout due to to many log lines
	if (sockfd < SOCKET_FD_LIMIT) {
		simpleLogger(LoggerPriority__INFO, " [-] send_dev(sockfd: %d, len: %zu, flags: %d)\n", sockfd, len, flags);
	}

	return globals.originalSharedLibraryMethods.send_global(sockfd, buf, len, flags);
}

int close_dev(int fd) {
	if (fd > -1 && fd < SOCKET_FD_LIMIT && globals.socketTracedToPort[fd] != 0) {
		simpleLogger(LoggerPriority__INFO, " [-] close_dev(%d) \n", fd);
	}

	return globals.originalSharedLibraryMethods.close_global(fd);
}

// --------------- not globally implemented function
/**
 * Methods not implemented within /src/core/src/SharedLibraries.c have to be commented out to not automatically overwrite original libc
 * functions.
 * TODO TR-55 Point 8: Refactor Makefile that /dev/src/ * files are only included if the `make dev` command is executed. With that this
 * should solve the problem of commenting the methods below.
 */

/*

int socket(int domain, int type, int protocol) {
	int sockfd = ((func_socket_t)dlsym(RTLD_NEXT, "socket"))(domain, type, protocol);

	if (sockfd > 2 && isIp(domain)) {
		simpleLogger(
				LoggerPriority__INFO, " [-] socket_dev(domain: %d, type: %d, protocol: %d); return %d\n", domain, type, protocol, sockfd);
	}

	return sockfd;
}

int getpeername(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len) {
	int success = (((func_getpeername_t)dlsym(RTLD_NEXT, "getpeername"))(socket, address, address_len));

	simpleLogger(LoggerPriority__INFO, " [-] getpeername_dev(sockfd: %d, address_len: %d, success: %d)\n", socket, address_len, success);

	return success;
}

int getsockopt(int socket, int level, int option_name, void* restrict option_value, socklen_t* restrict option_len) {
	int success = (((func_getsockopt_t)dlsym(RTLD_NEXT, "getsockopt"))(socket, level, option_name, option_value, option_len));

	simpleLogger(
			LoggerPriority__INFO,
			" [-] getsockopt_dev(socket: %d, level: %d, option_name: %d, option_len: %d, success: %d) \n",
			socket,
			level,
			option_name,
			option_len,
			success);

	return success;
}


int connect(int socket, const struct sockaddr* address, socklen_t address_len) {
	int success = (((func_connect_t)dlsym(RTLD_NEXT, "connect"))(socket, address, address_len));

	simpleLogger(LoggerPriority__INFO, " [-] connect_dev(socket: %d, address_len: %d, success: %d)\n", socket, address_len, success);

	return success;
}

int listen(int sockfd, int backlog) {
	int success = (((func_listen_t)dlsym(RTLD_NEXT, "listen"))(sockfd, backlog));

	simpleLogger(LoggerPriority__INFO, " [-] listen_dev(sockfd %d, backlog %d); return %d\n", sockfd, backlog, sockfd, success);

	return success;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
	ssize_t test = ((func_recvfrom_t)dlsym(RTLD_NEXT, "recvfrom"))(sockfd, buf, len, flags, src_addr, addrlen);

	simpleLogger(LoggerPriority__INFO, " [-] recvfrom_dev() \n");
	return test;
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
	ssize_t test = ((func_recvmsg_t)dlsym(RTLD_NEXT, "recvmsg"))(sockfd, msg, flags);

	simpleLogger(LoggerPriority__INFO, " [-] recvmsg_dev()");
	return test;
}

// called by __libc_start_main and for that reason it probably can't be intercepted
void __libc_init_first (int argc, char **argv, char **envp) {
		simpleLogger(LoggerPriority__INFO, " [-] __libc_init_first - socketFdTracing %d \n", globals.socketFdTracing);

	((func_first_t)dlsym(RTLD_NEXT, "__libc_init_first"))(argc, argv, envp);
}

// */

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

#include "SharedLibraries.h"
#include "HoneBookThread.h"
#include "Utils.h"
#include "structs/GlobalVariables.h"
#include "structs/HoneywireBook.h"
#include "structs/SupportedTechnology.h"

#include <arpa/inet.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Use the __libc_start_main to initialize variable and start a additional threat for handling asynchronous workload like updating
 * configuration
 */
int __libc_start_main(
		int (*main)(int, char**, char**),
		int argc,
		char** argv,
		int (*init)(int, char**, char**),
		void (*fini)(void),
		void (*rtld_fini)(void),
		void(*stack_end)) {

	SUPPORTED_TECHNOLOGY supportedTechnology = supportedTechnologyTypeID(argv[0]);

	if (supportedTechnology != SUPPORTED_TECHNOLOGY_NOT_FOUND) {
		int pid = getpid();
		simpleLogger(LoggerPriority__INFO, " [-] __libc_start_main(arguments count: %d; argv[0]: %s): pid: %d \n", argc, argv[0], pid);

		globals.honeywiresBook = initHoneywiresBook();
		startHoneyBookUpdateThread(globals.honeywiresBook);
	}

	setGlobalSharedLibrary(supportedTechnology);

	return globals.sharedLibraryMethods.main_global(main, argc, argv, init, fini, rtld_fini, stack_end);
}

int bind(int sockfd, const struct sockaddr* address, socklen_t address_len) {
	if (globals.sharedLibraryMethods.bind_global != NULL) {
		return globals.sharedLibraryMethods.bind_global(sockfd, address, address_len);
	} else {
		return ((func_bind_t)dlsym(RTLD_NEXT, "bind"))(sockfd, address, address_len);
	}
}

int accept(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len) {
	if (globals.sharedLibraryMethods.accept_global != NULL) {
		return globals.sharedLibraryMethods.accept_global(socket, address, address_len);
	} else {
		return ((func_accept_t)dlsym(RTLD_NEXT, "accept"))(socket, address, address_len);
	}
}

int accept4(int sockfd, struct sockaddr* address, socklen_t* addrlen, int flags) {
	if (globals.sharedLibraryMethods.accept4_global != NULL) {
		return globals.sharedLibraryMethods.accept4_global(sockfd, address, addrlen, flags);
	} else {
		return ((func_accept4_t)dlsym(RTLD_NEXT, "accept4"))(sockfd, address, addrlen, flags);
	}
}

int getsockname(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len) {
	if (globals.sharedLibraryMethods.getsockname_global != NULL) {
		return globals.sharedLibraryMethods.getsockname_global(socket, address, address_len);
	} else {
		return ((func_getsockname_t)dlsym(RTLD_NEXT, "getsockname"))(socket, address, address_len);
	}
}

ssize_t read(int fd, void* buf, size_t count) {
	if (globals.sharedLibraryMethods.read_global != NULL) {
		return globals.sharedLibraryMethods.read_global(fd, buf, count);
	} else {
		return ((func_read_t)dlsym(RTLD_NEXT, "read"))(fd, buf, count);
	}
}

ssize_t write(int fd, const void* buf, size_t count) {
	if (globals.sharedLibraryMethods.write_global != NULL) {
		return globals.sharedLibraryMethods.write_global(fd, buf, count);
	} else {
		return ((func_write_t)dlsym(RTLD_NEXT, "write"))(fd, buf, count);
	}
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
	if (globals.sharedLibraryMethods.write_global != NULL) {
		return globals.sharedLibraryMethods.recv_global(sockfd, buf, len, flags);
	} else {
		return ((func_recv_t)dlsym(RTLD_NEXT, "recv"))(sockfd, buf, len, flags);
	}
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
	if (globals.sharedLibraryMethods.write_global != NULL) {
		return globals.sharedLibraryMethods.send_global(sockfd, buf, len, flags);
	} else {
		return ((func_send_t)dlsym(RTLD_NEXT, "send"))(sockfd, buf, len, flags);
	}
}

int close(int fd) {
	if (globals.sharedLibraryMethods.close_global != NULL) {
		return globals.sharedLibraryMethods.close_global(fd);
	} else {
		return ((func_close_t)dlsym(RTLD_NEXT, "close"))(fd);
	}
}

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

#include "SharedLibraries_Default.h"

#include "../../core/src/HoneBookThread.h"
#include "../../core/src/Utils.h"
#include "../../core/src/structs/GlobalVariables.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <stdint.h>

int bind_default(int sockfd, const struct sockaddr* address, socklen_t address_len) {
	int success = globals.originalSharedLibraryMethods.bind_global(sockfd, address, address_len);

	// no increaseReader() needed, since bind() will not interact with globals.honeywiresBook.honeywiresConfig

	// filter out non-ipv4 request
	if (globals.honeywiresBook != NULL && sockfd < SOCKET_FD_LIMIT && success == 0 && address != NULL && isIp(address->sa_family)) {
		struct sockaddr_in* address_in = (struct sockaddr_in*)address;
		unsigned short port = htons(address_in->sin_port);

		simpleLogger(LoggerPriority__INFO, " [-] bind(socketFd: %d, port %d) \n", sockfd, port);

		if (globals.socketFdTracing == -1 && isSupportedPort(port)) {
			simpleLogger(LoggerPriority__INFO, "  |- bind -> socketFdTracing is set to: %d\n", sockfd);

			globals.socketFdTracing = sockfd;
		}
	}

	return success;
}

int accept_default(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len) {
	if (globals.honeywiresBook != NULL && globals.honeywiresBook->so_hw_model->accept4Model->enabled) {
		simpleLogger(LoggerPriority__INFO, " [>] accept() -> forward to accept4()\n");

		return accept4_default(socket, address, address_len, 0);
	}

	return globals.originalSharedLibraryMethods.accept_global(socket, address, address_len);
}

int accept4_default(int sockfd, struct sockaddr* address, socklen_t* addrlen, int flags) {
	int newSockfd = globals.originalSharedLibraryMethods.accept4_global(sockfd, address, addrlen, flags);

	// guards clauses: check if deception is active and currently possible for this process
	if (!checkFlagAndIncreaseReader(globals.honeywiresBook, globals.honeywiresBook->so_hw_model->accept4Model->enabled)) {
		return newSockfd;
	}

	// if (newSockfdPort will be an open IPv4 connection and is within the socketTracedToPort.length (= SOCKET_FD_LIMIT))
	if (sockfd < SOCKET_FD_LIMIT && newSockfd < SOCKET_FD_LIMIT && address != NULL && isIp(address->sa_family)) {
		struct sockaddr_in* address_in = (struct sockaddr_in*)address;
		unsigned short newSockfdPort = htons(address_in->sin_port);

		// valid port
		if (newSockfdPort > 1 && sockfd == globals.socketFdTracing) {
			globals.socketTracedToPort[newSockfd] = 1;
			simpleLogger(
					LoggerPriority__INFO,
					" [-] accept4: new relevant request detected on newSockFd: %d (linked to sockfd %d) \n",
					newSockfd,
					sockfd);
		}
	}

	readerFinished(globals.honeywiresBook);
	return newSockfd;
}

int getsockname_default(int socket, struct sockaddr* restrict address, socklen_t* restrict address_len) {
	int success = globals.originalSharedLibraryMethods.getsockname_global(socket, address, address_len);

	// no increaseReader() needed, since getsockname() will not interact with globals.honeywiresBook.honeywiresConfig

	if (socket < SOCKET_FD_LIMIT && success == 0 && isIp(address->sa_family)) {
		struct sockaddr_in* address_in = (struct sockaddr_in*)address;
		unsigned short port = htons(address_in->sin_port);

		if (globals.socketFdTracing == -1 && isSupportedPort(port)) {
			globals.socketFdTracing = socket; // initial main python process socket which can be linked to all
											  // incoming request though accept

			simpleLogger(LoggerPriority__INFO, "  |- getsockname -> socketFdTracing is set to: %d\n", socket);
		}
	}

	return success;
}

ssize_t read_default(int fd, void* buf, size_t count) {
	ssize_t bytesRead = globals.originalSharedLibraryMethods.read_global(fd, buf, count);

	// check if deception is active and currently possible for this process
	if (!checkFlagAndIncreaseReader(globals.honeywiresBook, globals.honeywiresBook->so_hw_model->recvModel->enabled)) {
		return bytesRead;
	}

	if (fd < SOCKET_FD_LIMIT && globals.socketTracedToPort[fd] != 0) {
		if (isSupportedHttpVersion(buf, count)) {
			SocketInfo* newSocketInfo = malloc(sizeof(SocketInfo));
			newSocketInfo->requestMode = NONE;
			newSocketInfo->socketProgress = 0;
			globals.socketInfos[fd] = newSocketInfo;

			if (isSupportedHttpVersionAndMatchingPath(buf, count)) {
				newSocketInfo->requestMode = ADMIN_PATH;

				globals.socketInfos[fd] = newSocketInfo;

				simpleLogger(
						LoggerPriority__INFO,
						" [-] read(fd: %d, buf-length %zu, bytesRead: %zd) detected path \"%s\"\n",
						fd,
						count,
						bytesRead,
						globals.honeywiresBook->so_hw_model->recvModel->matchingPathString);
			}
		}
	}

	readerFinished(globals.honeywiresBook);
	return bytesRead;
}

ssize_t write_default(int fd, const void* buf, size_t count) {
	// guards clauses: check if deception is active and currently possible for this process
	if (!checkFlagAndIncreaseReader(globals.honeywiresBook, globals.honeywiresBook->so_hw_model->sendModel->enabled)) {
		return globals.originalSharedLibraryMethods.write_global(fd, buf, count);
	}

	// guards clauses: header and body is split into multiple write() calls. Therefore only first response (header) of a fd have to be
	// intercepted for overwriting header attributes. Probably because write() only writes to a buffer which flushes out one singletcp
	// packages in the end. Tested with second container that called the backend with curl and logged the read_default() method and the
	// second container also received 2 packages.
	if (globals.socketInfos[fd] != NULL && globals.socketInfos[fd]->socketProgress++ != 0) {
		readerFinished(globals.honeywiresBook);
		return globals.originalSharedLibraryMethods.write_global(fd, buf, count);
	}

	int type;
	socklen_t olen = sizeof(type);
	int success = getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &olen);
	// guards clauses: check if deception is relevant for this fd
	if (fd >= SOCKET_FD_LIMIT || globals.socketTracedToPort[fd] <= 0 || success == -1 || type != SOCK_STREAM) {
		readerFinished(globals.honeywiresBook);
		return globals.originalSharedLibraryMethods.write_global(fd, buf, count);
	}

	int firstLineLength = (int)(strnstr(buf, "\n", count) - (char*)buf); // will be negative if strstrWithBound() return null pointer
	int httpVersion = isSupportedHttpVersion(buf, firstLineLength);
	// guards clauses: check if deception supports the httpVersion
	if (httpVersion == -1) {
		simpleLogger(LoggerPriority__INFO, "  |+ write: Http version string is not supported, send default buffer!\n");
		readerFinished(globals.honeywiresBook);
		return globals.originalSharedLibraryMethods.write_global(fd, buf, count);
	}

	simpleLogger(LoggerPriority__INFO, "  |+ write: try to modify response of sockfd %d\n", fd);

	// check and replace header attribute if flag is enabled
	if (globals.honeywiresBook->so_hw_model->sendModel->replaceServerStringEnabled) {
		replaceHttpHeader((char*)buf, count);
	}
	// guards clauses: if replaceStatusCodeEnabled isn't activated, or the read() method hasn't tracked the path (defined in honeyaml) for
	// this fd. In this case current possible modified buffer (replaceServerStringEnabled) can be sent
	if (!globals.honeywiresBook->so_hw_model->sendModel->replaceStatusCodeEnabled || globals.socketInfos[fd] == NULL ||
		globals.socketInfos[fd]->requestMode != ADMIN_PATH) {
		readerFinished(globals.honeywiresBook);
		return globals.originalSharedLibraryMethods.write_global(fd, buf, count);
	}

	// Exchange original status code (e.g., 404) with the one configured in honeybook. This block create a new buffer which might have a
	// different content length and call return;
	const char* actualBuffer = NULL;
	const char** bufPointerPosition = &actualBuffer;
	const char* statusCodeResponse = globals.honeywiresBook->so_hw_model->sendModel->newStatuscodeString;

	int newLength = overWriteStatusCode(
			buf, bufPointerPosition, count, globals.SUPPORTED_HTTP_VERSIONS[httpVersion], statusCodeResponse);

	// Send new buffer
	if (newLength != -1) {
		simpleLogger(LoggerPriority__INFO, "  |+ write: status code was overwrite\n");

		ssize_t originalResponseLen = globals.originalSharedLibraryMethods.write_global(fd, *bufPointerPosition, newLength);
		free(*bufPointerPosition);

		// if needed since it will otherwise send the end of the header-file again (exact len - actual response gets
		// send again) My guess is that logic between the python flask server and the send operation will check if
		// at least the len attribute was send and otherwise send the "not sent" header bytes a second time (even
		// though an overwritten version was sent)
		if (originalResponseLen < count) {
			originalResponseLen = count;
		}

		// increase the count on how often the write() was already triggered on this fd
		globals.socketInfos[fd]->socketProgress++;

		readerFinished(globals.honeywiresBook);
		return originalResponseLen;
	} else if (*bufPointerPosition != NULL) {
		simpleLogger(LoggerPriority__INFO, "  !-- write(): error while generating new status code! Original message will be sent.\n");
		readerFinished(globals.honeywiresBook);
		free(*bufPointerPosition);
		return globals.originalSharedLibraryMethods.write_global(fd, *bufPointerPosition, newLength);
	}

	simpleLogger(
			LoggerPriority__INFO,
			"  !-- write(): code path should not be executed! Original message will be sent, but there might be buffer corruption or "
			"leak.\n");
	readerFinished(globals.honeywiresBook);
	return globals.originalSharedLibraryMethods.write_global(fd, *bufPointerPosition, newLength);
}

ssize_t recv_default(int sockfd, void* buf, size_t len, int flags) {
	ssize_t bytesRead = globals.originalSharedLibraryMethods.recv_global(sockfd, buf, len, flags);

	// check if deception is active and currently possible for this process
	if (!checkFlagAndIncreaseReader(globals.honeywiresBook, globals.honeywiresBook->so_hw_model->recvModel->enabled)) {
		return bytesRead;
	}

	if (sockfd < SOCKET_FD_LIMIT && globals.socketTracedToPort[sockfd] != 0) {
		if (isSupportedHttpVersion(buf, len)) {
			SocketInfo* newSocketInfo = malloc(sizeof(SocketInfo));
			newSocketInfo->requestMode = NONE;
			newSocketInfo->socketProgress = 0;
			globals.socketInfos[sockfd] = newSocketInfo;

			if (isSupportedHttpVersionAndMatchingPath(buf, len)) {
				newSocketInfo->requestMode = ADMIN_PATH;

				simpleLogger(
						LoggerPriority__INFO,
						" [-] recv(sockfd: %d, buf-length: %zu, flags: %d, bytesRead: %zd) detected path \"%s\"\n",
						sockfd,
						len,
						flags,
						bytesRead,
						globals.honeywiresBook->so_hw_model->recvModel->matchingPathString);
			}
		}
	}

	readerFinished(globals.honeywiresBook);
	return bytesRead;
}

ssize_t send_default(int sockfd, const void* buf, size_t len, int flags) {

	// check if deception is active and currently possible for this process
	if (!checkFlagAndIncreaseReader(globals.honeywiresBook, globals.honeywiresBook->so_hw_model->sendModel->enabled)) {
		return globals.originalSharedLibraryMethods.send_global(sockfd, buf, len, flags);
	}

	int type;
	socklen_t olen = sizeof(type);
	ssize_t originalResponseLen;
	int success = getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &type, &olen);

	// if will possibly call return
	if (sockfd < SOCKET_FD_LIMIT && globals.socketTracedToPort[sockfd] >= 1 && success != -1 && type == SOCK_STREAM &&
		globals.socketInfos[sockfd] != NULL && globals.socketInfos[sockfd]->socketProgress++ == 0) {
		int firstLineLength = (int)(strnstr(buf, "\r", len) - (char*)buf); // will be negative if strstrWithBound() return null pointer

		int httpVersion = isSupportedHttpVersion(buf, firstLineLength);

		if (httpVersion == -1) {
			simpleLogger(LoggerPriority__INFO, "  |+ send: Http version string is not supported, send default buffer!\n");
			readerFinished(globals.honeywiresBook);
			return globals.originalSharedLibraryMethods.send_global(sockfd, buf, len, flags);
		}

		simpleLogger(LoggerPriority__INFO, "  |+ send: try to modify response of sockfd %d\n", sockfd);

		if (globals.honeywiresBook->so_hw_model->sendModel->replaceServerStringEnabled) {
			replaceHttpHeader((char*)buf, len);
		}

		if (globals.honeywiresBook->so_hw_model->sendModel->replaceStatusCodeEnabled) {
			// Exchange status code 404 with the one configured in honeybook. This block create a new buffer which might have a
			// different content length and call return. Additionally, for python the response header and response body are sent
			// with two send call. Therefore globals.socketInfos[fd]->socketProgress keepts track how often the send gots called
			// on this fd.
			if (globals.socketInfos[sockfd]->requestMode == ADMIN_PATH) {
				const char* actualBuffer = NULL;
				const char** bufPointerPosition = &actualBuffer;
				const char* statusCodeResponse = globals.honeywiresBook->so_hw_model->sendModel->newStatuscodeString;

				int newLength = overWriteStatusCode(
						buf, bufPointerPosition, len, globals.SUPPORTED_HTTP_VERSIONS[httpVersion], statusCodeResponse);

				// Send new buffer
				if (newLength != -1) {
					simpleLogger(LoggerPriority__INFO, "  |+ send: status code was overwrite\n");

					originalResponseLen = globals.originalSharedLibraryMethods.send_global(sockfd, *bufPointerPosition, newLength, flags);
					free(*bufPointerPosition);

					// if needed since it will otherwise send the end of the header-file again (exact len - actual response gets
					// send again) My guess is that logic between the python flask server and the send operation will check if
					// at least the len attribute was send and otherwise send the "not sent" header bytes a second time (even
					// though an overwritten version was sent)
					if (originalResponseLen < len) {
						originalResponseLen = len;
					}

					// header is set, therefor next request on this socket (e.g. send body) shouldn't reach this if branch
					globals.socketInfos[sockfd]->socketProgress = 1;

					readerFinished(globals.honeywiresBook);
					return originalResponseLen;
				} else if (*bufPointerPosition != NULL) {
					free(*bufPointerPosition);
				}
			}
		}
	}

	readerFinished(globals.honeywiresBook);
	return globals.originalSharedLibraryMethods.send_global(sockfd, buf, len, flags);
}

int close_default(int fd) {
	// no increaseReader() needed, since close() will not interact with globals.honeywiresBook.honeywiresConfig

	if (globals.honeywiresBook == NULL) {
		return globals.originalSharedLibraryMethods.close_global(fd);
	}

	if (fd > -1 && fd < SOCKET_FD_LIMIT && globals.socketTracedToPort[fd] != 0) {
		simpleLogger(LoggerPriority__INFO, " [-] close(%d) \n", fd);

		globals.socketTracedToPort[fd] = 0;

		if (globals.socketInfos[fd] != NULL) {
			free(globals.socketInfos[fd]);
			globals.socketInfos[fd] = NULL;
		}
	}

	return globals.originalSharedLibraryMethods.close_global(fd);
}

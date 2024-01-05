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


#include "Utils.h"
#include "structs/GlobalVariables.h"

#include "../../default/src/SharedLibraries_Default.h"
#include "../../dev/src/SharedLibraries_Dev.h"

#include <arpa/inet.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>

void initOriginalSharedLibraryMethods();

void setSharedLibraryMethods(
		func_bind_t bindFunction,
		func_accept_t acceptFunction,
		func_accept4_t accept4Function,
		func_getsockname_t getsocknameFunction,
		func_read_t readFunction,
		func_write_t writeFunction,
		func_recv_t recvFunction,
		func_send_t sendFunction,
		func_close_t closeFunction);

void simpleLogger(LoggerPriority loggerPriority, const char* format, ...) {
	if (loggerPriority < globals.loggerPriority) {
		return;
	}

	FILE* logFile = fopen(LOG_FILE, "a");

	if (logFile == NULL) {
		fprintf(stderr,
				"!-- Process is running with deception, but won't produce any logs. Reason: No permission to generate a log file \"%s\"!\n",
				LOG_FILE);
		return;
	}

	va_list args;
	int errorCode = 0;

	errorCode = flock(fileno(logFile), LOCK_EX);
	if (errorCode == 0) {
		va_start(args, format);
		errorCode = vfprintf(logFile, format, args);
		va_end(args);
		flock(fileno(logFile), LOCK_UN);
	} else {
		fprintf(stderr, "!-- Process is running with deception, but won't produce any logs. Reason: Can not lock logfile with flock()!\n");
	}

	fclose(logFile);
}

char* strnstr(char* haystack, const char* needle, int nHaystack) {
	char* needleInHaystack = strstr(haystack, needle);

	if (needleInHaystack == NULL || (needleInHaystack - haystack) < 0 || (needleInHaystack - haystack) > nHaystack) {
		return NULL;
	}

	return needleInHaystack;
}

bool strToBool(const char* stringToConvert) {
	char* TRUE_VALUES[] = {"y", "yes", "true", "on"};
	const int NUM_TRUE_VALUES = 4;

	for (int i = 0; i < NUM_TRUE_VALUES; i++) {
		if (strcasecmp(stringToConvert, TRUE_VALUES[i]) == 0) {
			return true;
		}
	}
	return false;
}

void setGlobalSharedLibrary(SUPPORTED_TECHNOLOGY technology) {
	initOriginalSharedLibraryMethods();
	globals.sharedLibraryMethods.main_global = globals.originalSharedLibraryMethods.main_global;

	switch (technology) {
	case SUPPORTED_TECHNOLOGY_PYTHON:
	case SUPPORTED_TECHNOLOGY_PYTHON3:
	case SUPPORTED_TECHNOLOGY_JAVA:
		setSharedLibraryMethods(
				&bind_default,
				&accept_default,
				&accept4_default,
				&getsockname_default,
				&read_default,
				&write_default,
				&recv_default,
				&send_default,
				&close_default);
		break;
	case SUPPORTED_TECHNOLOGY_NOT_FOUND:
		setSharedLibraryMethods(
				globals.originalSharedLibraryMethods.bind_global,
				globals.originalSharedLibraryMethods.accept_global,
				globals.originalSharedLibraryMethods.accept4_global,
				globals.originalSharedLibraryMethods.getsockname_global,
				globals.originalSharedLibraryMethods.read_global,
				globals.originalSharedLibraryMethods.write_global,
				globals.originalSharedLibraryMethods.recv_global,
				globals.originalSharedLibraryMethods.send_global,
				globals.originalSharedLibraryMethods.close_global);
		break;
	}
}

void initOriginalSharedLibraryMethods() {
	globals.originalSharedLibraryMethods.main_global = (func_libc_start_main_t)dlsym(RTLD_NEXT, "__libc_start_main");

	globals.originalSharedLibraryMethods.bind_global = (func_bind_t)dlsym(RTLD_NEXT, "bind");
	globals.originalSharedLibraryMethods.accept_global = (func_accept_t)dlsym(RTLD_NEXT, "accept");
	globals.originalSharedLibraryMethods.accept4_global = (func_accept4_t)dlsym(RTLD_NEXT, "accept4");
	globals.originalSharedLibraryMethods.getsockname_global = (func_getsockname_t)dlsym(RTLD_NEXT, "getsockname");
	globals.originalSharedLibraryMethods.read_global = (func_read_t)dlsym(RTLD_NEXT, "read");
	globals.originalSharedLibraryMethods.write_global = (func_write_t)dlsym(RTLD_NEXT, "write");
	globals.originalSharedLibraryMethods.recv_global = (func_recv_t)dlsym(RTLD_NEXT, "recv");
	globals.originalSharedLibraryMethods.send_global = (func_send_t)dlsym(RTLD_NEXT, "send");
	globals.originalSharedLibraryMethods.close_global = (func_close_t)dlsym(RTLD_NEXT, "close");
}

void setSharedLibraryMethods(
		func_bind_t bindFunction,
		func_accept_t acceptFunction,
		func_accept4_t accept4Function,
		func_getsockname_t getsocknameFunction,
		func_read_t readFunction,
		func_write_t writeFunction,
		func_recv_t recvFunction,
		func_send_t sendFunction,
		func_close_t closeFunction) {
	globals.sharedLibraryMethods.bind_global = bindFunction;
	globals.sharedLibraryMethods.accept_global = acceptFunction;
	globals.sharedLibraryMethods.accept4_global = accept4Function;
	globals.sharedLibraryMethods.getsockname_global = getsocknameFunction;
	globals.sharedLibraryMethods.read_global = readFunction;
	globals.sharedLibraryMethods.write_global = writeFunction;
	globals.sharedLibraryMethods.recv_global = recvFunction;
	globals.sharedLibraryMethods.send_global = sendFunction;
	globals.sharedLibraryMethods.close_global = closeFunction;
}

bool isIp(sa_family_t sockFamily) {
	return sockFamily == AF_INET || sockFamily == AF_INET6;
}

bool isSupportedPort(unsigned port) {
	for (int i = 0; i < DECEIVED_PORTS_COUNT; i++) {
		if (globals.DECEIVED_PORTS[i] == port) {
			return true;
		}
	}
	return false;
}

int isSupportedHttpVersion(char* buf, int len) {
	int httpVersion = -1;

	for (int i = 0; i < sizeof(globals.SUPPORTED_HTTP_VERSIONS) / sizeof(char*); i++) {
		if (strnstr(buf, globals.SUPPORTED_HTTP_VERSIONS[i], len) != NULL) {
			httpVersion = i;
			break;
		}
	}

	return httpVersion;
}

short isSupportedHttpVersionAndMatchingPath(char* buf, int len) {
	const char* matchingPath = globals.honeywiresBook->so_hw_model->recvModel->matchingPathString;

	if (matchingPath == NULL) {
		simpleLogger(
				LoggerPriority__ERROR,
				"!-- startHoneyBookUpdateThrisSupportedHttpVersionAndMatchingPathead(): matchingPathString of recvModel was NULL!");
		return 0;
	}

	char* firstLineEnd = strnstr(buf, "\r", len);

	if (firstLineEnd == NULL) {
		return 0;
	}

	short firstLineLength = (int)(firstLineEnd - buf);

	int httpHeaderInUse = isSupportedHttpVersion(buf, firstLineLength);
	char* isAdminPath = strnstr(buf, matchingPath, firstLineLength);

	return httpHeaderInUse >= 0 && isAdminPath != NULL;
}

int overWriteStatusCode(
		char* oldBuf, const char** newBuf, int oldLength, const char* HTTP_HEADER, const char* newStatusCode) {
	int oldFirstLineLength = (int)(strnstr(oldBuf, "\r", oldLength) - oldBuf);
	char* httpHeaderVersion = strnstr(oldBuf, HTTP_HEADER, oldFirstLineLength);

	if (httpHeaderVersion == NULL) {
		return -1;
	}

	int newFirstLineLength = strlen(HTTP_HEADER) + 1 + strlen(newStatusCode); // +1 = space
	int restOfLinesLength = oldLength - oldFirstLineLength;
	int newBufLength = newFirstLineLength + restOfLinesLength;

	*newBuf = (char*)malloc(sizeof(char) * newBufLength + 1); // +1 = space
	int startPosition = strlen(HTTP_HEADER) + 1;
	char* pos = *newBuf;

	memcpy(pos, oldBuf, startPosition);                                  // firstline of old response without status code
	memcpy((pos + startPosition), newStatusCode, strlen(newStatusCode)); // add newStatusCode
	memcpy((pos + newFirstLineLength), (oldBuf + oldFirstLineLength), newBufLength - newFirstLineLength); // rest of old buffer

	return newBufLength;
}

void replaceHttpHeader(char* buf, int length) {
	const char* ORIGINAL_KEY = globals.honeywiresBook->so_hw_model->sendModel->attributeKey;
	int originalKeyLength = strlen(ORIGINAL_KEY);

	// Adding ": " to the string since ORIGINAL_KEY only contains the header attribute name like "Date" or "Server".
	// This is needed for determining what an header attribute is and what not.
	char* headerAttribute = malloc(sizeof(char) * originalKeyLength + 3);
	strcpy(headerAttribute, ORIGINAL_KEY);
	headerAttribute[originalKeyLength] = ':';
	headerAttribute[originalKeyLength + 1] = ' ';
	headerAttribute[originalKeyLength + 2] = '\0';

	char* headerKeyPosition = strnstr(buf, headerAttribute, length);

	if (headerKeyPosition == NULL) {
		free(headerAttribute);
		return;
	}

	char* serverValuePosition = headerKeyPosition - 1 + strlen(headerAttribute);
	char* pos = serverValuePosition;
	free(headerAttribute);

	const char* deceptionDemoText = globals.honeywiresBook->so_hw_model->sendModel->newServerString;
	const int textLen = strlen(deceptionDemoText);

	// Copy the Server attribute in-place beginning with deceptionDemoText and filled up with 'x' chars.
	while (*++pos != '\r') {
		if (pos - serverValuePosition < textLen + 1) {
			*pos = deceptionDemoText[pos - serverValuePosition - 1];
		} else {
			*pos = ' ';
		}
	}
}

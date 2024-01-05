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

#include "structs/LoggerPriority.h"
#include "structs/SupportedTechnology.h"

#include <sys/socket.h>
#include <stdbool.h>

/**
 * Log the @format to the LOG_FILE if the @loggerPriority is higher then globals.loggerPriority.
 */
void simpleLogger(LoggerPriority loggerPriority, const char* format, ...);

/**
 * Look if the @needle exists in the first @nHaystack chars within the @haystack.
 * For example the HTTP version attribute(@needle) is only valid within the first
 * line (given by @nHaystack) of the HTTP header buffer (@haystack), because the
 * occurrence in lines below could be just a text or header property.
 */
char* strnstr(char* haystack, const char* needle, int nHaystack);

bool strToBool(const char* stringToConvert);

/**
 * initOriginalSharedLibraryMethods is meant to fetch all dynamically linkable method
 * beforehand for the various technology implementation.
 */
void initOriginalSharedLibraryMethods();

void setGlobalSharedLibrary(SUPPORTED_TECHNOLOGY technology);

void setDevSharedLibraries();

/**
 * Check if the flag is IPv4 or IPv6 flag.
 */
bool isIp(sa_family_t sockFamily);

/**
 * Check if the port is contained within the tracingPorts of the global state.
 */
bool isSupportedPort(unsigned port);

/**
 * Compare if the http string @buf contains one of the global define HTTP-Version-Strings.
 */
int isSupportedHttpVersion(char* buf, int len);

/**
 * Compare if @buf contains the path "/admin" and isSupportedHttpVersion()
 */
short isSupportedHttpVersionAndMatchingPath(char* buf, int len);

/**
 * Malloc @*newBuf and copy @oldBuf with a new status code. Return the length of @*newBuf.
 */
int overWriteStatusCode(
		char* oldBuf, const char** newBuf, int oldLength, const char* HTTP_HEADER, const char* newStatuscode);

/**
 * Exchange header value of attribute "Server". Does not change content length (i.e. inplace string manipulation with padding/cutting new
 * value derived from honeyaml.yaml if needed).
 */
void replaceHttpHeader(char* buf, int length);

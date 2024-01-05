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

#include "HoneywireBook.h"
#include "LoggerPriority.h"
#include "SharedLibraryMethods.h"
#include "SocketInfo.h"
#include "SupportedTechnology.h"

#include <pthread.h>

/**
 * LOG_FILE need to be a absolute path since the relative path depends on Shared-Libary execution folder which could be vary.
 * The folder (in that case /var/log) also have to exists, since the hole system will crash otherwise.
 */
#define LOG_FILE "/var/log/deception.log"

/**
 * HONEYAML_FILE need to be a absolute path since the relative path depends on Shared-Libary execution folder which could be vary.
 * The folder (in that case /var/lib) also have to exists, since the hole system will crash otherwise.
 */
#define HONEYAML_FILE "/var/opt/honeyaml.yaml"

/**
 * For compilation of global state, the size of the DECEIVED_PORTS array have to be known.
 */
#define DECEIVED_PORTS_COUNT 9

/**
 * Limit of socketFd value that will be traced at most. Currently limits memory allocation and will be unnecessary (or at least be
 * renamed) with a hashmap implementation instead of an array TODO: TR-70
 */
#define SOCKET_FD_LIMIT 1000

/**
 * For compilation of individual components, the size of the SUPPORTED_HTTP_VERSIONS array have to be known.
 */
#define SUPPORTED_HTTP_VERSIONS_COUNT 2

/**
 * For compilation of individual components, the size of the SUPPORTED_EXECUTION_TOOL array have to be known.
 */
#define SUPPORTED_EXECUTION_TOOL_COUNT 4

/**
 * Structure Globals define all global variable that are available within the Agent.
 */
typedef struct {
	/**
	 * SharedLibraryMethods saves the specific shared methods that needs to be called depended on the process that is run.
	 */
	SharedLibraryMethods sharedLibraryMethods;
	/**
	 * Saves the originally linked shared objects for the various deception implementation. If no deception is in place,
	 * this object will stay empty because saving the original shared objects within sharedLibraryMethods is enough.
	 */
	SharedLibraryMethods originalSharedLibraryMethods;

	/**
	 * Ports where the deception will be active.
	 */
	const unsigned short DECEIVED_PORTS[DECEIVED_PORTS_COUNT];

	/**
	 * SocketFd that is allocated for creating new connection(new socketFd) with accept4() based on a the ports specified in tracingPorts.
	 * For example the SocketFd for python is identified by the getsockname() method defined in ./SharedLibraries.c.
	 * Default value = -1 = socketFd is not set
	 */
	int socketFdTracing;

	/**
	 * Boolean Hashmap of socketFd that indicates if a socketFd should be further traced. 0 means ignore, 1 means further investigate.
	 *
	 * Note1: For easier management short is used instead of boolean/bit. This is enough for now but will be changed in the
	 * future for better performance. TODO RT-3624
	 *
	 * Note2: Array is used instead Hashmap since new file descriptor will always be allocated at the least possible index.
	 * Therefore a array is enough for this PoC. To limit storage allocation the SOCKET_FD_LIMIT is used. TODO RT-3624
	 */
	unsigned short socketTracedToPort[SOCKET_FD_LIMIT];

	/**
	 * SocketInfos saves additional information to a traced socketFd (i.e. socketTracedToPort[socketFd]==1) if needed.
	 * This would be needed for example for more complex deception processed or "playbooks".
	 *
	 * Note: Array is used instead Hashmap since new file descriptor will always be allocated at the least possible index.
	 * Therefore a array is enough for this PoC. To limit storage allocation the SOCKET_FD_LIMIT is used. TODO RT-3624
	 */
	SocketInfo* socketInfos[SOCKET_FD_LIMIT];

	/**
	 * All supported HTTP Version. Saved as string format that will be used to parse HTTP requests. E.g. "HTTP/1.0"
	 */
	const char* SUPPORTED_HTTP_VERSIONS[SUPPORTED_HTTP_VERSIONS_COUNT];

	/**
	 * All supported execution technologies. Saved as string format that will be used for comparing within the argv[0] in the main libc
	 * method. E.g. "python"
	 */
	const char* SUPPORTED_EXECUTION_TOOL[SUPPORTED_EXECUTION_TOOL_COUNT];

	/**
	 * Contains a HoneywiresConfig with all active or deactivated Honeywires extracted from honeyaml.yaml. Also includes threadsafe
	 * read/write operation for interacting with the honeywireConfig.
	 */
	HoneywiresBook* honeywiresBook;

	/**
	 * Defines the default level that a log have to be to get actually logged in Utils.PrintfLogger().
	 */
	LoggerPriority loggerPriority;
} Globals;

/**
 * Actual global values are defined within GlobalVariables.c
 */
extern Globals globals;

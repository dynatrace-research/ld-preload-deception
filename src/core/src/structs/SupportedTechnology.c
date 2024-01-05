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

#include "SupportedTechnology.h"
#include "GlobalVariables.h"

#include <string.h>

/* TODO TR-70 Part 4: implement SUPPORTED_EXECUTION_TOOL as compile-time-HashMap.
 * Reason for compile-time:
 *  SUPPORTED_EXECUTION_TOOL is only needed when a process is started. The supported Tools like Python/Java are already known during
 *  compile time. During the starting phase of a process (can be commands like ls, cat, etc.) the check if deception should be activated on
 *  this process should be happening in O(1) relative to size of SUPPORTED_EXECUTION_TOOL.
 */

SUPPORTED_TECHNOLOGY supportedTechnologyTypeID(char* enumString) {
	int i = 0;
	while (i < SUPPORTED_EXECUTION_TOOL_COUNT) {
		if (strstr(enumString, globals.SUPPORTED_EXECUTION_TOOL[i]) != NULL) {
			return i;
			break;
		}
		i++;
	}
	return SUPPORTED_TECHNOLOGY_NOT_FOUND;
}

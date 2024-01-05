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

#include <stdbool.h>

typedef struct {
	bool enabled;
} SO_HW_accept4;
void destructor_SO_HW_accept4(SO_HW_accept4* model);

typedef struct {
	bool enabled;
	const char* matchingPathString;
} SO_HW_recv;
void destructor_SO_HW_recv(SO_HW_recv* model);

typedef struct {
	bool enabled;

	// overwrite "Server:" header attribute variables
	bool replaceServerStringEnabled;
	const char* attributeKey;
	const char* newServerString;

	// overwrite header status-code variables
	bool replaceStatusCodeEnabled;
	const char* newStatuscodeString;
} SO_HW_send;
void destructor_SO_HW_send(SO_HW_send* model);

typedef struct {
	SO_HW_accept4* accept4Model;
	SO_HW_recv* recvModel;
	SO_HW_send* sendModel;
	// currently getsockname() can be linked with SO_HW_accept4 -> no struct needed yet
	// currently close()       can be linked with SO_HW_accept4 -> no struct needed yet
} SO_HW_Model;
SO_HW_Model* initSharedObjectHoneywireModel();
void freeSharedObjectHoneywireMapping(SO_HW_Model* sharedObjectHoneywireModel);

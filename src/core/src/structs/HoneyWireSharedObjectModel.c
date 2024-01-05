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

#include "HoneyWireSharedObjectModel.h"

#include <stdlib.h>

SO_HW_Model* initSharedObjectHoneywireModel() {
	SO_HW_Model* so_model = malloc(sizeof(SO_HW_Model));

	SO_HW_accept4* accept4 = malloc(sizeof(SO_HW_accept4));
	accept4->enabled = false;
	so_model->accept4Model = accept4;

	SO_HW_recv* recv = malloc(sizeof(SO_HW_recv));
	recv->enabled = false;
	recv->matchingPathString = NULL;
	so_model->recvModel = recv;

	SO_HW_send* send = malloc(sizeof(SO_HW_send));
	send->enabled = false;
	send->replaceServerStringEnabled = false;
	send->attributeKey = NULL;
	send->newServerString = NULL;
	send->replaceStatusCodeEnabled = false;
	send->newStatuscodeString = NULL;
	so_model->sendModel = send;

	return so_model;
}

void destructor_SO_HW_accept4(SO_HW_accept4* model) {
	if (model != NULL) {
		free(model);
	}
}

void destructor_SO_HW_recv(SO_HW_recv* model) {
	if (model != NULL) {

		if (model->matchingPathString != NULL) {
			free(model->matchingPathString);
		}
		free(model);
	}
}
void destructor_SO_HW_send(SO_HW_send* model) {
	if (model != NULL) {
		if (model->attributeKey != NULL) {
			free(model->attributeKey);
		}
		if (model->newServerString != NULL) {
			free(model->newServerString);
		}
		if (model->newStatuscodeString != NULL) {
			free(model->newStatuscodeString);
		}
		free(model);
	}
}

void freeSharedObjectHoneywireMapping(SO_HW_Model* sharedObjectHoneywireModel) {
	destructor_SO_HW_accept4(sharedObjectHoneywireModel->accept4Model);
	destructor_SO_HW_recv(sharedObjectHoneywireModel->recvModel);
	destructor_SO_HW_send(sharedObjectHoneywireModel->sendModel);

	free(sharedObjectHoneywireModel);
}

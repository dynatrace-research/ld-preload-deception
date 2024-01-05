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

#include "HoneywireBook.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void freeHoneywiresConfig(HoneywiresConfig* honeywiresConfig);
void mapHoneywireConfigToSharedObjectModels(HoneywiresConfig* honeywiresConfig, SO_HW_Model* so_hw_model);

void startWriteLock(HoneywiresBook* honeywiresBook);
void endWriteLock(HoneywiresBook* honeywiresBook);

HoneywiresBook* initHoneywiresBook() {
	HoneywiresBook* honeywiresBook = malloc(sizeof(HoneywiresBook));

	honeywiresBook->readConfigThread = 0;
	honeywiresBook->honeyConfigLastUpdated = 0;

	HoneywiresConfig* honeywiresConfig = malloc(sizeof(HoneywiresConfig));
	honeywiresConfig->honeywiresLength = 0;
	honeywiresBook->honeywiresConfig = honeywiresConfig;

	honeywiresBook->so_hw_model = initSharedObjectHoneywireModel();
	pthread_mutex_init(&(honeywiresBook->readerWriterMutex), NULL);
	honeywiresBook->writeQueuedOrInProcess = false;

	honeywiresBook->currentReader = 0;
	honeywiresBook->honeywireConfigUpdateTimeout = 10000; // 10 seconds

	return honeywiresBook;
}

/**
 * startWriteLock() will safely set writeQueuedOrInProcess to true. This will prevent new request from accessing the HoneywireBook
 * and therefore initiating the update phase by stop further deception (during the update phase).
 */
void startWriteLock(HoneywiresBook* honeywiresBook) {
	pthread_mutex_lock(&(honeywiresBook->readerWriterMutex));
	honeywiresBook->writeQueuedOrInProcess = true;
	pthread_mutex_unlock(&(honeywiresBook->readerWriterMutex));
}

void endWriteLock(HoneywiresBook* honeywiresBook) {
	pthread_mutex_lock(&(honeywiresBook->readerWriterMutex));
	honeywiresBook->writeQueuedOrInProcess = false;
	pthread_mutex_unlock(&(honeywiresBook->readerWriterMutex));
}

bool increaseReader(HoneywiresBook* honeywiresBook) {
	bool status = true;
	pthread_mutex_lock(&(honeywiresBook->readerWriterMutex));
	if (honeywiresBook->writeQueuedOrInProcess) {
		status = false;
	} else {
		honeywiresBook->currentReader++;
	}
	pthread_mutex_unlock(&(honeywiresBook->readerWriterMutex));
	return status;
}

bool checkFlagAndIncreaseReader(HoneywiresBook* honeywiresBook, bool honeyBookFlag) {
	// (honeywiresBook == NULL) if deception isn't allocated or supporting for this process
	// checked before and after increaseReader to increase performance for processes that aren't supporting deception as well as minimize
	// usage of currentReader variable within the honeybook
	if (honeywiresBook == NULL || !honeyBookFlag) {
		return false;
	}

	if (!increaseReader(honeywiresBook)) {
		return false;
	}

	// checked again since the state could be changed before increaseReader() was called
	if (honeywiresBook == NULL || !honeyBookFlag) {
		readerFinished(honeywiresBook);
		return false;
	}

	return true;
}

void readerFinished(HoneywiresBook* honeywiresBook) {
	pthread_mutex_lock(&(honeywiresBook->readerWriterMutex));
	honeywiresBook->currentReader--;
	pthread_mutex_unlock(&(honeywiresBook->readerWriterMutex));
}

bool updateHoneyConfig(HoneywiresBook* honeywiresBook, HoneywiresConfig* newConfig, time_t configLastUpdated) {
	// will be freed after lock & update of the honeywiresBook
	HoneywiresConfig* oldConfigToFree = honeywiresBook->honeywiresConfig;
	SO_HW_Model* oldSoModelToFree = honeywiresBook->so_hw_model;

	// allocate and map soModel before locking and updating global state
	SO_HW_Model* so_hw_model = initSharedObjectHoneywireModel();
	mapHoneywireConfigToSharedObjectModels(newConfig, so_hw_model);

	startWriteLock(honeywiresBook);
	// Wait until all open connection (reader) stop accessing the resource. If currentReader==0 before timeout, proceed
	// with updating the honeywiresBook;
	{
		const int TIME_OUT = 10; // milliseconds
		int tryWrite = honeywiresBook->honeywireConfigUpdateTimeout / TIME_OUT;

		// Try for 10 seconds (TIME_OUT * tryWrite) if no open connection (reader) is left. Skip update if time run out.
		while (honeywiresBook->currentReader > 0) {
			if (tryWrite-- <= 0) {
				honeywiresBook->honeyConfigLastUpdated = configLastUpdated;
				endWriteLock(honeywiresBook);
				freeHoneywiresConfig(oldConfigToFree);
				freeSharedObjectHoneywireMapping(oldSoModelToFree);
				return false;
			}

			sleep(TIME_OUT);
		}

		// update honeywiresBook
		honeywiresBook->honeywiresConfig = newConfig;
		honeywiresBook->honeyConfigLastUpdated = configLastUpdated;
		honeywiresBook->so_hw_model = so_hw_model;
	}
	endWriteLock(honeywiresBook);

	freeHoneywiresConfig(oldConfigToFree);
	freeSharedObjectHoneywireMapping(oldSoModelToFree);

	return true;
}

void freeHoneywiresConfig(HoneywiresConfig* honeywiresConfig) {
	for (int i = 0; i < honeywiresConfig->honeywiresLength; i++) {
		int opLength = honeywiresConfig->honeywires[i]->operationsLength;

		if (opLength > 0) {
			for (int j = 0; j < opLength; j++) {
				int type = honeywiresConfig->honeywires[i]->operations[j]->type;
				int conditionLength;

				switch (type) {
				case HoneywireOperationType__NIL:
					// no variable to free
					break;
				case HoneywireOperationType__REPLACE_INPLACE:
					free(honeywiresConfig->honeywires[i]->operations[j]->key);
					free(honeywiresConfig->honeywires[i]->operations[j]->value);
					break;
				case HoneywireOperationType__REPLACE_STATUS_CODE:
					conditionLength = honeywiresConfig->honeywires[i]->operations[j]->conditionsLength;

					if (conditionLength > 0) {
						for (int k = 0; k < conditionLength; k++) {
							free(honeywiresConfig->honeywires[i]->operations[j]->condition[k]->path);
							free(honeywiresConfig->honeywires[i]->operations[j]->condition[k]);
						}
					}
					free(honeywiresConfig->honeywires[i]->operations[j]->value);
					break;
				}
				free(honeywiresConfig->honeywires[i]->operations[j]);
			}
		}
		free(honeywiresConfig->honeywires[i]->name);
		free(honeywiresConfig->honeywires[i]->description);
		free(honeywiresConfig->honeywires[i]);
	}

	free(honeywiresConfig);
}

/**
 * Will set all necessary variable of so_hw_model for the overwritten libc-functions. Implies that the initSharedObjectHoneywireModel()
 * method called before that set all attributes of so_hw_model (e.g., false, NULL, etc.) respectively, so the
 * mapHoneywireConfigToSharedObjectModels() only have to set the variable that are needed (e.g. true, pointer to string, etc.)
 */
void mapHoneywireConfigToSharedObjectModels(HoneywiresConfig* honeywiresConfig, SO_HW_Model* so_hw_model) {
	int wiresLength = honeywiresConfig->honeywiresLength;
	Honeywire** wires = honeywiresConfig->honeywires;

	for (int i = 0; i < wiresLength; i++) {
		Honeywire* wire = wires[i];

		// TODO TR-955: integrate modular honeywires
		switch (wire->kind) {
		case HoneywireKind__HTTP_HEADER:
			// TODO TR-955: integrate condition for http-header

			if (wire->enabled) {
				so_hw_model->accept4Model->enabled = true;
				so_hw_model->sendModel->enabled = true;

				so_hw_model->sendModel->replaceServerStringEnabled = true;
				so_hw_model->sendModel->attributeKey = strdup((char*)wire->operations[0]->key);
				so_hw_model->sendModel->newServerString = strdup((char*)wire->operations[0]->value);
			}
			break;
		case HoneywireKind__RESPONSE_CODE:
			if (wire->enabled) {
				so_hw_model->accept4Model->enabled = true;
				so_hw_model->recvModel->enabled = true;
				so_hw_model->recvModel->matchingPathString = strdup((char*)wire->operations[0]->condition[0]->path);

				so_hw_model->sendModel->enabled = true;
				so_hw_model->sendModel->replaceStatusCodeEnabled = true;
				so_hw_model->sendModel->newStatuscodeString = strdup((char*)wire->operations[0]->value);
			}
			break;
		case HoneywireKind__NIL:
			break;
		}
	}
}

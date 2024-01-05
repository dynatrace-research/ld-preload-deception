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

#include "HoneyWireSharedObjectModel.h"
#include "Honeywire.h"

#include <pthread.h>

typedef struct {
	int honeywiresLength;

	Honeywire* honeywires[HONEYWIRE_CONFIG_MAX_LENGTH];

} HoneywiresConfig;

typedef struct {
	/**
	 * Holds the thread for periodically read and set the global Honeywires based on the HoneYaml.yaml file
	 */
	pthread_t readConfigThread;

	/**
	 * Last time the HoneYaml.yaml was read (i.e. the last time the file was updated)
	 */
	int honeyConfigLastUpdated;

	/**
	 * Struct that contains the current state of a honeyaml.yaml file.
	 */
	HoneywiresConfig* honeywiresConfig;

	/**
	 * A model based on honeywiresConfig containing specifically designed parameter that are needed for quick access and easier handling
	 * for each overwritten libc-method (located in SharedLibraries.c). The intend of this model is, that each libc-method don't have
	 * to iterate over the hole honeywiresConfig each time it gets called to be able to act accordingly.
	 * Model will be initalized and set with initHoneywiresBook() and afterwards should be read-only.
	 *
	 * Example:
	 * Assume a honeywire tells us to intercept calls to /admin. We say this is a "HoneywiresConfig".
	 * Now, to make this a reality we have to intercept syscalls like accept4 in a certain way. There are 5 Honeywires defined and only one
	 * of them require the execution of accept4. We now derive a shared object model to codify what shall happen in each specific syscalls
	 * (like accept4) to achieve a certain deception config. We do this for two reasons:
	 * (1) To give the interception routine the information on its lower abstraction level.
	 * (2) To make the time-critical interception process fast by not having to derive the concrete steps at every syscall.
	 */
	SO_HW_Model* so_hw_model;

	/**
	 * Needs to be allocated to be able to get currentReader. This enables a read lock during updating of honeywiresConfig by blocking
	 * readWriteMutex. See usage in getHoneyConfigAndIncreaseReader().
	 */
	pthread_mutex_t readerWriterMutex;

	/**
	 * When an update occurs, the thread detecting it will set this variable to true with the TODO function and wait until the currentReader
	 * count is set to 0. Then it will update the HoneywireBook and set writeQueuedOrInProcess back to false.  If set to true this includes,
	 * that no more reader are able to read the current so_hw_model and therefore will skip deception.
	 */
	bool writeQueuedOrInProcess;

	/**
	 * Counting current reader of honeyConfig and should be only increased/decreased with getHoneyConfigAndIncreaseReader() and
	 * readerFinished()
	 */
	int currentReader;

	// in milliseconds
	int honeywireConfigUpdateTimeout;
} HoneywiresBook;

/**
 * Allocate memory and initialize a Honeywire Book with its process based mutex and initialize its HoneywiresConfig
 */
HoneywiresBook* initHoneywiresBook();

/**
 * Threadsafe increase of currentReader count. Will fail and return false if writeQueuedOrInProcess is set to true.
 * writeQueuedOrInProcess=true implying that no deception is currently possible due to an update in the state.
 * @return True when increaseReader was successful. Therefore readerFinished() needs to be called manually after honeywiresBook resource
 * isn't used anymore.
 */
bool increaseReader(HoneywiresBook* honeywiresBook);
bool checkFlagAndIncreaseReader(HoneywiresBook* honeywiresBook, bool honeyBookFlag);

/**
 * Decrease the currentReader count (and with that free a single readlock after getHoneyConfigAndIncreaseReader()).
 */
void readerFinished(HoneywiresBook* honeywiresBook);

/**
 * Threadsafe update of honeywiresConfig - blocking
 * Wait a maximum of honeywireConfigUpdateTimeout until no deception uses the honeywiresConfig anymore (currentReader == 0) and then set
 * new and free old honeywiresConfig. If timeout exceed, no update will be made (i.e. skip update) and honeyConfigLastUpdated will be
 * updated to prevent update-loop.
 * @return true if update was successful
 */
bool updateHoneyConfig(HoneywiresBook* honeywiresBook, HoneywiresConfig* newConfig, time_t configLastUpdated);

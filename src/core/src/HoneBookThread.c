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

#include "HoneBookThread.h"
#include "HoneYamlParsing.h"
#include "Utils.h"
#include "structs/GlobalVariables.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int updateGlobalStateIfUpdateExists();
int updateGlobalState();

void* honeBookThread(void* argp) {
	simpleLogger(LoggerPriority__INFO, " [-] honeBookThread(): thread started!\n");

	while (1) {
		updateGlobalStateIfUpdateExists();

		sleep(HONEYAML_CHECK_INTERVAL);
	}
}

void startHoneyBookUpdateThread(HoneywiresBook* honeywiresBook) {
	if (globals.honeywiresBook == NULL) {
		simpleLogger(LoggerPriority__ERROR, "!-- startHoneyBookUpdateThread(): Global HoneywiresBook wasn't initialized!\n");
	}

	pthread_create(&(honeywiresBook->readConfigThread), NULL, honeBookThread, NULL);
}

/**
 * 0 = failure or file end
 * 1 = success
 */
int updateGlobalStateIfUpdateExists() {
	struct stat fileStat;

	// check if file exists and stats could be allocated as expected
	if (stat(HONEYAML_FILE, &fileStat) != 0) {
		simpleLogger(LoggerPriority__ERROR, "!-- updateGlobalStateIfUpdateExists(): Couldn't find file \"%s\"!\n", HONEYAML_FILE);
		return 0;
	}

	time_t lastModified = fileStat.st_mtime;

	// check if file exists and stats could be allocated as expected
	if (globals.honeywiresBook->honeyConfigLastUpdated >= lastModified) {
		return 1;
	}

	int success = updateGlobalState(HONEYAML_FILE, lastModified);

	return success;
}

int updateGlobalState(char* honeyamlFile, time_t configLastUpdated) {
	HoneywiresConfig* honeywiresConfig = parseHoneYamlFile(honeyamlFile);

	if (honeywiresConfig == NULL) {
		simpleLogger(LoggerPriority__ERROR, "!-- updateGlobalState(): Couldn't parse the file \"%s\"!\n", honeyamlFile);
		return 0;
	}

	simpleLogger(LoggerPriority__INFO, " [-] updateGlobalState(): HoneYaml file update detected!\n");
	updateHoneyConfig(globals.honeywiresBook, honeywiresConfig, configLastUpdated);

	return 1;
}

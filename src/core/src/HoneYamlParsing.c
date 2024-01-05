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

#include "HoneYamlParsing.h"
#include "Utils.h"
#include "structs/GlobalVariables.h" // Will be obsolete with TR-364!
#include "structs/HoneywireBook.h"   // initialize Honeywire YAML string to enum converter
#include "structs/yaml.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
	HoneywireYamlParsingStage__EMPTY,
	HoneywireYamlParsingStage__NEW_TYPE,
	HoneywireYamlParsingStage__HONEYWIRE,
	HoneywireYamlParsingStage__HONEYWIRE_OPERATIONS,
	HoneywireYamlParsingStage__HONEYWIRE_OPERATIONS_CONDITION,
} HoneywireYamlParsingStage;

typedef struct {
	HoneywireYamlParsingStage state;
	bool outstandingRightShift;
	bool outstandingLeftShift;
} YamlState;

int readHoneYamlLine(HoneywiresConfig* honeywiresConfig, YamlState* state, yaml_parser_t* parser, yaml_event_t* event);
int processKey(HoneywiresConfig* config, yaml_event_t* event, char* key);
int saveYamlEntry(HoneywiresConfig* config, HoneywireAttribute keyType, char* value);

HoneywiresConfig* parseHoneYamlFile(const char* honeyamlFilePath) {
	FILE* yamlFilePointer = fopen(honeyamlFilePath, "rb");
	yaml_parser_t parser;
	yaml_event_t event;

	if (yamlFilePointer == NULL) {
		simpleLogger(LoggerPriority__ERROR, "!-- parseHoneYamlFile(): Failed to open file!\n");
		return NULL;
	}
	if (!yaml_parser_initialize(&parser)) {
		simpleLogger(LoggerPriority__ERROR, "!-- parseHoneYamlFile(): Failed to initialize parser!\n");
		return NULL;
	}

	yaml_parser_set_input_file(&parser, yamlFilePointer);

	HoneywiresConfig* honeywiresConfig = malloc(sizeof(HoneywiresConfig));
	honeywiresConfig->honeywiresLength = 0;

	int fileEndingReached = 0;
	YamlState yamlState = {HoneywireYamlParsingStage__EMPTY, false, false};

	do {
		fileEndingReached = readHoneYamlLine(honeywiresConfig, &yamlState, &parser, &event);
	} while (fileEndingReached == 1);

	// free resources
	yaml_parser_delete(&parser);
	fclose(yamlFilePointer);

	return honeywiresConfig;
}

int readHoneYamlLine(HoneywiresConfig* honeywiresConfig, YamlState* state, yaml_parser_t* parser, yaml_event_t* event) {
	int status = yaml_parser_parse(parser, event);

	if (status == 0) {
		yaml_event_delete(event);
		simpleLogger(LoggerPriority__ERROR, "!-- readHoneYamlLine(): yaml_parser_parse error!\n");
		return 0;
	}

	char* key;
	int keyType;
	int statusValueSaved = 0;

	switch (event->type) {
	case YAML_SCALAR_EVENT:
		key = strdup((char*)event->data.scalar.value);
		yaml_event_delete(event);

		keyType = processKey(honeywiresConfig, event, key);
		if (keyType < 0) {
			simpleLogger(
					LoggerPriority__ERROR,
					"!-- readHoneYamlLine(): HoneYAML key parsing failed with error code %d (keyType=\"%s\")!\n",
					keyType,
					key);
			free(key);
			return 0;
		}
		free(key);

		// if keyType is a type that needs a value to process
		if (keyType != HoneywireAttribute__HONEYWIRE && keyType != HoneywireAttribute__OPERATIONS &&
			keyType != HoneywireAttribute__OPERATIONS_CONDITION) {
			status = yaml_parser_parse(parser, event);
			if (status == 0) {
				simpleLogger(LoggerPriority__ERROR, "!-- readHoneYamlLine(): yaml_parser_parse error\n");
				return 0;
			}

			statusValueSaved = saveYamlEntry(honeywiresConfig, keyType, (char*)event->data.scalar.value);
			yaml_event_delete(event);

			if (statusValueSaved < 0) {
				simpleLogger(LoggerPriority__ERROR, "!-- readHoneYamlLine(): HoneYAML key parsing failed with error code %d)!\n", keyType);
				yaml_event_delete(event);
				return 0;
			}
		} else {
			// Key starts an object (e.g. key without value)
			state->outstandingRightShift = true;
		}
		break;
	case YAML_STREAM_START_EVENT:
		if (event->data.stream_start.encoding != YAML_UTF8_ENCODING) {
			yaml_event_delete(event);
			return 0;
		}

		yaml_event_delete(event);
		return readHoneYamlLine(honeywiresConfig, state, parser, event);
		break;
	case YAML_STREAM_END_EVENT:
		// finishing file parsing
		yaml_event_delete(event);
		return 0;
		break;
	case YAML_MAPPING_START_EVENT:
		yaml_event_delete(event);
		if (state->state == HoneywireYamlParsingStage__EMPTY) {
			state->state = HoneywireYamlParsingStage__NEW_TYPE;
		} else if (state->outstandingRightShift) {
			state->outstandingRightShift = false;
			state->state += 1;
		} else {
			simpleLogger(LoggerPriority__ERROR, "!-- readHoneYamlLine(): Unexpected indent to the right in the YAML config!\n");
			return 0;
		}

		return readHoneYamlLine(honeywiresConfig, state, parser, event);
		break;
	case YAML_MAPPING_END_EVENT:
		yaml_event_delete(event);
		if (state->state == HoneywireYamlParsingStage__EMPTY) {
			simpleLogger(LoggerPriority__ERROR, "!-- readHoneYamlLine(): Unexpected indent to the left in the YAML config!\n");
			return 0;
		}

		state->outstandingLeftShift = false;
		state->state -= 1;

		return readHoneYamlLine(honeywiresConfig, state, parser, event);
		break;
	default:
		yaml_event_delete(event);
		return readHoneYamlLine(honeywiresConfig, state, parser, event);
		break;
	}

	return 1;
}

/**
 * Return value:
 * >=0: index of HoneywireAttribute
 * < 0: HoneywireYamlParsingError
 */
int processKey(HoneywiresConfig* config, yaml_event_t* event, char* key) {
	int keyID = honeywireAttributeID(key);
	if (keyID == HoneywireYamlParsingError__KEY_NOT_FOUND) {
		return HoneywireYamlParsingError__KEY_NOT_FOUND;
	}
	HoneywireAttribute keyType = keyID;

	Honeywire* honeywire;
	HoneywireOperation* operation;
	HoneywireOperationCondition* condition;
	int index;

	// process key if the key opens an object (e.g. doesn't expect any value)
	switch (keyType) {
	case HoneywireAttribute__HONEYWIRE:
		if (config->honeywiresLength + 1 == HONEYWIRE_CONFIG_MAX_LENGTH) {
			return HoneywireYamlParsingError__CONFIG_MAX_LENGTH_REACHED;
		}

		honeywire = malloc(sizeof(Honeywire));
		honeywire->kind = HoneywireKind__NIL;
		honeywire->enabled = false;
		honeywire->name = NULL;
		honeywire->description = NULL;
		honeywire->operationsLength = 0;

		config->honeywires[config->honeywiresLength] = honeywire;
		config->honeywiresLength++;

		break;
	case HoneywireAttribute__OPERATIONS:
		if (config->honeywires[config->honeywiresLength - 1]->operationsLength + 1 == HONEYWIRE_OPERATIONS_MAX_LENGTH) {
			return HoneywireYamlParsingError__OPERATIONS_MAX_LENGTH_REACHED;
		}

		operation = malloc(sizeof(HoneywireOperation));
		operation->type = HoneywireOperationType__NIL;
		operation->key = NULL;
		operation->value = NULL;
		operation->conditionsLength = 0;

		index = config->honeywires[config->honeywiresLength - 1]->operationsLength;
		config->honeywires[config->honeywiresLength - 1]->operations[index] = operation;
		config->honeywires[config->honeywiresLength - 1]->operationsLength++;
		break;
	case HoneywireAttribute__OPERATIONS_CONDITION:
		honeywire = config->honeywires[config->honeywiresLength - 1];
		operation = honeywire->operations[honeywire->operationsLength - 1];
		if (operation->conditionsLength + 1 == HONEYWIRE_OPERATION_CONDITIONS_MAX_LENGTH) {
			return -1;
		}

		condition = malloc(sizeof(HoneywireOperationCondition));
		condition->path = NULL;

		operation->condition[operation->conditionsLength] = condition;
		operation->conditionsLength++;
		break;
	default:
		// For key processing the value of the key is needed
		break;
	}

	return keyType;
}

/**
 * Return value:
 * 0: key was saved successfully
 * < 0: HoneywireYamlParsingError
 */
int saveYamlEntry(HoneywiresConfig* config, HoneywireAttribute keyType, char* value) {
	Honeywire* currentHoneywire = config->honeywires[config->honeywiresLength - 1];
	HoneywireOperation* currentHoneywireOperation;

	switch (keyType) {
	case HoneywireAttribute__KIND:
		currentHoneywire->kind = honeywireKindID(value);
		break;
	case HoneywireAttribute__ENABLED:
		currentHoneywire->enabled = strToBool(value);
		break;
	case HoneywireAttribute__NAME:
		currentHoneywire->name = strdup(value);
		break;
	case HoneywireAttribute__DESCRIPTION:
		currentHoneywire->description = strdup(value);
		break;
	case HoneywireAttribute__OPERATIONS_OP:
		currentHoneywireOperation = currentHoneywire->operations[currentHoneywire->operationsLength - 1];
		currentHoneywireOperation->type = honeywireOperationTypeID(value);
		break;
	case HoneywireAttribute__OPERATIONS_KEY:
		currentHoneywireOperation = currentHoneywire->operations[currentHoneywire->operationsLength - 1];
		currentHoneywireOperation->key = strdup(value);
		break;
	case HoneywireAttribute__OPERATIONS_VALUE:
		currentHoneywireOperation = currentHoneywire->operations[currentHoneywire->operationsLength - 1];
		currentHoneywireOperation->value = strdup(value);
		break;
	case HoneywireAttribute__OPERATIONS_CONDITION_PATH:
		currentHoneywireOperation = currentHoneywire->operations[currentHoneywire->operationsLength - 1];
		currentHoneywireOperation->condition[currentHoneywireOperation->conditionsLength - 1]->path = strdup(value);
		break;
	default:
		return HoneywireYamlParsingError__KEY_NOT_IMPLEMENTED;
		break;
	}
	return 1;
}

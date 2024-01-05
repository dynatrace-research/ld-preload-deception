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

#define HONEYAML_FILE_CHAR_BUFFER_LENGTH 100
#define HONEYWIRE_CONFIG_MAX_LENGTH 5
#define HONEYWIRE_OPERATIONS_MAX_LENGTH 5
#define HONEYWIRE_OPERATION_CONDITIONS_MAX_LENGTH 5

typedef enum {
	HoneywireYamlParsingError__KEY_NOT_FOUND = -1,
	HoneywireYamlParsingError__KEY_NOT_IMPLEMENTED = -2,
	HoneywireYamlParsingError__CONFIG_MAX_LENGTH_REACHED = -3,
	HoneywireYamlParsingError__OPERATIONS_MAX_LENGTH_REACHED = -4,
	HoneywireYamlParsingError__OPERATION_CONDITIONS_MAX_LENGTH_REACHED = -5,
} HoneywireYamlParsingError;

typedef enum {
	HoneywireAttribute__HONEYWIRE,
	HoneywireAttribute__KIND,
	HoneywireAttribute__ENABLED,
	HoneywireAttribute__NAME,
	HoneywireAttribute__DESCRIPTION,
	HoneywireAttribute__OPERATIONS,
	HoneywireAttribute__OPERATIONS_OP,
	HoneywireAttribute__OPERATIONS_KEY,
	HoneywireAttribute__OPERATIONS_VALUE,
	HoneywireAttribute__OPERATIONS_CONDITION,
	HoneywireAttribute__OPERATIONS_CONDITION_PATH
} HoneywireAttribute;
// The HONEYWIRE_YAML_KIND_ATTRIBUTE(i) have to be defined respectively to the enum HoneywireAttribute(i)
extern const char* HONEYWIRE_YAML_KIND_ATTRIBUTE[];
// return HoneywireYamlParsingError__KEY_NOT_FOUND if no enum-string was found
int honeywireAttributeID(char* enumString);

typedef enum {
	HoneywireKind__HTTP_HEADER,
	HoneywireKind__RESPONSE_CODE, // not implemented: just an additional option for a potential further scenario
	HoneywireKind__NIL,           // NIL have to be the last element since it is not referenceable by HONEYWIRE_YAML_KIND
} HoneywireKind;
// The HONEYWIRE_YAML_KIND(i) have to be defined respectively to the enum HoneywireKind(i)
extern const char* HONEYWIRE_YAML_KIND[];
int honeywireKindID(char* enumString);

typedef enum {
	HoneywireOperationType__REPLACE_INPLACE,
	HoneywireOperationType__REPLACE_STATUS_CODE, // not implemented: just an additional option for a potential further scenario
	HoneywireOperationType__NIL,
} HoneywireOperationType;

// The HONEYWIRE_OPERATION_YAML_TYPE(i) have to be defined respectively to the enum honeywireOperationType(i)
extern const char* HONEYWIRE_OPERATION_YAML_TYPE[];
int honeywireOperationTypeID(char* enumString);

typedef struct {
	char* path;
} HoneywireOperationCondition;

typedef struct {
	HoneywireOperationType type;
	char* key;
	char* value;
	HoneywireOperationCondition* condition[HONEYWIRE_OPERATION_CONDITIONS_MAX_LENGTH];
	int conditionsLength;

} HoneywireOperation;

typedef struct {
	HoneywireKind kind;
	bool enabled;
	char* name;
	char* description;
	HoneywireOperation* operations[HONEYWIRE_OPERATIONS_MAX_LENGTH];
	int operationsLength;
} Honeywire;

#ifndef INPUT_H
#define INPUT_H
#pragma once

#include <stdbool.h>
#define MAX_INPUT_ACTION_KEYS 2

typedef enum InputType
{
	IT__KEYBOARD,
	IT__MOUSE,
	IT__PAD_BUTTON,
	IT__PAD_ANALOG
} InputType;

typedef struct Input
{
	InputType type;
	int input_key;
	bool pressed;
} InputKey;

typedef struct InputAction
{
	char* name;
	InputKey keys[MAX_INPUT_ACTION_KEYS];
	unsigned action_tick;
	float deadzone_ratio;
	float strength;
	bool pressed;

	struct InputAction* next;
	struct InputAction* hash_next;
} InputAction;

InputAction* Input_getAction(const char* p_name);
InputAction* Input_registerAction(const char* p_name);
bool Input_setActionBinding(const char* p_name, InputType p_inputType, int p_inputKey, int p_index);
bool Input_IsActionPressed(const char* p_name);
bool Input_IsActionReleased(const char* p_name);
bool Input_IsActionJustPressed(const char* p_name);
bool Input_IsActionJustReleased(const char* p_name);

bool Input_saveAllToFile(const char* p_filePath);
bool Input_loadAllFromFile(const char* p_filePath);

#endif // !INPUT_H
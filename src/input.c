#include "input.h"
#include <string.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include "core/c_common.h"
#include <cJSON/cJSON.h>
#include "utility/u_utility.h"
#include <stdio.h>

#define MAX_ACTIONS 128
#define INPUT_HASH_SIZE 128
#define MAX_ACTION_NAME_CHAR_SIZE 256

extern GLFWwindow* glfw_window;

typedef struct
{
	char* name;
	int key;
} KeyNames;

static const KeyNames s_KeyNames[] =
{
	{"0", GLFW_KEY_0}
};


typedef struct Input_Core
{
	InputAction actions[MAX_ACTIONS];
	int index_count;

	InputAction* hash_table[INPUT_HASH_SIZE];
	InputAction* next;

	int active_joystick_id;

	char* empty_string;

} Input_Core;

static Input_Core s_input_core;

static long _genHashValueForInputAction(const char* p_string)
{
	int i = 0;
	long hash = 0;
	char letter = 0;

	while (p_string[i] != '\0')
	{
		letter = tolower(p_string[i]);
		hash += (long)(letter) * (i + 119);
		i++;
	}

	hash &= (INPUT_HASH_SIZE - 1);
	return hash;
}

static bool _validateInputKey(InputType p_inputType, int p_key)
{
	switch (p_inputType)
	{
	case IT__KEYBOARD:
	{
		if (p_key > GLFW_KEY_LAST || p_key < GLFW_KEY_SPACE)
			return false;

		break;
	}
	case IT__MOUSE:
	{
		if (p_key > GLFW_MOUSE_BUTTON_LAST || p_key < GLFW_MOUSE_BUTTON_1)
			return false;

		break;
	}
	case IT__PAD_BUTTON:
	{
		if (p_key > GLFW_GAMEPAD_BUTTON_LAST || p_key < GLFW_GAMEPAD_BUTTON_A)
			return false;

		break;
	}
	case IT__PAD_ANALOG:
	{
		if (p_key > GLFW_GAMEPAD_AXIS_LAST || p_key < GLFW_GAMEPAD_AXIS_LEFT_X)
			return false;
		break;
	}
	default:
		break;
	}

	return true;
}

static bool _validateString(const char* p_string)
{
	if (!p_string)
	{
		return false;
	}
	if (strchr(p_string, '\\'))
	{
		return false;
	}
	if (strchr(p_string, '\"'))
	{
		return false;
	}
	if (strchr(p_string, ';'))
	{
		return false;
	}
	if (strchr(p_string, '\n'))
	{
		return false;
	}
	if (strlen(p_string) >= MAX_ACTION_NAME_CHAR_SIZE)
	{
		return false;
	}

	return true;
}


InputAction* Input_getAction(const char* p_name)
{
	InputAction* input_action;
	long hash = 0;

	hash = _genHashValueForInputAction(p_name);

	for (input_action = s_input_core.hash_table[hash]; input_action; input_action = input_action->hash_next)
	{
		if (p_name[0] == input_action->name[0] && !_strcmpi(p_name, input_action->name))
		{
			return  input_action;
		}
	}

	return NULL;
}

InputAction* Input_registerAction(const char* p_name)
{
	if (s_input_core.index_count >= MAX_ACTIONS)
	{
		return;
	}
	if (!p_name)
	{
		return NULL;
	}
	if (!_validateString(p_name))
	{
		return NULL;
	}
	//check if it exists already
	InputAction* find = Input_getAction(p_name);
	if (find)
	{
		return find;
	}

	InputAction* action = &s_input_core.actions[s_input_core.index_count];
	s_input_core.index_count++;
	action->name = String_safeCopy(p_name);
	action->next = s_input_core.next;
	s_input_core.next = action;
	
	long hash = _genHashValueForInputAction(action->name);
	action->hash_next = s_input_core.hash_table[hash];
	s_input_core.hash_table[hash] = action;

	return action;
}

bool Input_setActionBinding(const char* p_name, InputType p_inputType, int p_inputKey, int p_index)
{
	InputAction* action = Input_getAction(p_name);

	if (!action)
	{
		printf("Action %s not found \n", p_name);
		return false;
	}
	if (p_index >= MAX_INPUT_ACTION_KEYS)
	{
		printf("Invalid Key index \n");
		return false;
	}
	if (!_validateInputKey(p_inputType, p_inputKey))
	{
		printf("Invalid key \n");
		return false;
	}
	
	action->keys[p_index].type = p_inputType;
	action->keys[p_index].input_key = p_inputKey;

	return true;
}

void Input_processActions()
{
	size_t current_tick = C_getTicks();

	bool process_gamepad = false;
	GLFWgamepadstate gamepad_state;
	if (s_input_core.active_joystick_id >= 0 && s_input_core.active_joystick_id < GLFW_JOYSTICK_LAST)
	{
		if (glfwGetGamepadState(s_input_core.active_joystick_id, &gamepad_state) == GLFW_TRUE)
			process_gamepad = true;
	}
	
	for (int i = 0; i < s_input_core.index_count; i++)
	{
		InputAction* action = &s_input_core.actions[i];

		for (int j = 0; j < MAX_INPUT_ACTION_KEYS; j++)
		{
			InputKey* key = &action->keys[j];

			int key_state = 0;
			float key_strength = 0.0;

			switch (key->type)
			{
			case IT__KEYBOARD:
			{
				if (key->input_key < GLFW_KEY_SPACE)
				{
					continue;
				}

				key_state = glfwGetKey(glfw_window, key->input_key);
				key_strength = key_state;
				break;
			}
			case IT__MOUSE:
			{
				if (key->input_key < GLFW_MOUSE_BUTTON_LEFT)
				{
					continue;
				}
				key_state = glfwGetMouseButton(glfw_window, key->input_key);
				key_strength = key_state;
				break;
			}
			case IT__PAD_BUTTON:
			{
				if (key->input_key < GLFW_GAMEPAD_BUTTON_A || !process_gamepad)
				{
					continue;
				}

				int key_state = gamepad_state.buttons[key->input_key];
				key_strength = key_state;
				break;
			}
			case IT__PAD_ANALOG:
			{
				if (key->input_key < GLFW_GAMEPAD_AXIS_LEFT_X || !process_gamepad)
				{
					continue;
				}
				float axis_value = gamepad_state.axes[key->input_key];
				float abs_axis_value = fabsf(axis_value);
				key_state = (abs_axis_value >= action->deadzone_ratio) ? GLFW_PRESS : GLFW_RELEASE;
				key_strength = axis_value;
				break;
			}
			default:
				break;
			}

			if (key_state == GLFW_PRESS && !key->pressed)
			{
				key->pressed = true;
				action->pressed = true;
				action->action_tick = current_tick;
				action->strength = key_strength;
			}
			else if (key_state == GLFW_RELEASE && key->pressed)
			{
				key->pressed = false;
				action->pressed = false;
				action->action_tick = current_tick;
				action->strength = 0;
			}
		}
	}
}

bool Input_Init()
{
	memset(&s_input_core, 0, sizeof(Input_Core));
	s_input_core.index_count = 0;
}

bool Input_IsActionPressed(const char* p_name)
{
	InputAction* action = Input_getAction(p_name);
	if (!action)
	{
		printf("Invalid input %s \n", p_name);
		return false;
	}

	return action->pressed;
}

bool Input_IsActionReleased(const char* p_name)
{
	InputAction* action = Input_getAction(p_name);
	if (!action)
	{
		return false;
	}

	return !action->pressed;
}

bool Input_IsActionJustPressed(const char* p_name)
{
	InputAction* action = Input_getAction(p_name);
	if (!action)
	{
		return false;
	}
	return action->pressed && action->action_tick == C_getTicks();
}

bool Input_IsActionJustReleased(const char* p_name)
{
	InputAction* action = Input_getAction(p_name);
	if (!action)
	{
		return false;
	}

	return !action->pressed && action->action_tick == C_getTicks();
}

bool Input_saveAllToFile(const char* p_filePath)
{
	FILE* file = NULL;

	fopen_s(&file, p_filePath, "w");
	if (!file)
	{
		printf("Failed to open file for printing at path: %s!\n", p_filePath);
		return false;
	}

	for (int i = 0; i < s_input_core.index_count; i++)
	{
		InputAction* action = &s_input_core.actions[i];

		const char* key_name1 = glfwGetKeyName(action->keys[0].input_key, 0);
		const char* key_name2 = glfwGetKeyName(action->keys[1].input_key, 0);

		//fprintf(file, "bind \"%i ; %i \" \" %s \" \n", key_name1, key_name2, action->name);
	}
	return fclose(file) == 0;
}

bool Input_loadAllFromFile(const char* p_filePath)
{
	FILE* file = NULL;

	fopen_s(&file, p_filePath, "r");
	if (!file)
	{
		printf("Failed to open file for parsing at path: %s!\n", p_filePath);
		return false;
	}

	//actions must first be registered
	
	char key1_buffer[256];
	char key2_buffer[256];
	char action_name_buffer[256];

	int return_value = 0;
	while (return_value != EOF)
	{
		memset(key1_buffer, 0, sizeof(key1_buffer));
		memset(key2_buffer, 0, sizeof(key2_buffer));
		memset(action_name_buffer, 0, sizeof(action_name_buffer));

		return_value = fscanf_s(file, "bind \"%s ; %s \" \" %s \" \n", key1_buffer, 256, &key2_buffer, 256, action_name_buffer, 256);

		if (return_value == EOF)
			break;

		InputAction* action = Input_getAction(action_name_buffer);

		//no action with that name found?
		if (!action)
		{
			continue;
		}
		action->keys[0].type = IT__KEYBOARD;
		action->keys[1].type = IT__KEYBOARD;

		//action->keys[0]

	}
	


	return fclose(file) == 0;
}

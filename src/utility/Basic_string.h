#ifndef BASIC_STRING_H
#define BASIC_STRING_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <assert.h>
#include <math.h>


#ifdef BASIC_STRING_USE_SECURE_ZERO_MEMORY
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined( _WIN64) ||defined(__NT__)
#include <Windows.h>
#include <WinBase.h>
#define __STRING_ZERO_MEMORY(DEST, SIZEOFDATA) SecureZeroMemory(DEST, SIZEOFDATA); 
#endif
#else
#define __STRING_ZERO_MEMORY(DEST, SIZEOFDATA) memset(DEST, 0, SIZEOFDATA)
#endif

#define STRING_NPOS SIZE_MAX
#define _STRING_BLOCK_ALLOC_SIZE 16
#define _STRING_NULL_TERMINATED_BYTE 1

typedef struct String
{
    size_t capacity; // The number of chars that the string can hold
    size_t size; // The amount of chars currently held NOTE: The size does not account for the null terminated character
    char* data; //The raw char data
} String;

/**
 * Init the the string
\param STRING The string to init with

\return Pointer to the String struct
*/
#define String_INIT(STRING) _String_Init(STRING);

/**
_Internal: DO NOT USE!
*/
static size_t _String_calculateBlocks(size_t p_charSize)
{
    //if less than the alloc block we can just give 1 without any more calculations
    if (p_charSize < _STRING_BLOCK_ALLOC_SIZE)
        return 1;

    //If char_size is equal we give 2 since the additional overhead fixes some issues
    else if (p_charSize == _STRING_BLOCK_ALLOC_SIZE)
        return 2;

    //casted to float and rounded up
    size_t rounded = ceill((double)p_charSize / (double)_STRING_BLOCK_ALLOC_SIZE);

    if (rounded < 1)
        return 1;

    return rounded;
}
/**
_Internal: DO NOT USE!
*/
static String* _String_Init(const char* p_firstString)
{
    String* ptr = malloc(sizeof(String));

    //FAILED TO ALLOC
    if (ptr == NULL)
        return NULL;

    const size_t str_length = strlen(p_firstString);
    const size_t size_with_null_char = str_length + _STRING_NULL_TERMINATED_BYTE;

    const size_t block_size = _String_calculateBlocks(size_with_null_char);
    const size_t byte_size = block_size * _STRING_BLOCK_ALLOC_SIZE;

    ptr->data = malloc(byte_size);

    //FAILED TO ALLOC
    if (ptr->data == NULL)
    {
        free(ptr);
        return NULL;
    }

    //this function sets null terminated char at the end
    //strncpy(ptr->data, p_firstString, size_with_null_char);
    
    strcpy_s(ptr->data, size_with_null_char, p_firstString);

    ptr->capacity = byte_size;
    ptr->size = str_length; //We do not factor the null byte

    return ptr;
}
/**
_Internal: DO NOT USE!
*/
static void _String_fixTail(String* const p_string)
{
    char* ptr = p_string->data + p_string->size;

    __STRING_ZERO_MEMORY(ptr, _STRING_NULL_TERMINATED_BYTE);
}

/**
_Internal: DO NOT USE!
*/
static bool _String_safeRealloc(String* const p_string, size_t p_size)
{
    char* prev = p_string->data;

    p_string->data = realloc(prev, p_size);

    //successfull realloc
    if (p_string->data)
    {
        return true;
    }

    //Free the previous data if we failed to realloc
    free(prev);

    return false;
}
/**
_Internal: DO NOT USE!
*/
static char* _String_handleAlloc(String* const p_string, size_t p_size)
{
    const size_t new_size = p_string->size + p_size + _STRING_NULL_TERMINATED_BYTE;
    const size_t old_size = p_string->size;

    //assert(old_size > 0 && "Should not happen");
 
    //realloc if our capacity is lower than the new element size
    if (new_size > p_string->capacity)
    {
        //get how many we blocks to allocate
        const size_t block_size = _String_calculateBlocks(new_size);
        const size_t byte_size = block_size * _STRING_BLOCK_ALLOC_SIZE;

        //failed to realloc
        if (!_String_safeRealloc(p_string, byte_size))
        {
            return NULL;
        }

        p_string->size += p_size; //We do not factor the null byte
        p_string->capacity = byte_size;
    }
    //there is enough capacity for the new elements
    else
    {
        p_string->size+= p_size; //We do not factor the null byte
    }

    //ptr to the null terminated byte
    char* ptr = p_string->data + old_size;

    //memset zero so that last char is null terminated
     __STRING_ZERO_MEMORY(ptr, p_size + _STRING_NULL_TERMINATED_BYTE);

    //and now the pointer points to the start of the allocated space
    return ptr;
}


/**
 * Append characters to the end 
\param p_string String pointer
\param p_toAppend Characters to append NOTE: Does nothing if strlen == 0

*/
void String_append(String* const p_string, const char* p_toAppend)
{
    const size_t char_size = strlen(p_toAppend);
    const size_t old_size = p_string->size;
    if (char_size == 0)
        return;

    char* ptr = _String_handleAlloc(p_string, char_size);

    if (ptr == NULL)
    {
        return;
    }

    memcpy(p_string->data + old_size, p_toAppend, char_size);
    _String_fixTail(p_string);
}

void String_insert(String* const p_string, const char* p_toInsert, size_t p_index)
{
    assert(p_index < p_string->size && "Index out of bounds");

    size_t str_len = strlen(p_toInsert);

    if (str_len == 0)
        return;

    const size_t before_size = p_string->size;

    char* ptr = _String_handleAlloc(p_string, str_len);

    if (ptr == NULL)
        return;

    //address where we want to insert the string
    char* pos_address = p_string->data + p_index;

    //address after the placed items
    char* next_address = pos_address + str_len;

    //byte size of the items we are moving forward
    const size_t byte_size = before_size - p_index;

    //copy memory from old address to new one;
    memcpy(next_address, pos_address, byte_size);

    //SET the data on the newly added blocks
    memcpy(pos_address, p_toInsert, str_len);

    _String_fixTail(p_string);
}

void String_erase(String* const p_string, size_t p_index, size_t p_amount)
{
    assert(p_index < p_string->size && "Index out of bounds");
    assert(p_amount < (p_string->size - p_index) && "Deleting more items possible");

    if (p_amount == 0)
        return;

    //address where we want to remove the data from
    char* pos_address = p_string->data + p_index;

    //address after the deleted items
    char* next_address = pos_address + p_amount;

    p_string->size -= p_amount;
    //byte size of the items we are moving back
    const size_t byte_size = p_string->size - p_index;

    //copy back
    memcpy(pos_address, next_address, byte_size);

    
    //memset zero the old values
    __STRING_ZERO_MEMORY(p_string->data + p_string->size, p_amount);

    _String_fixTail(p_string);
}

void String_replaceAtIndex(String* const p_string, size_t p_index, const char* p_toReplace)
{
    assert(p_index < p_string->size && "Index out of bounds");

    const size_t str_len = strlen(p_toReplace);

    if (str_len == 0)
        return;

    assert(str_len < (p_string->size - p_index) && "Can't replace");

    char* pos_address = p_string->data + p_index;

   // strncpy(pos_address, p_toReplace, str_len);
    
    strcpy_s(pos_address, str_len + _STRING_NULL_TERMINATED_BYTE, p_toReplace);
}


/**
 * Remove a single character from the end
\param p_string String pointer
*/
void String_popBack(String* const p_string)
{
    assert(p_string->size > 0 && "Nothing to pop");

    //pointer to the last character before the null terminated array
    char* ptr = p_string->data + (p_string->size - 1);

    //memset zero the value;
    __STRING_ZERO_MEMORY(ptr, 1);

    p_string->size--;
}

/**
 * Add a single character from the end
\param p_string String pointer
\param p_char character to add 
*/
void String_pushBack(String* const p_string, const char p_char)
{
    char* ptr = _String_handleAlloc(p_string, 1);

    if (ptr == NULL)
        return;

    *ptr = p_char;
}

/**
 * Increases and allocates more capacity NOTE: The allocations are 16 byte blocks so the specified amount won't be exact
\param p_string String pointer
\param p_toReserve Amount to reserve
*/
bool String_reserve(String* const p_string, size_t p_toReserve)
{
    if (p_toReserve == 0)
        return false;

    const size_t new_allocation_size = p_string->capacity + p_toReserve;
    const size_t block_size = _String_calculateBlocks(new_allocation_size);
    const size_t byte_size = block_size * _STRING_BLOCK_ALLOC_SIZE;

    if (_String_safeRealloc(p_string, byte_size))
    {
        p_string->capacity = byte_size;
        return true;
    }

    return false;
}

/**
 * Shrinks the current capacity to the number of characters NOTE: The allocations are 16 byte blocks so it won't shrink excactly to the number of elements
\param p_string String pointer
*/
bool String_shrinkToFit(String* const p_string)
{
    if (p_string->capacity == p_string->size)
    {
        return true;
    }

    const size_t block_size = _String_calculateBlocks(p_string->size + _STRING_NULL_TERMINATED_BYTE);
    const size_t byte_size = block_size * _STRING_BLOCK_ALLOC_SIZE;

    if (_String_safeRealloc(p_string, byte_size))
    {
        p_string->capacity = byte_size;
        return true;
    }

    return false;
}

bool String_resize(String* const p_string, size_t p_newSize)
{
    //do nothing if the sizes are the same
    if (p_string->size == p_newSize)
        return true;

    //are we increasing the size or decreasing it?
    if (p_newSize > p_string->size)
    {
        size_t diff = p_newSize - p_string->size;

        if (_String_handleAlloc(p_string, diff))
        {
            return true;
        }
    }
    else
    {
        //we decrease the element size and memsetzero the values
        char* ptr = p_string->data + p_newSize;
        memset(ptr, 0, p_newSize);

        p_string->size = p_newSize;
        return true;
    }

    return false;
}
/**
 * Returns the index of the first occurence of the specified characters
\param p_string String pointer
\param p_find What to find

return Start index of the string found. NOTE: Returns STRING_NPOS if nothing was found  
*/
size_t String_findFirstOf(String* const p_string, const char* p_find)
{
    assert(p_string->size > 0 && "Nothing to find");

    const size_t find_str_size = strlen(p_find);

    assert(find_str_size > 0 && "Can't find an empty string");

    for (size_t i = 0; i < p_string->size; i++)
    {
        //Check if chr matches the first chr of the find str
        if (p_string->data[i] == p_find[0])
        {
            //Pointer to the start of the found chr
            char* chr = p_string->data + (i);

            int compare_result = strncmp(chr, p_find, find_str_size);

            //0 means its a perfect match
            if (compare_result == 0)
            {
                return i;
            }         
        }
    }

    return STRING_NPOS;
}
/**
 * Returns the index of the last occurence of the specified characters
\param p_string String pointer
\param p_find What to find

return Start index of the string found. NOTE: Returns STRING_NPOS if nothing was found
*/
size_t String_findLastOf(String* const p_string, const char* p_find)
{
    assert(p_string->size > 0 && "Nothing to find");

    const size_t find_str_size = strlen(p_find);

    assert(find_str_size > 0 && "Can't find an empty string");

    size_t last_found_index = STRING_NPOS;

    for (size_t i = 0; i < p_string->size; i++)
    {
        //Check if chr matches the first chr of the find str
        if (p_string->data[i] == p_find[0])
        {
            //Pointer to the start of the found chr
            char* chr = p_string->data + (i);

            int compare_result = strncmp(chr, p_find, find_str_size);

            //0 means its a perfect match
            if (compare_result == 0)
                last_found_index = i;
        }
    }

    return last_found_index;
}

/**
 * Check if a string starts with specified characters
\param p_string String pointer
\param p_target Characters to find

return true if starts with p_target
*/
bool String_startsWith(String* const p_string, const char* p_target)
{
    size_t find = String_findFirstOf(p_string, p_target);

    return find == 0;
}
/**
 * Check if a string starts with specified characters
\param p_string String pointer
\param p_target String to find

return true if starts with p_target
*/
bool String_startsWithOther(String* const p_string, String* const p_target)
{
    assert(p_string->size > 0 && "Empty string");
    assert(p_target->size > 0 && "Empty String");

    return String_startsWith(p_string, p_target->data);
}
/**
 * Check if a string ends with specified characters
\param p_string String pointer
\param p_target String to find

return true if ends with p_target
*/
bool String_endsWith(String* const p_string, const char* p_target)
{
    size_t find = String_findLastOf(p_string, p_target);

    if (find == STRING_NPOS)
        return false;

    size_t str_len = strlen(p_target);
    
    return p_string->size - find == str_len;
}
/**
 * Check if a string ends with specified characters
\param p_string String pointer
\param p_target String to find

return true if ends with p_target
*/
bool String_endsWithOther(String* const p_string, String* const p_target)
{
    assert(p_string->size > 0 && "Empty string");
    assert(p_target->size > 0 && "Empty String");
    
    return String_endsWith(p_string, p_target->data);
}

/**
 * Check if a string contains the specified characters
\param p_string String pointer
\param p_target String to find

return true if contains p_target
*/
bool String_contains(String* const p_string, const char* p_target)
{
    if (p_string->size <= 0)
        return false;

    size_t find = String_findFirstOf(p_string, p_target);

    return find != STRING_NPOS;
}
/**
 * Check if a string contains the specified characters
\param p_string String pointer
\param p_target String to find

return true if contains p_target
*/
bool String_containsOther(String* const p_string, String* const p_target)
{
    if (p_string->size <= 0 || p_target->size < 0)
        return false;

    size_t find = String_findFirstOf(p_string, p_target->data);

    return find != STRING_NPOS;
}
/**
 * Returns a substring of the string
\param p_string String pointer
\param p_index Starting index of the substr
\param p_amount Amount of characters to extract

return String struct NOTE: Needs to be deleted using String_Destruct(...)
*/
String* String_substr(String* const p_string, size_t p_index, size_t p_amount)
{
    assert(p_string->size > 0 && "No characters to substring");
    assert(p_index < p_string->size && "Index out of bounds");

    size_t str_length = (p_amount > (p_string->size - p_index)) ? (p_string->size - p_index) : p_amount;

    String* str = _String_Init("");

    if (str == NULL)
        return NULL;
    
    char* ptr = _String_handleAlloc(str, str_length);

    if (ptr == NULL)
        return NULL;

    char* pos_address = p_string->data + p_index;

    strcpy_s(str->data, str_length + _STRING_NULL_TERMINATED_BYTE, pos_address);

    return str;
}
/**
 * Clears all characters but, does not change the capacity NOTE: Does nothing if string size is 0
\param p_string String pointer
*/
void String_clear(String* const p_string)
{
    if (p_string->size == 0)
        return;

    __STRING_ZERO_MEMORY(p_string->data, p_string->size);

    p_string->size = 0;
}
/**
 * Destroys the string and sets the pointer to NULL
\param p_string String pointer
*/
void String_Destruct(String* p_string)
{
    free(p_string->data);
    free(p_string);

    p_string = NULL;
}

/**
 * Clears the string and sets a new string 
\param p_string String pointer
\param p_toSet What to set the string to
*/
void String_set(String* const p_string, const char* p_toSet)
{
    String_clear(p_string);

    size_t char_size = strlen(p_toSet);

    if (char_size == 0)
        return;

    char* ptr = _String_handleAlloc(p_string, char_size);

    if (ptr == NULL)
        return;

    strcpy_s(ptr, char_size + _STRING_NULL_TERMINATED_BYTE, p_toSet);
}

#endif
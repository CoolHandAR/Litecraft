#include "c_resource_manager.h"


#include "utility/u_object_pool.h"
#include <assert.h>

typedef enum
{
	RSRC__SOUND,
	RSRC__TEXTURE,
	RSRC__FONT

} ResourceType;

typedef struct
{
	char path[256];
	ResourceType type;
	int ref_counter;
	void* data;
} Resource;

typedef struct
{
	//Resource cvars[MAX_CVARS];
	int index_count;

	//C_Cvar* hash_table[FILE_HASH_SIZE];
	//C_Cvar* next;

} ResourceManagerCore;




void RM_Init()
{
	

}

void RM_Exit()
{

}

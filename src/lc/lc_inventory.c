
//#include "lc_block_defs.h"

#define MAX_HOTBAR_SLOTS 9
typedef struct
{
	int index;
	//LC_BlockType items[MAX_HOTBAR_SLOTS];
} Hotbar;

typedef struct
{
	int x;
} Inventory;

static Hotbar hotbar;
static Inventory inventory;


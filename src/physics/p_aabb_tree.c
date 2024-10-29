#include "p_aabb_tree.h"
#include <assert.h>
#include <lc/lc_world.h>

//inspired by https://web.archive.org/web/20240328144640/https://www.azurefromthetrenches.com/introductory-guide-to-aabb-tree-collision-detection/

#define AABB_NODE_NULL -1


typedef struct
{
	int index;
	int fully_within;
} LocalStackItem;

//Util
typedef struct
{
	LocalStackItem _arr[64];
	int _index;
} LocalIndexStack64;

static inline LocalIndexStack64 LocalIndexStack_Create()
{
	LocalIndexStack64 stack;
	
	stack._index = 0;

	return stack;
}

static inline void LocalIndexStack_Push(LocalIndexStack64* p_stack, int p_value, bool p_fullyWithin)
{
	LocalStackItem item;
	item.index = p_value;
	item.fully_within = p_fullyWithin;

	p_stack->_arr[p_stack->_index] = item;
	p_stack->_index++;
}
static inline void LocalIndexStack_Pop(LocalIndexStack64* p_stack)
{
	p_stack->_arr[p_stack->_index].index = -1;
	p_stack->_index--;
}
static inline LocalStackItem LocalIndexStack_GetTop(LocalIndexStack64* p_stack)
{
	return p_stack->_arr[p_stack->_index - 1];
}

typedef struct AABB_Node
{
	AABB box;
	int left, right, parent, next;
	void* data;
} AABB_Node;

static void ReleaseNode(AABB_Tree* const p_tree, int p_index)
{
	AABB_Node* array = p_tree->nodes->pool->data;
	
	AABB_Node* node = &array[p_index];
	memset(node, 0, sizeof(AABB_Node));

	node->parent = AABB_NODE_NULL;
	node->left = AABB_NODE_NULL;
	node->right = AABB_NODE_NULL;
	node->data = NULL;

	Object_Pool_Free(p_tree->nodes, p_index, false);
}

static void fixUpwardsTree(AABB_Tree* const p_tree, int p_index)
{
	int index = p_index;

	AABB_Node* array = p_tree->nodes->pool->data;

	while (index != AABB_NODE_NULL)
	{
		AABB_Node* tree_node = &array[index];

		assert(tree_node->left != AABB_NODE_NULL && tree_node->right != AABB_NODE_NULL);

		AABB_Node* left_node = &array[tree_node->left];
		AABB_Node* right_node = &array[tree_node->right];
		
		AABB_MergeWith(&left_node->box, &right_node->box, &tree_node->box);

		index = tree_node->parent;
	}
}

static void insertLeaf(AABB_Tree* const p_tree, int p_index)
{

	//is our root index empty?
	if (p_tree->root == AABB_NODE_NULL)
	{
		//insert the element into root
		p_tree->root = p_index;
		return;
	}

	int tree_node_itr = p_tree->root;
	AABB_Node* array = p_tree->nodes->pool->data;
	AABB_Node* leaf_node = &array[p_index];
	
	assert(leaf_node->left == AABB_NODE_NULL);
	assert(leaf_node->right == AABB_NODE_NULL);
	assert(leaf_node->parent == AABB_NODE_NULL);


	//iterate from root
	while (array[tree_node_itr].left != AABB_NODE_NULL)
	{
		AABB_Node* itr_node = &array[tree_node_itr];
		AABB_Node* left_node = &array[itr_node->left];
		AABB_Node* right_node = &array[itr_node->right];

		AABB combined_aabb;
		AABB_MergeWith(&itr_node->box, &leaf_node->box, &combined_aabb);

		float new_parent_cost = 2.0f * AABB_GetSurfaceArea(&combined_aabb);
		float min_push_down_cost = 2.0f * (AABB_GetSurfaceArea(&combined_aabb) - AABB_GetSurfaceArea(&itr_node->box));

		float cost_left = 0;
		float cost_right = 0;

		//Is leaf?
		if (left_node->left == AABB_NODE_NULL)
		{
			AABB merged_aabb;
			AABB_MergeWith(&leaf_node->box, &left_node->box, &merged_aabb);

			cost_left = AABB_GetSurfaceArea(&merged_aabb) + min_push_down_cost;
		}
		else
		{
			AABB new_left_aabb;
			AABB_MergeWith(&leaf_node->box, &left_node->box, &new_left_aabb);

			cost_left = (AABB_GetSurfaceArea(&new_left_aabb) - AABB_GetSurfaceArea(&left_node->box)) + min_push_down_cost;
		}
		//Is leaf?
		if (right_node->left == AABB_NODE_NULL)
		{
			AABB merged_aabb;
			AABB_MergeWith(&leaf_node->box, &right_node->box, &merged_aabb);

			cost_right = AABB_GetSurfaceArea(&merged_aabb) + min_push_down_cost;
		}	
		else
		{
			AABB new_right_aabb;
			AABB_MergeWith(&leaf_node->box, &right_node->box, &new_right_aabb);

			cost_right = (AABB_GetSurfaceArea(&new_right_aabb) - AABB_GetSurfaceArea(&right_node->box)) + min_push_down_cost;
		}

		if (new_parent_cost < cost_left && new_parent_cost < cost_right)
		{
			break;
		}
		
		if (cost_left < cost_right)
		{
			tree_node_itr = itr_node->left;
		}
		else
		{
			tree_node_itr = itr_node->right;
		}
	}

	int sibling_index = tree_node_itr;
	AABB_Node* sibling_node = &array[sibling_index];
	int old_parent_index = sibling_node->parent;
	int new_parent_index = Object_Pool_Request(p_tree->nodes);

	//Update the pointers after a possible realloc
	array = p_tree->nodes->pool->data;
	sibling_node = &array[sibling_index];
	leaf_node = &array[p_index];

	AABB_Node* new_parent_node = &array[new_parent_index];
	AABB_MergeWith(&leaf_node->box, &sibling_node->box, &new_parent_node->box);
	new_parent_node->parent = old_parent_index;
	new_parent_node->left = sibling_index;
	new_parent_node->right = p_index;
	leaf_node->parent = new_parent_index;
	sibling_node->parent = new_parent_index;

	if (old_parent_index == AABB_NODE_NULL)
	{
		p_tree->root = new_parent_index;
	}
	else
	{
		AABB_Node* old_parent = &array[old_parent_index];
		if (old_parent->left == sibling_index)
		{
			old_parent->left = new_parent_index;
		}
		else
		{
			old_parent->right = new_parent_index;
		}
	}

	tree_node_itr = leaf_node->parent;

	fixUpwardsTree(p_tree, tree_node_itr);
}

static void removeLeaf(AABB_Tree* const p_tree, int p_index)
{
	if (p_index == p_tree->root)
	{
		p_tree->root = AABB_NODE_NULL;
		return;
	}

	AABB_Node* node_array = p_tree->nodes->pool->data;
	
	AABB_Node* leaf_node = &node_array[p_index];
	int parent_node_index = leaf_node->parent;
	AABB_Node* parent_node = &node_array[parent_node_index];
	int grand_parent_node_index = parent_node->parent;
	int sibling_node_index = parent_node->left == p_index ? parent_node->right : parent_node->left;
	
	assert(sibling_node_index != AABB_NODE_NULL);
	AABB_Node* sibling_node = &node_array[sibling_node_index];
	
	if (grand_parent_node_index != AABB_NODE_NULL)
	{
		AABB_Node* grand_parent_node = &node_array[grand_parent_node_index];

		if (grand_parent_node->left == parent_node_index)
		{
			grand_parent_node->left = sibling_node_index;
		}
		else
		{
			grand_parent_node->right = sibling_node_index;
		}
		sibling_node->parent = grand_parent_node_index;


		ReleaseNode(p_tree, parent_node_index);

		fixUpwardsTree(p_tree, grand_parent_node_index);
	}
	else
	{
		p_tree->root = sibling_node_index;
		sibling_node->parent = AABB_NODE_NULL;
		ReleaseNode(p_tree, parent_node_index);
	}

	leaf_node->parent = AABB_NODE_NULL;

}

AABB_Tree AABB_Tree_Create()
{
	AABB_Tree tree;
	
	tree.nodes = Object_Pool_INIT(AABB_Node, 0);
	tree.root = AABB_NODE_NULL;

	return tree;
}

void AABB_Tree_Destruct(AABB_Tree* p_tree)
{
	Object_Pool_Destruct(p_tree->nodes);
}

int AABB_Tree_Insert(AABB_Tree* const p_tree, AABB* const p_AABB, void* const p_data)
{
	int request_index = Object_Pool_Request(p_tree->nodes);
	AABB_Node* array = p_tree->nodes->pool->data;
	array[request_index].box = *p_AABB;
	array[request_index].left = AABB_NODE_NULL;
	array[request_index].right = AABB_NODE_NULL;
	array[request_index].next = AABB_NODE_NULL;
	array[request_index].parent = AABB_NODE_NULL;
	array[request_index].data = p_data;

	insertLeaf(p_tree, request_index);

	return request_index;
}

void AABB_Tree_Remove(AABB_Tree* const p_tree, int p_index)
{
	removeLeaf(p_tree, p_index);
	
	AABB_Node* array = p_tree->nodes->pool->data;

	AABB_Node* node = &array[p_index];

	memset(node, 0, sizeof(AABB_Node));
	node->left = AABB_NODE_NULL;
	node->right = AABB_NODE_NULL;
	node->parent = AABB_NODE_NULL;
	node->data = NULL;

	Object_Pool_Free(p_tree->nodes, p_index, false);
}

void* AABB_Tree_GetIndexData(AABB_Tree* const p_tree, int p_index)
{
	AABB_Node* node = dA_at(p_tree->nodes->pool, p_index);

	if (!node)
	{
		return NULL;
	}

	return node->data;
}

dynamic_array* AABB_Tree_QueryOverlaps(AABB_Tree* const p_tree, AABB* const p_AABB, void* const p_data)
{
	dynamic_array* overlaps = dA_INIT(AABB_QueryResult, 0);
	dynamic_array* stack = dA_INIT(int, 0);

	//used for faster tests
	AABB stack_aabb = *p_AABB;

	int* ins = dA_emplaceBack(stack);
	*ins = p_tree->root;

	while (stack->elements_size > 0)
	{
		int* stack_array = stack->data;
		AABB_Node* node_array = p_tree->nodes->pool->data;

		int node_index = *(int*)dA_getLast(stack);
		dA_popBack(stack);

		if (node_index == AABB_NODE_NULL)
			continue;

		AABB_Node* node = &node_array[node_index];

		if (AABB_intersectsOther(&node->box, &stack_aabb))
		{
			if (node->left == AABB_NODE_NULL && node->data != p_data)
			{
				AABB_QueryResult* overlap = dA_emplaceBack(overlaps);
				overlap->box = node->box;
				overlap->data = node->data;
			}
			else
			{
				int* ins_left = dA_emplaceBack(stack);
				*ins_left = node->left;

				int* ins_right = dA_emplaceBack(stack);
				*ins_right = node->right;
			}
		}
	}

	free(stack);

	return overlaps;
}

dynamic_array* AABB_Tree_QueryRay(AABB_Tree* const p_tree, vec3 from, vec3 dir)
{
	dynamic_array* overlaps = dA_INIT(AABB_RayQueryResult, 0);
	dynamic_array* stack = dA_INIT(int, 0);

	int* ins = dA_emplaceBack(stack);
	*ins = p_tree->root;

	while (stack->elements_size > 0)
	{
		int* stack_array = stack->data;
		AABB_Node* node_array = p_tree->nodes->pool->data;

		int node_index = *(int*)dA_getLast(stack);
		dA_popBack(stack);

		if (node_index == AABB_NODE_NULL)
			continue;

		AABB_Node* node = &node_array[node_index];

		vec3 clip, normal;
		float near, far;

		if (AABB_intersectsRay(&node->box, from, dir, clip, normal, &far, &near))
		{
			if (node->left == AABB_NODE_NULL)
			{
				AABB_RayQueryResult* overlap = dA_emplaceBack(overlaps);
				overlap->box = node->box;
				overlap->data = node->data;
				overlap->near2 = near;
				overlap->far2 = far;
				glm_vec3_copy(normal, overlap->normal);
				glm_vec3_copy(clip, overlap->clip);
			}
			else
			{
				int* ins_left = dA_emplaceBack(stack);
				*ins_left = node->left;

				int* ins_right = dA_emplaceBack(stack);
				*ins_right = node->right;
			}
		}
	}

	free(stack);

	return overlaps;
}

dynamic_array* AABB_Tree_QueryPlaneRay(AABB_Tree* const p_tree, vec3 from, vec3 dir, vec3 normal, float distance)
{
	dynamic_array* overlaps = dA_INIT(AABB_PlaneRayQueryResult, 0);
	dynamic_array* stack = dA_INIT(int, 0);

	int* ins = dA_emplaceBack(stack);
	*ins = p_tree->root;

	while (stack->elements_size > 0)
	{
		int* stack_array = stack->data;
		AABB_Node* node_array = p_tree->nodes->pool->data;

		int node_index = *(int*)dA_getLast(stack);
		dA_popBack(stack);

		if (node_index == AABB_NODE_NULL)
			continue;

		AABB_Node* node = &node_array[node_index];

		Plane p;
		p.distance = distance;
		glm_vec3_copy(normal, p.normal);
		vec3 intersection;
		float d = 0;

		if(Plane_IntersectsRay(&p, from, dir, intersection, &d))
		{
			if (node->left == AABB_NODE_NULL)
			{
				AABB_PlaneRayQueryResult* overlap = dA_emplaceBack(overlaps);
				overlap->distance = d;
				glm_vec3_copy(intersection, overlap->intersection);
			}
			else
			{
				int* ins_left = dA_emplaceBack(stack);
				*ins_left = node->left;

				int* ins_right = dA_emplaceBack(stack);
				*ins_right = node->right;
			}
		}
	}

	free(stack);

	return overlaps;
}

bool AABB_FullPlaneCheck(vec3 box[2], vec4 planes[6])
{
	float* p, dp;
	int    i;

	for (i = 0; i < 6; i++) {
		p = planes[i];
		dp = p[0] * box[p[0] < 0.0f][0]
			+ p[1] * box[p[1] < 0.0f][1]
			+ p[2] * box[p[2] < 0.0f][2];

		if (dp < -p[3])
			return false;
	}

	return true;
}

bool AABB_FrustrumCheck(vec3 box[2], vec4 planes[6], bool* r_fullyWithin)
{
	if (glm_aabb_frustum(box, planes))
	{
		if (AABB_FullPlaneCheck(box, planes))
		{
			*r_fullyWithin = true;
			return true;
		}

		*r_fullyWithin = false;
		return true;
	}

	*r_fullyWithin = false;

	return false;
}

int AABB_Tree_IntersectsFrustrumPlanes(AABB_Tree* const p_tree, vec4 frusturm_planes[6], int p_maxCullCount, AABB_FrustrumCullQueryResult* r_query, AABB_FrustrumCullQueryResult* r_queryVisibles)
{
	LocalIndexStack64 stack = LocalIndexStack_Create();

	LocalIndexStack_Push(&stack, p_tree->root, false);

	int hit_count = 0;

	AABB_Node* node_array = p_tree->nodes->pool->data;
	while (stack._index > 0)
	{
		
		LocalStackItem stack_item = LocalIndexStack_GetTop(&stack);
		LocalIndexStack_Pop(&stack);

		int node_index = stack_item.index;

		if (node_index == AABB_NODE_NULL)
			continue;



		AABB_Node* node = &node_array[node_index];


		bool fully_within = false;
		
		if (stack_item.fully_within == false)
		{
			vec3 box[2];
			glm_vec3_copy(node->box.position, box[0]);
			glm_vec3_copy(node->box.position, box[1]);

			box[1][0] += node->box.width;
			box[1][1] += node->box.height;
			box[1][2] += node->box.length;

			//failed frustrum check
			if (!AABB_FrustrumCheck(box, frusturm_planes, &fully_within))
			{
				continue;
			}

			stack_item.fully_within = fully_within;

			
		}

		if (node->left == AABB_NODE_NULL)
		{
			if (hit_count >= p_maxCullCount)
			{
				return hit_count;
			}

			LCTreeData* tree_data = node->data;;

			if (r_query)
			{
				int node_index2 = tree_data->chunk_data_index;

				int query_index = node_index2 / 32;

				AABB_FrustrumCullQueryResult* r = &r_query[query_index];

				int local_index = node_index2 % 32;

				r->data |= 1 << local_index;
			}
			if (r_queryVisibles)
			{
				AABB_FrustrumCullQueryResult* r2 = &r_queryVisibles[hit_count];

				r2->data = tree_data->chunk_data_index;

			}

			hit_count++;

		}
		else
		{
			LocalIndexStack_Push(&stack, node->left, stack_item.fully_within);
			LocalIndexStack_Push(&stack, node->right, stack_item.fully_within);
		}
	}

	return hit_count;
}

int AABB_Tree_IntersectsFrustrumBox(AABB_Tree* const p_tree, vec3 p_box[2], int p_maxCullCount, AABB_FrustrumCullQueryResult* r_query, AABB_FrustrumCullQueryResult* r_queryVisibles, AABB_FrustrumCullQueryResultTreeData* r_queryVertexIndex)
{
	LocalIndexStack64 stack = LocalIndexStack_Create();

	LocalIndexStack_Push(&stack, p_tree->root, false);

	int hit_count = 0;

	AABB_Node* node_array = p_tree->nodes->pool->data;
	while (stack._index > 0)
	{

		LocalStackItem stack_item = LocalIndexStack_GetTop(&stack);
		LocalIndexStack_Pop(&stack);

		int node_index = stack_item.index;

		if (node_index == AABB_NODE_NULL)
			continue;


		AABB_Node* node = &node_array[node_index];


		if (stack_item.fully_within == false)
		{
			vec3 box[2];
			glm_vec3_copy(node->box.position, box[0]);
			glm_vec3_copy(node->box.position, box[1]);

			box[1][0] += node->box.width;
			box[1][1] += node->box.height;
			box[1][2] += node->box.length;

			//failed frustrum check
			if (!glm_aabb_aabb(p_box, box))
			{
				continue;
			}

			stack_item.fully_within = glm_aabb_contains(p_box, box);

		}

		if (node->left == AABB_NODE_NULL)
		{
			if (hit_count >= p_maxCullCount)
			{
				return hit_count;
			}
			LCTreeData* tree_data = node->data;;

			if (r_query)
			{
				int node_index2 = tree_data->chunk_data_index;

				int query_index = node_index2 / 32;

				AABB_FrustrumCullQueryResult* r = &r_query[query_index];

				int local_index = node_index2 % 32;

				r->data |= 1 << local_index;
			}
			if (r_queryVisibles)
			{
				AABB_FrustrumCullQueryResult* r2 = &r_queryVisibles[hit_count];

				r2->data = tree_data->chunk_data_index;

			}
			if (r_queryVertexIndex)
			{
				AABB_FrustrumCullQueryResultTreeData* r2 = &r_queryVertexIndex[hit_count];

				r2->data = tree_data;
			}

			hit_count++;

			//printf(" fully within %i \n", stack_item.fully_within);
		}
		else
		{
			LocalIndexStack_Push(&stack, node->left, stack_item.fully_within);
			LocalIndexStack_Push(&stack, node->right, stack_item.fully_within);
		}
	}

	return hit_count;
}

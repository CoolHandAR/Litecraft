#include "p_aabb_tree.h"

#define AABB_NODE_NULL -1

typedef struct AABB_Node
{
	AABB box;
	int left, right, parent, next;
	void* data;
} AABB_Node;

static void ReleaseNode(AABB_Tree* const p_tree, int p_index)
{
	AABB_Node* array = p_tree->nodes->pool->data;
	memset(&array[p_index], 0, sizeof(AABB_Node));
	array[p_index].parent = AABB_NODE_NULL;
	array[p_index].left = AABB_NODE_NULL;
	array[p_index].right = AABB_NODE_NULL;
	Object_Pool_Free(p_tree->nodes, p_index, true);
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
		float min_push_down_cost = 2.0f * (AABB_GetSurfaceArea(&combined_aabb) - AABB_GetSurfaceArea(&itr_node));

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
	Object_Pool_Free(p_tree->nodes, p_index, true);
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
				overlap->near = near;
				overlap->far = far;
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

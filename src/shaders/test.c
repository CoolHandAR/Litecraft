bool _cull_convex_iterative(uint32_t p_node_id, CullParams &r_params, bool p_fully_within = false) {
	// our function parameters to keep on a stack
	struct CullConvexParams 
    {
		uint32_t node_id;
		bool fully_within;
	};

	// most of the iterative functionality is contained in this helper class
	BVH_IterativeInfo<CullConvexParams> ii;

	// alloca must allocate the stack from this function, it cannot be allocated in the
	// helper class
	ii.stack = (CullConvexParams *)alloca(ii.get_alloca_stacksize());

	// seed the stack
	ii.get_first()->node_id = p_node_id;
	ii.get_first()->fully_within = p_fully_within;

	// preallocate these as a once off to be reused
	uint32_t max_planes = r_params.hull.num_planes;
	uint32_t *plane_ids = (uint32_t *)alloca(sizeof(uint32_t) * max_planes);

	CullConvexParams ccp;

	// while there are still more nodes on the stack
	while (ii.pop(ccp)) 
    {
		const TNode &tnode = _nodes[ccp.node_id];

		if (!ccp.fully_within) 
        {
			typename BVHABB_CLASS::IntersectResult res = tnode.aabb.intersects_convex(r_params.hull);

			switch (res) 
            {
				default: {
					continue; // miss, just move on to the next node in the stack
				} break;
				case BVHABB_CLASS::IR_PARTIAL: {
				} break;
				case BVHABB_CLASS::IR_FULL: {
					ccp.fully_within = true;
				} break;
			}

		} // if not fully within already

		if (tnode.is_leaf()) 
        {
			// lazy check for hits full up condition
			if (_cull_hits_full(r_params)) 
            {
				return false;
			}

			const TLeaf &leaf = _node_get_leaf(tnode);

			// if fully within, simply add all items to the result
			// (taking into account masks)
			if (ccp.fully_within) 
            {
				for (int n = 0; n < leaf.num_items; n++) {
					uint32_t child_id = leaf.get_item_ref_id(n);

					// register hit
					_cull_hit(child_id, r_params);
				}

			} else 
            {
				// we can either use a naive check of all the planes against the AABB,
				// or an optimized check, which finds in advance which of the planes can possibly
				// cut the AABB, and only tests those. This can be much faster.
#define BVH_CONVEX_CULL_OPTIMIZED
#ifdef BVH_CONVEX_CULL_OPTIMIZED
				// first find which planes cut the aabb
				uint32_t num_planes = tnode.aabb.find_cutting_planes(r_params.hull, plane_ids);
				BVH_ASSERT(num_planes <= max_planes);

//#define BVH_CONVEX_CULL_OPTIMIZED_RIGOR_CHECK
#ifdef BVH_CONVEX_CULL_OPTIMIZED_RIGOR_CHECK
				// rigorous check
				uint32_t results[MAX_ITEMS];
				uint32_t num_results = 0;
#endif

				// test children individually
				for (int n = 0; n < leaf.num_items; n++) 
                {
					//const Item &item = leaf.get_item(n);
					const BVHABB_CLASS &aabb = leaf.get_aabb(n);

					if (aabb.intersects_convex_optimized(r_params.hull, plane_ids, num_planes)) {
						uint32_t child_id = leaf.get_item_ref_id(n);

#ifdef BVH_CONVEX_CULL_OPTIMIZED_RIGOR_CHECK
						results[num_results++] = child_id;
#endif

						// register hit
						_cull_hit(child_id, r_params);
					}
				}

#ifdef BVH_CONVEX_CULL_OPTIMIZED_RIGOR_CHECK
				uint32_t test_count = 0;

				for (int n = 0; n < leaf.num_items; n++) 
                {
					const BVHABB_CLASS &aabb = leaf.get_aabb(n);

					if (aabb.intersects_convex_partial(r_params.hull)) {
						uint32_t child_id = leaf.get_item_ref_id(n);

						CRASH_COND(child_id != results[test_count++]);
						CRASH_COND(test_count > num_results);
					}
				}
#endif

#else
				// not BVH_CONVEX_CULL_OPTIMIZED
				// test children individually
				for (int n = 0; n < leaf.num_items; n++)
                 {
					const BVHABB_CLASS &aabb = leaf.get_aabb(n);

					if (aabb.intersects_convex_partial(r_params.hull)) {
						uint32_t child_id = leaf.get_item_ref_id(n);

						// full up with results? exit early, no point in further testing
						if (!_cull_hit(child_id, r_params)) {
							return false;
						}
					}
				}
#endif // BVH_CONVEX_CULL_OPTIMIZED
			} // if not fully within
		} else 
        {
			for (int n = 0; n < tnode.num_children; n++) 
            {
				uint32_t child_id = tnode.children[n];

				// add to the stack
				CullConvexParams *child = ii.request();

				// should always return valid child
				child->node_id = child_id;
				child->fully_within = ccp.fully_within;
			}
		}

	} // while more nodes to pop

	// true indicates results are not full
	return true;

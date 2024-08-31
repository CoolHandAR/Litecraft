# Litecraft

## A simple "recreation" of minecraft in c.

## Renderer
The renderer uses deferred shading. This allows to handle way more lights than a forward one would.
Preparing world chunks for rendering happen mostly in a single compute shader. One vertex buffer allows all chunks to be rendered in a single draw call.
Chunks are frustrum culled and then a bounding box of the chunk is used for raster oclussion testing.
We depth test all the bounding boxes and check if they pass the fragment test and it did we mark the chunk as visible.

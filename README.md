# Litecraft

## A simple "recreation" of minecraft in c and openGL.

## Renderer
The renderer uses deferred shading. This allows to handle way more lights than a forward one would.
Preparing world chunks for rendering happen mostly in a single compute shader. One vertex buffer allows all chunks to be rendered in a single draw call.
Chunks are frustrum culled using an Aabb tree and then a bounding box of the chunk is used for raster oclussion testing.
We depth test all the bounding boxes and check if they pass the fragment test and if it did we mark the chunk as visible.

## Chunk meshing
Chunk dimensions are 16x16x16. Each block in a chunk is tested against nearby blocks for meshing and backface culling. This greatly allows for higher performance
and lower vertex count. The current algorithm is pretty slow and needs improvement...

## Assets
Assets are not my own. Block assets are taken from https://minecraftrtx.net/ and composed into an atlas for albedo, normals and mer (metallic, emission, roughness).
Other additional items like sounds and misc are sourced from https://mcasset.cloud/.

## Building
You need visual studio 2022 to build. You also need glfw3.lib which you can get from https://www.glfw.org/download.html. Place glfw3.lib in the lib folder,
run build_vs2022.bat, go to build/Litecraft and run the solution. Build in either debug or release. Go to bin/release/ and run the exe. The asset should be
copied automatically after building, if it doesn't copy it manually.

##Use at your own risk
There stil ton of bugs, so i don't take any responsability.

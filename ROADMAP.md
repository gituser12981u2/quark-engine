## Roadmap

Alex stuff

- Shape builder pattern
- 3d obj rendering
- indices

What we have

- Draw arbitrary 3d objects
- We have a camera that can move

Two things to do (independent of one another)

Object obj

PBRManager{obj}

obj.albdeo('')
obj.roughness(float)

1. - PBR (Physically based rendering)
    - Textures
    - Mesh and Material (Engine Primitives but not Vk primitives)
    - More to Materials
        - Textures
        - Albedo (Texture or just color)
        - Metallicness
        - Roughness
        - AO (Ambient Occlusion)
        - Normals (For lighting later)
    - Lightning (50 LOC in shader. look at BRDF)

2. - Render Scenes
    - Offscreen passes
    - Depth pre pass
    - Shadow map pass
    - Post processing (color dynamic)
   - Culminates into a Render Graph

3. Alex wants to do Audio Engine

4. Mario GUI
    - IMGUI maybe RMGUI
    - Be able to place objects

Alex does PBR and Aydrian does Render Scenes

Utilities needed by both 1 and 2

- Uploader System
- Task Manager (Multithreaded)

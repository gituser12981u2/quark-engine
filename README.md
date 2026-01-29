# quark-engine

## Work in Progress Game Engine for Quake-Style Game

This engine shall be written in Vulkan, targeting macOS, Windows, and Linux. This only supports devices 
with Vulkan 1.3+, for dynamic rendering mainly.

The engine shall have 
- Custom Physics Engine
- PBR
- Animations
- Water Rendering
- GUI Editor

Advanced possible supports
- Virtualized Geometry 
- Global Illumination

The game that shall be made with this engine is a game with movement inspired by quake.
The game shall feature older-quake-like graphics and will likely have a single player,
multiplayer, and a COD zombies inspired mode.

The focus of this game engine is to make a game playable on all devices with little hassle from the game developer.

## MVP 1

- [ ] Logger (spdlog)
- [ ] Dynamic Rendering
- [ ] Arbitrary 3d Meshes (Indices and Vertices)
- [ ] Moveable Camera (UBO)
- [ ] Profiler (custom and tracy) 
- [ ] ImGUI (profiler output, debug views, options, placing objects with gizmos)
- [ ] Depth pre-pass 
- [ ] Textures
- [ ] Material SSBO 
- [ ] Lighting (lambert/burley and cook-torrance GGX BRDF for metallic roughness)
- [ ] gITF imports
    - [ ] mesh import
    - [ ] PBR
- [ ] IBL
- [ ] Forward+ lighting
- [ ] Mipmaps
- [ ] offscreen intermediate pass
- [ ] post processing passes (HDR, bloom, etc)
- [ ] Rendergraph

## MVP 2
- [ ] RmGUI Editor
- [ ] Job System
    - [ ] Basic Multithreading for uploader
    - [ ] Dynamic picking of threads for jobs
- [ ] ECS
- [ ] Basic physics engine
- [ ] Player Controller

## MVP 3 
- [ ] Occlusion culling (gpu)
- [ ] Deferred rendering
- [ ] Streaming
- [ ] GPU indirect passes 


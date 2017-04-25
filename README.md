## Final Project : Demo

![](./demo2.PNG)

# Planned features / goals for final

- Every visual feature must be generated at runtime
- Multiple cameras and shots (will have camera class)
- Cloud / transparent elements
- Buildings / architectural elements
- Music, synchronization

# Current checkpoint status

- Currently a new version of my HW1 Planet
- Made a good old-fashioned OpenGL project for the raymarcher
- Raytraces on a shader to get close to the planet, then marches to finish the terrain
- Generates value noise texture at runtime, on CPU, for use in shader. Used in background and displacement

Initially I wanted to do a 4K Demo using IQ's basecode, but developing for that was a complete nightmare. 
Mariano insisted I use his basecode for a demo instead. So I cloned it then scooped absolutely everything out to make a fresh OpenGL project with glm / glew / SFML. 
Any code of his that I use is credited to him. (Currently, just some stuff for parsing and debugging shaders)
# multi-draw-indirect-demo

---

Demo displaying modern OpenGL features (OpenGL 4).

The demo uses Multi-Draw-Indirect (MDI) calls to dispatch draw calls from the GPU and Uniform Buffer Objects (UBOs) to pass large amounts of data to shaders.
Performance could be further increased through the use of Compute Shader draw dispatch and instancing, but this is outside the scope of this demo for simplicity's sake.

By default 125,000 objects and 200 lights are drawn in the scene.
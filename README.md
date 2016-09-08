libU3D
========
libU3D is a Universal 3D parsing library focused on readability and simplicity.
It is intended to be embedded into PDF viewers with OpenGL frontend.
## Major design decisions and limitations
### CLOD mesh generator
CLOD (Continuous Level of Detail) is disabled.
All meshes are thus always rendered at their maximum resolution.
This design decision was made because models that appear in 3DPDF files
are typically not so heavy as to require different levels of detail.
Absence of CLOD functionality speeds up decoding and reduces memory comsumption.
### TIFF support for texture images
## Implementation status
Implementation is not complete.
### CLOD mesh generator
### Texture
### Animation
### Subdivision

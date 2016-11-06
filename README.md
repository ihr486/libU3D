libU3D
========
libU3D is a Universal 3D parsing library focused on readability and simplicity.
## License
The library is licensed under GPLv3.
## Requirements
+ GLEW (minimum version depends on the target platform)
+ OpenGL 2.0 or later for rendering
+ Image loading backend that supports JPEG, PNG, TIFF and BMP loading
+ URL access backend for external texture reference
## Limitations
### CLOD mesh generator
Dynamic CLOD (Continuous Level of Detail) is disabled.
All meshes are thus always rendered at their maximum resolution.
This design decision was made because models that appear in 3DPDF files
are typically not so heavy as to require different levels of detail.
Absence of CLOD functionality speeds up decoding and reduces memory comsumption.
### List of unsupported features
+ Animation
+ Subdivision surface
+ 2D glyph
+ One- and two-point perspective projection
+ Free-form surface extension

libU3D
========
libU3D is a Universal 3D parsing library focused on readability and simplicity.
It is intended to be embedded into PDF viewers with OpenGL frontend.
## Overview
Universal 3D (U3D) is famous as the 3D format used in PDF.
Although the first version of the spec was released more than a decade ago,
there does not exist a single open-source library to parse U3D files except the Universal 3D Sample Software,
which is literally a labyrinth containing more than 400,000 lines of code.

The purpose of libU3D is to provide a more compact and maintainable version of U3D parser.
Compared to the original sample software which included everything from COM-like interface to basic containers like vectors and sets,
libU3D offloads such basic tasks to STL.
The library is expected to gain full decoder functionality except subdivision and animation.
## Major design decisions and limitations
### CLOD mesh generator
CLOD (Continuous Level of Detail) is disabled.
All meshes are thus always rendered at their maximum resolution.
This design decision was made because models that appear in 3DPDF files
are typically not so heavy as to require different levels of detail.
Absence of CLOD functionality speeds up decoding and reduces memory comsumption.
### TIFF support for texture images
Which version of libtiff to use?
## Implementation status
Implementation is not complete.
### CLOD mesh generator
The current version of the library is able to decode both Base and Progressive meshes, in addition to PointSet and LineSet.
### Texture
Texture decoder is being written as the last module to be added.
With texture support, the parser itself will be complete.
### Animation and Subdivision
Animation and subdivision are neither specified in the specification nor are implemented in the Acrobat Reader as far as I know.
Since libU3D is aimed at 3DPDF preview, these functionalities are omitted.
## License
License of the library is currently left blank, but it is *NOT IN THE PUBLIC DOMAIN*.
License terms will be discussed with developers at which place the library is to be embedded into.

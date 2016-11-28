DIBR player
===========
This is a simple depth-image-based rendering (DIBR) player. DIBR is the process
of generating virtual views from original ones and depth-frames. In special,
this project is intended to generate a stereoscopic-pair from one texture and
one depth images. For now, the input for this program is a side-by-side image or
video in which the left image is the texture, and the right image is the depth
map.

Dependencies
============
  * OpenGL
  * GLU 
  * SDL 1.2.x
  * libvlc

Compilation Instructions
========================
`dibr-player` uses [premak
Before compiling, make sure you have the above dependencies installed and
accessible in common directories (/usr/lib, /usr/include/ etc.). If so, in a
Linux environment you can easily run:

```bash
  $ premake5 gmake
  $ make
```

If you get some error, you probably need to edit Makefile file to point to the
correct directories of the dependencies are installed.

Usage
=====

```bash
  $ ./bin/debug/dibr-player samples/images/flower04.jpg
```

Keyboard shortcuts
==================
  - h   - enable/disable hole filling.
  - f   - enable/disable depth filtering.
  - a   - enable/disable show reference frames.
  - j   - increase the baseline distance between left and right eyes.
  - k   - decrease the baseline distance between left and right eyes.
  - ESC - close the program.

TODO List
=========

Options:

  * Add option to configure if the image/video contains an occlusion layer.
  * Add parameter to show filtered depth.
  * Add keyboard shortcut (or command-line option) to change output format.
  * Generate N images from minimum to maximum disparity (it can be useful for 
    multi-view and 'GIF-like' animation).

DIBR new features:

  * Support for occlusion layer.
  * Advanced depth-filtering approaches:
    - Adaptive filters, etc.
  * Improved hole-filling techniques:
    - Mixed-colors
    - Background information
    - In-painting,
    - Gradient-based.
  * Separation between cracks, holes and contours, and different hole filling
    approaches. -> The necessity became clear after the occlusion layer
    integration.
  * Support for YUV files.


----
Copyright (C) 2014-2015 Roberto Azevedo

Permission is granted to copy, distribute and/or modify this document under
the terms of the GNU Free Documentation License, Version 1.3 or any later
version published by the Free Software Foundation; with no Invariant
Sections, with no Front-Cover Texts, and with no Back-Cover Texts. A copy
of the license is included in the ``GNU Free Documentation License'' file as
part of this distribution.


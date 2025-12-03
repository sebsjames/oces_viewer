# oces_viewer

Open an OCES file and output the ommatidia data to stdout.

## Deps

Clone this with submodules. These include tinygltf and mathplot, which itself has a submodule

```bash
git clone git@github.com:sebsjames/oces_viewer
cd oces_viewer
git submodule update --init --recursive
```

## gcc-14

Currently, tinygltf won't compile with gcc-14:

```
[13:18:51 build] make
[ 50%] Building CXX object CMakeFiles/oces_viewer.dir/oces_viewer.cpp.o
In file included from /home/seb/src/oces_viewer/extern/tinygltf/tiny_gltf.h:1734,
                 from /home/seb/src/oces_viewer/oces_viewer.cpp:11:
/home/seb/src/oces_viewer/extern/tinygltf/stb_image.h: In function 'int stbi__parse_png_file(stbi__png*, int, int)':
/home/seb/src/oces_viewer/extern/tinygltf/stb_image.h:5186:56: error: writing 1 byte into a region of size 0 [-Werror=stringop-overflow=]
 5186 |                   for (k = 0; k < s->img_n; ++k) tc[k] = (stbi_uc)(stbi__get16be(s) & 255) * stbi__depth_scale_table[z->depth]; // non 8-bit images will be larger
      |                                                  ~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compilation terminated due to -Wfatal-errors.
cc1plus: all warnings being treated as errors
make[2]: *** [CMakeFiles/oces_viewer.dir/build.make:76: CMakeFiles/oces_viewer.dir/oces_viewer.cpp.o] Error 1
make[1]: *** [CMakeFiles/Makefile2:83: CMakeFiles/oces_viewer.dir/all] Error 2
make: *** [Makefile:91: all] Error 2
[13:19:39 build] gcc --version
gcc (Ubuntu 14.2.0-4ubuntu2~24.04) 14.2.0
Copyright (C) 2024 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

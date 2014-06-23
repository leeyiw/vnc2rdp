# vnc2rdp

**vnc2rdp** is a proxy for RDP client connect to VNC server, released under the [**Apache License, Version 2.0**](http://www.apache.org/licenses/LICENSE-2.0).

Currently, vnc2rdp only support:

RDP client:

* mstsc.exe
* [freerdp](http://www.freerdp.com/)

VNC server:

* RealVNC (depth 24)

## Build and install

vnc2rdp use [CMake](http://www.cmake.org/) as it's build system. To build vnc2rdp, you need to install CMake first. After CMake was installed, use these commands to generate Makefile:

```bash
$ git clone https://github.com/leeyiw/vnc2rdp.git
$ cd vnc2rdp
$ cmake .
```

After Makefile was generated, build vnc2rdp with `make`, and install with `make install`. All files installed to your system will be listed in `install_manifest.txt`.

## Screenshots

Here is a screenshot of mstsc.exe connect to RealVNC server:

![screenshot](http://leeyiw.github.io/vnc2rdp/images/screenshot.png =200x)

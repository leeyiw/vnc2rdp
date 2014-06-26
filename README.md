### Welcome

**vnc2rdp** is a proxy for RDP client connect to VNC server, released under the [**Apache License, Version 2.0**](http://www.apache.org/licenses/LICENSE-2.0).

Currently, vnc2rdp only support:

RDP client:

* mstsc.exe
* [FreeRDP](http://www.freerdp.com/)

VNC server:

* RealVNC

### Screenshots

Screenshot of using mstsc.exe connect to RealVNC Server via vnc2rdp:

<img src="http://leeyiw.github.io/vnc2rdp/images/screenshot.png" width="400" />

### Download

Current release: [vnc2rdp-0.1.0.tar.gz](https://github.com/leeyiw/vnc2rdp/archive/v0.1.0.tar.gz), MD5 checksum: `20d5197e9898ccc3a4947a5a1de39f26`. All releases are listed [**HERE**](https://github.com/leeyiw/vnc2rdp/releases).

To download the latest development tree, use the following command:

```bash
$ git clone git@github.com:leeyiw/vnc2rdp.git
```

### Build and Install

vnc2rdp use [CMake](http://www.cmake.org/) as it's build system. To build vnc2rdp, you need to install CMake first. After CMake was installed, use these commands to generate Makefile:

```bash
$ git clone https://github.com/leeyiw/vnc2rdp.git
$ cd vnc2rdp
$ cmake .
```

After Makefile was generated, build vnc2rdp with `make`, and install with `make install`. All files installed to your system will be listed in `install_manifest.txt`.

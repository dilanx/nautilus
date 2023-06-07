# VNC GPU Information

VNC-based network GPU device driver for the Nautilus kernel

## About this project

Virtual Network Computing (VNC) is a graphical system that uses the remote framebuffer protocol (RFB) to control one computer from another over a network. The Nautilus kernel has some graphical support as well as an implementation of lwIP, the lightweight IP stack, so this project aims to build a VNC server within Nautilus. The work focuses on LibVNC, a set of C libraries that includes a server implementation. However, with LibVNC’s heavy reliance on Linux and C standard libraries that Nautilus does not support, the progress towards the initial goal was interrupted. Here, we explain how a VNC-based GPU device driver would work and why LibVNC may not be the appropriate solution for implementing VNC within Nautilus.

Find more details in our slideshow: https://www.dilanxd.com/cs446-vncgpu

## Getting started

### 1. Installing Nautilus

Install our `nautilus` fork:

```
git clone https://github.com/dilanx/nautilus.git
```

### 2. Install LibVNC

You'll need [CMake](https://cmake.org/) and should be able to run [`cmake`](https://cmake.org/cmake/help/latest/manual/cmake.1.html) and [`cmake-gui`](https://cmake.org/cmake/help/latest/manual/cmake-gui.1.html).

Install and build our `libvncserver` fork:

```
git clone https://github.com/dilanx/libvncserver-nautilus.git libvnc
cd libvnc
mkdir build
cd build
cmake ..
cmake-gui .. // Then disable BUILD_SHARED_LIBS and all WITH_* options, then press "Configure"
make
```

## Notes on changes

MORE INFO COMING SOON

- new libvnc stuff
- conflicting types
- add instructions on using lwip

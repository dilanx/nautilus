/*
 * vnc_gpu.h
 * Copyright (c) 2023 Dilan Nair <https://www.dilanxd.com>
 * 
 * This file is part of the VNC GPU subproject of
 * the Nautilus AeroKernel and is built for Northwestern's
 * COMP_SCI 446 course.
 */

#ifndef _VNC_GPU
#define _VNC_GPU

//#include <rfb/rfbconfig.h>
#include <rfb/rfb.h>
#include <dev/virtio_gpu.h>

struct vnc_gpu_dev {
  rfbScreenInfoPtr screen;
};

struct vnc_gpu_dev* vnc_init(uint32_t fb_width, uint32_t fb_height, uint32_t bbp);

#endif

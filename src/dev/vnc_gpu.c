/*
 * vnc_gpu.c
 * Copyright (c) 2023 Dilan Nair <https://www.dilanxd.com>
 * 
 * This file is part of the VNC GPU subproject of
 * the Nautilus AeroKernel and is built for Northwestern's
 * COMP_SCI 446 course.
 */

#include <dev/vnc_gpu.h>
#include <nautilus/nautilus.h>

#define INFO(fmt, args...) INFO_PRINT("vnc_gpu: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("vnc_gpu: " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("vnc_gpu: " fmt, ##args)

struct vnc_gpu_dev* vnc_init(uint32_t fb_width, uint32_t fb_height, uint32_t bbp) {

  struct vnc_gpu_dev* vnc_dev = malloc(sizeof(struct vnc_gpu_dev));
  if (!vnc_dev) {
    ERROR("Failed to allocate vnc_gpu_dev\n");
    return NULL;
  }

  rfbScreenInfoPtr screen = rfbGetScreen(NULL, NULL, fb_width, fb_height, 8, 3, bbp);
  

  return vnc_dev;
}

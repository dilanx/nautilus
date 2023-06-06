/*
 * vnc_gpu.c
 * Copyright (c) 2023 Dilan Nair <https://www.dilanxd.com>
 * Copyright (c) 2023 Tanmeet Butani
 * 
 * This file is part of the VNC GPU subproject of
 * the Nautilus AeroKernel and is built for Northwestern's
 * COMP_SCI 446 course.
 */

#include <dev/vnc_gpu.h>

#define INFO(fmt, args...) INFO_PRINT("vnc_gpu: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("vnc_gpu: " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("vnc_gpu: " fmt, ##args)

struct vnc_gpu_dev* vnc_init(uint32_t fb_width, uint32_t fb_height, uint32_t bbp) {

  struct vnc_gpu_dev* vnc_dev = malloc(sizeof(struct vnc_gpu_dev));
  if (!vnc_dev) {
    ERROR("Failed to allocate vnc_gpu_dev\n");
    return NULL;
  }

  // rfbScreenInfoPtr screen = rfbGetScreen(NULL, NULL, fb_width, fb_height, 8, 3, bbp);
  // screen->frameBuffer = malloc(fb_width * fb_height * bbp);

  // rfbInitServer(screen);
  // rfbRunEventLoop(screen, -1, false);

  // vnc_dev->screen = screen;
  
  return vnc_dev;
}

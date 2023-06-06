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

// static void initBuffer(unsigned char* buffer)
// {
//   int i,j;
//   for(j=0;j<maxy;++j) {
//     for(i=0;i<maxx;++i) {
//       buffer[(j*maxx+i)*bpp+0]=(i+j)*128/(maxx+maxy); /* red */
//       buffer[(j*maxx+i)*bpp+1]=i*128/maxx; /* green */
//       buffer[(j*maxx+i)*bpp+2]=j*256/maxy; /* blue */
//     }
//     buffer[j*maxx*bpp+0]=0xff;
//     buffer[j*maxx*bpp+1]=0xff;
//     buffer[j*maxx*bpp+2]=0xff;
//     buffer[j*maxx*bpp+3]=0xff;
//   }
// }

typedef struct ClientData {
  rfbBool oldButton;
  int oldx,oldy;
} ClientData;

static void clientgone(rfbClientPtr cl)
{
  free(cl->clientData);
  cl->clientData = NULL;
}

static enum rfbNewClientAction newclient(rfbClientPtr cl)
{
  cl->clientData = malloc(sizeof(ClientData));
  cl->clientGoneHook = clientgone;
  return RFB_CLIENT_ACCEPT;
}

struct vnc_gpu_dev* vnc_init(uint32_t fb_width, uint32_t fb_height, uint32_t bbp) {

  struct vnc_gpu_dev* vnc_dev = malloc(sizeof(struct vnc_gpu_dev));
  if (!vnc_dev) {
    ERROR("Failed to allocate vnc_gpu_dev\n");
    return NULL;
  }
  rfbScreenInfoPtr rfbScreen = rfbGetScreen(NULL,NULL,fb_width,fb_height,8,3,bbp);
  if(!rfbScreen)
    return NULL;
  rfbScreen->desktopName = "LibVNCServer Example";
  rfbScreen->frameBuffer = malloc(fb_width*fb_height*bbp);
  rfbScreen->alwaysShared = TRUE;
  rfbScreen->newClientHook = newclient;
  rfbScreen->httpEnableProxyConnect = TRUE;

  // initBuffer((unsigned char*)rfbScreen->frameBuffer);

  // rfbDrawString(rfbScreen,&radonFont,20,100,"Hello, World!",0xffffff);

  rfbHttpInitSockets(rfbScreen);
  
  rfbRunEventLoop(rfbScreen,40000,FALSE);

  // DEBUG("initializing rfb screen\n");
  // rfbScreenInfoPtr screen = rfbGetScreen(NULL, NULL, fb_width, fb_height, 8, 3, bbp);

  // DEBUG("allocating rfb framebuffer\n");
  // screen->frameBuffer = malloc(fb_width * fb_height * bbp);

  // // memset(screen->frameBuffer, 0, fb_width*fb_height*bbp);

  // DEBUG("initializing rfb server\n");
  // rfbInitServer(screen);

  // DEBUG("starting rfb event loop in background\n");
  // rfbRunEventLoop(screen, -1, false);

  // vnc_dev->screen = screen;

  // DEBUG("initted vnc\n");
  
  return vnc_dev;
}

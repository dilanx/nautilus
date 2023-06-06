/*
 * vnc_gpu.c
 * Copyright (c) 2023 Dilan Nair <https://www.dilanxd.com>
 * Copyright (c) 2023 Tanmeet Butani
 * 
 * This file is part of the VNC GPU subproject of
 * the Nautilus AeroKernel and is built for Northwestern's
 * COMP_SCI 446 course.
 */

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "vnc.h"

#if LWIP_TCP

uint32_t fb_width = 640;
uint32_t fb_height = 480;
uint32_t bbp = 4;

static struct tcp_pcb *vnc_pcb;

static struct ClientData {
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

enum vnc_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

struct vnc_state
{
  u8_t state;
  u8_t retries;
  struct tcp_pcb *pcb;
  /* pbuf (chain) to recycle */
  struct pbuf *p;
};

err_t vnc_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
err_t vnc_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void vnc_error(void *arg, err_t err);
err_t vnc_poll(void *arg, struct tcp_pcb *tpcb);
err_t vnc_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
void vnc_send(struct tcp_pcb *tpcb, struct vnc_state *es);
void vnc_close(struct tcp_pcb *tpcb, struct vnc_state *es);

void
vnc_serve_init(void)
{
  vnc_pcb = tcp_new();
  if (vnc_pcb != NULL)
  {
    err_t err;

    err = tcp_bind(vnc_pcb, IP_ADDR_ANY, 5900);
    if (err == ERR_OK)
    {
      nk_vc_printf("testing\n");
      vnc_pcb = tcp_listen(vnc_pcb);
      tcp_accept(vnc_pcb, vnc_accept);
    }
    else 
    {
      /* abort? output diagnostic? */
    }
  }
  else
  {
    /* abort? output diagnostic? */
  }
}


err_t
vnc_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  err_t ret_err;
  struct vnc_state *es;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  /* commonly observed practive to call tcp_setprio(), why? */
  tcp_setprio(newpcb, TCP_PRIO_MIN);

  es = (struct vnc_state *)mem_malloc(sizeof(struct vnc_state));
  if (es != NULL)
  {
    es->state = ES_ACCEPTED;
    es->pcb = newpcb;
    es->retries = 0;
    es->p = NULL;
    DEBUG("Connection Accepted");
    /* pass newly allocated es to our callbacks */
    tcp_arg(newpcb, es);
    tcp_recv(newpcb, vnc_recv);
    tcp_err(newpcb, vnc_error);
    tcp_poll(newpcb, vnc_poll, 0);
    ret_err = ERR_OK;
  }
  else
  {
    DEBUG("Connection Not Accepted");
    ret_err = ERR_MEM;
  }
  return ret_err;  
}

err_t
vnc_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct vnc_state *es;
  err_t ret_err;

  struct vnc_gpu_dev* vnc_dev = malloc(sizeof(struct vnc_gpu_dev));
  if (!vnc_dev) {
    ERROR("Failed to allocate vnc_gpu_dev\n");
    return ERR_MEM;
  }
  rfbScreenInfoPtr rfbScreen = rfbGetScreen(NULL,NULL,fb_width,fb_height,8,3,bbp);
  if(!rfbScreen)
    return ERR_MEM;
  rfbScreen->desktopName = "LibVNCServer Example";
  rfbScreen->frameBuffer = malloc(fb_width*fb_height*bbp);
  rfbScreen->alwaysShared = TRUE;
  rfbScreen->newClientHook = newclient;
  rfbScreen->httpEnableProxyConnect = TRUE;

  // initBuffer((unsigned char*)rfbScreen->frameBuffer);

  // rfbDrawString(rfbScreen,&radonFont,20,100,"Hello, World!",0xffffff);

  rfbInitServer(rfbScreen);
  DEBUG("Running LibVnc Test");
  rfbRunEventLoop(rfbScreen,-1,TRUE);
  return ret_err;

  // LWIP_ASSERT("arg != NULL",arg != NULL);
  // es = (struct vnc_state *)arg;
  // if (p == NULL)
  // {
  //   /* remote host closed connection */
  //   es->state = ES_CLOSING;
  //   if(es->p == NULL)
  //   {
  //      /* we're done sending, close it */
  //      vnc_close(tpcb, es);
  //   }
  //   else
  //   {
  //     /* we're not done yet */
  //     tcp_sent(tpcb, vnc_sent);
  //     vnc_send(tpcb, es);
  //   }
  //   ret_err = ERR_OK;
  // }
  // else if(err != ERR_OK)
  // {
  //   /* cleanup, for unkown reason */
  //   if (p != NULL)
  //   {
  //     es->p = NULL;
  //     pbuf_free(p);
  //   }
  //   ret_err = err;
  // }
  // else if(es->state == ES_ACCEPTED)
  // {
  //   /* first data chunk in p->payload */
  //   es->state = ES_RECEIVED;
  //   /* store reference to incoming pbuf (chain) */
  //   es->p = p;
  //   /* install send completion notifier */
  //   tcp_sent(tpcb, vnc_sent);
  //   vnc_send(tpcb, es);
  //   ret_err = ERR_OK;
  // }
  // else if (es->state == ES_RECEIVED)
  // {
  //   /* read some more data */
  //   if(es->p == NULL)
  //   {
  //     es->p = p;
  //     tcp_sent(tpcb, vnc_sent);
  //     vnc_send(tpcb, es);
  //   }
  //   else
  //   {
  //     struct pbuf *ptr;

  //     /* chain pbufs to the end of what we recv'ed previously  */
  //     ptr = es->p;
  //     pbuf_chain(ptr,p);
  //   }
  //   ret_err = ERR_OK;
  // }
  // else if(es->state == ES_CLOSING)
  // {
  //   /* odd case, remote side closing twice, trash data */
  //   tcp_recved(tpcb, p->tot_len);
  //   es->p = NULL;
  //   pbuf_free(p);
  //   ret_err = ERR_OK;
  // }
  // else
  // {
  //   /* unkown es->state, trash data  */
  //   tcp_recved(tpcb, p->tot_len);
  //   es->p = NULL;
  //   pbuf_free(p);
  //   ret_err = ERR_OK;
  // }
  // return ret_err;
}

void
vnc_error(void *arg, err_t err)
{
  struct vnc_state *es;

  LWIP_UNUSED_ARG(err);

  es = (struct vnc_state *)arg;
  if (es != NULL)
  {
    mem_free(es);
  }
}

err_t
vnc_poll(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct vnc_state *es;

  es = (struct vnc_state *)arg;
  if (es != NULL)
  {
    if (es->p != NULL)
    {
      /* there is a remaining pbuf (chain)  */
      tcp_sent(tpcb, vnc_sent);
      vnc_send(tpcb, es);
    }
    else
    {
      /* no remaining pbuf (chain)  */
      if(es->state == ES_CLOSING)
      {
        vnc_close(tpcb, es);
      }
    }
    ret_err = ERR_OK;
  }
  else
  {
    /* nothing to be done */
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

err_t
vnc_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct vnc_state *es;

  LWIP_UNUSED_ARG(len);

  es = (struct vnc_state *)arg;
  es->retries = 0;
  
  if(es->p != NULL)
  {
    /* still got pbufs to send */
    tcp_sent(tpcb, vnc_sent);
    vnc_send(tpcb, es);
  }
  else
  {
    /* no more pbufs to send */
    if(es->state == ES_CLOSING)
    {
      vnc_close(tpcb, es);
    }
  }
  return ERR_OK;
}

void
vnc_send(struct tcp_pcb *tpcb, struct vnc_state *es)
{
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;
 
  while ((wr_err == ERR_OK) &&
         (es->p != NULL) && 
         (es->p->len <= tcp_sndbuf(tpcb)))
  {
  ptr = es->p;

  /* enqueue data for transmission */
  wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
  if (wr_err == ERR_OK)
  {
     u16_t plen;
      u8_t freed;

     plen = ptr->len;
     /* continue with next pbuf in chain (if any) */
     es->p = ptr->next;
     if(es->p != NULL)
     {
       /* new reference! */
       pbuf_ref(es->p);
     }
     /* chop first pbuf from chain */
      do
      {
        /* try hard to free pbuf */
        freed = pbuf_free(ptr);
      }
      while(freed == 0);
     /* we can read more data now */
     tcp_recved(tpcb, plen);
   }
   else if(wr_err == ERR_MEM)
   {
      /* we are low on memory, try later / harder, defer to poll */
     es->p = ptr;
   }
   else
   {
     /* other problem ?? */
   }
  }
}

void
vnc_close(struct tcp_pcb *tpcb, struct vnc_state *es)
{
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);
  
  if (es != NULL)
  {
    mem_free(es);
  }  
  tcp_close(tpcb);
}

#endif /* LWIP_TCP */

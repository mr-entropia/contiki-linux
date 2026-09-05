/*
 * Copyright (c) 2001, Adam Dunkels.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: vnc-server.c,v 1.4 2004/08/09 20:32:28 adamdunkels Exp $
 *
 */

/* A micro implementation of a VNC server. VNC is a protocol for
   remote network displays. See http://www.uk.research.att.com/vnc/
   for information about VNC.

   Initialization states:

   VNC_VERSION (send version string)
   VNC_AUTH    (send auth message)
   VNC_INIT    (send init message)

   Steady state:
   
   VNC_RUNNING (send RFB updates, parse incoming messages)

   What kind of message should be sent:

   SEND_NONE   (No message)
   SEND_BLANK  (Blank screen initially)
   SEND_SCREEN (Send entire screen, initially)
   SEND_UPDATE (Send incremental update)

*/

#include "uip.h"
#include "vnc-server.h"
#include "vnc-out.h"

#include <string.h>

/* RFB server initial handshaking string. */
#define RFB_SERVER_VERSION_STRING rfb_server_version_string

/* "RFB 003.003" */
static u8_t rfb_server_version_string[12] = {82,70,66,32,48,48,51,46,48,48,51,10};

/* uVNC */
static u8_t uvnc_name[4] = {117,86,78,67};
#if 1
#define PRINTF(x)
#else
#define PRINTF(x) printf x
#endif

/*-----------------------------------------------------------------------------------*/
u8_t
vnc_server_draw_rect(u8_t *ptr, u16_t x, u16_t y, u16_t w, u16_t h, u8_t c)
{
  register struct rfb_fb_update_rect_hdr *recthdr;
  u8_t *rrehdr;

  recthdr = (struct rfb_fb_update_rect_hdr *)ptr;
  rrehdr = ptr + sizeof(struct rfb_fb_update_rect_hdr);
  
  recthdr->rect.x = x;
  recthdr->rect.y = y;
  recthdr->rect.w = w;
  recthdr->rect.h = h; 
  recthdr->encoding[0] =
    recthdr->encoding[1] =
    recthdr->encoding[2] = 0;
  recthdr->encoding[3] = RFB_ENC_RRE;

  /* The RRE header (number of subrectangles + background pixel) is
     written explicitly so that no struct padding is sent on the
     wire. TigerVNC (and the original Contiki implementation) treats
     the number of subrectangles as a four-byte field, even though the
     RFB spec describes it as U16. */
  rrehdr[0] = 0;
  rrehdr[1] = 0;
  rrehdr[2] = 0;
  rrehdr[3] = 0;
  rrehdr[4] = c;
      
  return sizeof(struct rfb_fb_update_rect_hdr) + 5;
}
/*-----------------------------------------------------------------------------------*/
void
vnc_server_init(void)
{
  vnc_out_init();
}
/*-----------------------------------------------------------------------------------*/
static void
vnc_send_blank(struct vnc_server_state *vs)
{
  switch(vs->type) {
  case 0:	
    vnc_out_send_blank(vs);
    break;
    /*  case 1:
    vnc_stats_send_blank(vs);
    break;   */
  }
}
/*-----------------------------------------------------------------------------------*/
static void
vnc_send_screen(struct vnc_server_state *vs)
{
  switch(vs->type) {
  case 0:	
    vnc_out_send_screen(vs);
    break;
    /*  case 1:
    vnc_stats_send_screen(vs);
    break;*/
  }
}
/*-----------------------------------------------------------------------------------*/
static void
vnc_send_update(struct vnc_server_state *vs)
{
  switch(vs->type) {
  case 0:	
    vnc_out_send_update(vs);
    break;
    /*  case 1:
    vnc_stats_send_update(vs);
    break;*/
  }
}
/*-----------------------------------------------------------------------------------*/
void
vnc_server_send_data(struct vnc_server_state *vs)
{
  register struct rfb_server_init *initmsg;
  
  switch(vs->state) {
  case VNC_VERSION:
    uip_send(RFB_SERVER_VERSION_STRING, sizeof(RFB_SERVER_VERSION_STRING));
    break;
  case VNC_AUTH:
    uip_appdata[0] = 0;
    uip_appdata[1] = 0;
    uip_appdata[2] = 0;
    uip_appdata[3] = RFB_AUTH_NONE;
    uip_send(uip_appdata, 4);
    break;
  case VNC_INIT:
    initmsg = (struct rfb_server_init *)uip_appdata;
    initmsg->width = htons(vs->width);
    initmsg->height = htons(vs->height);
    /* Advertise a 32-bit RGB888 true-color format. The native screen
       storage is 8-bit BGR233; transmitted pixels are converted to
       the advertised (or client-requested) format on the fly. Modern
       clients such as TigerVNC reject sub-16-bit formats. */
    initmsg->format.bps = 32;
    initmsg->format.depth = 24;
    initmsg->format.endian = 0;
    initmsg->format.truecolor = 1;
    initmsg->format.red_max = htons(255);
    initmsg->format.green_max = htons(255);
    initmsg->format.blue_max = htons(255);
    initmsg->format.red_shift = 16;
    initmsg->format.green_shift = 8;
    initmsg->format.blue_shift = 0;
    initmsg->format.pad1 = 0;
    initmsg->format.pad2 = 0;
    initmsg->namelength[0] = 0;
    initmsg->namelength[1] = 0;
    initmsg->namelength[2] = 0;	    
    initmsg->namelength[3] = 4;
    memcpy(&uip_appdata[sizeof(struct rfb_server_init)], uvnc_name, 4);
    uip_send(uip_appdata, sizeof(struct rfb_server_init) + 4);
    break;
  case VNC_RUNNING:
    switch(vs->sendmsg) {
    case SEND_NONE:
      PRINTF(("Sending none\n"));
      break;
      
    case SEND_BLANK:
    case SENT_BLANK:
      PRINTF(("Sending blank\n"));
      vnc_send_blank(vs);
      break;
      
    case SEND_SCREEN:
      PRINTF(("Sending screen\n"));
      vnc_send_screen(vs);
      break;

    case SEND_UPDATE:
      PRINTF(("Sending update\n"));
      vnc_send_update(vs);
      break;
    }
    break;
    
  default:
    break;
  }

}
/*-----------------------------------------------------------------------------------*/
static void
vnc_key_event(struct vnc_server_state *vs)
{
  switch(vs->type) {
  case 0:	
    vnc_out_key_event(vs);
    break;
    /*  case 1:
    vnc_stats_key_event(vs);
    break;*/
  }
}
/*-----------------------------------------------------------------------------------*/
static void
vnc_pointer_event(struct vnc_server_state *vs)
{
  switch(vs->type) {
  case 0:	
    vnc_out_pointer_event(vs);
    break;
    /*  case 1:
    vnc_stats_pointer_event(vs);
    break;*/
  }
}
/*-----------------------------------------------------------------------------------*/
static u8_t
vnc_read_data(register struct vnc_server_state *vs)
{
  u8_t *appdata;
  u16_t len;
  struct rfb_fb_update_request *req;
  /*  u8_t niter;*/
  
  len = uip_datalen();
  appdata = (u8_t *)uip_appdata;
  
  /* First, check if there is data left to discard since last read. */
  if(vs->readlen > 0) {
    appdata += vs->readlen;
    if(len > vs->readlen) {
      len -= vs->readlen;
      vs->readlen = 0;
    } else {
      vs->readlen -= len;
      len = 0;
    }
  }

  if(vs->readlen != 0) {
    return 1;
  }

  /* All data read and ignored, parse next message. */
  /*  for(niter = 32; niter > 0 && len > 0; --niter) {*/
  while(len > 0) {
    switch(vs->state) {
    case VNC_VERSION:
    case VNC_VERSION2:
      PRINTF(("Read in version\n"));
      /* Receive and ignore client version string (12 bytes). */
      vs->state = VNC_AUTH;
      vs->readlen = 12;
      break;
      
    case VNC_AUTH:
    case VNC_AUTH2:
      PRINTF(("Read in auth \n"));
      /* Read and discard the single byte the client sends after the
	 security type list (RFB 3.3 "None" has no security result
	 message - clients such as TigerVNC assume success immediately
	 and proceed to the server initialisation). */
      vs->readlen = 1;
      vs->state = VNC_INIT;
      break;
      
    case VNC_INIT:
    case VNC_INIT2:
      PRINTF(("Read in init \n"));
      /* No further handshake bytes are expected. The next byte (if
	 any) is the first message from the client, so fall through to
	 normal message parsing. */
      vs->readlen = 0;
      vs->state = VNC_RUNNING;
      
    case VNC_RUNNING:
      /* Handle all client events. */
      switch(*appdata) {
      case RFB_SET_PIXEL_FORMAT:
	PRINTF(("Set pixel format\n"));
	{
	  struct rfb_set_pixel_format *pfmsg;
	  struct rfb_pixel_format *pf;

	  pfmsg = (struct rfb_set_pixel_format *)appdata;
	  pf = &pfmsg->format;

	  vs->pf_set = 1;
	  vs->pf_bpp = pf->bps;
	  vs->pf_depth = pf->depth;
	  vs->pf_endian = pf->endian;
	  vs->pf_red_max = htons(pf->red_max);
	  vs->pf_green_max = htons(pf->green_max);
	  vs->pf_blue_max = htons(pf->blue_max);
	  vs->pf_red_shift = pf->red_shift;
	  vs->pf_green_shift = pf->green_shift;
	  vs->pf_blue_shift = pf->blue_shift;
	}
	vs->readlen = sizeof(struct rfb_set_pixel_format);
	break;
	
      case RFB_FIX_COLORMAP_ENTRIES:
	PRINTF(("Fix colormap entries\n"));
	return 0;
	
      case RFB_SET_ENCODINGS:
	PRINTF(("Set encodings\n"));
	vs->readlen = sizeof(struct rfb_set_encoding);
	vs->readlen += htons(((struct rfb_set_encoding *)appdata)->encodings) * 4;
	/* Check if the client advertised the Cursor encoding (-239,
	   0xFFFFFF11), in which case we may send pointer cursor shapes. */
	{
	  u16_t i;
	  u16_t ne = htons(((struct rfb_set_encoding *)appdata)->encodings);
	  for(i = 0; i < ne && i < 64; ++i) {
	    unsigned long enc;
	    u8_t *p = (u8_t *)appdata + sizeof(struct rfb_set_encoding) + i * 4;
	    enc = ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
	      ((unsigned long)p[2] << 8) | (unsigned long)p[3];
	    if(enc == 0xffffff11UL) {
	      vs->cursoradved = 1;
	    }
	  }
	}
	/* Make sure that client supports the encodings we use. */
	/* XXX: not implemented yet. */
	break;
	
      case RFB_FB_UPDATE_REQ:
	PRINTF(("Update request\n"));
	vs->update_requested = 1;
	vs->readlen = sizeof(struct rfb_fb_update_request);
	/* blank the screen initially */
	req = (struct rfb_fb_update_request *)appdata;
	if(req->incremental == 0) {
	  /*	  vs->sendmsg = SEND_BLANK;*/
	  vnc_out_update_area(vs, 0, 0, vs->w, vs->h);
	  vs->screensent = 1;
	} else if(!vs->screensent) {
	  /* Some clients (e.g. TigerVNC) send their first update request
	     with incremental set, expecting the server to send the
	     current screen contents. Consider the whole screen dirty
	     until it has been transmitted at least once. */
	  vnc_out_update_area(vs, 0, 0, vs->w, vs->h);
	  vs->screensent = 1;
	}
	break;
	
      case RFB_KEY_EVENT:
	vs->readlen = sizeof(struct rfb_key_event);
	vnc_key_event(vs);
	break;
	
      case RFB_POINTER_EVENT:
	vs->readlen = sizeof(struct rfb_pointer_event);
	vnc_pointer_event(vs);
	break;
	
      case RFB_CLIENT_CUT_TEXT:
	PRINTF(("Client cut text\n"));

	if(((struct rfb_client_cut_text *)appdata)->len[0] != 0 ||
	   ((struct rfb_client_cut_text *)appdata)->len[1] != 0) {
	  return 0;
	  
	}
	vs->readlen = sizeof(struct rfb_client_cut_text) +
	  (((struct rfb_client_cut_text *)appdata)->len[2] << 8) +
	  ((struct rfb_client_cut_text *)appdata)->len[3];
	/*	return 0;*/
	break;
	
      default:
	PRINTF(("Unknown message %d\n", *appdata));
	return 0;
      }
      break;
      
    default:
      return 0;
    }

    if(vs->readlen > 0) {
      if(len > vs->readlen) {
	len -= vs->readlen;
	appdata += vs->readlen;
	vs->readlen = 0;
      } else {
	vs->readlen -= len;
	len = 0;
      }
    } else {
      /* Lost data. */
      break;
    }
    
  }

  /*  if(vs->readlen > 0) {
    printf("More data %d\n", vs->readlen);
    }*/
  
  /*  uip_appdata = appdata;*/

  return 1;
}
/*-----------------------------------------------------------------------------------*/
static void
vnc_new(register struct vnc_server_state *vs)
{
  vs->counter = 0;
  vs->readlen = 0;
  vs->sendmsg = SEND_NONE;
  vs->update_requested = 1;
  switch(vs->type) {
  case 0:	
    vnc_out_new(vs);
    break;
    /*  case 1:
    vnc_stats_new(vs);
    break;*/
  }
}
/*-----------------------------------------------------------------------------------*/
static void
vnc_acked(register struct vnc_server_state *vs)
{
  switch(vs->state) {
  case VNC_VERSION:
    vs->state = VNC_VERSION2;
    break;
    
  case VNC_AUTH:
    vs->state = VNC_AUTH2;
    break;
    
  case VNC_INIT:
    vs->state = VNC_INIT2;
    break;

  case VNC_RUNNING:
    switch(vs->type) {
    case 0:	
      vnc_out_acked(vs);
      break;
      /*    case 1:
      vnc_stats_acked(vs);
      break;*/
    }
    break;
  }
}
/*-----------------------------------------------------------------------------------*/
void
vnc_server_appcall(struct vnc_server_state *vs)
{
  
  vs->type = htons(uip_conn->lport) - 5900;
  
  if(uip_connected()) {      
    vnc_new(vs);
    vs->state = VNC_VERSION;
    vnc_server_send_data(vs);
    return;
  }
  if(uip_acked()) {
    PRINTF(("Acked\n"));
    vnc_acked(vs);
  }
  
  if(uip_newdata()) {
    PRINTF(("Newdata\n"));
    vs->counter = 0;
    if(vnc_read_data(vs) == 0) {
      uip_abort();
      return;
    }
  }
  
  if(uip_rexmit()) {
    PRINTF(("Rexmit\n"));
  }
  
  
  if(uip_newdata() ||
     uip_rexmit() ||
     uip_acked()) {
    vnc_server_send_data(vs);
  } else if(uip_poll()) {
    ++vs->counter;
    /* Abort connection after about 20 seconds of inactivity. */
    if(vs->counter >= 40) {
      uip_abort();
      return;
    }
    
    vnc_out_poll(vs);
  }
  
}
/*-----------------------------------------------------------------------------------*/

/*
 * linux/drivers/video/riva/focus.c - Xbox driver for Focus encoder
 *
 * Maintainer: Oliver Schwartz <Oliver.Schwartz@gmx.de>
 *
 * Contributors: David Pye (dmp) <dmp@davidmpye.dyndns.org>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Known bugs and issues:
 *
 * HDTV/VGA SoG/internal sync not yet implemented
 * NTSC untested
 * hoc/voc ratios ignored
 * Only 800x600 and 640x480 work at present
*/
#include "focus.h"
#include "encoder.h"


static const unsigned char focus_defaults[0xc4] = {
	/*0x00*/ 0x00,0x00,0x00,0x00,0x80,0x02,0xaa,0x0a,
	/*0x08*/ 0x00,0x10,0x00,0x00,0x03,0x21,0x15,0x04,
	/*0x10*/ 0x00,0xe9,0x07,0x00,0x80,0xf5,0x20,0x00,
	/*0x18*/ 0xef,0x21,0x1f,0x00,0x03,0x03,0x00,0x00,
	/*0x20*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
	/*0x28*/ 0x0c,0x01,0x00,0x00,0x00,0x00,0x08,0x11,
	/*0x30*/ 0x00,0x0f,0x05,0xfe,0x0b,0x80,0x00,0x00,
	/*0x38*/ 0xa4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	/*0x40*/ 0x2a,0x09,0x8a,0xcb,0x00,0x00,0x8d,0x00,
	/*0x48*/ 0x7c,0x3c,0x9a,0x2f,0x21,0x01,0x3f,0x00,
	/*0x50*/ 0x3e,0x03,0x17,0x21,0x1b,0x1b,0x24,0x9c,
	/*0x58*/ 0x01,0x3e,0x0f,0x0f,0x60,0x05,0xc8,0x00,
	/*0x60*/ 0x9d,0x04,0x9d,0x01,0x02,0x00,0x0a,0x05,
	/*0x68*/ 0x00,0x1a,0xff,0x03,0x1e,0x0f,0x78,0x00,
	/*0x70*/ 0x00,0xb1,0x04,0x15,0x49,0x10,0x00,0xa3,
	/*0x78*/ 0xc8,0x15,0x05,0x15,0x3e,0x03,0x00,0x20,
	/*0x80*/ 0x57,0x2f,0x07,0x00,0x00,0x08,0x00,0x00,
	/*0x88*/ 0x08,0x16,0x16,0x9c,0x03,0x00,0x00,0x00,
	/*0x90*/ 0x00,0x00,0xc4,0x48,0x00,0x00,0x00,0x00,
	/*0x98*/ 0x00,0x00,0x00,0x80,0x00,0x00,0xe4,0x00,
	/*0xa0*/ 0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,
	/*0xa8*/ 0xFF,0x00,0xFF,0x00,0xFF,0x00,0x00,0x00,
	/*0xb0*/ 0x00,0x00,0xd7,0x05,0x00,0x00,0xf0,0x00,
	/*0xb8*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	/*0xc0*/ 0x00,0x00,0xee,0x00
};

int focus_calc_hdtv_mode(
	xbox_hdtv_mode hdtv_mode,
	unsigned char pll_int,
	unsigned char * regs
	){
	unsigned char b;
	int m=0;
	int tv_vtotal, tv_htotal, tv_vactive, tv_hactive, vga_htotal, vga_vtotal;
	int pll_n, pll_m, pll_p, ncon, ncod;

	for (m=0x00; m<sizeof(focus_defaults); m++) {
		regs[m] = focus_defaults[m];
	}

	/* Uncomment for SDTV colour bars */
	//regs[0x45]=0x02;
	

	/* Legacy Conexant code - not sure what to do with it here.
	if (pll_int > 36) {
             pll_int = 36; // 36 / 6 * 13.5 MHz = 81 MHz, just above the limit.
        }
        if (pll_int == 0) {
             pll_int = 1;  // 0 will cause a burnout ...
        }
	*/

	/* Turn on bridge bypass */
	regs[0x0a] &= 0x10;
	/* Turn on the HDTV clock, and turn off the SDTV one */	
	regs[0xa1] = 0x04;
	
	/* HDTV Hor start */
	regs[0xb8] = 0xbe;
	
	//HACK - hdtv porches
	regs[0x94] = 0x40;
	regs[0x95] = 0x05;
	regs[0x96] = 0x50;

	/* Colour scaling */
	regs[0xa8] = 0x92;
	regs[0x09] = 0x00;
	regs[0xaa] = 0xa0;
	regs[0xab] = 0x00;
	regs[0xac] = 0x92;
	regs[0xad] = 0x00;
	
	/*Set up video mode to HDTV + progressive*/
	regs[0x92] = 0x1e;		
	
	switch (hdtv_mode) {
		case HDTV_480p:
			break;
		case HDTV_720p:
			break;
		case HDTV_1080i:
			/* Set mode to interlaced */
			regs[0x92] |= 0x80;
			break;
	}
	return 1;
}

int focus_calc_mode(xbox_video_mode * mode, struct riva_regs * riva_out)
{
	unsigned char b;
	char* regs = riva_out->encoder_mode;
	int m = 0;
	int tv_htotal, tv_vtotal, tv_vactive, tv_hactive;
	int vga_htotal, vga_vtotal;
	long ncon, ncod;
	int pll_m, pll_n, pll_p, vsc, hsc;

	for (m=0x00; m<sizeof(focus_defaults); m++) {
		regs[m] = focus_defaults[m];
	}
	switch(mode->tv_encoding) {
		case TV_ENC_NTSC:
			tv_vtotal=525;
			tv_vactive=480;			
			tv_hactive = 720;
			tv_htotal  = 858;
			regs[0x0d] &= ~0x01;
			regs[0x40] = 0x21;
			regs[0x41] = 0xF0;
			regs[0x42] = 0x7C;
			regs[0x43] = 0x1F;
			regs[0x49] = 0x44;
			regs[0x4a] = 0x76;
			regs[0x4b] = 0x3B;
			regs[0x4c] = 0x00;
			regs[0x60] = 0x89;
			regs[0x62] = 0x89;
			regs[0x69] = 0x16;
			regs[0x6C] = 0x20;
			regs[0x74] = 0x04;		
			regs[0x75] = 0x10;
			regs[0x80] = 0x67; 
			regs[0x81] = 0x21; 
			regs[0x82] = 0x0C;
			regs[0x83] = 0x18;
			regs[0x86] = 0x18;
			regs[0x89] = 0x13;
			regs[0x8A] = 0x13;
			break;
		case TV_ENC_PALBDGHI:
			tv_vtotal = 625;
			tv_vactive = 576;
			tv_hactive = 720;
			tv_htotal = 864;
			break;
		default:
			/* Default to PAL */
			tv_vtotal = 625;
			tv_vactive = 576;
			tv_hactive = 720;
			tv_htotal = 864;
			break;
	}

	switch (mode->av_type) {
		case AV_SCART_RGB:
			/* Video control */
			b = (regs[0x92] &= ~0x04);
			regs[0x92] = (b|= 0x01);
			regs[0x93] &= ~0x40;
			/* MTX */
			regs[0xA2] = 0x4D;
			regs[0xA4] = 0x96;
			regs[0xA6] = 0x1D;
			/* Color scaling */
			regs[0xA8] = 0xA0;
			regs[0xAA] = 0xDB;
			regs[0xAC] = 0x7E;
			
			break;
	
		default:
			break;
			/* Not tested with other cable types than SVIDEO + RGB yet */
	}
	tv_vactive = tv_vactive * (1.0f-mode->voc);
	vga_vtotal = mode->yres * ((float)tv_vtotal/tv_vactive);
	vga_htotal = mode->xres * 1.25f;
	tv_hactive = tv_hactive * (1.0f-mode->hoc);
	/*These should be calculated or at least deduced.
	 *However they are good enough for 640x480 -> 800x600 now*/
	pll_n  = 32;
	pll_m = 512;
	pll_p = 4; 
	
	ncon = vga_htotal * vga_vtotal;
	ncod = tv_htotal * tv_vtotal  * pll_p;
	
	regs[0x04] = (mode->xres+64)&0xFF;
	regs[0x05] = ((mode->xres+64)>>8)&0xFF;

	if (tv_vtotal>vga_vtotal) {
		/* Upscaling */
		vsc = ((((float)tv_vtotal/(float)vga_vtotal)-1)*65536);
		/* For upscaling, adjust FIFO_LAT (FIFO latency) */
		regs[0x38] = 0x82;
	}
	else {
		/* Downscaling */
		vsc = ((((float)tv_vtotal/(float)vga_vtotal))*65536);
	}
	regs[0x06] = (vsc)&0xFF;
	regs[0x07] = (vsc>>8)&0xFF;

	hsc = 128*((float)tv_hactive/(float)mode->xres-1);
	if (tv_hactive > mode->xres) {
		/* Upscaling */
		regs[0x08] = 0;
		regs[0x09] = hsc&0xFF;
	}
	else {  /* Downscaling */
		hsc = 256 + hsc;
		regs[0x08] = hsc&0xFF;
		regs[0x09] = 0;
	}


	regs[0x10] = (ncon)&0xFF;
	regs[0x11] = (ncon>>8)&0xFF ;
	regs[0x12] = (ncon>>16)&0xFF ;
	regs[0x13] = (ncon>>24)&0xFF ;
	regs[0x14] = (ncod)&0xFF ;
	regs[0x15] = (ncod>>8)&0xFF ;
	regs[0x16] = (ncod>>16)&0xFF ;
	regs[0x17] = (ncod>>24)&0xFF ;
	regs[0x18] = (pll_m-17)&0xFF;
	regs[0x19] = ((pll_m-17)>>8)&0xFF;
	regs[0x1A] = (pll_n-1)&0xFF ;
	regs[0x1B] = ((pll_n-1)>>8)&0xFF ;
	regs[0x1C] = (pll_p-1)&0xFF;
	regs[0x1D] = (pll_p-1)&0xFF;
	        
	/* Guesswork */
	riva_out->ext.vsyncstart = vga_vtotal * 0.95;
	riva_out->ext.hsyncstart = vga_htotal * 0.95;
	
	riva_out->ext.width = mode->xres;
	riva_out->ext.height = mode->yres;
	riva_out->ext.htotal = vga_htotal - 1;
	riva_out->ext.vend = mode->yres - 1;
	riva_out->ext.vtotal = vga_vtotal- 1;
	riva_out->ext.vcrtc = mode->yres - 1;
	riva_out->ext.vsyncend = riva_out->ext.vsyncstart + 3;
        riva_out->ext.vvalidstart = 0;
	riva_out->ext.vvalidend = mode->yres - 1;
	riva_out->ext.hend = mode->xres + 7 ;
	riva_out->ext.hcrtc = mode->xres - 1;
        riva_out->ext.hsyncend = riva_out->ext.hsyncstart + 32;
        riva_out->ext.hvalidstart = 0;
        riva_out->ext.hvalidend = mode->xres - 1;
	riva_out->ext.crtchdispend = mode->xres;
        riva_out->ext.crtcvstart = mode->yres + 32;
	riva_out->ext.bpp = mode->bpp;
	//increased from 32
	riva_out->ext.crtcvtotal = mode->yres + 64;

	return 1;
}

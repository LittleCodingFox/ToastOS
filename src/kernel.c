/*
 * mykernel/kernel.c
 *
 * Copyright (C) 2017 - 2020 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief A sample BOOTBOOT compatible kernel
 *
 */

/* function to display a string, see below */
void puts(const char *s);

#include <stdint.h>
#include <framebuffer/framebuffer.h>
#include <text/psf2.h>
#include <low-level/serial/serial.h>
#include <bootboot.h>
#include <string.h>

/* imported virtual addresses, see linker script */
extern BOOTBOOT bootboot;           // see bootboot.h
extern unsigned char *environment;  // configuration, UTF-8 text key=value pairs

/******************************************
 * Entry point, called by BOOTBOOT Loader *
 ******************************************/
void _start() {

    framebufferInit();
    kernelInitText();

    serialPortInit(SERIAL_PORT_COM1, SERIAL_PORT_SPEED_115200);

    /*** NOTE: this code runs on all cores in parallel ***/
    int x = 0, y = 0,
	w=framebufferWidth(),
	h=framebufferHeight();

    // cross-hair to see screen dimension detected correctly
    for(y=0;y<h;y++) {

    	framebufferPutPixel(w / 2, y, 0x00FFFFFF);
    }

    for(x=0;x<w;x++) {

    	framebufferPutPixel(x, h / 2, 0x00FFFFFF);
    }

    // red, green, blue boxes in order
    framebufferPutPixels(20, 20, 20, 20, 0x00FF0000);

    framebufferPutPixels(50, 20, 20, 20, 0x0000FF00);

    framebufferPutPixels(80, 20, 20, 20, 0x000000FF);

    // say hello
    puts("A");

    // hang for now
    while(1);
}

/**************************
 * Display text on screen *
 **************************/

void puts(const char *s) {
    
	kernelRenderText(50, 50, s, kernelDefaultFont);
}

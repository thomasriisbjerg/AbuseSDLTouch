/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2001 Anthony Kruize <trandor@labyrinth.net.au>
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include <SDL.h>

#ifdef HAVE_OPENGL
#   ifdef __APPLE__
#       include <OpenGL/gl.h>
#       include <OpenGL/glu.h>
#   else
#       include <GL/gl.h>
#       include <GL/glu.h>
#   endif    /* __APPLE__ */
#endif    /* HAVE_OPENGL */
#ifdef HAVE_OPENGLES1
#   include <GLES/gl.h>
#endif    /* HAVE_OPENGLES1 */

#include "common.h"

#include "filter.h"
#include "video.h"
#include "image.h"
#include "setup.h"
#include "game.h"
#include "joy.h"

SDL_Surface *window = NULL, *surface = NULL;
image *main_screen = NULL;
int win_xscale, win_yscale, mouse_xscale, mouse_yscale;
int xres, yres;
int xwinres, ywinres;

extern palette *lastl;
extern flags_struct flags;
#if defined(HAVE_OPENGL)
GLfloat texcoord[4];
#endif
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES1)
GLuint texid;
GLuint touchoverlayid;
SDL_Surface *texture = NULL;
#endif /* HAVE_OPENGL || HAVE_OPENGLES1 */

extern int has_multitouch;

static void update_window_part(SDL_Rect *rect);

//
// power_of_two()
// Get the nearest power of two
//
static int power_of_two(int input)
{
    int value;
    for(value = 1 ; value < input ; value <<= 1);
    return value;
}

//
// set_mode()
// Set the video mode
//
void set_mode(int mode, int argc, char **argv)
{
    const SDL_VideoInfo *vidInfo;
    int vidFlags = 0;//SDL_HWPALETTE; thomasr playbook

    // Check for video capabilities
    vidInfo = SDL_GetVideoInfo();
    if(vidInfo->hw_available)
        vidFlags |= SDL_HWSURFACE;
    else
        vidFlags |= SDL_SWSURFACE;

    //printf("VidInfo w %d h %d\n", vidInfo->current_w, vidInfo->current_h);

    if(flags.fullscreen)
        vidFlags |= SDL_FULLSCREEN;

    if(flags.doublebuf)
        vidFlags |= SDL_DOUBLEBUF;

    // Calculate the window scale
    win_xscale = mouse_xscale = (flags.xres << 16) / xres;
    win_yscale = mouse_yscale = (flags.yres << 16) / yres;

    // Try using opengl hw accell
    if(flags.gl) {
#ifdef HAVE_OPENGL
        printf("Video : OpenGL enabled\n");
        // allow doublebuffering in with gl too
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, flags.doublebuf);
        // set video gl capability
        vidFlags = SDL_OPENGL; // clear sdl video flags
        if (flags.fullscreen)
        	vidFlags |= SDL_FULLSCREEN;
        // force no scaling, let the hw do it
        win_xscale = win_yscale = 1 << 16;
#else
        // ignore the option if not available
        printf("Video : OpenGL disabled (Support missing in executable)\n");
        flags.gl = 0;
#endif
    }

    // Try using opengl es 1 hw accell
    if(flags.gles1) {
#ifdef HAVE_OPENGLES1
        printf("Video : OpenGL ES 1 enabled\n");
        // allow doublebuffering in with gl too
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, flags.doublebuf);
        // set video gl capability
        vidFlags = SDL_OPENGL; // clear sdl video flags
		if (flags.fullscreen)
			vidFlags |= SDL_FULLSCREEN;
        // force no scaling, let the hw do it
        win_xscale = win_yscale = 1 << 16;
#else
        // ignore the option if not available
        printf("Video : OpenGL ES 1 disabled (Support missing in executable)\n");
        flags.gles1 = 0;
#endif
    }

    // Set the icon for this window.  Looks nice on taskbars etc.
    ///SDL_WM_SetIcon(SDL_LoadBMP("abuse.bmp"), NULL); // thomasr playbook

    // flags.xres and flags.yres will be 0 if scale is 0 in setup.cpp
    if ((flags.gl || flags.gles1) && flags.fullscreen && flags.xres == 0 && flags.yres == 0)
    {
    	xwinres = vidInfo->current_w;
    	ywinres = vidInfo->current_h;
    	float scalex = float(xwinres) / float(xres);
    	float scaley = float(ywinres) / float(yres);
    	float scale = scalex < scaley ? scalex : scaley;
    	flags.xres = short(xres * scale);
    	flags.yres = short(yres * scale);
    	flags.antialias = scale != float(int(scale)) ? GL_LINEAR : flags.antialias;
    	mouse_xscale = (flags.xres << 16) / xres;
    	mouse_yscale = (flags.yres << 16) / yres;
    	/*printf("scale %f\n", scale);
    	printf("xwinres %d\n", xwinres);
    	printf("ywinres %d\n", ywinres);
    	printf("flags.xres %i\n", flags.xres);
    	printf("flags.yres %i\n", flags.yres);
    	printf("flags.antialias %s\n", flags.antialias == GL_NEAREST ? "NEAREST" : flags.antialias == GL_LINEAR ? "LINEAR" : "<unknown>");
    	fflush(stdout);*/
    }

    // Create the window with a preference for 8-bit (palette animations!), but accept any depth */
    window = SDL_SetVideoMode(xwinres, ywinres, 8, vidFlags | SDL_ANYFORMAT);

    if(window == NULL)
    {
        printf("Video : Unable to set video mode : %s\n", SDL_GetError());
        exit(1);
    }

    // Create the screen image
    main_screen = new image(ivec2(xres, yres), NULL, 2);
    if(main_screen == NULL)
    {
        // Our screen image is no good, we have to bail.
        printf("Video : Unable to create screen image.\n");
        exit(1);
    }
    main_screen->clear();

#ifdef HAVE_OPENGL
    if (flags.gl)
    {
        int w, h;

        // texture width/height should be power of 2
        // FIXME: we can use GL_ARB_texture_non_power_of_two or
        // GL_ARB_texture_rectangle to avoid the extra memory allocation
        w = power_of_two(xres);
        h = power_of_two(yres);

        // create texture surface
        texture = SDL_CreateRGBSurface(SDL_SWSURFACE, w , h , 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
#else
                0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
#endif

        // setup 2D gl environment
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);

        glViewport(0, 0, window->w, window->h);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glOrtho(0.0, (GLdouble)window->w, (GLdouble)window->h, 0.0, 0.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // texture coordinates
        texcoord[0] = 0.0f;
        texcoord[1] = 0.0f;
        texcoord[2] = (GLfloat)xres / texture->w;
        texcoord[3] = (GLfloat)yres / texture->h;

        // create an RGBA texture for the texture surface
        glGenTextures(1, &texid);
        glBindTexture(GL_TEXTURE_2D, texid);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, flags.antialias);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, flags.antialias);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->w, texture->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->pixels);
    }
#endif

#ifdef HAVE_OPENGLES1
    if (flags.gles1)
    {
        int w, h;

        // texture width/height should be power of 2
        // FIXME: we can use GL_ARB_texture_non_power_of_two or
        // GL_ARB_texture_rectangle to avoid the extra memory allocation
        w = power_of_two(xres);
        h = power_of_two(yres);

        printf("Creating texture surface: %i, %i\n", w, h);
        // create texture surface
        texture = SDL_CreateRGBSurface(SDL_SWSURFACE, w , h , 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
#else
                0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
#endif

        // setup 2D gl environment
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        printf("Setting viewport to %i, %i\n", window->w, window->h);
        glViewport(0, 0, window->w, window->h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glOrthof(0.0, (GLfloat)window->w, (GLfloat)window->h, 0.0, 0.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // create an RGBA texture for the texture surface
        glGenTextures(1, &texid);
        glBindTexture(GL_TEXTURE_2D, texid);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, flags.antialias);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, flags.antialias);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->w, texture->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->pixels);

        if (has_multitouch)
        {
        	SDL_Surface* touchOverlay8 = SDL_LoadBMP("app/native/touchoverlay.bmp");
        	SDL_Surface* touchOverlay32 = SDL_CreateRGBSurface(SDL_SWSURFACE, touchOverlay8->w, touchOverlay8->h , 32,
        	#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        	                0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        	#else
        	                0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
        	#endif

        	SDL_BlitSurface(touchOverlay8, NULL, touchOverlay32, NULL);

        	glGenTextures(1, &touchoverlayid);
        	glBindTexture(GL_TEXTURE_2D, touchoverlayid);
        	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, touchOverlay32->w, touchOverlay32->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, touchOverlay32->pixels);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        	if (touchOverlay8)
        		SDL_FreeSurface(touchOverlay8);
        	if (touchOverlay32)
        		SDL_FreeSurface(touchOverlay32);
        	glBindTexture(GL_TEXTURE_2D, texid);

        	touch.resize();
        }
    }
#endif

    // Create our 8-bit surface
    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, window->w, window->h, 8, 0xff, 0xff, 0xff, 0xff);
    if(surface == NULL)
    {
        // Our surface is no good, we have to bail.
        printf("Video : Unable to create 8-bit surface.\n");
        exit(1);
    }

	printf("Video : %dx%d %dbpp\n", window->w, window->h, window->format->BitsPerPixel);

    // Set the window caption
    SDL_WM_SetCaption("Abuse", "Abuse");

    // Grab and hide the mouse cursor
    SDL_ShowCursor(0);
    if(flags.grabmouse)
        SDL_WM_GrabInput(SDL_GRAB_ON);

    update_dirty(main_screen);
}

//
// close_graphics()
// Shutdown the video mode
//
void close_graphics()
{
    if(lastl)
        delete lastl;
    lastl = NULL;
    // Free our 8-bit surface
    if(surface)
        SDL_FreeSurface(surface);

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES1)
    if (texture)
        SDL_FreeSurface(texture);
#endif
    delete main_screen;
}

// put_part_image()
// Draw only dirty parts of the image
//
void put_part_image(image *im, int x, int y, int x1, int y1, int x2, int y2)
{
    int xe, ye;
    SDL_Rect srcrect, dstrect;
    int ii, jj;
    int srcx, srcy, xstep, ystep;
    Uint8 *dpixel;
    Uint16 dinset;

    if(y > yres || x > xres)
        return;

    CHECK(x1 >= 0 && x2 >= x1 && y1 >= 0 && y2 >= y1);

    // Adjust if we are trying to draw off the screen
    if(x < 0)
    {
        x1 += -x;
        x = 0;
    }
    srcrect.x = x1;
    if(x + (x2 - x1) >= xres)
        xe = xres - x + x1 - 1;
    else
        xe = x2;

    if(y < 0)
    {
        y1 += -y;
        y = 0;
    }
    srcrect.y = y1;
    if(y + (y2 - y1) >= yres)
        ye = yres - y + y1 - 1;
    else
        ye = y2;

    if(srcrect.x >= xe || srcrect.y >= ye)
        return;

    // Scale the image onto the surface
    srcrect.w = xe - srcrect.x;
    srcrect.h = ye - srcrect.y;
    dstrect.x = ((x * win_xscale) >> 16);
    dstrect.y = ((y * win_yscale) >> 16);
    dstrect.w = ((srcrect.w * win_xscale) >> 16);
    dstrect.h = ((srcrect.h * win_yscale) >> 16);

    xstep = (srcrect.w << 16) / dstrect.w;
    ystep = (srcrect.h << 16) / dstrect.h;

    srcy = ((srcrect.y) << 16);
    dinset = ((surface->w - dstrect.w)) * surface->format->BytesPerPixel;

    // Lock the surface if necessary
    if(SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);

    dpixel = (Uint8 *)surface->pixels;
    dpixel += (dstrect.x + ((dstrect.y) * surface->w)) * surface->format->BytesPerPixel;

    // Update surface part
    if ((win_xscale==1<<16) && (win_yscale==1<<16)) // no scaling or hw scaling
    {
        srcy = srcrect.y;
        dpixel = ((Uint8 *)surface->pixels) + y * surface->w + x ;
        for(ii=0 ; ii < srcrect.h; ii++)
        {
            memcpy(dpixel, im->scan_line(srcy) + srcrect.x , srcrect.w);
            dpixel += surface->w;
            srcy ++;
        }
    }
    else    // sw scaling
    {
        xstep = (srcrect.w << 16) / dstrect.w;
        ystep = (srcrect.h << 16) / dstrect.h;

        srcy = ((srcrect.y) << 16);
        dinset = ((surface->w - dstrect.w)) * surface->format->BytesPerPixel;

        dpixel = (Uint8 *)surface->pixels + (dstrect.x + ((dstrect.y) * surface->w)) * surface->format->BytesPerPixel;

        for(ii = 0; ii < dstrect.h; ii++)
        {
            srcx = (srcrect.x << 16);
            for(jj = 0; jj < dstrect.w; jj++)
            {
                memcpy(dpixel, im->scan_line((srcy >> 16)) + ((srcx >> 16) * surface->format->BytesPerPixel), surface->format->BytesPerPixel);
                dpixel += surface->format->BytesPerPixel;
                srcx += xstep;
            }
            dpixel += dinset;
            srcy += ystep;
        }
//        dpixel += dinset;
//        srcy += ystep;
    }

    // Unlock the surface if we locked it.
    if(SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    // Now blit the surface
    update_window_part(&dstrect);
}

//
// load()
// Set the palette
//
void palette::load()
{
    if(lastl)
        delete lastl;
    lastl = copy();

    // Force to only 256 colours.
    // Shouldn't be needed, but best to be safe.
    if(ncolors > 256)
        ncolors = 256;

    SDL_Color colors[ncolors];
    for(int ii = 0; ii < ncolors; ii++)
    {
        colors[ii].r = red(ii);
        colors[ii].g = green(ii);
        colors[ii].b = blue(ii);
    }
    SDL_SetColors(surface, colors, 0, ncolors);
    if(window->format->BitsPerPixel == 8)
        SDL_SetColors(window, colors, 0, ncolors);

    // Now redraw the surface
    update_window_part(NULL);
    update_window_done();
}

//
// load_nice()
//
void palette::load_nice()
{
    load();
}

// ---- support functions ----

void update_window_done()
{
#ifdef HAVE_OPENGL
    // opengl blit complete surface to window
    if(flags.gl)
    {
        // convert color-indexed surface to RGB texture
        SDL_BlitSurface(surface, NULL, texture, NULL);

        // Texturemap complete texture to surface so we have free scaling
        // and antialiasing
        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0, texture->w, texture->h,
                        GL_RGBA, GL_UNSIGNED_BYTE, texture->pixels);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(texcoord[0], texcoord[1]); glVertex3i(0, 0, 0);
        glTexCoord2f(texcoord[2], texcoord[1]); glVertex3i(window->w, 0, 0);
        glTexCoord2f(texcoord[0], texcoord[3]); glVertex3i(0, window->h, 0);
        glTexCoord2f(texcoord[2], texcoord[3]); glVertex3i(window->w, window->h, 0);
        glEnd();

        if(flags.doublebuf)
            SDL_GL_SwapBuffers();
    }
	else
#endif
#ifdef HAVE_OPENGLES1
    // opengl blit complete surface to window
    if(flags.gles1)
    {
        // convert color-indexed surface to RGB texture
        SDL_BlitSurface(surface, NULL, texture, NULL);

        float x1 = xwinres / 2 - flags.xres / 2;
        float x2 = xwinres / 2 + flags.xres / 2;
        float y1 = ywinres / 2 - flags.yres / 2;
        float y2 = ywinres / 2 + flags.yres / 2;
        float vertices[] = {
			x1, y1, 0,
			x2, y1, 0,
			x1, y2, 0,
			x2, y2, 0,
		};

		float x = (float)xres / texture->w;
        float y = (float)yres / texture->h;
		float texcoords[] = {
			0, 0,
			x, 0,
			0, y,
			x, y,
		};

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glVertexPointer(3, GL_FLOAT, 0, vertices);
		glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

		glBindTexture(GL_TEXTURE_2D, texid);

        // Texturemap complete texture to surface so we have free scaling
        // and antialiasing
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture->w, texture->h, GL_RGBA, GL_UNSIGNED_BYTE, texture->pixels);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		touch.alpha = touch.visible() ? (touch.alpha + 1.0f/15.0f) : 0;
		touch.alpha = touch.alpha > 1 ? 1 : touch.alpha;
		if (touch.alpha > 0)
		{
			const float touchoverlay_vertices[] = {
				touch.move.top_left.x + 0 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 0
				touch.move.top_left.x + 0 * touch.move.size.x / 3, touch.move.top_left.y + 0 * touch.move.size.y / 3, // 1
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 2
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 0 * touch.move.size.y / 3, // 3
                                                                                                                      //
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 4
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 0 * touch.move.size.y / 3, // 5
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 6
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 0 * touch.move.size.y / 3, // 7
                                                                                                                      //
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 8
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 0 * touch.move.size.y / 3, // 9
				touch.move.top_left.x + 3 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 10
				touch.move.top_left.x + 3 * touch.move.size.x / 3, touch.move.top_left.y + 0 * touch.move.size.y / 3, // 11
                                                                                                                      //
				touch.move.top_left.x + 0 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 12
				touch.move.top_left.x + 0 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 13
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 14
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 15
                                                                                                                      //
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 16
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 17
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 18
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 19
                                                                                                                      //
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 20
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 21
				touch.move.top_left.x + 3 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 22
				touch.move.top_left.x + 3 * touch.move.size.x / 3, touch.move.top_left.y + 1 * touch.move.size.y / 3, // 23
                                                                                                                      //
				touch.move.top_left.x + 0 * touch.move.size.x / 3, touch.move.top_left.y + 3 * touch.move.size.y / 3, // 24
				touch.move.top_left.x + 0 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 25
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 3 * touch.move.size.y / 3, // 26
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 27
                                                                                                                      //
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 3 * touch.move.size.y / 3, // 28
				touch.move.top_left.x + 1 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 29
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 3 * touch.move.size.y / 3, // 30
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 31
                                                                                                                      //
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 3 * touch.move.size.y / 3, // 32
				touch.move.top_left.x + 2 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 33
				touch.move.top_left.x + 3 * touch.move.size.x / 3, touch.move.top_left.y + 3 * touch.move.size.y / 3, // 34
				touch.move.top_left.x + 3 * touch.move.size.x / 3, touch.move.top_left.y + 2 * touch.move.size.y / 3, // 35

				// aim
				touch.aim.top_left.x, touch.aim.top_left.y,                                                           // 36
				touch.aim.top_left.x + touch.aim.size.x, touch.aim.top_left.y,                                        // 37
				touch.aim.top_left.x, (touch.aim.top_left + touch.aim.size).y,                                        // 38
				(touch.aim.top_left + touch.aim.size).x, (touch.aim.top_left + touch.aim.size).y,                     // 39
			};

			const float touchoverlay_colors[] = {
					1, 1, 1, touch.alpha, // 0
					1, 1, 1, touch.alpha, // 1
					1, 1, 1, touch.alpha, // 2
					1, 1, 1, touch.alpha, // 3
					1, 1, 1, touch.alpha, // 4
					1, 1, 1, touch.alpha, // 5
					1, 1, 1, touch.alpha, // 6
					1, 1, 1, touch.alpha, // 7
					1, 1, 1, touch.alpha, // 8
					1, 1, 1, touch.alpha, // 9
					1, 1, 1, touch.alpha, // 10
					1, 1, 1, touch.alpha, // 11
					1, 1, 1, touch.alpha, // 12
					1, 1, 1, touch.alpha, // 13
					1, 1, 1, touch.alpha, // 14
					1, 1, 1, touch.alpha, // 15
					1, 1, 1, touch.alpha, // 16
					1, 1, 1, touch.alpha, // 17
					1, 1, 1, touch.alpha, // 18
					1, 1, 1, touch.alpha, // 19
					1, 1, 1, touch.alpha, // 20
					1, 1, 1, touch.alpha, // 21
					1, 1, 1, touch.alpha, // 22
					1, 1, 1, touch.alpha, // 23
					1, 1, 1, touch.alpha, // 24
					1, 1, 1, touch.alpha, // 25
					1, 1, 1, touch.alpha, // 26
					1, 1, 1, touch.alpha, // 27
					1, 1, 1, touch.alpha, // 28
					1, 1, 1, touch.alpha, // 29
					1, 1, 1, touch.alpha, // 30
					1, 1, 1, touch.alpha, // 31
					1, 1, 1, touch.alpha, // 32
					1, 1, 1, touch.alpha, // 33
					1, 1, 1, touch.alpha, // 34
					1, 1, 1, touch.alpha, // 35
					1, 1, 1, touch.alpha, // 36
					1, 1, 1, touch.alpha, // 37
					1, 1, 1, touch.alpha, // 38
					1, 1, 1, touch.alpha, // 39
			};

			const float imgw = 240.0f;
			const float imgh = 240.0f;
			const float texw = 512.0f;
			const float texh = 512.0f;

			const float move_upleft_offset = (touch.move.move_up_pressed && touch.move.move_left_pressed) ? 0.5f : 0;
			const float move_up_offset = (touch.move.move_up_pressed && !touch.move.move_left_pressed && !touch.move.move_right_pressed) ? 0.5f: 0;
			const float move_upright_offset = (touch.move.move_up_pressed && touch.move.move_right_pressed) ? 0.5f: 0;
			const float move_left_offset = (touch.move.move_left_pressed && !touch.move.move_up_pressed && !touch.move.move_down_pressed) ? 0.5f: 0;
			const float move_right_offset = (touch.move.move_right_pressed && !touch.move.move_up_pressed && !touch.move.move_down_pressed) ? 0.5f: 0;
			const float move_downleft_offset = (touch.move.move_down_pressed && touch.move.move_left_pressed) ? 0.5f: 0;
			const float move_down_offset = (touch.move.move_down_pressed && !touch.move.move_left_pressed && !touch.move.move_right_pressed) ? 0.5f: 0;
			const float move_downright_offset = (touch.move.move_down_pressed && touch.move.move_right_pressed) ? 0.5f: 0;

			const float touchoverlay_texcoords[] = {
				0 * imgw / (texw * 3) + move_upleft_offset, 1 * imgh / (texh * 3),
				0 * imgw / (texw * 3) + move_upleft_offset, 0 * imgh / (texh * 3),
				1 * imgw / (texw * 3) + move_upleft_offset, 1 * imgh / (texh * 3),
				1 * imgw / (texw * 3) + move_upleft_offset, 0 * imgh / (texh * 3),

				1 * imgw / (texw * 3) + move_up_offset, 1 * imgh / (texh * 3),
				1 * imgw / (texw * 3) + move_up_offset, 0 * imgh / (texh * 3),
				2 * imgw / (texw * 3) + move_up_offset, 1 * imgh / (texh * 3),
				2 * imgw / (texw * 3) + move_up_offset, 0 * imgh / (texh * 3),

				2 * imgw / (texw * 3) + move_upright_offset, 1 * imgh / (texh * 3),
				2 * imgw / (texw * 3) + move_upright_offset, 0 * imgh / (texh * 3),
				3 * imgw / (texw * 3) + move_upright_offset, 1 * imgh / (texh * 3),
				3 * imgw / (texw * 3) + move_upright_offset, 0 * imgh / (texh * 3),

				0 * imgw / (texw * 3) + move_left_offset, 2 * imgh / (texh * 3),
				0 * imgw / (texw * 3) + move_left_offset, 1 * imgh / (texh * 3),
				1 * imgw / (texw * 3) + move_left_offset, 2 * imgh / (texh * 3),
				1 * imgw / (texw * 3) + move_left_offset, 1 * imgh / (texh * 3),

				1 * imgw / (texw * 3), 2 * imgh / (texh * 3),
				1 * imgw / (texw * 3), 1 * imgh / (texh * 3),
				2 * imgw / (texw * 3), 2 * imgh / (texh * 3),
				2 * imgw / (texw * 3), 1 * imgh / (texh * 3),

				2 * imgw / (texw * 3) + move_right_offset, 2 * imgh / (texh * 3),
				2 * imgw / (texw * 3) + move_right_offset, 1 * imgh / (texh * 3),
				3 * imgw / (texw * 3) + move_right_offset, 2 * imgh / (texh * 3),
				3 * imgw / (texw * 3) + move_right_offset, 1 * imgh / (texh * 3),

				0 * imgw / (texw * 3) + move_downleft_offset, 3 * imgh / (texh * 3),
				0 * imgw / (texw * 3) + move_downleft_offset, 2 * imgh / (texh * 3),
				1 * imgw / (texw * 3) + move_downleft_offset, 3 * imgh / (texh * 3),
				1 * imgw / (texw * 3) + move_downleft_offset, 2 * imgh / (texh * 3),

				1 * imgw / (texw * 3) + move_down_offset, 3 * imgh / (texh * 3),
				1 * imgw / (texw * 3) + move_down_offset, 2 * imgh / (texh * 3),
				2 * imgw / (texw * 3) + move_down_offset, 3 * imgh / (texh * 3),
				2 * imgw / (texw * 3) + move_down_offset, 2 * imgh / (texh * 3),

				2 * imgw / (texw * 3) + move_downright_offset, 3 * imgh / (texh * 3),
				2 * imgw / (texw * 3) + move_downright_offset, 2 * imgh / (texh * 3),
				3 * imgw / (texw * 3) + move_downright_offset, 3 * imgh / (texh * 3),
				3 * imgw / (texw * 3) + move_downright_offset, 2 * imgh / (texh * 3),

				// aim
				0, 256.0f / texh,
				imgw / texw, 256.0f / texh,
				0, 256.0f / texh + imgh / texh,
				imgw / texw, 256.0f / texh + imgh / texh,
			};

			GLubyte touchoverlay_move_indices[] = {
				0, 1, 2, 3,  3, 4,
				4, 5, 6, 7,  7, 8,
				8, 9, 10, 11,  11, 12,

				12, 13, 14, 15,  15, 16,
				16, 17, 18, 19,  19, 20,
				20, 21, 22, 23,  23, 24,

				24, 25, 26, 27,  27, 28,
				28, 29, 30, 31,  31, 32,
				32, 33, 34, 35,
			};

			glEnableClientState(GL_COLOR_ARRAY);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glVertexPointer(2, GL_FLOAT, 0, touchoverlay_vertices);
			glColorPointer(4, GL_FLOAT, 0, touchoverlay_colors);
			glTexCoordPointer(2, GL_FLOAT, 0, touchoverlay_texcoords);
			glBindTexture(GL_TEXTURE_2D, touchoverlayid);
			glDrawElements(GL_TRIANGLE_STRIP, sizeof(touchoverlay_move_indices)/sizeof(touchoverlay_move_indices[0]), GL_UNSIGNED_BYTE, touchoverlay_move_indices); // draw movement
			glPushMatrix();
			glTranslatef(touch.aim.top_left.x + touch.aim.size.x / 2, touch.aim.top_left.y + touch.aim.size.y / 2, 0);
			extern int _best_angle;
			int angle = 360 - _best_angle; // flip rotation horizontally
			glRotatef(angle, 0, 0, 1);
			glTranslatef(-touch.aim.top_left.x - touch.aim.size.x / 2, -touch.aim.top_left.y - touch.aim.size.y / 2, 0);
			glDrawArrays(GL_TRIANGLE_STRIP, 36, 4); // draw aim
			glPopMatrix();
			glDisable(GL_BLEND);
			glDisableClientState(GL_COLOR_ARRAY);
		}

        if(flags.doublebuf)
            SDL_GL_SwapBuffers();
    }
	else
#endif
    // swap buffers in case of double buffering
    // do nothing in case of single buffering
    if(flags.doublebuf)
        SDL_Flip(window);
}

static void update_window_part(SDL_Rect *rect)
{
    // no partial blit's in case of opengl
    // complete blit + scaling just before flip
    if (flags.gl || flags.gles1)
        return;

    SDL_BlitSurface(surface, rect, window, rect);

	// no window update needed until end of run
    if(flags.doublebuf)
        return;

    // update window part for single buffer
    if(rect == NULL)
        SDL_UpdateRect(window, 0, 0, 0, 0);
    else
        SDL_UpdateRect(window, rect->x, rect->y, rect->w, rect->h);
}

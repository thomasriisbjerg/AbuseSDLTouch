/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef _SETUP_H_
#define _SETUP_H_

struct flags_struct
{
    short fullscreen;
    short doublebuf;
    short mono;
    short nosound;
    short grabmouse;
    short hidemouse;
    short nosdlparachute;
    short xres;
    short yres;
    short overlay;
    short gl;
    short gles1;
    short use_multitouch;
    int antialias;
    const char *language;
};

struct keys_struct
{
    int left;
    int right;
    int up;
    int down;
    int b1;
    int b2;
    int b3;
    int b4;
};

extern flags_struct flags;

int get_key_binding(char const *dir, int i);

const char *get_default_language();

#endif // _SETUP_H_

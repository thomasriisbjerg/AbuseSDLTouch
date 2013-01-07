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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <SDL.h>
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif    /* __APPLE__ */
#ifdef HAVE_OPENGL
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif    /* __APPLE__ */
#endif    /* HAVE_OPENGL */
#ifdef HAVE_OPENGLES1
#include <GLES/gl.h>
#endif    /* HAVE_OPENGLES1 */
#ifdef __QNXNTO__
#include <bps/bps.h>
#include <bps/locale.h>
#endif // __QNXNTO__

#include "specs.h"
#include "keys.h"
#include "setup.h"

flags_struct flags;
keys_struct keys;

extern int xres, yres;
extern int xwinres, ywinres;
static unsigned int scale;

//
// Display help
//
void showHelp()
{
    printf( "\n" );
    printf( "Usage: abuse.sdl [options]\n" );
    printf( "Options:\n\n" );
    printf( "** Abuse Options **\n" );
    printf( "  -size <arg>       Set the size of the screen\n" );
    printf( "  -edit             Startup in editor mode\n" );
    printf( "  -a <arg>          Use addon named <arg>\n" );
    printf( "  -f <arg>          Load map file named <arg>\n" );
    printf( "  -lisp             Startup in lisp interpreter mode\n" );
    printf( "  -nodelay          Run at maximum speed\n" );
    printf( "\n" );
    printf( "** Abuse-SDL Options **\n" );
    printf( "  -datadir <arg>    Set the location of the game data to <arg>\n" );
    printf( "  -doublebuf        Enable double buffering\n" );
    printf( "  -fullscreen       Enable fullscreen mode\n" );
#ifdef HAVE_OPENGL
    printf( "  -gl               Enable OpenGL\n" );
#endif
#ifdef HAVE_OPENGLES1
    printf( "  -gles1            Enable OpenGL ES 1\n" );
#endif
#if defined (HAVE_OPENGL) || defined (HAVE_OPENGLES1)
    printf( "  -antialias        Enable anti-aliasing (with -gl only)\n" );
#endif
    printf( "  -h, --help        Display this text\n" );
    printf( "  -mono             Disable stereo sound\n" );
    printf( "  -nosound          Disable sound\n" );
    printf( "  -hidemouse        Hide the mouse cursor\n" );
    printf( "  -scale <arg>      Scale to <arg>\n" );
    printf( "  -touch_scale <x> <y> Touch-screen controls scale <x> <y>\n" );
//    printf( "  -x <arg>          Set the width to <arg>\n" );
//    printf( "  -y <arg>          Set the height to <arg>\n" );
    printf( "  -language <arg>   Select english (default), french or german language\n" );
    printf( "\n" );
    printf( "Anthony Kruize <trandor@labyrinth.net.au>\n" );
    printf( "\n" );
}

//
// Create a default 'abuserc' file
//
void createRCFile(char *rcfile) {
    FILE *fd = NULL;

    if ((fd = fopen(rcfile, "w")) != NULL)
    {
        fprintf(fd, "; Abuse-SDL Configuration file\n\n");
        fprintf(fd, "; Startup fullscreen\nfullscreen=%i\n\n",
                flags.fullscreen);
        fprintf(fd, "; Use DoubleBuffering\ndoublebuf=%i\n\n", flags.doublebuf);
        fprintf(fd, "; Use OpenGL\ngl=%i\n\n", flags.gl);
        fprintf(fd, "; Use OpenGL ES 1\ngles1=%i\n\n", flags.gles1);
#ifndef __APPLE__
        fprintf(fd, "; Location of the datafiles\ndatadir=%s\n\n", get_filename_prefix());
#endif
        fprintf(fd, "; Use mono audio only\nmono=%i\n\n", flags.mono);
        fprintf(fd, "; Grab the mouse to the window\ngrabmouse=%i\n\n", flags.grabmouse);
        fprintf(fd, "; Set the scale factor\nscale=%i\n\n", scale);
        fprintf(fd, "; Use anti-aliasing (with gl=1 only)\nantialias=%i\n\n", flags.antialias);
        fprintf(fd, "; Hide the mouse cursor\nhidemouse=%i\n\n", flags.hidemouse);
        fprintf(fd, "; Hide the mouse cursor\nuse_multitouch=%i\n\n", flags.use_multitouch);
        fprintf(fd, "; Touch-screen controls horizontal scale\ntouch_scale_x=%f\n\n", flags.touch_scale_x);
        fprintf(fd, "; Touch-screen controls vertical scale\ntouch_scale_y=%f\n\n", flags.touch_scale_y);
//        fprintf( fd, "; Set the width of the window\nx=%i\n\n", flags.xres );
//        fprintf( fd, "; Set the height of the window\ny=%i\n\n", flags.yres );
        fprintf(fd, "; Disable the SDL parachute in the case of a crash\nnosdlparachute=%i\n\n", flags.nosdlparachute);
#if !defined(__QNXNTO__) // don't save language setting on BB10, use system setting
        fprintf(fd, "; Select language\nlanguage=%s\n\n", flags.language);
#endif
        fprintf(fd, "; Key mappings\n");
        // TODO: print actual default values for key bindings using key_name(int, char*)
        fprintf(fd, "left=LEFT\nright=RIGHT\nup=UP\ndown=DOWN\n");
        fprintf(fd, "fire=SPACE\nspecial=SHIFT_L\nweapprev=CTRL_R\nweapnext=INSERT\n");
        fclose(fd);
    }
    else
    {
        printf("Unable to create 'abuserc' file.\n");
    }
}

//
// Read in the 'abuserc' file
//
void readRCFile()
{
    FILE *fd = NULL;
    char buf[255];
    char *result;

    const size_t rcfilesize = 256;
    char rcfile[rcfilesize];
    snprintf(rcfile, rcfilesize, "%s/abuserc", get_save_filename_prefix());
    if( (fd = fopen( rcfile, "r" )) != NULL )
    {
        while( fgets( buf, sizeof( buf ), fd ) != NULL )
        {
            result = strtok( buf, "=" );
            if( strcasecmp( result, "fullscreen" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.fullscreen = atoi( result );
            }
            else if( strcasecmp( result, "doublebuf" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.doublebuf = atoi( result );
            }
            else if( strcasecmp( result, "mono" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.mono = atoi( result );
            }
            else if( strcasecmp( result, "grabmouse" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.grabmouse = atoi( result );
            }
            else if( strcasecmp( result, "scale" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                scale = atoi( result );
            }
/*            else if( strcasecmp( result, "x" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.xres = atoi( result );
            }
            else if( strcasecmp( result, "y" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.yres = atoi( result );
            }*/
            else if( strcasecmp( result, "gl" ) == 0 )
            {
                // We leave this in even if we don't have OpenGL so we can
                // at least inform the user.
                result = strtok( NULL, "\n" );
                flags.gl = atoi( result );
            }
            else if( strcasecmp( result, "gles1" ) == 0 )
            {
                // We leave this in even if we don't have OpenGL ES so we can
                // at least inform the user.
                result = strtok( NULL, "\n" );
                flags.gles1 = atoi( result );
            }
#ifdef HAVE_OPENGL
            else if( strcasecmp( result, "antialias" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                if( atoi( result ) )
                {
                    flags.antialias = GL_LINEAR;
                }
            }
#endif
            else if( strcasecmp( result, "nosdlparachute" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.nosdlparachute = atoi( result );
            }
            else if( strcasecmp( result, "datadir" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                set_filename_prefix( result );
            }
            else if( strcasecmp( result, "hidemouse" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.hidemouse = atoi( result );
            }
            else if ( strcasecmp(result, "use_multitouch" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.use_multitouch = atoi( result );
            }
            else if ( strcasecmp(result, "touch_scale_x" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.touch_scale_x = (float)atof( result );
            }
            else if ( strcasecmp(result, "touch_scale_y" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.touch_scale_y = (float)atof( result );
            }
            else if ( strcasecmp(result, "language" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                if (strncmp(result, "french", sizeof("french")) == 0)
                	flags.language = "french";
                else if (strncmp(result, "german", sizeof("german")) == 0)
                	flags.language = "german";
                else // default to english
                	flags.language = "english"; // we're intentionally referring to a compile-time constant string here
            }
            else if( strcasecmp( result, "left" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.left = key_value( result );
            }
            else if( strcasecmp( result, "right" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.right = key_value( result );
            }
            else if( strcasecmp( result, "up" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.up = key_value( result );
            }
            else if( strcasecmp( result, "down" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.down = key_value( result );
            }
            else if( strcasecmp( result, "fire" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.b2 = key_value( result );
            }
            else if( strcasecmp( result, "special" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.b1 = key_value( result );
            }
            else if( strcasecmp( result, "weapprev" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.b3 = key_value( result );
            }
            else if( strcasecmp( result, "weapnext" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.b4 = key_value( result );
            }
        }
        fclose(fd);
    }
    else
    {
        // Couldn't open the abuserc file so let's create a default one
        createRCFile( rcfile );
    }
}

//
// Parse the command-line parameters
//
void parseCommandLine( int argc, char **argv )
{
    for( int ii = 1; ii < argc; ii++ )
    {
        if( !strcasecmp( argv[ii], "-fullscreen" ) )
        {
            flags.fullscreen = 1;
        }
        else if( !strcasecmp( argv[ii], "-doublebuf" ) )
        {
            flags.doublebuf = 1;
        }
        else if( !strcasecmp( argv[ii], "-size" ) )
        {
            if( ii + 1 < argc && !sscanf( argv[++ii], "%d", &xres ) )
            {
                xres = 320;
            }
            if( ii + 1 < argc && !sscanf( argv[++ii], "%d", &yres ) )
            {
                yres = 200;
            }
        }
        else if( !strcasecmp( argv[ii], "-scale" ) )
        {
            int result;
            if( sscanf( argv[++ii], "%d", &result ) )
            {
                scale = result;
            }
        }
/*        else if( !strcasecmp( argv[ii], "-x" ) )
        {
            int x;
            if( sscanf( argv[++ii], "%d", &x ) )
            {
                flags.xres = x;
            }
        }
        else if( !strcasecmp( argv[ii], "-y" ) )
        {
            int y;
            if( sscanf( argv[++ii], "%d", &y ) )
            {
                flags.yres = y;
            }
        }*/
        else if (!strcasecmp(argv[ii], "-nosound")) {
            flags.nosound = 1;
        }
        else if( !strcasecmp( argv[ii], "-gl" ) )
        {
            // We leave this in even if we don't have OpenGL so we can
            // at least inform the user.
            flags.gl = 1;
        }
        else if ( !strcasecmp( argv[ii], "-gles1" ) )
        {
            // We leave this in even if we don't have OpenGL ES so we can
            // at least inform the user.
            flags.gles1 = 1;
        }
#if defined HAVE_OPENGL || defined HAVE_OPENGLES1
        else if( !strcasecmp( argv[ii], "-antialias" ) )
        {
            flags.antialias = GL_LINEAR;
        }
#endif
        else if (!strcasecmp(argv[ii], "-mono")) {
            flags.mono = 1;
        }
        else if( !strcasecmp( argv[ii], "-datadir" ) )
        {
            char datadir[255];
            if( ii + 1 < argc && sscanf( argv[++ii], "%s", datadir ) )
            {
                set_filename_prefix( datadir );
            }
        }
        else if( !strcasecmp( argv[ii], "-h" ) || !strcasecmp( argv[ii], "--help" ) )
        {
            showHelp();
            exit( 0 );
        }
        else if( !strcasecmp( argv[ii], "-hidemouse" ) )
        {
            flags.hidemouse = 1;
        }
        else if( !strcasecmp(argv[ii], "-use_multitouch" ) )
        {
            flags.use_multitouch = 1;
        }
        else if( !strcasecmp(argv[ii], "-touch_scale" ) )
        {
        	float scale_x = 1.0f;
        	float scale_y = 1.0f;
        	if (ii + 2 < argc)
        	{
        		scale_x = atof(argv[ii + 1]);
        		scale_y = atof(argv[ii + 2]);
        	}
            flags.touch_scale_x = scale_x;
            flags.touch_scale_y = scale_y;
        }
        else if( !strcasecmp(argv[ii], "-language" ) )
        {
        	const size_t buffersize = 16;
            char buffer[buffersize];
            if (ii + 1 < argc)
            {
            	if (strncmp(argv[ii + 1], "french", buffersize))
            		flags.language = "french";
            	else if (strncmp(argv[ii + 1], "german", buffersize))
            		flags.language = "german";
            	else
            	{
            		printf("unknown language option '%s'\n", argv[ii]);
            		flags.language = "english";
            	}
            	++ii;
			}
        }
    }
}

//
// Setup SDL and configuration
//
void setup(int argc, char **argv) {
#ifdef __QNXNTO__
    bps_initialize();
#endif // __QNXNTO__

    // Initialise default settings
    flags.mono = 0; // Enable stereo sound
    flags.nosound = 0; // Enable sound
    flags.grabmouse = 0; // Don't grab the mouse
    flags.nosdlparachute = 0; // SDL error handling
    flags.xres = xwinres = xres = 320; // Default window width
    flags.yres = ywinres = yres = 200; // Default window height
#if defined(__APPLE__)
    flags.fullscreen = 0; // Start in a window
    flags.gl = 1; // Use opengl
    flags.doublebuf = 1;// Do double buffering
    flags.gles1 = 0;
    flags.hidemouse = 0;
    flags.use_multitouch = 0;
    scale = 1;
#elif defined (__QNXNTO__) // BB10 and PlayBook
    flags.fullscreen = 1;
    flags.gl = 0;
    flags.gles1 = 1;
    flags.doublebuf = 1;
    flags.hidemouse = 1;
    flags.use_multitouch = 1;
    flags.touch_scale_x = 1;
    flags.touch_scale_y = 1;
    scale = 0;
#else
    flags.fullscreen = 0; // Start in a window
    flags.gl = 0; // Don't use opengl
    flags.doublebuf = 0;// No double buffering
    flags.gles1 = 0;
    flags.hidemouse = 0;
    flags.use_multitouch = 0;
    scale = 1;
#endif
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES1)
    flags.antialias = GL_NEAREST; // Don't anti-alias
#endif
    flags.language = get_default_language();
    keys.up = key_value("UP");
    keys.down = key_value("DOWN");
    keys.left = key_value("LEFT");
    keys.right = key_value("RIGHT");
    keys.b1 = key_value("SHIFT_L");
    keys.b2 = key_value("SPACE");
    keys.b3 = key_value("CTRL_R");
    keys.b4 = key_value("INSERT");

    // Display our name and version
    //printf( "%s %s\n", PACKAGE_NAME, PACKAGE_VERSION );

    // Initialize SDL with video and audio support
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
    {
        printf( "Unable to initialise SDL : %s\n", SDL_GetError() );
        exit( 1 );
    }
    atexit( SDL_Quit );

    // Set the savegame directory
    char *homedir;
    FILE *fd = NULL;

    if ((homedir = getenv("HOME")) != NULL)
    {
        const size_t savedirsize = 256;
        char savedir[savedirsize];
        snprintf(savedir, savedirsize, "%s/.abuse/", homedir);
        // Check if we already have a savegame directory
        if ((fd = fopen(savedir, "r")) == NULL)
        {
            // FIXME: Add some error checking here
            mkdir(savedir, S_IRUSR | S_IWUSR | S_IXUSR);
        }
        else
        {
            fclose(fd);
        }
        set_save_filename_prefix(savedir);
    }
    else
    {
        // Warn the user that we couldn't set the savename prefix
        printf("WARNING: Unable to get $HOME environment variable.\n");
        printf("         Savegames will probably fail.\n");
        // Just use the working directory.
        // Hopefully they have write permissions....
        set_save_filename_prefix("");
    }

    // Set the datadir to a default value
    // (The current directory)
#ifdef __APPLE__
    UInt8 buffer[255];
    CFURLRef bundleurl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFURLRef url = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, bundleurl, CFSTR("Contents/Resources/data"), true);

    if (!CFURLGetFileSystemRepresentation(url, true, buffer, 255))
    {
        exit(1);
    }
    else
        set_filename_prefix( (const char*)buffer );
#else
    set_filename_prefix (ASSETDIR);
#endif

    // Load the users configuration
    readRCFile();

    // Handle command-line parameters
    parseCommandLine(argc, argv);

    // Calculate the scaled window size.
    flags.xres = xwinres = xres * scale;
    flags.yres = ywinres = yres * scale;

    // Stop SDL handling some errors
    if (flags.nosdlparachute) {
        // segmentation faults
        signal(SIGSEGV, SIG_DFL);
        // floating point errors
        signal(SIGFPE, SIG_DFL);
    }

    // dump flags
    printf("flags.fullscreen %d\n", flags.fullscreen);
    printf("flags.doublebuf %d\n", flags.doublebuf);
    printf("flags.mono %d\n", flags.mono);
    printf("flags.nosound %d\n", flags.nosound);
    printf("flags.grabmouse %d\n", flags.grabmouse);
    printf("flags.nosdlparachute %d\n", flags.nosdlparachute);
    printf("flags.xres %d\n", flags.xres);
    printf("flags.yres %d\n", flags.yres);
    printf("flags.gl %d\n", flags.gl);
    printf("flags.gles1 %d\n", flags.gles1);
    printf("flags.antialias %s\n", flags.antialias == GL_NEAREST ? "NEAREST" : flags.antialias == GL_LINEAR ? "LINEAR" : "<unknown>");
    printf("flags.hidemouse %d\n", flags.hidemouse);
    printf("flags.use_multitouch %d\n", flags.use_multitouch);
    printf("flags.language %s\n", flags.language);
    printf("scale %d\n", scale);
    printf("flags done\n");
    fflush(stdout);
}

//
// Get the key binding for the requested function
//
int get_key_binding(char const *dir, int i)
{
    if (strcasecmp(dir, "left") == 0)
        return keys.left;
    else if (strcasecmp(dir, "right") == 0)
        return keys.right;
    else if (strcasecmp(dir, "up") == 0)
        return keys.up;
    else if (strcasecmp(dir, "down") == 0)
        return keys.down;
    else if (strcasecmp(dir, "b1") == 0)
        return keys.b1;
    else if (strcasecmp(dir, "b2") == 0)
        return keys.b2;
    else if (strcasecmp(dir, "b3") == 0)
        return keys.b3;
    else if (strcasecmp(dir, "b4") == 0)
        return keys.b4;

    return 0;
}

const char *get_default_language()
{
	const char *result = "english";
#ifdef __QNXNTO__
	char *country = 0;
	char *language = 0;
	int rc = locale_get(&language, &country);
	if (rc == BPS_SUCCESS)
	{
		if (strcmp(language, "fr") == 0)
			result = "french";
		if (strcmp(language, "de") == 0)
			result = "german";
		bps_free(language);
		bps_free(country);
	}
	else
	{
		printf("locale_get failed with return code %i, defaulting to english\n", rc); fflush(stdout);
	}
#endif // __QNXNTO__
	return result;
}

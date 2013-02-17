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
#include <SDL_syswm.h>

#include "common.h"

#include "image.h"
#include "palette.h"
#include "video.h"
#include "event.h"
#include "timing.h"
#include "sprite.h"
#include "game.h"
#include "dev.h"
#include "setup.h"
#include "joy.h"
#include "onlineservice.h"
#ifdef __QNXNTO__
#include <bps/navigator.h>
#endif

extern int has_multitouch;
extern int get_key_binding(char const *dir, int i);
extern int mouse_xscale, mouse_yscale;
extern int xwinres, ywinres;
extern flags_struct flags;
short mouse_buttons[5] = { 0, 0, 0, 0, 0 };

/*int touch_index = -1;
ivec2 aim_aa = ivec2(720, 360);
ivec2 aim_bb = ivec2(960, 600);
int aim_threshold = 30;
int move_touch_index = -1;
bool move_up_pressed = false;
bool move_down_pressed = false;
bool move_left_pressed = false;
bool move_right_pressed = false;
int move_threshold = 40;
ivec2 move_aa = ivec2(0, 360);
ivec2 move_bb = ivec2(240, 600);

int special_touch_index = -1;
bool special_pressed = false;
ivec2 special_aa = ivec2(320-33, 33);
ivec2 special_bb = ivec2(320, 2*33);
int pause_touch_index = -1;
bool pause_pressed = false;
ivec2 pause_aa = ivec2(85, 50);
ivec2 pause_bb = ivec2(320-85, 150);
int statbar_touch_index = -1;
bool statbar_pressed = false;
int statbar_weapon_width = 33;
int statbar_weapon_selected = -1;
ivec2 statbar_aa = ivec2(47, 0);
ivec2 statbar_bb = ivec2(47 + 7*33, 33);*/

void show_help(const char *msg, ...)
{
	va_list fmtargs;
	const int buffer_size = 200;
	char buffer[buffer_size];
	va_start(fmtargs, msg);
	vsnprintf(buffer, buffer_size, msg, fmtargs);
	va_end(fmtargs);
	the_game->show_help(buffer);
}

void EventHandler::SysInit()
{
    // Ignore activate events
    //SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
#ifdef WEBOS
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
#endif
}

void EventHandler::SysWarpMouse(ivec2 pos)
{
    SDL_WarpMouse(pos.x, pos.y);
}

//
// IsPending()
// Are there any events in the queue?
//
int EventHandler::IsPending()
{
    if (!m_pending && SDL_PollEvent(NULL))
        m_pending = 1;

    return m_pending;
}

void EventHandler::reset_keymap()
{
	m_button = 0;
	mouse_buttons[0] = 0;
	mouse_buttons[1] = 0;
	mouse_buttons[2] = 0;
	mouse_buttons[3] = 0;
	mouse_buttons[4] = 0;
}

//
// Get and handle waiting events
//
void EventHandler::SysEvent(Event &ev)
{
    // No more events
    m_pending = 0;

    // NOTE : that the mouse status should be known
    // even if another event has occurred.
    ev.mouse_move.x = m_pos.x;
    ev.mouse_move.y = m_pos.y;
    ev.mouse_button = m_button;

    // Gather next event
    SDL_Event sdlev;
    if (!SDL_PollEvent(&sdlev))
        return; // This should not happen

	// Sort the mouse out
	int x, y;
	uint8_t buttons = SDL_GetMouseState(&x, &y);

	// remove window offset
	x = x - (xwinres / 2 - flags.xres / 2);
	y = y - (ywinres / 2 - flags.yres / 2);

	// scale from window to main_screen coords
	x = (x << 16) / mouse_xscale;
	y = (y << 16) / mouse_yscale;

	// clamp to main_screen coords
	x = Min(Max(x, 0), main_screen->Size().x - 1);
	y = Min(Max(y, 0), main_screen->Size().y - 1);

    if (wm->m_first || !playing_state(the_game->state) || !has_multitouch)
    {
		ev.mouse_move.x = x;
		ev.mouse_move.y = y;
		ev.type = EV_MOUSE_MOVE;

		// Left button
		if((buttons & SDL_BUTTON(1)) && !mouse_buttons[1])
		{
			ev.type = EV_MOUSE_BUTTON;
			mouse_buttons[1] = !mouse_buttons[1];
			ev.mouse_button |= LEFT_BUTTON;
		}
		else if(!(buttons & SDL_BUTTON(1)) && mouse_buttons[1])
		{
			ev.type = EV_MOUSE_BUTTON;
			mouse_buttons[1] = !mouse_buttons[1];
			ev.mouse_button &= (0xff - LEFT_BUTTON);
		}

		// Middle button
		if((buttons & SDL_BUTTON(2)) && !mouse_buttons[2])
		{
			ev.type = EV_MOUSE_BUTTON;
			mouse_buttons[2] = !mouse_buttons[2];
			ev.mouse_button |= LEFT_BUTTON;
			ev.mouse_button |= RIGHT_BUTTON;
		}
		else if(!(buttons & SDL_BUTTON(2)) && mouse_buttons[2])
		{
			ev.type = EV_MOUSE_BUTTON;
			mouse_buttons[2] = !mouse_buttons[2];
			ev.mouse_button &= (0xff - LEFT_BUTTON);
			ev.mouse_button &= (0xff - RIGHT_BUTTON);
		}

		// Right button
		if((buttons & SDL_BUTTON(3)) && !mouse_buttons[3])
		{
			ev.type = EV_MOUSE_BUTTON;
			mouse_buttons[3] = !mouse_buttons[3];
			ev.mouse_button |= RIGHT_BUTTON;
		}
		else if(!(buttons & SDL_BUTTON(3)) && mouse_buttons[3])
		{
			ev.type = EV_MOUSE_BUTTON;
			mouse_buttons[3] = !mouse_buttons[3];
			ev.mouse_button &= (0xff - RIGHT_BUTTON);
		}
    }
    else if (m_button != 0)
    {
    	// Left button
		if(!(buttons & SDL_BUTTON(1)) && mouse_buttons[1])
		{
			ev.type = EV_MOUSE_BUTTON;
			mouse_buttons[1] = !mouse_buttons[1];
			ev.mouse_button &= (0xff - LEFT_BUTTON);
		}

		// Middle button
		if(!(buttons & SDL_BUTTON(2)) && mouse_buttons[2])
		{
			ev.type = EV_MOUSE_BUTTON;
			mouse_buttons[2] = !mouse_buttons[2];
			ev.mouse_button &= (0xff - LEFT_BUTTON);
			ev.mouse_button &= (0xff - RIGHT_BUTTON);
		}

		// Right button
		if(!(buttons & SDL_BUTTON(3)) && mouse_buttons[3])
		{
			ev.type = EV_MOUSE_BUTTON;
			mouse_buttons[3] = !mouse_buttons[3];
			ev.mouse_button &= (0xff - RIGHT_BUTTON);
		}
    }

    int _x;
    int _y;
    switch (sdlev.type)
    {
    case SDL_MOUSEMOTION:
    	_x = sdlev.motion.x;
    	_y = sdlev.motion.y;
    	break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    	_x = sdlev.button.x;
    	_y = sdlev.button.y;
    	break;
    default:
    	SDL_GetMouseState(&_x, &_y);
    	break;
    }

    // remove window offset
	x = _x - (xwinres / 2 - flags.xres / 2);
	y = _y - (ywinres / 2 - flags.yres / 2);

	// scale from window to main_screen coords
	x = (x << 16) / mouse_xscale;
	y = (y << 16) / mouse_yscale;

	// clamp to main_screen coords
	x = Min(Max(x, 0), main_screen->Size().x - 1);
	y = Min(Max(y, 0), main_screen->Size().y - 1);

    ivec2 touchview = ivec2(_x, _y);
    ivec2 touchgame = ivec2(x, y);

    // Sort out other kinds of events
    switch(sdlev.type)
    {
    case SDL_QUIT:
        if (current_level)
        {
        	current_level->save(autosavename, 1);
        }
        the_game->end_session();
        break;
    case SDL_ACTIVEEVENT:
    	if (wm->m_first == 0 && the_game->state == RUN_STATE && sdlev.active.gain == 0 && (sdlev.active.state & SDL_APPINPUTFOCUS))
    	{
    		// simulate a P keypress to pause the game
			ev.type = EV_KEY;
			ev.key = 'p';

			Event *e = new Event();
			e->type = EV_KEYRELEASE;
			e->key = 'p';
			e->mouse_move = m_pos;
			e->mouse_button = m_button;
			Push(e);
    	}
    	break;
    case SDL_MOUSEMOTION:
    	touch.mouse_motion(sdlev.button.which, touchgame, touchview, ev);
    	break;
    case SDL_MOUSEBUTTONUP:
        switch(sdlev.button.button)
        {
        case 1:
        	touch.mouse_up(sdlev.button.which, ev);
        	break;
        case 4:        // Mouse wheel goes up...
            ev.key = get_key_binding("b4", 0);
            ev.type = EV_KEYRELEASE;
            break;
        case 5:        // Mouse wheel goes down...
            ev.key = get_key_binding("b3", 0);
            ev.type = EV_KEYRELEASE;
            break;
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        switch(sdlev.button.button)
        {
        case 1:
        	touch.mouse_down(sdlev.button.which, touchgame, touchview, ev);
        	break;
        case 4:        // Mouse wheel goes up...
            ev.key = get_key_binding("b4", 0);
            ev.type = EV_KEY;
            break;
        case 5:        // Mouse wheel goes down...
            ev.key = get_key_binding("b3", 0);
            ev.type = EV_KEY;
            break;
        }
        break;
#if !defined WEBOS
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		ev.key = EV_SPURIOUS;
		if(sdlev.type == SDL_JOYBUTTONDOWN)
			ev.type = EV_KEY;
		else
			ev.type = EV_KEYRELEASE;

		joy_eval_button(sdlev.jbutton.which, sdlev.jbutton.button, ev.key);
/*		switch (sdlev.jbutton.button)
		{
			case 0:		ev.key = get_key_binding("up", 0); break; // dpad up
			case 1:		ev.key = get_key_binding("down", 0); break; // dpad down
			case 2:		ev.key = get_key_binding("left", 0); break; // dpad left
			case 3:		ev.key = get_key_binding("right", 0); break; // dpad right
			case 4:		ev.key = JK_SPACE; break; // start button
			case 5:		ev.key = JK_ESC; break; // back buttoninput
			case 8:		ev.key = get_key_binding("b3", 0); break; // left bumper
			case 9:		ev.key = get_key_binding("b4", 0); break; // right bumper
			case 11:	ev.key = JK_ENTER; break; // a button
			case 12:	ev.key = JK_ESC; break; // b button
		}*/
		break;
    case SDL_JOYAXISMOTION:
    	joy_eval_axis(sdlev.jaxis.which, sdlev.jaxis.axis, sdlev.jaxis.value, ev.type, ev.key);
/*		switch (sdlev.jaxis.axis)
		{
			case 0: // left analog x
				if (sdlev.jaxis.value < 16000 && sdlev.jaxis.value > -16000)
					ev.type = EV_KEYRELEASE;
				else
					ev.type = EV_KEY;
				ev.key=get_key_binding(sdlev.jaxis.value < 0 ? "left" : "right", 0);
				break;
			case 1: // left analog y
				if (sdlev.jaxis.value < 16000 && sdlev.jaxis.value > -16000)
					ev.type = EV_KEYRELEASE;
				else
					ev.type = EV_KEY;
				ev.key=get_key_binding(sdlev.jaxis.value < 0 ? "up" : "down", 0);
				break;
			case 2: // right analog stick x
				ev.type = EV_AIM_X;
				if (sdlev.jaxis.value < 3200 && sdlev.jaxis.value > -3200)
					ev.key = 0;
				else
					ev.key = sdlev.jaxis.value;
				break;
			case 3: // right analog stick y
				ev.type = EV_AIM_Y;
				if (sdlev.jaxis.value < 3200 && sdlev.jaxis.value > -3200)
					ev.key = 0;
				else
					ev.key = sdlev.jaxis.value;
				break;
			case 4: // left analog trigger
				if (sdlev.jaxis.value < 0x7FFF - 3200)
					ev.type = EV_KEYRELEASE;
				else
					ev.type = EV_KEY;
				ev.key=get_key_binding("b1", 0);
				break;
			case 5: // right analog trigger
				if (sdlev.jaxis.value < 0x7FFF - 3200)
					ev.type = EV_KEYRELEASE;
				else
					ev.type = EV_KEY;
				ev.key=get_key_binding("b2", 0);
				break;
			default:
				break;
		}*/
#endif // !defined WEBOS
		break;
#if defined __QNXNTO__
	case SDL_USEREVENT:
		switch (sdlev.user.code)
		{
		case NAVIGATOR_SWIPE_DOWN:
			ev.type = EV_KEY;
			ev.key = JK_ESC;
			break;
		}
		break;
		case SDL_SYSWMEVENT:
			if (onlineservice && onlineservice->isConnected())
			{
				onlineservice->updateUI(sdlev.syswm.msg->event);
			}
			break;
#endif // __QNXNTO__
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        // Default to EV_SPURIOUS
        ev.key = EV_SPURIOUS;
        if(sdlev.type == SDL_KEYDOWN)
        {
            ev.type = EV_KEY;
        }
        else
        {
            ev.type = EV_KEYRELEASE;
        }
        switch(sdlev.key.keysym.sym)
        {
        case SDLK_DOWN:         ev.key = JK_DOWN; break;
        case SDLK_UP:           ev.key = JK_UP; break;
        case SDLK_LEFT:         ev.key = JK_LEFT; break;
        case SDLK_RIGHT:        ev.key = JK_RIGHT; break;
        case SDLK_LCTRL:        ev.key = JK_CTRL_L; break;
        case SDLK_RCTRL:        ev.key = JK_CTRL_R; break;
        case SDLK_LALT:         ev.key = JK_ALT_L; break;
        case SDLK_RALT:         ev.key = JK_ALT_R; break;
        case SDLK_LSHIFT:       ev.key = JK_SHIFT_L; break;
        case SDLK_RSHIFT:       ev.key = JK_SHIFT_R; break;
        case SDLK_NUMLOCK:      ev.key = JK_NUM_LOCK; break;
        case SDLK_HOME:         ev.key = JK_HOME; break;
        case SDLK_END:          ev.key = JK_END; break;
        case SDLK_BACKSPACE:    ev.key = JK_BACKSPACE; break;
        case SDLK_TAB:          ev.key = JK_TAB; break;
        case SDLK_RETURN:       ev.key = JK_ENTER; break;
        case SDLK_SPACE:        ev.key = JK_SPACE; break;
        case SDLK_CAPSLOCK:     ev.key = JK_CAPS; break;
        case SDLK_ESCAPE:       ev.key = JK_ESC; break;
        case SDLK_F1:           ev.key = JK_F1; break;
        case SDLK_F2:           ev.key = JK_F2; break;
        case SDLK_F3:           ev.key = JK_F3; break;
        case SDLK_F4:           ev.key = JK_F4; break;
        case SDLK_F5:           ev.key = JK_F5; break;
        case SDLK_F6:           ev.key = JK_F6; break;
        case SDLK_F7:           ev.key = JK_F7; break;
        case SDLK_F8:           ev.key = JK_F8; break;
        case SDLK_F9:           ev.key = JK_F9; break;
        case SDLK_F10:          ev.key = JK_F10; break;
        case SDLK_INSERT:       ev.key = JK_INSERT; break;
        case SDLK_KP0:          ev.key = JK_INSERT; break;
        case SDLK_PAGEUP:       ev.key = JK_PAGEUP; break;
        case SDLK_PAGEDOWN:     ev.key = JK_PAGEDOWN; break;
        case SDLK_KP8:          ev.key = JK_UP; break;
        case SDLK_KP2:          ev.key = JK_DOWN; break;
        case SDLK_KP4:          ev.key = JK_LEFT; break;
        case SDLK_KP6:          ev.key = JK_RIGHT; break;
        case SDLK_F11:
            // Only handle key down
            if(ev.type == EV_KEY)
            {
                // Toggle fullscreen
                SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
            }
            ev.key = EV_SPURIOUS;
            break;
        case SDLK_F12:
            // Only handle key down
            if(ev.type == EV_KEY)
            {
                // Toggle grab mouse
                if(SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON)
                {
                    the_game->show_help("Grab Mouse: OFF\n");
                    SDL_WM_GrabInput(SDL_GRAB_OFF);
                }
                else
                {
                    the_game->show_help("Grab Mouse: ON\n");
                    SDL_WM_GrabInput(SDL_GRAB_ON);
                }
            }
            ev.key = EV_SPURIOUS;
            break;
        case SDLK_PRINT:    // print-screen key
            // Only handle key down
            if(ev.type == EV_KEY)
            {
                // Grab a screenshot
                SDL_SaveBMP(SDL_GetVideoSurface(), "screenshot.bmp");
                the_game->show_help("Screenshot saved to: screenshot.bmp.\n");
            }
            ev.key = EV_SPURIOUS;
            break;
        default:
            ev.key = (int)sdlev.key.keysym.sym;
            // Need to handle the case of shift being pressed
            // There has to be a better way
            if((sdlev.key.keysym.mod & KMOD_SHIFT) != 0)
            {
                if(sdlev.key.keysym.sym >= SDLK_a &&
                    sdlev.key.keysym.sym <= SDLK_z)
                {
                    ev.key -= 32;
                }
                else if(sdlev.key.keysym.sym >= SDLK_1 &&
                         sdlev.key.keysym.sym <= SDLK_5)
                {
                    ev.key -= 16;
                }
                else
                {
                    switch(sdlev.key.keysym.sym)
                    {
                    case SDLK_6:
                        ev.key = SDLK_CARET; break;
                    case SDLK_7:
                    case SDLK_9:
                    case SDLK_0:
                        ev.key -= 17; break;
                    case SDLK_8:
                        ev.key = SDLK_ASTERISK; break;
                    case SDLK_MINUS:
                        ev.key = SDLK_UNDERSCORE; break;
                    case SDLK_EQUALS:
                        ev.key = SDLK_PLUS; break;
                    case SDLK_COMMA:
                        ev.key = SDLK_LESS; break;
                    case SDLK_PERIOD:
                        ev.key = SDLK_GREATER; break;
                    case SDLK_SLASH:
                        ev.key = SDLK_QUESTION; break;
                    case SDLK_SEMICOLON:
                        ev.key = SDLK_COLON; break;
                    case SDLK_QUOTE:
                        ev.key = SDLK_QUOTEDBL; break;
                    default:
                        break;
                    }
                }
            }
            break;
        }
        break;
    }

    m_pos = ev.mouse_move;
    m_button = ev.mouse_button;
}


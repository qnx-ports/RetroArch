/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *  Copyright (C) 2013-2014 - CatalystG
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */
/*##########################################################################################*/
/* QNX Changes:
 * Authored by Jai Moraes 1/24/2025 M/D/Y
 */
/*##########################################################################################*/

#include <screen/screen.h>
#include "qnx_input.h"

/*### Globals ###*/
#include "../../qnx/qnx_common_ctx.h"

/*##############################################*/
/*                   Functions                  */
/*##############################################*/

/*### Initialization ###*/

/**
 * qnx_init_controller:
 * Initialized controller data to defaults (0).
 */
static void qnx_init_controller(qnx_input_t *qnx, qnx_input_device_t *controller){
    if (!qnx) return;
    controller->handle      = 0;
    controller->type        = 0;
    controller->analogCount = 0;
    controller->buttonCount = 0;
    controller->buttons     = 0;
    controller->analog0[0]  = 0;
    controller->analog0[1]  = 0;
    controller->analog0[2]  = 0;
    controller->analog1[0]  = 0;
    controller->analog1[1]  = 0;
    controller->analog1[2]  = 0;
    controller->port        = -1;
    controller->device      = -1;
    controller->index       = -1;

    memset(controller->id, 0, sizeof(controller->id));
}

static void *qnx_input_init(const char *joypad_driver){
    int i;
    qnx_input_t *qnx = (qnx_input_t*)calloc(1, sizeof(*qnx));

    if (!qnx) return NULL;

    input_keymaps_init_keyboard_lut(rarch_key_map_qnx);

    for (i = 0; i < MAX_TOUCH; ++i){
        qnx->pointer[i].contact_id = -1;
        qnx->touch_map[i] = -1;
    }

    for (i = 0; i < DEFAULT_MAX_PADS; ++i)
        qnx_init_controller(qnx, &qnx->devices[i]);


    /* Initialize Playbook keyboard. */
    strlcpy(qnx->devices[0].id, "0A5C-8502",
        sizeof(qnx->devices[0].id));
    qnx_input_autodetect_gamepad(qnx, &qnx->devices[0]);
    qnx->pads_connected = 1;

    return qnx;
}

/*### Input Polling ###*/

/**
 * qnx_input_poll:
 * Polls for input from various devices and updates stored data about them.
 */
static void qnx_input_poll( void *data){
    /*## Output ##*/
    qnx_input_t *qnx = (qnx_input_t*)data;
    //return; //TEMPORARY TO PREVENT POLLING LOOP AS INPUT IS BROKEN.
    
    if(!(*screen_ctx_qnx)){
        RARCH_LOG("[Screen/In]: Invalid Context\n");
        return;
    }

    /*## Request and process all screen events ##*/
#define DUMMY_VALUE_EVENT -17 //DUMMY VALUE, NOT PART OF SCREEN
    int val = DUMMY_VALUE_EVENT; //Stores type of event
    screen_event_t screen_ev;
    screen_create_event(&screen_ev);
    
    //INFINITELY LOOPING ATM
    while (true){
        /* Poll For new events */
        while (screen_get_event(*screen_ctx_qnx, screen_ev, 0)!=0){
            //0 is successful - thus any non zero goes here.
            RARCH_LOG("[Screen/In]: Failed to get event with. errno %d\n", errno);
            if(errno == 22) return;
            screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_TYPE, &val);
            if (val == SCREEN_EVENT_NONE) break;
        }

        screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_TYPE, &val);

        if(val == SCREEN_EVENT_NONE) break;
        if(val == DUMMY_VALUE_EVENT){
            RARCH_ERR("[Screen/In]: No event type passed in.\n");
            break;
        }

        /* Process based on result */
        switch (val){
            /* Pass to Processing Functions */
            case SCREEN_EVENT_KEYBOARD:
                qnx_process_keyboard_event(qnx, screen_ev, val);
            break;
            case SCREEN_EVENT_GAMEPAD:
                qnx_process_gamepad_event(qnx, screen_ev, val);
            break;
            case SCREEN_EVENT_JOYSTICK:
                qnx_process_joystick_event(qnx, screen_ev, val);
            break;
            case SCREEN_EVENT_MTOUCH_TOUCH:
            case SCREEN_EVENT_MTOUCH_MOVE:
            case SCREEN_EVENT_MTOUCH_RELEASE:
                qnx_process_touch_event(qnx, screen_ev, val);
            break;

            /* Device connect/disconnect */
            case SCREEN_EVENT_DEVICE:
                screen_device_t device;
                int attached, type, i;

                /* Get Info */
                screen_get_event_property_pv(screen_ev, SCREEN_PROPERTY_DEVICE, (void**)&device);
                screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_ATTACHED, &attached);
                if (attached)
                screen_get_device_property_iv(device, SCREEN_PROPERTY_TYPE, &type);

                /* Process if one of our supported devices */
                /* Check if attachment or detachment */
                if (attached && (type == SCREEN_EVENT_GAMEPAD || type == SCREEN_EVENT_JOYSTICK || type == SCREEN_EVENT_KEYBOARD)){
                    /* Search for open slot & attach device to that handle. */
                    for (i=0; i < DEFAULT_MAX_PADS; i++){
                        if (!qnx->devices[i].handle){
                            qnx->devices[i].handle = device;
                            qnx_handle_device(qnx, &qnx->devices[i]);
                        break;
                }}} /* if, for, if */
                else {  /* Search for matching slot and disconnect. */
                    for (i=0; i < DEFAULT_MAX_PADS; i++){
                        if (device == qnx->devices[i].handle){
                            RARCH_LOG("Device %s Disconnected.\n", qnx->devices[i].id);
                            qnx_init_controller(qnx, &qnx->devices[i]);
                        break;
                }}}/* if, for, else */
            break; /*case SCREEN_EVENT_DEVICE*/
            default:
            break;
        }
    }
}

/*### Processing Events ###*/

/**
 * qnx_process_keyboard_event:
 * Processes screen's keyboard input and adjusts the input state accordingly.
 */
static void qnx_process_keyboard_event(qnx_input_t *qnx, screen_event_t screen_ev, int type){
    /* Get key properties from screen event */
    int flags=0, cap=0, mod=0;
    screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_FLAGS, &flags);
    screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_KEY_CAP, &cap);
    screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_MODIFIERS, &mod);

    /* Calculate state */
    unsigned keycode = input_keymaps_translate_keysym_to_rk(cap);
    bool keydown     = flags & KEY_DOWN;
    bool keyrepeat   = flags & KEY_REPEAT;
    /* Fire keyboard event */
    if (!keyrepeat)
        input_keyboard_event(keydown, keycode, 0, mod, RETRO_DEVICE_KEYBOARD);

    /* Apply keyboard state */
    if (keydown && !keyrepeat)
    {
       BIT_SET(qnx->keyboard_state, cap);
    }
    else if (!keydown && !keyrepeat)
    {
       BIT_CLEAR(qnx->keyboard_state, cap);
    }
}

/**
 * qnx_process_gamepad_event:
 * Processes screen's gamepad connections and updates the state of the system accordingly.
 */
static void qnx_process_gamepad_event(qnx_input_t *qnx, screen_event_t screen_event, int type){
    /* Prepping Variables */
    int i;
    screen_device_t device;
    qnx_input_device_t* controller = NULL;
    (void) type;

    /* Locate the device which created this event */
    screen_get_event_property_pv(screen_event, SCREEN_PROPERTY_DEVICE, (void**)&device);
    for (i = 0; i < DEFAULT_MAX_PADS; ++i){
      if (device == qnx->devices[i].handle){
         controller = (qnx_input_device_t*)&qnx->devices[i];
         break;
    }} /*if, for*/
    if (!controller) return; 

    /* Store the new state */
    screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_BUTTONS, &controller->buttons);
    if (controller->analogCount > 0){
            screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_ANALOG0, controller->analog0);
            controller->analog0[0] *= 256;
            controller->analog0[1] *= 256;
        if (controller->analogCount == 2){
            screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_ANALOG1, controller->analog1);
            controller->analog1[0] *= 256;
            controller->analog1[1] *= 256;
    }} //if, if
}

/**
 * qnx_process_joystick_event:
 * Processes screen's joystick connections and updates the state of the system accordingly.
 */
static void qnx_process_joystick_event(qnx_input_t *qnx, screen_event_t screen_ev, int type){
    int displacement[2];
    screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_DISPLACEMENT, displacement);

    if (displacement != 0){
        qnx->trackpad_acc[0] += displacement[0];
        if (abs(qnx->trackpad_acc[0]) > TRACKPAD_THRESHOLD){
            if (qnx->trackpad_acc < 0){
                input_keyboard_event(true, RETROK_LEFT, 0, 0, RETRO_DEVICE_KEYBOARD);
                input_keyboard_event(false, RETROK_LEFT, 0, 0, RETRO_DEVICE_KEYBOARD);
            }
            else if (qnx->trackpad_acc > 0){
                input_keyboard_event(true, RETROK_RIGHT, 0, 0, RETRO_DEVICE_KEYBOARD);
                input_keyboard_event(false, RETROK_RIGHT, 0, 0, RETRO_DEVICE_KEYBOARD);
            }
            qnx->trackpad_acc[0] = 0;
        }

        qnx->trackpad_acc[1] += displacement[1];
        if (abs(qnx->trackpad_acc[1]) > TRACKPAD_THRESHOLD){
            if (qnx->trackpad_acc < 0){
                input_keyboard_event(true, RETROK_UP, 0, 0, RETRO_DEVICE_KEYBOARD);
                input_keyboard_event(false, RETROK_UP, 0, 0, RETRO_DEVICE_KEYBOARD);
            }
            else if (qnx->trackpad_acc > 0){
                input_keyboard_event(true, RETROK_DOWN, 0, 0, RETRO_DEVICE_KEYBOARD);
                input_keyboard_event(false, RETROK_DOWN, 0, 0, RETRO_DEVICE_KEYBOARD);
            }
            qnx->trackpad_acc[1] = 0;
        }
    }

    int buttons = 0;
    screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_BUTTONS, &buttons);
    input_keyboard_event(buttons != 0, RETROK_RETURN, 0, 0, RETRO_DEVICE_KEYBOARD);
}

/**
 * qnx_process_touch_event:
 * Processes screen's touch related events and properly updates the state of the system.
 */
static void qnx_process_touch_event(qnx_input_t *qnx, screen_event_t screen_ev, int type){
    /* Touch Info */
    int contact_id, pos[2];
    unsigned i, j;

    /* Event Properties */
    screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_TOUCH_ID, (int*)&contact_id);
    screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_SOURCE_POSITION, pos);

    /* Processing Switch */
    switch(type){
        /* New Touch Location */
        case SCREEN_EVENT_MTOUCH_TOUCH:
            /* Find a free touch struct. */
            for (i = 0; i < MAX_TOUCH; ++i){
                if (qnx->pointer[i].contact_id == -1){
                    struct video_viewport vp;
                    vp.x            = 0;
                    vp.y            = 0;
                    vp.width        = 0;
                    vp.height       = 0;
                    vp.full_width   = 0;
                    vp.full_height  = 0;
                    qnx->pointer[i].contact_id  = contact_id;

                    video_driver_translate_coord_viewport_wrap(
                        &vp,
                        pos[0], pos[1],
                        &qnx->pointer[i].x, &qnx->pointer[i].y,
                        &qnx->pointer[i].full_x, &qnx->pointer[i].full_y);

                    /* Add this pointer to the map to signal it's valid. */
                    qnx->pointer[i].map = qnx->pointer_count;
                    qnx->touch_map[qnx->pointer_count] = i;
                    qnx->pointer_count++;
                    break;
                }
            }
#ifdef TOUCH_LOGGING
            RARCH_LOG("New Touch: x:%d, y:%d, id:%d\n", pos[0], pos[1], contact_id);
            RARCH_LOG("Map: %d %d %d %d %d %d\n", qnx->touch_map[0], qnx->touch_map[1],
                qnx->touch_map[2], qnx->touch_map[3], qnx->touch_map[4],
                qnx->touch_map[5]);
#endif
        break;
        /* Removal of Touch Location */
        case SCREEN_EVENT_MTOUCH_RELEASE:
            for (i = 0; i < MAX_TOUCH; ++i){
                if (qnx->pointer[i].contact_id == contact_id){
                    /* Invalidate the finger. */
                    qnx->pointer[i].contact_id = -1;

                    /* Remove pointer from map and shift remaining valid ones to the front. */
                    qnx->touch_map[qnx->pointer[i].map] = -1;
                    for (j = qnx->pointer[i].map; j < qnx->pointer_count; ++j){
                        qnx->touch_map[j] = qnx->touch_map[j+1];
                        qnx->pointer[qnx->touch_map[j+1]].map = j;
                        qnx->touch_map[j+1] = -1;
                    }
                    qnx->pointer_count--;
                    break;
                }
            }
#ifdef TOUCH_LOGGING
         RARCH_LOG("Release: x:%d, y:%d, id:%d\n", pos[0], pos[1], contact_id);
         RARCH_LOG("Map: %d %d %d %d %d %d\n", qnx->touch_map[0], qnx->touch_map[1],
               qnx->touch_map[2], qnx->touch_map[3], qnx->touch_map[4],
               qnx->touch_map[5]);
#endif
         break;

        case SCREEN_EVENT_MTOUCH_MOVE:
            /* Find the finger we're tracking and update. */
            for (i = 0; i < qnx->pointer_count; ++i){
                if (qnx->pointer[i].contact_id == contact_id){
                    struct video_viewport vp;

                    vp.x                        = 0;
                    vp.y                        = 0;
                    vp.width                    = 0;
                    vp.height                   = 0;
                    vp.full_width               = 0;
                    vp.full_height              = 0;

#if 0
               gl_t *gl = (gl_t*)video_driver_get_ptr();

               /*During a move, we can go ~30 pixel into the
                * bezel which gives negative numbers or
                * numbers larger than the screen resolution.
                *
                * Normalize. */
               if (pos[0] < 0)
                  pos[0] = 0;
               if (pos[0] > gl->full_x)
                  pos[0] = gl->full_x;

               if (pos[1] < 0)
                  pos[1] = 0;
               if (pos[1] > gl->full_y)
                  pos[1] = gl->full_y;
#endif

                    video_driver_translate_coord_viewport_wrap(&vp,
                        pos[0], pos[1],
                        &qnx->pointer[i].x, &qnx->pointer[i].y,
                        &qnx->pointer[i].full_x, &qnx->pointer[i].full_y);
#if 0
               RARCH_LOG("Move: x:%d, y:%d, id:%d\n", pos[0], pos[1],
                     contact_id);
#endif
                    break;
                }
            }
        break;
    }
}

/**
 * qnx_handle_device:
 * Handles new device connections and properly registers them.
 */
static void qnx_handle_device(qnx_input_t *qnx, qnx_input_device_t* controller){
    if (!qnx) return;

    /* Grab info from screen and update */
    /*info*/
    screen_get_device_property_iv(controller->handle, SCREEN_PROPERTY_TYPE, &controller->type);
    /*context*/
    screen_get_device_property_cv(controller->handle, SCREEN_PROPERTY_ID_STRING, sizeof(controller->id), controller->id);
    screen_get_device_property_cv(controller->handle, SCREEN_PROPERTY_VENDOR, sizeof(controller->vid), controller->vid);
    screen_get_device_property_cv(controller->handle, SCREEN_PROPERTY_PRODUCT, sizeof(controller->pid), controller->pid);

    /* Special Gamepad Processing */
    if (controller->type == SCREEN_EVENT_GAMEPAD){
        screen_get_device_property_iv(controller->handle, SCREEN_PROPERTY_BUTTON_COUNT, &controller->buttonCount);
        /* Check for the existence of analog sticks. */
        if (!screen_get_device_property_iv(controller->handle, SCREEN_PROPERTY_ANALOG0, controller->analog0))
            ++controller->analogCount;
        if (!screen_get_device_property_iv(controller->handle, SCREEN_PROPERTY_ANALOG1, controller->analog1))
            ++controller->analogCount;
    }

    /*Screen service will map supported controllers, might need to adjust. */
    qnx_input_autodetect_gamepad(qnx, controller);

#ifdef DEBUG
    if (controller->type == SCREEN_EVENT_GAMEPAD)
        RARCH_LOG("Gamepad Device Connected:\n");
    else if (controller->type == SCREEN_EVENT_JOYSTICK)
        RARCH_LOG("Joystick Device Connected:\n");
    else if (controller->type == SCREEN_EVENT_KEYBOARD)
        RARCH_LOG("Keyboard Device Connected:\n");

    RARCH_LOG("\tID: %s\n", controller->id);
    RARCH_LOG("\tVendor  ID: %s\n", controller->vid);
    RARCH_LOG("\tProduct ID: %s\n", controller->pid);
    RARCH_LOG("\tButton Count: %d\n", controller->buttonCount);
    RARCH_LOG("\tAnalog Count: %d\n", controller->analogCount);
#endif
}



/*### Detection & Discovery ###*/

/**
 * qnx_input_autodetect_gamepad:
 * automatically detect gamepad info and configure them.
 */
static void qnx_input_autodetect_gamepad(qnx_input_t *qnx, qnx_input_device_t *controller){
    if (!qnx) return;

    char name_buf[256];
    name_buf[0] = '\0';

    if (controller && controller->type == SCREEN_EVENT_GAMEPAD){
        if (strstr(controller->id, "0-054C-05C4-1.0"))
            strlcpy(name_buf, "DS4 Controller", sizeof(name_buf));
        else
            strlcpy(name_buf, "QNX Gamepad", sizeof(name_buf));
    }

    if (!string_is_empty(name_buf)){
        controller->port = qnx->pads_connected;
        input_autoconfigure_connect(name_buf, NULL, "qnx", controller->port, *controller->vid, *controller->pid);
        qnx->pads_connected++;
    }
}

/**
 * qnx_discover_controllers:
 * Finds connected gamepads from screen.
 */
static int qnx_discover_controllers(qnx_input_t *qnx){
    /* Get array of connected devices */
    int deviceCount = 0, ret;
    unsigned i;
    ret = screen_get_context_property_iv(*screen_ctx_qnx, SCREEN_PROPERTY_DEVICE_COUNT, &deviceCount);

    /* Failed Query Error */
    if (ret < 0){ 
        RARCH_ERR("Error querying SCREEN_PROPERTY_DEVICE_COUNT: [%d] %s\n", errno, strerror(errno));
        return false;
    }

    screen_device_t* devices_found = (screen_device_t*) calloc(deviceCount, sizeof(screen_device_t));
    
    /* Allocation Error*/
    if (!devices_found){
        RARCH_ERR("Error allocating devices_found, deviceCount=%d\n", deviceCount);
        return false;
    }

    ret = screen_get_context_property_pv(*screen_ctx_qnx, SCREEN_PROPERTY_DEVICES, (void**)devices_found);

    /* Failed Query Error */
    if (ret < 0){ 
        RARCH_ERR("Error querying SCREEN_PROPERTY_DEVICES: [%d] %s\n", errno, strerror(errno));
        return false;
    }

    /* Scan the list for gamepad and joystick devices. */
    for (i = 0; i < qnx->pads_connected; ++i)
        qnx_init_controller(qnx, &qnx->devices[i]);
    qnx->pads_connected = 0;

    /* Check all devices */
    for (i = 0; i < deviceCount; i++){
        /* Query type */
        int type;
        screen_get_device_property_iv(devices_found[i], SCREEN_PROPERTY_TYPE, &type);

        /* Make sure type is supported */
        if (type == SCREEN_EVENT_GAMEPAD  || type == SCREEN_EVENT_JOYSTICK || type == SCREEN_EVENT_KEYBOARD){
            qnx->devices[qnx->pads_connected].handle = devices_found[i];
            qnx->devices[qnx->pads_connected].index = qnx->pads_connected;
            qnx_handle_device(qnx, &qnx->devices[qnx->pads_connected]);
            if (qnx->pads_connected == DEFAULT_MAX_PADS) break;
        }
    }

    /* Cleanup */
    free(devices_found);
    return true;
}

/*### State Processing ###*/

/**
 * qnx_keyboard_pressed:
 * Return whether a specific key is pressed.
 */
static bool qnx_keyboard_pressed(qnx_input_t *qnx, unsigned id){
    unsigned bit = rarch_keysym_lut[(enum retro_key)id];
    return id < RETROK_LAST && BIT_GET(qnx->keyboard_state, bit);
}

/**
 * qnx_pointer_input_state:
 * Returns mouse pointer info.
 */
static int16_t qnx_pointer_input_state(qnx_input_t *qnx, unsigned idx, unsigned id, bool screen){
    int16_t x;
    int16_t y;

    if (screen){
        x = qnx->pointer[idx].full_x;
        y = qnx->pointer[idx].full_y;
    } else {
        x = qnx->pointer[idx].x;
        y = qnx->pointer[idx].y;
    }

    switch (id){
        case RETRO_DEVICE_ID_POINTER_X:
            return x;
        case RETRO_DEVICE_ID_POINTER_Y:
            return y;
        case RETRO_DEVICE_ID_POINTER_PRESSED:
            return (idx < qnx->pointer_count) && (x != -0x8000) && (y != -0x8000);
    }
}

/**
 * qnx_input_state:
 * returns infor about a specific input's state.
 */
static int16_t qnx_input_state(
      void *data,
      const input_device_driver_t *joypad,
      const input_device_driver_t *sec_joypad,
      rarch_joypad_info_t *joypad_info,
      const retro_keybind_set *binds,
      bool keyboard_mapping_blocked,
      unsigned port,
      unsigned device,
      unsigned idx,
      unsigned id){
    qnx_input_t *qnx = (qnx_input_t*)data;

    switch (device){
        case RETRO_DEVICE_JOYPAD:
            if (id == RETRO_DEVICE_ID_JOYPAD_MASK){
                unsigned i;
                int16_t ret = 0;
                if (!keyboard_mapping_blocked){
                    for (i = 0; i < RARCH_FIRST_CUSTOM_BIND; i++){
                        if (binds[port][i].valid){
                            if (qnx_keyboard_pressed(qnx, binds[port][i].key))
                            ret |= (1 << i);
                        }
                    }
                }
                return ret;
            }

            if (id < RARCH_BIND_LIST_END){
                if (binds[port][id].valid){
                    if (
                            ((id == RARCH_GAME_FOCUS_TOGGLE) || 
                            !keyboard_mapping_blocked) && 
                            qnx_keyboard_pressed(qnx, binds[port][id].key)
                        )
                        return 1;
                }
            }
        break;
        case RETRO_DEVICE_ANALOG:
        break;
        case RETRO_DEVICE_KEYBOARD:
        return qnx_keyboard_pressed(qnx, id);
        case RETRO_DEVICE_POINTER:
        case RARCH_DEVICE_POINTER_SCREEN:
        return qnx_pointer_input_state(qnx, idx, id, device == RARCH_DEVICE_POINTER_SCREEN);
        default:
        break;
   }
   return 0;
}

/*### Information & Management ###*/

/**
 * qnx_free_input:
 * Provides a safer 'free()' call by checking if pointer is valid.
 * Note that free(NULL) causes a segmentation fault on QNX.
 */
static void qnx_input_free_input(void *data){
   if (data) free(data);
}

/**
 * qnx_input_get_capabilities:
 * Helper that displays which devices are available.
 */
static uint64_t qnx_input_get_capabilities(void *data){
    /* Very clever implementation!!! */
    return
          (1 << RETRO_DEVICE_JOYPAD)
        | (1 << RETRO_DEVICE_POINTER)
        | (1 << RETRO_DEVICE_ANALOG)
        | (1 << RETRO_DEVICE_KEYBOARD);
}

/*##############################################*/
/*                    Driver                    */
/*##############################################*/

input_driver_t input_qnx = {
   qnx_input_init,
   qnx_input_poll,
   qnx_input_state,
   qnx_input_free_input,
   NULL,
   NULL,
   qnx_input_get_capabilities,
   "qnx_input",
   NULL,
   NULL,
   NULL
};
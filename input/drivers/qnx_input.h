/*##########################################################################################*/
/* QNX Changes:
 * Authored by Jai Moraes 1/24/2025 M/D/Y
 */
/*##########################################################################################*/

/*### Standard Headers ###*/
#include <errno.h>

/*### Type Headers ###*/
#include <boolean.h>
#include <string/stdstring.h>

/*### Screen and Input Headers ###*/
#include <screen/screen.h>
#include <sys/keycodes.h>

/*### Configuration ###*/
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "../../config.def.h"

/*### RetroArch ###*/
#include "../../retroarch.h"
#include "../../tasks/tasks_internal.h"
#include "../../command.h"
#include "../input_keymaps.h"
#include "../../verbosity.h"

/*### Structs ###*/
typedef struct {
    screen_device_t handle;
    int type;
    int analogCount;
    int buttonCount;
    int device;
    int port;
    int index;
    /* Current state. */
    int buttons;
    int analog0[3];
    int analog1[3];
    char id[64];
    char vid[64];
    char pid[64];
} qnx_input_device_t;

struct input_pointer {
   int contact_id;
   int map;
   int16_t x, y;
   int16_t full_x, full_y;
};

struct input_mouse {
    int16_t x, y, x_del, y_del;
    bool lmb, mmb, rmb;
};

#define QNX_MAX_KEYS (65535 + 7) / 8
#define TRACKPAD_CPI 500
#define TRACKPAD_THRESHOLD TRACKPAD_CPI / 2
#define MAX_TOUCH 4

#define QNX_LMB_MASK 0b001;
#define QNX_MMB_MASK 0b010;
#define QNX_RMB_MASK 0b100;

typedef struct qnx_input {
    uint64_t pad_state[DEFAULT_MAX_PADS];
    uint8_t keyboard_state[QNX_MAX_KEYS];

    /*The first pointer_count indices of touch_map will be a valid,
    * active index in pointer array.
    * Saves us from searching through pointer array when polling state.
    */
   struct input_pointer pointer[MAX_TOUCH]; /* int alignment */
   int touch_map[MAX_TOUCH];
   int trackpad_acc[2];
   unsigned pointer_count;
   unsigned pads_connected;

   struct input_mouse mouse;

   qnx_input_device_t devices[DEFAULT_MAX_PADS];

} qnx_input_t;

/*### Function Declarations ###*/
static void qnx_init_controller(qnx_input_t *qnx, qnx_input_device_t *controller);
static void qnx_input_poll( void *data);
static void *qnx_input_init(const char *joypad_driver);
static void qnx_process_keyboard_event(qnx_input_t *qnx, screen_event_t screen_ev, int type);
static void qnx_process_joystick_event(qnx_input_t *qnx, screen_event_t screen_ev, int type);
static void qnx_process_gamepad_event(qnx_input_t *qnx, screen_event_t screen_ev, int type);
static void qnx_process_touch_event(qnx_input_t *qnx, screen_event_t screen_ev, int type);
static void qnx_process_mouse_event(qnx_input_t *qnx, screen_event_t screen_ev, int type);
static void qnx_handle_device(qnx_input_t *qnx, qnx_input_device_t* controller);
static void qnx_input_autodetect_gamepad(qnx_input_t *qnx, qnx_input_device_t *controller);
static int qnx_discover_controllers(qnx_input_t *qnx);
static bool qnx_keyboard_pressed(qnx_input_t *qnx, unsigned id);
static int16_t qnx_pointer_input_state(qnx_input_t *qnx, unsigned idx, unsigned id, bool screen);
static int16_t qnx_mouse_input_state(qnx_input_t *qnx, unsigned id);
static int16_t qnx_input_state(void *data, const input_device_driver_t *joypad, const input_device_driver_t *sec_joypad, rarch_joypad_info_t *joypad_info, const retro_keybind_set *binds, bool keyboard_mapping_blocked, unsigned port, unsigned device, unsigned idx, unsigned id);
static void qnx_input_free_input(void *data);
void qnx_grab_mouse(void *data, bool state);
static uint64_t qnx_input_get_capabilities(void *data);
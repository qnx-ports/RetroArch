/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
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
/* QNX Changes:
 * Authored by Jai Moraes 1/31/2025 M/D/Y
 */
/*##########################################################################################*/

/*### Standard Headers ###*/
#include <stdint.h>
#include <stdbool.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

/*### Platform-Specifics ###*/
#include <screen/screen.h>
#include <sys/platform.h>
#include <errno.h>

/*### Retro Arch ###*/
#include "../../config.h"
#include "../../configuration.h"
#include "../../verbosity.h"
#include <retro_timers.h>

#include "../include/vulkan/vulkan.h"
#include "../common/vksym.h"
#include <libretro_vulkan.h>

/*### Vulkan ###*/
#include "../common/vulkan_common.h"

/*##############################################*/
/*                  Structures                  */
/*##############################################*/
typedef struct {
    gfx_ctx_vulkan_data_t vk;
    screen_context_t ctx;
    screen_window_t win;
    unsigned width;
    unsigned height;
    unsigned swap_interval;
} qnx_ctx_data_vk_t; 

/*##############################################*/
/*                  Functions                   */
/*##############################################*/

/**
 * qnx_gfx_ctx_vk_destroy
 * Destroys the gfx context
 */
static void qnx_gfx_ctx_vk_destroy(void *data){
    RARCH_LOG("qnx_gfx_ctx_vk_destroy\n"); //QNX DEBUG
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)data;

    if(!qnx) return;
    vulkan_context_destroy(&qnx->vk, qnx->ctx);

    if (qnx->vk.context.queue_lock)
        slock_free(qnx->vk.context.queue_lock);
    screen_destroy_window(qnx->win);
    screen_destroy_context(qnx->ctx);
    free(qnx->win);
    free(qnx->ctx);
    free(data);

} /*qnx_gfx_ctx_vk_destroy*/

static void get_display_info_qnx(qnx_ctx_data_vk_t* qnx){  
    char * buf = malloc(64*sizeof(char));
    int buf_i = -1;
    if(!buf){
        RARCH_LOG("[Screen/VK]: Failed to allocate memory for display qnx info.\n");
        return;
    }
    int len = 63;

    RARCH_LOG("=============================\n");
    RARCH_LOG("QNX Display, Ctx, Window Info\n");
    RARCH_LOG("=============================\n");

    if(screen_get_context_property_cv(qnx->ctx, SCREEN_PROPERTY_ID_STRING, len, buf))
        RARCH_LOG("[Screen/VK]: Failed to query context for id_str with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Context ID String: %s\n", buf);

    if(screen_get_context_property_cv(qnx->ctx, SCREEN_PROPERTY_PRODUCT, len, buf))
        RARCH_LOG("[Screen/VK]: Failed to query context for pid with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Context Product ID String: %s\n", buf);

    if(screen_get_context_property_cv(qnx->ctx, SCREEN_PROPERTY_ID, len, buf))
        RARCH_LOG("[Screen/VK]: Failed to query context for vid with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Context ID (Generated): %s\n", buf);

    if(screen_get_context_property_iv(qnx->ctx, SCREEN_PROPERTY_STATUS, &buf_i))
        RARCH_LOG("[Screen/VK]: Failed to query context for status with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Context Status: %d [0=DESTRYD., 1=CREATED]\n", buf_i);

    if(screen_get_context_property_iv(qnx->ctx, SCREEN_PROPERTY_TYPE, &buf_i))
        RARCH_LOG("[Screen/VK]: Failed to query context for type with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Context Type: %d [0=APP, 1=WIN_MGR, 2=INP_PVDR, 4=PWR_MGR, 8=DISP_MGR, 16=INP_MGR, 32=BUF_PVR]\n", buf_i);

    if(screen_get_context_property_iv(qnx->ctx, SCREEN_PROPERTY_WINDOW_COUNT, &buf_i))
        RARCH_LOG("[Screen/VK]: Failed to query context for Window count with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Context Window Count: %d\n", buf_i);

    if(screen_get_context_property_iv(qnx->ctx, SCREEN_PROPERTY_IDLE_STATE, &buf_i))
        RARCH_LOG("[Screen/VK]: Failed to query context for idle state with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Context Idle: %s\n", buf_i?"Idle":"Not Idle");

    RARCH_LOG("-----------------------------\n");
    
    if(screen_get_window_property_cv(qnx->win, SCREEN_PROPERTY_ID, len, buf))
        RARCH_LOG("[Screen/VK]: Failed to query window for id with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Window ID (generated): %s\n", buf);

    if(screen_get_window_property_cv(qnx->win, SCREEN_PROPERTY_ID_STRING, len, buf))
        RARCH_LOG("[Screen/VK]: Failed to query window for id_str with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Window ID String: %s\n", buf);

    if(screen_get_window_property_cv(qnx->win, SCREEN_PROPERTY_CLASS, len, buf))
        RARCH_LOG("[Screen/VK]: Failed to query window for class with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Window Class: %s\n", buf);

    if(screen_get_window_property_iv(qnx->win, SCREEN_PROPERTY_TYPE, &buf_i))
        RARCH_LOG("[Screen/VK]: Failed to query window for type with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Window Type: %d [0=app, 1=child, 2=embed, 4=root]\n", buf_i);

    if(screen_get_window_property_iv(qnx->win, SCREEN_PROPERTY_RENDER_BUFFER_COUNT, &buf_i))
        RARCH_LOG("[Screen/VK]: Failed to query window for render buffer count with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Window Render Buffer Count: %d\n", buf_i);
    
    if(screen_get_window_property_iv(qnx->win, SCREEN_PROPERTY_STATUS, &buf_i))
        RARCH_LOG("[Screen/VK]: Failed to query window for status with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Window Status: %d [<0=ERROR, 0=DESTRYD., 1=CREATED, 2=REALIZ., 3=INVIS., 4/5=OBSC. , 9=VISIB., 10=FULL VIS., 11=FULLSCR.]\n", buf_i);

    if(screen_get_window_property_iv(qnx->win, SCREEN_PROPERTY_VISIBLE, &buf_i))
        RARCH_LOG("[Screen/VK]: Failed to query window for visibility with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Window Visibility: %d\n", buf_i);

    if(screen_get_window_property_cv(qnx->win, SCREEN_PROPERTY_GROUP, len, buf))
        RARCH_LOG("[Screen/VK]: Failed to query window for group with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Window Group: %s\n", buf);

    if(screen_get_window_property_cv(qnx->win, SCREEN_PROPERTY_PARENT, len, buf))
        RARCH_LOG("[Screen/VK]: Failed to query window for parent with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Window Parent: %s\n", buf);

    if(screen_get_window_property_iv(qnx->win, SCREEN_PROPERTY_FORMAT, &buf_i))
        RARCH_LOG("[Screen/VK]: Failed to query window for type with errno %d.\n", errno);
    else RARCH_LOG("[Screen/VK]: Window Format: %d [See Docs]\n", buf_i);

    RARCH_LOG("-----------------------------\n");

    int num_disps = 0;
    if(screen_get_context_property_iv(qnx->ctx, SCREEN_PROPERTY_DISPLAY_COUNT, &num_disps)){
        RARCH_LOG("[Screen/VK]: Failed to query context for display pointers with errno %d.\n", errno);
    }else{
        RARCH_LOG("[Screen/VK]: %d displays detected in context.\n");
        if(num_disps > 0){
            screen_display_t* disps = malloc(sizeof(screen_display_t)*num_disps);
            if(screen_get_context_property_pv(qnx->ctx, SCREEN_PROPERTY_DISPLAYS, disps)){
                RARCH_LOG("[Screen/VK]: %d display(s) detected in context.\n");
            }else{
                for(int i = 0; i < num_disps; i++){
                    RARCH_LOG("[Screen/VK]: Info for Display %d%s\n", i, i==0?" [default]":"");
                    if(!disps[i]){
                        RARCH_LOG("[Screen/VK]: Display pointer %d is invalid.\n", i);
                    }else{
                        if(screen_get_display_property_cv(disps[i], SCREEN_PROPERTY_ID_STRING, len, buf))
                            RARCH_LOG("[Screen/VK]: Failed to query display for id_str with errno %d.\n", errno);
                        else RARCH_LOG("[Screen/VK]: Display ID String: %s\n", buf);

                        if(screen_get_display_property_cv(disps[i], SCREEN_PROPERTY_PRODUCT, len, buf))
                            RARCH_LOG("[Screen/VK]: Failed to query display for pid with errno %d.\n", errno);
                        else RARCH_LOG("[Screen/VK]: Display Product ID String: %s\n", buf);

                        if(screen_get_display_property_cv(disps[i], SCREEN_PROPERTY_VENDOR, len, buf))
                            RARCH_LOG("[Screen/VK]: Failed to query display for vid with errno %d.\n", errno);
                        else RARCH_LOG("[Screen/VK]: Display Vendor ID String: %s\n", buf);

                        int nforms = 0;
                        if(screen_get_display_property_iv(disps[i], SCREEN_PROPERTY_FORMAT_COUNT, &nforms))
                            RARCH_LOG("[Screen/VK]: Failed to query display for format count with errno %d.\n", errno);
                        else if(nforms > 0){
                            RARCH_LOG("[Screen/VK]: Display returned %d valid formats.\n", nforms);
                            int* forms = malloc(sizeof(int)*nforms);
                            if(forms){
                                if(screen_get_display_property_iv(disps[i], SCREEN_PROPERTY_FORMATS,forms))
                                    RARCH_LOG("[Screen/VK]: Failed to query display for valid formats with errno %d.\n", errno);
                                else{
                                    for(int i = 0; i < nforms; i++)
                                        RARCH_LOG("[Screen/VK]: Supports format type %d.\n", forms[i]);
                                }
                                free(forms);
                            }
                        }else
                            RARCH_LOG("[Screen/VK]: Display returned no valid formats.\n");
                    }
                }
            }
        }
    }
    RARCH_LOG("=============================\n");
    free(buf);
}

/**
 * qnx_gfx_ctx_vk_init
 * Initializes the gfx context
 */
static void *qnx_gfx_ctx_vk_init(void *video_driver) {
    RARCH_LOG("qnx_gfx_ctx_vk_init\n"); //QNX DEBUG

    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)calloc(1, sizeof(*qnx));
    if(!qnx){
        RARCH_ERR("[Screen/VK]: Fatal: Could not create a data pointer.");
        return false;
    }

    RARCH_LOG("[Screen/VK]: Initializing vulkan context...\n");
    if(!vulkan_context_init(&qnx->vk, VULKAN_WSI_QNX)){
        RARCH_ERR("[Screen/VK]: Failed to initialize vulkan context. Destroying Window and Context.\n");
        free(qnx);
        return NULL;
    }

    screen_context_t* screen_ctx = malloc(sizeof(screen_context_t));
    screen_window_t* screen_win = malloc(sizeof(screen_window_t));
    if(screen_create_context(screen_ctx, SCREEN_APPLICATION_CONTEXT)){
        RARCH_ERR("[Screen/VK]: Fatal: Context init failed with errno %d.\n", errno);
        free(qnx);
        return false;
    }
    
    if(screen_create_window_type(screen_win, *screen_ctx, SCREEN_APPLICATION_WINDOW)){
        RARCH_ERR("[Screen/VK]: Fatal: Window init failed with errno %d.\n", errno);
        screen_destroy_context(*screen_ctx);
        free(screen_ctx);
        free(qnx);
        return false;
    }

    int usage = SCREEN_USAGE_VULKAN;
    if(screen_set_window_property_iv(*screen_win, SCREEN_PROPERTY_USAGE, &usage))
        RARCH_WARN("[Screen/VK]: Could not set window type to SCREEN_USAGE VULKAN, errno %d.\n", errno);

    RARCH_LOG("[Screen/VK]: Context, Window initialized.\n");

    /*
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)calloc(1, sizeof(*qnx));
    if(!qnx){
        RARCH_ERR("[Screen/VK]: Fatal: Could not create a data pointer.");
        screen_destroy_window(*screen_win);
        screen_destroy_context(*screen_ctx);
        free(screen_win);
        free(screen_ctx);
        return false;
    }

    RARCH_LOG("[Screen/VK]: Initializing vulkan context...\n");
    if(!vulkan_context_init(&qnx->vk, VULKAN_WSI_QNX)){
        RARCH_ERR("[Screen/VK]: Failed to initialize vulkan context. Destroying Window and Context.\n");
        qnx_gfx_ctx_vk_destroy(qnx);
        free(screen_win);
        free(screen_ctx);
        return NULL;
    }
    */

    qnx->ctx  = *screen_ctx;
    qnx->win  = *screen_win;

    int size[2] = {0,0};
    RARCH_LOG("[Screen/VK]: Getting screen size...\n");
    if(screen_get_window_property_iv(qnx->win, SCREEN_PROPERTY_SIZE, &size))
        RARCH_ERR("[Screen/VK]: Failed to get screen size with errno %d.\n", errno);
    else
        RARCH_LOG("[Screen/VK]: Screen Size: %d x %d \n", size[0], size[1]);

    get_display_info_qnx(qnx);
    
    if(min(size[0], size[1]) < 0 || min(size[0], size[1]) > 100000){
        RARCH_LOG("[Screen/VK]: Window size invalid! Setting to 1920x1080\n");
        size[0] = 1920;
        size[1] = 1080;
        if(screen_set_window_property_iv(qnx->win, SCREEN_PROPERTY_SIZE, &size))
            RARCH_LOG("[Screen/VK]: Failed to set window size with errno %d.\n", errno);
    }

    qnx->width  = size[0];
    qnx->height = size[1];

    free(screen_win);
    free(screen_ctx);

    return qnx;
} /*qnx_gfx_ctx_vk_init*/

static enum gfx_ctx_api qnx_gfx_ctx_vk_get_api(void *data){
   RARCH_LOG("qnx_gfx_ctx_vk_get_api\n"); //QNX DEBUG
   return GFX_CTX_VULKAN_API;
}

static bool qnx_gfx_ctx_vk_bind_api(void *data, enum gfx_ctx_api api, unsigned major, unsigned minor){
   RARCH_LOG("qnx_gfx_ctx_vk_bind_api\n"); //QNX DEBUG
   return (api == GFX_CTX_VULKAN_API);
}

/**
 * TODO Comments
 */
static bool qnx_gfx_ctx_vk_set_video_mode(void *data, unsigned width, unsigned height, bool fullscreen){
    RARCH_LOG("qnx_gfx_ctx_vk_set_video_mode\n"); //QNX DEBUG
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)data;
    if(!qnx){
        RARCH_ERR("[Screen/VK]: Null Data Pointer.");
        return false;
    }

    int size[2] = {width,height}, /*size_d[2],*/ swap_interval = 0/*, ndisplays*/; 
    if(screen_set_window_property_iv(qnx->win, SCREEN_PROPERTY_SIZE, &size))
        RARCH_ERR("[Screen/VK]: Failed to set screen sizes from window with errno %d.", errno);
    if(screen_get_window_property_iv(qnx->win, SCREEN_PROPERTY_SWAP_INTERVAL, &swap_interval))
        RARCH_ERR("[Screen/VK]: Failed to get swap interval from window with errno %d.", errno);

    if(!vulkan_surface_create(&qnx->vk, VULKAN_WSI_QNX, &qnx->ctx, &qnx->win, size[0], size[1], swap_interval)){ 
        RARCH_ERR("[Screen/VK]: Failed to create surface.\n");
        return false;
    }

    int form = SCREEN_FORMAT_RGB565;
    if(screen_set_window_property_iv(qnx->win, SCREEN_PROPERTY_FORMAT,&form))
        RARCH_ERR("[Screen/VK]: Failed to set format for window with errno %d.", errno);

    get_display_info_qnx(qnx);
    return true;
}

static void qnx_gfx_ctx_vk_set_swap_interval(void *data, int swap_interval){
    RARCH_LOG("qnx_gfx_ctx_vk_set_swap_interval\n"); //QNX DEBUG
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)data;
    if(!qnx){
        RARCH_ERR("[Screen/VK]: Invalid Data passed to qnx_gfx_ctx_vk_set_swap_interval.\n");
        return;
    }

    if(screen_set_window_property_iv(qnx->win, SCREEN_PROPERTY_SWAP_INTERVAL, &swap_interval))
        RARCH_ERR("[Screen/VK]: Setting swap interval of window failed with errno %d.\n", errno);
}

static void qnx_gfx_ctx_vk_get_video_size(void *data, unsigned *width, unsigned *height){
    RARCH_LOG("qnx_gfx_ctx_vk_get_video_size\n"); //QNX DEBUG
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)data;
    if(!qnx){
        RARCH_ERR("[Screen/VK]: Invalid data passed to qnx_gfx_ctx_vk_get_video_size\n");
        return;
    }
    int size[2] = {0,0};
    if(screen_get_window_property_iv(qnx->win, SCREEN_PROPERTY_SIZE, &size)){
        RARCH_ERR("[Screen/VK]: Failed to get video size. Setting default values...\n");
        //Lilliput Defaults
        size[0] = 1920;
        size[1] = 1080;
    }
    *width = (unsigned) size[0];
    *height = (unsigned) size[1];
}

static bool qnx_gfx_ctx_vk_has_focus(void *data){
    RARCH_LOG("qnx_gfx_ctx_vk_has_focus\n"); //QNX DEBUG
    uint_t focused = 0;
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)data;

    screen_get_window_property_iv(qnx->win, SCREEN_PROPERTY_FOCUS, &focused);
    bool toret;
    toret = (focused != 0)? true: false;

    return toret; /*I know this is unneeded, but bools can be strange*/
}

static bool qnx_gfx_ctx_vk_suppress_screensaver(void *data, bool enable) { 
    RARCH_LOG("qnx_gfx_ctx_vk_suppress_screensaver\n"); //QNX DEBUG
    return false; 
}

static void qnx_gfx_ctx_vk_check_window(void *data, bool *quit, bool *resize, unsigned *width, unsigned *height){
    RARCH_LOG("qnx_gfx_ctx_vk_check_window\n"); //QNX DEBUG
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)data;
    uint_t new_width, new_height, size[2];
    *quit=false; //TODO: CHECK VIA EVENTS

    screen_get_window_property_iv(qnx->ctx, SCREEN_PROPERTY_SIZE, &size);

    if(new_width!=qnx->width || new_height!=qnx->height){
        *width=new_width;
        *height=new_height;
        *resize=true;
        qnx->width=new_width;
        qnx->height=new_height;
    }
}

static int dpi_get_density(qnx_ctx_data_vk_t *qnx){
    int screen_dpi[2];

    if(!qnx) return -1;

    screen_device_t disp;
    if(screen_get_window_property_pv(qnx->win, SCREEN_PROPERTY_DISPLAY, &disp)) goto error;
    if(screen_get_display_property_iv(disp, SCREEN_PROPERTY_DPI, &screen_dpi)) goto error;

    return min(screen_dpi[0], screen_dpi[1]);

    error:
    RARCH_ERR("screen failed to get DPI\n");
    return NULL;
}


static bool qnx_gfx_ctx_vk_get_metrics(void *data, enum display_metric_types type, float *value){
    RARCH_LOG("qnx_gfx_ctx_vk_get_metrics\n"); //QNX DEBUG
    static int dpi = -1;
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)data;

    switch(type){
        case DISPLAY_METRIC_MM_WIDTH:
        case DISPLAY_METRIC_MM_HEIGHT:
            return false;
        case DISPLAY_METRIC_DPI:
            if(dpi == -1){
                dpi = dpi_get_density(qnx);
            }
            if(dpi <= 0){
                //TODO: Needs to be investigated as a fallback, perhaps the dpi of the lilliput
                dpi = 345;
                *value = (float)dpi;
                return true;
            }
            *value = (float)dpi;
            break;
            case DISPLAY_METRIC_NONE:
            default:
                *value = 0;
                return false;
    }
    return true;
}

static void qnx_gfx_ctx_vk_swap_buffers(void *data){
    RARCH_LOG("qnx_gfx_ctx_vk_swap_buffers\n"); //QNX DEBUG
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)data;

    if(qnx->vk.context.flags & VK_CTX_FLAG_HAS_ACQUIRED_SWAPCHAIN){
        qnx->vk.context.flags &= ~VK_CTX_FLAG_HAS_ACQUIRED_SWAPCHAIN;
        if(qnx->vk.swapchain == VK_NULL_HANDLE){
            retro_sleep(10);
        }else
            vulkan_present(&(qnx->vk), qnx->vk.context.current_swapchain_index);
    }
    vulkan_acquire_next_image(&(qnx->vk));
}

static void qnx_gfx_ctx_vk_input_driver (void *data, const char *joypad_name, input_driver_t **input, void **input_data){
    RARCH_LOG("qnx_gfx_ctx_vk_input_driver\n"); //QNX DEBUG
    void *qnxinput  = input_driver_init_wrap(&input_qnx, joypad_name);
    *input           = qnxinput ? &input_qnx : NULL;
    *input_data      = qnxinput;
}

static uint32_t qnx_gfx_ctx_vk_get_flags(void *data){
    RARCH_LOG("qnx_gfx_ctx_vk_get_flags\n"); //QNX DEBUG
    uint32_t flags = 0;

#if defined(HAVE_SLANG) && defined(HAVE_SPIRV_CROSS)
   BIT32_SET(flags, GFX_CTX_FLAGS_SHADERS_SLANG);
#endif

    return flags;
}

static void qnx_gfx_ctx_vk_set_flags(void *data, uint32_t flags){ 
    RARCH_LOG("qnx_gfx_ctx_vk_set_flags\n"); //QNX DEBUG
}
static void qnx_gfx_ctx_vk_bind_hw_render(void *data, bool enable){
    RARCH_LOG("qnx_gfx_ctx_vk_bind_hw_render\n"); //QNX DEBUG
}

static void* qnx_gfx_ctx_vk_get_context_data(void *data){
    RARCH_LOG("qnx_gfx_ctx_vk_get_context_data\n"); //QNX DEBUG
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*)data;
    return &qnx->vk.context;
}

static bool qnx_gfx_ctx_vk_set_resize(void *data, unsigned width, unsigned height){
    RARCH_LOG("qnx_gfx_ctx_vk_set_resize\n"); //QNX DEBUG
    qnx_ctx_data_vk_t *qnx = (qnx_ctx_data_vk_t*) data;
    if(!qnx){
        RARCH_ERR("[Screen/VK]: Invalid data pointer passed to qnx_gfx_ctx_vk_set_resize\n");
        return false;
    }

    if(!vulkan_create_swapchain(&qnx->vk, qnx->width, qnx->height, qnx->swap_interval)){
      RARCH_ERR("[Screen/VK]: Failed to update swapchain.\n");
      return false;
    }
    return true;
}

static gfx_ctx_proc_t qnx_gfx_ctx_vk_get_proc_address(const char *symbol){ 
    RARCH_LOG("qnx_gfx_ctx_vk_get_proc_address\n"); //QNX DEBUG
    return NULL; 
}

/*##############################################*/
/*              Driver Structure                */
/*##############################################*/

const gfx_ctx_driver_t gfx_ctx_qnx_vk = {
   qnx_gfx_ctx_vk_init,
   qnx_gfx_ctx_vk_destroy,
   qnx_gfx_ctx_vk_get_api,
   qnx_gfx_ctx_vk_bind_api,
   qnx_gfx_ctx_vk_set_swap_interval,
   qnx_gfx_ctx_vk_set_video_mode,
   qnx_gfx_ctx_vk_get_video_size,
   NULL, /* get_refresh_rate */
   NULL, /* get_video_output_size */
   NULL, /* get_video_output_prev */
   NULL, /* get_video_output_next */
   qnx_gfx_ctx_vk_get_metrics,
   NULL,
   NULL, /* update_title */
   qnx_gfx_ctx_vk_check_window,
   qnx_gfx_ctx_vk_set_resize, 
   qnx_gfx_ctx_vk_has_focus,
   qnx_gfx_ctx_vk_suppress_screensaver,
   false, /* has_windowed */
   qnx_gfx_ctx_vk_swap_buffers,
   qnx_gfx_ctx_vk_input_driver,
   qnx_gfx_ctx_vk_get_proc_address,
   NULL,
   NULL,
   NULL,
   "vk_qnx",
   qnx_gfx_ctx_vk_get_flags,
   qnx_gfx_ctx_vk_set_flags,
   qnx_gfx_ctx_vk_bind_hw_render,
   qnx_gfx_ctx_vk_get_context_data,
   NULL
};
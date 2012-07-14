// ------------------------------------------------------------------------------------------------
// usb/controller.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// USB Controller

typedef struct USB_Controller
{
    struct USB_Controller* next;
    void* hc;

    void (*poll)(struct USB_Controller* controller);
} USB_Controller;

// ------------------------------------------------------------------------------------------------
// Globals

extern USB_Controller* usb_controller_list;

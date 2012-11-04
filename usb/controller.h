// ------------------------------------------------------------------------------------------------
// usb/controller.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// USB Controller

typedef struct UsbController
{
    struct UsbController *next;
    void *hc;

    void (*poll)(struct UsbController *controller);
} UsbController;

// ------------------------------------------------------------------------------------------------
// Globals

extern UsbController *g_usbControllerList;

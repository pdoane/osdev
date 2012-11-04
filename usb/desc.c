// ------------------------------------------------------------------------------------------------
// usb/desc.c
// ------------------------------------------------------------------------------------------------

#include "usb/desc.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
void UsbPrintDeviceDesc(UsbDeviceDesc *desc)
{
    ConsolePrint(" USB: Version=%d.%d Vendor ID=%04x Product ID=%04x Configs=%d\n",
        desc->usbVer >> 8, (desc->usbVer >> 4) & 0xf,
        desc->vendorId, desc->productId,
        desc->confCount);
}

// ------------------------------------------------------------------------------------------------
void UsbPrintConfDesc(UsbConfDesc *desc)
{
    ConsolePrint("  Conf: totalLen=%d intfCount=%d confValue=%d confStr=%d\n",
        desc->totalLen,
        desc->intfCount,
        desc->confValue,
        desc->confStr);
}

// ------------------------------------------------------------------------------------------------
void UsbPrintIntfDesc(UsbIntfDesc *desc)
{
    ConsolePrint("  Intf: altSetting=%d endpCount=%d class=%d subclass=%d protocol=%d str=%d\n",
        desc->altSetting,
        desc->endpCount,
        desc->intfClass,
        desc->intfSubClass,
        desc->intfProtocol,
        desc->intfStr);
}

// ------------------------------------------------------------------------------------------------
void UsbPrintEndpDesc(UsbEndpDesc *desc)
{
    ConsolePrint("  Endp: addr=0x%02x attributes=%d maxPacketSize=%d interval=%d\n",
        desc->addr,
        desc->attributes,
        desc->maxPacketSize,
        desc->interval);
}

// ------------------------------------------------------------------------------------------------
void UsbPrintHidDesc(UsbHidDesc *desc)
{
    ConsolePrint("  HID: ver=%d.%d country=%d descCount=%d descType=%x descLen=%d\n",
        desc->hidVer >> 8, (desc->hidVer >> 8) & 0xff,
        desc->countryCode,
        desc->descCount,
        desc->descType,
        desc->descLen);
}

// ------------------------------------------------------------------------------------------------
void UsbPrintHubDesc(UsbHubDesc *desc)
{
    ConsolePrint(" Hub: port count=%d characteristics=0x%x power time=%d current=%d\n",
            desc->portCount,
            desc->chars,
            desc->portPowerTime,
            desc->current);
}

// ------------------------------------------------------------------------------------------------
// pci/pci.c
// ------------------------------------------------------------------------------------------------

#include "pci/pci.h"
#include "pci/driver.h"
#include "pci/registry.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
static void PciVisit(uint bus, uint dev, uint func)
{
    uint id = PCI_MAKE_ID(bus, dev, func);

    PciDeviceInfo info;
    info.vendorId = PciRead16(id, PCI_CONFIG_VENDOR_ID);
    if (info.vendorId == 0xffff)
    {
        return;
    }

    info.deviceId = PciRead16(id, PCI_CONFIG_DEVICE_ID);
    info.progIntf = PciRead8(id, PCI_CONFIG_PROG_INTF);
    info.subclass = PciRead8(id, PCI_CONFIG_SUBCLASS);
    info.classCode = PciRead8(id, PCI_CONFIG_CLASS_CODE);

    ConsolePrint("%02x:%02x:%d 0x%04x/0x%04x: %s\n",
        bus, dev, func,
        info.vendorId, info.deviceId,
        PciClassName(info.classCode, info.subclass, info.progIntf)
        );

    const PciDriver *driver = g_pciDriverTable;
    while (driver->init)
    {
        driver->init(id, &info);
        ++driver;
    }
}

// ------------------------------------------------------------------------------------------------
void PciInit()
{
    ConsolePrint("PCI Initialization\n");
    for (uint bus = 0; bus < 256; ++bus)
    {
        for (uint dev = 0; dev < 32; ++dev)
        {
            uint baseId = PCI_MAKE_ID(bus, dev, 0);
            u8 headerType = PciRead8(baseId, PCI_CONFIG_HEADER_TYPE);
            uint funcCount = headerType & PCI_TYPE_MULTIFUNC ? 8 : 1;

            for (uint func = 0; func < funcCount; ++func)
            {
                PciVisit(bus, dev, func);
            }
        }
    }
}

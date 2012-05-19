// ------------------------------------------------------------------------------------------------
// pci.c
// ------------------------------------------------------------------------------------------------

#include "pci.h"
#include "console.h"
#include "pci_classify.h"
#include "pci_driver.h"

// ------------------------------------------------------------------------------------------------
static void pci_visit(uint bus, uint dev, uint func)
{
    uint id = PCI_MAKE_ID(bus, dev, func);

    PCI_DeviceInfo info;
    info.vendor_id = pci_in16(id, PCI_CONFIG_VENDOR_ID);
    if (info.vendor_id == 0xffff)
    {
        return;
    }

    info.device_id = pci_in16(id, PCI_CONFIG_DEVICE_ID);
    info.prog_intf = pci_in8(id, PCI_CONFIG_PROG_INTF);
    info.subclass = pci_in8(id, PCI_CONFIG_SUBCLASS);
    info.class_code = pci_in8(id, PCI_CONFIG_CLASS_CODE);

    console_print("%02x:%02x:%d 0x%04x/0x%04x: %s\n",
        bus, dev, func,
        info.vendor_id, info.device_id,
        pci_class_name(info.class_code, info.subclass, info.prog_intf)
        );

    PCI_Driver* driver = pci_driver_table;
    while (driver->init)
    {
        driver->init(id, &info);
        ++driver;
    }
}

// ------------------------------------------------------------------------------------------------
void pci_init()
{
    console_print("PCI Initialization\n");
    for (uint bus = 0; bus < 256; ++bus)
    {
        for (uint dev = 0; dev < 32; ++dev)
        {
            uint base_id = PCI_MAKE_ID(bus, dev, 0);
            u8 header_type = pci_in8(base_id, PCI_CONFIG_HEADER_TYPE);
            uint func_count = header_type & PCI_TYPE_MULTIFUNC ? 8 : 1;

            for (uint func = 0; func < func_count; ++func)
            {
                pci_visit(bus, dev, func);
            }
        }
    }
}

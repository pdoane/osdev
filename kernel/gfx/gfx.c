// ------------------------------------------------------------------------------------------------
// gfx.c
// ------------------------------------------------------------------------------------------------

#include "gfx.h"
#include "../console.h"
#include "../pci_classify.h"
#include "../string.h"

#define DEVICE_HD3000 0x0162

typedef struct GfxDevice
{
	uint    pciId;

    void   *pApertureBar;
    void   *pMMIOBar;
    u16     ioBarAddr;

    // MWDD FIX: Shouldn't size_t be 64-bits?  We are a 64-bit OS.
    u64     apertureBarSize;
    size_t  mmioBarSize;
    u16     ioBarSize;
} GfxDevice;


static GfxDevice s_gfxDevice;

// MWDD FIX: Should this be in the PCI system?
static void ReadBar(uint barNum, u32 *pBarAddress, u32 *pBarMask)
{
    uint pciId  = s_gfxDevice.pciId;
	uint barReg = PCI_CONFIG_BAR0 + barNum * sizeof(uint);

    u32 barAddress = pci_in32(pciId, barReg);
		
    // Find out the size of the bar
    pci_out32(pciId, barReg, 0xFFFFFFFF);
    u32 barMask = pci_in32(pciId, barReg);

    // Restore original address
    pci_out32(pciId, barReg, barAddress);

    *pBarAddress = barAddress;
    *pBarMask    = barMask;
}


void gfx_init(uint id, PCI_DeviceInfo* info)
{
    if (!(((info->class_code << 8) | info->subclass) == PCI_DISPLAY_VGA &&
        info->prog_intf == 0))
    {
        return;
    }

	if ((info->vendor_id != VENDOR_INTEL) ||
		(info->device_id != DEVICE_HD3000))
	{
		console_print("Graphics Controller not recongised!\n");
		return;
	}


	memset(&s_gfxDevice, 0, sizeof(s_gfxDevice));
	s_gfxDevice.pciId = id;
}

void gfx_start()
{
	console_print("...Probing PCIe Config:\n");

	for (uint barNum = 0; barNum < 6; ++barNum)
	{
        u32 barAddress = 0;
        u32 barMask    = 0;
        ReadBar(barNum, &barAddress, &barMask);

        switch (barAddress & 0xF)
        {
            case 0x1: // I/O bar (cheating because we know I/O bar is at least 16 bytes in size).
                s_gfxDevice.ioBarAddr = barAddress & 0xFFFC;
                s_gfxDevice.ioBarSize = (~(u16)(barMask & 0xFFFC)) + 1;
                break;

            case 0x4: // 32-bit prefetchable memory bar
                s_gfxDevice.pMMIOBar    = (void *)((uintptr_t)(barAddress & 0xFFFFFFF0));
                s_gfxDevice.mmioBarSize = (~(barMask & 0xFFFFFFF0)) + 1;
                break;

            case 0xC: // 64-bit prefetchable memory bar
                {
                    barNum++;

                    u32 barAddressUpper = 0;
                    u32 barMaskUpper    = 0;
                    ReadBar(barNum, &barAddressUpper, &barMaskUpper);

                    s_gfxDevice.pApertureBar    = (void *)(((uintptr_t)barAddressUpper) << 32 |  (barAddress & 0xFFFFFFF0));
                    s_gfxDevice.apertureBarSize = ~(((u64)barMaskUpper) << 32 |  (barMask & 0xFFFFFFF0)) + 1;
                }
                break;
        }
	}

 	console_print("    PCI id:       0x%X\n",             s_gfxDevice.pciId);
    console_print("    Aperture Bar: 0x%llX (%llu MB)\n", s_gfxDevice.pApertureBar, s_gfxDevice.apertureBarSize / (1024 * 1024));
    console_print("    MMIO Bar:     0x%X (%u MB)\n",     s_gfxDevice.pMMIOBar,     s_gfxDevice.mmioBarSize / (1024 * 1024));
    console_print("    IO Bar:       0x%X (%u bytes)\n",  s_gfxDevice.ioBarAddr,    s_gfxDevice.ioBarSize);
}
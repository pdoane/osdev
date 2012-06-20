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
	uint pciId;
} GfxDevice;


static GfxDevice s_gfxDevice;

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
	console_print("   PCI id: 0x%X\n", s_gfxDevice.pciId);

	uint barAddress[6];
	uint barMask[6];
	
	for (uint barNum = 0; barNum < 6; ++barNum)
	{
		uint pciId  = s_gfxDevice.pciId;
		uint barReg = PCI_CONFIG_BAR0 + barNum * sizeof(uint);

		barAddress[barNum] = pci_in32(pciId, barReg);
		
		// Find out the size of the bar
		pci_out32(pciId, barReg, 0xFFFFFFFF);
		barMask[barNum] = pci_in32(pciId, barReg);

		// Restore original address
		pci_out32(pciId, barReg, barAddress[barNum]);
	
	    console_print("  Bar[%d] - Address: 0x%08X, Mask: 0x%08X\n", barNum, barAddress[barNum], barMask[barNum]);
	}
}
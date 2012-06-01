// ------------------------------------------------------------------------------------------------
// pci_classify.c
// ------------------------------------------------------------------------------------------------

#include "pci_classify.h"

// ------------------------------------------------------------------------------------------------
const char* pci_device_name(uint vendor_id, uint device_id)
{
    return "Unknown Device";
}

// ------------------------------------------------------------------------------------------------
const char* pci_class_name(uint class_code, uint subclass, uint prog_intf)
{
    switch ((class_code << 8) | subclass)
    {
    case PCI_VGA_COMPATIBLE:            return "VGA-Compatible Device";
    case PCI_STORAGE_SCSI:              return "SCSI Storage Controller";
    case PCI_STORAGE_IDE:               return "IDE Interface";
    case PCI_STORAGE_FLOPPY:            return "Floppy Disk Controller";
    case PCI_STORAGE_IPI:               return "IPI Bus Controller";
    case PCI_STORAGE_RAID:              return "RAID Bus Controller";
    case PCI_STORAGE_ATA:               return "ATA Controller";
    case PCI_STORAGE_SATA:              return "SATA Controller";
    case PCI_STORAGE_OTHER:             return "Mass Storage Controller";
    case PCI_NETWORK_ETHERNET:          return "Ethernet Controller";
    case PCI_NETWORK_TOKEN_RING:        return "Token Ring Controller";
    case PCI_NETWORK_FDDI:              return "FDDI Controller";
    case PCI_NETWORK_ATM:               return "ATM Controller";
    case PCI_NETWORK_ISDN:              return "ISDN Controller";
    case PCI_NETWORK_WORLDFIP:          return "WorldFip Controller";
    case PCI_NETWORK_PICGMG:            return "PICMG Controller";
    case PCI_NETWORK_OTHER:             return "Network Controller";
    case PCI_DISPLAY_VGA:               return "VGA-Compatible Controller";
    case PCI_DISPLAY_XGA:               return "XGA-Compatible Controller";
    case PCI_DISPLAY_3D:                return "3D Controller";
    case PCI_DISPLAY_OTHER:             return "Display Controller";
    case PCI_MULTIMEDIA_VIDEO:          return "Multimedia Video Controller";
    case PCI_MULTIMEDIA_AUDIO:          return "Multimedia Audio Controller";
    case PCI_MULTIMEDIA_PHONE:          return "Computer Telephony Device";
    case PCI_MULTIMEDIA_AUDIO_DEVICE:   return "Audio Device";
    case PCI_MULTIMEDIA_OTHER:          return "Multimedia Controller";
    case PCI_MEMORY_RAM:                return "RAM Memory";
    case PCI_MEMORY_FLASH:              return "Flash Memory";
    case PCI_MEMORY_OTHER:              return "Memory Controller";
    case PCI_BRIDGE_HOST:               return "Host Bridge";
    case PCI_BRIDGE_ISA:                return "ISA Bridge";
    case PCI_BRIDGE_EISA:               return "EISA Bridge";
    case PCI_BRIDGE_MCA:                return "MicroChannel Bridge";
    case PCI_BRIDGE_PCI:                return "PCI Bridge";
    case PCI_BRIDGE_PCMCIA:             return "PCMCIA Bridge";
    case PCI_BRIDGE_NUBUS:              return "NuBus Bridge";
    case PCI_BRIDGE_CARDBUS:            return "CardBus Bridge";
    case PCI_BRIDGE_RACEWAY:            return "RACEway Bridge";
    case PCI_BRIDGE_OTHER:              return "Bridge Device";
    case PCI_COMM_SERIAL:               return "Serial Controller";
    case PCI_COMM_PARALLEL:             return "Parallel Controller";
    case PCI_COMM_MULTIPORT:            return "Multiport Serial Controller";
    case PCI_COMM_MODEM:                return "Modem";
    case PCI_COMM_OTHER:                return "Communication Controller";
    case PCI_SYSTEM_PIC:                return "PIC";
    case PCI_SYSTEM_DMA:                return "DMA Controller";
    case PCI_SYSTEM_TIMER:              return "Timer";
    case PCI_SYSTEM_RTC:                return "RTC";
    case PCI_SYSTEM_PCI_HOTPLUG:        return "PCI Hot-Plug Controller";
    case PCI_SYSTEM_SD:                 return "SD Host Controller";
    case PCI_SYSTEM_OTHER:              return "System Peripheral";
    case PCI_INPUT_KEYBOARD:            return "Keyboard Controller";
    case PCI_INPUT_PEN:                 return "Pen Controller";
    case PCI_INPUT_MOUSE:               return "Mouse Controller";
    case PCI_INPUT_SCANNER:             return "Scanner Controller";
    case PCI_INPUT_GAMEPORT:            return "Gameport Controller";
    case PCI_INPUT_OTHER:               return "Input Controller";
    case PCI_DOCKING_GENERIC:           return "Generic Docking Station";
    case PCI_DOCKING_OTHER:             return "Docking Station";
    case PCI_PROCESSOR_386:             return "386";
    case PCI_PROCESSOR_486:             return "486";
    case PCI_PROCESSOR_PENTIUM:         return "Pentium";
    case PCI_PROCESSOR_ALPHA:           return "Alpha";
    case PCI_PROCESSOR_MIPS:            return "MIPS";
    case PCI_PROCESSOR_CO:              return "CO-Processor";
    case PCI_SERIAL_FIREWIRE:           return "FireWire (IEEE 1394)";
    case PCI_SERIAL_SSA:                return "SSA";
    case PCI_SERIAL_USB:
        switch (prog_intf)
        {
        case PCI_SERIAL_USB_UHCI:       return "USB (UHCI)";
        case PCI_SERIAL_USB_OHCI:       return "USB (OHCI)";
        case PCI_SERIAL_USB_EHCI:       return "USB2";
        case PCI_SERIAL_USB_XHCI:       return "USB3";
        case PCI_SERIAL_USB_OTHER:      return "USB Controller";
        default:                        return "Unknown USB Class";
        }
        break;
    case PCI_SERIAL_FIBER:              return "Fiber Channel";
    case PCI_SERIAL_SMBUS:              return "SMBus";
    case PCI_WIRELESS_IRDA:             return "iRDA Compatible Controller";
    case PCI_WIRLESSS_IR:               return "Consumer IR Controller";
    case PCI_WIRLESSS_RF:               return "RF Controller";
    case PCI_WIRLESSS_BLUETOOTH:        return "Bluetooth";
    case PCI_WIRLESSS_BROADBAND:        return "Broadband";
    case PCI_WIRLESSS_ETHERNET_A:       return "802.1a Controller";
    case PCI_WIRLESSS_ETHERNET_B:       return "802.1b Controller";
    case PCI_WIRELESS_OTHER:            return "Wireless Controller";
    case PCI_INTELLIGENT_I2O:           return "I2O Controller";
    case PCI_SATELLITE_TV:              return "Satellite TV Controller";
    case PCI_SATELLITE_AUDIO:           return "Satellite Audio Controller";
    case PCI_SATELLITE_VOICE:           return "Satellite Voice Controller";
    case PCI_SATELLITE_DATA:            return "Satellite Data Controller";
    case PCI_CRYPT_NETWORK:             return "Network and Computing Encryption Device";
    case PCI_CRYPT_ENTERTAINMENT:       return "Entertainment Encryption Device";
    case PCI_CRYPT_OTHER:               return "Encryption Device";
    case PCI_SP_DPIO:                   return "DPIO Modules";
    case PCI_SP_OTHER:                  return "Signal Processing Controller";
    }

    return "Unknown PCI Class";
}

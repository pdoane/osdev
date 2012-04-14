// ------------------------------------------------------------------------------------------------
// fat16.h
// ------------------------------------------------------------------------------------------------

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned int uint;

#define PACKED __attribute__((__packed__))

#define SECTOR_SIZE				512

// ------------------------------------------------------------------------------------------------
typedef struct BiosParamBlock
{
    u8 jump[3];
    u8 oem[8];
    u16 bytesPerSector;
    u8 sectorsPerCluster;
    u16 reservedSectorCount;
    u8 fatCount;
    u16 rootEntryCount;
    u16 sectorCount;
    u8 mediaType;
    u16 sectorsPerFat;
    u16 sectorsPerTrack;
    u16 headCount;
    u32 hiddenSectorCount;
    u32 largeSectorCount;

    // Extended block
    u8 driveNumber;
    u8 flags;
    u8 signature;
    u32 volumeId;
    u8 volumeLabel[11];
    u8 fileSystem[8];
} PACKED BiosParamBlock;


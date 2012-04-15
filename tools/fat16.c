// ------------------------------------------------------------------------------------------------
// fat16.c
// ------------------------------------------------------------------------------------------------

#include "fat16.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// ------------------------------------------------------------------------------------------------
uint FatGetTotalSectorCount(u8* image)
{
    BiosParamBlock* bpb = (BiosParamBlock*)image;

    if (bpb->sectorCount)
    {
        return bpb->sectorCount;
    }
    else
    {
        return bpb->largeSectorCount;
    }
}

// ------------------------------------------------------------------------------------------------
uint FatGetMetaSectorCount(u8* image)
{
    BiosParamBlock* bpb = (BiosParamBlock*)image;

    return
        bpb->reservedSectorCount +
        bpb->fatCount * bpb->sectorsPerFat +
        (bpb->rootEntryCount * sizeof(DirEntry)) / bpb->bytesPerSector;
}

// ------------------------------------------------------------------------------------------------
uint FatGetClusterCount(u8* image)
{
    BiosParamBlock* bpb = (BiosParamBlock*)image;

    uint totalSectorCount = FatGetTotalSectorCount(image);
    uint metaSectorCount = FatGetMetaSectorCount(image);
    uint dataSectorCount = totalSectorCount - metaSectorCount;

    return dataSectorCount / bpb->sectorsPerCluster;
}

// ------------------------------------------------------------------------------------------------
uint FatGetImageSize(u8* image)
{
    BiosParamBlock* bpb = (BiosParamBlock*)image;

    return FatGetTotalSectorCount(image) * bpb->bytesPerSector;
}

// ------------------------------------------------------------------------------------------------
u16* FatGetTable(u8* image, uint fatIndex)
{
    BiosParamBlock* bpb = (BiosParamBlock*)image;

    assert(fatIndex < bpb->fatCount);

    uint offset = (bpb->reservedSectorCount + fatIndex * bpb->sectorsPerFat) * bpb->bytesPerSector;

    return (u16*)(image + offset);
}

// ------------------------------------------------------------------------------------------------
u16 FatGetClusterValue(u8* image, uint fatIndex, uint clusterIndex)
{
    u16* fat = FatGetTable(image, fatIndex);

    assert(clusterIndex < FatGetClusterCount(image));

    return fat[clusterIndex];
}

// ------------------------------------------------------------------------------------------------
uint FatGetClusterOffset(u8* image, uint clusterIndex)
{
    BiosParamBlock* bpb = (BiosParamBlock*)image;

    return
        (bpb->reservedSectorCount + bpb->fatCount * bpb->sectorsPerFat) * bpb->bytesPerSector +
        bpb->rootEntryCount * sizeof(DirEntry) +
        (clusterIndex - 2) * (bpb->sectorsPerCluster * bpb->bytesPerSector);
}

// ------------------------------------------------------------------------------------------------
void FatSetClusterValue(u8* image, uint fatIndex, uint clusterIndex, u16 value)
{
    u16* fat = FatGetTable(image, fatIndex);

    assert(clusterIndex < FatGetClusterCount(image));

    fat[clusterIndex] = value;
}

// ------------------------------------------------------------------------------------------------
DirEntry* FatGetRootDirectory(u8* image)
{
    BiosParamBlock* bpb = (BiosParamBlock*)image;

    uint offset = (bpb->reservedSectorCount + bpb->fatCount * bpb->sectorsPerFat) * bpb->bytesPerSector;
    uint dataSize = bpb->rootEntryCount * sizeof(DirEntry);

    assert(offset + dataSize <= FatGetImageSize(image));

    return (DirEntry*)(image + offset);
}

// ------------------------------------------------------------------------------------------------
u8* FatAllocImage(uint imageSize)
{
    u8* image = (u8*)malloc(imageSize);
    memset(image, ENTRY_ERASED, imageSize);
    return image;
}

// ------------------------------------------------------------------------------------------------
bool FatInitImage(u8* image, u8* bootSector)
{
    BiosParamBlock* bpb = (BiosParamBlock*)bootSector;

    // Validate signature
    if (bootSector[0x1fe] != 0x55 || bootSector[0x1ff] != 0xaa)
    {
        return false;
    }

    // Copy to sector 0
    memcpy(image, bootSector, bpb->bytesPerSector);

    // Initialize clusters
    uint clusterCount = FatGetClusterCount(image);
    assert(clusterCount >= 2);

    FatUpdateCluster(image, 0, 0xff00 | bpb->mediaType);    // media type
    FatUpdateCluster(image, 1, 0xffff);                     // end of chain cluster marker

    for (uint clusterIndex = 2; clusterIndex < clusterCount; ++clusterIndex)
    {
        FatUpdateCluster(image, clusterIndex, 0x0000);
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
u16 FatFindFreeCluster(u8* image)
{
    uint clusterCount = FatGetClusterCount(image);

    u16* fat = FatGetTable(image, 0);

    for (uint clusterIndex = 2; clusterIndex < clusterCount; ++clusterIndex)
    {
        u16 data = fat[clusterIndex];
        if (data == 0)
        {
            return clusterIndex;
        }
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
void FatUpdateCluster(u8* image, uint clusterIndex, u16 value)
{
    BiosParamBlock* bpb = (BiosParamBlock*)image;

    for (uint fatIndex = 0; fatIndex < bpb->fatCount; ++fatIndex)
    {
        FatSetClusterValue(image, fatIndex, clusterIndex, value);
    }
}

// ------------------------------------------------------------------------------------------------
DirEntry* FatFindFreeRootEntry(u8* image)
{
    BiosParamBlock* bpb = (BiosParamBlock*)image;

    DirEntry* start = FatGetRootDirectory(image);
    DirEntry* end = start + bpb->rootEntryCount;

    for (DirEntry* entry = start; entry != end; ++entry)
    {
        u8 marker = entry->name[0];
        if (marker == ENTRY_AVAILABLE || marker == ENTRY_ERASED)
        {
            return entry;
        }
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
static void SetPaddedString(u8* dst, uint dstLen, const char* src, uint srcLen)
{
    if (src)
    {
        if (srcLen > dstLen)
        {
            memcpy(dst, src, dstLen);
        }
        else
        {
            memcpy(dst, src, srcLen);
            memset(dst + srcLen, ' ', dstLen - srcLen);
        }

        for (uint i = 0; i < dstLen; ++i)
        {
            dst[i] = toupper(dst[i]);
        }
    }
    else
    {
        memset(dst, ' ', dstLen);
    }
}

// ------------------------------------------------------------------------------------------------
void FatSplitPath(u8 dstName[8], u8 dstExt[3], const char* path)
{
    const char* name = strrchr(path, '/');
    if (name)
    {
        name = name + 1;
    }
    else
    {
        name = path;
    }

    uint nameLen = strlen(name);

    char* ext = 0;
    uint extLen = 0;
    char* p = strchr(name, '.');
    if (p)
    {
        nameLen = p - name;
        ext = p + 1;
        extLen = strlen(ext);
    }

    SetPaddedString(dstName, 8, name, nameLen);
    SetPaddedString(dstExt, 3, ext, extLen);
}

// ------------------------------------------------------------------------------------------------
void FatUpdateDirEntry(DirEntry* entry, u16 clusterIndex, const u8 name[8], const u8 ext[3], uint fileSize)
{
    entry->clusterIndex = clusterIndex;
    memcpy(entry->name, name, sizeof(entry->name));
    memcpy(entry->ext, ext, sizeof(entry->ext));
    entry->fileSize = fileSize;
}

// ------------------------------------------------------------------------------------------------
void FatRemoveDirEntry(DirEntry* entry)
{
    entry->name[0] = ENTRY_AVAILABLE;
}

// ------------------------------------------------------------------------------------------------
u16 FatAddData(u8* image, const void* data, uint len)
{
    BiosParamBlock* bpb = (BiosParamBlock*)image;
    uint bytesPerCluster = bpb->sectorsPerCluster * bpb->bytesPerSector;

    // Skip empty files
    if (len == 0)
    {
        return 0;
    }

    u16 endOfChainValue = FatGetClusterValue(image, 0, 1);

    u16 prevClusterIndex = 0;
    u16 rootClusterIndex = 0;

    // Copy data one cluster at a time.
    const u8* p = (const u8*)data;
    const u8* end = p + len;
    while (p < end)
    {
        // Find a free cluster
        u16 clusterIndex = FatFindFreeCluster(image);
        if (clusterIndex == 0)
        {
            // Ran out of disk space, free allocated clusters
            if (rootClusterIndex != 0)
            {
                FatRemoveData(image, rootClusterIndex);
            }

            return 0;
        }

        // Determine amount of data to copy
        uint count = end - p;
        if (count > bytesPerCluster)
        {
            count = bytesPerCluster;
        }

        // Transfer bytes into image at cluster location
        uint offset = FatGetClusterOffset(image, clusterIndex);
        memcpy(image + offset, p, count);
        p += count;

        // Update FAT clusters
        FatUpdateCluster(image, clusterIndex, endOfChainValue);
        if (prevClusterIndex)
        {
            FatUpdateCluster(image, prevClusterIndex, clusterIndex);
        }
        else
        {
            rootClusterIndex = clusterIndex;
        }

        prevClusterIndex = clusterIndex;
    }

    return rootClusterIndex;
}

// ------------------------------------------------------------------------------------------------
void FatRemoveData(u8* image, uint clusterIndex)
{
    assert(clusterIndex != 0);

    u16 endOfChainValue = FatGetClusterValue(image, 0, 1);

    while (clusterIndex != endOfChainValue)
    {
        u16 nextClusterIndex = FatGetClusterValue(image, 0, clusterIndex);
        FatUpdateCluster(image, clusterIndex, 0);
        clusterIndex = nextClusterIndex;
    }
}

// ------------------------------------------------------------------------------------------------
DirEntry* FatAddFile(u8* image, const char* path, const void* data, uint len)
{
    // Find Directory Entry
    DirEntry* entry = FatFindFreeRootEntry(image);
    if (!entry)
    {
        return 0;
    }

    // Add File
    u16 rootClusterIndex = FatAddData(image, data, len);
    if (!rootClusterIndex)
    {
        return 0;
    }

    // Update Directory Entry
    u8 name[8];
    u8 ext[3];
    FatSplitPath(name, ext, path);

    FatUpdateDirEntry(entry, rootClusterIndex, name, ext, len);
    return entry;
}

// ------------------------------------------------------------------------------------------------
void FatRemoveFile(u8* image, DirEntry* entry)
{
    FatRemoveData(image, entry->clusterIndex);
    FatRemoveDirEntry(entry);
}

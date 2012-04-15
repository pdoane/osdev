// ------------------------------------------------------------------------------------------------
// fat16_test
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "fat16.h"
#include <string.h>

// ------------------------------------------------------------------------------------------------
BiosParamBlock bpb =
{
    .jump = { 0xeb, 0x3c, 0x90 }, // short jmp followed by nop
    .oem = { 'F', 'A', 'T', ' ', 'T', 'E', 'S', 'T' },
    .bytesPerSector = 512,
    .sectorsPerCluster = 1,
    .reservedSectorCount = 4,
    .fatCount = 2,
    .rootEntryCount = 512,
    .sectorCount = 64416,
    .mediaType = 0xf8,
    .sectorsPerFat = 250,
    .sectorsPerTrack = 63,
    .headCount = 255,
    .hiddenSectorCount = 0,
    .largeSectorCount = 0,
    .driveNumber = 0x80,
    .flags = 0x00,
    .signature = 0x29,
    .volumeId = 0xa0a1a2a3,
    .volumeLabel = { 'V', 'o', 'l', 'u', 'm', 'e', ' ', 'N', 'a', 'm', 'e' },
    .fileSystem = { 'F', 'A', 'T', '1', '6', ' ', ' ', ' ' },
};

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    expect(sizeof(struct BiosParamBlock) == 62);
    expect(sizeof(struct DirEntry) == 32);

    uint metaSectorCount = 536;
    uint clusterCount = 63880;

    // Allocate image
    uint imageSize = bpb.sectorCount * bpb.bytesPerSector;
    u8* image = FatAllocImage(imageSize);
    for (uint i = 0; i < imageSize; ++i)
    {
        expect(image[i] == ENTRY_ERASED);
    }

    // Create dummy boot sector
    u8 bootSector[0x200];
    memset(bootSector, 0, sizeof(bootSector));
    memcpy(bootSector, &bpb, sizeof(bpb));
    bootSector[0x1fe] = 0x55;
    bootSector[0x1ff] = 0xaa;

    // Initialize image
    expect(FatInitImage(image, bootSector));
    expect(FatGetImageSize(image) == imageSize);
    expect(FatGetMetaSectorCount(image) == metaSectorCount);
    expect(FatGetClusterCount(image) == clusterCount);

    expect(FatGetClusterValue(image, 0, 0) == 0xfff8);
    expect(FatGetClusterValue(image, 1, 0) == 0xfff8);
    expect(FatGetClusterValue(image, 0, 1) == 0xffff);
    expect(FatGetClusterValue(image, 1, 1) == 0xffff);

    // Allocate all clusters
    for (uint i = 2; i < clusterCount; ++i)
    {
        u16 index = FatFindFreeCluster(image);
        expect(index != 0);
        FatUpdateCluster(image, index, 0xffff);
    }

    // Try to allocate another cluster -- this should fail.
    u16 clusterIndex = FatFindFreeCluster(image);
    expect(clusterIndex == 0);

    // Free all clusters we just allocated
    for (uint i = 2; i < clusterCount; ++i)
    {
        FatUpdateCluster(image, i, 0x0000);
    }

    // Name splitting
    u8 name[8];
    u8 ext[3];

    FatSplitPath(name, ext, "a");
    expect(memcmp(name, "A       ", sizeof(name)) == 0);
    expect(memcmp(ext, "   ", sizeof(ext)) == 0);

    FatSplitPath(name, ext, "abc.de");
    expect(memcmp(name, "ABC     ", sizeof(name)) == 0);
    expect(memcmp(ext, "DE ", sizeof(ext)) == 0);

    FatSplitPath(name, ext, "abcdEFGH.iJk");
    expect(memcmp(name, "ABCDEFGH", sizeof(name)) == 0);
    expect(memcmp(ext, "IJK", sizeof(ext)) == 0);

    FatSplitPath(name, ext, "dir/abcdefgh0.ijk0");
    expect(memcmp(name, "ABCDEFGH", sizeof(name)) == 0);
    expect(memcmp(ext, "IJK", sizeof(ext)) == 0);

    // Directory Entry Updating
    DirEntry* entry = FatFindFreeRootEntry(image);
    expect(entry != 0);

    FatSplitPath(name, ext, "dir/abcdefgh0.ijk0");
    FatUpdateDirEntry(entry, 1, name, ext, 2);
    expect(entry->clusterIndex == 1);
    expect(entry->fileSize == 2);
    expect(memcmp(entry->name, "ABCDEFGH", sizeof(entry->name)) == 0);
    expect(memcmp(entry->ext, "IJK", sizeof(entry->ext)) == 0);

    FatRemoveDirEntry(entry);

    // Allocate all directory entries
    for (uint i = 0; i < bpb.rootEntryCount; ++i)
    {
        entry = FatFindFreeRootEntry(image);
        expect(entry != 0);

        FatUpdateDirEntry(entry, 1, name, ext, 0);
    }

    // Try to allocate another directory -- this should fail.
    entry = FatFindFreeRootEntry(image);
    expect(entry == 0);

    // Free all directory entries we just allocated
    DirEntry* root = FatGetRootDirectory(image);
    for (uint i = 0; i < bpb.rootEntryCount; ++i)
    {
        DirEntry* entry = root + i;
        FatRemoveDirEntry(entry);
    }

    // Add file data, single cluster
    char* data = "Hello World!";
    u16 rootClusterIndex = FatAddData(image, data, strlen(data) + 1);
    expect(rootClusterIndex != 0);
    expect(FatGetClusterValue(image, 0, rootClusterIndex) == 0xffff);
    char* writtenData = (char*)(image + FatGetClusterOffset(image, rootClusterIndex));
    expect(strcmp(writtenData, data) == 0);
    FatRemoveData(image, rootClusterIndex);
    expect(FatGetClusterValue(image, 0, rootClusterIndex) == 0x0000);

    // Add file data, multi-cluster
#define INT_DATA_COUNT 500

    uint intData[INT_DATA_COUNT];
    for (uint i = 0; i < INT_DATA_COUNT; ++i)
    {
        intData[i] = i;
    }

    rootClusterIndex = FatAddData(image, intData, sizeof(intData));
    expect(rootClusterIndex != 0);

    uint index = 0;
    clusterIndex = rootClusterIndex;
    while (clusterIndex != 0xffff && index < INT_DATA_COUNT)
    {
        uint* writtenData = (uint*)(image + FatGetClusterOffset(image, clusterIndex));
        for (uint i = 0; i < 128; ++i)
        {
            if (index < INT_DATA_COUNT)
            {
                expect(writtenData[i] == intData[index]);
                ++index;
            }
        }

        clusterIndex = FatGetClusterValue(image, 0, clusterIndex);
    }

    expect(index == INT_DATA_COUNT);

    FatRemoveData(image, rootClusterIndex);
    expect(FatGetClusterValue(image, 0, rootClusterIndex) == 0x0000);

    // Add file test
    entry = FatAddFile(image, "boot/boot.bin", data, strlen(data) + 1);
    expect(entry);
    rootClusterIndex = entry->clusterIndex;
    expect(rootClusterIndex != 0);
    expect(FatGetClusterValue(image, 0, rootClusterIndex) == 0xffff);
    writtenData = (char*)(image + FatGetClusterOffset(image, rootClusterIndex));
    expect(strcmp(writtenData, data) == 0);
    FatRemoveFile(image, entry);

    expect(FatGetClusterValue(image, 0, rootClusterIndex) == 0x0000);
    expect(entry->name[0] == ENTRY_AVAILABLE);

    printf("Success!\n");
    return EXIT_SUCCESS;
}

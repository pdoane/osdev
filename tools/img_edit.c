// ------------------------------------------------------------------------------------------------
// tools/img_edit.c
// ------------------------------------------------------------------------------------------------

#include "tools/fat16.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ------------------------------------------------------------------------------------------------
static void Usage()
{
    fprintf(stdout, "Usage: img_edit <command> <options>\n");
    fprintf(stdout, "       img_edit create <image path> <boot sector path>\n");
    fprintf(stdout, "       img_edit add <image path> <data path>\n");
}

// ------------------------------------------------------------------------------------------------
static bool ReadFile(const char* path, u8** outData, uint* outLen)
{
    bool result = false;

    // Open File for Input
    FILE* fp = fopen(path, "rb");
    if (fp)
    {
        // Determine File Size
        fseek(fp, 0, SEEK_END);
        uint len = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // Allocate Storage
        u8* data = malloc(len);
        if (data)
        {
            // Read File
            if (fread(data, 1, len, fp) == len)
            {
                *outData = data;
                *outLen = len;
                result = true;
            }
        }

        fclose(fp);
    }

    return result;
}


// ------------------------------------------------------------------------------------------------
static bool WriteFile(const char* path, u8* data, uint len)
{
    bool result = false;

    // Open File for Output
    FILE* fp = fopen(path, "wb");
    if (fp)
    {
        // Write Image
        if (fwrite(data, 1, len, fp) == len)
        {
            result = true;
        }

        fclose(fp);
    }

    return result;
}

// ------------------------------------------------------------------------------------------------
static bool CreateImage(const char* imagePath, const char* bootSectorPath)
{
    bool result = false;

    u8* image = 0;
    u8* bootSector = 0;

    uint bytesPerSector = 512;
    uint sectorCount = 64416;

    // Allocate Image
    image = FatAllocImage(sectorCount * bytesPerSector);
    if (!image)
    {
        fprintf(stderr, "Failed to allocate image\n");
        goto failure;
    }

    // Read Boot Sector
    uint bootSectorLen;
    if (!ReadFile(bootSectorPath, &bootSector, &bootSectorLen))
    {
        fprintf(stderr, "Failed to read boot loader '%s'\n", bootSectorPath);
        goto failure;
    }

    if (bootSectorLen != bytesPerSector)
    {
        fprintf(stderr, "Invalid boot loader size %d, expected %d\n", bootSectorLen, 512);
        return false;
    }

    // Prepare Bios Parameter Block
    BiosParamBlock* bpb = (BiosParamBlock*)bootSector;
    memcpy(bpb->oem, "POS     ", sizeof(bpb->oem));
    bpb->bytesPerSector = bytesPerSector;
    bpb->sectorsPerCluster = 1;
    bpb->reservedSectorCount = 1;
    bpb->fatCount = 2;
    bpb->rootEntryCount = 512;
    bpb->sectorCount = sectorCount;
    bpb->mediaType = 0xf8;
    bpb->sectorsPerFat = 250;
    bpb->sectorsPerTrack = 63;
    bpb->headCount = 255;
    bpb->hiddenSectorCount = 0;
    bpb->largeSectorCount = 0;
    bpb->driveNumber = 0x80;
    bpb->flags = 0x00;
    bpb->signature = 0x29;
    bpb->volumeId = 0xa0a1a2a3;
    memcpy(bpb->volumeLabel, "Volume Name", sizeof(bpb->volumeLabel));
    memcpy(bpb->fileSystem, "FAT16   ", sizeof(bpb->fileSystem));

    // Initialize Image
    if (!FatInitImage(image, bootSector))
    {
        fprintf(stderr, "Failed to initialize image\n");
        goto failure;
    }

    // Write Image
    if (!WriteFile(imagePath, image, sectorCount * bytesPerSector))
    {
        fprintf(stderr, "Failed to write image file '%s'\n", imagePath);
        goto failure;
    }

    result = true;

 failure:
    free(image);
    free(bootSector);
    return result;
}

// ------------------------------------------------------------------------------------------------
static bool AddFile(const char* imagePath, const char* dataPath)
{
    bool result = false;
    u8* image = 0;
    u8* data = 0;

    // Read Image
    uint imageSize;
    if (!ReadFile(imagePath, &image, &imageSize))
    {
        fprintf(stderr, "Failed to read image file '%s'\n", imagePath);
        goto failure;
    }

    // Read Data
    uint dataSize;
    if (!ReadFile(dataPath, &data, &dataSize))
    {
        fprintf(stderr, "Failed to read data file '%s'\n", dataPath);
        goto failure;
    }

    // Find Directory Entry
    DirEntry* entry = FatFindFreeRootEntry(image);
    if (!entry)
    {
        fprintf(stderr, "Failed to find directory entry\n");
        goto failure;
    }

    // Add File
    if (!FatAddFile(image, dataPath, data, dataSize))
    {
        fprintf(stderr, "Failed to add file to image\n");
        goto failure;
    }

    // Write Image
    if (!WriteFile(imagePath, image, imageSize))
    {
        fprintf(stderr, "Failed to write image file '%s'\n", imagePath);
        goto failure;
    }

    result = true;

failure:
    free(image);
    free(data);
    return result;
}

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    // Parse arguments
    if (argc < 2)
    {
        Usage();
        return EXIT_FAILURE;
    }

    const char* cmd = argv[1];
    if (!strcmp(cmd, "create"))
    {
        if (argc != 4)
        {
            Usage();
            return EXIT_FAILURE;
        }

        const char* imagePath = argv[2];
        const char* bootSectorPath = argv[3];

        if (!CreateImage(imagePath, bootSectorPath))
        {
            return EXIT_FAILURE;
        }
    }
    else if (!strcmp(cmd, "add"))
    {
        if (argc != 4)
        {
            Usage();
            return EXIT_FAILURE;
        }

        const char* imagePath = argv[2];
        const char* dataPath = argv[3];

        if (!AddFile(imagePath, dataPath))
        {
            return EXIT_FAILURE;
        }
    }

    // Success!
    return EXIT_SUCCESS;
}

// ------------------------------------------------------------------------------------------------
// tools/read_boot.c
// ------------------------------------------------------------------------------------------------

#include "tools/fat16.h"
#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// ------------------------------------------------------------------------------------------------
static void Usage()
{
    fprintf(stdout, "Usage: read_boot <disk path> <dump path>\n");
}

// ------------------------------------------------------------------------------------------------
static bool ReadSector(const char* path, char* data)
{
    bool result = false;

    HANDLE hFile = CreateFile(path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytesRead;
        if (ReadFile(hFile, data, SECTOR_SIZE, &bytesRead, NULL))
        {
            result = true;
        }

        CloseHandle(hFile);
    }

    return result;
}

// ------------------------------------------------------------------------------------------------
static bool WriteSector(const char* path, const char* data)
{
    bool result = false;

    HANDLE hFile = CreateFile(path,
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL, CREATE_ALWAYS, 0, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if (WriteFile(hFile, data, SECTOR_SIZE, &bytesWritten, NULL))
        {
            result = true;
        }

        CloseHandle(hFile);
    }

    return result;
}

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    // Parse arguments
    if (argc < 3)
    {
        Usage();
        return EXIT_FAILURE;
    }

    const char* diskPath = argv[1];
    const char* dumpPath = argv[2];

    // Read sector 0
    char curSector[SECTOR_SIZE];
    if (!ReadSector(diskPath, curSector))
    {
        fprintf(stderr, "Failed to read current boot sector: %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Dump sector 0
    if (!WriteSector(dumpPath, curSector))
    {
        fprintf(stderr, "Failed to dump boot sector: %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Output parameter block
    BiosParamBlock* bpb = (BiosParamBlock *)curSector;

    char oem[9];
    char volumeLabel[12];
    char fileSystem[9];
    memcpy(oem, bpb->oem, sizeof(bpb->oem));
    memcpy(volumeLabel, bpb->volumeLabel, sizeof(bpb->volumeLabel));
    memcpy(fileSystem, bpb->fileSystem, sizeof(bpb->fileSystem));
    oem[8] = '\0';
    volumeLabel[11] = '\0';
    fileSystem[8] = '\0';

    fprintf(stdout, "OEM = %s\n", oem);
    fprintf(stdout, "Bytes Per Sector = %d\n", bpb->bytesPerSector);
    fprintf(stdout, "Sectors Per Cluster = %d\n", bpb->sectorsPerCluster);
    fprintf(stdout, "Reserved Sector Count = %d\n", bpb->reservedSectorCount);
    fprintf(stdout, "FAT Count = %d\n", bpb->fatCount);
    fprintf(stdout, "Root Entry Count = %d\n", bpb->rootEntryCount);
    fprintf(stdout, "Sector Count = %d\n", bpb->sectorCount);
    fprintf(stdout, "Media Type = 0x%x\n", bpb->mediaType);
    fprintf(stdout, "Sectors Per FAT = %d\n", bpb->sectorsPerFat);
    fprintf(stdout, "Sectors Per Track = %d\n", bpb->sectorsPerTrack);
    fprintf(stdout, "Head Count = %d\n", bpb->headCount);
    fprintf(stdout, "Hidden Sector Count = %d\n", bpb->hiddenSectorCount);
    fprintf(stdout, "Large Sector Count = %d\n", bpb->largeSectorCount);

    fprintf(stdout, "Drive Number = %d\n", bpb->driveNumber);
    fprintf(stdout, "Flags = %d\n", bpb->flags);
    fprintf(stdout, "Signature = %d\n", bpb->signature);
    fprintf(stdout, "Volume ID = 0x%8x\n", bpb->volumeId);
    fprintf(stdout, "Volume Label = %s\n", volumeLabel);
    fprintf(stdout, "File System = %s\n", fileSystem);

    // Success!
    return EXIT_SUCCESS;
}

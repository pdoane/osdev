// ------------------------------------------------------------------------------------------------
// set_boot.c
// ------------------------------------------------------------------------------------------------

#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SECTOR_SIZE             512
#define JMP_CODE_SIZE           3
#define BIOS_PARAM_BLOCK_SIZE   62

// ------------------------------------------------------------------------------------------------
static void Usage()
{
    fprintf(stdout, "Usage: set_boot <disk path> <boot loader image path>\n");
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
        NULL, OPEN_EXISTING, 0, NULL);

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
    const char* bootSectorPath = argv[2];

    // Read boot loader
    char bootLoader[SECTOR_SIZE];
    if (!ReadSector(bootSectorPath, bootLoader))
    {
        fprintf(stderr, "Failed to read boot loader: %d\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Read sector 0
    char curSector[SECTOR_SIZE];
    if (!ReadSector(diskPath, curSector))
    {
        fprintf(stderr, "Failed to read current boot sector: %d\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Prepare new boot sector preserving the bios parameter block
    char newSector[SECTOR_SIZE];
    memcpy(newSector, bootLoader, SECTOR_SIZE);
    memcpy(newSector + JMP_CODE_SIZE, curSector + JMP_CODE_SIZE, BIOS_PARAM_BLOCK_SIZE - JMP_CODE_SIZE);

    // Write sector 0
    if (!WriteSector(diskPath, newSector))
    {
        fprintf(stderr, "Failed to write new boot sector: %d\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Success!
    fprintf(stdout, "Success!\n");
    return EXIT_SUCCESS;
}

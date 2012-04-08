// ------------------------------------------------------------------------------------------------
// diskwrite.cpp
// ------------------------------------------------------------------------------------------------

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>

// ------------------------------------------------------------------------------------------------
static void Usage()
{
	fprintf(stdout, "Usage: diskwrite <disk path> <boot sector image path>\n");
}

// ------------------------------------------------------------------------------------------------
static BOOL ReadBootSector(const char* path, char* data)
{
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	DWORD bytesRead;
	BOOL bResult = ReadFile(hFile, data, 512, &bytesRead, NULL);

	CloseHandle(hFile);

	return bResult;
}

// ------------------------------------------------------------------------------------------------
static BOOL WriteBootSector(const char* path, const char* data)
{
	HANDLE hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	
	DWORD bytesWritten;
	BOOL bResult = WriteFile(hFile, data, 512, &bytesWritten, NULL);

	CloseHandle(hFile);

	return bResult;
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

	// Read boot sector
	char buffer[512];
	if (!ReadBootSector(bootSectorPath, buffer))
	{
		fprintf(stderr, "Failed to read boot sector image: %d\n", GetLastError());
		return EXIT_FAILURE;
	}

	// Write boot sector
	if (!WriteBootSector(diskPath, buffer))
	{
		fprintf(stderr, "Failed to write boot sector image: %d\n", GetLastError());
		return EXIT_FAILURE;
	}

	// Success!
	fprintf(stdout, "Success!\n");
	return EXIT_SUCCESS;
}

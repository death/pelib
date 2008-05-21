// Includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <conio.h>
#include <cassert>

#include "PortableExecutable.h"
#include "Util.h"

using namespace std;
using namespace PE;

#pragma warning(disable:4326)

// Enumerations
typedef enum {
    SUCCESS, FAILURE
} RETURN_CODE;

// Function prototypes
void DisplayDirInfo(int nLevel, const char *pszName, CResourceDirectory *prd);
void StartLine(int nLevel);
void TraverseDir(const char *pszName, CResourceDirectory *prd, int nLevel);

// Main programme
RETURN_CODE main(int argc, char *argv[])
{
    PE_ERROR error;
    CPortableExecutable pe;

    // Remove stdout buffer
    setbuf(stdout, NULL);

    // Headline
    printf("Resource Info (class version %d.%d) by DEATH of Execution in 2002\n", PE_MAJVER, PE_MINVER);

    // Check number of arguments
    if (argc < 2) {
        printf("Usage: ResourceInfo <pefile>\n");
        return(SUCCESS);
    }

    // Load the PE
    error = pe.Load(argv[1]);
    if (error != PE_NONE) {
        printf("Error: %s\n", CUtil::GetErrorString(error));
        return(FAILURE);
    }

    // Check if resource directory was loaded
    if (pe.m_ddResource.IsInitialized() == TRUE) {
        CResourceDirectory *prd = pe.m_ddResource.GetRootDirectory();
        assert(prd);
        
        // Get resource directory info
        TraverseDir("Root", prd, 0);

    } else {
        printf("Error: Resource directory wasn't loaded\n");
        return(FAILURE);
    }

    return(SUCCESS);
}

void DisplayDirInfo(int nLevel, const char *pszName, CResourceDirectory *prd)
{
    StartLine(nLevel);
    printf("+ %s (Dirs: %d, Entries: %d)\n", pszName, prd->GetNumDirectories(), prd->GetNumEntries());

    for (DWORD i = 0; i < prd->GetNumEntries(); i++) {
        CResourceEntry *pre = prd->GetEntry(i);
        assert(pre);

        IMAGE_RESOURCE_DIRECTORY_ENTRY rde;
        pre->GetDirInfo(&rde);

        IMAGE_RESOURCE_DATA_ENTRY de;
        pre->GetInfo(&de);

        StartLine(nLevel + 1);
        printf("Entry #%ld - ID:%04X RVA: %08lX Size: %08lX\n", i, rde.Id, de.OffsetToData, de.Size);
    }
}

void StartLine(int nLevel)
{
    for (int i = 0; i < (nLevel * 2); i++) {
        printf(" ");
    }
}

void TraverseDir(const char *pszName, CResourceDirectory *prd, int nLevel)
{
    DisplayDirInfo(nLevel, pszName, prd);

    for (DWORD i = 0; i < prd->GetNumDirectories(); i++) {
        CResourceDirectory *prdSub = prd->GetDirectory(i);
        TraverseDir(prdSub->GetName().c_str(), prdSub, nLevel + 1);
    }
}
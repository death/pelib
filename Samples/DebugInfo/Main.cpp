// Includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>

#include "PortableExecutable.h"
#include "Util.h"

using namespace PE;
using namespace std;

#pragma warning(disable:4326)

// Enumerations
typedef enum {
    SUCCESS, FAILURE
} RETURN_CODE;

// Main programme
RETURN_CODE main(int argc, char *argv[])
{
    PE_ERROR error;
    CPortableExecutable pe;
    IMAGE_DEBUG_DIRECTORY dirDeb;
    DWORD dwCount;

    // Headline
    printf("Debug Info (class version %d.%d) by DEATH of Execution in 2002\n", PE_MAJVER, PE_MINVER);

    // Check number of arguments
    if (argc < 2) {
        printf("Usage: DebugInfo <pefile>\n");
        return(SUCCESS);
    }

    // Load the PE
    error = pe.Load(argv[1]);
    if (error != PE_NONE) {
        printf("Error: %s\n", CUtil::GetErrorString(error));
        return(FAILURE);
    }

    if (pe.m_ddDebug.IsInitialized()) {
        printf("Number of debug directory tables: %ld\n\n", pe.m_ddDebug.GetNumTables());

        // Display information on each of the tables
        for (dwCount = 0; dwCount < pe.m_ddDebug.GetNumTables(); dwCount++) {
            pe.m_ddDebug.GetTable(dwCount, &dirDeb);

            printf("Debug Table #%ld\n", dwCount + 1);

            printf("\tDebug Flags: %08lX\n", dirDeb.Characteristics);
            printf("\tTime/Date debug data was created: %s", CUtil::GetDateString(dirDeb.TimeDateStamp));
            printf("\tDebug data version: %d.%d\n", dirDeb.MajorVersion, dirDeb.MinorVersion);
            printf("\tDebug type: %s (%08lX)\n", CUtil::GetDebugTypeString(dirDeb.Type), dirDeb.Type);
            printf("\tDebug data size: %ld\n", dirDeb.SizeOfData);
            printf("\tDebug data RVA: %08lX\n", dirDeb.AddressOfRawData);
            printf("\tDebug data offset: %08lX\n", dirDeb.PointerToRawData);
        }
    } else {
        printf("No debug directory\n");
    }

    return(SUCCESS);
}

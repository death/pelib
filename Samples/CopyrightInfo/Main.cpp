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

    // Headline
    printf("Copyright Info (class version %d.%d) by DEATH of Execution in 2002\n", PE_MAJVER, PE_MINVER);

    // Check number of arguments
    if (argc < 2) {
        printf("Usage: CopyrightInfo <pefile>\n");
        return(SUCCESS);
    }

    // Load the PE
    error = pe.Load(argv[1]);
    if (error != PE_NONE) {
        printf("Error: %s\n", CUtil::GetErrorString(error));
        return(FAILURE);
    }

    if (pe.m_ddCopyright.IsInitialized() == TRUE) {
        printf("Copyright info:\n");
        printf("%s\n", pe.m_ddCopyright.GetString());
    } else {
        printf("No copyright directory\n");
    }

    return(SUCCESS);
}

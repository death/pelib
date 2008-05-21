// Includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>

#include "PortableExecutable.h"
#include "Util.h"

using namespace std;
using namespace PE;

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
    DWORD dwCount;
    DWORD dwCount2;
    IMAGE_IMPORT_DESCRIPTOR impDesc;
    LPSTR pszName;
    DWORD dwLength;
    WORD wHint;

    // Headline
    printf("Import Info (class version %d.%d) by DEATH of Execution in 2002\n", PE_MAJVER, PE_MINVER);

    // Check number of arguments
    if (argc < 2) {
        printf("Usage: ImportInfo <pefile>\n");
        return(SUCCESS);
    }

    // Load the PE
    error = pe.Load(argv[1]);
    if (error != PE_NONE) {
        printf("Error: %s\n", CUtil::GetErrorString(error));
        return(FAILURE);
    }

    // Check if there is an Import table
    if (pe.m_ddImport.IsInitialized() == TRUE) {
        printf("Number of import descriptors: %ld\n", pe.m_ddImport.GetNumEntries());
        printf("Import descriptor information:\n");

        for (dwCount = 0; dwCount < pe.m_ddImport.GetNumEntries(); dwCount++) {
            printf("\tDescriptor #%ld:\n", dwCount + 1);
            pe.m_ddImport.GetEntry(dwCount, &impDesc);

            // Show imported DLL name
            if (impDesc.Name) {
                dwLength = pe.m_ddImport.GetDLLName(dwCount, NULL);
                if (dwLength) {
                    pszName = new char [dwLength + 1];
                    if (pszName) {
                        pe.m_ddImport.GetDLLName(dwCount, pszName);
                        printf("\t\tDLL Name: %s\n", pszName);
                        delete [] pszName;
                    }
                }
            }

            if (impDesc.TimeDateStamp) {
                printf("\t\tTime/Date stamp pre-snapped: %s", CUtil::GetDateString(impDesc.TimeDateStamp));
            }

            // Show imported functions
            printf("\t\tNumber of functions imported: %ld\n", pe.m_ddImport.GetNumFunctions(dwCount));

            printf("\t\t%-5s %-50s %-4s\n", "Index", "Ordinal/Name", "Hint");

            for (dwCount2 = 0; dwCount2 < pe.m_ddImport.GetNumFunctions(dwCount); dwCount2++) {
                if (pe.m_ddImport.IsOrdinal(dwCount, dwCount2) == TRUE) {
                    printf("\t\t%5ld #%08lX\n", dwCount2, pe.m_ddImport.GetOrdinal(dwCount, dwCount2));
                } else {
                    dwLength = pe.m_ddImport.GetFuncName(dwCount, dwCount2, NULL, NULL);
                    if (dwLength) {
                        pszName = new char [dwLength + 1];
                        if (pszName) {
                            pe.m_ddImport.GetFuncName(dwCount, dwCount2, pszName, &wHint);
                            printf("\t\t%5ld %-50s %04lX\n", dwCount2, pszName, wHint);
                            delete [] pszName;
                        }
                    }
                }
            }

            printf("\n");
        }
    } else {
        printf("Error: No import table\n");
        return(FAILURE);
    }

    return(SUCCESS);
}

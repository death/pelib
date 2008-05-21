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
    DWORD dwRVA;
    WORD wOrdinal;
    char *pszText;
    DWORD dwLength;
    IMAGE_EXPORT_DIRECTORY dirExp;

    // Headline
    printf("Export Info (class version %d.%d) by DEATH of Execution in 2002\n", PE_MAJVER, PE_MINVER);

    // Check number of arguments
    if (argc < 2) {
        printf("Usage: ExportInfo <pefile>\n");
        return(SUCCESS);
    }

    // Load the PE
    error = pe.Load(argv[1]);
    if (error != PE_NONE) {
		printf("Error: %s\n", CUtil::GetErrorString(error));
        return(FAILURE);
    }

    // Check if there is an export table
    if (pe.m_ddExport.IsInitialized()) {
        pe.m_ddExport.GetTable(&dirExp);

        printf("Export data found\n");
        printf("\tCharacteristics: %08lX\n", dirExp.Characteristics);
        printf("\tTime/Date stamp: %s", CUtil::GetDateString(dirExp.TimeDateStamp));
        printf("\tMajor/Minor version: %d.%d\n", dirExp.MajorVersion, dirExp.MinorVersion);
        printf("\tName RVA: %08lX\n", dirExp.Name);
        printf("\tOrdinal base: %08lX\n", dirExp.Base);
        printf("\tNumber of Export Address Table entries: %ld\n", dirExp.NumberOfFunctions);
        printf("\tNumber of name entries: %ld\n", dirExp.NumberOfNames);
        printf("\tAddress Table RVA: %08lX\n", (DWORD )dirExp.AddressOfFunctions);
        printf("\tName Table RVA: %08lX\n", (DWORD )dirExp.AddressOfNames);
        printf("\tOrdinals Table RVA: %08lX\n", (DWORD )dirExp.AddressOfNameOrdinals);
        printf("\n");

        // Get DLL name
        dwLength = pe.m_ddExport.GetDLLName(NULL);
        if (dwLength) {
            pszText = new char [dwLength + 1];
            if (pszText) {
                pe.m_ddExport.GetDLLName(pszText);
                printf("\tDLL name: %s\n", pszText);
                delete [] pszText;
                pszText = NULL;
            }
        }

        printf("\n");

        // Manipulate Export Address Table
        if (pe.m_ddExport.IsEATInitialized()) {
            printf("\tExport Address Table (EAT)\n");
            printf("\t\t%-10s %-10s %-10s\n", "RVA", "Ordinal", "Name");
            pszText = NULL;
            for (wOrdinal = (WORD )dirExp.Base; wOrdinal < (dirExp.NumberOfFunctions + dirExp.Base); wOrdinal++) {
                dwRVA = pe.m_ddExport.GetExportAddressByIndex(wOrdinal - dirExp.Base);
                dwLength = pe.m_ddExport.GetNameByOrdinal(wOrdinal, NULL);
                if (dwLength) {
                    pszText = new char [dwLength + 1];
                    if (pszText) {
                        pe.m_ddExport.GetNameByOrdinal(wOrdinal, pszText);
                    }
                }
                printf("\t\t%08lX   %04X       %s\n", dwRVA, wOrdinal, (pszText == NULL) ? "" : pszText);
                if (pszText) {
                    delete [] pszText;
                    pszText = NULL;
                }
            }
            printf("\n");
        } else {
            printf("No EAT found\n");
        }
    } else {
        printf("No export data found\n");
    }

    return(SUCCESS);
}

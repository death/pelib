// Util.cpp: implementation of the CUtil class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Util.h"

using namespace std;

namespace PE {

const DWORD CUtil::c_dwNumDebugTypes = 9;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// Get string by error value
LPCSTR CUtil::GetErrorString(PE_ERROR error)
{
    static const LPSTR pErrorStrings[PE_LAST] = {
        "None",
        "Open",
        "Mapping",
        "Map view",
        "Size",
        "Close",
        "Not MZ",
        "Not PE",
        "No file loaded",
        "NULL pointer",
        "Memory access"
    };

    if (error >= PE_LAST)    // Sanity check
        return(pErrorStrings[PE_NONE]);

    return(pErrorStrings[error]);
}

// Get debug data type string
LPCSTR CUtil::GetDebugTypeString(DWORD dwType)
{
    static const LPSTR lpTypes[c_dwNumDebugTypes] = {
        "Unknown",
        "COFF",
        "CodeView",
        "FPO",
        "Misc",
        "Exception",
        "Fixup",
        "Object Map To Src",
        "Object Map From Src"
    };

    if (dwType >= c_dwNumDebugTypes)
        return(lpTypes[0]);

    return(lpTypes[dwType]);
}

// Convert date stamp to string
LPCSTR CUtil::GetDateString(DWORD dwStamp)
{
    return(ctime((time_t *)&dwStamp));
}

// Get number of debug types supported by GetDebugTypeString
DWORD CUtil::GetNumDebugTypes(void)
{
    return(c_dwNumDebugTypes);
}

// Align a value to another value
DWORD CUtil::Align(DWORD dwVal, DWORD dwAlignVal)
{
    dwAlignVal--;
    return((dwVal + dwAlignVal) & ~dwAlignVal);
}

LPCSTR CUtil::GetResourceName(DWORD dwID)
{
    static const LPSTR pResNames[] = {
        "Unknown",                      // 0
        "Cursor",                       // 1
        "Bitmap",                       // 2
        "Icon",                         // 3
        "Menu",                         // 4
        "Dialog",                       // 5
        "String table",                 // 6
        "Font directory",               // 7
        "Font",                         // 8
        "Accelerators",                 // 9
        "Unformatted resource data",    // 10
        "Message table",                // 11
        "Group cursor",                 // 12
        "Unknown",                      // 13
        "Group icon",                   // 14
        "Unknown",                      // 15
        "Version information"           // 16
    };

    if (dwID >= 17)
        return(pResNames[0]);

    return(pResNames[dwID]);
}

string CUtil::GetResourceString(LPBYTE pData)
{
    short wLength = *(short *)pData;
    pData += sizeof(short);

    char *psz = new char [wLength + 1];
    string str;

    if (psz) {
        for (short i = 0; i < wLength; i++) {
            psz[i] = pData[i * 2];
        }
        psz[i] = '\0';
        
        str = psz;

        delete [] psz;
    }

    return(str);
}

}
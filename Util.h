// Util.h: interface for the CUtil class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "defs.h"

namespace PE {

class CUtil
{
private:
    static const DWORD c_dwNumDebugTypes;

    // Make constructor private so no objects can be created from CUtil
    CUtil();

public:
    static LPCSTR GetDebugTypeString(DWORD dwType);
    static LPCSTR GetErrorString(PE_ERROR error);
    static LPCSTR GetDateString(DWORD dwStamp);
    // Get number of debug types supported by GetDebugTypeString
    static DWORD GetNumDebugTypes(void);
    // Align value to another value
    static DWORD Align(DWORD dwVal, DWORD dwAlignVal);
    static LPCSTR GetResourceName(DWORD dwID);
    static std::string GetResourceString(LPBYTE pData);
};

}
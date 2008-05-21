// DataDirectory.h: interface for the CDataDirectory class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "defs.h"

namespace PE {

class CPortableExecutable;

class CDataDirectory
{
public:
    BOOL IsInitialized(void) const;
    virtual void Assign(LPBYTE pData, DWORD dwSize);
    CDataDirectory(CPortableExecutable *ppeOwner);
    virtual ~CDataDirectory();

protected:
    LPBYTE m_pData;
    DWORD m_dwSize;

    CPortableExecutable * m_ppeOwner;
};

}
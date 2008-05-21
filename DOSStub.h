// DOSStub.h: interface for the CDOSStub class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"

namespace PE {

class CDOSStub
{
public:
    friend class CPortableExecutable;

    void Set(LPBYTE pData, DWORD dwSize);
    DWORD Get(LPBYTE pData) const;
    CDOSStub();
    virtual ~CDOSStub();

protected:
    DWORD m_dwSize;
    LPBYTE m_pData;

protected: // Functions
    void Cleanup(void);
};

}
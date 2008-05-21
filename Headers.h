// Headers.h: interface for the CHeaders class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"

namespace PE {

class CHeaders
{
public:
    DWORD GetHeaderSize(void) const;
    void SetHeaderSize(DWORD dwSize);
    void GetDos(IMAGE_DOS_HEADER &hdrDos) const;
    void GetNt(IMAGE_NT_HEADERS &hdrNt) const;
    void FixSizeOfHeaders(void);
    DWORD SectionAlignment(DWORD dwSize) const;
    DWORD FileAlignment(DWORD dwSize) const;
    void SetNt(IMAGE_NT_HEADERS& hdrNt);
    void SetDos(IMAGE_DOS_HEADER& hdrDos);
    CHeaders();
    virtual ~CHeaders();

    friend class CPortableExecutable;

protected:
    DWORD m_dwDOSHeaderSize;
    // Member variables
    IMAGE_DOS_HEADER m_hdrDos;
    IMAGE_NT_HEADERS m_hdrNt;
};

}
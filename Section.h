// Section.h: interface for the CSection class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"

namespace PE {

class CSection
{
public:
    CSection(CSection const &rSection);
    void Resize(DWORD dwNewSize);
    DWORD GetData(LPBYTE pData) const;
    void GetHeader(IMAGE_SECTION_HEADER &hdr) const;
    BOOL WithinFO(DWORD dwFO) const;
    BOOL WithinRVA(DWORD dwRVA) const;
    void SetData(LPBYTE pData, DWORD cbData);
    void SetHeader(IMAGE_SECTION_HEADER &hdrNew);
    CSection();
    virtual ~CSection();

    friend class CPortableExecutable;

protected: // Functions
    void Cleanup(void);

protected:
    DWORD m_dwSize;
    LPBYTE m_pData;
    IMAGE_SECTION_HEADER m_hdr;
};

}
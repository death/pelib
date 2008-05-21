#pragma once

#include "stdafx.h"

namespace PE {

class CResourceEntry
{
public:
    CResourceEntry(LPBYTE pData, LPBYTE pBase);
    virtual ~CResourceEntry(void);

    void GetInfo(IMAGE_RESOURCE_DATA_ENTRY *prde) const;
    void GetDirInfo(IMAGE_RESOURCE_DIRECTORY_ENTRY *prde) const;

private:
    LPBYTE GetDataEntry(void) const;

private:
    LPBYTE m_pBase;
    IMAGE_RESOURCE_DATA_ENTRY m_rde;
    IMAGE_RESOURCE_DIRECTORY_ENTRY m_rdire;
};

}
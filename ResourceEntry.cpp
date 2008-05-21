#include "stdafx.h"
#include "resourceentry.h"

namespace PE {

CResourceEntry::CResourceEntry(LPBYTE pData, LPBYTE pBase)
: m_pBase(pBase)
{
    if (IsBadReadPtr(pData, sizeof(IMAGE_RESOURCE_DIRECTORY)) == FALSE) {
        CopyMemory(&m_rdire, pData, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
    }

    ZeroMemory(&m_rde, sizeof(IMAGE_RESOURCE_DATA_ENTRY));

    if (m_rdire.DataIsDirectory == 0) {
        LPBYTE pDataEntry = GetDataEntry();
        if (IsBadReadPtr(pDataEntry, sizeof(IMAGE_RESOURCE_DATA_ENTRY)) == FALSE) {
            CopyMemory(&m_rde, pDataEntry, sizeof(IMAGE_RESOURCE_DATA_ENTRY));
        }
    }
}

CResourceEntry::~CResourceEntry(void)
{
}

void CResourceEntry::GetInfo(IMAGE_RESOURCE_DATA_ENTRY *prde) const
{
    if (IsBadWritePtr(prde, sizeof(IMAGE_RESOURCE_DATA_ENTRY)) == FALSE) {
        CopyMemory(prde, &m_rde, sizeof(IMAGE_RESOURCE_DATA_ENTRY));
    }
}

void CResourceEntry::GetDirInfo(IMAGE_RESOURCE_DIRECTORY_ENTRY *prde) const
{
    if (IsBadWritePtr(prde, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY)) == FALSE) {
        CopyMemory(prde, &m_rdire, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
    }
}

LPBYTE CResourceEntry::GetDataEntry(void) const
{
    LPBYTE pData = &m_pBase[m_rdire.OffsetToData];
    return(pData);
}

}
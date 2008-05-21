// DD_Export.h: interface for the CDD_Export class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "DataDirectory.h"

namespace PE {

class CDD_Export : public CDataDirectory
{
public:
    void SetTable(PIMAGE_EXPORT_DIRECTORY pdirExp);
    void GetTable(PIMAGE_EXPORT_DIRECTORY pdirExp) const;
    DWORD GetOrdinalByName(LPSTR pName) const;
    BOOL IsEATInitialized(void) const;
    void SetExportAddressByIndex(DWORD dwIndex, DWORD dwAddr);
    DWORD GetExportAddressByIndex(DWORD dwIndex) const;
    DWORD GetNameByOrdinal(DWORD dwOrdinal, LPSTR pName) const;
    DWORD GetNameByIndex(DWORD dwIndex, LPSTR pName) const;
    DWORD GetDLLName(LPSTR pName) const;
    DWORD GetIndexByOrdinal(DWORD dwOrdinal) const;
    void Assign(LPBYTE pData, DWORD dwSize);
    CDD_Export(CPortableExecutable *ppeOwner);
    virtual ~CDD_Export();

protected:
    PIMAGE_EXPORT_DIRECTORY m_ped;
    LPDWORD m_pdwEAT;
    LPDWORD m_pdwENT;
    LPWORD m_pwEOT;

};

}
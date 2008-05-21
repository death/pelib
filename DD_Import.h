// DD_Import.h: interface for the CDD_Import class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "DataDirectory.h"

namespace PE {

class CDD_Import : public CDataDirectory
{
public:
    DWORD GetFuncName(DWORD dwDescIndex, DWORD dwFuncIndex, LPSTR pszName, LPWORD pwHint = NULL) const;
    DWORD GetOrdinal(DWORD dwDescIndex, DWORD dwFuncIndex) const;
    BOOL IsOrdinal(DWORD dwDescIndex, DWORD dwFuncIndex) const;
    DWORD GetNumFunctions(DWORD dwDescIndex) const;
    DWORD GetDLLName(DWORD dwIndex, LPSTR pszName) const;
    void GetEntry(DWORD dwIndex, PIMAGE_IMPORT_DESCRIPTOR pid) const;
    DWORD GetNumEntries(void) const;
    void Assign(LPBYTE pData, DWORD dwSize);
    CDD_Import(CPortableExecutable *ppeOwner);
    virtual ~CDD_Import();

protected: // Definitions
    typedef std::list<PIMAGE_IMPORT_DESCRIPTOR>::iterator dirImpIterator;
    typedef std::list<PIMAGE_IMPORT_DESCRIPTOR>::const_iterator dirCImpIterator;
    typedef std::list<PIMAGE_IMPORT_DESCRIPTOR> dirImpList;

protected:
    void Cleanup(void);
    PIMAGE_THUNK_DATA GetFirstThunk(DWORD dwDescIndex) const;
    dirImpList m_lstEntries;
};

}

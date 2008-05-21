// DD_Debug.h: interface for the CDD_Debug class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "DataDirectory.h"

namespace PE {

class CDD_Debug : public CDataDirectory
{
public:
    void SetTable(DWORD dwIndex, PIMAGE_DEBUG_DIRECTORY pdirDeb);
    void GetTable(DWORD dwIndex, PIMAGE_DEBUG_DIRECTORY pdirDeb) const;
    void Assign(LPBYTE pData, DWORD dwSize);
    DWORD GetNumTables(void) const;
    CDD_Debug(CPortableExecutable *ppeOwner);
    virtual ~CDD_Debug();

protected: // Definitions
    typedef std::list<PIMAGE_DEBUG_DIRECTORY>::iterator dirDebIterator;
    typedef std::list<PIMAGE_DEBUG_DIRECTORY> dirDebList;
    typedef std::list<PIMAGE_DEBUG_DIRECTORY>::const_iterator dirDebCIterator;

protected:
    dirDebCIterator GetTableCIterator(DWORD dwIndex) const;
    dirDebIterator GetTableIterator(DWORD dwIndex);
    void Cleanup(void);

    // Protected variables
    dirDebList m_lstDeb;
};

}
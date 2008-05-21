// DataDirectory.cpp: implementation of the CDataDirectory class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "DataDirectory.h"

namespace PE {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// Constructor
CDataDirectory::CDataDirectory(CPortableExecutable *ppeOwner)
{
    m_ppeOwner = ppeOwner;
}

// Destructor
CDataDirectory::~CDataDirectory()
{

}

// Assign a data directory entry
void CDataDirectory::Assign(LPBYTE pData, DWORD dwSize)
{
    m_pData = pData;
    m_dwSize = dwSize;
}

// Check if entry was initialized
BOOL CDataDirectory::IsInitialized(void) const
{
    if (m_dwSize)
        return(!IsBadReadPtr(m_pData, m_dwSize));

    return(FALSE);
}

}
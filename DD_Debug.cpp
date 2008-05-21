// DD_Debug.cpp: implementation of the CDD_Debug class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DD_Debug.h"
#include "PortableExecutable.h"

using namespace std;

namespace PE {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDD_Debug::CDD_Debug(CPortableExecutable *ppeOwner) : CDataDirectory(ppeOwner)
{

}

CDD_Debug::~CDD_Debug()
{
    Cleanup();
}

// Get number of debug directory tables
DWORD CDD_Debug::GetNumTables(void) const
{
    return(m_dwSize / sizeof(IMAGE_DEBUG_DIRECTORY));
}

// Assign a debug directory
void CDD_Debug::Assign(LPBYTE pData, DWORD dwSize)
{
    DWORD dwCount;
    PIMAGE_DEBUG_DIRECTORY pDebugDirectory;

    Cleanup();

    CDataDirectory::Assign(pData, dwSize);

    if (IsInitialized() == FALSE) return;

    for (dwCount = 0; dwCount < GetNumTables(); dwCount++) {
        pDebugDirectory = (PIMAGE_DEBUG_DIRECTORY )&pData[dwCount * sizeof(IMAGE_DEBUG_DIRECTORY)];
        m_lstDeb.push_back(pDebugDirectory);
    }
}

// Clean up
void CDD_Debug::Cleanup(void)
{
    if (m_lstDeb.empty() == FALSE)
        m_lstDeb.clear();
}

// Get debug table
void CDD_Debug::GetTable(DWORD dwIndex, PIMAGE_DEBUG_DIRECTORY pdirDeb) const
{
    dirDebCIterator i;

    i = GetTableCIterator(dwIndex);
    if ((i != NULL) && (IsBadWritePtr(pdirDeb, sizeof(IMAGE_DEBUG_DIRECTORY)) == FALSE))
        CopyMemory(pdirDeb, *i, sizeof(IMAGE_DEBUG_DIRECTORY));
}

// Set debug table
void CDD_Debug::SetTable(DWORD dwIndex, PIMAGE_DEBUG_DIRECTORY pdirDeb)
{
    dirDebIterator i;

    i = GetTableIterator(dwIndex);
    if ((i != NULL) && (IsBadReadPtr(pdirDeb, sizeof(IMAGE_DEBUG_DIRECTORY)) == FALSE))
        CopyMemory(*i, pdirDeb, sizeof(IMAGE_DEBUG_DIRECTORY));
}

// Get table iterator (constant)
CDD_Debug::dirDebCIterator CDD_Debug::GetTableCIterator(DWORD dwIndex) const
{
    dirDebCIterator i;

    i = NULL;

    if (m_lstDeb.empty() == FALSE && dwIndex < GetNumTables()) {
        i = m_lstDeb.begin();
        while (dwIndex--) i++;
    }

    return(i);
}

// Get table iterator
CDD_Debug::dirDebIterator CDD_Debug::GetTableIterator(DWORD dwIndex)
{
    dirDebIterator i;

    i = NULL;

    if (m_lstDeb.empty() == FALSE && dwIndex < GetNumTables()) {
        i = m_lstDeb.begin();
        while (dwIndex--) i++;
    }

    return(i);
}

}
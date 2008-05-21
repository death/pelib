#include "stdafx.h"
#include "dd_resource.h"

namespace PE {

CDD_Resource::CDD_Resource(CPortableExecutable *ppeOwner)
: CDataDirectory(ppeOwner),
  m_prd(0)
{
}

CDD_Resource::~CDD_Resource(void)
{
    if (m_prd) {
        delete m_prd;
        m_prd = 0;
    }
}

// Assign resource section
void CDD_Resource::Assign(LPBYTE pData, DWORD dwSize)
{
    CDataDirectory::Assign(pData, dwSize);

    if (IsInitialized() == FALSE) return;

    m_prd = new CResourceDirectory(pData, pData, TRUE);
}

// Get root resource directory
CResourceDirectory *CDD_Resource::GetRootDirectory(void) const
{
    return(m_prd);
}

}
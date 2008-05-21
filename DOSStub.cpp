// DOSStub.cpp: implementation of the CDOSStub class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DOSStub.h"

namespace PE {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDOSStub::CDOSStub()
{
    m_dwSize = 0L;
    m_pData = NULL;
}

CDOSStub::~CDOSStub()
{
    Cleanup();
}

// Get stub
DWORD CDOSStub::Get(LPBYTE pData) const
{
    if (m_dwSize && (IsBadWritePtr(pData, m_dwSize) == FALSE)) {
        CopyMemory(pData, m_pData, m_dwSize);
    }

    return(m_dwSize);
}

// Set stub
void CDOSStub::Set(LPBYTE pData, DWORD dwSize)
{
    Cleanup();

    if (dwSize) {
        if (IsBadReadPtr(pData, dwSize) == FALSE) {
            m_pData = new BYTE [dwSize];
    
            if (IsBadWritePtr(m_pData, dwSize) == FALSE)
                CopyMemory(m_pData, pData, dwSize);
            
            m_dwSize = dwSize;
        }
    }
}

// Clean up
void CDOSStub::Cleanup(void)
{
    if (m_pData) {
        delete [] m_pData;
        m_pData = NULL;
    }

    m_dwSize = 0;
}

}
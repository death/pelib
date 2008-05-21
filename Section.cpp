// Section.cpp: implementation of the CSection class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Section.h"

using namespace std;

namespace PE {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSection::CSection()
{
    m_pData = NULL;
    m_dwSize = 0;
    ZeroMemory(&m_hdr, sizeof(IMAGE_SECTION_HEADER));
}

CSection::~CSection()
{
    Cleanup();
}

// Copy constructor
CSection::CSection(const CSection &rSection)
{
    rSection.GetHeader(m_hdr);
    m_dwSize = rSection.GetData(NULL);

    if (m_dwSize) {
        m_pData = new BYTE [m_dwSize];
        rSection.GetData(m_pData);
    } else {
        m_pData = NULL;
    }
}

// Cleanup
void CSection::Cleanup(void)
{
    if (m_pData) {
        delete [] m_pData;
        m_pData = NULL;
    }

    m_dwSize = 0;
    ZeroMemory(&m_hdr, sizeof(IMAGE_SECTION_HEADER));
}

// Set section header
void CSection::SetHeader(IMAGE_SECTION_HEADER &hdrNew)
{
    CopyMemory(&m_hdr, &hdrNew, sizeof(IMAGE_SECTION_HEADER));
}

// Set section data
void CSection::SetData(LPBYTE pData, DWORD cbData)
{
    if (m_pData) {
        delete [] m_pData;
        m_pData = NULL;
    }

    m_dwSize = 0;

    if (cbData) {
        if (IsBadReadPtr(pData, cbData) == FALSE) {
            m_pData = new BYTE[cbData];

            if (m_pData) {
                CopyMemory(m_pData, pData, cbData);
                m_dwSize = cbData;
            }
        }
    }
}

// Check if Relative Virtual Address within section
BOOL CSection::WithinRVA(DWORD dwRVA) const
{
    if ((dwRVA >= m_hdr.VirtualAddress) && (dwRVA < (m_hdr.VirtualAddress + m_hdr.Misc.VirtualSize)))
        return (TRUE);

    return(FALSE);
}

// Check if File Offset within section
BOOL CSection::WithinFO(DWORD dwFO) const
{
    if ((dwFO >= m_hdr.PointerToRawData) && (dwFO < (m_hdr.PointerToRawData + m_hdr.SizeOfRawData)))
        return(TRUE);

    return(FALSE);
}

// Get header
void CSection::GetHeader(IMAGE_SECTION_HEADER &hdr) const
{
    CopyMemory(&hdr, &m_hdr, sizeof(IMAGE_SECTION_HEADER));
}

// Get data
DWORD CSection::GetData(LPBYTE pData) const
{
    if (m_dwSize) {
        if (IsBadWritePtr(pData, m_dwSize) == FALSE) {
            if (IsBadReadPtr(m_pData, m_dwSize) == FALSE) {
                CopyMemory(pData, m_pData, m_dwSize);
            }
        }
    }

    return(m_dwSize);
}

// Resize section
void CSection::Resize(DWORD dwNewSize)
{
    LPBYTE pNewData;

    if (dwNewSize) {
        if (IsBadReadPtr(m_pData, m_dwSize) == FALSE) {
            pNewData = new BYTE [dwNewSize];
            if (pNewData) {
                if (dwNewSize > m_dwSize) {
                    // New size more than original
                    ZeroMemory(pNewData, dwNewSize);
                    CopyMemory(pNewData, m_pData, m_dwSize);
                } else {
                    // New size less than original
                    CopyMemory(pNewData, m_pData, dwNewSize);
                }
                SetData(pNewData, dwNewSize);
            }
            if (pNewData) delete [] pNewData;
        }
    }
}

}
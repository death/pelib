// Headers.cpp: implementation of the CHeaders class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Headers.h"
#include "Util.h"

namespace PE {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHeaders::CHeaders()
{
    ZeroMemory(&m_hdrDos, sizeof(IMAGE_DOS_HEADER));
    ZeroMemory(&m_hdrNt, sizeof(IMAGE_NT_HEADERS));
    m_dwDOSHeaderSize = 0;
}

CHeaders::~CHeaders()
{

}

// Set DOS header
void CHeaders::SetDos(IMAGE_DOS_HEADER &hdrDos)
{
    CopyMemory(&m_hdrDos, &hdrDos, sizeof(IMAGE_DOS_HEADER));
}

// Set NT header
void CHeaders::SetNt(IMAGE_NT_HEADERS &hdrNt)
{
    CopyMemory(&m_hdrNt, &hdrNt, sizeof(IMAGE_NT_HEADERS));
}

// Return size aligned according to FileAlignment field
DWORD CHeaders::FileAlignment(DWORD dwSize) const
{
    return(CUtil::Align(dwSize, m_hdrNt.OptionalHeader.FileAlignment));
}

// Return size aligned according to SectionAlignment field
DWORD CHeaders::SectionAlignment(DWORD dwSize) const
{
    return(CUtil::Align(dwSize, m_hdrNt.OptionalHeader.SectionAlignment));
}

// Fix SizeOfHeaders field
void CHeaders::FixSizeOfHeaders(void)
{
    DWORD dwSize;

    dwSize = m_hdrDos.e_lfanew;
    dwSize += sizeof(IMAGE_NT_HEADERS);
    dwSize += sizeof(IMAGE_SECTION_HEADER) * m_hdrNt.FileHeader.NumberOfSections;
    dwSize = FileAlignment(dwSize);

    m_hdrNt.OptionalHeader.SizeOfHeaders = dwSize;
}

// Get NT header
void CHeaders::GetNt(IMAGE_NT_HEADERS &hdrNt) const
{
    if (IsBadWritePtr(&hdrNt, sizeof(IMAGE_NT_HEADERS)) == FALSE)
        CopyMemory(&hdrNt, &m_hdrNt, sizeof(IMAGE_NT_HEADERS));
}

// Get DOS header
void CHeaders::GetDos(IMAGE_DOS_HEADER &hdrDos) const
{
    if (IsBadWritePtr(&hdrDos, sizeof(IMAGE_DOS_HEADER)) == FALSE)
        CopyMemory(&hdrDos, &m_hdrDos, sizeof(IMAGE_DOS_HEADER));
}

// Get stub header size
DWORD CHeaders::GetHeaderSize(void) const
{
    return(m_dwDOSHeaderSize);
}

// Set stub header size
void CHeaders::SetHeaderSize(DWORD dwSize)
{
    m_dwDOSHeaderSize = dwSize;
}

}
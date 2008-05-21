// DD_Export.cpp: implementation of the CDD_Export class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DD_Export.h"
#include "PortableExecutable.h"

namespace PE {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDD_Export::CDD_Export(CPortableExecutable *ppeOwner) : CDataDirectory(ppeOwner)
{

}

CDD_Export::~CDD_Export()
{

}

// Assign export directory
void CDD_Export::Assign(LPBYTE pData, DWORD dwSize)
{
    CDataDirectory::Assign(pData, dwSize);

    if (IsInitialized() == FALSE) return;

    // Export directory
    m_ped = (PIMAGE_EXPORT_DIRECTORY )pData;

    // Export Address Table (EAT)
    if (m_ped->AddressOfFunctions) {
        m_pdwEAT = (LPDWORD )m_ppeOwner->GetDataAtRVA((DWORD )m_ped->AddressOfFunctions);
    } else {
        m_pdwEAT = NULL;
    }

    // Export Name Table (ENT)
    if (m_ped->AddressOfNames) {
        m_pdwENT = (LPDWORD )m_ppeOwner->GetDataAtRVA((DWORD )m_ped->AddressOfNames);
    } else {
        m_pdwENT = NULL;
    }

    // Export Ordinal Table (EOT)
    if (m_ped->AddressOfNameOrdinals) {
        m_pwEOT = (LPWORD )m_ppeOwner->GetDataAtRVA((DWORD )m_ped->AddressOfNameOrdinals);
    } else {
        m_pwEOT = NULL;
    }
}

// Get export name by index
DWORD CDD_Export::GetNameByIndex(DWORD dwIndex, LPSTR pName) const
{
    LPBYTE pData;
    DWORD dwLength;

    dwLength = 0;

    if (m_pdwENT && dwIndex < m_ped->NumberOfNames) {
        pData = m_ppeOwner->GetDataAtRVA(m_pdwENT[dwIndex]);

        if (IsBadReadPtr(pData, 1) == TRUE) return(0);

        dwLength = lstrlen((LPSTR )pData);
        if (IsBadWritePtr(pName, dwLength + 1) == FALSE)
            lstrcpy(pName, (LPSTR )pData);
    }

    return(dwLength);
}

// Get index by ordinal
DWORD CDD_Export::GetIndexByOrdinal(DWORD dwOrdinal) const
{
    DWORD dwIndex;

    if (m_pwEOT && (dwOrdinal - m_ped->Base) < m_ped->NumberOfFunctions) {
        for (dwIndex = 0; dwIndex < m_ped->NumberOfFunctions; dwIndex++) {
            if (m_pwEOT[dwIndex] == dwOrdinal - m_ped->Base)
                return(dwIndex);
        }
    }
    return(PE_INVALID);
}

// Get export name by ordinal
DWORD CDD_Export::GetNameByOrdinal(DWORD dwOrdinal, LPSTR pName) const
{
    DWORD dwIndex;

    dwIndex = GetIndexByOrdinal(dwOrdinal);
    return(GetNameByIndex(dwIndex, pName));
}

// Get DLL name
DWORD CDD_Export::GetDLLName(LPSTR pName) const
{
    LPBYTE pData;
    DWORD dwLength;

    dwLength = 0;

    if (m_ped->Name) {
        pData = m_ppeOwner->GetDataAtRVA(m_ped->Name);

        if (IsBadReadPtr(pData, 1) == TRUE) return(0);

        dwLength = lstrlen((LPSTR )pData);
        if (IsBadWritePtr(pName, dwLength + 1) == FALSE)
            lstrcpy(pName, (LPSTR )pData);
    }

    return(dwLength);
}

// Get export address by index
DWORD CDD_Export::GetExportAddressByIndex(DWORD dwIndex) const
{
    if (IsEATInitialized() && dwIndex < m_ped->NumberOfFunctions) {
        return(m_pdwEAT[dwIndex]);
    }

    return(0L);
}

// Set export address by index
void CDD_Export::SetExportAddressByIndex(DWORD dwIndex, DWORD dwAddr)
{
    if (IsEATInitialized() && dwIndex < m_ped->NumberOfFunctions) {
        m_pdwEAT[dwIndex] = dwAddr;
    }
}

// Check if EAT initialized
BOOL CDD_Export::IsEATInitialized(void) const
{
    if (m_pdwEAT)
        return(TRUE);
    return(FALSE);
}

// Get ordinal by function name
DWORD CDD_Export::GetOrdinalByName(LPSTR pName) const
{
    DWORD dwOrdinal;
    LPSTR pTemp;
    DWORD dwLength;

    if (m_pdwENT && pName) {
        for (dwOrdinal = m_ped->Base; dwOrdinal < (m_ped->Base + m_ped->NumberOfFunctions); dwOrdinal++) {
            dwLength = GetNameByOrdinal(dwOrdinal, NULL);
            if (dwLength) {
                pTemp = new char [dwLength + 1];
                if (pTemp) {
                    GetNameByOrdinal(dwOrdinal, pTemp);
                    if (lstrcmp(pTemp, pName) == 0) {
                        delete [] pTemp;
                        return(dwOrdinal);
                    }
                    delete [] pTemp;
                }
            }
        }
    }

    return(PE_INVALID);
}

// Get export directory table
void CDD_Export::GetTable(PIMAGE_EXPORT_DIRECTORY pdirExp) const
{
    if (IsBadWritePtr(pdirExp, sizeof(IMAGE_EXPORT_DIRECTORY)) == FALSE)
        CopyMemory(pdirExp, m_ped, sizeof(IMAGE_EXPORT_DIRECTORY));
}

// Set export directory table
void CDD_Export::SetTable(PIMAGE_EXPORT_DIRECTORY pdirExp)
{
    if (IsBadReadPtr(pdirExp, sizeof(IMAGE_EXPORT_DIRECTORY)) == FALSE)
        CopyMemory(m_ped, pdirExp, sizeof(IMAGE_EXPORT_DIRECTORY));
}

}
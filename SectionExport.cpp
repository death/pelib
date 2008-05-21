// SectionExport.cpp: implementation of the CSectionExport class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SectionExport.h"
#include "PortableExecutable.h"

using namespace std;

namespace PE {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSectionExport::CSectionExport() 
{
    m_pDLLName = NULL;
    m_ppe = NULL;
    m_wLastOrdinal = 0;
}

CSectionExport::~CSectionExport()
{
    Cleanup();
}

// Create an export directory
void CSectionExport::Create(LPSTR pSectionName, LPSTR pDLLName)
{
    IMAGE_SECTION_HEADER hdrSection;

    // Cleanup
    Cleanup();

    // Zero all fields
    ZeroMemory(&m_imgExport, sizeof(IMAGE_EXPORT_DIRECTORY));

    // Set section name and characteristics
    if (IsBadReadPtr(pSectionName, 1) == FALSE) {
        if (lstrlen(pSectionName) >= 8)
            lstrcpyn((LPSTR )m_hdr.Name, pSectionName, 8);
        else
            lstrcpy((LPSTR )m_hdr.Name, pSectionName);
    }

    hdrSection.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

    m_imgExport.Base = 0xFFFF; // Largest base possible
    m_imgExport.TimeDateStamp = time(NULL);

    // Initialize DLL name
    if (m_pDLLName) {
        delete [] m_pDLLName;
        m_pDLLName = NULL;
    }

    if (IsBadReadPtr(pDLLName, 1) == FALSE) {
        m_pDLLName = new char [lstrlen(pDLLName) + 1];
    
        if (m_pDLLName) {
            lstrcpy(m_pDLLName, pDLLName);
        }
    } else {
        // NULL byte
        m_pDLLName = new char[1];
        if (m_pDLLName)
            *m_pDLLName = '\0';
    }
}

// Add an export
void CSectionExport::Add(LPSTR pFuncName, WORD wOrdinal, DWORD dwAddress)
{
    EXPORT_ENTRY ee;

    ee.wOrdinal = wOrdinal;
    ee.dwAddress = dwAddress;
    ee.pName = NULL;

    if (IsBadReadPtr(pFuncName, 1) == FALSE) {
        ee.pName = new char [lstrlen(pFuncName) + 1];

        if (ee.pName) {
            lstrcpy(ee.pName, pFuncName);
            m_imgExport.NumberOfNames++;
        }
    }

    m_lstExports.push_back(ee);
    m_imgExport.NumberOfFunctions++;

    // Fix base
    if (wOrdinal < m_imgExport.Base)
        m_imgExport.Base = wOrdinal;

    // Fix last ordinal
    if (m_wLastOrdinal < wOrdinal)
        m_wLastOrdinal = wOrdinal;
}

// Update section data
void CSectionExport::Update(void)
{
    LPBYTE pData;
    LPBYTE pCurrent;
    DWORD dwSize;
    IMAGE_SECTION_HEADER hdrSection;
    expIterator i;
    LPDWORD pdwAddresses;
    IMAGE_NT_HEADERS hdr;
    DWORD dwUnalignedSize;
    DWORD dwCount;

    if (m_ppe == NULL || m_pAttached == NULL) return; // Need to be attached first

    if (m_lstExports.empty() == TRUE || m_pDLLName == NULL) {
        // Need export list and DLL name, not found, so zero export entry and create dummy data for section
        m_ppe->m_Headers.GetNt(hdr);
        hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = 0;
        hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = 0;
        m_ppe->m_Headers.SetNt(hdr);

        dwSize = m_ppe->m_Headers.SectionAlignment(1);
        pData = new BYTE [dwSize];

        if (pData) {

            ZeroMemory(pData, dwSize);
            m_pAttached->SetData(pData, dwSize);

            delete [] pData;

        }

        m_ppe->m_ddExport.Assign(NULL, 0);
        return;
    }

    Attach(m_ppe); // Reattach
    if (m_pAttached == NULL) return;

    pdwAddresses = new DWORD [m_imgExport.NumberOfNames];
    if (pdwAddresses == NULL) return;

    dwSize = sizeof(IMAGE_EXPORT_DIRECTORY);
    dwSize += m_imgExport.NumberOfFunctions * sizeof(DWORD); // For Export Address
    dwSize += m_imgExport.NumberOfNames * sizeof(WORD); // Ordinals
    dwSize += m_imgExport.NumberOfNames * sizeof(DWORD); // Name pointers
    dwSize += GetTotalExportNamesLength();  // Export names length
    dwSize += lstrlen(m_pDLLName) + 1; // DLL name length
    dwUnalignedSize = dwSize;
    dwSize = m_ppe->m_Headers.SectionAlignment(dwSize);

    pData = new BYTE [dwSize];

    if (pData) {
        // Zero data
        ZeroMemory(pData, dwSize);

        // Set directory information
        m_pAttached->GetHeader(hdrSection);

        // Leave space for export directory
        pCurrent = pData;
        pCurrent +=  sizeof(IMAGE_EXPORT_DIRECTORY);

        SortByOrdinal();

        // Right after the data directory, there is the EAT (export address table)
        m_imgExport.AddressOfFunctions = hdrSection.VirtualAddress + (pCurrent - pData);

        for (i = m_lstExports.begin(); i != m_lstExports.end(); i++) {
            *(LPDWORD )pCurrent = (*i).dwAddress;
            pCurrent += sizeof(DWORD);
        }

        SortByName();

        // The Ordinal table
        m_imgExport.AddressOfNameOrdinals = hdrSection.VirtualAddress + (pCurrent - pData);

        for (i = m_lstExports.begin(); i != m_lstExports.end(); i++) {
            if ((*i).pName) {
                *(LPWORD )pCurrent = (WORD )((*i).wOrdinal - m_imgExport.Base);
                pCurrent += sizeof(WORD);
            }
        }

        // Now come the names themselves
        dwCount = 0;
        for (i = m_lstExports.begin(); i != m_lstExports.end(); i++) {
            if ((*i).pName) {
                pdwAddresses[dwCount] = hdrSection.VirtualAddress + (pCurrent - pData);
                lstrcpy((LPSTR )pCurrent, (LPSTR )(*i).pName);
                pCurrent += lstrlen((*i).pName) + 1;
                dwCount++;
            }
        }

        // Now comes the export name pointer table
        m_imgExport.AddressOfNames = hdrSection.VirtualAddress + (pCurrent - pData);

        dwCount = 0;
        for (i = m_lstExports.begin(); i != m_lstExports.end(); i++) {
            if ((*i).pName) {
                *(LPDWORD )pCurrent = pdwAddresses[dwCount];
                pCurrent += sizeof(DWORD);
                dwCount++;
            }
        }

        // Finally, the DLL Name
        m_imgExport.Name = hdrSection.VirtualAddress + (pCurrent - pData);
        lstrcpy((LPSTR )pCurrent, m_pDLLName);
        pCurrent += lstrlen(m_pDLLName) + 1;

        // Update sizes
        hdrSection.SizeOfRawData = dwSize;
        hdrSection.Misc.VirtualSize = dwSize;
        m_pAttached->SetHeader(hdrSection);

        // Copy updated export directory
        CopyMemory(pData, &m_imgExport, sizeof(IMAGE_EXPORT_DIRECTORY));

        // Update export directory size in NT headers and size of initialized data
        m_ppe->m_Headers.GetNt(hdr);
        hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = dwUnalignedSize;
        hdr.OptionalHeader.SizeOfInitializedData += dwSize;
        m_ppe->m_Headers.SetNt(hdr);

        // Update size of image
        m_ppe->FixImageSize();

        m_pAttached->SetData(pData, dwSize);
        delete [] pData;

        // Reinitialize the m_ddExport class in CPortableExecutable class
        m_ppe->m_ddExport.Assign(m_ppe->GetDataAtRVA(hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), dwUnalignedSize);
    }

    delete [] pdwAddresses;
}

// Attach to a PE
void CSectionExport::Attach(CPortableExecutable *ppe)
{
    IMAGE_NT_HEADERS hdr;
    IMAGE_SECTION_HEADER hdrSection;
    char szName[16];

    m_ppe = ppe;

    // Copy section name
    ZeroMemory(szName, sizeof(szName));
    lstrcpyn(szName, (LPSTR )m_hdr.Name, 8);

    // Get section
    m_pAttached = ppe->GetSectionByName(szName);

    if (m_pAttached == NULL) {
        CSection sct;
        sct.SetHeader(m_hdr);

        // Add section
        ppe->AddSection(&sct);
        m_pAttached = ppe->GetLastSectionInFile(); // The newly added section

    }

    if (m_pAttached) {
        // Change data directory address
        ppe->m_Headers.GetNt(hdr);
        m_pAttached->GetHeader(hdrSection);
        hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = hdrSection.VirtualAddress;
        ppe->m_Headers.SetNt(hdr);
    }
}

// Sort entries by function name
void CSectionExport::SortByName(void)
{
    expIterator i;
    expIterator j;
    EXPORT_ENTRY ee;

    for (i = m_lstExports.begin(); i != m_lstExports.end(); i++) {
        for (j = i; j != m_lstExports.end(); j++) {
            if ((*i).pName && (*j).pName) {
                if (NameCompare((*i).pName, (*j).pName) < 0) {
                    /* Swap */
                    ee.dwAddress = (*i).dwAddress;
                    ee.wOrdinal = (*i).wOrdinal;
                    ee.pName = (*i).pName;

                    (*i).dwAddress = (*j).dwAddress;
                    (*i).pName = (*j).pName;
                    (*i).wOrdinal = (*j).wOrdinal;

                    (*j).dwAddress = ee.dwAddress;
                    (*j).pName = ee.pName;
                    (*j).wOrdinal = ee.wOrdinal;
                }
            }
        }
    }
}

// Get total export names length
DWORD CSectionExport::GetTotalExportNamesLength(void)
{
    DWORD dwLength;
    expIterator i;

    dwLength = 0;

    for (i = m_lstExports.begin(); i != m_lstExports.end(); i++) {
        if ((*i).pName) {
            dwLength += lstrlen((*i).pName);
            dwLength++; // NULL
        }
    }

    return(dwLength);
}

// Copy existing export section
void CSectionExport::Copy(CDD_Export *pExport, LPSTR pSectionName, LPSTR pDLLName)
{
    WORD wOrdinal;
    LPSTR pFuncName;
    DWORD dwLength;
    DWORD dwNumFuncs;

    // Cleanup
    Cleanup();

    // Get export directory table
    pExport->GetTable(&m_imgExport);

    // Set section name
    if (IsBadReadPtr(pSectionName, 1) == FALSE) {
        if (lstrlen(pSectionName) < 8) {
            lstrcpy((LPSTR )m_hdr.Name, pSectionName);
        } else {
            lstrcpyn((LPSTR )m_hdr.Name, pSectionName, 8);
        }
    }

    m_hdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

    // Initialize DLL name
    if (m_pDLLName) {
        delete [] m_pDLLName;
        m_pDLLName = NULL;
    }

    if (pDLLName)
        m_pDLLName = new char [lstrlen(pDLLName) + 1];
    else
        m_pDLLName = new char [pExport->GetDLLName(NULL) + 1];

    if (m_pDLLName) {
        if (pDLLName)
            lstrcpy(m_pDLLName, pDLLName);
        else
            pExport->GetDLLName(m_pDLLName);
    }

    // Reset number of functions/names
    dwNumFuncs = m_imgExport.NumberOfFunctions;
    m_imgExport.NumberOfFunctions = 0;
    m_imgExport.NumberOfNames = 0;

    // Add existing functions
    for (wOrdinal = (WORD )m_imgExport.Base; wOrdinal < m_imgExport.Base + dwNumFuncs; wOrdinal++) {
        dwLength = pExport->GetNameByOrdinal(wOrdinal, NULL);
        if (dwLength) {
            pFuncName = new char [dwLength + 1];
            pExport->GetNameByOrdinal(wOrdinal, pFuncName);
        } else {
            pFuncName = NULL;
        }

        Add(pFuncName, wOrdinal, pExport->GetExportAddressByIndex(wOrdinal - m_imgExport.Base));

        if (pFuncName)
            delete [] pFuncName;
    }
}

// Get a new ordinal value
WORD CSectionExport::NewOrdinal(void)
{
    return(m_wLastOrdinal + 1);
}

void CSectionExport::SortByOrdinal(void)
{
    expIterator i;
    expIterator j;
    EXPORT_ENTRY ee;

    for (i = m_lstExports.begin(); i != m_lstExports.end(); i++) {
        for (j = i; j != m_lstExports.end(); j++) {
            if ((*i).wOrdinal > (*j).wOrdinal) {
                /* Swap */
                ee.dwAddress = (*i).dwAddress;
                ee.wOrdinal = (*i).wOrdinal;
                ee.pName = (*i).pName;

                (*i).dwAddress = (*j).dwAddress;
                (*i).pName = (*j).pName;
                (*i).wOrdinal = (*j).wOrdinal;

                (*j).dwAddress = ee.dwAddress;
                (*j).pName = ee.pName;
                (*j).wOrdinal = ee.wOrdinal;
            }
        }
    }
}

// Compare names
LONG CSectionExport::NameCompare(LPSTR pStr1, LPSTR pStr2)
{
    DWORD dwCount;

    dwCount = 0;
    while (pStr1[dwCount] && pStr2[dwCount]) {
        if (pStr1[dwCount] > pStr2[dwCount]) return(-1);
        if (pStr1[dwCount] < pStr2[dwCount]) return(1);
        dwCount++;
    }

    if (lstrlen(pStr1) > lstrlen(pStr2)) return(-1);
    if (lstrlen(pStr1) < lstrlen(pStr2)) return(1);

    return(0);
}

// Clean up variables used by the class
void CSectionExport::Cleanup(void)
{
    expIterator i;

    // Free DLL name
    if (m_pDLLName) {
        delete [] m_pDLLName;
        m_pDLLName = NULL;
    }

    // Free export entries
    if (m_lstExports.empty() == FALSE) {
        for (i = m_lstExports.begin(); i != m_lstExports.end(); i++) {
            if ((*i).pName) {
                delete [] (*i).pName;
            }
        }
        m_lstExports.clear();
    }
}

// Get export section data
DWORD CSectionExport::GetData(LPBYTE pData) const
{
    DWORD dwSize;

    dwSize = 0;

    if (m_pAttached) {
        return(m_pAttached->GetData(pData));
    }

    return(dwSize);
}

// Get export section header
void CSectionExport::GetHeader(IMAGE_SECTION_HEADER &hdr) const
{
    if (m_pAttached) {
        m_pAttached->GetHeader(hdr);
    }
}

// Check if a file offset is within the section
BOOL CSectionExport::WithinFO(DWORD dwFO) const
{
    if (m_pAttached) {
        return(m_pAttached->WithinFO(dwFO));
    }
    return(FALSE);
}

// Check if a relative virtual address is within the section
BOOL CSectionExport::WithinRVA(DWORD dwRVA) const
{
    if (m_pAttached) {
        return(m_pAttached->WithinRVA(dwRVA));
    }
    return(FALSE);
}

// Set section header
void CSectionExport::SetHeader(IMAGE_SECTION_HEADER &hdrNew)
{
    if (m_pAttached) {
        m_pAttached->SetHeader(hdrNew);
    }
}

}
// PortableExecutable.cpp: implementation of the CPortableExecutable class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "PortableExecutable.h"

using namespace std;

namespace PE {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#pragma warning(disable : 4355)     // Disable warning ( 'this' : used in base member initializer list )

CPortableExecutable::CPortableExecutable() 
: m_ddDebug(this), m_ddExport(this), m_ddImport(this), m_ddCopyright(this), m_ddResource(this)
{
    m_flIsLoaded = FALSE;
}

CPortableExecutable::~CPortableExecutable()
{
    Cleanup();
}

// Load a portable executable
PE_ERROR CPortableExecutable::Load(LPSTR pszFileName)
{
    DWORD dwStubStart;
    CSection sctSection;
    DWORD dwCount;
    PIMAGE_NT_HEADERS pNtHeaders;
    HANDLE hFile;
    HANDLE hMap;
    LPBYTE pMap;

    // Clean up before loading
    Cleanup();

    // Try to open the file
    hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return(PE_OPEN);

    // Make sure file isn't empty
    if (GetFileSize(hFile, NULL) == 0) {
        CloseHandle(hFile);
        return(PE_SIZE);
    }

    // Try to create file mapping
    hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (hMap == NULL) {
        CloseHandle(hFile);
        return(PE_MAP);
    }

    // Get a mapping view of the file
    pMap = (LPBYTE )MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);

    if (pMap == NULL) {
        CloseHandle(hMap);
        CloseHandle(hFile);
        return(PE_VIEW);
    }

    // Get DOS header
    if (IsBadReadPtr(pMap, sizeof(IMAGE_DOS_HEADER))) {
        UnmapViewOfFile(pMap);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return(PE_MEM);
    }

    // Check if MZ executable
    m_Headers.SetDos(*(PIMAGE_DOS_HEADER )pMap);
    if (m_Headers.m_hdrDos.e_magic != IMAGE_DOS_SIGNATURE) {
        UnmapViewOfFile(pMap);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return(PE_NOTMZ);
    }

    // Check if this is a new executable
    if (m_Headers.m_hdrDos.e_lfanew == 0) {
        // No new header
        UnmapViewOfFile(pMap);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return(PE_NOTPE);
    }

    // Get NT headers
    pNtHeaders = (PIMAGE_NT_HEADERS )(pMap + m_Headers.m_hdrDos.e_lfanew);

    if (IsBadReadPtr(pNtHeaders, sizeof(IMAGE_NT_HEADERS))) {
        UnmapViewOfFile(pMap);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return(PE_NOTPE);
    }

    // Check if PE executable
    m_Headers.SetNt(*pNtHeaders);
    if (m_Headers.m_hdrNt.Signature != IMAGE_NT_SIGNATURE) {
        UnmapViewOfFile(pMap);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return(PE_NOTPE);
    }

    // Get DOS stub
    dwStubStart = (m_Headers.m_hdrDos.e_cs << 4) + m_Headers.m_hdrDos.e_ip + (m_Headers.m_hdrDos.e_cparhdr << 4);
    if (dwStubStart >= (65536 << 4)) dwStubStart -= (65536 << 4);
    
    m_Headers.SetHeaderSize((signed long )dwStubStart < sizeof(IMAGE_DOS_HEADER) ? sizeof(IMAGE_DOS_HEADER) : dwStubStart);
    m_DOSStub.Set((pMap + dwStubStart), ((m_Headers.m_hdrDos.e_cp - 1) * 512) + m_Headers.m_hdrDos.e_cblp - dwStubStart);

    // Load sections
    for (dwCount = 0; dwCount < m_Headers.m_hdrNt.FileHeader.NumberOfSections; dwCount++) {

        if (IsBadReadPtr((LPBYTE )pNtHeaders + pNtHeaders->FileHeader.SizeOfOptionalHeader + (sizeof(IMAGE_SECTION_HEADER) * dwCount) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), sizeof(IMAGE_SECTION_HEADER)) == FALSE) {

            sctSection.SetHeader(*(PIMAGE_SECTION_HEADER )((LPBYTE )pNtHeaders + pNtHeaders->FileHeader.SizeOfOptionalHeader + (sizeof(IMAGE_SECTION_HEADER) * dwCount) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER)));

            if (sctSection.m_hdr.SizeOfRawData && sctSection.m_hdr.PointerToRawData) {
                if (IsBadReadPtr(&pMap[sctSection.m_hdr.PointerToRawData], sctSection.m_hdr.SizeOfRawData) == FALSE)
                    sctSection.SetData(&pMap[sctSection.m_hdr.PointerToRawData], sctSection.m_hdr.SizeOfRawData);
            }

            m_lstSections.push_back(sctSection);
        }
    }

    // We don't need the file anymore
    UnmapViewOfFile(pMap);
    CloseHandle(hMap);
    CloseHandle(hFile);

    // Set the Loaded flag
    m_flIsLoaded = TRUE;

    // Fix various stuff
    FixSectionOffsets();
    FixSectionSizes();
    FixImageSize();
    m_Headers.FixSizeOfHeaders();

    // Set up data directories
    m_ddDebug.Assign(GetDataAtRVA(m_Headers.m_hdrNt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress), m_Headers.m_hdrNt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
    m_ddExport.Assign(GetDataAtRVA(m_Headers.m_hdrNt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), m_Headers.m_hdrNt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);
    m_ddImport.Assign(GetDataAtRVA(m_Headers.m_hdrNt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress), m_Headers.m_hdrNt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);
    m_ddCopyright.Assign(GetDataAtRVA(m_Headers.m_hdrNt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_ARCHITECTURE].VirtualAddress), m_Headers.m_hdrNt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_ARCHITECTURE].Size);
    m_ddResource.Assign(GetDataAtRVA(m_Headers.m_hdrNt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress), m_Headers.m_hdrNt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size);

    return(PE_NONE);
}

// Write portable executable
PE_ERROR CPortableExecutable::Write(LPSTR pszFileName, BOOL flSetChecksum)
{
    HANDLE hFile;
    sctIterator i;
    DWORD dwAlign;
    DWORD dwCurrent;
    CSection *pCurrentSection;
    LPBYTE pStub;
    BYTE byNull;
    DWORD dwWritten;

    // Change checksum
    if (flSetChecksum == TRUE)
        m_Headers.m_hdrNt.OptionalHeader.CheckSum = CalcCheckSum();

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_LOAD);

    // Open output file
    hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == NULL)
        return(PE_OPEN);

    // Write MZ header
    WriteFile(hFile, &m_Headers.m_hdrDos, m_Headers.GetHeaderSize(), &dwWritten, NULL);

    // Write DOS stub
    if (m_DOSStub.Get(NULL)) {
        pStub = new BYTE [m_DOSStub.Get(NULL)];
        if (pStub) {
            m_DOSStub.Get(pStub);
            WriteFile(hFile, pStub, m_DOSStub.Get(NULL), &dwWritten, NULL);
            delete [] pStub;
        }
    }

    // Seek to new PE header offset
    SetFilePointer(hFile, m_Headers.m_hdrDos.e_lfanew, NULL, FILE_BEGIN);

    // Write PE header
    WriteFile(hFile, &m_Headers.m_hdrNt, sizeof(IMAGE_NT_HEADERS), &dwWritten, NULL);

    // Write section headers
    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        WriteFile(hFile, &(*i).m_hdr, sizeof(IMAGE_SECTION_HEADER), &dwWritten, NULL);
    }

    // Pad to alignment
    dwCurrent = SetFilePointer(hFile, 0L, NULL, FILE_CURRENT);

    dwAlign = m_Headers.FileAlignment(dwCurrent);
    byNull = 0;

    while (dwCurrent < dwAlign) {
        WriteFile(hFile, &byNull, 1, &dwWritten, NULL);
        dwCurrent++;
    }

    // Write section data
    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        pCurrentSection = &(*i);

        if (pCurrentSection->m_hdr.PointerToRawData && pCurrentSection->m_hdr.SizeOfRawData) {
            SetFilePointer(hFile, pCurrentSection->m_hdr.PointerToRawData, NULL, FILE_BEGIN);
            if (IsBadReadPtr(pCurrentSection->m_pData, pCurrentSection->m_hdr.SizeOfRawData) == FALSE)
                WriteFile(hFile, pCurrentSection->m_pData, pCurrentSection->m_hdr.SizeOfRawData, &dwWritten, NULL);
        }
    }

    // Close file
    CloseHandle(hFile);

    return(PE_NONE);
}

// Clean up variables
void CPortableExecutable::Cleanup()
{
    // Free section list
    if (m_lstSections.empty() == FALSE)
        m_lstSections.clear();

    // Reset Loaded flag
    m_flIsLoaded = FALSE;
}

// Relative Virtual Address to File Offset
DWORD CPortableExecutable::RVA2FO(DWORD dwRVA)
{
    CSection *pSection;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_INVALID);

    pSection = GetSectionByRVA(dwRVA);

    if (pSection) {
        // Convert RVA to File Offset
        dwRVA -= pSection->m_hdr.VirtualAddress;
        dwRVA += pSection->m_hdr.PointerToRawData;

        // In section but not in file
        if (dwRVA >= (pSection->m_hdr.PointerToRawData + pSection->m_hdr.SizeOfRawData))
            dwRVA = PE_INVALID;

    } else {
        dwRVA = PE_INVALID;
    }

    return(dwRVA);
}

// Virtual Address to File Offset
DWORD CPortableExecutable::VA2FO(DWORD dwVA)
{
    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_INVALID);

    // Convert Virtual Address to Relative Virtual Address
    if (dwVA != PE_INVALID)
        dwVA -= m_Headers.m_hdrNt.OptionalHeader.ImageBase;

    return(RVA2FO(dwVA));
}

// File Offset to Relative Virtual Address
DWORD CPortableExecutable::FO2RVA(DWORD dwFO)
{
    CSection *pSection;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_INVALID);

    pSection = GetSectionByFO(dwFO);

    if (pSection) {
        // Convert File Offset to RVA
        dwFO += pSection->m_hdr.VirtualAddress;
        dwFO -= pSection->m_hdr.PointerToRawData;
    } else {
        dwFO = PE_INVALID;
    }

    return(dwFO);
}

// File Offset to Virtual Address
DWORD CPortableExecutable::FO2VA(DWORD dwFO)
{
    DWORD dwVA;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_INVALID);

    // Convert File Offset to Virtual Address
    dwVA = FO2RVA(dwFO);

    if (dwVA != PE_INVALID)
        dwVA += m_Headers.m_hdrNt.OptionalHeader.ImageBase;

    return(dwVA);
}

// Get section by Relative Virtual Address
CSection * CPortableExecutable::GetSectionByRVA(DWORD dwRVA)
{
    CSection *pSection;
    sctIterator i;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(NULL);

    pSection = NULL;

    // Search which section contains the RVA
    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        if ((*i).WithinRVA(dwRVA) == TRUE) {
            pSection = &(*i);
            break;
        }
    }

    // RVA isn't in the file limits
    if (i == m_lstSections.end())
        return(NULL);

    // Return the section that the RVA is within
    return(pSection);
}

// Get section by Virtual Address
CSection * CPortableExecutable::GetSectionByVA(DWORD dwVA)
{
    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(NULL);

    dwVA -= m_Headers.m_hdrNt.OptionalHeader.ImageBase;
    return(GetSectionByRVA(dwVA));
}

// Get section by File Offset
CSection * CPortableExecutable::GetSectionByFO(DWORD dwFO)
{
    CSection *pSection;
    sctIterator i;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(NULL);

    pSection = NULL;

    // Search which section contains the RVA
    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        if ((*i).WithinFO(dwFO) == TRUE) {
            pSection = &(*i);
            break;
        }
    }

    // RVA isn't in the file limits
    if (i == m_lstSections.end())
        return(NULL);

    // Return the section that the RVA is within
    return(pSection);
}

// Get pointer to data at Relative Virtual Address
LPBYTE CPortableExecutable::GetDataAtRVA(DWORD dwRVA)
{
    CSection *pSection;

    pSection = GetSectionByRVA(dwRVA);

    if (pSection) {
        dwRVA -= pSection->m_hdr.VirtualAddress;
        if (dwRVA < pSection->m_hdr.SizeOfRawData)
            return(&pSection->m_pData[dwRVA]);
    }

    return(NULL);
}

// Get pointer to data at Virtual Address
LPBYTE CPortableExecutable::GetDataAtVA(DWORD dwVA)
{
    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(NULL);

    dwVA -= m_Headers.m_hdrNt.OptionalHeader.ImageBase;
    return(GetDataAtRVA(dwVA));
}

// Virtual Address to Relative Virtual Address
DWORD CPortableExecutable::VA2RVA(DWORD dwVA)
{
    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_INVALID);

    if (dwVA != PE_INVALID)
        dwVA -= m_Headers.m_hdrNt.OptionalHeader.ImageBase;

    return(dwVA);
}

// Relative Virtual Address to Virtual Address
DWORD CPortableExecutable::RVA2VA(DWORD dwRVA)
{
    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_INVALID);

    if (dwRVA != PE_INVALID)
        dwRVA += m_Headers.m_hdrNt.OptionalHeader.ImageBase;

    return(dwRVA);
}

// Remove a section
PE_ERROR CPortableExecutable::RemoveSection(CSection *pSection)
{
    sctIterator i;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_LOAD);

    if (pSection) {
        // Fix offset of all sections that go after pSection
        for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
            if ((*i).m_hdr.PointerToRawData > pSection->m_hdr.PointerToRawData) {
                (*i).m_hdr.PointerToRawData -= pSection->m_hdr.SizeOfRawData;
            }
        }

        // Decrease number of section in file header
        m_Headers.m_hdrNt.FileHeader.NumberOfSections--;

        // Remove section from list
        for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
            if (&(*i) == pSection) {
                m_lstSections.erase(i);
                break;
            }
        }
    }

    FixImageSize();
    m_Headers.FixSizeOfHeaders();

    return(PE_NONE);
}

// Insert a section
PE_ERROR CPortableExecutable::InsertSection(CSection *pToAdd, CSection *pNext)
{
    sctIterator iNext;
    sctIterator i;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_LOAD);

    if (pToAdd) {
        // Find where to insert the section
        if (pNext) {
            for (iNext = m_lstSections.begin(); iNext != m_lstSections.end(); iNext++) {
                if (&(*iNext) == pNext) {
                    // Found where to insert the section
                    break;
                }
            }
        } else {
            // Insert at end
            iNext = m_lstSections.end();
        }

        // Fix offset of all sections that go after pSection
        for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
            if ((*i).m_hdr.PointerToRawData >= pToAdd->m_hdr.PointerToRawData) {
                (*i).m_hdr.PointerToRawData += pToAdd->m_hdr.SizeOfRawData;
            }
        }

        // Increase number of section in file header
        m_Headers.m_hdrNt.FileHeader.NumberOfSections++;

        m_lstSections.insert(iNext, *pToAdd);
    }

    FixImageSize();
    m_Headers.FixSizeOfHeaders();

    return(PE_NONE);
}

// Fix ImageSize field
PE_ERROR CPortableExecutable::FixImageSize(void)
{
    DWORD dwImageSize;
    sctIterator i;
    CSection *pLast;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_LOAD);

    FixSectionSizes();

    pLast = GetLastSectionInMemory();

    dwImageSize = pLast->m_hdr.VirtualAddress + pLast->m_hdr.Misc.VirtualSize;

    dwImageSize = m_Headers.SectionAlignment(dwImageSize);
    m_Headers.m_hdrNt.OptionalHeader.SizeOfImage = dwImageSize;

    return(PE_NONE);
}

// Get last section in memory of the portable executable
CSection * CPortableExecutable::GetLastSectionInMemory(void)
{
    CSection *pLast;
    sctIterator i;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(NULL);

    pLast = NULL;

    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        if (i == m_lstSections.begin()) {
            pLast = &(*i);
        } else {
            if ((*i).m_hdr.VirtualAddress > pLast->m_hdr.VirtualAddress)
                pLast = &(*i);
        }
    }

    return(pLast);
}

// Get first section in memory of the portable executable
CSection * CPortableExecutable::GetFirstSectionInMemory(void)
{
    CSection *pFirst;
    sctIterator i;
    BOOL bFoundFirst;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(NULL);

    pFirst = NULL;
    bFoundFirst = FALSE;

    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        if (i == m_lstSections.begin() || bFoundFirst == FALSE) {
            if ((*i).m_hdr.VirtualAddress) {
                pFirst = &(*i);
                bFoundFirst = TRUE;
            }
        } else {
            if ((*i).m_hdr.VirtualAddress)
                if ((*i).m_hdr.VirtualAddress < pFirst->m_hdr.VirtualAddress)
                    pFirst = &(*i);
        }
    }

    return(pFirst);
}

// Get first section in file
CSection * CPortableExecutable::GetFirstSectionInFile(void)
{
    CSection *pFirst;
    sctIterator i;
    BOOL bFoundFirst;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(NULL);

    pFirst = NULL;
    bFoundFirst = FALSE;

    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        if (i == m_lstSections.begin() || bFoundFirst == FALSE) {
            if ((*i).m_hdr.PointerToRawData) {
                pFirst = &(*i);
                bFoundFirst = TRUE;
            }
        } else {
            if ((*i).m_hdr.PointerToRawData)
                if ((*i).m_hdr.PointerToRawData < pFirst->m_hdr.PointerToRawData)
                    pFirst = &(*i);
        }
    }

    return(pFirst);
}

// Get last section in file
CSection * CPortableExecutable::GetLastSectionInFile()
{
    CSection *pLast;
    sctIterator i;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(NULL);

    pLast = NULL;

    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        if (i == m_lstSections.begin()) {
            pLast = &(*i);
        } else {
            if ((*i).m_hdr.PointerToRawData > pLast->m_hdr.PointerToRawData)
                pLast = &(*i);
        }
    }

    return(pLast);
}

// Realign the portable executable according to the FileAlignment
PE_ERROR CPortableExecutable::Realign(void)
{
    sctIterator i;
    sctIterator j;
    CSection *pSection;
    CSection *pSection2;
    DWORD dwFAlign;
    LPBYTE lpData;
    DWORD dwSize;
    BOOL flFirst;
    DWORD dwFix;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_LOAD);

    // Fix offsets (remove junk traces)
    FixSectionOffsets();
    
    // Fix section sizes
    FixSectionSizes();

    // Assign alignment value to local var for comfort
    dwFAlign = m_Headers.m_hdrNt.OptionalHeader.FileAlignment;

    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        pSection = &(*i);

        // Alignment
        if (pSection->m_pData) {
            dwSize = m_Headers.FileAlignment(pSection->m_hdr.SizeOfRawData);

            // If VirtualSize wasn't aligned we need to align it too
            if (pSection->m_hdr.Misc.VirtualSize < dwSize) {
                pSection->m_hdr.Misc.VirtualSize = dwSize;
            }

            // Adjust file offset of further sections
            for (j = m_lstSections.begin(); j != m_lstSections.end(); j++) {
                pSection2 = &(*j);
                if (pSection2->m_hdr.PointerToRawData > pSection->m_hdr.PointerToRawData) {
                    pSection2->m_hdr.PointerToRawData += (dwSize - pSection->m_hdr.SizeOfRawData);
                }
            }

            // Replace data with aligned data
            lpData = new BYTE [dwSize];

            if (lpData) {
                ZeroMemory(lpData, dwSize);
                CopyMemory(lpData, pSection->m_pData, min(pSection->m_hdr.SizeOfRawData, dwSize));
                pSection->m_hdr.SizeOfRawData = dwSize;
                pSection->SetData(lpData, dwSize);

                delete [] lpData;
            }
        }
    }

    FixImageSize();
    m_Headers.FixSizeOfHeaders();

    // Adjust file offsets for sections (to follow PE header)
    flFirst = TRUE;

    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        pSection = &(*i);
        if (pSection->m_hdr.PointerToRawData) {
            if (flFirst) {
                dwFix = pSection->m_hdr.PointerToRawData - m_Headers.m_hdrNt.OptionalHeader.SizeOfHeaders;
                flFirst = FALSE;
            }
            pSection->m_hdr.PointerToRawData -= dwFix;
        }
    }

    return(PE_NONE);
}

// Fix section offsets (so no junk between them)
PE_ERROR CPortableExecutable::FixSectionOffsets(void)
{
    sctIterator si;
    list<CSection *> lstSorted; // Pointers to sections sorted by PointerToRawData
    list<CSection *>::iterator i;
    list<CSection *>::iterator j;
    list<CSection *>::iterator temp;
    list<CSection *>::iterator temp2;
    CSection *scti;
    CSection *sctj;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_LOAD);

    // First create a copy of the unsorted list
    for (si = m_lstSections.begin(); si != m_lstSections.end(); si++) {
        if ((*si).m_hdr.PointerToRawData)
            lstSorted.push_back(&(*si));
    }

    // Sort the list by PointerToRawData
    for (i = lstSorted.begin(); i != lstSorted.end(); i++) {
        i++;
        j = i;
        i--;
        while (j != lstSorted.end()) {
            if ((*i)->m_hdr.PointerToRawData > (*j)->m_hdr.PointerToRawData) {
                scti = *i;
                sctj = *j;

                // Swap
                temp = i;
                temp2 = j;
                lstSorted.insert(i, sctj);
                lstSorted.insert(j, scti);
                temp--;
                temp2--;
                lstSorted.erase(i);
                lstSorted.erase(j);
                i = temp;
                j = temp2;
            }
            j++;
        }
    }

    // Fix offsets
    for (i = lstSorted.begin(); i != lstSorted.end(); i++) {
        scti = *i;
        if (i != lstSorted.begin()) {
            // Get previous section
            i--;
            j = i;
            i++;
            sctj = *j;

            scti->m_hdr.PointerToRawData = sctj->m_hdr.PointerToRawData + sctj->m_hdr.SizeOfRawData;
        }
    }

    // Clear list
    lstSorted.clear();

    return(PE_NONE);
}

// Insert as last section
PE_ERROR CPortableExecutable::AddSection(CSection *pSection)
{
    IMAGE_SECTION_HEADER hdrSection;
    IMAGE_SECTION_HEADER hdrTemp;
    CSection *pTemp;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(PE_LOAD);

    // Fix section sizes
    FixSectionSizes();
    
    if (pSection) {
        pSection->GetHeader(hdrSection);

        // Fix Raw offset
        pTemp = GetLastSectionInFile();
        if (pTemp) {
            pTemp->GetHeader(hdrTemp);

            hdrSection.PointerToRawData = hdrTemp.PointerToRawData + hdrTemp.SizeOfRawData;

            // Fix virtual offset
            pTemp = GetLastSectionInMemory();
            if (pTemp) {
                pTemp->GetHeader(hdrTemp);
                hdrSection.VirtualAddress = hdrTemp.VirtualAddress + m_Headers.SectionAlignment(hdrTemp.Misc.VirtualSize);
                pSection->SetHeader(hdrSection);
            }

            // Add section
            return(InsertSection(pSection, NULL));
        }
    }

    return(PE_NULL);
}

// Get section by name
CSection * CPortableExecutable::GetSectionByName(LPSTR pName)
{
    CSection *pSection;
    sctIterator i;

    // Check if a file was loaded
    if (m_flIsLoaded == FALSE)
        return(NULL);

    pSection = NULL;

    // Search which section contains the RVA
    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        if (strncmp((char *)(*i).m_hdr.Name, pName, 8) == 0) {
            pSection = &(*i);
            break;
        }
    }

    // RVA isn't in the file limits
    if (i == m_lstSections.end())
        return(NULL);

    // Return the section that the RVA is within
    return(pSection);
}

// Calculate PE checksum
DWORD CPortableExecutable::CalcCheckSum(void)
{
    DWORD dwSave;
    DWORD dwSum;
    sctIterator i;
    DWORD dwSize;

    if (m_flIsLoaded == FALSE)
        return(PE_INVALID);

    // Set checksum field to zero first
    dwSave = m_Headers.m_hdrNt.OptionalHeader.CheckSum;
    m_Headers.m_hdrNt.OptionalHeader.CheckSum = 0;

    // Sum the headers
    dwSum = GetCheckSum((LPWORD )&m_Headers.m_hdrDos, sizeof(IMAGE_DOS_HEADER), 0);
    dwSum = GetCheckSum((LPWORD )m_DOSStub.m_pData, m_Headers.m_hdrDos.e_lfanew - sizeof(IMAGE_DOS_HEADER), dwSum);
    dwSum = GetCheckSum((LPWORD )&m_Headers.m_hdrNt, sizeof(IMAGE_NT_HEADERS), dwSum);

    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        dwSum = GetCheckSum((LPWORD )&(*i).m_hdr, sizeof(IMAGE_SECTION_HEADER), dwSum);
    }

    dwSize = m_Headers.m_hdrNt.OptionalHeader.SizeOfHeaders;

    // Restore checksum field
    m_Headers.m_hdrNt.OptionalHeader.CheckSum = dwSave;

    // Sum the sections
    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        dwSum = GetCheckSum((LPWORD )(*i).m_pData, (*i).m_dwSize, dwSum);
        dwSize += (*i).m_dwSize;
    }

    // Finalize checksum
    dwSum = ((dwSum & 0xFFFF) + (dwSum >> 16)) & 0xFFFF; // Add hi/lo words and zero hi word
    dwSum += dwSize; // Add size

    return(dwSum);
}

// Get checksum (without finalizing) of data
DWORD CPortableExecutable::GetCheckSum(LPWORD pwData, DWORD dwSize, DWORD dwSum)
{
    DWORD dwCount;

    if (IsBadReadPtr((LPVOID )pwData, dwSize) == TRUE) return(0);

    dwCount = dwSize / sizeof(WORD);    // We count words

    while (dwCount--) {
        dwSum += *pwData;
        pwData++;
    }

    dwSum = (dwSum >> 16) + (dwSum & 0xFFFF); // Add low/high words

    return(dwSum);
}

CSection * CPortableExecutable::GetSectionByIndex(DWORD dwIndex)
{
    sctIterator i;

    if (m_flIsLoaded == FALSE)
        return(NULL);

    if (dwIndex < m_lstSections.size()) {
        i = m_lstSections.begin();
        while (dwIndex--) i++;
        return(&(*i));
    }

    return(NULL);
}

// Fix non-standard virtual sizes of section
PE_ERROR CPortableExecutable::FixSectionSizes(void)
{
    sctIterator i;
    IMAGE_SECTION_HEADER hdr;

    if (m_flIsLoaded == FALSE)
        return(PE_LOAD);

    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        (*i).GetHeader(hdr);
        hdr.Misc.VirtualSize = max(hdr.Misc.VirtualSize, hdr.SizeOfRawData);
        (*i).SetHeader(hdr);
    }

    return(PE_NONE);
}

// Set new DOS stub
PE_ERROR CPortableExecutable::SetNewStub(LPBYTE pbStub, DWORD cbStub)
{
    signed long lFix;
    sctIterator i;
    IMAGE_SECTION_HEADER hdr;
    PIMAGE_DOS_HEADER phdrDos;
    CSection *pSection;
    DWORD dwOld;
    DWORD dwNew;

    if (m_flIsLoaded == FALSE)
        return(PE_LOAD);

    // Calculate fix value
    lFix = - (signed long )m_DOSStub.Get(NULL);
    lFix += (signed long )(cbStub - sizeof(IMAGE_DOS_HEADER));
    
    // Set new stub
    phdrDos = (PIMAGE_DOS_HEADER )pbStub;
    phdrDos->e_lfanew = cbStub;
    m_Headers.SetDos(*phdrDos);
    m_DOSStub.Set(pbStub + sizeof(IMAGE_DOS_HEADER), cbStub - sizeof(IMAGE_DOS_HEADER));

    // Fix section offsets
    pSection = GetFirstSectionInFile();
    pSection->GetHeader(hdr);
    dwOld = hdr.PointerToRawData;
    dwNew = hdr.PointerToRawData = m_Headers.FileAlignment(cbStub + sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER) * m_lstSections.size());
    pSection->SetHeader(hdr);

    for (i = m_lstSections.begin(); i != m_lstSections.end(); i++) {
        (*i).GetHeader(hdr);
        if (hdr.PointerToRawData) {
            if (&(*i) != pSection) {
                hdr.PointerToRawData -= dwOld;
                hdr.PointerToRawData += dwNew;
                (*i).SetHeader(hdr);
            }
        }
    }

    return(PE_NONE);
}

DWORD CPortableExecutable::GetNumSections(void) const
{
    return(m_lstSections.size());
}

}

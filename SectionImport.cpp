#include "stdafx.h"
#include "sectionimport.h"

using namespace std;

namespace PE {

CSectionImport::CSectionImport(void) 
{
}

CSectionImport::~CSectionImport(void)
{
    Cleanup();
}

// Copy import directory entries
void CSectionImport::Copy(void)
{
    IMAGE_IMPORT_DESCRIPTOR id;
    DWORD dwCount;
    CSection *pOurSection;
    char szSectionName[16];
    IMAGE_SECTION_HEADER hdrSection;

    if (m_ppe == NULL || m_pAttached == NULL) return; // Need to be attached first
    
    // Copy section name
    m_pAttached->GetHeader(hdrSection);
    lstrcpyn(szSectionName, (LPCSTR )hdrSection.Name, 8);
    szSectionName[8] = '\0';

    pOurSection = m_ppe->GetSectionByName(szSectionName);

    // Copy descriptors
    if (m_ppe->m_ddImport.IsInitialized() == TRUE) {
        for (dwCount = 0; dwCount < m_ppe->m_ddImport.GetNumEntries(); dwCount++) {
            m_ppe->m_ddImport.GetEntry(dwCount, &id);

            if (pOurSection->WithinRVA(id.Name)) {
                // These imported functions were added by SectionImport before, readd them
                LPSTR pDLLName = (LPSTR )m_ppe->GetDataAtRVA(id.Name);
                DWORD dwCount2;

                for (dwCount2 = 0; dwCount2 < m_ppe->m_ddImport.GetNumFunctions(dwCount); dwCount2++) {
                    if (m_ppe->m_ddImport.IsOrdinal(dwCount, dwCount2) == TRUE) {
                        DWORD dwOrdinal = m_ppe->m_ddImport.GetOrdinal(dwCount, dwCount2);
                        if (dwOrdinal != PE_INVALID) {
                            Add(pDLLName, (WORD )dwOrdinal);
                        }
                    } else {
                        DWORD dwLen = m_ppe->m_ddImport.GetFuncName(dwCount, dwCount2, NULL);
                        if (dwLen) {
                            LPSTR pFuncName = new char[dwLen + 1];
                            if (pFuncName) {
                                WORD wHint;
    
                                m_ppe->m_ddImport.GetFuncName(dwCount, dwCount2, pFuncName, &wHint);
                                Add(pDLLName, pFuncName, wHint);
                                
                                delete [] pFuncName;
                            }
                        }
                    }
                }

            } else {
                m_lstExisting.push_back(id);
            }
        }
    }
}

void CSectionImport::Attach(CPortableExecutable *ppe, LPSTR pSectionName)
{
    IMAGE_NT_HEADERS hdr;
    IMAGE_SECTION_HEADER hdrSection;
    CSection sct;

    // Clean up
    Cleanup();

    // Set section name and characteristics
    if (IsBadReadPtr(pSectionName, 1) == FALSE) {
        if (lstrlen(pSectionName) < 8)
            lstrcpy((LPSTR )hdrSection.Name, pSectionName);
        else
            lstrcpyn((LPSTR )hdrSection.Name, pSectionName, 8);
    }

    hdrSection.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    sct.SetHeader(hdrSection);

    // Attach CPortableExecutable class
    m_ppe = ppe;

    // Get section
    m_pAttached = ppe->GetSectionByName(pSectionName);

    if (m_pAttached == NULL) {

        // Add section
        ppe->AddSection(&sct);
        m_pAttached = ppe->GetLastSectionInFile(); // The newly added section

    }

    if (m_pAttached) {
        // Change data directory address
        ppe->m_Headers.GetNt(hdr);
        m_pAttached->GetHeader(hdrSection);
        hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = hdrSection.VirtualAddress;
        ppe->m_Headers.SetNt(hdr);
    }
}

void CSectionImport::Update(void)
{
    DWORD dwSize;
    DWORD dwUnalignedSize;
    LPBYTE pData;
    LPBYTE pCurrent;
    IMAGE_NT_HEADERS hdr;
    IMAGE_SECTION_HEADER hdrSection;
    impDescIterator dii;
    impIterator ii;
    IMAGE_IMPORT_DESCRIPTOR id;
    strIterator si;
    DWORD dwNamesRVA;
    DWORD dwFuncsRVA;
    DWORD dwFuncNamesRVA;
    DWORD dwTemp;

    if (m_ppe == NULL || m_pAttached == NULL) return; // Need to be attached first

    // Get size
    dwSize = m_lstExisting.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR); // Existing descriptors
    dwSize += m_lstDLLs.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR); // New descriptors
    dwSize += sizeof(IMAGE_IMPORT_DESCRIPTOR); // NULL descriptor
    dwUnalignedSize = dwSize;
    dwSize = m_ppe->m_Headers.SectionAlignment(dwSize);

    pData = new BYTE [dwSize];

    if (pData) {
        // Zero section data
        ZeroMemory(pData, dwSize);

        // Get section header
        m_pAttached->GetHeader(hdrSection);

        pCurrent = pData;

        // Copy existing descriptors
        if (m_lstExisting.empty() == FALSE) {
            for (dii = m_lstExisting.begin(); dii != m_lstExisting.end(); dii++) {
                CopyMemory(pCurrent, &(*dii), sizeof(IMAGE_IMPORT_DESCRIPTOR));
                pCurrent += sizeof(IMAGE_IMPORT_DESCRIPTOR);
            }
        }

        dwNamesRVA = hdrSection.VirtualAddress + dwUnalignedSize;
        dwFuncsRVA = dwNamesRVA + GetTotalDLLNamesLength();

        // Write new descriptors for each DLL in the list
        if (m_lstDLLs.empty() == FALSE) {
            for (si = m_lstDLLs.begin(); si != m_lstDLLs.end(); si++) {
                // Create import descriptor
                id.OriginalFirstThunk = dwFuncsRVA + (m_lstImports.size() * sizeof(DWORD) + sizeof(DWORD) * m_lstDLLs.size());
                id.Name = dwNamesRVA;
                id.ForwarderChain = 0;
                id.FirstThunk = dwFuncsRVA;
                id.TimeDateStamp = 0; // Not pre-snapped
                
                dwNamesRVA += lstrlen(*si) + 1;
                dwFuncsRVA += (GetNumFuncs(si) + 1) * sizeof(DWORD);

                CopyMemory(pCurrent, &id, sizeof(IMAGE_IMPORT_DESCRIPTOR));
                pCurrent += sizeof(IMAGE_IMPORT_DESCRIPTOR);
            }
        }

        dwNamesRVA = hdrSection.VirtualAddress + dwUnalignedSize;
        dwFuncsRVA = dwNamesRVA + GetTotalDLLNamesLength();

        // Last descriptor - NULL descriptor
        ZeroMemory(pCurrent, sizeof(IMAGE_IMPORT_DESCRIPTOR));
        pCurrent += sizeof(IMAGE_IMPORT_DESCRIPTOR);

        // Write DLL names
        if (m_lstDLLs.empty() == FALSE) {
            for (si = m_lstDLLs.begin(); si != m_lstDLLs.end(); si++) {
                CopyMemory(pCurrent, (*si), lstrlen(*si) + 1);
                pCurrent += (lstrlen(*si) + 1);
            }
        }

        // Write DLL functions
        if (m_lstDLLs.empty() == FALSE && m_lstImports.empty() == FALSE) {
            // FirstThunk table
            // Write pointers to functions
            dwFuncNamesRVA = dwFuncsRVA + ((sizeof(DWORD) * m_lstImports.size() + sizeof(DWORD) * m_lstDLLs.size()) * 2);

            for (si = m_lstDLLs.begin(); si != m_lstDLLs.end(); si++) {
                for (ii = m_lstImports.begin(); ii != m_lstImports.end(); ii++) {
                    if ((*ii).dll == si) {
                        if ((*ii).flIsOrdinal == FALSE) {
                            // Write pointer
                            CopyMemory(pCurrent, &dwFuncNamesRVA, sizeof(DWORD));
                            pCurrent += sizeof(DWORD);
    
                            dwFuncNamesRVA += sizeof(WORD);
                            if ((*ii).Type.pName) {
                                dwFuncNamesRVA += lstrlen((*ii).Type.pName);
                            }
                            dwFuncNamesRVA++; // NULL padding
                        } else {
                            // Write ordinal
                            dwTemp = (*ii).Type.wOrdinal;
                            dwTemp |= IMAGE_ORDINAL_FLAG;
                            CopyMemory(pCurrent, &dwTemp, sizeof(DWORD));
                            pCurrent += sizeof(DWORD);
                        }
                    }
                }

                // Write NULL pointer
                ZeroMemory(pCurrent, sizeof(DWORD));
                pCurrent += sizeof(DWORD);
            }

            // OriginalFirstThunk table
            // Write pointers to functions
            dwFuncNamesRVA = dwFuncsRVA + ((sizeof(DWORD) * m_lstImports.size() + sizeof(DWORD) * m_lstDLLs.size()) * 2);

            for (si = m_lstDLLs.begin(); si != m_lstDLLs.end(); si++) {
                for (ii = m_lstImports.begin(); ii != m_lstImports.end(); ii++) {
                    if ((*ii).dll == si) {
                        if ((*ii).flIsOrdinal == FALSE) {
                            // Write pointer
                            CopyMemory(pCurrent, &dwFuncNamesRVA, sizeof(DWORD));
                            pCurrent += sizeof(DWORD);
    
                            dwFuncNamesRVA += sizeof(WORD);
                            if ((*ii).Type.pName) {
                                dwFuncNamesRVA += lstrlen((*ii).Type.pName);
                            }
                            dwFuncNamesRVA++; // NULL padding
                        } else {
                            // Write ordinal
                            dwTemp = (*ii).Type.wOrdinal;
                            dwTemp |= IMAGE_ORDINAL_FLAG;
                            CopyMemory(pCurrent, &dwTemp, sizeof(DWORD));
                            pCurrent += sizeof(DWORD);
                        }
                    }
                }

                // Write NULL pointer
                ZeroMemory(pCurrent, sizeof(DWORD));
                pCurrent += sizeof(DWORD);
            }

            // Write functions
            for (si = m_lstDLLs.begin(); si != m_lstDLLs.end(); si++) {
                for (ii = m_lstImports.begin(); ii != m_lstImports.end(); ii++) {
                    if (((*ii).dll == si) && ((*ii).flIsOrdinal == FALSE)) {
                        // Write hint/function name
                        CopyMemory(pCurrent, &(*ii).wHint, sizeof(WORD));
                        pCurrent += sizeof(WORD);
                        if ((*ii).Type.pName) {
                            CopyMemory(pCurrent, (*ii).Type.pName, lstrlen((*ii).Type.pName) + 1);
                            pCurrent += lstrlen((*ii).Type.pName) + 1;
                        } else {
                            // Write zero
                            *pCurrent = 0;
                            pCurrent++;
                        }
                    }
                }
            }
        }

        hdrSection.SizeOfRawData = dwSize;
        hdrSection.Misc.VirtualSize = dwSize;
        m_pAttached->SetHeader(hdrSection);

        // Update import directory size in NT headers and size of initialized data
        m_ppe->m_Headers.GetNt(hdr);
        hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = dwUnalignedSize;
        hdr.OptionalHeader.SizeOfInitializedData += dwSize;
        m_ppe->m_Headers.SetNt(hdr);

        // Update size of image
        m_ppe->FixImageSize();

        m_pAttached->SetData(pData, dwSize);
        delete [] pData;

        m_ppe->m_ddImport.Assign(m_ppe->GetDataAtRVA(hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress), dwUnalignedSize);
    }
}

// Get number of functions for a specific DLL
DWORD CSectionImport::GetNumFuncs(strCIterator si) const
{
    impCIterator ii;
    DWORD dwFuncs;

    dwFuncs = 0;

    for (ii = m_lstImports.begin(); ii != m_lstImports.end(); ii++) {
        if ((*ii).dll == si) dwFuncs++;
    }

    return(dwFuncs);
}

// Get total DLL names length
DWORD CSectionImport::GetTotalDLLNamesLength(void) const
{
    DWORD dwLength;
    strCIterator i;

    dwLength = 0;

    if (m_lstDLLs.empty() == FALSE) {
        for (i = m_lstDLLs.begin(); i != m_lstDLLs.end(); i++) {
            if (*i) {
                dwLength += lstrlen(*i) + 1;
            }
        }
    }

    return(dwLength);
}


// Add an import by name
void CSectionImport::Add(LPSTR pDLLName, LPSTR pFuncName, WORD wHint)
{
    strIterator i;
    IMPORT_ENTRY ie;

    // Zero import entry
    ZeroMemory(&ie, sizeof(IMPORT_ENTRY));

    // Insert DLL to the list
    ie.dll = InsertDLL(pDLLName);
    
    // Set name
    if (pFuncName) {
        ie.Type.pName = new char [lstrlen(pFuncName) + 1];
        if (ie.Type.pName) {
            lstrcpy(ie.Type.pName, pFuncName);
        }
    }
    
    // Set hint
    ie.wHint = wHint;

    m_lstImports.push_back(ie);
}

// Add an import by ordinal
void CSectionImport::Add(LPSTR pDLLName, WORD wOrdinal, WORD wHint)
{
    strIterator i;
    IMPORT_ENTRY ie;

    // Zero import entry
    ZeroMemory(&ie, sizeof(IMPORT_ENTRY));

    // Insert DLL to the list
    ie.dll = InsertDLL(pDLLName);
    
    // Set ordinal
    ie.flIsOrdinal = TRUE;
    ie.Type.wOrdinal = wOrdinal;

    // Set hint
    ie.wHint = wHint;

    m_lstImports.push_back(ie);
}

// Insert a DLL to the list
CSectionImport::strIterator CSectionImport::InsertDLL(LPSTR pDLLName)
{
    strIterator i;
    LPSTR pszName;

    // First make sure the DLL isn't already there
    if (m_lstDLLs.empty() == FALSE) {
        for (i = m_lstDLLs.begin(); i != m_lstDLLs.end(); i++) {
            if (lstrcmpi((*i), pDLLName) == 0) {
                // DLL already exists in list
                return(i);
            }
        }
    }

    // Insert DLL if didn't exist
    pszName = new char [lstrlen(pDLLName) + 1];
    if (pszName) {
        lstrcpy(pszName, pDLLName);
        m_lstDLLs.push_back(pszName);
    }

    return(--m_lstDLLs.end());
}

// Cleanup
void CSectionImport::Cleanup(void)
{
    strIterator si;
    impIterator ii;

    if (m_lstExisting.empty() == FALSE)
        m_lstExisting.clear();

    if (m_lstDLLs.empty() == FALSE) {
        for (si = m_lstDLLs.begin(); si != m_lstDLLs.end(); si++) {
            if (*si)
                delete [] (*si);
        }
        m_lstDLLs.clear();
    }

    if (m_lstImports.empty() == FALSE) {
        for (ii = m_lstImports.begin(); ii != m_lstImports.end(); ii++) {
            if ((*ii).flIsOrdinal == FALSE) {
                if ((*ii).Type.pName) {
                    delete [] (*ii).Type.pName;
                }
            }
        }
        m_lstImports.clear();
    }

    m_ppe = NULL;
    m_pAttached = NULL;
}

// Get export section data
DWORD CSectionImport::GetData(LPBYTE pData) const
{
    DWORD dwSize;

    dwSize = 0;

    if (m_pAttached) {
        return(m_pAttached->GetData(pData));
    }

    return(dwSize);
}

// Get export section header
void CSectionImport::GetHeader(IMAGE_SECTION_HEADER &hdr) const
{
    if (m_pAttached) {
        m_pAttached->GetHeader(hdr);
    }
}

// Check if a file offset is within the section
BOOL CSectionImport::WithinFO(DWORD dwFO) const
{
    if (m_pAttached) {
        return(m_pAttached->WithinFO(dwFO));
    }
    return(FALSE);
}

// Check if a relative virtual address is within the section
BOOL CSectionImport::WithinRVA(DWORD dwRVA) const
{
    if (m_pAttached) {
        return(m_pAttached->WithinRVA(dwRVA));
    }
    return(FALSE);
}

// Set section header
void CSectionImport::SetHeader(IMAGE_SECTION_HEADER &hdrNew)
{
    if (m_pAttached) {
        m_pAttached->SetHeader(hdrNew);
    }
}

}
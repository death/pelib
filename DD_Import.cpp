// DD_Import.cpp: implementation of the CDD_Import class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DD_Import.h"
#include "PortableExecutable.h"

namespace PE {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDD_Import::CDD_Import(CPortableExecutable *ppeOwner) : CDataDirectory(ppeOwner)
{

}

CDD_Import::~CDD_Import()
{
    Cleanup();
}

// Assign import directory
void CDD_Import::Assign(LPBYTE pData, DWORD dwSize)
{
    PIMAGE_IMPORT_DESCRIPTOR pid;

    Cleanup();

    CDataDirectory::Assign(pData, dwSize);

    if (IsInitialized() == FALSE) return;

    pid = (PIMAGE_IMPORT_DESCRIPTOR )pData;

    while (pid->Name) {
        m_lstEntries.push_back(pid);
        pid++;
    }
}

// Get number of descriptor entries
DWORD CDD_Import::GetNumEntries(void) const
{
    return(m_lstEntries.size());
}

// Get a descriptor entry by index
void CDD_Import::GetEntry(DWORD dwIndex, PIMAGE_IMPORT_DESCRIPTOR pid) const
{
    dirCImpIterator i;

    if ((dwIndex < GetNumEntries()) && (IsBadWritePtr(pid, sizeof(IMAGE_IMPORT_DESCRIPTOR)) == FALSE)) {
        for (i = m_lstEntries.begin(); dwIndex; i++, dwIndex--);
        CopyMemory(pid, *i, sizeof(IMAGE_IMPORT_DESCRIPTOR));
    }
}

// Get import DLL name by index
DWORD CDD_Import::GetDLLName(DWORD dwIndex, LPSTR pszName) const
{
    IMAGE_IMPORT_DESCRIPTOR impDesc;
    LPSTR pszDLL;
    DWORD dwLength;

    if (dwIndex < GetNumEntries()) {

        GetEntry(dwIndex, &impDesc);
        if (impDesc.Name == 0) return(0);

        pszDLL = (LPSTR )m_ppeOwner->GetDataAtRVA(impDesc.Name);
        if (IsBadReadPtr(pszDLL, 1)) return(0);

        dwLength = lstrlen(pszDLL);

        if (IsBadWritePtr(pszName, dwLength + 1) == FALSE)
            lstrcpy(pszName, pszDLL);

        return(dwLength);

    }

    return(0);
}

// Get number of functions imported from a DLL by Descriptor index
DWORD CDD_Import::GetNumFunctions(DWORD dwDescIndex) const
{
    PIMAGE_THUNK_DATA pTData;
    DWORD dwNumFuncs;

    pTData = GetFirstThunk(dwDescIndex);

    dwNumFuncs = 0;

    while ((IsBadReadPtr(pTData, sizeof(IMAGE_THUNK_DATA)) == FALSE) && pTData->u1.Function) {
        dwNumFuncs++;
        pTData++;
    }

    return(dwNumFuncs);
}

// Check if function represented by ordinal
BOOL CDD_Import::IsOrdinal(DWORD dwDescIndex, DWORD dwFuncIndex) const
{
    PIMAGE_THUNK_DATA pTData;

    pTData = GetFirstThunk(dwDescIndex);

    if (IsBadReadPtr(pTData, sizeof(IMAGE_THUNK_DATA))) return(TRUE); // Better than 'address'

    if (dwFuncIndex >= GetNumFunctions(dwDescIndex)) return(TRUE); // Better than 'address'

    if (IMAGE_SNAP_BY_ORDINAL(pTData[dwFuncIndex].u1.Ordinal))
        return(TRUE);

    return(FALSE);
}

// Get first thunk
PIMAGE_THUNK_DATA CDD_Import::GetFirstThunk(DWORD dwDescIndex) const
{
    IMAGE_IMPORT_DESCRIPTOR impDesc;
    PIMAGE_THUNK_DATA pTData;

    if (dwDescIndex < GetNumEntries()) {

        GetEntry(dwDescIndex, &impDesc);
        if (impDesc.OriginalFirstThunk) {
            pTData = (PIMAGE_THUNK_DATA )m_ppeOwner->GetDataAtRVA(impDesc.OriginalFirstThunk);
        } else {
            pTData = (PIMAGE_THUNK_DATA )m_ppeOwner->GetDataAtRVA(impDesc.FirstThunk);
        }

        if (IsBadReadPtr(pTData, sizeof(IMAGE_THUNK_DATA)))
            return(NULL);

        return(pTData);
    }

    return(NULL);
}

// Get function ordinal
DWORD CDD_Import::GetOrdinal(DWORD dwDescIndex, DWORD dwFuncIndex) const
{
    PIMAGE_THUNK_DATA pTData;

    pTData = GetFirstThunk(dwDescIndex);

    if (IsBadReadPtr(pTData, sizeof(IMAGE_THUNK_DATA))) return(PE_INVALID);
    if (dwFuncIndex >= GetNumFunctions(dwDescIndex)) return(PE_INVALID);

    if (IsOrdinal(dwDescIndex, dwFuncIndex))
        return(IMAGE_ORDINAL(pTData[dwFuncIndex].u1.Ordinal));

    return(PE_INVALID);
}

// Get function name
DWORD CDD_Import::GetFuncName(DWORD dwDescIndex, DWORD dwFuncIndex, LPSTR pszName, LPWORD pwHint) const
{
    PIMAGE_THUNK_DATA pTData;
    LPSTR pszFuncName;
    DWORD dwLength;

    pTData = GetFirstThunk(dwDescIndex);

    if (IsBadReadPtr(pTData, sizeof(IMAGE_THUNK_DATA))) return(0);
    if (dwFuncIndex >= GetNumFunctions(dwDescIndex)) return(0);

    if (IsOrdinal(dwDescIndex, dwFuncIndex) == FALSE) {
        // Function has a name
        pszFuncName = (LPSTR )m_ppeOwner->GetDataAtRVA(pTData[dwFuncIndex].u1.Ordinal); // Easier conversion with .Ordinal

        if (IsBadReadPtr(pszFuncName, 1)) return(0);

        // Assign hint
        if (IsBadWritePtr(pwHint, sizeof(WORD)) == FALSE)
            *pwHint = *(LPWORD )pszFuncName;

        // Skip hint
        pszFuncName += 2;

        dwLength = lstrlen(pszFuncName);

        // Assign name
        if (IsBadWritePtr(pszName, dwLength + 1) == FALSE)
            lstrcpy(pszName, pszFuncName);

        return(dwLength);
    }

    return(0);
}

// Clean up
void CDD_Import::Cleanup(void)
{
    if (m_lstEntries.empty() == FALSE)
        m_lstEntries.clear();
}

}
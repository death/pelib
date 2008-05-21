#include "stdafx.h"
#include "resourcedirectory.h"
#include "util.h"

using namespace std;

namespace PE {

CResourceDirectory::CResourceDirectory(LPBYTE pData, LPBYTE pBase, BOOL bRoot, IMAGE_RESOURCE_DIRECTORY_ENTRY *prde)
: m_pBase(pBase),
  m_strName("")
{
    // Copy resource directory
    if (IsBadReadPtr(pData, sizeof(IMAGE_RESOURCE_DIRECTORY)) == FALSE) {
        CopyMemory(&m_rd, pData, sizeof(IMAGE_RESOURCE_DIRECTORY));
    }

    // Copy resource directory entry
    if (IsBadReadPtr(prde, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY)) == FALSE) {
        CopyMemory(&m_rde, prde, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
    }

    LPBYTE pEntries = (pData + sizeof(IMAGE_RESOURCE_DIRECTORY));
    int nNumEntries = m_rd.NumberOfIdEntries + m_rd.NumberOfNamedEntries;

    // Load entries/directories
    for (int i = 0; i < nNumEntries; i++) {
        CResourceEntry *pre = new CResourceEntry(pEntries, pBase);
        
        if (pre) {
            IMAGE_RESOURCE_DIRECTORY_ENTRY rde;
            pre->GetDirInfo(&rde);

            if (rde.DataIsDirectory) {
                LPBYTE pDir = &pBase[rde.OffsetToDirectory];

                // Determine name
                string strName;

                if (rde.NameIsString) {
                    // Name is specified
                    LPBYTE pName = &pBase[rde.NameOffset];
                    strName = CUtil::GetResourceString(pName);
                } else {
                    // Name's not specified, check names table
                    if (rde.Id < 17 && bRoot == TRUE) {
                        strName = CUtil::GetResourceName(rde.Id);
                    } else {
                        char szName[64];
                        wsprintf(szName, "#%u", rde.Id);
                        strName = szName;
                    }
                }
                
                CResourceDirectory *prd = new CResourceDirectory(pDir, pBase, FALSE, &rde);
                
                // Set name
                prd->SetName(strName);

                m_lstDirs.push_back(prd);
                delete pre;
            } else {
                // Add to entries list
                m_lstEntries.push_back(pre);
            }
        }

        pEntries += sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);
    }
}

CResourceDirectory::~CResourceDirectory(void)
{
    // Clean entries
    resEntIterator ei;

    for (ei = m_lstEntries.begin(); ei != m_lstEntries.end(); ei++) {
        CResourceEntry *pre = (*ei);
        if (pre) {
            delete pre;
        }
    }

    m_lstEntries.clear();

    // Clean directories
    resDirIterator di;

    for (di = m_lstDirs.begin(); di != m_lstDirs.end(); di++) {
        CResourceDirectory *prd = (*di);
        if (prd) {
            delete prd;
        }
    }

    m_lstDirs.clear();
}

void CResourceDirectory::GetInfo(IMAGE_RESOURCE_DIRECTORY *prd) const
{
    if (IsBadWritePtr(prd, sizeof(IMAGE_RESOURCE_DIRECTORY)) == FALSE) {
        CopyMemory(prd, &m_rd, sizeof(IMAGE_RESOURCE_DIRECTORY));
    }
}

void CResourceDirectory::GetEntInfo(IMAGE_RESOURCE_DIRECTORY_ENTRY *prde) const
{
    if (IsBadWritePtr(prde, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY)) == FALSE) {
        CopyMemory(prde, &m_rde, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
    }
}

DWORD CResourceDirectory::GetNumDirectories(void) const
{
    return(m_lstDirs.size());
}

CResourceDirectory *CResourceDirectory::GetDirectory(DWORD dwIndex) const
{
    if (dwIndex < GetNumDirectories()) {
        resDirCIterator i;
        
        for (i = m_lstDirs.begin(); dwIndex; i++, dwIndex--);
        CResourceDirectory *prd = (*i);

        return(prd);
    }

    return(0);
}

DWORD CResourceDirectory::GetNumEntries(void) const
{
    return(m_lstEntries.size());
}

CResourceEntry *CResourceDirectory::GetEntry(DWORD dwIndex) const
{
    if (dwIndex < GetNumEntries()) {
        resEntCIterator i;

        for (i = m_lstEntries.begin(); dwIndex; i++, dwIndex--);
        CResourceEntry *pre = (*i);

        return(pre);
    }

    return(0);
}

void CResourceDirectory::SetName(string strName)
{
    m_strName = strName;
}

string CResourceDirectory::GetName(void) const
{
    return(m_strName);
}

}
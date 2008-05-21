#pragma once

#include "stdafx.h"
#include "resourceentry.h"

namespace PE {

class CResourceDirectory
{
public:
    CResourceDirectory(LPBYTE pData, LPBYTE pBase, BOOL bRoot = FALSE, IMAGE_RESOURCE_DIRECTORY_ENTRY *prde = NULL);
    virtual ~CResourceDirectory(void);

    // Get directory information
    void GetInfo(IMAGE_RESOURCE_DIRECTORY *prd) const;
    void GetEntInfo(IMAGE_RESOURCE_DIRECTORY_ENTRY *prde) const;

    // Directory enumeration functions
    DWORD GetNumDirectories(void) const;
    CResourceDirectory *GetDirectory(DWORD dwIndex) const;

    // Entry enumeration functions
    DWORD GetNumEntries(void) const;
    CResourceEntry *GetEntry(DWORD dwIndex) const;

    // Current directory information functions
    std::string GetName(void) const;

protected:
    void SetName(std::string strName);

private: // Definitions
    typedef std::list< CResourceEntry * > resEntList;
    typedef resEntList::const_iterator resEntCIterator;
    typedef resEntList::iterator resEntIterator;

    typedef std::list< CResourceDirectory * > resDirList;
    typedef resDirList::const_iterator resDirCIterator;
    typedef resDirList::iterator resDirIterator;

private: // Variables
    IMAGE_RESOURCE_DIRECTORY m_rd;
    IMAGE_RESOURCE_DIRECTORY_ENTRY m_rde;
    LPBYTE m_pBase;
    resEntList m_lstEntries;
    resDirList m_lstDirs;

    std::string m_strName;
};

}
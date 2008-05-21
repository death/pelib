#pragma once

#include "stdafx.h"
#include "datadirectory.h"
#include "resourcedirectory.h"

namespace PE {

class CDD_Resource :
    public CDataDirectory
{
public:
    CDD_Resource(CPortableExecutable *ppeOwner);
    virtual ~CDD_Resource(void);

    // Assign resource section
    void Assign(LPBYTE pData, DWORD dwSize);

    // Get root resource directory
    CResourceDirectory *GetRootDirectory(void) const;

private:
    CResourceDirectory *m_prd;
};

}
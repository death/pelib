#pragma once

#include "stdafx.h"
#include "datadirectory.h"

namespace PE {

class CDD_Copyright :
    public CDataDirectory
{
public:
    CDD_Copyright(CPortableExecutable *ppeOwner);
    virtual ~CDD_Copyright(void);
    LPCSTR GetString(void) const;
};

}
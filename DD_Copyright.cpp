#include "stdafx.h"
#include "dd_copyright.h"

namespace PE {

CDD_Copyright::CDD_Copyright(CPortableExecutable *ppeOwner)
: CDataDirectory(ppeOwner)
{
}

CDD_Copyright::~CDD_Copyright(void)
{
}

LPCSTR CDD_Copyright::GetString(void) const
{
    return((LPCSTR )m_pData);
}

}
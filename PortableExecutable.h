// PortableExecutable.h: interface for the CPortableExecutable class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "Headers.h"
#include "Section.h"
#include "DataDirectory.h"
#include "DOSStub.h"

#include "DD_Debug.h"
#include "DD_Export.h"
#include "DD_Import.h"
#include "DD_Copyright.h"
#include "DD_Resource.h"

namespace PE {

class CPortableExecutable
{
public: // Definitions
    typedef std::list<CSection>::iterator sctIterator;
    typedef std::list<CSection>::const_iterator sctCIterator;
    typedef std::list<CSection> sctList;

public:
    // Loading/Saving related functions --------------------------------------

    PE_ERROR Write(LPSTR pszFileName, BOOL flSetChecksum = FALSE);
    PE_ERROR Load(LPSTR pszFileName);

    // Section related functions ---------------------------------------------

    // General section related functions
    DWORD GetNumSections(void) const;
    PE_ERROR AddSection(CSection *pSection);
    PE_ERROR InsertSection(CSection *pToAdd, CSection *pNext);
    PE_ERROR RemoveSection(CSection *pSection);

    // Section retrieval functions
    CSection * GetSectionByIndex(DWORD dwIndex);
    CSection * GetSectionByName(LPSTR pName);
    CSection * GetLastSectionInFile(void);
    CSection * GetFirstSectionInFile(void);
    CSection * GetFirstSectionInMemory(void);
    CSection * GetLastSectionInMemory(void);
    CSection * GetSectionByFO(DWORD dwFO);
    CSection * GetSectionByVA(DWORD dwVA);
    CSection * GetSectionByRVA(DWORD dwRVA);

    // Functions to fix section information
    PE_ERROR FixSectionSizes(void);
    PE_ERROR FixSectionOffsets(void);

    // Data related functions ------------------------------------------------

    LPBYTE GetDataAtVA(DWORD dwVA);
    LPBYTE GetDataAtRVA(DWORD dwRVA);

    // Address conversion functions ------------------------------------------

    DWORD RVA2VA(DWORD dwRVA);
    DWORD VA2RVA(DWORD dwVA);
    DWORD FO2VA(DWORD dwFO);
    DWORD FO2RVA(DWORD dwFO);
    DWORD VA2FO(DWORD dwVA);
    DWORD RVA2FO(DWORD dwRVA);

    // Miscelleneous functions -----------------------------------------------

    DWORD CalcCheckSum(void);
    PE_ERROR Realign(void);
    PE_ERROR FixImageSize(void);
    PE_ERROR SetNewStub(LPBYTE pbStub, DWORD cbStub);

    // Standard constructor/destructor ---------------------------------------

    CPortableExecutable();
    virtual ~CPortableExecutable();

    // Public objects --------------------------------------------------------

    CHeaders m_Headers;
    CDOSStub m_DOSStub;

    // Data directory classes
    CDD_Debug m_ddDebug;
    CDD_Export m_ddExport;
    CDD_Import m_ddImport;
    CDD_Copyright m_ddCopyright;
    CDD_Resource m_ddResource;

protected:
    static DWORD GetCheckSum(LPWORD pwData, DWORD dwSize, DWORD dwSum = 0);
    void Cleanup();

    // Protected variables
    sctList m_lstSections;
    BOOL m_flIsLoaded;
};

}
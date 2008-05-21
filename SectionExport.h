// SectionExport.h: interface for the CSectionExport class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"
#include "Section.h"
#include "PortableExecutable.h"    

namespace PE {

class CSectionExport
{
public: // Definitions
    typedef struct _EXPORT_ENTRY {
        char *pName;
        DWORD dwAddress;
        WORD wOrdinal;
    } EXPORT_ENTRY;

    typedef std::list<EXPORT_ENTRY> expList;
    typedef std::list<EXPORT_ENTRY>::iterator expIterator;

public:
    CSectionExport();
    virtual ~CSectionExport();

    // CSection delegates
    void SetHeader(IMAGE_SECTION_HEADER &hdrNew);
    BOOL WithinRVA(DWORD dwRVA) const;
    BOOL WithinFO(DWORD dwFO) const;
    void GetHeader(IMAGE_SECTION_HEADER &hdr) const;
    DWORD GetData(LPBYTE pData) const;

    WORD NewOrdinal(void);
    void Copy(CDD_Export *pExport, LPSTR pSectionName, LPSTR pDLLName = NULL);
    void Attach(CPortableExecutable *ppe);
    void Update(void);
    void Add(LPSTR pFuncName, WORD wOrdinal, DWORD dwAddress);
    void Create(LPSTR pSectionName, LPSTR pDLLName);

protected:
    void Cleanup(void);
    LONG NameCompare(LPSTR pStr1, LPSTR pStr2);
    void SortByOrdinal(void);
    DWORD GetTotalExportNamesLength(void);
    void SortByName(void);

    WORD m_wLastOrdinal;
    CSection * m_pAttached;
    CPortableExecutable *m_ppe;
    LPSTR m_pDLLName;
    IMAGE_EXPORT_DIRECTORY m_imgExport;
    expList m_lstExports;
    IMAGE_SECTION_HEADER m_hdr;
};

}
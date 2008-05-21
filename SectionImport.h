#pragma once

#include "stdafx.h"
#include "section.h"
#include "PortableExecutable.h"

namespace PE {

class CSectionImport
{
public: // Definitions
    // For DLL names list
    typedef std::list<LPSTR> strList;
    typedef std::list<LPSTR>::iterator strIterator;
    typedef std::list<LPSTR>::const_iterator strCIterator;

    typedef struct _IMPORT_ENTRY {
        union {
            char *pName;
            WORD wOrdinal;
        } Type;
        WORD wHint;
        strIterator dll;
        BOOL flIsOrdinal; // Needed because we don't know Ordinal base
    } IMPORT_ENTRY;

    typedef std::list<IMPORT_ENTRY> impList;
    typedef std::list<IMPORT_ENTRY>::iterator impIterator;
    typedef std::list<IMPORT_ENTRY>::const_iterator impCIterator;

    typedef std::list<IMAGE_IMPORT_DESCRIPTOR> impDescList;
    typedef std::list<IMAGE_IMPORT_DESCRIPTOR>::iterator impDescIterator;
    
public:
    CSectionImport(void);
    virtual ~CSectionImport(void);

    // CSection delegates
    void SetHeader(IMAGE_SECTION_HEADER &hdrNew);
    BOOL WithinRVA(DWORD dwRVA) const;
    BOOL WithinFO(DWORD dwFO) const;
    void GetHeader(IMAGE_SECTION_HEADER &hdr) const;
    DWORD GetData(LPBYTE pData) const;

    void Copy(void);
    void Attach(CPortableExecutable *ppe, LPSTR pSectionName);
    void Update(void);
    void Add(LPSTR pDLLName, LPSTR pFuncName, WORD wHint = 0);
    void Add(LPSTR pDLLName, WORD wOrdinal, WORD wHint = 0);

protected:
    // Cleanup
    void Cleanup(void);
    // Insert a DLL to the list
    strIterator InsertDLL(LPSTR pDLLName);
    // Get total DLL names length
    DWORD GetTotalDLLNamesLength(void) const;
    // Get number of functions for a specific DLL
    DWORD GetNumFuncs(strCIterator si) const;

    // Attached section
    CSection *m_pAttached;
    // Attached PE
    CPortableExecutable *m_ppe;
    // Existing import descriptors list
    impDescList m_lstExisting;
    // Added DLL names list
    strList m_lstDLLs;
    // Imports list (added)
    impList m_lstImports;
};

}
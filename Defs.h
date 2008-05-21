#pragma once

namespace PE {

#define PE_MAJVER   1                   // Major version of the library
#define PE_MINVER   2                   // Minor version of the library

#define PE_INVALID (0xFFFFFFFF)         // Invalid address/offset/whatever

typedef enum _PE_ERROR {
    PE_NONE, PE_OPEN, PE_MAP, PE_VIEW, PE_SIZE, PE_CLOSE, PE_NOTMZ, PE_NOTPE, PE_LOAD, PE_NULL, PE_MEM, PE_LAST
} PE_ERROR;

}

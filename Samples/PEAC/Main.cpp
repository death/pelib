// Includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include "Resource.h"
#include "PortableExecutable.h"
#include "Util.h"

using namespace PE;

// Enumeration structures
typedef enum {
    CM_FO, CM_RVA, CM_VA
} CONV_MODE;                            // Conversion mode

// Global variables
HINSTANCE g_hInst;
HICON g_hIconMain;
CONV_MODE g_cm;                           // Current conversion mode
CPortableExecutable g_pe;
PE_ERROR g_error;
BOOL g_flLoaded;

// Constants
const char cszAppName[] = "PEAC";
const char cszAbout[] = "PE Address Converter by DEATH of Execution in 2002 (class version: %d.%d)";

// Function prototypes
BOOL CALLBACK DlgMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateOptions(HWND hDlg);
void EditReadOnly(HWND hwndEdit, BOOL flReadOnly);
BOOL BrowseInputFile(HWND hwndOwner, HWND hwndFile);
void LoadInitialValues(HWND hDlg);
void Convert(HWND hDlg);
void EnableOptions(HWND hwnd);

// Main programme
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInst = hInstance;
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgMain);
    return(0);
}

// Main dialog procedure
BOOL CALLBACK DlgMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char szBuffer[128];

    switch (uMsg) {
    case WM_INITDIALOG:
        // Dialog initialization
        g_hIconMain = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_MAIN));
        SendMessage(hDlg, WM_SETICON, (WPARAM )ICON_BIG, (LPARAM )g_hIconMain);

        g_cm = CM_FO;                     // Initial conversion mode
        UpdateOptions(hDlg);

        g_flLoaded = FALSE;               // No file is currently loaded

        EnableOptions(hDlg);
        break;
    case WM_COMMAND:
        // Dialog command
        switch (HIWORD(wParam)) {
        case BN_CLICKED:
            // Button click
            switch (LOWORD(wParam)) {
            case IDC_BROWSE:
                // Browse button
                if (BrowseInputFile(hDlg, GetDlgItem(hDlg, IDT_FILENAME)) == TRUE) {
                    LoadInitialValues(hDlg);
                    EnableOptions(hDlg);
                }
                break;
            case IDC_ABOUT:
                // About button
                wsprintf(szBuffer, cszAbout, PE_MAJVER, PE_MINVER);
                MessageBox(hDlg, szBuffer, cszAppName, MB_OK | MB_ICONINFORMATION);
                break;
            case IDC_CONVERT:
                // Convert button
                Convert(hDlg);
                break;
            case IDC_EXIT:
                // Exit button
                PostMessage(hDlg, WM_CLOSE, (WPARAM )0, (LPARAM )0);
                break;
            case IDC_MODE_FO:
                // File Offset mode
                g_cm = CM_FO;
                UpdateOptions(hDlg);
                break;
            case IDC_MODE_RVA:
                // Relative Virtual Address mode
                g_cm = CM_RVA;
                UpdateOptions(hDlg);
                break;
            case IDC_MODE_VA:
                // Virtual Address mode
                g_cm = CM_VA;
                UpdateOptions(hDlg);
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        break;
    case WM_CLOSE:
        // Close dialog
        EndDialog(hDlg, TRUE);
        break;
    default:
        break;
    }

    return(FALSE);
}

// Update conversion options
void UpdateOptions(HWND hDlg)
{
    switch (g_cm) {
    case CM_FO:
        // File offset conversion
        EditReadOnly(GetDlgItem(hDlg, IDT_FO), FALSE);
        EditReadOnly(GetDlgItem(hDlg, IDT_RVA), TRUE);
        EditReadOnly(GetDlgItem(hDlg, IDT_VA), TRUE);

        CheckRadioButton(hDlg, IDC_MODE_FO, IDC_MODE_VA, IDC_MODE_FO);
        break;
    case CM_RVA:
        // Relative Virtual Address conversion
        EditReadOnly(GetDlgItem(hDlg, IDT_FO), TRUE);
        EditReadOnly(GetDlgItem(hDlg, IDT_RVA), FALSE);
        EditReadOnly(GetDlgItem(hDlg, IDT_VA), TRUE);

        CheckRadioButton(hDlg, IDC_MODE_FO, IDC_MODE_VA, IDC_MODE_RVA);
        break;
    case CM_VA:
        // Virtual Address conversion
        EditReadOnly(GetDlgItem(hDlg, IDT_FO), TRUE);
        EditReadOnly(GetDlgItem(hDlg, IDT_RVA), TRUE);
        EditReadOnly(GetDlgItem(hDlg, IDT_VA), FALSE);

        CheckRadioButton(hDlg, IDC_MODE_FO, IDC_MODE_VA, IDC_MODE_VA);
        break;
    default:
        break;
    }
}

// Disable/Enable Read Only mode of an Edit control and set Tab Stop flag
void EditReadOnly(HWND hwndEdit, BOOL flReadOnly)
{        
    DWORD dwStyle;

    SendMessage(hwndEdit, EM_SETREADONLY, (WPARAM )flReadOnly, (LPARAM )0);
    dwStyle = GetWindowLong(hwndEdit, GWL_STYLE);
    
    if (flReadOnly == FALSE)
        dwStyle |= WS_TABSTOP;
    else
        dwStyle = (dwStyle & ~(WS_TABSTOP));

    SetWindowLong(hwndEdit, GWL_STYLE, dwStyle);
}

// Browse for input file
BOOL BrowseInputFile(HWND hwndOwner, HWND hwndFile)
{
    OPENFILENAME ofn;
    char szFilter[] = "Portable Executables\0*.EXE;*.DLL;*.OCX\0All Files\0*.*\0";
    char szFileName[256];
    char szTitle[] = "Choose input file";
    char szDefExt[] = "EXE";

    GetWindowText(hwndFile, szFileName, sizeof(szFileName));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndOwner;
    ofn.hInstance = g_hInst;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName);
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = szDefExt;

    if (GetOpenFileName(&ofn)) {
        SetWindowText(hwndFile, szFileName);
        return(TRUE);
    }

    return(FALSE);
}

// Load initial values from PE
void LoadInitialValues(HWND hDlg)
{
    char szFileName[256];
    char szBuffer[128];
    IMAGE_NT_HEADERS hdrNt;

    // Get filename
    GetDlgItemText(hDlg, IDT_FILENAME, szFileName, sizeof(szFileName));
    if (szFileName[0] == '\0')
        return;

    // Try to load the file
    g_error = g_pe.Load(szFileName);
    if (g_error != PE_NONE) {
        MessageBox(hDlg, CUtil::GetErrorString(g_error), cszAppName, MB_OK | MB_ICONERROR);
        return;
    }

    g_flLoaded = TRUE;

    // Fix section sizes
    g_pe.FixSectionSizes();

    // Get NT headers
    g_pe.m_Headers.GetNt(hdrNt);
    
    // Image base
    wsprintf(szBuffer, "%08lX", hdrNt.OptionalHeader.ImageBase);
    SetDlgItemText(hDlg, IDT_IMAGEBASE, szBuffer);

    // Set RVA field to Entry Point
    g_cm = CM_RVA;
    UpdateOptions(hDlg);

    wsprintf(szBuffer, "%lX", hdrNt.OptionalHeader.AddressOfEntryPoint);
    SetDlgItemText(hDlg, IDT_RVA, szBuffer);

    // Change other values
    Convert(hDlg);
}

// Convert values
void Convert(HWND hDlg)
{
    char szBuffer[128];
    DWORD dwFO;
    DWORD dwRVA;
    DWORD dwVA;
    CSection *pSection;
    DWORD dwCount;
    LPBYTE lpData;
    char szUndefined[] = "Undefined";
    IMAGE_SECTION_HEADER hdrSection;

    // Check if a file was loaded
    if (g_flLoaded == FALSE)
        return;

    // Convert address
    switch (g_cm) {
    case CM_FO:
        // File Offset
        GetDlgItemText(hDlg, IDT_FO, szBuffer, sizeof(szBuffer));
        dwFO = strtoul(szBuffer, NULL, 16);
        dwVA = g_pe.FO2VA(dwFO);
        dwRVA = g_pe.FO2RVA(dwFO);
        break;
    case CM_RVA:
        // Relative Virtual Address
        GetDlgItemText(hDlg, IDT_RVA, szBuffer, sizeof(szBuffer));
        dwRVA = strtoul(szBuffer, NULL, 16);
        dwVA = g_pe.RVA2VA(dwRVA);
        dwFO = g_pe.RVA2FO(dwRVA);
        break;
    case CM_VA:
        // Virtual Address
        GetDlgItemText(hDlg, IDT_VA, szBuffer, sizeof(szBuffer));
        dwVA = strtoul(szBuffer, NULL, 16);
        dwRVA = g_pe.VA2RVA(dwVA);
        dwFO = g_pe.VA2FO(dwVA);
        break;
    default:
        break;
    }

    // Check validity of addresses
    if ((LONG )dwFO < 0) {
        dwRVA = dwVA = dwFO = PE_INVALID;
    } else if ((LONG )dwRVA < 0) {
        dwRVA = dwVA = PE_INVALID;
    }

    // Set File Offset field
    if (dwFO == PE_INVALID)
        lstrcpy(szBuffer, szUndefined);
    else
        wsprintf(szBuffer, "%lX", dwFO);
    SetDlgItemText(hDlg, IDT_FO, szBuffer);

    // Set Relative Virtual Address field
    if (dwRVA == PE_INVALID)
        lstrcpy(szBuffer, szUndefined);
    else
        wsprintf(szBuffer, "%lX", dwRVA);
    SetDlgItemText(hDlg, IDT_RVA, szBuffer);

    // Set Virtual Address field
    if (dwVA == PE_INVALID)
        lstrcpy(szBuffer, szUndefined);
    else
        wsprintf(szBuffer, "%lX", dwVA);
    SetDlgItemText(hDlg, IDT_VA, szBuffer);

    // Set section name
    ZeroMemory(szBuffer, sizeof(szBuffer));
    pSection = g_pe.GetSectionByFO(dwFO);
    if (pSection) {
        pSection->GetHeader(hdrSection);
        lstrcpyn(szBuffer, (LPSTR )hdrSection.Name, 8);
    } else {
        lstrcpy(szBuffer, "None");
    }
    SetDlgItemText(hDlg, IDT_SECTION, szBuffer);

    // Set bytes field (5 bytes)
    lpData = g_pe.GetDataAtRVA(dwRVA);
    if (lpData != NULL) {
        DWORD dwNumBytes;

        if (pSection)
            dwNumBytes = min(5, hdrSection.PointerToRawData + hdrSection.SizeOfRawData - dwFO);
        else
            dwNumBytes = 0;

        szBuffer[0] = '\0';

        for (dwCount = 0; dwCount < dwNumBytes; dwCount++) {
            wsprintf((LPSTR )&szBuffer[dwCount * 3], "%02X ", lpData[dwCount]);
        }
    } else {
        lstrcpy(szBuffer, "Only bytes inside section");
    }
    SetDlgItemText(hDlg, IDT_BYTES, szBuffer);
}

// Enable/Disable dialog options
void EnableOptions(HWND hwnd)
{
    if (g_flLoaded == TRUE) {
        // PE loaded
        EnableWindow(GetDlgItem(hwnd, IDC_CONVERT), TRUE);
    } else {
        EnableWindow(GetDlgItem(hwnd, IDC_CONVERT), FALSE);
    }
}
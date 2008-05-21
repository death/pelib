// Includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include "Resource.h"
#include "PortableExecutable.h"
#include "Util.h"

using namespace PE;

// Global variables
HINSTANCE g_hInst;
HICON g_hIconMain;
BOOL g_flLoaded;

// Constants
const char cszAppName[] = "Align PE";
const char cszAbout[] = "Align PE by DEATH of Execution in 2002 (class version: %d.%d)";

// Function prototypes
BOOL CALLBACK DlgMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL BrowseInputFile(HWND hwndOwner, HWND hwndFile);
void LoadAlignValues(HWND hwndOwner, HWND hwndFile, HWND hwndFAlign);
BOOL BrowseOutputFile(HWND hwndOwner, LPSTR lpOutputFile, DWORD cbOutputFile);
void SaveAlignValues(HWND hwndOwner, LPSTR lpOutputFile, HWND hwndInputFile, HWND hwndFAlign);
DWORD CountBits(DWORD dw);
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
    char szOutputFile[256];
    char szBuffer[128];

    switch (uMsg) {
    case WM_INITDIALOG:
        // Dialog initialization
        
        // Set dialog icon
        g_hIconMain = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_MAIN));
        SendMessage(hDlg, WM_SETICON, (WPARAM )ICON_BIG, (LPARAM )g_hIconMain);

        // Reset load flag
        g_flLoaded = FALSE;

        EnableOptions(hDlg);
        break;
    case WM_COMMAND:
        // Dialog command
        switch (HIWORD(wParam)) {
        case BN_CLICKED:
            // Button click
            switch (LOWORD(wParam)) {
            case IDC_EXIT:
                // Exit button
                PostMessage(hDlg, WM_CLOSE, (WPARAM )0, (LPARAM )0);
                break;
            case IDC_ABOUT:
                // About button
                wsprintf(szBuffer, cszAbout, PE_MAJVER, PE_MINVER);
                MessageBox(hDlg, szBuffer, cszAppName, MB_OK | MB_ICONINFORMATION);
                break;
            case IDC_BROWSE:
                // Browse button
                if (BrowseInputFile(hDlg, GetDlgItem(hDlg, IDT_FILE)) == TRUE) {
                    LoadAlignValues(hDlg, GetDlgItem(hDlg, IDT_FILE), GetDlgItem(hDlg, IDT_FALIGN));
                    g_flLoaded = TRUE;
                    EnableOptions(hDlg);
                }
                break;
            case IDC_ALIGN:
                // Align button
                if (g_flLoaded == TRUE) {
                    if (BrowseOutputFile(hDlg, szOutputFile, sizeof(szOutputFile)) == TRUE) {
                        SaveAlignValues(hDlg, szOutputFile, GetDlgItem(hDlg, IDT_FILE), GetDlgItem(hDlg, IDT_FALIGN));
                    }
                }
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

// Load align values from a PE
void LoadAlignValues(HWND hwndOwner, HWND hwndFile, HWND hwndFAlign)
{
    char szFileName[256];
    char szBuffer[64];
    CPortableExecutable pe;
    PE_ERROR err;
    IMAGE_NT_HEADERS hdrNt;

    // Get file to load
    GetWindowText(hwndFile, szFileName, sizeof(szFileName));

    // Load PE
    err = pe.Load(szFileName);
    if (err != PE_NONE) {
        MessageBox(hwndOwner, CUtil::GetErrorString(err), NULL, MB_OK | MB_ICONERROR);
        return;
    }

    // Set file align value
    pe.m_Headers.GetNt(hdrNt);
    wsprintf(szBuffer, "%ld", hdrNt.OptionalHeader.FileAlignment);
    SetWindowText(hwndFAlign, szBuffer);
}

// Browse for output file
BOOL BrowseOutputFile(HWND hwndOwner, LPSTR lpOutputFile, DWORD cbOutputFile)
{
    OPENFILENAME ofn;
    char szFilter[] = "Portable Executables\0*.EXE;*.DLL;*.OCX\0All Files\0*.*\0";
    char szTitle[] = "Choose output file";
    char szDefExt[] = "EXE";

    ZeroMemory(lpOutputFile, cbOutputFile);
    
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndOwner;
    ofn.hInstance = g_hInst;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = lpOutputFile;
    ofn.nMaxFile = cbOutputFile;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = szDefExt;

    if (GetSaveFileName(&ofn)) {
        return(TRUE);
    }

    return(FALSE);
}

// Save align values to a PE
void SaveAlignValues(HWND hwndOwner, LPSTR lpOutputFile, HWND hwndInputFile, HWND hwndFAlign)
{
    CPortableExecutable pe;
    PE_ERROR err;
    char szInputFile[256];
    char szBuffer[128];
    DWORD dwAlign;
    char szMinError[] = "Minimum alignment value is 16";
    char sz9xError[] = "Warning: Win9x does not support alignment value less than 512";
    char szExpError[] = "Value must be exponentiation of 2";
    char szMaxError[] = "Maximum file alignment value is %ld";
    IMAGE_NT_HEADERS hdrNt;

    // Get input file
    GetWindowText(hwndInputFile, szInputFile, sizeof(szInputFile));    

    // Load input PE
    err = pe.Load(szInputFile);
    if (err != PE_NONE) {
        MessageBox(hwndOwner, CUtil::GetErrorString(err), NULL, MB_OK | MB_ICONERROR);
        return;
    }

    // Get file align value
    GetWindowText(hwndFAlign, szBuffer, sizeof(szBuffer));
    dwAlign = atol(szBuffer);

    // Check alignment value and error if less than 16
    if (dwAlign < 0x10) {
        MessageBox(hwndOwner, szMinError, NULL, MB_OK | MB_ICONERROR);
        return;
    }

    // Check alignment value and warn if less than 512 (9x cannot support it)
    if (dwAlign < 0x200) {
        MessageBox(hwndOwner, sz9xError, NULL, MB_OK | MB_ICONERROR);
    }

    // Get NT headers
    pe.m_Headers.GetNt(hdrNt);
    
    // Check maximum alignment value
    if (dwAlign > hdrNt.OptionalHeader.SectionAlignment) {
        wsprintf(szBuffer, szMaxError, hdrNt.OptionalHeader.SectionAlignment);
        MessageBox(hwndOwner, szBuffer, NULL, MB_OK | MB_ICONERROR);
        return;
    }
    
    // Must be exponentiation of 2
    if (CountBits(dwAlign) != 1) {
        MessageBox(hwndOwner, szExpError, NULL, MB_OK | MB_ICONERROR);
        return;
    }

    // Set file alignment value
    hdrNt.OptionalHeader.FileAlignment = dwAlign;
    pe.m_Headers.SetNt(hdrNt);

    // Realign PE
    pe.Realign();
    
    // Write new PE
    pe.Write(lpOutputFile);
}

// Count the number of bits set in a DWORD
DWORD CountBits(DWORD dw)
{
    DWORD dwBits;
    DWORD dwCount;

    dwBits = 0;

    for (dwCount = 0; dwCount < 32; dwCount++) {
        dwBits += ((dw >> dwCount) & 1);
    }

    return(dwBits);
}

// Enable/Disable dialog options
void EnableOptions(HWND hwnd)
{
    if (g_flLoaded == TRUE) {
        // PE loaded
        EnableWindow(GetDlgItem(hwnd, IDC_ALIGN), TRUE);
    } else {
        EnableWindow(GetDlgItem(hwnd, IDC_ALIGN), FALSE);
    }
}
// Includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <cstdio>
#include <io.h>

#include "Resource.h"
#include "PortableExecutable.h"
#include "Util.h"

using namespace std;
using namespace PE;

// Global variables
HINSTANCE g_hInst;
HICON g_hIconMain;
BOOL g_flLoaded;

// Constants
const char cszAppName[] = "SetStub";
const char cszAbout[] = "SetStub by DEATH of Execution in 2002 (class version: %d.%d)";

// Function prototypes
BOOL CALLBACK DlgMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL BrowseInputFile(HWND hwndOwner, HWND hwndFile);
BOOL BrowseOutputFile(HWND hwndOwner, LPSTR lpOutputFile, DWORD cbOutputFile);
void EnableOptions(HWND hwnd);
void SetNewStub(HWND hwndParent, HWND hwndPE, HWND hwndStub, LPSTR pszOutput);

// Main programme
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInst = hInstance;
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgMain);
    return(0);
}

// Main dialog
BOOL CALLBACK DlgMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char szBuffer[128];
    char szFileName[256];

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
                    switch (LOWORD(wParam)) {
                        case IDC_ABOUT:
                            // About button
                            wsprintf(szBuffer, cszAbout, PE_MAJVER, PE_MINVER);
                            MessageBox(hDlg, szBuffer, cszAppName, MB_OK | MB_ICONINFORMATION);
                            break;
                        case IDC_EXIT:
                            // Exit button
                            PostMessage(hDlg, WM_CLOSE, (WPARAM )0, (LPARAM )0);
                            break;
                        case IDC_SAVE:
                            if (g_flLoaded == TRUE) {
                                if (BrowseOutputFile(hDlg, szFileName, sizeof(szFileName)) == TRUE) {
                                    SetNewStub(hDlg, GetDlgItem(hDlg, IDT_PEFILE), GetDlgItem(hDlg, IDT_STUBFILE), szFileName);
                                }
                            }
                            break;
                        case IDC_PEBROWSE:
                            if (BrowseInputFile(hDlg, GetDlgItem(hDlg, IDT_PEFILE)) == TRUE) {
                                if (GetWindowTextLength(GetDlgItem(hDlg, IDT_STUBFILE))) {
                                    g_flLoaded = TRUE;
                                }
                                EnableOptions(hDlg);
                            }
                            break;
                        case IDC_STUBBROWSE:
                            if (BrowseInputFile(hDlg, GetDlgItem(hDlg, IDT_STUBFILE)) == TRUE) {
                                if (GetWindowTextLength(GetDlgItem(hDlg, IDT_PEFILE))) {
                                    g_flLoaded = TRUE;
                                }
                                EnableOptions(hDlg);
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
            EndDialog(hDlg, TRUE);
            break;
        default:
            break;
    }

    return(FALSE);
}

// Enable/Disable dialog options
void EnableOptions(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd, IDC_SAVE), g_flLoaded);
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

// Set new stub for PE
void SetNewStub(HWND hwndParent, HWND hwndPE, HWND hwndStub, LPSTR pszOutputFile)
{
    CPortableExecutable pe;
    char szPEFile[256];
    char szStubFile[256];
    PE_ERROR error;
    CPortableExecutable stub;
    LPBYTE pbStubData = NULL;
    DWORD dwStubSize;
    FILE *pFile;
    CDOSStub stubNew;

    // Get filenames
    GetWindowText(hwndPE, szPEFile, sizeof(szPEFile));
    GetWindowText(hwndStub, szStubFile, sizeof(szStubFile));

    // Load PE
    error = pe.Load(szPEFile);
    if (error != PE_NONE) {
        MessageBox(hwndParent, CUtil::GetErrorString(error), cszAppName, MB_OK | MB_ICONERROR);
        return;
    }

    // Try to load stub from another PE
    error = stub.Load(szStubFile);
    if (error != PE_NONE) {
        if (error == PE_NOTPE) {
            // Only stub
            pFile = fopen(szStubFile, "rb");
            if (pFile) {
                dwStubSize = _filelength(fileno(pFile));
                pbStubData = new BYTE[dwStubSize];
                if (pbStubData) {
                    fread(pbStubData, sizeof(char), dwStubSize, pFile);
                }
                fclose(pFile);
            }
        } else {
            MessageBox(hwndParent, CUtil::GetErrorString(error), cszAppName, MB_OK | MB_ICONERROR);
            return;
        }
    } else {
        dwStubSize = stub.m_DOSStub.Get(NULL);
        pbStubData = new BYTE[dwStubSize + sizeof(IMAGE_DOS_HEADER)];
        if (pbStubData) {
			stub.m_Headers.GetDos(*((PIMAGE_DOS_HEADER )pbStubData));
            stub.m_DOSStub.Get(&pbStubData[sizeof(IMAGE_DOS_HEADER)]);
			dwStubSize += sizeof(IMAGE_DOS_HEADER);
        }
    }

    if (pbStubData) {
        stubNew.Set(pbStubData, dwStubSize);
        pe.SetNewStub(pbStubData, dwStubSize);

        pe.Write(pszOutputFile);

        delete [] pbStubData;
    }
}

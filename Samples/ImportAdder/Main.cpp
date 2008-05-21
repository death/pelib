// Includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <cstring>

#include "PortableExecutable.h"
#include "SectionImport.h"
#include "Util.h"
#include "resource.h"

using namespace std;
using namespace PE;

// Global variables
CPortableExecutable g_pe;
CPortableExecutable g_dll;
CSectionImport g_import;
HINSTANCE g_hInst;
BOOL g_flLoaded;
BOOL g_flLoadedDLL;
BOOL g_flDupe;
HICON g_hIcon;

// Constants
const char cszAppName[] = "Import Adder";
const char cszAbout[] = "Import Adder by DEATH of Execution in 2002 (class version: %d.%d)";
const char cszNone[] = "None selected";

// Function prototypes
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void EnableOptions(HWND hwnd);
BOOL BrowseInputFile(HWND hwndOwner, HWND hwndFile);
void DisplayImports(HWND hwndParent, HWND hwndList);
BOOL BrowseOutputFile(HWND hwndOwner, LPSTR lpOutputFile, DWORD cbOutputFile);
BOOL CALLBACK DlgSelect(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DisplayExports(HWND hwndParent, HWND hwndList);
BOOL AddImport(HWND hwndParent, HWND hwndDLL, HWND hwndFunc);
LPSTR GetFileName(LPSTR pszPath);
BOOL IsDupe(HWND hwndParent, HWND hwndList, HWND hwndDLL, HWND hwndFunc);

// Main programme
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInst = hInstance;
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgProc);
    return(0);
}

// Dialog procedure
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char szBuffer[256];
    PE_ERROR error;

    switch (uMsg) {
        case WM_INITDIALOG:
            // Initialize dialog
            g_flLoaded = FALSE;
            g_flLoadedDLL = FALSE;
            g_flDupe = TRUE;
            EnableOptions(hDlg);
            g_hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_MAIN));
            SendMessage(hDlg, WM_SETICON, (WPARAM )ICON_BIG, (LPARAM )g_hIcon);
            break;
        case WM_COMMAND:
            // Dialog command
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    // Button clicked
                    switch (LOWORD(wParam)) {
                        case IDC_ADD:
                            // Add import
                            if (AddImport(hDlg, GetDlgItem(hDlg, IDT_DLL), GetDlgItem(hDlg, IDC_IMPORT)) == TRUE) {
                                g_import.Update();
                                DisplayImports(hDlg, GetDlgItem(hDlg, IDC_IMPORTSLIST));
                                g_flDupe = TRUE;
                                EnableOptions(hDlg);
                            }
                            break;
                        case IDC_BROWSE:
                            // Browse PE
                            if (BrowseInputFile(hDlg, GetDlgItem(hDlg, IDT_FILE)) == TRUE) {
                                // Load file
                                GetDlgItemText(hDlg, IDT_FILE, szBuffer, sizeof(szBuffer));
                                error = g_pe.Load(szBuffer);
                                if (error != PE_NONE) {
									wsprintf(szBuffer, "Error: %s", CUtil::GetErrorString(error));
                                    MessageBox(hDlg, szBuffer, cszAppName, MB_OK | MB_ICONERROR);
                                    break;
                                }

                                // Check if imports directory initialized
                                if (g_pe.m_ddImport.IsInitialized() == FALSE) {
                                    MessageBox(hDlg, "Cannot manipulate executable", cszAppName, MB_OK | MB_ICONERROR);
                                    break;
                                }
                                
                                // Set loaded flag
                                g_flLoaded = TRUE;

                                // Create new import section
                                g_import.Attach(&g_pe, "impadd");
                                g_import.Copy();
                                g_import.Update();
                                
                                // Display imports
                                DisplayImports(hDlg, GetDlgItem(hDlg, IDC_IMPORTSLIST));

                                EnableOptions(hDlg);
                            }
                            break;
                        case IDC_BROWSEDLL:
                            // Browse PE
                            if (BrowseInputFile(hDlg, GetDlgItem(hDlg, IDT_DLL)) == TRUE) {
                                // Load file
                                GetDlgItemText(hDlg, IDT_DLL, szBuffer, sizeof(szBuffer));
                                error = g_dll.Load(szBuffer);
                                if (error != PE_NONE) {
									wsprintf(szBuffer, "Error: %s", CUtil::GetErrorString(error));
                                    MessageBox(hDlg, szBuffer, cszAppName, MB_OK | MB_ICONERROR);
                                    break;
                                }

                                // Check if exports directory initialized
                                if (g_dll.m_ddExport.IsInitialized() == FALSE) {
                                    MessageBox(hDlg, "PE has no exports!", cszAppName, MB_OK | MB_ICONERROR);
                                    break;
                                }
                                
                                // Set loaded flag
                                g_flLoadedDLL = TRUE;

                                // Set function name
                                SetDlgItemText(hDlg, IDC_IMPORT, cszNone);

                                EnableOptions(hDlg);
                            }
                            break;
                        case IDC_EXIT:
                            // Exit programme
                            PostMessage(hDlg, WM_CLOSE, (WPARAM )0, (LPARAM )0);
                            break;
                        case IDC_SAVE:
                            // Save PE
                            if (g_flLoaded == TRUE) {
                                if (BrowseOutputFile(hDlg, szBuffer, sizeof(szBuffer)) == TRUE) {
                                    g_import.Update();
                                    g_pe.Write(szBuffer);
                                }
                            }
                            break;
                        case IDC_ABOUT:
                             // About programme
                            wsprintf(szBuffer, cszAbout, PE_MAJVER, PE_MINVER);
                            MessageBox(hDlg, szBuffer, cszAppName, MB_OK | MB_ICONINFORMATION);
                            break;
                        case IDC_SELECT:
                            // Select import to add
                            DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_SELECT), hDlg, DlgSelect, (LPARAM )GetDlgItem(hDlg, IDC_IMPORT));
                            g_flDupe = IsDupe(hDlg, GetDlgItem(hDlg, IDC_IMPORTSLIST), GetDlgItem(hDlg, IDT_DLL), GetDlgItem(hDlg, IDC_IMPORT));
                            EnableOptions(hDlg);
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

// Enable/Disable dialog options
void EnableOptions(HWND hwnd)
{
    if (g_flLoaded == FALSE) {
        // No PE loaded
        EnableWindow(GetDlgItem(hwnd, IDC_SAVE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_BROWSEDLL), FALSE);
    } else {
        EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_BROWSEDLL), TRUE);
    }

    if (g_flLoadedDLL == FALSE) {
        EnableWindow(GetDlgItem(hwnd, IDC_ADD), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_SELECT), FALSE);
    } else {
        if (g_flDupe == FALSE)
            EnableWindow(GetDlgItem(hwnd, IDC_ADD), TRUE);
        else
            EnableWindow(GetDlgItem(hwnd, IDC_ADD), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_SELECT), TRUE);
    }
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

// Display imports in a listbox
void DisplayImports(HWND hwndParent, HWND hwndList)
{
    DWORD dwIndexDLL;
    DWORD dwIndexFunc;
    DWORD dwLength;
    LPSTR pszNameDLL;
    LPSTR pszNameFunc;
    LPSTR pszBuffer;
    DWORD dwOrdinal;

    SendMessage(hwndList, LB_RESETCONTENT, (WPARAM )0, (LPARAM )0);

    if (g_pe.m_ddImport.IsInitialized() == TRUE) {
        for (dwIndexDLL = 0; dwIndexDLL < g_pe.m_ddImport.GetNumEntries(); dwIndexDLL++) {
            dwLength = g_pe.m_ddImport.GetDLLName(dwIndexDLL, NULL);
            if (dwLength) {
                
                pszNameDLL = new char [dwLength + 1];
                if (pszNameDLL) {
                
                    // Get DLL name
                    g_pe.m_ddImport.GetDLLName(dwIndexDLL, pszNameDLL);
                    // Remove extension
                    if (strchr(pszNameDLL, '.')) *strchr(pszNameDLL, '.') = '\0';
                    // Convert to uppercase
                    strupr(pszNameDLL);

                    for (dwIndexFunc = 0; dwIndexFunc < g_pe.m_ddImport.GetNumFunctions(dwIndexDLL); dwIndexFunc++) {
                        
                        if (g_pe.m_ddImport.IsOrdinal(dwIndexDLL, dwIndexFunc) == FALSE) {
                            
                            // Import by name
                            dwLength = g_pe.m_ddImport.GetFuncName(dwIndexDLL, dwIndexFunc, NULL);
                            if (dwLength) {
                            
                                pszNameFunc = new char [dwLength + 1];

                                if (pszNameFunc) {
    
                                    g_pe.m_ddImport.GetFuncName(dwIndexDLL, dwIndexFunc, pszNameFunc);
                                    pszBuffer = new char [lstrlen(pszNameDLL) + lstrlen(pszNameFunc) + 2];
    
                                    if (pszBuffer) {
                                            
                                        wsprintf(pszBuffer, "%s.%s", pszNameDLL, pszNameFunc);
                                        SendMessage(hwndList, LB_ADDSTRING, (WPARAM )0, (LPARAM )pszBuffer);
    
                                        delete [] pszBuffer;

                                    }
    
                                    delete [] pszNameFunc;
    
                                }
    
                            } 
                        
                        } else {

                            // Import by ordinal
                            dwOrdinal = g_pe.m_ddImport.GetOrdinal(dwIndexDLL, dwIndexFunc);

                            if (dwOrdinal != PE_INVALID) {

                                pszBuffer = new char [lstrlen(pszNameDLL) + 7];
                                
                                if (pszBuffer) {

                                    wsprintf(pszBuffer, "%s.#%04X", pszNameDLL, (WORD )dwOrdinal);
                                    SendMessage(hwndList, LB_ADDSTRING, (WPARAM )0, (LPARAM )pszBuffer);

                                    delete [] pszBuffer;

                                }

                            }

                        }
                    }

                    delete [] pszNameDLL;

                }

            }
        }
    }
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

// Select dialog procedure
BOOL CALLBACK DlgSelect(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndOutput;
    DWORD dwLength;
    LPSTR pszName;
    DWORD dwIndex;

    switch (uMsg) {
        case WM_INITDIALOG:
            // Initialize dialog
            DisplayExports(hDlg, GetDlgItem(hDlg, IDC_LIST));
            hwndOutput = (HWND )lParam;
            SetWindowText(hwndOutput, cszNone);
            break;
        case WM_COMMAND:
            // Dialog command
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    // Button clicked
                    switch (LOWORD(wParam)) {
                        case IDOK:
                            // OK button
                            dwIndex = (DWORD )SendMessage(GetDlgItem(hDlg, IDC_LIST), LB_GETCURSEL, (WPARAM )0, (LPARAM )0);
                            if (dwIndex != LB_ERR) {
                                dwLength = (DWORD )SendMessage(GetDlgItem(hDlg, IDC_LIST), LB_GETTEXTLEN, (WPARAM )dwIndex, (LPARAM )0);
                                if (dwLength) {
                                    pszName = new char [dwLength + 1];
                                    if (pszName) {
                                        SendMessage(GetDlgItem(hDlg, IDC_LIST), LB_GETTEXT, (WPARAM )dwIndex, (LPARAM )pszName);
                                        SetWindowText(hwndOutput, pszName);
                                        delete [] pszName;
                                    }
                                }
                            } else {
                                MessageBox(hDlg, "Please choose a function", cszAppName, MB_OK | MB_ICONERROR);
                                break;
                            }
                        case IDCANCEL:
                            // Cancel button
                            PostMessage(hDlg, WM_CLOSE, (WPARAM )0, (LPARAM )0);
                            break;
                        default:
                            break;
                    }
                    break;
                case LBN_DBLCLK:
                    // ListBox double click, same as OK button
                    PostMessage(hDlg, WM_COMMAND, (WPARAM )((BN_CLICKED << 16) | IDOK), (LPARAM )GetDlgItem(hDlg, IDOK));
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

// Display DLL exports
void DisplayExports(HWND hwndParent, HWND hwndList)
{
    WORD wOrdinal;
    IMAGE_EXPORT_DIRECTORY dirExp;
    LPSTR pszName;
    DWORD dwLength;
    char szBuffer[8];

    g_dll.m_ddExport.GetTable(&dirExp);
    
    for (wOrdinal = (WORD )dirExp.Base; wOrdinal < (WORD )(dirExp.Base + dirExp.NumberOfFunctions); wOrdinal++) {
        dwLength = g_dll.m_ddExport.GetNameByOrdinal(wOrdinal, NULL);
        if (dwLength) {
            // Name
            pszName = new char [dwLength + 1];
            if (pszName) {
                g_dll.m_ddExport.GetNameByOrdinal(wOrdinal, pszName);
                SendMessage(hwndList, LB_ADDSTRING, (WPARAM )0, (LPARAM )pszName);
                delete [] pszName;
            }
        } else {
            // Ordinal
            wsprintf(szBuffer, "#%04X", wOrdinal);
            SendMessage(hwndList, LB_ADDSTRING, (WPARAM )0, (LPARAM )szBuffer);
        }
    }
}

// Add an import
BOOL AddImport(HWND hwndParent, HWND hwndDLL, HWND hwndFunc)
{
    char szDLL[256];
    char szFunc[256];
    LPSTR pszNameDLL;
    DWORD dwOrdinal;

    GetWindowText(hwndDLL, szDLL, sizeof(szDLL));
    GetWindowText(hwndFunc, szFunc, sizeof(szFunc));
    pszNameDLL = GetFileName(szDLL);

    // Check if import function name is valid
    if (strchr(szFunc, ' ')) {
        MessageBox(hwndParent, "Please choose a valid function", cszAppName, MB_OK | MB_ICONERROR);
        return(FALSE);
    }

    // Check if it is ordinal
    dwOrdinal = PE_INVALID;
    if (szFunc[0] == '#') {
        dwOrdinal = strtol(&szFunc[1], NULL, 16);
    }

    // Add import
    if (dwOrdinal != PE_INVALID) {
        // Add by ordinal (hint=0)
        g_import.Add(pszNameDLL, (WORD )dwOrdinal);
    } else {
        // Add by name (hint=0)
        g_import.Add(pszNameDLL, szFunc);
    }

    return(TRUE);
}

// Get file name
LPSTR GetFileName(LPSTR pszPath)
{
    LPSTR pszFile;

    pszFile = pszPath;

    do {
        pszFile = strchr(pszFile, '\\') + 1;
    } while (strchr(pszFile, '\\'));

    return(pszFile);
}

// Check if import already found
BOOL IsDupe(HWND hwndParent, HWND hwndList, HWND hwndDLL, HWND hwndFunc)
{
    char szDLL[256];
    char szFunc[256];
    char szFullName[512];
    LPSTR pszNameDLL;
    DWORD dwIndex;

    // Create import name that matches the ones in the list
    GetWindowText(hwndDLL, szDLL, sizeof(szDLL));
    GetWindowText(hwndFunc, szFunc, sizeof(szFunc));
    
    // See if function name is valid
    if (strchr(szFunc, ' ')) {
        return(TRUE); // Lie and say it's a dupe (bad proggy! bad proggy!)
    }

    // Get DLL name in uppercase
    pszNameDLL = GetFileName(szDLL);
    strupr(pszNameDLL);
    
    // Remove extension
    if (strchr(pszNameDLL, '.')) *strchr(pszNameDLL, '.') = '\0';

    wsprintf(szFullName, "%s.%s", pszNameDLL, szFunc);

    dwIndex = (DWORD )SendMessage(hwndList, LB_FINDSTRINGEXACT, (WPARAM )-1, (LPARAM )szFullName);

    if (dwIndex != LB_ERR) {
        MessageBox(hwndParent, "Function already imported", cszAppName, MB_OK | MB_ICONERROR);
        return(TRUE); // Dupe
    }

    return(FALSE); // Not a dupe
}
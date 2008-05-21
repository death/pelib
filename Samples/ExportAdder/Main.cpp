// Includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cstring>

#include "resource.h"
#include "PortableExecutable.h"
#include "Util.h"
#include "SectionExport.h"

using namespace PE;
using namespace std;

// Definitions
#define NUM_COLS (sizeof(ce) / sizeof(COLUMN_ENTRY))

// Structures
typedef struct {
    char szText[64];
    DWORD dwPercent;
} COLUMN_ENTRY;

// Static variables
static COLUMN_ENTRY ce[] = {
    {"Ordinal", 20},
    {"Name", 45},
    {"RVA", 25}
};

// Global variables
HINSTANCE g_hInst;
DWORD g_dwColumnIndex;
DWORD g_dwItemIndex;
CPortableExecutable g_pe;
CSectionExport g_export;
BOOL g_flLoaded;
HICON g_hIcon;

// Constants
const char cszAppName[] = "Export Adder";
const char cszAbout[] = "Export Adder by DEATH of Execution in 2002 (class version: %d.%d)";

// Function prototypes
BOOL CALLBACK DlgMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AddColumns(HWND hDlg);
void AddColumn(HWND hwndList, LPSTR pszText, DWORD dwWidthPercent);
BOOL BrowseInputFile(HWND hwndOwner, HWND hwndFile);
void DisplayExports(HWND hwndOwner, HWND hwndList);
void AddExportToList(HWND hwndList, WORD wOrdinal, LPSTR pszName, DWORD dwAddress);
BOOL BrowseOutputFile(HWND hwndOwner, LPSTR lpOutputFile, DWORD cbOutputFile);
BOOL AddExport(HWND hwndOwner, HWND hwndName, HWND hwndAddress);
void DisplayOrdinal(HWND hwndOrdinal);
void EnableOptions(HWND hwnd);
LPSTR GetFileName(LPSTR pszPath);

// Main programme
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    InitCommonControls();
    g_hInst = hInstance;
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgMain);
    return(0);
}

// Main dialog function
BOOL CALLBACK DlgMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char szBuffer[256];
    PE_ERROR error;

    switch (uMsg) {
    case WM_INITDIALOG:
        // Initialize dialog
        AddColumns(hDlg);
        g_flLoaded = FALSE;
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
            case IDC_EXIT:
                // Exit programme
                PostMessage(hDlg, WM_CLOSE, (WPARAM )0, (LPARAM )0);
                break;
            case IDC_ABOUT:
                // About programme
                wsprintf(szBuffer, cszAbout, PE_MAJVER, PE_MINVER);
                MessageBox(hDlg, szBuffer, cszAppName, MB_OK | MB_ICONINFORMATION);
                break;
            case IDC_SAVE:
                // Save PE
                if (g_flLoaded == TRUE) {
                    if (BrowseOutputFile(hDlg, szBuffer, sizeof(szBuffer)) == TRUE) {
                        g_export.Update();
                        g_pe.Write(szBuffer);
                    }
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

                    // Set loaded flag
                    g_flLoaded = TRUE;

                    // Create export section
                    if (g_pe.m_ddExport.IsInitialized() == TRUE)
                        g_export.Copy(&g_pe.m_ddExport, "exadd");
                    else
                        g_export.Create("exadd", GetFileName(szBuffer));

                    g_export.Attach(&g_pe);
                    g_export.Update();
                    
                    // Display exports
                    DisplayExports(hDlg, GetDlgItem(hDlg, IDC_EXPORTSLIST));

                    // Display next ordinal
                    DisplayOrdinal(GetDlgItem(hDlg, IDT_ORDINAL));

                    EnableOptions(hDlg);
                }
                break;
            case IDC_ADD:
                // Add export
                if (g_flLoaded == TRUE) {
                    if (AddExport(hDlg, GetDlgItem(hDlg, IDT_FUNCNAME), GetDlgItem(hDlg, IDT_RVA)) == TRUE) {
                        g_export.Update();
                        DisplayExports(hDlg, GetDlgItem(hDlg, IDC_EXPORTSLIST));
                        DisplayOrdinal(GetDlgItem(hDlg, IDT_ORDINAL));
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

// Add columns to listview
void AddColumns(HWND hDlg)
{
    DWORD dwCount;

    for (dwCount = 0; dwCount < NUM_COLS; dwCount++) {
        AddColumn(GetDlgItem(hDlg, IDC_EXPORTSLIST), ce[dwCount].szText, ce[dwCount].dwPercent);
    }
}

// Add a column to listview
void AddColumn(HWND hwndList, LPSTR pszText, DWORD dwWidthPercent)
{
    LVCOLUMN col;
    RECT rect;
    DWORD dwWidth;

    // Get width of control
    GetClientRect(hwndList, &rect);
    dwWidth = rect.right - rect.left;

    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.pszText = pszText;
    col.cx = dwWidthPercent * dwWidth / 100;    // Convert percentage of width to width
    ListView_InsertColumn(hwndList, g_dwColumnIndex, &col);
    g_dwColumnIndex++;
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

// Display exports in listview
void DisplayExports(HWND hwndOwner, HWND hwndList)
{
    WORD wOrdinal;
    IMAGE_EXPORT_DIRECTORY imgExport;
    DWORD dwAddress;
    LPSTR pszName;
    DWORD dwLength;

    ListView_DeleteAllItems(hwndList);
    g_dwItemIndex = 0;

    if (g_pe.m_ddExport.IsInitialized() == TRUE) {
        // There are exports to display
        g_pe.m_ddExport.GetTable(&imgExport);

        for (wOrdinal = (WORD )imgExport.Base; wOrdinal < (imgExport.Base + imgExport.NumberOfFunctions); wOrdinal++) {
            // Add export to listview
            dwAddress = g_pe.m_ddExport.GetExportAddressByIndex(wOrdinal - imgExport.Base);
            
            dwLength = g_pe.m_ddExport.GetNameByOrdinal(wOrdinal, NULL);
            if (dwLength) {
                pszName = new char [dwLength + 1];
                if (pszName) {
                    g_pe.m_ddExport.GetNameByOrdinal(wOrdinal, pszName);
                    AddExportToList(hwndList, wOrdinal, pszName, dwAddress);
                    delete [] pszName;
                }
            } else {
                AddExportToList(hwndList, wOrdinal, NULL, dwAddress);
            }
        }
    }
}

// Add an export entry to listview
void AddExportToList(HWND hwndList, WORD wOrdinal, LPSTR pszName, DWORD dwAddress)
{
    LVITEM item;
    char szBuffer[64];

    item.iItem = g_dwItemIndex;
    item.iSubItem = 0;
    item.mask = LVIF_TEXT;
    wsprintf(szBuffer, "%d", wOrdinal);
    item.pszText = szBuffer;

    ListView_InsertItem(hwndList, &item);
    if (pszName) {
        ListView_SetItemText(hwndList, item.iItem, ++item.iSubItem, pszName);
    } else {
        item.iSubItem++;
    }

    wsprintf(szBuffer, "%08lX", dwAddress);
    ListView_SetItemText(hwndList, item.iItem, ++item.iSubItem, szBuffer);

    g_dwItemIndex++;
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

BOOL AddExport(HWND hwndOwner, HWND hwndName, HWND hwndAddress)
{
    char szBuffer[256];
    DWORD dwAddress;

    // Get address
    GetWindowText(hwndAddress, szBuffer, sizeof(szBuffer));
    if (szBuffer[0] == '\0') {
        MessageBox(hwndOwner, "Enter function address", cszAppName, MB_OK | MB_ICONERROR);
        return(FALSE);
    }

    dwAddress = strtol(szBuffer, NULL, 16);

    // Get name
    GetWindowText(hwndName, szBuffer, sizeof(szBuffer));

    if (szBuffer[0]) {
        // Add function with name
        g_export.Add(szBuffer, g_export.NewOrdinal(), dwAddress);
    } else {
        // Add function with ordinal
        g_export.Add(NULL, g_export.NewOrdinal(), dwAddress);
    }

    return(TRUE);
}

// Display next ordinal number
void DisplayOrdinal(HWND hwndOrdinal)
{
    char szBuffer[256];

    wsprintf(szBuffer, "%d", g_export.NewOrdinal());
    SetWindowText(hwndOrdinal, szBuffer);
}

// Enable/Disable dialog options
void EnableOptions(HWND hwnd)
{
    if (g_flLoaded == FALSE) {
        // No PE loaded
        EnableWindow(GetDlgItem(hwnd, IDC_SAVE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ADD), FALSE);
    } else {
        EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_ADD), TRUE);
    }
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
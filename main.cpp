//would use all wide-string interally, since this is windows-only
//printing would be in utf8 since wcout suck

//link the icon file to object file: "ld -r -b binary -o goicon.o goicon.ico"
//then compile and link with -lole32 -loleaut32 -luuid -lshlwapi (and the icon object file ofc)

// #define _WIN32_WINNT _WIN32_WINNT_WIN7
// #define NTDDI_VERSION 0x06000000


#include <windows.h>
#include <shlobj.h>
#include <objidl.h>
#include <shellapi.h>
#include <shlwapi.h>

#include <stdio.h>
#include <assert.h>

//evil marcos
#define func(x) assert(SUCCEEDED(x))
#define inloop_func(x) if(!SUCCEEDED(x)) {\
    printf("Error code: %lu\n", x);\
    goto __FUNC_END_LOOP;\
}

//for the builtin icon file
//linking format: for file goicon.ico, the symbol would be _binary_goicon_ico_start
extern char _binary_goicon_ico_start;
extern char _binary_goicon_ico_end;
static const char* goicon_ico = &_binary_goicon_ico_start;
static const size_t goicon_ico_size = &_binary_goicon_ico_end - &_binary_goicon_ico_start;

//no c++ stl lul, handmade implementation
//non-zero: success, zero otherwise
long long my_hash_function(char *str, long long num1, long long num2, long long num3) {
    const long long const_multiplier = (1LL << 8)+1;
    const long long mod[3] = {(119LL << 23)+1,  1000696969LL, 232499LL};
    long long val[3] = {0,0,0};
    long long multiplier[3] = {const_multiplier, const_multiplier, const_multiplier};
    while(*str != L'\0') {
        val[0] = (val[0] + (multiplier[0] * ((*str) + 1))) % mod[0];
        val[1] = (val[1] + (multiplier[1] * ((*str) + 1))) % mod[1];
        val[2] = (val[2] + (multiplier[2] * ((*str) + 1))) % mod[2];
        multiplier[0] = (multiplier[0] * const_multiplier) % mod[0];
        multiplier[1] = (multiplier[1] * const_multiplier) % mod[1];
        multiplier[2] = (multiplier[2] * const_multiplier) % mod[2];
        str++;
    }

    return (val[0] == num1) & (val[1] == num2) & (val[2] == num3);
}

long long __gen_my_hash_function(char *str, long long *num1, long long *num2, long long *num3) {
    const long long const_multiplier = (1LL << 8)+1;
    const long long mod[3] = {(119LL << 23)+1,  1000696969LL, 232499LL};
    long long val[3] = {0,0,0};
    long long multiplier[3] = {const_multiplier, const_multiplier, const_multiplier};
    while(*str != L'\0') {
        val[0] = (val[0] + (multiplier[0] * ((*str) + 1))) % mod[0];
        val[1] = (val[1] + (multiplier[1] * ((*str) + 1))) % mod[1];
        val[2] = (val[2] + (multiplier[2] * ((*str) + 1))) % mod[2];
        multiplier[0] = (multiplier[0] * const_multiplier) % mod[0];
        multiplier[1] = (multiplier[1] * const_multiplier) % mod[1];
        multiplier[2] = (multiplier[2] * const_multiplier) % mod[2];
        str++;
    }

    *num1 = val[0];
    *num2 = val[1];
    *num2 = val[2];
}

void version() {
    #if defined(__GNUC__)
        printf("Built with GCC %d.%d.%d ", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
        #if defined(__MINGW64__)
            printf("(MinGW-w64 %d.%d) ", __MINGW64_VERSION_MAJOR, __MINGW64_VERSION_MINOR);
        #elif defined(__MINGW32__)
            printf("(MinGW %d.%d) ", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
        #endif
    #elif defined(_MSC_VER)
        printf("Built with MSVC %d ", _MSC_VER);
    #else
        printf("Built with unknown compiler ");
    #endif
        printf("on %s at %s. %d-bit build", __DATE__, __TIME__, sizeof(void*) << 3);
    exit(0);
}

void recover() {
    printf("Not implemented\n");
    exit(EXIT_FAILURE);
}


/// @brief Find all shortcut in the folder specified by the CDISL id and "edit" it
/// @param cdisl_id the CDISL id of the folder
/// @param is_safe indicate if original icon backup is required
/// @param backupfolder_name the name, for the backup folder in AppData
/// @param errorstring the return error string if error occured 
/// @return zero if success, non-zero otherwise
long proceed_shortcut(int cdisl_id, int is_safe, WCHAR *backupfolder_name, char *errorstring) {
    //dirdeskpath: directory path with trailing splash for appending file
    //deskpath: directory path contains '*' for file searching
    //linkpath: full path of the .lnk file
    //old_iconpath: full path of the OLD icon of the .lnk file
    //iconpath: full path of the NEW icon (stored in AppData, would replace the old one in .lnk file)
    //filepath: full path of the file pointed by .lnk file
    //backuplinkpath: full path of the backup .lnk file
    //backupstorepath: full path of the backup folder in AppData 
    //extname: extension of current lookup shortcut file (check for .lnk file)
    WCHAR deskpath[5002], dirdeskpath[5002], linkpath[5002], old_iconpath[5002], filepath[5002], iconpath[5002],
            backuplinkpath[5002], backupstorepath[5002], *extname;
    HANDLE findfilehnd; 
    WIN32_FIND_DATAW filedataptr;
    IShellLinkW *ishlinkptr;
    IPersistFile *ipfileptr;
    long res = 0;

    res = SHGetFolderPathW(NULL, cdisl_id, NULL, 0, dirdeskpath);
    if(SUCCEEDED(res)) {            //long if-else begin

        wcscat(dirdeskpath, L"\\");
        wcscpy(deskpath, dirdeskpath);
        wcscat(deskpath, L"*");

        findfilehnd = FindFirstFileW(deskpath, &filedataptr);
        do {
            extname = PathFindExtensionW(filedataptr.cFileName);
            if(!wcscmp(extname, L".lnk")) {
                wcscpy(linkpath, dirdeskpath);
                wcscat(linkpath, filedataptr.cFileName);

                res = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**) &ishlinkptr);
                if(!SUCCEEDED(res)) {
                    strcpy(errorstring, "failed somewhere");
                    goto proceedshortcut__cleanup;
                }
                res = ishlinkptr->QueryInterface(IID_IPersistFile, (void**) &ipfileptr);
                if(!SUCCEEDED(res)) {
                    strcpy(errorstring, "failed somewhere");
                    goto proceedshortcut__cleanup;
                }
                res = ipfileptr->Load(linkpath, STGM_READ);
                if(!SUCCEEDED(res)) {
                    strcpy(errorstring, "failed somewhere");
                    goto proceedshortcut__cleanup;
                }
                res = ishlinkptr->Resolve(NULL, 1);
                if(!SUCCEEDED(res)) {
                    strcpy(errorstring, "failed somewhere");
                    goto proceedshortcut__cleanup;
                }

                //backup
                if(is_safe) {
                    wcscpy(backuplinkpath, backupstorepath);
                    wcscat(backuplinkpath, backupfolder_name);
                    wcscat(backuplinkpath, L"\\");
                    if(!CreateDirectoryW(backuplinkpath, NULL)) {
                        if(GetLastError() != ERROR_ALREADY_EXISTS) {
                            strcpy(errorstring, "Unexpected error occured (create backup directory)");
                            return ERROR_ALREADY_EXISTS;
                        }
                    }
                }

                res = ishlinkptr->SetIconLocation(iconpath, 0);
                if(!SUCCEEDED(res)) {
                    strcpy(errorstring, "failed somewhere");
                    goto proceedshortcut__cleanup;
                }
                res = ipfileptr->Save(linkpath, TRUE);
                if(!SUCCEEDED(res)) {
                    strcpy(errorstring, "failed somewhere (saving back)");
                    goto proceedshortcut__cleanup;
                }
            }
        } while(FindNextFileW(findfilehnd, &filedataptr));
        FindClose(findfilehnd);
    } else {
        strcpy(errorstring, "failed getting special folder");
        goto proceedshortcut__cleanup;
    }

    if(res >= 0) res = 0;
    proceedshortcut__cleanup:
    ishlinkptr->Release();
    ipfileptr->Release();
    return res;
}

int main(int argc, char* argv[]) {
    /*begin declaration*/
    //dirdeskpath: directory path with trailing splash for appending file
    //deskpath: directory path contains '*' for file searching
    //linkpath: full path of the .lnk file
    //old_iconpath: full path of the OLD icon of the .lnk file
    //iconpath: full path of the NEW icon (stored in AppData, would replace the old one in .lnk file)
    //filepath: full path of the file pointed by .lnk file
    //extname: extension of current lookup shortcut file (check for .lnk file)
    //backuplinkpath: full path of the backup .lnk file
    //backupstorepath: full path of the backup folder in AppData 
    WCHAR deskpath[5002], dirdeskpath[5002], linkpath[5002], old_iconpath[5002], filepath[5002], iconpath[5002],
            backuplinkpath[5002], backupstorepath[5002], *extname;
    CHAR mblinkpath[5002], mbiconpath[5002], mbfilepath[5002];
    IShellLinkW* ishlinkptr;
    IPersistFile* ipfileptr;
    WIN32_FIND_DATAW filedataptr;
    //int iconindex;
    int is_safe = true;

    WCHAR __sh_alloc_tempstr[MAX_PATH];
    /*end declaration*/

    //parse command line
    if(argc >= 2) {
        if(my_hash_function(argv[1], 95954239LL, 87563843LL, 107641LL)) {   //-version
            version();
        } else
        if(my_hash_function(argv[1], 7871653LL, 7871653LL, 199186LL)) {     //-v
            version();
        } else
        if(my_hash_function(argv[1], 745950439LL, 896514936LL, 33687LL)) {  //-nosafe or -ns
            is_safe = false;
        } else
        if(my_hash_function(argv[1], 978151696LL, 975699080LL, 154549LL)) { //-nosafe or -ns
            is_safe = false;
        } else
        if(my_hash_function(argv[1], 792000581LL, 281688927LL, 232331LL)) { //-recover
            recover();
        } else
        if(my_hash_function(argv[1], 7607457LL, 7607457LL, 167489LL)) {     //-r
            recover();
        } else {
            printf("Invalid argument\n");
            return EXIT_FAILURE;
        }
    }

    //init
    func(CoInitialize(NULL));

    func(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, __sh_alloc_tempstr));

    wcscpy(backupstorepath, __sh_alloc_tempstr);
    wcscat(backupstorepath, L"\\i_shell_link_shortcut_backup\\");
    if(!CreateDirectoryW(backupstorepath, NULL)) {
        if(GetLastError() != ERROR_ALREADY_EXISTS) {
            printf("Unexpected error occured (create main directory)");
            exit(EXIT_FAILURE);
        }
    }

    //burn the icon file
    wcscpy(iconpath, backupstorepath);
    wcscat(iconpath, L"goicon.ico");
    DWORD __ret__written_byte;
    HANDLE ico_file_handle = CreateFileW(iconpath,                      //for best power we use winapi
                                        GENERIC_READ | GENERIC_WRITE, 
                                        0, 
                                        NULL, 
                                        CREATE_ALWAYS, 
                                        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NORMAL,
                                        NULL);
    if(ico_file_handle == INVALID_HANDLE_VALUE) {
        printf("Unexpected error occured (ico_file_handle)");
        exit(EXIT_FAILURE);
    }
    if(!WriteFile(ico_file_handle, goicon_ico, goicon_ico_size, &__ret__written_byte, NULL) || __ret__written_byte == 0) {
        printf("Unexpected error occured (ico_file write)");
        exit(EXIT_FAILURE);
    }
    CloseHandle(ico_file_handle);


    char errstr[5001];
    proceed_shortcut(CSIDL_DESKTOPDIRECTORY, is_safe, L"Desktop", errstr);

    proceed_shortcut(CSIDL_COMMON_DESKTOPDIRECTORY, is_safe, L"PublicDesktop", errstr);


    CoUninitialize();
    return EXIT_SUCCESS;
}
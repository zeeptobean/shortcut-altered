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

//old old mingw gcc version doesn't have SHGetKnownFolderPath and others defined in its
//Windows headers. This marco would filter all of them
#if __GNUC__ < 11
#define __MINGW_OLD_HEADER
#endif

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

    if(val[0] == num1 && val[1] == num2 && val[2] == num3) {
        return num1;
    } 
    return 0;
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
        printf("at %s %s", __DATE__, __TIME__);
    exit(0);
}

void recover() {
    printf("Not implemented\n");
    exit(EXIT_FAILURE);
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
    int iconindex;
    bool __is_main_run = false, is_safe = true;

    #ifdef __MINGW_OLD_HEADER
    WCHAR __sh_alloc_tempstr[MAX_PATH];
    #else
    WCHAR *__sh_alloc_tempstr;
    #endif // __MINGW_OLD_HEADER
    /*end declaration*/

    //parse command line
    if(argc >= 2) {
        if(my_hash_function(argv[1], 95954239LL, 87563843LL, 107641LL)) {
            version();
        } else
        if(my_hash_function(argv[1], 7871653LL, 7871653LL, 199186LL)) {
            version();
        } else
        if(my_hash_function(argv[1], 745950439LL, 896514936LL, 33687LL)) {
            is_safe = false;
        } else
        if(my_hash_function(argv[1], 978151696LL, 975699080LL, 154549LL)) {
            is_safe = false;
        } else
        if(my_hash_function(argv[1], 792000581LL, 281688927LL, 232331LL)) {
            recover();
        } else
        if(my_hash_function(argv[1], 7607457LL, 7607457LL, 167489LL)) {
            recover();
        } else {
            printf("Invalid argument\n");
            return EXIT_FAILURE;
        }
    }

    //init
    func(CoInitialize(NULL));

    //create directory in AppData
    #ifdef __MINGW_OLD_HEADER
    func(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, __sh_alloc_tempstr));
    #else
    func(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &__sh_alloc_tempstr));
    #endif

    wcscpy(backupstorepath, __sh_alloc_tempstr);
    wcscat(backupstorepath, L"\\i_shell_link_shortcut_backup\\");
    if(!CreateDirectoryW(backupstorepath, NULL)) {
        if(GetLastError() != ERROR_ALREADY_EXISTS) {
            printf("Unexpected error occured (create main directory)");
            exit(EXIT_FAILURE);
        }
    }

    #ifndef __MINGW_OLD_HEADER
    CoTaskMemFree(__sh_alloc_tempstr);
    #endif

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


    //get local desktop
    #ifdef __MINGW_OLD_HEADER
    func(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, __sh_alloc_tempstr));
    #else
    func(SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &__sh_alloc_tempstr));
    #endif

    __main_run:
    wcscpy(dirdeskpath, __sh_alloc_tempstr);
    wcscat(dirdeskpath, L"\\");
    wcscpy(deskpath, dirdeskpath);
    wcscat(deskpath, L"*");

    #ifndef __MINGW_OLD_HEADER
    CoTaskMemFree(__sh_alloc_tempstr);
    #endif

    HANDLE findfilehnd = FindFirstFileW(deskpath, &filedataptr);
    do {
        extname = PathFindExtensionW(filedataptr.cFileName);
        if(wcscmp(extname, L".lnk") == 0) {

            wcscpy(linkpath, dirdeskpath);
            wcscat(linkpath, filedataptr.cFileName);

            WideCharToMultiByte(CP_UTF8, 0, linkpath, -1, mblinkpath, 5000, NULL, NULL);
            printf("%s\n", mblinkpath);

            inloop_func(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**) &ishlinkptr));
            inloop_func(ishlinkptr->QueryInterface(IID_IPersistFile, (void**) &ipfileptr));
            inloop_func(ipfileptr->Load(linkpath, STGM_READ));
            inloop_func(ishlinkptr->Resolve(NULL, 1));
            inloop_func(ishlinkptr->GetIconLocation(old_iconpath, 5000, &iconindex));
            inloop_func(ishlinkptr->GetPath(filepath, 5000, NULL, SLGP_RAWPATH));

            if(is_safe) {
                wcscpy(backuplinkpath, backupstorepath);
                wcscat(backuplinkpath, L"Desktop\\");   //TODO seperate 2 different desktop backup
                if(!CreateDirectoryW(backuplinkpath, NULL)) {
                    if(GetLastError() != ERROR_ALREADY_EXISTS) {
                        printf("Unexpected error occured (create backup directory)");
                        exit(EXIT_FAILURE);
                    }
                }
                wcscat(backuplinkpath, filedataptr.cFileName);
                
                if(!CopyFileW(linkpath, backuplinkpath, FALSE)) {
                    printf("Unexpected error occured (CopyFileW)");
                    exit(EXIT_FAILURE);
                }
                // //debug
                // WideCharToMultiByte(CP_UTF8, 0, linkpath, -1, mbiconpath, 5000, NULL, NULL);
                // printf("Backup-ed path: %s\n", mbiconpath);
            }

            WideCharToMultiByte(CP_UTF8, 0, old_iconpath, -1, mbiconpath, 5000, NULL, NULL);
            WideCharToMultiByte(CP_UTF8, 0, filepath, -1, mbfilepath, 5000, NULL, NULL);
            printf("File location: %s \nIcon path: %s \nIcon index: %d\n", mbfilepath, mbiconpath, iconindex);

            inloop_func(ishlinkptr->SetIconLocation(iconpath, 0));
            inloop_func(ipfileptr->Save(linkpath, TRUE));

            inloop_func(ishlinkptr->GetIconLocation(old_iconpath, 5000, &iconindex));
            WideCharToMultiByte(CP_UTF8, 0, old_iconpath, -1, mbiconpath, 5000, NULL, NULL);
            WideCharToMultiByte(CP_UTF8, 0, filepath, -1, mbfilepath, 5000, NULL, NULL);
            printf("File location: %s \nIcon path: %s \nIcon index: %d\n\n\n", mbfilepath, mbiconpath, iconindex);

            __FUNC_END_LOOP:
            ipfileptr->Release();
            ishlinkptr->Release();
        }
    } while(FindNextFileW(findfilehnd, &filedataptr) != 0);
    FindClose(findfilehnd);

    //old old mingw gcc version doesn't have SHGetKnownFolderPath defined
    //get public desktop
    #ifdef __MINGW_OLD_HEADER
    func(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, __sh_alloc_tempstr));
    #else
    func(SHGetKnownFolderPath(FOLDERID_PublicDesktop, 0, NULL, &__sh_alloc_tempstr));
    #endif

    if(!__is_main_run) {
        __is_main_run = true;
        goto __main_run;
    }

    CoUninitialize();
    return EXIT_SUCCESS;
}
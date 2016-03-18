#include "dk_createdir.h"
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif //WIN32
#include <string.h>
#include <stdio.h>
#include <errno.h>
//
#ifdef WIN32
    #define SLASH ('\\')
    int dk_createdir_win32(const char* szDirPath);
#else
    #define SLASH ('/')
    int dk_createdir_linux(const char* szDirPath);
#endif //WIN32

int dk_createdir( const char* szDirPath ){

#ifdef WIN32
    return dk_createdir_win32( szDirPath );
#else
    return dk_createdir_linux( szDirPath );
#endif //WIN32

}

#ifdef WIN32
int dk_createdir_win32(const char* szDirPath) {
    if ( !szDirPath || strlen(szDirPath) == 0 )
        return -1;
    unsigned int uLenDirPath = strlen( szDirPath );
    int bEndWithSlash = (szDirPath[ uLenDirPath - 1 ] == SLASH);
    char* pDestDirPath = (char*)malloc( uLenDirPath + ( bEndWithSlash ? 1 : 2) );
    char* pPosSlash = 0;
    int nRet = 0;
    DWORD dwFileAttri = 0;

    //
    memcpy( pDestDirPath, szDirPath, uLenDirPath );
    if ( bEndWithSlash ) {
        pDestDirPath[ uLenDirPath ] = 0;
    } else {
        pDestDirPath[ uLenDirPath ] = SLASH;
        pDestDirPath[ uLenDirPath + 1 ] = 0;
    }
    //
    pPosSlash = pDestDirPath;
    while ( (pPosSlash = strchr( pPosSlash+1, SLASH )) != 0 ) {
        *pPosSlash = 0;
        dwFileAttri = GetFileAttributesA( pDestDirPath );
        if ( INVALID_FILE_ATTRIBUTES == dwFileAttri
        || !( dwFileAttri & FILE_ATTRIBUTE_DIRECTORY ) ) {
            if ( !CreateDirectoryA( pDestDirPath, NULL ) ) {
                printf( "Create Directory failed. path:%s errno:%d\n", pDestDirPath, errno );
                free( pDestDirPath );
                return -1;
            }
        }
        *pPosSlash = SLASH;
    }

    free( pDestDirPath );
    pDestDirPath = NULL;
    return 0;
}
#else
//
int dk_createdir_linux( const char* szDirPath ) {
    if ( !szDirPath || strlen(szDirPath) == 0 )
        return -1;
    unsigned int uLenDirPath = strlen( szDirPath );
    int bEndWithSlash = (szDirPath[ uLenDirPath - 1 ] == SLASH);
    char* pDestDirPath = (char*)malloc( uLenDirPath + bEndWithSlash ? 1 : 2 );
    char* pPosSlash = 0;
    int nRet = 0;
    struct stat stat_st;

    //
    memcpy( pDestDirPath, szDirPath, uLenDirPath );
    if ( bEndWithSlash ) {
        pDestDirPath[ uLenDirPath ] = 0;
    } else {
        pDestDirPath[ uLenDirPath ] = SLASH;
        pDestDirPath[ uLenDirPath + 1 ] = 0;
    }
    //
    pPosSlash = pDestDirPath;
    while ( (pPosSlash = strchr( pPosSlash+1, SLASH )) != 0 ) {
        *pPosSlash = 0;    
        nRet = stat( pDestDirPath, &stat_st );
        if ( nRet != 0 || !S_ISDIR( stat_st.st_mode) ) {
            if ( 0 != mkdir( pDestDirPath, 0775 ) ) {
                printf( "mkdir failed. path:%s, errno:%d\n", pDestDirPath, errno );
                free( pDestDirPath );
                return -1; 
            }
        }
        *pPosSlash = SLASH;
    }

    free( pDestDirPath );
    return 0;
}
#endif //WIN32







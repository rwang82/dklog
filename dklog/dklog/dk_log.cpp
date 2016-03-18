#include "dk_log.h"
#include "dk_createdir.h"
#include "dk_block_cache.h"
#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <pwd.h>
#include <pthread.h>
#include <sys/time.h>
#endif //WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
//
#define DKLOG_BIT_HITTEST( val, bit ) ( ( (val) & (bit) ) == (bit) )
#define DKLOG_BIT_CLEAR( val, bit ) ( (val) &= ~(bit) )
#define DKLOG_BIT_ADD( val, bit ) ( (val) |= (bit) )
//
#define LEN_DIRPATH_MAX (256)
#define LEN_PREFIXED_MAX (30)
#define LEN_DATATIME_MAX (20)
#define LEN_LOGFILEPATH_MAX (LEN_DIRPATH_MAX+LEN_PREFIXED_MAX+LEN_DATATIME_MAX)
#define LEN_DKLOG_CONTENT_MAX ( 1*1024*1024 )
#define SIZE_DKLOG_CACHE_MAX ( 2*LEN_DKLOG_CONTENT_MAX )
//
#define DKLOG_FLAG_NONE (0x00000000)
#define DKLOG_FLAG_EXIT_ALL (0x00000001)
//
static void _dklog_vlogf( const char* szLevel, const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, va_list ap );
static void _dklog_save_thread_win32( void* pParam );
static void* _dklog_save_thread( void* pParam );
//
struct dklog {
    unsigned int m_uFlag;
    char m_szDirPath[ LEN_DIRPATH_MAX+1 ];
    char m_szPrefixed[ LEN_PREFIXED_MAX+1 ];
    char m_szLogFilePath[ LEN_LOGFILEPATH_MAX + 1 ];
    char m_szLogContent[ LEN_DKLOG_CONTENT_MAX + 1 ];
    struct dk_block_cache* m_cache;
#ifdef WIN32
    HANDLE m_hLogFile;
    HANDLE m_hEventAccess;
#else
    int m_fd;
    pthread_mutex_t m_lock;
    pthread_t m_threadSave;
#endif //WIN32
};
//
static struct dklog* s_dklog = 0;
//
int dklog_init( const char* szDirPath, const char* szPrefixed ) {
    if ( s_dklog != 0 ) return -1;
    char szDirPathDefault[ LEN_DIRPATH_MAX ];
    time_t t;
    struct tm* pCurTM = 0;
    
    //
    s_dklog = ( struct dklog* )malloc( sizeof( struct dklog ) );
    memset( s_dklog, 0, sizeof( struct dklog ) );
    s_dklog->m_uFlag = DKLOG_FLAG_NONE;
#ifdef WIN32
    s_dklog->m_hEventAccess = ::CreateEvent( NULL, FALSE, TRUE, NULL );
#else
    pthread_mutex_init( &s_dklog->m_lock, 0 );
#endif //WIN32
    //
    s_dklog->m_cache = dk_block_create( SIZE_DKLOG_CACHE_MAX );
    if ( !s_dklog->m_cache ) goto err;
    // get szDirPath
    if ( !szDirPath || strlen(szDirPath) == 0 ) {

#ifdef WIN32
        sprintf_s( szDirPathDefault, "C:\\log" );
#else
        // get current username. 
        uid_t uid;
        struct passwd* pwd_st;
    
        uid = getuid();
        pwd_st = getpwuid( uid );
        if ( 0 == strcmp( "root", pwd_st->pw_name ) ) {
            strcpy( szDirPathDefault, "/root/" );
        } else {
            sprintf( szDirPathDefault, "/home/%s", pwd_st->pw_name );
        }
#endif //WIN32
        szDirPath = szDirPathDefault;
    }
    if ( 0 != dk_createdir( szDirPath ) ) {
        printf( "dk_createdir failed. %s\n", szDirPath );
        goto err;
    }
    // get current data/time.
    time( &t );
    pCurTM = localtime( &t );
    // prepare m_szLogFilePath.
    sprintf( s_dklog->m_szLogFilePath, "%s/%s_%04d%02d%02d.log", szDirPath, szPrefixed?szPrefixed:"", (1900+pCurTM->tm_year), (1+pCurTM->tm_mon), pCurTM->tm_mday );
    strcpy( s_dklog->m_szDirPath, szDirPath );
    strcpy( s_dklog->m_szPrefixed, szPrefixed?szPrefixed:"" );
    // open log file.
#ifdef WIN32
    s_dklog->m_hLogFile = ::CreateFileA( s_dklog->m_szLogFilePath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( !s_dklog->m_hLogFile )
        goto err;
    ::SetFilePointer( s_dklog->m_hLogFile, 0, 0, FILE_END );
#else
    s_dklog->m_fd = open( s_dklog->m_szLogFilePath, O_RDWR|O_CREAT, 0664 );
    if ( !s_dklog->m_fd ) 
        goto err;
    lseek( s_dklog->m_fd, 0, SEEK_END );
#endif //WIN32

#ifdef WIN32
    _beginthread( _dklog_save_thread_win32, 0, s_dklog );
#else
    if ( 0 != pthread_create( &s_dklog->m_threadSave, 0, _dklog_save_thread, s_dklog ) )
        goto err;
#endif //WIN32

    return 0;

err:
    if ( s_dklog ) {
        if ( s_dklog->m_cache ) {
            dk_block_destroy( s_dklog->m_cache );
            s_dklog->m_cache = 0;
        }

#ifdef WIN32
        if ( s_dklog->m_hLogFile ) {
            ::CloseHandle( s_dklog->m_hLogFile );
            s_dklog->m_hLogFile = NULL;
        }
#else
        if ( s_dklog->m_fd ) {
            close( s_dklog->m_fd );
            s_dklog->m_fd = 0;
        }
#endif //WIN32
        free( s_dklog );
        s_dklog = 0;
    }
    
    return -1;
}

void dklog_uninit() {
    if ( s_dklog == 0 ) return;
    
    DKLOG_BIT_ADD( s_dklog->m_uFlag, DKLOG_FLAG_EXIT_ALL );

#ifdef WIN32
    Sleep( 100 );
#else
    usleep( 100*1000 );
#endif //WIN32

    if ( s_dklog->m_cache ) {
        dk_block_destroy( s_dklog->m_cache );
        s_dklog->m_cache = 0;
    }

#ifdef WIN32
    if ( s_dklog->m_hLogFile ) {
        CloseHandle( s_dklog->m_hLogFile );
        s_dklog->m_hLogFile = NULL;
    }
#else
    if ( s_dklog->m_fd ) {
        close( s_dklog->m_fd );
        s_dklog->m_fd = 0;
    }
#endif //WIN32

    free( s_dklog );
    s_dklog = 0;
}

int _dklog_lock() {
    if ( !s_dklog ) return -1;
    if ( DKLOG_BIT_HITTEST( s_dklog->m_uFlag, DKLOG_FLAG_EXIT_ALL ) ) 
        return -1;
#ifdef WIN32
    return (WAIT_OBJECT_0 == ::WaitForSingleObject( s_dklog->m_hEventAccess, INFINITE )) ? 0 : -1;
#else
    pthread_mutex_lock( &s_dklog->m_lock );
#endif //WIN32

    return 0;
}

void _dklog_unlock() {
    if ( !s_dklog ) return;
#ifdef WIN32
    ::SetEvent( s_dklog->m_hEventAccess );
#else
    pthread_mutex_unlock( &s_dklog->m_lock );
#endif //WIN32
}

void _dklog_fatalf( const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, ... ) {
    va_list ap;
    va_start( ap, fmt );
    _dklog_vlogf( "FAT", szFile, uLine, szFunction, fmt, ap );
    va_end( ap );
}

void _dklog_errorf( const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, ... ) {
    va_list ap;
    va_start( ap, fmt );
    _dklog_vlogf( "ERR", szFile, uLine, szFunction, fmt, ap );
    va_end( ap );
}

void _dklog_warnf( const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, ... ) {
    va_list ap;
    va_start( ap, fmt );
    _dklog_vlogf( "WAR", szFile, uLine, szFunction, fmt, ap );
    va_end( ap );
}

void _dklog_debugf( const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, ... ) {
    va_list ap;
    va_start( ap, fmt );
    _dklog_vlogf( "DBG", szFile, uLine, szFunction, fmt, ap );
    va_end( ap );
}

void _dklog_infof( const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, ... ) {
    va_list ap;
    va_start( ap, fmt );
    _dklog_vlogf( "INF", szFile, uLine, szFunction, fmt, ap );
    va_end( ap );
}

void _dklog_vlogf( const char* szLevel, const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, va_list ap ) {
    if ( !s_dklog ) return;
    char* pPosStart = 0;
    unsigned int uLenContent = 0;
    const char* pFName = 0;
    char szLogTitle[ 30 ];

#ifdef WIN32
    SYSTEMTIME sysTime;
    ::GetLocalTime( &sysTime );
    sprintf( szLogTitle, "[%s|%02d:%02d:%02d.%03ld] ", szLevel, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds );
#else
    struct timeval tv;
    struct timezone tz;
    struct tm* pCurTM = 0;
    time( &tv.tv_sec );
    pCurTM = localtime( &tv.tv_sec );
    gettimeofday( &tv, &tz );
    sprintf( szLogTitle, "[%s|%02d:%02d:%02d.%03ld] ", szLevel, pCurTM->tm_hour, pCurTM->tm_min, pCurTM->tm_sec, tv.tv_usec/1000 );
#endif //WIN32
   
    pFName = strrchr( szFile, '/' );
    if ( !pFName ) {
        pFName = szFile;
    } else {
        pFName = szFile + 1;
    }

    if ( 0 != _dklog_lock() )
        return;
    strcpy( s_dklog->m_szLogContent, szLogTitle );
    pPosStart = s_dklog->m_szLogContent + strlen( s_dklog->m_szLogContent );
    vsprintf( pPosStart, fmt, ap );
    //
    uLenContent = strlen( s_dklog->m_szLogContent );
#ifdef WIN32
    s_dklog->m_szLogContent[ uLenContent ] = '\r';
    s_dklog->m_szLogContent[ uLenContent + 1 ] = '\n';
    dk_block_pushback( s_dklog->m_cache, (unsigned char*)s_dklog->m_szLogContent, uLenContent + 2 );
#else
    s_dklog->m_szLogContent[ uLenContent ] = '\n';
    dk_block_pushback( s_dklog->m_cache, (unsigned char*)s_dklog->m_szLogContent, uLenContent + 1 );
#endif //WIN32
    _dklog_unlock(); 
    //printf( "%s\n", s_dklog->m_szLogContent );

}

static void _dklog_save_thread_win32( void* pParam ) {
    _dklog_save_thread( pParam );
}

void* _dklog_save_thread( void* pParam ) {
    unsigned char* pBufLogContent = 0;
    unsigned int uLenPop = 0;
    DWORD dwWritten = 0;
   
    pBufLogContent = (unsigned char*)malloc( LEN_DKLOG_CONTENT_MAX ); 
    while( s_dklog && !DKLOG_BIT_HITTEST( s_dklog->m_uFlag, DKLOG_FLAG_EXIT_ALL ) ) {
        uLenPop = dk_block_pop1stblock( s_dklog->m_cache, pBufLogContent, LEN_DKLOG_CONTENT_MAX );
        if ( uLenPop == 0 ) {
#ifdef WIN32
            Sleep(1);
#else
            usleep( 1*1000 );
#endif //WIN32
            continue;
        }

#ifdef WIN32
        ::WriteFile( s_dklog->m_hLogFile, pBufLogContent, uLenPop, &dwWritten, NULL );
#else
        write( s_dklog->m_fd, pBufLogContent, uLenPop );
#endif //WIN32
    }

    free( pBufLogContent );
    pBufLogContent = 0;
    return 0;
}












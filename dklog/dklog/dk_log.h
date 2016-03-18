#ifndef __DK_LOG_H__
#define __DK_LOG_H__


int dklog_init( const char* szDirPath, const char* szPrefixed );

void dklog_uninit();

#define DKLOG_FATAL( fmt, ... ) do{ \
        _dklog_fatalf( __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__ );\
        }while(0)

#define DKLOG_ERROR( fmt, ... ) do{ \
        _dklog_errorf( __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__ );\
        }while(0)

#define DKLOG_WARN( fmt, ... ) do{ \
        _dklog_warnf( __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__ );\
        }while(0)

#define DKLOG_DEBUG( fmt, ... ) do{ \
        _dklog_debugf( __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__ );\
        }while(0)

#define DKLOG_INFO( fmt, ... ) do{ \
        _dklog_infof( __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__ );\
        }while(0)

int _dklog_lock();

void _dklog_unlock();

void _dklog_fatalf( const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, ... );

void _dklog_errorf( const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, ... );

void _dklog_warnf( const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, ... );

void _dklog_debugf( const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, ... );

void _dklog_infof( const char* szFile, unsigned int uLine, const char* szFunction, const char* fmt, ... );




#endif //__DK_LOG_H__

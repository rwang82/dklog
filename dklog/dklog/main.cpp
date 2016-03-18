// dklog.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "dk_createdir.h"
#include "dk_log.h"

int _tmain(int argc, _TCHAR* argv[])
{
    dklog_init( "F:\\tmp\\AAA\\bb", "dklog" );

DKLOG_FATAL( "chilema%d", 1 );
DKLOG_ERROR( "chilema%d", 2 );
DKLOG_WARN( "chilema%d", 3 );
DKLOG_DEBUG( "chilema%d", 4 );
DKLOG_INFO( "chilema%d", 5 );


::Sleep( 2000 );
    dklog_uninit();


	return 0;
}


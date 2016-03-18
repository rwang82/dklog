#ifndef __DK_BLOCK_CACHE_H__
#define __DK_BLOCK_CACHE_H__
//
#ifdef WIN32
#include "Windows.h"
#else
#include <pthread.h>
#endif //WIN32
//
struct dk_block_cache {
#ifdef WIN32
    HANDLE m_hEventAccessCache;
    HANDLE m_hEventNeedExit;
#else
    pthread_mutex_t m_lockCache; 
#endif //WIN32
    unsigned int m_uFlag;
    unsigned char* m_pBufCache;
    unsigned int m_uSizeBufCache;
    volatile unsigned int m_uLenData;
    unsigned int m_uPosStart;
};
    
struct dk_block_cache* dk_block_create( unsigned int uSizeCache );

void dk_block_destroy( struct dk_block_cache* pDKBC );

void dk_block_reset( struct dk_block_cache* pDKBC );

int dk_block_pushback( struct dk_block_cache* pDKBC, unsigned char* pBufBlock, unsigned int uLenBlock );

unsigned int dk_block_get1stblocksize( struct dk_block_cache* pDKBC );

////////////////////////////////////////////////////////////////////////////
// return value : specify the number of byte copy to pBufBlock.
unsigned int dk_block_pop1stblock( struct dk_block_cache* pDKBC, unsigned char* pBufBlock, unsigned int uLenBlock );












#endif //__DK_BLOCK_CACHE_H__

/**
 * @file	util_public.c
 * @brief	ユーティリティ
 * @author	井上淳
 * @date	2005/5/29 新規作成
  */

/* Include */
#include "kernel.h"
#include "code_rules_def.h"
#include "i2c_drv_local.h"			/* "common/util_public.h" */
/* #include "string.h" */

/* Prototype */

/* Valiables */
#define next(index, size) (((index)+1)%(size))

#if 0
#define BUF_SIZE 256
static char assert_state[BUF_SIZE];

extern void kassert_str(const char* exp);

void _assert_kk( char* state )
{
	strcpy(assert_state, state);
	/* 2007/08/01 KK hirota-tomohito */
	kassert_str(assert_state);
	while( 1 );
}
#endif

/**
 *  \function	initqueue
 *  \brief		キューの初期化
 *				型依存なし
 *  \author		井上淳
 *  \return		none
 */
void initqueue( QUEUE_DATA* pQd, void* pPtr, int queuesize, int accesssize )
{
	pQd->front = 0;
	pQd->last = 0;
	pQd->pQueue = pPtr;
	pQd->size = queuesize / accesssize;
}

/**
 *  \function	queueisempty
 *  \brief		キューの空判断
 *				型依存なし
 *  \author		井上淳
 *  \return		0  : データなし
 *				-1 : データあり
 */
int queueisempty( QUEUE_DATA* pQd )
{
	if( pQd->front == pQd->last )
	{
		return 0;
	}
	return -1;
}

/**
 *  \function	enqueue_c
 *  \brief		キューにデータ登録(8bitデータ)
 *  \author		井上淳
 *  \return		0  : 正常終了
 *				-1 : キューFullで追加できない
 */
int enqueue_c( QUEUE_DATA* pQd, void* pdata )
{
	if( pQd->front == next(pQd->last, pQd->size) )
	{
		return -1;//FULL
	}
	else
	{
		((unsigned char*)(pQd->pQueue))[pQd->last] = *(unsigned char*)pdata;
		pQd->last = next(pQd->last, pQd->size);
	}
	return 0;
}

/**
 *  \function	dequeue_c
 *  \brief		キューからデータ取り出し(8bitデータ)
 *  \author		井上淳
 *  \return		0  : 正常終了
 *				-1 : データなし
 */
int dequeue_c( QUEUE_DATA* pQd, void* pdata )
{
	if( pQd->front == pQd->last )
	{
		return -1;//EMPTY
	}
	else
	{
		*(unsigned char*)pdata = ((unsigned char*)(pQd->pQueue))[pQd->front];
		pQd->front = next(pQd->front, pQd->size);
	}
	return 0;
}

/**
 *  \function	enqueue_s
 *  \brief		キューにデータ登録(16bitデータ)
 *  \author		井上淳
 *  \return		0  : 正常終了
 *				-1 : キューFullで追加できない
 */
int enqueue_s( QUEUE_DATA* pQd, void* pdata )
{
	if( pQd->front == next(pQd->last, pQd->size) )
	{
		return -1;//FULL
	}
	else
	{
		((unsigned short*)(pQd->pQueue))[pQd->last] = *(unsigned short*)pdata;
		pQd->last = next(pQd->last, pQd->size);
	}
	return 0;
}

/**
 *  \function	dequeue_s
 *  \brief		キューからデータ取り出し(16bitデータ)
 *  \author		井上淳
 *  \return		0  : 正常終了
 *				-1 : データなし
 */
int dequeue_s( QUEUE_DATA* pQd, void* pdata )
{
	if( pQd->front == pQd->last )
	{
		return -1;//EMPTY
	}
	else
	{
		*(unsigned short*)pdata = ((unsigned short*)(pQd->pQueue))[pQd->front];
		pQd->front = next(pQd->front, pQd->size);
	}
	return 0;
}

/**
 *  \function	enqueue_l
 *  \brief		キューにデータ登録(32bitデータ)
 *  \author		井上淳
 *  \return		0  : 正常終了
 *				-1 : キューFullで追加できない
 */
int enqueue_l( QUEUE_DATA* pQd, void* pdata )
{
	if( pQd->front == next(pQd->last, pQd->size) )
	{
		return -1;//FULL
	}
	else
	{
		((unsigned long*)(pQd->pQueue))[pQd->last] = *(unsigned long*)pdata;
		pQd->last = next(pQd->last, pQd->size);
	}
	return 0;
}

/**
 *  \function	dequeue_l
 *  \brief		キューからデータ取り出し(32bitデータ)
 *  \author		井上淳
 *  \return		0  : 正常終了
 *				-1 : データなし
 */
int dequeue_l( QUEUE_DATA* pQd, void* pdata )
{
	if( pQd->front == pQd->last )
	{
		return -1;//EMPTY
	}
	else
	{
		*(unsigned long*)pdata = ((unsigned long*)(pQd->pQueue))[pQd->front];
		pQd->front = next(pQd->front, pQd->size);
	}
	return 0;
}

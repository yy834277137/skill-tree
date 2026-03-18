/*
 *   @file   cif_list.c
 *
 *   @brief  Customer interface list.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "cif_list.h"

unsigned int cif_list_alloc_nr = 0;
extern unsigned int gmlib_dbg_level;

#define CIF_LIST_MALLOC(a)       ({cif_list_alloc_nr++; malloc((a));})
#define CIF_LIST_FREE(a)         {cif_list_alloc_nr--; free((a));}

/*------------------------------------------------------------------------
* void init_list(list_head_t *head)
* Purpose:
*
* Parameters:
*    Input:   None
*    Output:  None
*    Return:  None
* Notes:
*------------------------------------------------------------------------
*/
void init_list(list_head_t *head)
{
	head->next = head->prev = head;
}

/*------------------------------------------------------------------------
* void add_list(list_head_t *new, list_head_t *head)
* Purpose:
*
* Parameters:
*    Input:   None
*    Output:  None
*    Return:  None
* Notes:
*------------------------------------------------------------------------
*/
int add_list(int fd, list_head_t *head)
{
	list_head_t *newEntry;

	newEntry = (list_head_t *)CIF_LIST_MALLOC(sizeof(list_head_t));
	if (newEntry == NULL) {
		printf("%s:%d alloc entry fail! \n ", __FUNCTION__, __LINE__);
		return -1;
	}
	newEntry->member = fd;
	common_add_listTailEntry(newEntry, head);
	return 0;
}

/*------------------------------------------------------------------------
* void free_list(list_head_t *head)
* Purpose:
*
* Parameters:
*    Input:   None
*    Output:  None
*    Return:  None
* Notes:
*------------------------------------------------------------------------
*/
void free_list(list_head_t *head)
{
	list_head_t *pos, *next;

	common_get_listEachEntry(pos, next, head) {
		common_del_listEntry(pos);
		CIF_LIST_FREE(pos);
	}
	return;
}


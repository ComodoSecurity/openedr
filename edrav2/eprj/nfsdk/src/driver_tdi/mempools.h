//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _MEMPOOLS_H
#define _MEMPOOLS_H

void mempools_init();
void mempools_free();

void * mp_alloc(unsigned int size);
void mp_free(void * buffer, unsigned int maxPoolSize);

#endif
/**********************************************************************************************************************************
*
* 版权信息：版权所有 (c) 2021, 杭州海康威数字技术股份有限公司, 保留所有权利
*
* 文件名称：AES.h
* 摘    要：libXRay算法库加密文件
*
* 当前版本：V 1.0
* 作    者：张浩然
* 日    期：2021年08月24日
* 备    注：加密算法采用AES_128_ECB, 主要功能：对明文进行加密，并与DSP解密后的明文进行比较
***********************************************************************************************************************************/
#pragma once

#include <string.h>
#include <stdlib.h>


class AES {
public:
	AES(unsigned char* key);
	virtual ~AES();
	unsigned char* Cipher(unsigned char* input);
	void* Cipher(void* input, int length = 0);

private:
	unsigned char Sbox[256];
	unsigned char w[11][4][4];

	void KeyExpansion(unsigned char* key, unsigned char w[][4][4]);
	unsigned char FFmul(unsigned char a, unsigned char b);

	void SubBytes(unsigned char state[][4]);
	void ShiftRows(unsigned char state[][4]);
	void MixColumns(unsigned char state[][4]);
	void AddRoundKey(unsigned char state[][4], unsigned char k[][4]);
};




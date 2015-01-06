#include <stdio.h>
#include <iostream>

#include "lzma_file.h"
#include "..\lzma\sdk\Alloc.h"
#include "..\lzma\sdk\LzmaLib.h"
#include "..\lzma\sdk\LzmaDec.h"
#include "..\lzma\sdk\LzmaEnc.h"

#ifndef LZMA_MAIN_H
#define LZMA_MAIN_H

#define _CRT_SECURE_NO_WARNINGS

#define IN_BUF_SIZE (1 << 16)
#define OUT_BUF_SIZE (1 << 16)

class lzma {

	public:
	static unsigned long int Encode(std::FILE *inFile, std::FILE *outFile, unsigned int lzma_lvl = 8, ICompressProgress *progress = NULL);
	static unsigned long int Decode(std::FILE *inFile, std::FILE *outFile, ICompressProgress *progress = NULL);
};



#endif
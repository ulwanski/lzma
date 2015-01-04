#ifndef LZMA_H
#define LZMA_H

#define _CRT_SECURE_NO_WARNINGS

#include "..\lzma\sdk\Types.h"
#include "..\lzma\sdk\LzmaLib.h"
#include "..\lzma\sdk\LzmaEnc.h"
#include "..\lzma\sdk\LzmaDec.h"
#include "..\lzma\sdk\LzmaEnc.h"

class lzma {

	public:
	static int lzma::Encode(const char* inFile, const char* outFile, ICompressProgress *progress = NULL);
	static int lzma::Decode(const char* inFile, const char* outFile);
};

#endif
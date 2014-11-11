#ifndef LZMA_H
#define LZMA_H

#define _CRT_SECURE_NO_WARNINGS

#include "..\lzma\sdk\Types.h"

class lzma {

public:
	static int lzma::Encode(const char* inFile, const char* outFile);
	static int lzma::Decode(const char* inFile, const char* outFile);

private:
	static SRes lzma::lzma_encode(ISeqOutStream *outStream, ISeqInStream *inStream, UInt64 fileSize);
	static SRes lzma::lzma_decode(ISeqOutStream *outStream, ISeqInStream *inStream);
	static SRes lzma::lzma_decode2(CLzmaDec *state, ISeqOutStream *outStream, ISeqInStream *inStream, UInt64 unpackSize);

};

#endif
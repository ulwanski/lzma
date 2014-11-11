#define _CRT_SECURE_NO_WARNINGS

#include "..\lzma\sdk\LzmaLib.h"
#include "..\lzma\sdk\LzmaEnc.h"
#include "..\lzma\sdk\Alloc.h"
#include "..\lzma\sdk\7zFile.h"
#include "..\lzma\sdk\7zVersion.h"
#include "..\lzma\sdk\LzmaDec.h"
#include "..\lzma\sdk\LzmaEnc.h"

class lzma {

public:
	static int lzma::Encode(const char* inFile, const char* outFile);
	static int lzma::Decode(const char* inFile, const char* outFile);

private:
	static SRes lzma::lzma_encode(ISeqOutStream *outStream, ISeqInStream *inStream, UInt64 fileSize);
	static SRes lzma::lzma_decode(ISeqOutStream *outStream, ISeqInStream *inStream);
	static SRes lzma::lzma_decode2(CLzmaDec *state, ISeqOutStream *outStream, ISeqInStream *inStream, UInt64 unpackSize);

};
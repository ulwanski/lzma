#include "lzma.h"
#include "..\lzma\sdk\LzmaLib.h"
#include "..\lzma\sdk\LzmaEnc.h"
#include "..\lzma\sdk\Alloc.h"
#include "..\lzma\sdk\7zFile.h"
#include "..\lzma\sdk\7zVersion.h"
#include "..\lzma\sdk\LzmaDec.h"
#include "..\lzma\sdk\LzmaEnc.h"

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

#define IN_BUF_SIZE (1 << 16)
#define OUT_BUF_SIZE (1 << 16)

int lzma::Encode(const char* inFile, const char* outFile){
		
	CFileSeqInStream inStream;
	CFileOutStream outStream;

	FileSeqInStream_CreateVTable(&inStream);
	File_Construct(&inStream.file);
	FileOutStream_CreateVTable(&outStream);
	File_Construct(&outStream.file);

	if (InFile_Open(&inStream.file, inFile) != 0){
		// Can not open input file
		return 1;
	}

	if (OutFile_Open(&outStream.file, outFile) != 0){
		// Can not open output file
		return 1;
	}

	UInt64 fileSize;
	File_GetLength(&inStream.file, &fileSize);
	int res = lzma::lzma_encode(&outStream.s, &inStream.s, fileSize);

	File_Close(&outStream.file);
	File_Close(&inStream.file);
	return 0;
}

int lzma::Decode(const char* inFile, const char* outFile){

	CFileSeqInStream inStream;
	CFileOutStream outStream;

	FileSeqInStream_CreateVTable(&inStream);
	File_Construct(&inStream.file);
	FileOutStream_CreateVTable(&outStream);
	File_Construct(&outStream.file);

	if (InFile_Open(&inStream.file, inFile) != 0){
		// Can not open input file
		return 1;
	}

	if (OutFile_Open(&outStream.file, outFile) != 0){
		// Can not open output file
		return 1;
	}
		
	int res = lzma::lzma_decode(&outStream.s, &inStream.s);
		
	File_Close(&outStream.file);
	File_Close(&inStream.file);
	return 0;
}

SRes lzma::lzma_encode(ISeqOutStream *outStream, ISeqInStream *inStream, UInt64 fileSize){
	CLzmaEncHandle enc;
	SRes res = 0;
	CLzmaEncProps props;

	enc = LzmaEnc_Create(&g_Alloc);
	if (enc == 0) return SZ_ERROR_MEM;

	LzmaEncProps_Init(&props);
	res = LzmaEnc_SetProps(enc, &props);

	if (res == SZ_OK)
	{
	Byte header[LZMA_PROPS_SIZE + 8];
	size_t headerSize = LZMA_PROPS_SIZE;
	int i;

	res = LzmaEnc_WriteProperties(enc, header, &headerSize);
	for (i = 0; i < 8; i++) header[headerSize++] = (Byte)(fileSize >> (8 * i));
	if (outStream->Write(outStream, header, headerSize) != headerSize)
		res = SZ_ERROR_WRITE;
	else
	{
		if (res == SZ_OK)
		res = LzmaEnc_Encode(enc, outStream, inStream, NULL, &g_Alloc, &g_Alloc);
	}
	}
	LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
	return res;
}

SRes lzma::lzma_decode(ISeqOutStream *outStream, ISeqInStream *inStream){
		
	UInt64 unpackSize;
	int i;
	SRes res = 0;

	CLzmaDec state;

	/* header: 5 bytes of LZMA properties and 8 bytes of uncompressed size */
	unsigned char header[LZMA_PROPS_SIZE + 8];

	/* Read and parse header */
	RINOK(SeqInStream_Read(inStream, header, sizeof(header)));

	unpackSize = 0;
	for (i = 0; i < 8; i++) unpackSize += (UInt64)header[LZMA_PROPS_SIZE + i] << (i * 8);

	LzmaDec_Construct(&state);
	RINOK(LzmaDec_Allocate(&state, header, LZMA_PROPS_SIZE, &g_Alloc));
	res = lzma::lzma_decode2(&state, outStream, inStream, unpackSize);
	LzmaDec_Free(&state, &g_Alloc);
	return res;
}

SRes lzma::lzma_decode2(CLzmaDec *state, ISeqOutStream *outStream, ISeqInStream *inStream, UInt64 unpackSize){
	int thereIsSize = (unpackSize != (UInt64)(Int64)-1);
	Byte inBuf[IN_BUF_SIZE];
	Byte outBuf[OUT_BUF_SIZE];
	size_t inPos = 0, inSize = 0, outPos = 0;
	LzmaDec_Init(state);
	for (;;){
		if (inPos == inSize){
			inSize = IN_BUF_SIZE;
			RINOK(inStream->Read(inStream, inBuf, &inSize));
			inPos = 0;
		}
		{
		SRes res;
		SizeT inProcessed = inSize - inPos;
		SizeT outProcessed = OUT_BUF_SIZE - outPos;
		ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
		ELzmaStatus status;
		if (thereIsSize && outProcessed > unpackSize){
			outProcessed = (SizeT)unpackSize;
			finishMode = LZMA_FINISH_END;
		}
      
		res = LzmaDec_DecodeToBuf(state, outBuf + outPos, &outProcessed,
		inBuf + inPos, &inProcessed, finishMode, &status);
		inPos += inProcessed;
		outPos += outProcessed;
		unpackSize -= outProcessed;
      
		if (outStream) if (outStream->Write(outStream, outBuf, outPos) != outPos) return SZ_ERROR_WRITE;
        
		outPos = 0;
      
		if (res != SZ_OK || thereIsSize && unpackSize == 0) return res;
      
		if (inProcessed == 0 && outProcessed == 0){
			if (thereIsSize || status != LZMA_STATUS_FINISHED_WITH_MARK) return SZ_ERROR_DATA;
			return res;
		}
		}
	}
}
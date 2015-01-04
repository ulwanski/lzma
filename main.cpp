#include "lzma.h"
#include "..\lzma\sdk\Alloc.h"
#include "..\lzma\sdk\7z.h"
#include "..\lzma\sdk\7zFile.h"
#include "..\lzma\sdk\7zVersion.h"
#include "..\lzma\sdk\Lzma2Dec.h"
#include "..\lzma\sdk\Lzma2Enc.h"

static void *AllocForLzma2(void *, size_t size) { return BigAlloc(size); }
static void FreeForLzma2(void *, void *address) { BigFree(address); }
static ISzAlloc SzAllocForLzma2 = { AllocForLzma2, FreeForLzma2 };

static void *AllocForLzma(void *, size_t size) { return MyAlloc(size); }
static void FreeForLzma(void *, void *address) { MyFree(address); }
static ISzAlloc SzAllocForLzma = { AllocForLzma, FreeForLzma };

#define IN_BUF_SIZE (1 << 16)
#define OUT_BUF_SIZE (1 << 16)

#include <iostream>
#include <vector>

typedef struct{
    ISeqInStream SeqInStream;
    const std::vector<unsigned char> *Buf;
    unsigned BufPos;
} VectorInStream;

SRes VectorInStream_Read(void *p, void *buf, size_t *size){
    VectorInStream *ctx = (VectorInStream*)p;
    *size = min(*size, ctx->Buf->size() - ctx->BufPos);
    if (*size)
        memcpy(buf, &(*ctx->Buf)[ctx->BufPos], *size);
    ctx->BufPos += *size;
    return SZ_OK;
}

typedef struct {
    ISeqOutStream SeqOutStream;
    std::vector<unsigned char> *Buf;
} VectorOutStream;

size_t VectorOutStream_Write(void *p, const void *buf, size_t size){
    VectorOutStream *ctx = (VectorOutStream*)p;
    if (size)
    {
        unsigned oldSize = ctx->Buf->size();
        ctx->Buf->resize(oldSize + size);
        memcpy(&(*ctx->Buf)[oldSize], buf, size);
    }
    return size;
}

int lzma::Encode(const char* inFile, const char* outFile, ICompressProgress *progress){
		
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

	CLzma2EncHandle enc = Lzma2Enc_Create(&SzAllocForLzma, &SzAllocForLzma2);
	if (enc == 0) return SZ_ERROR_MEM;

	CLzma2EncProps props;
    Lzma2EncProps_Init(&props);
	props.lzmaProps.writeEndMark = 0;
	props.lzmaProps.level = 8;
    SRes res = Lzma2Enc_SetProps(enc, &props);

	if(res == SZ_OK){
		Lzma2Enc_WriteProperties(enc);
		Lzma2Enc_Encode(enc, &outStream.s, &inStream.s, progress);
	}
	Lzma2Enc_Destroy(enc);

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

	/* header: 5 bytes of LZMA properties and 8 bytes of uncompressed size */
	unsigned char header[LZMA_PROPS_SIZE + 8];

	/* Read and parse header */
	//RINOK(SeqInStream_Read(&inStream.s, header, sizeof(header)));

	UInt64 unpackSize = 0;
	for (int i = 0; i < 8; i++) unpackSize += (UInt64)header[LZMA_PROPS_SIZE + i] << (i * 8);


	CLzma2Dec dec;
	Lzma2Dec_Construct(&dec);
	Lzma2Dec_Allocate(&dec, 8 /*(Byte)header*/, &SzAllocForLzma);
	Lzma2Dec_Init(&dec);
	
	int thereIsSize = (unpackSize != (UInt64)(Int64)-1);
	Byte inBuf[IN_BUF_SIZE];
	Byte outBuf[OUT_BUF_SIZE];
	size_t inPos = LZMA_PROPS_SIZE, inSize = IN_BUF_SIZE, outPos = 0;
	
	ELzmaStatus status;

	for (;;){
		
		if (inPos == inSize){
			inSize = IN_BUF_SIZE;
			RINOK(inStream.s.Read(&inStream.s, inBuf, &inSize));
			inPos = 0;
		}
		
		SizeT inProcessed = inSize - inPos;
		SizeT outProcessed = OUT_BUF_SIZE - outPos;

		SRes res = Lzma2Dec_DecodeToBuf(&dec, outBuf + outPos, &outProcessed, inBuf + inPos, &inProcessed, LZMA_FINISH_ANY, &status);
		inPos += inProcessed;
		outPos += outProcessed;
		unpackSize -= outProcessed;
      
		//if(&outStream.s) if(outStream.s.Write(&outStream.s, outBuf, outPos) != outPos) return SZ_ERROR_WRITE;        
		//outPos = 0;
      
		if (res != SZ_OK) return res;
      
		/*
		if (inProcessed == 0 && outProcessed == 0){
			if (thereIsSize || status != LZMA_STATUS_FINISHED_WITH_MARK) return SZ_ERROR_DATA;
			return res;
		}
		*/
	}

	Lzma2Dec_Free(&dec, &SzAllocForLzma2);

	File_Close(&outStream.file);
	File_Close(&inStream.file);
	return 0;
}
#include "lzma.h"

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

unsigned long int lzma::Encode(std::FILE *inFile, std::FILE *outFile, unsigned int lzma_lvl, ICompressProgress *progress){
		
	CFileSeqInStream inStream;
	CFileOutStream outStream;

	FileSeqInStream_CreateVTable(&inStream);
	File_Construct(&inStream.file);
	FileOutStream_CreateVTable(&outStream);
	File_Construct(&outStream.file);

	inStream.file.file = inFile;
	outStream.file.file = outFile;

	UInt64 fileSize;
	File_GetLength(&inStream.file, &fileSize);

	CLzmaEncHandle enc = LzmaEnc_Create(&g_Alloc);
	CLzmaEncProps props;
	LzmaEncProps_Init(&props);

	props.writeEndMark	= 0;
	props.level			= lzma_lvl;

	LzmaEnc_SetProps(enc, &props);

	Byte header[LZMA_PROPS_SIZE + 8];
	size_t headerSize = LZMA_PROPS_SIZE;
	SRes res = LzmaEnc_WriteProperties(enc, header, &headerSize);
	for (int i = 0; i < 8; i++) header[headerSize++] = (Byte)(fileSize >> (8 * i));
	
	if (outStream.s.Write(&outStream, header, headerSize) != headerSize){
		res = SZ_ERROR_WRITE;
	} else {
		if (res == SZ_OK) LzmaEnc_Encode(enc, &outStream.s, &inStream.s, progress, &g_Alloc, &g_Alloc);
	}
	
	unsigned long int result = outStream.file.lenght;

	LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
	return result;
}

unsigned long int lzma::Decode(std::FILE *inFile, std::FILE *outFile, ICompressProgress *progress){

	CFileSeqInStream inStream;
	CFileOutStream outStream;

	FileSeqInStream_CreateVTable(&inStream);
	File_Construct(&inStream.file);
	FileOutStream_CreateVTable(&outStream);
	File_Construct(&outStream.file);

	inStream.file.file = inFile;
	outStream.file.file = outFile;

	UInt64 unpackSize;
	int i;
	SRes res = 0;
	CLzmaDec state;

	/* header: 5 bytes of LZMA properties and 8 bytes of uncompressed size */
	unsigned char header[LZMA_PROPS_SIZE + 8];

	/* Read and parse header */
	RINOK(SeqInStream_Read(&inStream.s, header, sizeof(header)));

	unpackSize = 0;
	for (i = 0; i < 8; i++) unpackSize += (UInt64)header[LZMA_PROPS_SIZE + i] << (i * 8);

	LzmaDec_Construct(&state);
	RINOK(LzmaDec_Allocate(&state, header, LZMA_PROPS_SIZE, &g_Alloc));
	
	int thereIsSize = (unpackSize != (UInt64)(Int64)-1);
	Byte inBuf[IN_BUF_SIZE];
	Byte outBuf[OUT_BUF_SIZE];
	size_t inPos = 0, inSize = 0, outPos = 0;
	LzmaDec_Init(&state);
	for (;;){
		if (inPos == inSize){
			inSize = IN_BUF_SIZE;
			RINOK(inStream.s.Read(&inStream.s, inBuf, &inSize));
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
      
		res = LzmaDec_DecodeToBuf(&state, outBuf + outPos, &outProcessed, inBuf + inPos, &inProcessed, finishMode, &status);
		inPos += inProcessed;
		outPos += outProcessed;
		unpackSize -= outProcessed;
      
		if(&outStream.s) if(outStream.s.Write(&outStream.s, outBuf, outPos) != outPos) return SZ_ERROR_WRITE;
        
		outPos = 0;
      
		if (res != SZ_OK || thereIsSize && unpackSize == 0) return res;
      
		if (inProcessed == 0 && outProcessed == 0){
			if (thereIsSize || status != LZMA_STATUS_FINISHED_WITH_MARK) return SZ_ERROR_DATA;
			return res;
		}
		}
	}
	
	LzmaDec_Free(&state, &g_Alloc);
	return 0;
}
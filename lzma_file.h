#ifndef __7Z_FILE_H
#define __7Z_FILE_H

#include <stdio.h>
#include <errno.h>
#include "sdk\7zTypes.h"
#include "sdk\Precomp.h"

EXTERN_C_BEGIN

typedef struct {
	FILE *file;
	unsigned long int lenght;
} CSzFile;

void File_Construct(CSzFile *p);
WRes InFile_Open(CSzFile *p, const char *name);
WRes OutFile_Open(CSzFile *p, const char *name);
WRes File_Close(CSzFile *p);
WRes File_Read(CSzFile *p, void *data, size_t *size);
WRes File_Write(CSzFile *p, const void *data, size_t *size);
WRes File_Seek(CSzFile *p, Int64 *pos, ESzSeek origin);
WRes File_GetLength(CSzFile *p, UInt64 *length);

typedef struct {
	ISeqInStream s;
	CSzFile file;
} CFileSeqInStream;

typedef struct {
	ISeekInStream s;
	CSzFile file;
} CFileInStream;

typedef struct {
	ISeqOutStream s;
	CSzFile file;
} CFileOutStream;

void FileOutStream_CreateVTable(CFileOutStream *p);
void FileInStream_CreateVTable(CFileInStream *p);
void FileSeqInStream_CreateVTable(CFileSeqInStream *p);

EXTERN_C_END
#endif

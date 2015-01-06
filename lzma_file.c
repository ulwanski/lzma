#include "lzma_file.h"

void File_Construct(CSzFile *p){
	p->file = NULL;
	p->lenght = 0;
}

static WRes File_Open(CSzFile *p, const char *name, int writeMode){
	p->file = fopen(name, writeMode ? "wb+" : "rb");
	return (p->file != 0) ? 0 : errno;
}

WRes InFile_Open(CSzFile *p, const char *name) { return File_Open(p, name, 0); }
WRes OutFile_Open(CSzFile *p, const char *name) { return File_Open(p, name, 1); }

WRes File_Close(CSzFile *p){
	if (p->file != NULL){
		int res = fclose(p->file);
		if (res != 0) return res;
		p->file = NULL;
	}
	return 0;
}

WRes File_Read(CSzFile *p, void *data, size_t *size){
	size_t originalSize = *size;
	if (originalSize == 0) return 0;  
	
	*size = fread(data, 1, originalSize, p->file);
	if (*size == originalSize) return 0;
	return ferror(p->file);
}

WRes File_Write(CSzFile *p, const void *data, size_t *size){
	int count = 0;
	size_t originalSize = *size;
	if (originalSize == 0) return 0;

	count = fwrite(data, 1, originalSize, p->file);
	p->lenght += count;
	*size = count;

	if (*size == originalSize) return 0;
	return ferror(p->file);
}

WRes File_Seek(CSzFile *p, Int64 *pos, ESzSeek origin){
	int moveMethod;
	int res;
	switch (origin){
		case SZ_SEEK_SET: moveMethod = SEEK_SET; break;
		case SZ_SEEK_CUR: moveMethod = SEEK_CUR; break;
		case SZ_SEEK_END: moveMethod = SEEK_END; break;
		default: return 1;
	}
	res = fseek(p->file, (long)*pos, moveMethod);
	*pos = ftell(p->file);
	return res;
}

WRes File_GetLength(CSzFile *p, UInt64 *length){
	long pos = ftell(p->file);
	int res = fseek(p->file, 0, SEEK_END);
	*length = ftell(p->file);
	fseek(p->file, pos, SEEK_SET);
	return res;
}

static SRes FileSeqInStream_Read(void *pp, void *buf, size_t *size){
	CFileSeqInStream *p = (CFileSeqInStream *)pp;
	return File_Read(&p->file, buf, size) == 0 ? SZ_OK : SZ_ERROR_READ;
}

void FileSeqInStream_CreateVTable(CFileSeqInStream *p){
	p->s.Read = FileSeqInStream_Read;
}

static SRes FileInStream_Read(void *pp, void *buf, size_t *size){
	CFileInStream *p = (CFileInStream *)pp;
	return (File_Read(&p->file, buf, size) == 0) ? SZ_OK : SZ_ERROR_READ;
}

static SRes FileInStream_Seek(void *pp, Int64 *pos, ESzSeek origin){
	CFileInStream *p = (CFileInStream *)pp;
	return File_Seek(&p->file, pos, origin);
}

void FileInStream_CreateVTable(CFileInStream *p){
	p->s.Read = FileInStream_Read;
	p->s.Seek = FileInStream_Seek;
}

static size_t FileOutStream_Write(void *pp, const void *data, size_t size){
	CFileOutStream *p = (CFileOutStream *)pp;
	File_Write(&p->file, data, &size);
	return size;
}

void FileOutStream_CreateVTable(CFileOutStream *p){
	p->s.Write = FileOutStream_Write;
}

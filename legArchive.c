#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "legArchive.h"
//////
char* strnstr(char* _big, char* _small, int _length){
	int _cachedStrlen = strlen(_small);
	char* _endPointer = _big+_length-_cachedStrlen+1;
	for (;_big!=_endPointer;++_big){
		int i;
		for (i=0;i<_cachedStrlen;++i){
			if (_big[i]==_small[i]){
				if (i==_cachedStrlen-1){
					return _big;
				}
			}else{
				break;
			}
		}
	}
	return NULL;
}
void makeLowercase(char* _fixThis){
	int i;
	for (i=0;_fixThis[i]!=0;++i){
		_fixThis[i]=tolower(_fixThis[i]);
	}
}
// returns 1 if fail to find, 2 on error
signed char searchForString(FILE* fp, char* _searchTerm){
	int _cachedStrlen = strlen(_searchTerm);
	int _buffSize = _cachedStrlen>=256 ? _cachedStrlen*2 : 512;
	char _lastReadData[_buffSize];
	char _isFirst=1;
	while (1){
		int _totalRead;
		if (_isFirst){
			_isFirst=0;
			_totalRead = fread(_lastReadData,1,_buffSize,fp);
		}else{
			memcpy(_lastReadData,&(_lastReadData[_buffSize-_cachedStrlen]),_cachedStrlen);
			_totalRead = fread(&(_lastReadData[_cachedStrlen]),1,_buffSize-_cachedStrlen,fp);
			_totalRead+=_cachedStrlen;
		}
		if (_totalRead!=_buffSize){
			return 2;
		}
		char* _foundString = strnstr(_lastReadData,_searchTerm,_buffSize);
		if (_foundString!=NULL){
			// Seek to where we found the string
			fseek(fp,-1*(_totalRead-(long)(_foundString-&(_lastReadData[0]))),SEEK_CUR);
			return 0;
		}
	}
	return 1;
}
char* readNullString(FILE* fp){
	char* _destLine=NULL;
	size_t _lineSize = 0;
	ssize_t _lineRes =  getdelim(&_destLine,&_lineSize,0,fp);
	if (_lineRes==-1){
		free(_destLine);
		return NULL;
	}else{
		char* _ret = strdup(_destLine); // trim big buffer
		free(_destLine);
		return _ret;
	}
}
//////
// the filename in the return struct will be NULL if file not found
void getAdvancedFile(struct legArchive* _passedArchive, const char* _passedFilename, struct legArchiveFile* _ret){
	char* _usableSearch = strdup(_passedFilename);
	if (!_passedArchive->caseSensitive){
		makeLowercase(_usableSearch);
	}
	_ret->filename=NULL;
	int32_t i;
	for (i=0;i<_passedArchive->totalFiles;++i){
		if (strcmp(_passedArchive->fileList[i].filename,_usableSearch)==0){
			_ret->fp = fopen(_passedArchive->filename,"rb");
			fseek(_ret->fp,_passedArchive->fileList[i].position,SEEK_SET);
			_ret->internalPosition=0;
			_ret->totalLength = _passedArchive->fileList[i].length;
			_ret->startPosition = _passedArchive->fileList[i].position;
			_ret->inArchive=1;
			_ret->filename = strdup(_passedArchive->filename);
			break;
		}
	}
	free(_usableSearch);
}
FILE* openArchiveFile(struct legArchive* _passedArchive, const char* _passedFilename){
	struct legArchiveFile _gottenFile;
	getAdvancedFile(_passedArchive,_passedFilename,&_gottenFile);
	return _gottenFile.fp;
}
// if the returned archive's filename is NULL, loading failed. do not free the returned struct
void loadLegArchive(const char* _filename, char _caseSensitive, struct legArchive* _returnArchive){
	_returnArchive->caseSensitive=_caseSensitive;
	_returnArchive->filename=NULL;
	FILE* fp = fopen(_filename,"rb");
	if (fp==NULL){
		return;
	}
	
	fseek(fp,-8,SEEK_END);
	int64_t _tableStartPosition;
	fread(&_tableStartPosition,8,1,fp);
	_tableStartPosition = le64toh(_tableStartPosition);
	fseek(fp,_tableStartPosition,SEEK_SET);

	char _foundMagic[10];
	if (fread(_foundMagic,1,10,fp)!=10 || memcmp(_foundMagic,ENDTABLEIDENTIFICATION,10)!=0){
		fclose(fp);
	}else{
		_returnArchive->filename = strdup(_filename);
		
		int32_t _readTotalFiles;
		fread(&_readTotalFiles,4,1,fp);
		_readTotalFiles = le32toh(_readTotalFiles);

		_returnArchive->fileList = malloc(sizeof(struct legArchiveEntry)*_readTotalFiles);
		_returnArchive->totalFiles = _readTotalFiles;

		int i;
		for (i=0;i<_readTotalFiles;++i){
			_returnArchive->fileList[i].filename = readNullString(fp);
			if (!_caseSensitive){
				makeLowercase(_returnArchive->fileList[i].filename);
			}
			fread(&(_returnArchive->fileList[i].position),8,1,fp);
			fread(&(_returnArchive->fileList[i].length),4,1,fp);
			_returnArchive->fileList[i].position = le64toh(_returnArchive->fileList[i].position);
			_returnArchive->fileList[i].length = le32toh(_returnArchive->fileList[i].length);
		}
		fclose(fp);
	}
}
void freeArchive(struct legArchive* _toFree){
	int i;
	for (i=0;i<_toFree->totalFiles;++i){
		free(_toFree->fileList[i].filename);
	}
	free(_toFree->fileList);
	free(_toFree->filename);
}

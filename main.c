#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fts.h>
#include <errno.h>
#include "legArchive.h"

#define READBUFF 512

void easymkdir(const char* _path){
	int _mkRes = mkdir(_path,0777);
	if (_mkRes==-1 && errno!=EEXIST){
		fprintf(stderr,"failed to make directory %s\n",_path);
		exit(1);
	}
}
// returns file size
int32_t copyFile(FILE* _source, FILE* _dest){
	int32_t _retSize=0;
	char _buff[READBUFF];
	while(!feof(_source)){
		size_t _numRead = fread(_buff,1,READBUFF,_source);
		if (_numRead!=READBUFF && !feof(_source)){
			fprintf(stderr,"read error in copyFile\n");
			exit(1);
		}
		_retSize+=_numRead;
		if (fwrite(_buff,1,_numRead,_dest)!=_numRead){
			fprintf(stderr,"write error in copyFile\n");
			exit(1);
		}
	}
	return _retSize;
}
void replaceAllChar(char* _str, unsigned char _orig, unsigned char _new){
	char* _curPos;
	while((_curPos=strchr(_str,_orig))!=NULL){
		_curPos[0]=_new;
		_str=_curPos;
	}
}
void easyOpenArchive(const char* _filename, char _caseSensitive, struct legArchive* _ret){
	loadLegArchive(_filename,_caseSensitive,_ret);
	if (_ret->filename==NULL){
		printf("failed to open archive\n");
		exit(1);
	}
}
char endsInSlash(const char* _dirString){
	int _dirLen = strlen(_dirString);
	if (_dirString[_dirLen-1]=='/'){
		return 1;
	}
	#ifdef _WIN32
		if (_dirString[_dirLen-1]=='\\'){
			return 1;
		}
	#endif
	return 0;
}
char* fileInDir(const char* _dirString, const char* _addThis){
	int _dirLen = strlen(_dirString);
	char* _ret = malloc(_dirLen+strlen(_addThis)+2);
	char _needsSlash = !endsInSlash(_dirString);
	memcpy(_ret,_dirString,_dirLen);
	_ret[_dirLen]='\0';
	if (_needsSlash){
		_ret[_dirLen]='/';
		_ret[_dirLen+1]='\0';
	}
	strcat(_ret,_addThis);
	return _ret;
}
void printArchive(const char* _filename, char _caseSensitive){
	struct legArchive _arch;
	easyOpenArchive(_filename,_caseSensitive,&_arch);
	int i;
	for (i=0;i<_arch.totalFiles;++i){
		puts(_arch.fileList[i].filename);
	}
	freeArchive(&_arch);
}
void extractArchive(const char* _filename, const char* _destDir, char _caseSensitive){
	struct legArchive _arch;
	easyOpenArchive(_filename,_caseSensitive,&_arch);
	FILE* fp = fopen(_filename,"rb");
	if (fp==NULL){
		fprintf(stderr,"failed to open %s for reading\n",_filename);
		exit(1);
	}
	//
	easymkdir(_destDir);
	// extract all contents
	char _curRead[READBUFF];
	int i;
	for (i=0;i<_arch.totalFiles;++i){
		// make subdirectories if required. a bit of a hack
		char* _slashPos = strchr(_arch.fileList[i].filename,'/');
		if (_slashPos!=NULL){
			do{
				_slashPos[0]='\0';
				char* _foldername = fileInDir(_destDir,_arch.fileList[i].filename);
				easymkdir(_foldername);
				free(_foldername);
				_slashPos[0]='/';
			}while((_slashPos=strchr(_slashPos+1,'/'))!=NULL);
		}
		// open output
		char* _specificFilename = fileInDir(_destDir,_arch.fileList[i].filename);
		puts(_specificFilename);
		FILE* _out = fopen(_specificFilename,"wb");
		if (_out==NULL){
			fprintf(stderr,"failed to open %s for writing\n",_specificFilename);
			exit(1);
		}
		free(_specificFilename);
		
		fseek(fp,_arch.fileList[i].position,SEEK_SET);
		size_t _sizeLeft = _arch.fileList[i].length;
		while(_sizeLeft>0){
			size_t _readThis = _sizeLeft>=READBUFF ? READBUFF : _sizeLeft;
			_sizeLeft-=_readThis;
			if (fread(_curRead,1,_readThis,fp)!=_readThis){
				fprintf(stderr,"IO error reading from archive\n");
				exit(1);
			}
			if (fwrite(_curRead,1,_readThis,_out)!=_readThis){
				fprintf(stderr,"IO error writing to dest file\n");
				exit(1);
			}
		}
		fclose(_out);
	}
	fclose(fp);
	freeArchive(&_arch);
}

struct writeEntry{
	char* filename;
	int64_t pos;
	int32_t len;
	struct writeEntry* next;
};
void makeArchive(const char* _sourceDir, const char* _destFile){
	char* paths[] = {(char*)_sourceDir,NULL};
	FILE* _out = fopen(_destFile,"wb");
	if (_out==NULL){
		fprintf(stderr,"failed to open %s for writing\n",_destFile);
		exit(1);
	}
	// header info
	if (fwrite(MAGICNUMBERS,1,strlen(MAGICNUMBERS),_out)!=strlen(MAGICNUMBERS)){
		fprintf(stderr,"write error\n");
		exit(1);
	}
	if (fputc(ARCHIVEVERSION,_out)==EOF){
		fprintf(stderr,"write error\n");
		exit(1);
	}
	//
	FTS *ftsp = fts_open(paths,FTS_LOGICAL,NULL);
	if(ftsp==NULL){
		perror("fts_open");
		exit(1);
	}
	int _dirPrefix = strlen(_sourceDir)+(endsInSlash(_sourceDir) ? 0 : 1);
	int32_t _totalFiles=0;
	struct writeEntry _firstEntry;
	struct writeEntry* _lastEntry=&_firstEntry;
	while(1){
		errno=0;
		FTSENT* ent = fts_read(ftsp);
		if(ent == NULL){
			if(errno == 0){
				// no more items
				break;
			}else{
				fprintf(stderr,"fts_read error\n");
				exit(1);
			}
		}
		if(ent->fts_info & FTS_F){ // files only
			// verify prefix
			if (strncmp(_sourceDir,ent->fts_path,_dirPrefix-1)!=0){
				fprintf(stderr,"bad prefix for \"%s\" expected \"%s\" %d",ent->fts_path,_sourceDir,_dirPrefix-1);
				exit(1);
			}
			// get filename for archive
			char* _writeName = strdup(&(ent->fts_path[_dirPrefix]));
			replaceAllChar(_writeName,'\\','/'); // backslashes unsupported in filename
			// init new list entry
			struct writeEntry* _new = malloc(sizeof(struct writeEntry));
			_new->filename = _writeName;
			_new->pos = ftell(_out);
			
			// copy file data
			FILE* _in = fopen(ent->fts_path,"rb");
			if (_in==NULL){
				fprintf(stderr,"error opening %s\n",ent->fts_path);
				exit(1);
			}
			_new->len=copyFile(_in,_out);
			fclose(_in);
			// finish list entry
			_lastEntry->next=_new;
			_lastEntry=_new;
			++_totalFiles;
		}
	}
	fts_close(ftsp);

	// write file table
	int64_t _tableStart = ftell(_out);
	if (fwrite(ENDTABLEIDENTIFICATION,1,strlen(ENDTABLEIDENTIFICATION),_out)!=strlen(ENDTABLEIDENTIFICATION)){
		fprintf(stderr,"write error at table start\n");
		exit(1);
	}
	_totalFiles = htole32(_totalFiles);
	fwrite(&_totalFiles,1,4,_out);
	struct writeEntry* _curEntry = _firstEntry.next;
	int i;
	for (i=0;_curEntry!=NULL;++i){
		if (fwrite(_curEntry->filename,1,strlen(_curEntry->filename)+1,_out)!=strlen(_curEntry->filename)+1){
			fprintf(stderr,"error writing filename\n");
			exit(1);
		}
		_curEntry->pos = htole64(_curEntry->pos);
		if (fwrite(&_curEntry->pos,1,8,_out)!=8){
			fprintf(stderr,"error writing pos\n");
			exit(1);
		}
		_curEntry->len = htole32(_curEntry->len);
		if (fwrite(&_curEntry->len,1,4,_out)!=4){
			fprintf(stderr,"error writing len\n");
			exit(1);
		}
		struct writeEntry* _tempNext = _curEntry->next;
		free(_curEntry->filename);
		free(_curEntry);
		_curEntry=_tempNext;
	}
	// write table pos finally
	_tableStart = htole64(_tableStart);
	if (fwrite(&_tableStart,1,8,_out)!=8){
		fprintf(stderr,"error writing table pos\n");
	}
	fclose(_out);
}
int main(int numArgs, char** args){
	char _caseSensitive=1;
	if (numArgs>=2){
		if (strcmp(args[1],"printContents")==0){
			printArchive(args[2],_caseSensitive);
		}else if (strcmp(args[1],"makeArchive")==0){
			makeArchive(args[2],args[3]);
		}else if (strcmp(args[1],"extractArchive")==0){
			extractArchive(args[2],args[3],_caseSensitive);
		}else{
			fprintf(stderr,"invalid first argument. run without arguments for help.\n");
			return 1;
		}
	}else{
		fprintf(stderr,"%s printContents <archive filename>\n%s makeArchive <directory> <dest file>\n%s extractArchive <archive filename> <out directory>\n",args[0],args[0],args[0]);
		return 1;
	}
}

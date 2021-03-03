#if __MINGW32__
#include <ftw.h>

static struct writeEntry _wfirstEntry;
static struct writeEntry* _wLastEntry=&_wfirstEntry;
static int _wdirprefixlen;
static int _wtotalFiles;
static char* _wprefix;
static FILE* _wout;
int windowsscansinglefile(const char *fpath, const struct stat *sb,int typeflag, struct FTW *ftwbuf){
	if(typeflag==FTW_F){
		// verify prefix
		if (strncmp(_wprefix,fpath,_wdirprefixlen-1)!=0){
			fprintf(stderr,"bad prefix for \"%s\" expected \"%s\" %d",fpath,_wprefix,_wdirprefixlen-1);
			exit(1);
		}
		// get filename for archive
		char* _writeName;
		{
			if (fpath[_wdirprefixlen]=='/'){
				_writeName=strdup(&(fpath[_wdirprefixlen+1]));
			}else{
				_writeName=strdup(&(fpath[_wdirprefixlen]));
			}
		}
		replaceAllChar(_writeName,'\\','/'); // backslashes unsupported in filename
		// init new list entry
		struct writeEntry* _new = malloc(sizeof(struct writeEntry));
		_new->filename = _writeName;
		_new->pos = ftell(_wout);
		_new->next=NULL;
		// copy file data
		FILE* _in = fopen(fpath,"rb");
		if (_in==NULL){
			fprintf(stderr,"error opening %s\n",fpath);
			exit(1);
		}
		_new->len=copyFile(_in,_wout);
		fclose(_in);
		// finish list entry
		_wLastEntry->next=_new;
		_wLastEntry=_new;
		++_wtotalFiles;
	}
	return 0;
}
struct writeEntry* windowsscandir(const char* path){
	_wdirprefixlen=strlen(path)+(endsInSlash(path) ? 0 : 1);
	_wtotalFiles=0;
	_wprefix=(char*)path;
	if (nftw(path,windowsscansinglefile,20,FTW_PHYS)){
		fprintf(stderr,"directory scan failed\n");
		return NULL;
	}
	return &_wfirstEntry;
}
#endif

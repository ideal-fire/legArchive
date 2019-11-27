#ifndef THELEGARCHIVEHEADERFILEHASBEENINCLUDED
#define THELEGARCHIVEHEADERFILEHASBEENINCLUDED 
#include <stdint.h>

struct legArchiveEntry{
	char* filename;
	int64_t position;
	int32_t length;
};
struct legArchive{
	int32_t totalFiles;
	char* filename;
	char caseSensitive;
	struct legArchiveEntry* fileList;
};
struct legArchiveFile{
	FILE* fp;
	long internalPosition; // Relative to the start of this file
	size_t totalLength;
	char inArchive; // stupid variable that only exists to allow fake legArchiveFile
	// These next two are for reloading the file.
	char* filename; // Filename of the archive, malloc'd
	uint64_t startPosition; // Start position in the archive
};

#define MAGICNUMBERS "LEGARCH"
#define ENDTABLEIDENTIFICATION "LEGARCHTBL" // length must be 10
#define ARCHIVEVERSION 1

FILE* openArchiveFile(struct legArchive* _passedArchive, const char* _passedFilename);
void loadLegArchive(const char* _filename, char _caseSensitive, struct legArchive* _returnArchive);
void freeArchive(struct legArchive* _toFree);
void getAdvancedFile(struct legArchive* _passedArchive, const char* _passedFilename, struct legArchiveFile* _ret);
#endif

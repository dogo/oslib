/** @file VirtualFile.h
 *  @brief Virtual File Support Header File
 *
 *  This file contains functions and definitions related to Virtual File operations
 *  in the Oldschool Library (OSLib). It provides a universal interface for manipulating
 *  various file sources, including memory and standard file I/O, allowing users to define their own.
 */

#ifndef __OSL_VIRTUALFILE_H__
#define __OSL_VIRTUALFILE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup virtualfile Virtual Files
 * Virtual File support for OSLib.
 * This API is meant to provide a universal interface for manipulating file sources, allowing users to define their own.
 * @{
 */

/**
 * \brief Virtual File type.
 */
typedef struct {
	void *ioPtr;            //!< User data for IO processing (usually a pointer to data, FILE*, etc.)
	unsigned short type;    //!< Virtual file type (source number).
	unsigned short mode;    //!< File open mode (VF_O_READ, VF_O_WRITE, VF_O_READWRITE).
	unsigned long userData; //!< Additional data
	int offset, maxSize;    //!< Internal variables for memory-based (RAM / ROM) sources
} VIRTUAL_FILE;

/**
 * \brief Enumeration of the available file open modes.
 *
 * Note that some sources do not support some modes like READWRITE or WRITE.
 * In such cases, opening a file in one of these modes will fail, and VirtualFileOpen will return NULL.
 */
enum VF_OPEN_MODES {
	VF_O_READ,              //!< Read only
	VF_O_READWRITE,         //!< Read & Write
	VF_O_WRITE              //!< Write only
};

/**
 * \brief Structure representing a Virtual File Source.
 *
 * The structure holds function pointers for various file operations.
 */
typedef struct {
	int (*fOpen)(void *param1, int param2, int type, int mode, VIRTUAL_FILE* f); //!< Open a file
	int (*fClose)(VIRTUAL_FILE *f); //!< Close a file
	int (*fRead)(void *ptr, size_t size, size_t n, VIRTUAL_FILE* f); //!< Read from a file
	int (*fWrite)(const void *ptr, size_t size, size_t n, VIRTUAL_FILE* f); //!< Write to a file
	int (*fGetc)(VIRTUAL_FILE *f); //!< Read a single character from a file
	int (*fPutc)(int caractere, VIRTUAL_FILE *f); //!< Write a single character to a file
	char* (*fGets)(char *str, int maxLen, VIRTUAL_FILE *f); //!< Read a string from a file
	void (*fPuts)(const char *s, VIRTUAL_FILE *f); //!< Write a string to a file
	void (*fSeek)(VIRTUAL_FILE *f, int offset, int whence); //!< Set file position
	int (*fTell)(VIRTUAL_FILE *f); //!< Get current file position
	int (*fEof)(VIRTUAL_FILE *f); //!< Check for end of file
} VIRTUAL_FILE_SOURCE;

/**
 * \brief Virtual file list item.
 *
 * Used for RAM-based devices.
 */
typedef struct {
	const char *name;       //!< Virtual file name
	void *data;             //!< RAM data block
	int size;               //!< Block data size
	int *type;              //!< Associated source (e.g., &VF_MEMORY)
} OSL_VIRTUALFILENAME;

/**
 * \brief Initializes the virtual filesystem.
 *
 * This function is called automatically by OSLib, so there is no need to call it manually.
 */
void VirtualFileInit();

/**
 * \brief Opens a new virtual file.
 *
 * \param param1 Pointer to a string representing the file name.
 * \param param2 Should always be 0.
 * \param type File type. By default, can be VF_MEMORY or VF_FILE.
 * \param mode One of VF_OPEN_MODES.
 *
 * \return A pointer to the opened virtual file, or NULL if the operation failed.
 */
VIRTUAL_FILE *VirtualFileOpen(void *param1, int param2, int type, int mode);

/**
 * \brief Closes an open virtual file.
 *
 * \param f Pointer to the virtual file to close.
 *
 * \return 1 if successful, 0 otherwise.
 */
int VirtualFileClose(VIRTUAL_FILE *f);

/**
 * \defgroup virtualfile_io I/O routines
 * Routines for reading/writing to a virtual file.
 * @{
 */

/**
 * \brief Writes data to a file.
 *
 * \param ptr Pointer to the data to write.
 * \param size Size of each element to write.
 * \param n Number of elements to write.
 * \param f Pointer to the virtual file.
 *
 * \return The number of bytes effectively written.
 */
#define VirtualFileWrite(ptr, size, n, f) (VirtualFileGetSource(f)->fWrite(ptr, size, n, f))

/**
 * \brief Reads data from a file.
 *
 * \param ptr Pointer to the buffer where the data will be stored.
 * \param size Size of each element to read.
 * \param n Number of elements to read.
 * \param f Pointer to the virtual file.
 *
 * \return The number of bytes effectively read.
 */
#define VirtualFileRead(ptr, size, n, f) (VirtualFileGetSource(f)->fRead(ptr, size, n, f))

/**
 * \brief Reads a single character from a file.
 *
 * \param f Pointer to the virtual file.
 *
 * \return The next character (byte) in the file.
 */
#define VirtualFileGetc(f) (VirtualFileGetSource(f)->fGetc(f))

/**
 * \brief Writes a single character to a file.
 *
 * \param caractere The character to write.
 * \param f Pointer to the virtual file.
 *
 * \return The character value if successful, -1 otherwise.
 */
#define VirtualFilePutc(caractere, f) (VirtualFileGetSource(f)->fPutc(caractere, f))

/**
 * \brief Reads a string from a file.
 *
 * \param str Pointer to the buffer where the string will be stored.
 * \param maxLen Maximum length of the string.
 * \param f Pointer to the virtual file.
 *
 * \return A pointer to the read string.
 */
#define VirtualFileGets(str, maxLen, f) (VirtualFileGetSource(f)->fGets(str, maxLen, f))

/**
 * \brief Writes a string to a file.
 *
 * \param s Pointer to the string to write.
 * \param f Pointer to the virtual file.
 */
#define VirtualFilePuts(s, f) (VirtualFileGetSource(f)->fPuts(s, f))

/**
 * \brief Sets the current file position.
 *
 * \param f Pointer to the virtual file.
 * \param offset Offset from the whence parameter.
 * \param whence Position to base the offset from (SEEK_SET, SEEK_CUR, SEEK_END).
 */
#define VirtualFileSeek(f, offset, whence) (VirtualFileGetSource(f)->fSeek(f, offset, whence))

/**
 * \brief Returns the current file pointer position.
 *
 * \param f Pointer to the virtual file.
 *
 * \return The current file position.
 */
#define VirtualFileTell(f) (VirtualFileGetSource(f)->fTell(f))

/**
 * \brief Checks if the end of a file has been reached.
 *
 * \param f Pointer to the virtual file.
 *
 * \return 1 if the end of the file is reached, 0 otherwise.
 */
#define VirtualFileEof(f) (VirtualFileGetSource(f)->fEof(f))

/** @} */ // end of virtualfile_io

/**
 * \brief Registers a new virtual file source.
 *
 * \param vfs Pointer to a valid VIRTUAL_FILE_SOURCE interface containing the functions for handling the file source.
 *
 * \return The identifier of the source, or -1 if the operation failed.
 */
int VirtualFileRegisterSource(VIRTUAL_FILE_SOURCE *vfs);

/**
 * \brief Finds a file in the virtual filename list.
 *
 * This function is used in the virtual file source OPEN handler if it's a RAM-based device and param2 is 0.
 *
 * \param fname The name of the file.
 * \param type The file type.
 *
 * \return A pointer to the OSL_VIRTUALFILENAME structure, or NULL if the file is not found.
 */
OSL_VIRTUALFILENAME *oslFindFileInVirtualFilenameList(const char *fname, int type);

/** @brief Maximum number of virtual file sources. */
#define VF_MAX_SOURCES 16

/**
 * @brief Gets a pointer to the virtual file source associated with a given virtual file.
 *
 * @param vf Pointer to the virtual file.
 * @return A pointer to the associated VIRTUAL_FILE_SOURCE.
 */
#define VirtualFileGetSource(vf) (VirtualFileSources[(vf)->type])

/** @brief List of virtual file sources. */
extern VIRTUAL_FILE_SOURCE *VirtualFileSources[VF_MAX_SOURCES];

/** @brief Number of registered virtual file sources. */
extern int VirtualFileSourcesNb;

/** @brief List of virtual filenames used for RAM-based virtual files. */
extern OSL_VIRTUALFILENAME *osl_virtualFileList;

/** @brief Number of entries in the virtual file list. */
extern int osl_virtualFileListNumber;

/** @brief Default virtual file source type. */
extern int osl_defaultVirtualFileSource;

/** @brief Name of the temporary virtual file. */
extern const char *osl_tempFileName;

/**
 * \brief Reads an entire file into memory.
 *
 * The memory block is allocated in steps of 4kB, so even a 1kB file will occupy 16kB.
 * This function is not recommended for opening a lot of small files.
 *
 * \param f Pointer to the virtual file.
 * \param size Pointer to an integer that will store the number of bytes read.
 *
 * \return A pointer to the memory block containing the file data.
 */
extern void *oslReadEntireFileToMemory(VIRTUAL_FILE *f, int *size);

/* Memory-based source handlers */
extern int vfsMemOpen(void *param1, int param2, int type, int mode, VIRTUAL_FILE* f);
extern int vfsMemClose(VIRTUAL_FILE *f);
extern int vfsMemWrite(const void *ptr, size_t size, size_t n, VIRTUAL_FILE *f);
extern int vfsMemRead(void *ptr, size_t size, size_t n, VIRTUAL_FILE *f);
extern int vfsMemGetc(VIRTUAL_FILE *f);
extern int vfsMemPutc(int caractere, VIRTUAL_FILE *f);
extern char *vfsMemGets(char *str, int maxLen, VIRTUAL_FILE *f);
extern void vfsMemPuts(const char *s, VIRTUAL_FILE *f);
extern void vfsMemSeek(VIRTUAL_FILE *f, int offset, int whence);
extern int vfsMemTell(VIRTUAL_FILE *f);
extern int vfsMemEof(VIRTUAL_FILE *f);
extern VIRTUAL_FILE_SOURCE vfsMemory;

/**
 * \defgroup virtualfile_sources Virtual file sources
 * Virtual file sources available by default: memory and file.
 * @{
 */

/**
 * \brief Initializes the file system.
 *
 * This function is automatically called by OSLib during initialization.
 */
int oslInitVfsFile();

/**
 * \brief Sets the default virtual file source.
 *
 * OSLib will search in the current file list to determine if the file exists and is of a different type.
 *
 * \param source The default source (e.g., VF_FILE, VF_MEMORY, or any registered virtual file source).
 */
static inline void oslSetDefaultVirtualFileSource(int source) {
	osl_defaultVirtualFileSource = source;
}

/**
 * \brief Memory-based virtual file source.
 */
extern int VF_MEMORY;

/**
 * \brief File-based virtual file source.
 */
extern int VF_FILE;

/**
 * \brief Auto-select source.
 *
 * This option uses the current file list to find whether the file name exists in it.
 * If the file is not found, it uses the currently active source.
 */
#define VF_AUTO -2

/** @} */ // end of virtualfile_sources

/**
 * \defgroup virtualfile_ram RAM virtual files
 * RAM-based virtual file sources.
 * @{
 */

/**
 * \brief Gets the name of the temporary file.
 *
 * \return The name of the temporary file.
 */
static inline char *oslGetTempFileName() {
	return (char*)osl_tempFileName;
}

/**
 * \brief Sets the data associated with a temporary file.
 *
 * \param data Pointer to the data to associate with the temporary file.
 * \param size Size of the data.
 * \param type Pointer to the file type (e.g., &VF_MEMORY).
 */
extern void oslSetTempFileData(void *data, int size, int *type);

/**
 * \brief Adds a list of virtual files.
 *
 * This function allows the addition of files to the virtual file list, making them accessible as if they were real files.
 *
 * \param vfl Pointer to an array of OSL_VIRTUALFILENAME entries.
 * \param numberOfEntries Number of entries in the array.
 */
extern int oslAddVirtualFileList(OSL_VIRTUALFILENAME *vfl, int numberOfEntries);

/**
 * \brief Removes file entries from the virtual file list.
 *
 * \param vfl Pointer to an array of OSL_VIRTUALFILENAME entries.
 * \param numberOfEntries Number of entries in the array.
 */
extern void oslRemoveVirtualFileList(OSL_VIRTUALFILENAME *vfl, int numberOfEntries);

/** @} */ // end of virtualfile_ram

/** @} */ // end of virtualfile

#ifdef __cplusplus
}
#endif

#endif // __OSL_VIRTUALFILE_H__

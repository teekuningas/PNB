#if defined(__wii__)

#ifndef _LOADOBJ_INTERNAL_H_
#define _LOADOBJ_INTERNAL_H_

/**
 * \file loadobj_internal.h
 * \brief OBJ loading module internal functions
 */

/* Functions */

__inline int ReadLine ( char* objfile, char *line );

__inline int GetFaceType ( const char *line, u8 *uFaceType );

__inline int SeekToObject ( char *objfile, const char *objectname );

#else
#endif

#endif /* _LOADOBJ_INTERNAL_H_ */
/*
  Copyright (c) 2009-2017 Dave Gamble and CJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef __CJSON_H__
#define __CJSON_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* project version */
#define CJSON_VERSION_MAJOR 1
#define CJSON_VERSION_MINOR 7
#define CJSON_VERSION_PATCH 4

#include <stddef.h>

/* CJSON Types: */
#define CJSON_INVALID (0)
#define CJSON_FALSE  (1 << 0)
#define CJSON_TRUE   (1 << 1)
#define CJSON_NULL   (1 << 2)
#define CJSON_NUMBER (1 << 3)
#define CJSON_STRING (1 << 4)
#define CJSON_ARRAY  (1 << 5)
#define CJSON_OBJECT (1 << 6)
#define CJSON_RAW    (1 << 7) /* raw json */
#define CJSON_LONG   (1 << 8)
#define CJSON_DOUBLE (1 << 9)
#define CJSON_BOOLEAN (1 << 10)


#define CJSON_ISREFERENCE 256
#define CJSON_STRINGISCONST 512

/* The CJSON structure: */
typedef struct CJSON
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct CJSON *next;
    struct CJSON *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct CJSON *child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==CJSON_STRING  and type == CJSON_RAW */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cjson_setNumberValue instead */
    long valuelong;
    /* The item's number, if type==CJSON_NUMBER */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} CJSON;

typedef struct cjson_hooks
{
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} CJSON_HOOKS;

typedef int CJSON_BOOL;

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif
#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 2 define options:

CJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
CJSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
CJSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the CJSON_API_VISIBILITY flag to "export" the same symbols the way CJSON_EXPORT_SYMBOLS does

*/

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(CJSON_HIDE_SYMBOLS) && !defined(CJSON_IMPORT_SYMBOLS) && !defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_EXPORT_SYMBOLS
#endif

#if defined(CJSON_HIDE_SYMBOLS)
#define CJSON_PUBLIC(type)   type __stdcall
#elif defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_PUBLIC(type)   __declspec(dllexport) type __stdcall
#elif defined(CJSON_IMPORT_SYMBOLS)
#define CJSON_PUBLIC(type)   __declspec(dllimport) type __stdcall
#endif
#else /* !WIN32 */
#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(CJSON_API_VISIBILITY)
#define CJSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define CJSON_PUBLIC(type) type
#endif
#endif

/* Limits how deeply nested arrays/objects can be before CJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef CJSON_NESTING_LIMIT
#define CJSON_NESTING_LIMIT 1000
#endif

/* returns the version of CJSON as a string */
CJSON_PUBLIC(const char*) cjson_version(void);

/* Supply malloc, realloc and free functions to CJSON */
CJSON_PUBLIC(void) cjson_initHooks(CJSON_HOOKS* hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of cjson_parse (with cjson_delete) and cjson_print (with stdlib free, cjson_hooks.free_fn, or cjson_free as appropriate). The exception is cjson_printPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a CJSON object you can interrogate. */
CJSON_PUBLIC(CJSON *) cjson_parse(const char *value);
/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match cjson_getErrorPtr(). */
CJSON_PUBLIC(CJSON *) cjson_parseWithOpts(const char *value, const char **return_parse_end, CJSON_BOOL require_null_terminated);

/* Render a CJSON entity to text for transfer/storage. */
CJSON_PUBLIC(char *) cjson_print(const CJSON *item);
/* Render a CJSON entity to text for transfer/storage without any formatting. */
CJSON_PUBLIC(char *) cjson_printUnformatted(const CJSON *item);
/* Render a CJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
CJSON_PUBLIC(char *) cjson_printBuffered(const CJSON *item, int prebuffer, CJSON_BOOL fmt);
/* Render a CJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: CJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
CJSON_PUBLIC(CJSON_BOOL) cjson_printPreallocated(CJSON *item, char *buffer, const int length, const CJSON_BOOL format);
/* Delete a CJSON entity and all subentities. */
CJSON_PUBLIC(void) cjson_delete(CJSON *c);

/* Returns the number of items in an array (or object). */
CJSON_PUBLIC(int) cjson_getArraySize(const CJSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
CJSON_PUBLIC(CJSON *) cjson_getArrayItem(const CJSON *array, int index);
/* Get item "string" from object. Case insensitive. */
CJSON_PUBLIC(CJSON *) cjson_getObjectItem(const CJSON * const object, const char * const string);
CJSON_PUBLIC(CJSON *) cjson_getObjectItemCaseSensitive(const CJSON * const object, const char * const string);
CJSON_PUBLIC(CJSON_BOOL) cjson_hasObjectItem(const CJSON *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cjson_parse() returns 0. 0 when cjson_parse() succeeds. */
CJSON_PUBLIC(const char *) cjson_getErrorPtr(void);

/* Check if the item is a string and return its valuestring */
CJSON_PUBLIC(char *) cjson_getStringValue(CJSON *item);

/* These functions check the type of an item */
CJSON_PUBLIC(CJSON_BOOL) cjson_isInvalid(const CJSON * const item);
CJSON_PUBLIC(CJSON_BOOL) cjson_isFalse(const CJSON * const item);
CJSON_PUBLIC(CJSON_BOOL) cjson_isTrue(const CJSON * const item);
CJSON_PUBLIC(CJSON_BOOL) cjson_isBool(const CJSON * const item);
CJSON_PUBLIC(CJSON_BOOL) cjson_isNull(const CJSON * const item);
CJSON_PUBLIC(CJSON_BOOL) cjson_isNumber(const CJSON * const item);
CJSON_PUBLIC(CJSON_BOOL) cjson_isString(const CJSON * const item);
CJSON_PUBLIC(CJSON_BOOL) cjson_isArray(const CJSON * const item);
CJSON_PUBLIC(CJSON_BOOL) cjson_isObject(const CJSON * const item);
CJSON_PUBLIC(CJSON_BOOL) cjson_isRaw(const CJSON * const item);

/* These calls create a CJSON item of the appropriate type. */
CJSON_PUBLIC(CJSON *) cjson_createNull(void);
CJSON_PUBLIC(CJSON *) cjson_createTrue(void);
CJSON_PUBLIC(CJSON *) cjson_createFalse(void);
CJSON_PUBLIC(CJSON *) cjson_createBool(CJSON_BOOL boolean);
CJSON_PUBLIC(CJSON *) cjson_createNumber(double num);
CJSON_PUBLIC(CJSON *) cjson_createString(const char *string);
/* raw json */
CJSON_PUBLIC(CJSON *) cjson_createRaw(const char *raw);
CJSON_PUBLIC(CJSON *) cjson_createArray(void);
CJSON_PUBLIC(CJSON *) cjson_createObject(void);

/* Create a string where valuestring references a string so
 * it will not be freed by cjson_delete */
CJSON_PUBLIC(CJSON *) cjson_createStringReference(const char *string);
/* Create an object/arrray that only references it's elements so
 * they will not be freed by cjson_delete */
CJSON_PUBLIC(CJSON *) cjson_createObjectReference(const CJSON *child);
CJSON_PUBLIC(CJSON *) cjson_createArrayReference(const CJSON *child);

/* These utilities create an Array of count items. */
CJSON_PUBLIC(CJSON *) cjson_createIntArray(const int *numbers, int count);
CJSON_PUBLIC(CJSON *) cjson_createFloatArray(const float *numbers, int count);
CJSON_PUBLIC(CJSON *) cjson_createDoubleArray(const double *numbers, int count);
CJSON_PUBLIC(CJSON *) cjson_createStringArray(const char **strings, int count);

/* Append item to the specified array/object. */
CJSON_PUBLIC(void) cjson_addItemToArray(CJSON *array, CJSON *item);
CJSON_PUBLIC(void) cjson_addItemToObject(CJSON *object, const char *string, CJSON *item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the CJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & CJSON_STRINGISCONST) is zero before
 * writing to `item->string` */
CJSON_PUBLIC(void) cjson_addItemToObjectCS(CJSON *object, const char *string, CJSON *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing CJSON to a new CJSON, but don't want to corrupt your existing CJSON. */
CJSON_PUBLIC(void) cjson_addItemReferenceToArray(CJSON *array, CJSON *item);
CJSON_PUBLIC(void) cjson_addItemReferenceToObject(CJSON *object, const char *string, CJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
CJSON_PUBLIC(CJSON *) cjson_detachItemViaPointer(CJSON *parent, CJSON * const item);
CJSON_PUBLIC(CJSON *) cjson_detachItemFromArray(CJSON *array, int which);
CJSON_PUBLIC(void) cjson_deleteItemFromArray(CJSON *array, int which);
CJSON_PUBLIC(CJSON *) cjson_detachItemFromObject(CJSON *object, const char *string);
CJSON_PUBLIC(CJSON *) cjson_detachItemFromObjectCaseSensitive(CJSON *object, const char *string);
CJSON_PUBLIC(void) cjson_deleteItemFromObject(CJSON *object, const char *string);
CJSON_PUBLIC(void) cjson_deleteItemFromObjectCaseSensitive(CJSON *object, const char *string);

/* Update array items. */
CJSON_PUBLIC(void) cjson_insertItemInArray(CJSON *array, int which, CJSON *newitem); /* Shifts pre-existing items to the right. */
CJSON_PUBLIC(CJSON_BOOL) cjson_replaceItemViaPointer(CJSON * const parent, CJSON * const item, CJSON * replacement);
CJSON_PUBLIC(void) cjson_replaceItemInArray(CJSON *array, int which, CJSON *newitem);
CJSON_PUBLIC(void) cjson_replaceItemInObject(CJSON *object,const char *string,CJSON *newitem);
CJSON_PUBLIC(void) cjson_replaceItemInObjectCaseSensitive(CJSON *object,const char *string,CJSON *newitem);

/* Duplicate a CJSON item */
CJSON_PUBLIC(CJSON *) cjson_duplicate(const CJSON *item, CJSON_BOOL recurse);
/* Duplicate will create a new, identical CJSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two CJSON items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
CJSON_PUBLIC(CJSON_BOOL) cjson_compare(const CJSON * const a, const CJSON * const b, const CJSON_BOOL case_sensitive);


CJSON_PUBLIC(void) cjson_minify(char *json);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
CJSON_PUBLIC(CJSON*) cjson_addNullToObject(CJSON * const object, const char * const name);
CJSON_PUBLIC(CJSON*) cjson_addTrueToObject(CJSON * const object, const char * const name);
CJSON_PUBLIC(CJSON*) cjson_addFalseToObject(CJSON * const object, const char * const name);
CJSON_PUBLIC(CJSON*) cjson_addBoolToObject(CJSON * const object, const char * const name, const CJSON_BOOL boolean);
CJSON_PUBLIC(CJSON*) cjson_addNumberToObject(CJSON * const object, const char * const name, const double number);
CJSON_PUBLIC(CJSON*) cjson_addStringToObject(CJSON * const object, const char * const name, const char * const string);
CJSON_PUBLIC(CJSON*) cjson_addRawToObject(CJSON * const object, const char * const name, const char * const raw);
CJSON_PUBLIC(CJSON*) cjson_addObjectToObject(CJSON * const object, const char * const name);
CJSON_PUBLIC(CJSON*) cjson_addArrayToObject(CJSON * const object, const char * const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define cjson_setIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the cjson_setNumberValue macro */
CJSON_PUBLIC(double) cjson_setNumberHelper(CJSON *object, double number);
#define cjson_setNumberValue(object, number) ((object != NULL) ? cjson_setNumberHelper(object, (double)number) : (number))

/* Macro for iterating over an array or object */
#define cjson_arrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with cjson_initHooks */
CJSON_PUBLIC(void *) cjson_malloc(size_t size);
CJSON_PUBLIC(void) cjson_free(void *object);

#ifdef __cplusplus
}
#endif

#endif

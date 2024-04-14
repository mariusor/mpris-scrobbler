/* stb_ds.h - v0.4 - public domain data structures - Sean Barrett 2019

   This is a single-header-file library that provides easy-to-use
   dynamic arrays and hash tables for C (also works in C++).

   For a gentle introduction:
      http://nothings.org/stb_ds

   To use this library, do this in *one* C or C++ file:
      #define STB_DS_IMPLEMENTATION
      #include "stb_ds.h"

TABLE OF CONTENTS

  Table of Contents
  Compile-time options
  License
  Documentation
  Notes
  Credits

COMPILE-TIME OPTIONS

  #define STBDS_NO_SHORT_NAMES

     This flag needs to be set globally.

     By default stb_ds exposes shorter function names that are not qualified
     with the "stbds_" prefix. If these names conflict with the names in your
     code, define this flag.

  #define STBDS_SIPHASH_2_4

     This flag only needs to be set in the file containing #define STB_DS_IMPLEMENTATION.

     By default stb_ds.h hashes using a weaker variant of SipHash and a custom hash for
     4- and 8-byte keys. On 64-bit platforms, you can define the above flag to force
     stb_ds.h to use specification-compliant SipHash-2-4 for all keys. Doing so makes
     hash table insertion about 20% slower on 4- and 8-byte keys, 5% slower on
     64-byte keys, and 10% slower on 256-byte keys on my test computer.

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

DOCUMENTATION

  Dynamic Arrays

    Non-function interface:

      Declare an empty dynamic array of type T
        T* foo = NULL;

      Access the i'th item of a dynamic array 'foo' of type T, T* foo:
        foo[i]

    Functions (actually macros)

      arrfree:
        void arrfree(T*);
          Frees the array.

      arrlen:
        ptrdiff_t arrlen(T*);
          Returns the number of elements in the array.

      arrlenu:
        size_t arrlenu(T*);
          Returns the number of elements in the array as an unsigned type.

      arrpop:
        T arrpop(T* a)
          Removes the final element of the array and returns it.

      arrput:
        T arrput(T* a, T b);
          Appends the item b to the end of array a. Returns b.

      arrins:
        T arrins(T* a, int p, T b);
          Inserts the item b into the middle of array a, into a[p],
          moving the rest of the array over. Returns b.

      arrinsn:
        void arrins(T* a, int p, int n);
          Inserts n uninitialized items into array a starting at a[p],
          moving the rest of the array over.

      arrdel:
        void arrdel(T* a, int p);
          Deletes the element at a[p], moving the rest of the array over.

      arrdeln:
        void arrdel(T* a, int p, int n);
          Deletes n elements starting at a[p], moving the rest of the array over.

      arrdelswap:
        void arrdelswap(T* a, int p);
          Deletes the element at a[p], replacing it with the element from
          the end of the array. O(1) performance.

      arrsetlen:
        void arrsetlen(T* a, int n);
          Changes the length of the array to n. Allocates uninitialized
          slots at the end if necessary.

      arrsetcap:
        size_t arrsetcap(T* a, int n);
          Sets the length of allocated storage to at least n. It will not
          change the length of the array.

      arrcap:
        size_t arrcap(T* a);
          Returns the number of total elements the array can contain without
          needing to be reallocated.

NOTES

  * These data structures are realloc'd when they grow, and the macro "functions"
    write to the provided pointer. This means: (a) the pointer must be an lvalue,
    and (b) the pointer to the data structure is not stable, and you must maintain
    it the same as you would a realloc'd pointer. For example, if you pass a pointer
    to a dynamic array to a function which updates it, the function must return
    back the new pointer to the caller. This is the price of trying to do this in C.

  * You iterate over the contents of a dynamic array and a hashmap in exactly
    the same way, using arrlen/hmlen/shlen:

      for (i=0; i < arrlen(foo); ++i)
         ... foo[i] ...

  * All operations except arrins/arrdel are O(1) amortized, but individual
    operations can be slow, so these data structures may not be suitable
    for real time use. Dynamic arrays double in capacity as needed, so
    elements are copied an average of once. Hash tables double/halve
    their size as needed, with appropriate hysteresis to maintain O(1)
    performance.

NOTES - DYNAMIC ARRAY

  * If you know how long a dynamic array is going to be in advance, you can avoid
    extra memory allocations by using arrsetlen to allocate it to that length in
    advance and use foo[n] while filling it out, or arrsetcap to allocate the memory
    for that length and use arrput/arrpush as normal.

  * Unlike some other versions of the dynamic array, this version should
    be safe to use with strict-aliasing optimizations.

CREDITS

  Sean Barrett -- library, idea for dynamic array API/implementation
  Per Vognsen  -- idea for hash table API/implementation
  Rafael Sachetto -- arrpop()
*/
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#ifndef INCLUDE_STB_DS_H
#define INCLUDE_STB_DS_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#ifndef STBDS_NO_SHORT_NAMES
#define arrlen      stbds_arrlen
#define arrlenu     stbds_arrlenu
#define arrput      stbds_arrput
#define arrpush     stbds_arrput
#define arrpop      stbds_arrpop
#define arrfree     stbds_arrfree
#define arraddn     stbds_arraddn
#define arrsetlen   stbds_arrsetlen
#define arrlast     stbds_arrlast
#define arrins      stbds_arrins
#define arrinsn     stbds_arrinsn
#define arrdel      stbds_arrdel
#define arrdeln     stbds_arrdeln
#define arrdelswap  stbds_arrdelswap
#define arrcap      stbds_arrcap
#define arrsetcap   stbds_arrsetcap
#endif

///////////////
//
// Everything below here is implementation details
//

extern void * stbds_arrgrowf(void *a, size_t elemsize, size_t addlen, size_t min_cap);


#if defined(__GNUC__) || defined(__clang__)
#define STBDS_HAS_TYPEOF
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define STBDS_HAS_LITERAL_ARRAY
#endif

#define stbds_header(t)  ((stbds_array_header *) (t) - 1)

#define stbds_arrsetcap(a,n) (stbds_arrgrow(a,0,n))
#define stbds_arrsetlen(a,n) ((stbds_arrcap(a) < n ? stbds_arrsetcap(a,n),0 : 0), (a) ? stbds_header(a)->length = (n) : 0)
#define stbds_arrcap(a)       ((a) ? stbds_header(a)->capacity : 0)
#define stbds_arrlen(a)       (size_t)((a) ? (ptrdiff_t) stbds_header(a)->length : 0)
#define stbds_arrlenu(a)      ((a) ?             stbds_header(a)->length : 0)
#define stbds_arrput(a,v)     (stbds_arrmaybegrow(a,1), (a)[stbds_header(a)->length++] = (v))
#define stbds_arrpush         stbds_arrput  // synonym
#define stbds_arrpop(a)       (stbds_header(a)->length--, (a)[stbds_header(a)->length])
#define stbds_arraddn(a,n)    (stbds_arrmaybegrow(a,n), stbds_header(a)->length += (n))
#define stbds_arrlast(a)      ((a)[stbds_header(a)->length-1])
#define stbds_arrfree(a)      ((void) ((a) ? realloc(stbds_header(a),0) : 0), (a)=NULL)
#define stbds_arrdel(a,i)     stbds_arrdeln(a,i,1)
#define stbds_arrdeln(a,i,n)  (memmove(&(a)[i], &(a)[(i)+(n)], sizeof *(a) * (stbds_header(a)->length-(n)-(i))), stbds_header(a)->length -= (n))
#define stbds_arrdelswap(a,i) ((a)[i] = stbds_arrlast(a), stbds_header(a)->length -= 1)
#define stbds_arrinsn(a,i,n)  (stbds_arraddn((a),(n)), memmove(&(a)[(i)+(n)], &(a)[i], sizeof *(a) * (stbds_header(a)->length-(n)-(i))))
#define stbds_arrins(a,i,v)   (stbds_arrinsn((a),(i),1), (a)[i]=(v))

#define stbds_arrmaybegrow(a,n)  ((!(a) || stbds_header(a)->length + (n) > stbds_header(a)->capacity) \
                                  ? (stbds_arrgrow(a,n,0),0) : 0)

#define stbds_arrgrow(a,b,c)   ((a) = stbds_arrgrowf_wrapper((a), sizeof *(a), (b), (c)))


typedef struct
{
  size_t      length;
  size_t      capacity;
  void      * hash_table;
  ptrdiff_t   temp;
} stbds_array_header;

#define stbds_arrgrowf_wrapper            stbds_arrgrowf

#endif // INCLUDE_STB_DS_H


//////////////////////////////////////////////////////////////////////////////
//
//   IMPLEMENTATION
//

#ifdef STB_DS_IMPLEMENTATION
#include <assert.h>
#include <string.h>

#ifndef STBDS_ASSERT
#define STBDS_ASSERT_WAS_UNDEFINED
#define STBDS_ASSERT(x)   ((void) 0)
#endif

#ifdef STBDS_STATISTICS
#define STBDS_STATS(x)   x
size_t stbds_array_grow;
#else
#define STBDS_STATS(x)
#endif

//
// stbds_arr implementation
//

void *stbds_arrgrowf(void *a, size_t elemsize, size_t addlen, size_t min_cap)
{
  void *b;
  size_t min_len = stbds_arrlen(a) + addlen;

  // compute the minimum capacity needed
  if (min_len > min_cap)
    min_cap = min_len;

  if (min_cap <= stbds_arrcap(a))
    return a;

  // increase needed capacity to guarantee O(1) amortized
  if (min_cap < 2 * stbds_arrcap(a))
    min_cap = 2 * stbds_arrcap(a);
  else if (min_cap < 4)
    min_cap = 4;

  b = realloc((a) ? stbds_header(a) : 0, elemsize * min_cap + sizeof(stbds_array_header));
  b = (char *) b + sizeof(stbds_array_header);
  if (a == NULL) {
    stbds_header(b)->length = 0;
    stbds_header(b)->hash_table = 0;
  } else {
    STBDS_STATS(++stbds_array_grow);
  }
  stbds_header(b)->capacity = min_cap;
  return b;
}

#endif
/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2019 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/

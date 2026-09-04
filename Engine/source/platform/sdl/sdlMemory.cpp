#include "platform/platform.h"

#include <stdlib.h>
#include <string.h>

void* dRealMalloc(dsize_t s)
{
   return malloc(s);
}

void dRealFree(void* p)
{
   free(p);
}

void* dMemcpy(void *dst, const void *src, dsize_t size)
{
   return memcpy(dst, src, size);
}

void* dMemmove(void *dst, const void *src, dsize_t size)
{
   return memmove(dst, src, size);
}

void* dMemset(void *dst, int c, dsize_t size)
{
   return memset(dst, c, size);
}

S32 dMemcmp(const void *ptr1, const void *ptr2, dsize_t len)
{
   return memcmp(ptr1, ptr2, len);
}

void* dMalloc_aligned(dsize_t size, int alignment)
{
   void* ptr = NULL;
   if (posix_memalign(&ptr, alignment, size) != 0)
      return NULL;
   return ptr;
}

void dFree_aligned(void* ptr)
{
   free(ptr);
}

// The contents of this file are subject to the Mozilla Public License
// Version 1.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License
// at http://www.mozilla.org/MPL/
//
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
// the License for the specific language governing rights and
// limitations under the License.
//
// The Initial Developer of the Original Code is Kipp E.B. Hickman.

#include "libmalloc.h"
#include <memory.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <setjmp.h>
#include <dlfcn.h>
#include <link.h>

extern "C" {
#ifdef NEED_WRAPPERS
  void* __wrap_malloc(size_t);
  void* __wrap_realloc(void*, size_t);
  void __wrap_free(void*);
  void* __wrap___builtin_new(size_t);
  void __wrap___builtin_delete(void*);
  void* __wrap___builtin_vec_new(size_t);
  void __wrap___builtin_vec_delete(void*);
  void* __wrap_PR_Malloc(size_t);
  void* __wrap_PR_Calloc(size_t, size_t);
  void* __wrap_PR_Realloc(void*, size_t);
  void __wrap_PR_Free(void*);
#endif
}

static int gLogFD = -1;
static u_long gFlags;

#define MARKER0		0xFFFFFFFF
#define MARKER1		0xEEEEEEEE
#define MARKER2_0	0xD8D8D8D8
#define MARKER2_1	0xE8E8E8E8

#define PATTERN_BYTE      0xFE
#define FREE_PATTERN_BYTE 0xDE

struct Header {
  u_long marker0;
  size_t rawSize;		// user requested size
  size_t size;			// size rounded up
  u_long marker1;
};

struct Trailer {
  u_long marker2[2];
};

//----------------------------------------------------------------------

#if defined(i386)
static void CrawlStack(malloc_log_entry* me, jmp_buf jb)
{
  u_long* bp = (u_long*) (jb[0].__jmpbuf[JB_BP]);
  u_long numpcs = 0;
  int skip = 2;
  while (numpcs < MAX_STACK_CRAWL) {
    u_long* nextbp = (u_long*) *bp++;
    u_long pc = *bp;
    if ((pc < 0x08000000) || (pc > 0x7fffffff) || (nextbp < bp)) {
      break;
    }
    if (--skip < 0) {
      me->pcs[numpcs++] = (char*) pc;
    }
    bp = nextbp;
  }
  me->numpcs = numpcs;
}
#endif

//----------------------------------------------------------------------

#ifdef linux
static void DumpAddressMap()
{
  int mfd = open("malloc-map", O_CREAT|O_WRONLY|O_TRUNC, 0666);
  if (mfd >= 0) {
    malloc_map_entry mme;
    link_map* map = _dl_loaded;
    while (NULL != map) {
      if (0 != map->l_addr) {
	mme.nameLen = strlen(map->l_name);
	mme.address = map->l_addr;
	write(mfd, &mme, sizeof(mme));
	write(mfd, map->l_name, mme.nameLen);
      }
      map = map->l_next;
    }
    close(mfd);
  }
}
#endif

//----------------------------------------------------------------------

static int Verify(Header* h)
{
  // Sanity check the header first
  if ((h->marker0 != MARKER0) ||
      (h->marker1 != MARKER1) ||
      (h->rawSize > h->size)) {
    DumpAddressMap();
    abort();
  }

  // Sanity check the trailer second
  Trailer* t = (Trailer*) ((char*)(h + 1) + h->size);
  if ((t->marker2[0] != MARKER2_0) ||
      (t->marker2[1] != MARKER2_1)) {
    DumpAddressMap();
    abort();
  }

  // Verify there were no overruns
  size_t fill = h->size - h->rawSize;
  if (0 != fill) {
    unsigned char* cp = ((unsigned char*)(h + 1)) + h->rawSize;
    unsigned char* end = cp + fill;
    while (cp < end) {
      unsigned char ch = *cp++;
      if (ch != PATTERN_BYTE) {
	DumpAddressMap();
	abort();
      }
    }
  }
  return 1;
}

static void
Log(int aType, void* aAddr, size_t aSize, void* aOldAddr)
{
  malloc_log_entry me;

  me.type = (u_long) aType;
  me.address = (u_long) aAddr;
  me.size = (u_long) aSize;
  me.oldaddress = (u_long) aOldAddr;

  jmp_buf jb;
  setjmp(jb);
  CrawlStack(&me, jb);

  write(gLogFD, &me, sizeof(me) - MAX_STACK_CRAWL*sizeof(char*) +
	me.numpcs*sizeof(char*));
}

static void*
MallocHook(size_t aSize, u_long aLogType)
{
  size_t roundedSize = aSize;
  roundedSize = ((roundedSize + 4) >> 2) << 2;

  void* ptr = REAL_MALLOC(sizeof(Header) + roundedSize + sizeof(Trailer));

  if (NULL != ptr) {
    Header* h = (Header*) ptr;
    h->rawSize = aSize;
    h->size = roundedSize;
    h->marker0 = MARKER0;
    h->marker1 = MARKER1;

    ptr = (void*) ((char*)(h+1));

    // Fill entire memory with a pattern to help detect overruns and
    // usage of un-written memory
    if (0 != aSize) {
      memset(ptr, PATTERN_BYTE, aSize);
    }
    size_t fill = roundedSize - aSize;
    if (0 != fill) {
      memset((char*)ptr + aSize, PATTERN_BYTE, fill);
    }

    Trailer* t = (Trailer*) ((char*)ptr + roundedSize);
    t->marker2[0] = MARKER2_0;
    t->marker2[1] = MARKER2_1;

    if (LIBMALLOC_LOG & gFlags) {
      Log(aLogType, ptr, aSize, 0);
    }
  }

  return ptr;
}

static void
FreeHook(void* aAddr, u_long aLogType)
{
  if (0 == aAddr) {
    return;
  }
  if (LIBMALLOC_LOG & gFlags) {
    Log(aLogType, aAddr, 0, 0);
  }
  Header* h = (Header*) ((char*)aAddr - sizeof(Header));
  if (Verify(h)) {
    // Munge the header so that a dup-free will fail the verify
    h->marker0 = 0xDEADBEEF;
    h->marker1 = 0xDEADBEEF;

    // Munge the body of the allocation so that if the user
    // still has a live reference they get messed up
    void* ptr = (void*) ((char*)(h+1));
    memset(ptr, FREE_PATTERN_BYTE, h->rawSize);
    REAL_FREE(h);
  }
  else {
    REAL_FREE(aAddr);
  }
}

static void*
ReallocHook(void* aOldAddr, size_t aSize)
{
  if (0 == aOldAddr) {
    return MallocHook(aSize, malloc_log_malloc);
  }
  Header* oldh = (Header*) ((char*)aOldAddr - sizeof(Header));
  if (!Verify(oldh)) {
    return REAL_REALLOC(aOldAddr, aSize);
  }
  size_t oldSize = oldh->rawSize;

  size_t roundedSize = aSize;
  roundedSize = ((roundedSize + 4) >> 2) << 2;

  void* ptr = REAL_MALLOC(sizeof(Header) + roundedSize + sizeof(Trailer));

  if (NULL != ptr) {
    Header* h = (Header*) ptr;
    h->rawSize = aSize;
    h->size = roundedSize;
    h->marker0 = MARKER0;
    h->marker1 = MARKER1;

    ptr = (void*) ((char*)(h+1));

    // Fill extra memory with a pattern to help detect overruns
    size_t fill = roundedSize - aSize;
    if (0 != fill) {
      memset((char*)ptr + aSize, PATTERN_BYTE, fill);
    }

    Trailer* t = (Trailer*) ((char*)ptr + roundedSize);
    t->marker2[0] = MARKER2_0;
    t->marker2[1] = MARKER2_1;

    // Copy old memory into new memory (don't copy too much!)
    size_t copy = oldSize;
    if (copy > aSize) copy = aSize;
    memcpy(ptr, aOldAddr, copy);

    REAL_FREE(oldh);

    if (LIBMALLOC_LOG & gFlags) {
      Log(malloc_log_realloc, ptr, aSize, aOldAddr);
    }
  }
  return ptr;
}

u_long
SetMallocFlags(u_long aFlags)
{
  u_long old = gFlags;
  gFlags = aFlags;

  if ((-1 == gLogFD) && ((LIBMALLOC_LOG|LIBMALLOC_LOG_RC) & gFlags)) {
    gLogFD = open("malloc-log", O_CREAT|O_WRONLY|O_TRUNC, 0666);
    if (gLogFD < 0) {
      gFlags &= ~LIBMALLOC_LOG;
      printf("unable to create malloc-log: %d\n", errno);
    }
  }
  if ((gLogFD >= 0) && (0 == ((LIBMALLOC_LOG|LIBMALLOC_LOG_RC) & gFlags))) {
    close(gLogFD);
    gLogFD = -1;
  }

  // Try to guarantee that the address map is always dumped
  atexit(DumpAddressMap);

  return old;
}

static int gFirstTime = 1;

static void Init()
{
  gFirstTime = 0;
  u_long flags = 0;
  char* s = getenv("LIBMALLOC_LOG");
  if (s) {
    flags = atoi(s);
    if (LIBMALLOC_LOG & flags) {
      char m1[] = "dbgmalloc: enabled memory logging\n";
      write(1, m1, sizeof(m1)-1);
    }
    if (LIBMALLOC_LOG_RC & flags) {
      char m2[] = "dbgmalloc: enabled refcnt logging\n";
      write(1, m2, sizeof(m2)-1);
    }
  }
  SetMallocFlags(flags);
}

//----------------------------------------------------------------------

#ifdef NEED_WRAPPERS
void* __wrap_malloc(size_t aSize)
{
  if (gFirstTime) {
    Init();
  }
  return MallocHook(aSize, malloc_log_malloc);
}

void* __wrap_realloc(void* aPtr, size_t aSize)
{
  if (gFirstTime) {
    Init();
  }
  return ReallocHook(aPtr, aSize);
}

void __wrap_free(void* aPtr)
{
  if (gFirstTime) {
    Init();
  }
  FreeHook(aPtr, malloc_log_free);
}

void* __wrap___builtin_new(size_t aSize)
{
  if (gFirstTime) {
    Init();
  }
  return MallocHook(aSize, malloc_log_new);
}

void __wrap___builtin_delete(void* aPtr)
{
  if (gFirstTime) {
    Init();
  }
  FreeHook(aPtr, malloc_log_delete);
}

void* __wrap___builtin_vec_new(size_t aSize)
{
  if (gFirstTime) {
    Init();
  }
  return MallocHook(aSize, malloc_log_new);
}

void __wrap___builtin_vec_delete(void* aPtr)
{
  if (gFirstTime) {
    Init();
  }
  FreeHook(aPtr, malloc_log_delete);
}

void* __wrap_PR_Malloc(size_t aSize)
{
  if (gFirstTime) {
    Init();
  }
  return MallocHook(aSize, malloc_log_malloc);
}

void* __wrap_PR_Calloc(size_t aSize, size_t aBsize)
{
  if (gFirstTime) {
    Init();
  }
  size_t size = aSize*aBsize;
  void* ptr = MallocHook(size, malloc_log_malloc);
  if (NULL != ptr) {
    memset(ptr, 0, size);
  }
  return ptr;
}

void* __wrap_PR_Realloc(void* aPtr, size_t aSize)
{
  if (gFirstTime) {
    Init();
  }
  return ReallocHook(aPtr, aSize);
}

void __wrap_PR_Free(void* aPtr)
{
  if (gFirstTime) {
    Init();
  }
  FreeHook(aPtr, malloc_log_free);
}
#endif

//----------------------------------------

// Strong symbols so that libc references are routed to us

void* malloc(size_t aSize)
{
  if (gFirstTime) {
    Init();
  }
  return MallocHook(aSize, malloc_log_malloc);
}

void* realloc(void* aPtr, size_t aSize)
{
  if (gFirstTime) {
    Init();
  }
  return ReallocHook(aPtr, aSize);
}

void free(void* aPtr)
{
  if (gFirstTime) {
    Init();
  }
  FreeHook(aPtr, malloc_log_free);
}

void* calloc(size_t aSize, size_t aBsize)
{
  if (gFirstTime) {
    Init();
  }
  size_t size = aSize*aBsize;
  void* ptr = MallocHook(size, malloc_log_malloc);
  if (NULL != ptr) {
    memset(ptr, 0, size);
  }
  return ptr;
}

void cfree(void* ptr)
{
  if (gFirstTime) {
    Init();
  }
  FreeHook(ptr, malloc_log_free);
}

void* memalign(size_t alignment, size_t size)
{
  ::abort();
}

void* valloc(size_t size)
{
  ::abort();
}

void* pvalloc(size_t size)
{
  ::abort();
}

void* __builtin_new(size_t aSize)
{
  if (gFirstTime) {
    Init();
  }
  return MallocHook(aSize, malloc_log_new);
}

void __builtin_delete(void* aPtr)
{
  if (gFirstTime) {
    Init();
  }
  FreeHook(aPtr, malloc_log_delete);
}

void* __builtin_vec_new(size_t aSize)
{
  if (gFirstTime) {
    Init();
  }
  return MallocHook(aSize, malloc_log_new);
}

void __builtin_vec_delete(void* aPtr)
{
  if (gFirstTime) {
    Init();
  }
  FreeHook(aPtr, malloc_log_delete);
}

void
__log_addref(void* p, int oldrc, int newrc)
{
  if (gFirstTime) {
    Init();
  }
  if (LIBMALLOC_LOG_RC & gFlags) {
    Log(malloc_log_addref, p, size_t(oldrc), (void*)newrc);
  }
}

void
__log_release(void* p, int oldrc, int newrc)
{
  if (gFirstTime) {
    Init();
  }
  if (LIBMALLOC_LOG_RC & gFlags) {
    Log(malloc_log_release, p, size_t(oldrc), (void*)newrc);
  }
}

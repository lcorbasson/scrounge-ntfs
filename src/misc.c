/* 
 * AUTHOR
 * N. Nielsen
 *
 * LICENSE
 * This software is in the public domain.
 *
 * The software is provided "as is", without warranty of any kind,
 * express or implied, including but not limited to the warranties
 * of merchantability, fitness for a particular purpose, and
 * noninfringement. In no event shall the author(s) be liable for any
 * claim, damages, or other liability, whether in an action of
 * contract, tort, or otherwise, arising from, out of, or in connection
 * with the software or the use or other dealings in the software.
 * 
 * SUPPORT
 * Send bug reports to: <nielsen@memberwebs.com>
 */

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "usuals.h"
#include "compat.h"
#include "memref.h"
#include "locks.h"


/* These locks are used to signify which */
struct drivelock
{
	uint64 beg;
	uint64 end;
};

void addLocationLock(drivelocks* locks, uint64 beg, uint64 end)
{
  uint32 i;

	if(locks->_count <= locks->_current)
	{
		locks->_count += 0x400;
		locks->_locks = (struct drivelock*)reallocf(locks->_locks, sizeof(struct drivelock) * locks->_count);
  
    if(!locks->_locks)
      errx(1, "out of memory");
  }

	/* TODO: Implement a more efficient method here! */
	if(locks->_locks)
	{
		/* Go through and check for a current lock we can tag onto */
		for(i = 0; i < locks->_current; i++)
		{
			if(INTERSECTS(locks->_locks[i].beg, locks->_locks[i].end, beg, end))
      {
        locks->_locks[i].beg = min(locks->_locks[i].beg, beg);
        locks->_locks[i].end = max(locks->_locks[i].end, end);
        return;
      }
		}

		locks->_locks[locks->_current].beg = beg;
		locks->_locks[locks->_current].end = end;
		locks->_current++;
	}
}

bool checkLocationLock(drivelocks* locks, uint64 sec)
{
  uint32 i;

  if(locks->_locks)
  {
  	/* Go through and check for a lock */
	  for(i = 0; i < locks->_current; i++)
	  {
		  if(sec >= locks->_locks[i].beg && 
			  sec < locks->_locks[i].end)
		  {
			  sec = locks->_locks[i].end;
			  return true;
		  }
    }
	}

	return false;
}

#ifdef _DEBUG
void dumpLocationLocks(drivelocks* locks)
{
  uint32 i;

	for(i = 0; i < locks->_current; i++)
		printf("%u\t%u\n", (uint32)locks->_locks[i].beg, (uint32)locks->_locks[i].end);

	printf("\n");
}
#endif




/*
 * WARNING!! Not thread safe or very efficient for large
 * amounts of memory allocations
 */

#ifdef _DEBUG
const size_t kRefSig = 0x1F2F3F4F;

void* _refalloc_dbg(size_t sz)
{
	/* Allocate extra counter value before memory */
	size_t* mem = (size_t*)malloc(sz * sizeof(size_t) * 2);

	if(mem)
	{
		mem[0] = kRefSig;
		mem[1] = 1;
		return mem + 2;
	}

	return mem;
}
#endif 

void* _refalloc(size_t sz)
{
	/* Allocate extra counter value before memory */
	size_t* mem = (size_t*)malloc(sz * sizeof(size_t) * 1);

	if(mem)
	{
		mem[0] = 1;
		return mem + 1;
	}

	return mem;
}

#ifdef _DEBUG
void* _refadd_dbg(void* buf)
{
	if(buf)
	{
		/* Increment the counter value */
		size_t* mem = (size_t*)buf - 2;
		assert(mem[0] == kRefSig);
		mem[1]++;
	}

	return buf;
}
#endif

void* _refadd(void* buf)
{
	if(buf)
		/* Increment the counter value */
		((size_t*)buf)[-1]++;

	return buf;
}

#ifdef _DEBUG
void _refrelease_dbg(void* buf)
{
	if(buf)
	{
		/* Decrement the counter value */
		size_t* mem = (size_t*)buf - 2;
		assert(mem[0] == kRefSig);

		if(!--mem[1])
			free(mem);
	}
}
#endif

void _refrelease(void* buf)
{
	if(buf)
	{
		/* Decrement the counter value */
		size_t* mem = (size_t*)buf - 1;

		if(!--mem[0])
			free(mem);
	}
}



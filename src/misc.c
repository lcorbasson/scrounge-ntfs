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
		locks->_locks = (struct drivelock*)realloc(locks->_locks, sizeof(struct drivelock) * locks->_count);
	}

	/* TODO: Implement a more efficient method here! */
  /* TODO: What happens when the above memory allocation fails? */

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
	size_t* pMem = (size_t*)malloc(sz * sizeof(size_t) * 2);

	if(pMem)
	{
		pMem[0] = kRefSig;
		pMem[1] = 1;
		return pMem + 2;
	}

	return pMem;
}
#endif 

void* _refalloc(size_t sz)
{
	/* Allocate extra counter value before memory */
	size_t* pMem = (size_t*)malloc(sz * sizeof(size_t) * 1);

	if(pMem)
	{
		pMem[0] = 1;
		return pMem + 1;
	}

	return pMem;
}

#ifdef _DEBUG
void* _refadd_dbg(void* pBuff)
{
	if(pBuff)
	{
		/* Increment the counter value */
		size_t* pMem = (size_t*)pBuff - 2;
		assert(pMem[0] == kRefSig);
		pMem[1]++;
	}

	return pBuff;
}
#endif

void* _refadd(void* pBuff)
{
	if(pBuff)
		/* Increment the counter value */
		((size_t*)pBuff)[-1]++;

	return pBuff;
}

#ifdef _DEBUG
void _refrelease_dbg(void* pBuff)
{
	if(pBuff)
	{
		/* Decrement the counter value */
		size_t* pMem = (size_t*)pBuff - 2;
		assert(pMem[0] == kRefSig);

		if(!--pMem[1])
			free(pMem);
	}
}
#endif

void _refrelease(void* pBuff)
{
	if(pBuff)
	{
		/* Decrement the counter value */
		size_t* pMem = (size_t*)pBuff - 1;

		if(!--pMem[0])
			free(pMem);
	}
}

// 
// AUTHOR
// N. Nielsen
//
// VERSION
// 0.7
// 
// LICENSE
// This software is in the public domain.
//
// The software is provided "as is", without warranty of any kind,
// express or implied, including but not limited to the warranties
// of merchantability, fitness for a particular purpose, and
// noninfringement. In no event shall the author(s) be liable for any
// claim, damages, or other liability, whether in an action of
// contract, tort, or otherwise, arising from, out of, or in connection
// with the software or the use or other dealings in the software.
// 
// SUPPORT
// Send bug reports to: <nielsen@memberwebs.com>
//


#include "drive.h"
#include "memref.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>


PartitionInfo* CreatePartitionInfo()
{
	PartitionInfo* pInfo = (PartitionInfo*)refalloc(sizeof(PartitionInfo));
	if(pInfo)
		memset(pInfo, 0, sizeof(PartitionInfo));

	return pInfo;
}

void FreePartitionInfo(PartitionInfo* pInfo)
{
	if(pInfo)
	{
		if(pInfo->pLocks)
			free(pInfo->pLocks);

		refrelease(pInfo);
	}
}

// These locks are used to signify which 
struct DriveLock
{
	uint64 beg;
	uint64 end;
};


bool IntersectRange(uint64& b1, uint64& e1, uint64 b2, uint64 e2)
{
	// TODO: (later) This is stupid! There's a simple quick way
	// need to update

	if(b1 <= b2 && e1 > b2 && b1 < e2) // Overlaps the first portion
		e1 = e2;
	else if(b1 < e2 && e1 >= e2 && b2 < e1) // Overlaps second portion
		b1 = b2;
	else if(b1 > b2 && e1 < e2)		// Encompassed
		{	}
	else if (b1 == b2 && e1 == e2)	// Identical
		{	}
	else
		return false;

	return true;
}

void AddLocationLock(PartitionInfo* pInfo, uint64 beg, uint64 end)
{
	if(pInfo->cLocks <= pInfo->curLock)
	{
		pInfo->cLocks += 0x400;
		pInfo->pLocks = (DriveLock*)realloc(pInfo->pLocks, sizeof(DriveLock) * pInfo->cLocks);
	}

	// TODO: Implement a more efficient method here!

	if(pInfo->pLocks)
	{
		bool bFound = false;

		// Go through and check for a current lock we can tag onto
		for(uint32 i = 0; i < pInfo->curLock; i++)
		{
			if(IntersectRange(pInfo->pLocks[i].beg, pInfo->pLocks[i].end,
						      beg, end))
			{
				bFound = true;
			}
		}

		if(!bFound)
		{
			pInfo->pLocks[pInfo->curLock].beg = beg;
			pInfo->pLocks[pInfo->curLock].end = end;
			pInfo->curLock++;
		}
	}
}

bool CheckLocationLock(PartitionInfo* pInfo, uint64& sec)
{
	// Go through and check for a lock
	for(uint32 i = 0; i < pInfo->curLock; i++)
	{
		if(sec >= pInfo->pLocks[i].beg && 
			sec < pInfo->pLocks[i].end)
		{
			sec = pInfo->pLocks[i].end;
			return true;
		}
	}

	return false;
}

#ifdef _DEBUG
void DumpLocationLocks(PartitionInfo* pInfo)
{
	for(uint32 i = 0; i < pInfo->curLock; i++)
	{
		printf("%u\t%u\n", (uint32)pInfo->pLocks[i].beg, (uint32)pInfo->pLocks[i].end);
	}

	printf("\n");
}
#endif

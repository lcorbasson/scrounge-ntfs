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


#include "memref.h"
#include "malloc.h"
#include "assert.h"

// WARNING!! Not thread safe or very efficient for large
// amounts of memory allocations

const size_t kRefSig = 0x1F2F3F4F;

void* _refalloc_dbg(size_t sz)
{
	// Allocate extra counter value before memory
	size_t* pMem = (size_t*)malloc(sz * sizeof(size_t) * 2);

	if(pMem)
	{
		pMem[0] = kRefSig;
		pMem[1] = 1;
		return pMem + 2;
	}

	return pMem;
}

void* _refalloc(size_t sz)
{
	// Allocate extra counter value before memory
	size_t* pMem = (size_t*)malloc(sz * sizeof(size_t) * 1);

	if(pMem)
	{
		pMem[0] = 1;
		return pMem + 1;
	}

	return pMem;
}

void* _refadd_dbg(void* pBuff)
{
	if(pBuff)
	{
		// Increment the counter value
		size_t* pMem = (size_t*)pBuff - 2;
		assert(pMem[0] = kRefSig);
		pMem[1]++;
	}

	return pBuff;
}

void* _refadd(void* pBuff)
{
	if(pBuff)
		// Increment the counter value
		((size_t*)pBuff)[-1]++;

	return pBuff;
}

void _refrelease_dbg(void* pBuff)
{
	if(pBuff)
	{
		// Decrement the counter value
		size_t* pMem = (size_t*)pBuff - 2;
		assert(pMem[0] = kRefSig);

		if(!--pMem[1])
			free(pMem);
	}
}

void _refrelease(void* pBuff)
{
	if(pBuff)
	{
		// Decrement the counter value
		size_t* pMem = (size_t*)pBuff - 1;

		if(!--pMem[0])
			free(pMem);
	}
}
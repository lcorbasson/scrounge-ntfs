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


// ntfsx
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NTFSX__9363C7D2_D3CC_4D49_BEE0_27AD025670F2__INCLUDED_)
#define AFX_NTFSX__9363C7D2_D3CC_4D49_BEE0_27AD025670F2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ntfs.h"

class NTFS_DataRun
{
public:
	NTFS_DataRun(byte* pClus, byte* pDataRun);
	~NTFS_DataRun()
		{ refrelease(m_pMem); }

	bool First();
	bool Next();

	uint64 m_firstCluster;
	uint64 m_numClusters;
	bool m_bSparse; 

protected:
	byte* m_pMem;
	byte* m_pDataRun;
	byte* m_pCurPos;
};

class NTFS_Cluster
{
public:
	NTFS_Cluster()
		{ m_pCluster = NULL; };
	~NTFS_Cluster()
		{ Free(); }

	bool New(PartitionInfo* pInfo);
	bool Read(PartitionInfo* pInfo, uint64 begSector, HANDLE hIn);
	void Free()
		{ if(m_pCluster) refrelease(m_pCluster); }
	
	uint32 m_cbCluster;
	byte* m_pCluster;
};
	
class NTFS_Attribute
{
public:
	NTFS_Attribute(NTFS_Cluster& clus, NTFS_AttribHeader* pHeader);
	~NTFS_Attribute()
		{ refrelease(m_pMem); }

	NTFS_AttribHeader* GetHeader()
		{ return m_pHeader; }

	void* GetResidentData();
	uint32 GetResidentSize();

	NTFS_DataRun* GetDataRun();

	bool NextAttribute(uint32 attrType);


protected:
	NTFS_AttribHeader* m_pHeader;
	byte* m_pMem;
	uint32 m_cbMem;
};

class NTFS_Record : public NTFS_Cluster
{
public:
	NTFS_Record(PartitionInfo* pInfo);
	~NTFS_Record();

	bool Read(uint64 begSector, HANDLE hIn);
	NTFS_RecordHeader* GetHeader()
		{ return (NTFS_RecordHeader*)m_pCluster; }

	NTFS_Attribute* FindAttribute(uint32 attrType, HANDLE hIn);

protected:
	PartitionInfo* m_pInfo;
};

#endif // !defined(AFX_NTFSX__9363C7D2_D3CC_4D49_BEE0_27AD025670F2__INCLUDED_)

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

// ntfsx.cpp: implementation of the NTFS_Record class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ntfs.h"
#include "ntfsx.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool NTFS_Cluster::New(PartitionInfo* pInfo)
{
	Free();

	m_cbCluster = CLUSTER_SIZE(*pInfo);

	m_pCluster = (byte*)refalloc(m_cbCluster);
	return m_pCluster != NULL;
}

bool NTFS_Cluster::Read(PartitionInfo* pInfo, uint64 begSector, HANDLE hIn)
{
	if(!m_pCluster)
	{
		if(!New(pInfo))
			return false;
	}

	// Read	the mftRecord
	uint64 offRecord = SECTOR_TO_BYTES(begSector);
	LONG lHigh = HIGHDWORD(offRecord);
	if(SetFilePointer(hIn, LOWDWORD(offRecord), &lHigh, FILE_BEGIN) == -1
	   && GetLastError() != NO_ERROR)
	{
		Free();
		return false;
	}

	DWORD dwRead = 0;
	if(!ReadFile(hIn, m_pCluster, m_cbCluster, &dwRead, NULL) ||
		dwRead != m_cbCluster)
	{
		Free();
		::SetLastError(ERROR_READ_FAULT);
		return false;
	}

	::SetLastError(ERROR_SUCCESS);
	return true;
}

NTFS_Record::NTFS_Record(PartitionInfo* pInfo)
{
	m_pInfo	= (PartitionInfo*)refadd(pInfo);
}

NTFS_Record::~NTFS_Record()
{
	refrelease(m_pInfo);
}

bool NTFS_Record::Read(uint64 begSector, HANDLE hIn)
{
	if(!NTFS_Cluster::Read(m_pInfo, begSector, hIn))
		return false;

	// Check and validate this record
	NTFS_RecordHeader* pRecord = GetHeader();
	if(pRecord->magic != kNTFS_RecMagic || 
	   !NTFS_DoFixups(m_pCluster, m_cbCluster))
	{
		NTFS_Cluster::Free();
		::SetLastError(ERROR_NTFS_INVALID);
		return false;
	}

	return true;
}

NTFS_Attribute* NTFS_Record::FindAttribute(uint32 attrType, HANDLE hIn)
{
	NTFS_Attribute* pAttribute = NULL;

	// Make sure we have a valid record
	ASSERT(GetHeader());
	NTFS_AttribHeader* pHeader = NTFS_FindAttribute(GetHeader(), attrType, (m_pCluster + m_cbCluster));

	if(pHeader)
	{
		pAttribute = new NTFS_Attribute(*this, pHeader);
	}
	else
	{
		// Do attribute list thing here!
		pHeader = NTFS_FindAttribute(GetHeader(), kNTFS_ATTRIBUTE_LIST, (m_pCluster + m_cbCluster));

		// For now we only support Resident Attribute lists
		if(hIn && pHeader && !pHeader->bNonResident)
		{
			NTFS_AttribResident* pResident = (NTFS_AttribResident*)pHeader;
			NTFS_AttrListRecord* pAttr = (NTFS_AttrListRecord*)((byte*)pHeader + pResident->offAttribData);

			// Go through AttrList records looking for this attribute
			while((byte*)pAttr < (byte*)pHeader + pHeader->cbAttribute)
			{
				// Found it!
				if(pAttr->type == attrType)
				{
					// Read in appropriate cluster
					uint64 mftRecord = NTFS_RefToSector(*m_pInfo, pAttr->refAttrib);

					NTFS_Record recAttr(m_pInfo);
					if(recAttr.Read(mftRecord, hIn))
					{
						pAttribute = recAttr.FindAttribute(attrType, hIn);
						break;
					}

				}

 				pAttr = (NTFS_AttrListRecord*)((byte*)pAttr + pAttr->cbRecord);
			}
		}
	}

	return pAttribute;
}

bool NTFS_Attribute::NextAttribute(uint32 attrType)
{
	NTFS_AttribHeader* pHeader = NTFS_NextAttribute(GetHeader(), attrType, m_pMem + m_cbMem);	
	if(pHeader)
	{
		m_pHeader = pHeader;
		return true;
	}

	return false;
}

NTFS_Attribute::NTFS_Attribute(NTFS_Cluster& clus, NTFS_AttribHeader* pHeader)
{
	m_pHeader = pHeader;
	m_pMem = clus.m_pCluster;
	m_cbMem = clus.m_cbCluster;
	refadd(m_pMem);
}

void* NTFS_Attribute::GetResidentData()
{
	ASSERT(!m_pHeader->bNonResident);
	NTFS_AttribResident* pRes = (NTFS_AttribResident*)m_pHeader;
	return (byte*)m_pHeader + pRes->offAttribData;
}

uint32 NTFS_Attribute::GetResidentSize()
{
	ASSERT(!m_pHeader->bNonResident);
	NTFS_AttribResident* pRes = (NTFS_AttribResident*)m_pHeader;
	return pRes->cbAttribData;
}

NTFS_DataRun* NTFS_Attribute::GetDataRun()
{
	ASSERT(m_pHeader->bNonResident);
	NTFS_AttribNonResident* pNonRes = (NTFS_AttribNonResident*)m_pHeader;

	return new NTFS_DataRun(m_pMem, (byte*)m_pHeader + pNonRes->offDataRuns);
}

NTFS_DataRun::NTFS_DataRun(byte* pClus, byte* pDataRun)
{
	ASSERT(pDataRun);
	m_pMem = (byte*)refadd(pClus);
	m_pDataRun = pDataRun;
	m_pCurPos = NULL;
	m_firstCluster = m_numClusters = 0;
	m_bSparse = false;
}

bool NTFS_DataRun::First()
{
	m_pCurPos = m_pDataRun;
	m_firstCluster = m_numClusters = 0;
	m_bSparse = false;
	return Next();
}

bool NTFS_DataRun::Next()
{
	ASSERT(m_pCurPos);

	if(!*m_pCurPos)
		return false;

	byte cbLen = *m_pCurPos & 0x0F;
	byte cbOff = *m_pCurPos >> 4;

	// ASSUMPTION length and offset are less 64 bit numbers
	if(cbLen == 0 || cbLen > 8 || cbOff > 8)
		return false;
		
	ASSERT(cbLen <= 8);
	ASSERT(cbOff <= 8);

	m_pCurPos++;

	memset(&m_numClusters, 0, sizeof(uint64));

	memcpy(&m_numClusters, m_pCurPos, cbLen);
	m_pCurPos += cbLen;

	int64 offset;

	// Note that offset can be negative
	if(*(m_pCurPos + (cbOff - 1)) & 0x80)
		memset(&offset, ~0, sizeof(int64));
	else
		memset(&offset, 0, sizeof(int64));

	memcpy(&offset, m_pCurPos, cbOff);
	m_pCurPos += cbOff;

	if(offset == 0)
	{
		m_bSparse = true;
	}
	else
	{
		m_bSparse = false;
		m_firstCluster += offset;
	}

	return true;
}

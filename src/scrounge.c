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

// Scrounge.cpp 
//

#include "stdafx.h"
#include "ntfs.h"
#include "ntfsx.h"
#include "usuals.h"
#include "drive.h"
#include "locks.h"
#include "scrounge.h"

#define RET_ERROR(l) { ::SetLastError(l); bRet = TRUE; goto clean_up; }
#define PASS_ERROR() {bRet = TRUE; goto clean_up; }
#define RET_FATAL(l) { ::SetLastError(l); bRet = FALSE; goto clean_up; }
#define PASS_FATAL() {bRet = FALSE; goto clean_up; }


// ----------------------------------------------------------------------
// Process a potential MFT Record. For directories create the directory
// and for files write out the file.
// 
// Current Directory is the output directory
// hIn is an open drive handle 
// pInfo is partition info about hIn (needs to be a ref counted pointer)

BOOL ProcessMFTRecord(PartitionInfo* pInfo, uint64 mftRecord, HANDLE hIn)
{
	// Declare data that needs cleaning up first
	BOOL bRet = TRUE;					// Return value
	HANDLE hFile = NULL;				// Output file handle
	NTFS_Attribute* pAttribName = NULL;	// Filename Attribute
	NTFS_Attribute* pAttribData = NULL; // Data Attribute
	NTFS_DataRun* pDataRun = NULL;		// Data runs for nonresident data


	// Tracks whether or not we output a file
	bool bFile = false;

	{

		// Read the MFT record
		NTFS_Record record(pInfo);
		if(!record.Read(mftRecord, hIn))
			PASS_ERROR();

		NTFS_RecordHeader* pRecord = record.GetHeader();


		// Check if this record is in use
		if(!(pRecord->flags & kNTFS_RecFlagUse))
			RET_ERROR(ERROR_SUCCESS);


		// Info that we use later
		WCHAR fileName[MAX_PATH + 1];
		FILETIME ftCreated;
		FILETIME ftModified;
		FILETIME ftAccessed;
		DWORD fileAttrib = 0;
		uint64 mftParent = 0;
		byte* pResidentData = NULL;


		// Now get the name and info...
		pAttribName = record.FindAttribute(kNTFS_FILENAME, hIn);
		if(!pAttribName) RET_ERROR(ERROR_SUCCESS);


		byte nameSpace = kNTFS_NameSpacePOSIX;
		memset(fileName, 0, sizeof(fileName));

		do
		{
			// TODO ASSUMPTION: File name is always resident
			ASSERT(!pAttribName->GetHeader()->bNonResident);


			// Get out all the info we need
			NTFS_AttrFileName* pFileName = (NTFS_AttrFileName*)pAttribName->GetResidentData();
		
			// There can be multiple filenames with different namespaces
			// so choose the best one
			if(NTFS_IsBetterNameSpace(nameSpace, pFileName->nameSpace))
			{
				// Dates
				NTFS_MakeFileTime(pFileName->timeCreated, ftCreated);
				NTFS_MakeFileTime(pFileName->timeModified, ftModified);
				NTFS_MakeFileTime(pFileName->timeRead, ftAccessed);

				// File Name
				wcsncpy(fileName, (wchar_t*)(((byte*)pFileName) + sizeof(NTFS_AttrFileName)), pFileName->cFileName);
				fileName[pFileName->cFileName] = 0;

				// Attributes
				if(pFileName->flags & kNTFS_FileReadOnly)
					fileAttrib |= FILE_ATTRIBUTE_READONLY;
				if(pFileName->flags & kNTFS_FileHidden)
					fileAttrib |= FILE_ATTRIBUTE_HIDDEN;
				if(pFileName->flags & kNTFS_FileArchive)
					fileAttrib |= FILE_ATTRIBUTE_ARCHIVE;
				if(pFileName->flags &  kNTFS_FileSystem)
					fileAttrib |= FILE_ATTRIBUTE_SYSTEM;

				// Parent Directory
				mftParent = NTFS_RefToSector(*pInfo, pFileName->refParent);

				// Namespace
				nameSpace = pFileName->nameSpace;
			}
		}
		while(pAttribName->NextAttribute(kNTFS_FILENAME));


		// Check if we got a file name
		if(fileName[0] == 0)
			RET_ERROR(ERROR_NTFS_INVALID);



		// Check if it's the root
		// If so then bumm out cuz we don't want to have anything to do with it
		if(mftRecord == mftParent ||	// Root is it's own parent
		   !wcscmp(fileName, L".") ||   // Or is called '.'
		   !wcscmp(fileName, kNTFS_MFTName)) // Or it's the MFT
		    RET_ERROR(ERROR_SUCCESS);



		// Create Parent folders
		if(!ProcessMFTRecord(pInfo, mftParent, hIn))
			PASS_FATAL();



		// If it's a folder then create it
		if(pRecord->flags & kNTFS_RecFlagDir)
		{
			// Try to change to dir
			if(!SetCurrentDirectoryW(fileName))
			{
				// Otherwise create dir
				if(CreateDirectoryW(fileName, NULL))
				{
					// And set attributes
					SetFileAttributesW(fileName, fileAttrib);
					SetCurrentDirectoryW(fileName);
				}
			}

			wprintf(L"\\%s", fileName);
		}


		// Otherwise write the file data
		else
		{
			// Write to the File
			hFile = CreateFileW(fileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);

			uint16 nRename = 0;	
			wchar_t fileName2[MAX_PATH + 1];
			wcscpy(fileName2, fileName);

			// For duplicate files we add .x to the file name where x is a number
			while(hFile == INVALID_HANDLE_VALUE && ::GetLastError() == ERROR_FILE_EXISTS
				  && nRename < 0x1000000)
			{
				wcscpy(fileName, fileName2);
				wcscat(fileName, L".");
				uint16 len = wcslen(fileName);
				
				// Make sure we don't have a buffer overflow
				if(len > MAX_PATH - 5)
					break;

				_itow(nRename, fileName + len, 10);
				nRename++;

				hFile = CreateFileW(fileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
			}
			
			wprintf(L"\\%s", fileName);
			bFile = true;

			// Check if successful after all that
			if(hFile == INVALID_HANDLE_VALUE)
				PASS_FATAL();

			
			DWORD dwDone = 0;
			uint64 fileSize = 0;


			// Get the File's data
			pAttribData = record.FindAttribute(kNTFS_DATA, hIn);
			if(!pAttribData) RET_ERROR(ERROR_NTFS_INVALID);

			
			// For Resident data just write it out
			if(!pAttribData->GetHeader()->bNonResident)
			{
				if(!WriteFile(hFile, pAttribData->GetResidentData(), pAttribData->GetResidentSize(), &dwDone, NULL))
					PASS_FATAL();
			}

			// For Nonresident data a bit more involved
			else
			{
				pDataRun = pAttribData->GetDataRun();
				ASSERT(pDataRun != NULL);

				NTFS_AttribNonResident* pNonRes = (NTFS_AttribNonResident*)pAttribData->GetHeader();
				fileSize = pNonRes->cbAttribData;

				// Allocate a cluster for reading and writing
				NTFS_Cluster clus;
				if(!clus.New(pInfo))
					RET_FATAL(ERROR_NOT_ENOUGH_MEMORY);
		

				// Now loop through the data run
				if(pDataRun->First())
				{
					do
					{
						// If it's a sparse cluster then just write zeros
						if(pDataRun->m_bSparse)
						{
							memset(clus.m_pCluster, 0, clus.m_cbCluster);

							for(uint32 i = 0; i < pDataRun->m_numClusters && fileSize; i++)
							{
								DWORD dwToWrite = clus.m_cbCluster;
								if(!HIGHDWORD(fileSize) && dwToWrite > (DWORD)fileSize)
									dwToWrite = (DWORD)fileSize;

								if(!WriteFile(hFile, clus.m_pCluster, dwToWrite, &dwDone, NULL))
									PASS_FATAL();

								fileSize -= dwToWrite;
							}
						}

						// Not sparse
						else
						{
							// Add a lock on those clusters so we don't have to scrounge'm later
							AddLocationLock(pInfo, CLUSTER_TO_SECTOR(*pInfo, pDataRun->m_firstCluster), 
												   CLUSTER_TO_SECTOR(*pInfo, pDataRun->m_firstCluster + pDataRun->m_numClusters));

							// Read and write clusters out
							for(uint32 i = 0; i < pDataRun->m_numClusters && fileSize; i++)
							{
								DWORD dwToWrite = min(clus.m_cbCluster, (DWORD)fileSize);
								uint64 sector = CLUSTER_TO_SECTOR(*pInfo, (pDataRun->m_firstCluster + i));
							
								if(!clus.Read(pInfo, sector, hIn))
									PASS_ERROR();

								if(!WriteFile(hFile, clus.m_pCluster, dwToWrite, &dwDone, NULL))
									PASS_FATAL();

								fileSize -= dwToWrite;
							}
						}
					} 
					while(pDataRun->Next());
				}
			}

			// TODO: More intelligence needed here
			if(fileSize != 0)
				printf(" (Entire file not written)");

			SetFileTime(hFile, &ftCreated, &ftAccessed, &ftModified);
				
			CloseHandle(hFile);
			hFile = NULL;

			SetFileAttributesW(fileName, fileAttrib);
		}
	}

	bRet = TRUE;
	::SetLastError(ERROR_SUCCESS);

clean_up:
	if(hFile && hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if(pAttribName)
		delete pAttribName;
	if(pAttribData)
		delete pAttribData;
	if(pDataRun)
		delete pDataRun;

	if(bFile)
	{
		if(::GetLastError() != ERROR_SUCCESS)
		{
			printf(" (");
			PrintLastError();
			fputc(')', stdout);
		}
	}


	return bRet; 
}


// ----------------------------------------------------------------------
//  Helper function to print out errors

void PrintLastError()
{
	DWORD dwErr = ::GetLastError();
	switch(dwErr)
	{
	case ERROR_NTFS_INVALID:
		printf("Invalid NTFS data structure");
		break;

	case ERROR_NTFS_NOTIMPLEMENT:
		printf("NTFS feature not implemented");
		break;

	default:
		{
		    LPVOID lpMsgBuf;

		    DWORD dwRet = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
										  FORMAT_MESSAGE_MAX_WIDTH_MASK,
										  NULL,
										  dwErr,
										  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
										  (LPTSTR) &lpMsgBuf,
										  0,
										  NULL); 

			if(dwRet && lpMsgBuf)
			{
				// Remove return
				((LPTSTR)lpMsgBuf)[dwRet - 2] = 0;

				wprintf((LPTSTR)lpMsgBuf);

				// Free the buffer.
				::LocalFree(lpMsgBuf);
			}
		}
	};
}


// ----------------------------------------------------------------------
//  Scrounge the partition for MFT Records and hand off to 
//  ProcessMFTRecord for processing

BOOL ScroungeMFTRecords(PartitionInfo* pInfo, HANDLE hIn)
{
	byte buffSec[kSectorSize];
	DWORD dwDummy = 0;

	uint64 numRecords = 0;

	// Save current directory away
	TCHAR curDir[MAX_PATH + 1];
	GetCurrentDirectory(MAX_PATH, curDir);

	// Loop through sectors
	for(uint64 sec = pInfo->firstSector; sec < pInfo->lastSector; sec++)
	{
		// See if the current sector has already been read
		if(CheckLocationLock(pInfo, sec))
		{
			// TODO: check this
			sec--;
			continue;
		}

		// Read	the mftRecord
		uint64 offRecord = SECTOR_TO_BYTES(sec);
		LONG lHigh = HIGHDWORD(offRecord);
		if(SetFilePointer(hIn, LOWDWORD(offRecord), &lHigh, FILE_BEGIN) == -1
		   && GetLastError() != NO_ERROR) 
			return FALSE;

		if(!ReadFile(hIn, buffSec, kSectorSize, &dwDummy, NULL))
			return FALSE;

		// Check beginning of sector for the magic signature
		if(!memcmp(&kNTFS_RecMagic, &buffSec, sizeof(kNTFS_RecMagic)))
		{
			// Move to right output directory
			SetCurrentDirectory(curDir);

			// Then process it
			BOOL bRet = ProcessMFTRecord(pInfo, sec, hIn);

			fputc('\n', stdout);

			if(!bRet)
				return FALSE;

		}
	}

	return TRUE;
}


// ----------------------------------------------------------------------
//  Output strings

const char kPrintHeader[]		= "Scrounge (NTFS) Version 0.7\n\n";

const char kPrintData[]		= "\
    Start Sector    End Sector      Cluster Size    MFT Offset    \n\
==================================================================\n\
";

const char kPrintDrive[]		= "\nDrive: %u\n";
const char kPrintDriveInfo[]	= "    %-15u %-15u ";
const char kPrintNTFSInfo[]		= "%-15u %-15u";

const char kPrintHelp[]         = "\
Recovers an NTFS partition with a corrupted MFT.                     \n\
                                                                     \n\
Usage: scrounge drive start end cluster mft [outdir]                 \n\
                                                                     \n\
  drive:     Physical drive number.                                  \n\
  start:     First sector of partition.                              \n\
  end:       Last sector of partition.                               \n\
  cluster:   Cluster size for the partition (in sectors).            \n\
  mft:       Offset from beginning of partition to MFT (in sectors). \n\
  outdir:    Output directory (optional).                            \n\
                                                                     \n\
";


// ----------------------------------------------------------------------
//  Info functions

int PrintNTFSInfo(HANDLE hDrive, uint64 tblSector)
{
	byte sector[kSectorSize];

	uint64 pos = SECTOR_TO_BYTES(tblSector);
	LONG lHigh = HIGHDWORD(pos);
	if(SetFilePointer(hDrive, LOWDWORD(pos), &lHigh, FILE_BEGIN) == -1
	   && GetLastError() != NO_ERROR)
		return 1;

	DWORD dwRead = 0;
	if(!ReadFile(hDrive, sector, kSectorSize, &dwRead, NULL))
		return 1;

	NTFS_BootSector* pBoot = (NTFS_BootSector*)sector;
	if(!memcmp(pBoot->sysId, kNTFS_SysId, sizeof(pBoot->sysId)))
		printf(kPrintNTFSInfo, pBoot->secPerClus, pBoot->offMFT * pBoot->secPerClus);

	wprintf(L"\n");
	return 0;
}


int PrintPartitionInfo(HANDLE hDrive, uint64 tblSector)
{
	ASSERT(sizeof(Drive_MBR) == kSectorSize);
	Drive_MBR mbr;

	uint64 pos = SECTOR_TO_BYTES(tblSector);
	LONG lHigh = HIGHDWORD(pos);
	if(SetFilePointer(hDrive, LOWDWORD(pos), &lHigh, FILE_BEGIN) == -1
	   && GetLastError() != NO_ERROR)
		return 1;

	DWORD dwRead = 0;
	if(!ReadFile(hDrive, &mbr, sizeof(Drive_MBR), &dwRead, NULL))
		return 1;

	if(mbr.sig == kMBR_Sig)
	{
		for(int i = 0; i < 4; i++)
		{
			if(mbr.partitions[i].system == kPartition_Extended ||
			   mbr.partitions[i].system == kPartition_ExtendedLBA)
			{
				PrintPartitionInfo(hDrive, tblSector + mbr.partitions[i].startsec);
			}
			else if(!mbr.partitions[i].system == kPartition_Invalid)
			{
				printf(kPrintDriveInfo, (uint32)tblSector + mbr.partitions[i].startsec, (uint32)tblSector + mbr.partitions[i].endsec);
				PrintNTFSInfo(hDrive, tblSector + (uint64)mbr.partitions[i].startsec);
			}
		}
	}

	return 0;
}

const WCHAR kDriveName[] = L"\\\\.\\PhysicalDrive%d";

int PrintData()
{
	printf(kPrintHeader);
	printf(kPrintData);

	WCHAR driveName[MAX_PATH];

	// LIMIT: 256 Drives
	for(int i = 0; i < 0x100; i++)
	{
		wsprintf(driveName, kDriveName, i);

		HANDLE hDrive = CreateFile(driveName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if(hDrive != INVALID_HANDLE_VALUE)
		{
			printf(kPrintDrive, i);

			PrintPartitionInfo(hDrive, 0);

			CloseHandle(hDrive);
		}
	}

	return 2;
}


// ----------------------------------------------------------------------
//  Main Program

int main(int argc, char* argv[])
{
	int curArg = 1;

	if(argc < 2)
		return PrintData();


	// Check for flags
	if(*(argv[curArg]) == '-' || *(argv[curArg]) == '/' )
	{
		char* arg = argv[curArg];
		arg++;

		while(*arg != '\0')
		{
			switch(tolower(*arg))
			{

			// Help
			case 'h':
				printf(kPrintHeader);
				printf(kPrintHelp);
				return 2;

			default:
				printf("scrounge: invalid option '%c'\n", *arg);
				return PrintData();
			}

			arg++;
		}

		curArg++;
	}

	PartitionInfo* pInfo = CreatePartitionInfo();
	if(!pInfo)
	{
		printf("scrounge: Out of Memory.\n");
		return 1;
	}

	if(curArg + 5 > argc)
	{
		printf("scrounge: invalid option(s).\n");
		return 2;
	}

	// Next param should be the drive
	byte driveNum = atoi(argv[curArg++]);

	// Followed by the partition info
	pInfo->firstSector = atoi(argv[curArg++]);
	pInfo->lastSector = atoi(argv[curArg++]);
	pInfo->clusterSize = atoi(argv[curArg++]);
	pInfo->offMFT = atoi(argv[curArg++]);

	if(pInfo->firstSector == 0 ||
	   pInfo->lastSector == 0 ||
	   pInfo->clusterSize == 0 ||
	   pInfo->offMFT == 0)
	{
		printf("scrounge: invalid option(s).\n");
		return 2;
	}

//	pInfo->clusterSize = 8;
//	pInfo->firstSector = 20482938/*128*/;
//	pInfo->lastSector = 80019765/*15358077*/;
//	pInfo->offMFT = 32;

	if(curArg <= argc)
		SetCurrentDirectoryA(argv[curArg++]);

	WCHAR driveName[MAX_PATH];

	wsprintf(driveName, kDriveName, driveNum);

	HANDLE hDrive = CreateFile(driveName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(hDrive == INVALID_HANDLE_VALUE)
	{
		printf("scrounge: Can't open drive %d.\n", driveNum);
		return 2;
	}

	if(!ScroungeMFTRecords(pInfo, hDrive))
	{
		printf("scrounge: ");
		PrintLastError();
		fputc('\n', stdout);
		return 2;
	}

	FreePartitionInfo(pInfo);
	
	return 0;
}
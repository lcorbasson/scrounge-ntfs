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

#ifndef __SCROUNGE_H__
#define __SCROUNGE_H__

#define RET_ERROR(l) { ::SetLastError(l); bRet = TRUE; goto clean_up; }
#define PASS_ERROR() {bRet = TRUE; goto clean_up; }
#define RET_FATAL(l) { ::SetLastError(l); bRet = FALSE; goto clean_up; }
#define PASS_FATAL() {bRet = FALSE; goto clean_up; }

BOOL ProcessMFTRecord(PartitionInfo* pInfo, uint64 sector, NTFS_MFTMap* map, HANDLE hIn);
BOOL ScroungeMFTRecords(PartitionInfo* pInfo, HANDLE hIn);
BOOL ScroungeRawRecords(PartitionInfo* pInfo, HANDLE hIn);
void PrintLastError();



#endif //__SCROUNGE_H__
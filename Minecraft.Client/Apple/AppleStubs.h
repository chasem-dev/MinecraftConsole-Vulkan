#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cfloat>
#include <climits>
#include <cwchar>
#include <string>

#include "../Orbis/OrbisExtras/OrbisTypes.h"

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

// ---------------------------------------------------------------------------
// Macros that the Windows headers normally supply
// ---------------------------------------------------------------------------
#ifndef WINAPI
#define WINAPI
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef CDECL
#define CDECL
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif

// ---------------------------------------------------------------------------
// HRESULT helpers
// ---------------------------------------------------------------------------
#ifndef S_OK
#define S_OK    ((HRESULT)0L)
#endif
#ifndef S_FALSE
#define S_FALSE ((HRESULT)1L)
#endif
#ifndef E_FAIL
#define E_FAIL  ((HRESULT)0x80004005L)
#endif
#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#endif
#ifndef E_INVALIDARG
#define E_INVALIDARG  ((HRESULT)0x80070057L)
#endif
#ifndef E_NOTIMPL
#define E_NOTIMPL     ((HRESULT)0x80004001L)
#endif
#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#define FAILED(hr)    (((HRESULT)(hr)) < 0)
#endif

// ---------------------------------------------------------------------------
// Windows error codes
// ---------------------------------------------------------------------------
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS        0L
#endif
#ifndef ERROR_IO_PENDING
#define ERROR_IO_PENDING     997L
#endif
#ifndef ERROR_CANCELLED
#define ERROR_CANCELLED      1223L
#endif

// ---------------------------------------------------------------------------
// File I/O constants
// ---------------------------------------------------------------------------
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif
#ifndef GENERIC_READ
#define GENERIC_READ   0x80000000UL
#endif
#ifndef GENERIC_WRITE
#define GENERIC_WRITE  0x40000000UL
#endif
#ifndef FILE_SHARE_READ
#define FILE_SHARE_READ  0x00000001
#endif
#ifndef FILE_SHARE_WRITE
#define FILE_SHARE_WRITE 0x00000002
#endif
#ifndef OPEN_EXISTING
#define OPEN_EXISTING  3
#endif
#ifndef CREATE_ALWAYS
#define CREATE_ALWAYS  2
#endif
#ifndef OPEN_ALWAYS
#define OPEN_ALWAYS    4
#endif
#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#endif
#ifndef FILE_FLAG_SEQUENTIAL_SCAN
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000
#endif
#ifndef FILE_FLAG_RANDOM_ACCESS
#define FILE_FLAG_RANDOM_ACCESS   0x10000000
#endif
#ifndef FILE_BEGIN
#define FILE_BEGIN   0
#endif
#ifndef FILE_CURRENT
#define FILE_CURRENT 1
#endif
#ifndef FILE_END
#define FILE_END     2
#endif

// ---------------------------------------------------------------------------
// Wait / thread constants
// ---------------------------------------------------------------------------
#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0  0x00000000L
#endif
#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT   0x00000102L
#endif
#ifndef WAIT_ABANDONED
#define WAIT_ABANDONED 0x00000080L
#endif
#ifndef WAIT_FAILED
#define WAIT_FAILED    0xFFFFFFFFL
#endif
#ifndef STILL_ACTIVE
#define STILL_ACTIVE   259
#endif
#ifndef THREAD_PRIORITY_HIGHEST
#define THREAD_PRIORITY_HIGHEST 2
#endif
#ifndef THREAD_PRIORITY_NORMAL
#define THREAD_PRIORITY_NORMAL  0
#endif
#ifndef THREAD_PRIORITY_LOWEST
#define THREAD_PRIORITY_LOWEST  (-2)
#endif

// ---------------------------------------------------------------------------
// Virtual-key codes used in UI code
// ---------------------------------------------------------------------------
#ifndef VK_ESCAPE
#define VK_ESCAPE 0x1B
#endif
#ifndef VK_RETURN
#define VK_RETURN 0x0D
#endif

// ---------------------------------------------------------------------------
// Structures
// ---------------------------------------------------------------------------
typedef struct _RECT
{
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *PRECT;

// Prevent 4J_Render.h from redefining basic types and D3D11_RECT
#define _4J_RENDER_BASIC_TYPES

struct D3D11_RECT
{
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
};

typedef struct _SYSTEMTIME
{
  WORD wYear;
  WORD wMonth;
  WORD wDayOfWeek;
  WORD wDay;
  WORD wHour;
  WORD wMinute;
  WORD wSecond;
  WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef DWORD (*PTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

struct CRITICAL_SECTION
{
  pthread_mutex_t mutex;
};

typedef CRITICAL_SECTION *PCRITICAL_SECTION;
typedef CRITICAL_SECTION *LPCRITICAL_SECTION;

// ---------------------------------------------------------------------------
// WIN32_FIND_DATA (simplified for POSIX)
// ---------------------------------------------------------------------------
#ifndef _WIN32_FIND_DATA_DEFINED
#define _WIN32_FIND_DATA_DEFINED
typedef struct _WIN32_FIND_DATAA
{
  DWORD dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD nFileSizeHigh;
  DWORD nFileSizeLow;
  DWORD dwReserved0;
  DWORD dwReserved1;
  CHAR cFileName[MAX_PATH];
  CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;
typedef WIN32_FIND_DATAA WIN32_FIND_DATA;
typedef LPWIN32_FIND_DATAA LPWIN32_FIND_DATA;
#endif

// ---------------------------------------------------------------------------
// File attribute constants for WIN32_FIND_DATA
// ---------------------------------------------------------------------------
#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#endif
#ifndef FILE_ATTRIBUTE_READONLY
#define FILE_ATTRIBUTE_READONLY  0x00000001
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

// ---------------------------------------------------------------------------
// DirectXMath stub types (used by rendering code)
// ---------------------------------------------------------------------------
struct XMFLOAT4
{
  float x, y, z, w;
  XMFLOAT4() : x(0), y(0), z(0), w(0) {}
  XMFLOAT4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

struct XMFLOAT4X4
{
  float m[4][4];
};

typedef XMFLOAT4 XMVECTOR;

struct XMMATRIX
{
  XMVECTOR r[4];
};

// ---------------------------------------------------------------------------
// Critical section functions
// ---------------------------------------------------------------------------
VOID InitializeCriticalSection(PCRITICAL_SECTION criticalSection);
VOID InitializeCriticalSectionAndSpinCount(PCRITICAL_SECTION criticalSection, ULONG spinCount);
VOID DeleteCriticalSection(PCRITICAL_SECTION criticalSection);
VOID EnterCriticalSection(PCRITICAL_SECTION criticalSection);
VOID LeaveCriticalSection(PCRITICAL_SECTION criticalSection);
ULONG TryEnterCriticalSection(PCRITICAL_SECTION criticalSection);

// ---------------------------------------------------------------------------
// Thread Local Storage
// ---------------------------------------------------------------------------
DWORD TlsAlloc(VOID);
LPVOID TlsGetValue(DWORD dwTlsIndex);
BOOL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue);

// ---------------------------------------------------------------------------
// Threading
// ---------------------------------------------------------------------------
HANDLE CreateThread(
  LPVOID lpThreadAttributes,
  SIZE_T dwStackSize,
  LPTHREAD_START_ROUTINE lpStartAddress,
  LPVOID lpParameter,
  DWORD dwCreationFlags,
  LPDWORD lpThreadId);

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
BOOL CloseHandle(HANDLE hObject);
BOOL GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode);

HANDLE CreateEvent(LPVOID lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
BOOL SetEvent(HANDLE hEvent);
BOOL ResetEvent(HANDLE hEvent);

// ---------------------------------------------------------------------------
// System info
// ---------------------------------------------------------------------------
VOID Sleep(DWORD dwMilliseconds);
DWORD GetCurrentThreadId(VOID);
DWORD GetCurrentProcessId(VOID);
VOID GetSystemTime(LPSYSTEMTIME lpSystemTime);
VOID GetLocalTime(LPSYSTEMTIME lpSystemTime);
HMODULE GetModuleHandle(LPCSTR lpModuleName);
DWORD GetTickCount(VOID);
BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
DWORD GetLastError(VOID);
VOID SetLastError(DWORD dwErrCode);
BOOL SystemTimeToFileTime(const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime);
BOOL FileTimeToSystemTime(const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime);

// ---------------------------------------------------------------------------
// File I/O
// ---------------------------------------------------------------------------
HANDLE CreateFileA(
  LPCSTR lpFileName,
  DWORD dwDesiredAccess,
  DWORD dwShareMode,
  LPVOID lpSecurityAttributes,
  DWORD dwCreationDisposition,
  DWORD dwFlagsAndAttributes,
  HANDLE hTemplateFile);

#ifndef CreateFile
#define CreateFile CreateFileA
#endif

BOOL ReadFile(
  HANDLE hFile,
  LPVOID lpBuffer,
  DWORD nNumberOfBytesToRead,
  LPDWORD lpNumberOfBytesRead,
  LPVOID lpOverlapped);

BOOL WriteFile(
  HANDLE hFile,
  LPCVOID lpBuffer,
  DWORD nNumberOfBytesToWrite,
  LPDWORD lpNumberOfBytesWritten,
  LPVOID lpOverlapped);

DWORD SetFilePointer(
  HANDLE hFile,
  LONG lDistanceToMove,
  LONG *lpDistanceToMoveHigh,
  DWORD dwMoveMethod);

DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
BOOL DeleteFileA(LPCSTR lpFileName);
#ifndef DeleteFile
#define DeleteFile DeleteFileA
#endif
BOOL CreateDirectoryA(LPCSTR lpPathName, LPVOID lpSecurityAttributes);
#ifndef CreateDirectory
#define CreateDirectory CreateDirectoryA
#endif
BOOL RemoveDirectoryA(LPCSTR lpPathName);
#ifndef RemoveDirectory
#define RemoveDirectory RemoveDirectoryA
#endif
DWORD GetFileAttributesA(LPCSTR lpFileName);
#ifndef GetFileAttributes
#define GetFileAttributes GetFileAttributesA
#endif
BOOL CopyFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);
#ifndef CopyFile
#define CopyFile CopyFileA
#endif
BOOL MoveFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName);
#ifndef MoveFile
#define MoveFile MoveFileA
#endif
DWORD GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer);
#ifndef GetCurrentDirectory
#define GetCurrentDirectory GetCurrentDirectoryA
#endif
DWORD GetTempPathA(DWORD nBufferLength, LPSTR lpBuffer);
#ifndef GetTempPath
#define GetTempPath GetTempPathA
#endif
DWORD GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);
#ifndef GetModuleFileName
#define GetModuleFileName GetModuleFileNameA
#endif

HANDLE FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
#ifndef FindFirstFile
#define FindFirstFile FindFirstFileA
#endif
BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
#ifndef FindNextFile
#define FindNextFile FindNextFileA
#endif
BOOL FindClose(HANDLE hFindFile);

// ---------------------------------------------------------------------------
// Interlocked
// ---------------------------------------------------------------------------
LONG InterlockedCompareExchangeRelease(LONG volatile *destination, LONG exchange, LONG comparand);
LONG64 InterlockedCompareExchangeRelease64(LONG64 volatile *destination, LONG64 exchange, LONG64 comparand);
LONG InterlockedIncrement(LONG volatile *addend);
LONG InterlockedDecrement(LONG volatile *addend);
LONG InterlockedExchange(LONG volatile *target, LONG value);

// ---------------------------------------------------------------------------
// Debug output
// ---------------------------------------------------------------------------
void OutputDebugStringA(LPCSTR outputString);
void OutputDebugStringW(LPCWSTR outputString);
#ifndef OutputDebugString
#define OutputDebugString OutputDebugStringA
#endif
void __debugbreak();
VOID DebugBreak(VOID);

// ---------------------------------------------------------------------------
// String functions
// ---------------------------------------------------------------------------
int _wcsicmp(const wchar_t *dst, const wchar_t *src);
int _wcsnicmp(const wchar_t *s1, const wchar_t *s2, size_t n);
int _stricmp(const char *s1, const char *s2);
int _strnicmp(const char *s1, const char *s2, size_t n);
wchar_t *_wcsdup(const wchar_t *s);
unsigned long long _wcstoui64(const wchar_t *nptr, wchar_t **endptr, int base);

// Safe string functions (map to standard equivalents)
#ifndef sprintf_s
#define sprintf_s snprintf
#endif

// Variadic sprintf_s with explicit size
inline int sprintf_s_n(char *buf, size_t bufSize, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
inline int sprintf_s_n(char *buf, size_t bufSize, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int result = vsnprintf(buf, bufSize, fmt, args);
  va_end(args);
  return result;
}

#ifndef swprintf_s
#define swprintf_s swprintf
#endif

#ifndef swscanf_s
#define swscanf_s swscanf
#endif

#ifndef sscanf_s
#define sscanf_s sscanf
#endif

#ifndef _snprintf
#define _snprintf snprintf
#endif

#ifndef _snwprintf
#define _snwprintf swprintf
#endif

#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif

#ifndef _vsnwprintf
#define _vsnwprintf vswprintf
#endif

#ifndef strcpy_s
inline void strcpy_s(char *dst, size_t dstSize, const char *src)
{
  if (dst && dstSize > 0)
  {
    strncpy(dst, src ? src : "", dstSize - 1);
    dst[dstSize - 1] = '\0';
  }
}
#endif

#ifndef strncpy_s
inline void strncpy_s(char *dst, size_t dstSize, const char *src, size_t count)
{
  if (dst && dstSize > 0)
  {
    size_t toCopy = (count < dstSize - 1) ? count : dstSize - 1;
    if (src) strncpy(dst, src, toCopy);
    dst[toCopy] = '\0';
  }
}
#endif

#ifndef wcsncpy_s
inline void wcsncpy_s(wchar_t *dst, size_t dstSize, const wchar_t *src, size_t count)
{
  if (dst && dstSize > 0)
  {
    size_t toCopy = (count < dstSize - 1) ? count : dstSize - 1;
    if (src) wcsncpy(dst, src, toCopy);
    dst[toCopy] = L'\0';
  }
}
// 3-arg overload matching MSVC template version: treats count as dest buffer size
inline void wcsncpy_s(wchar_t *dst, const wchar_t *src, size_t count)
{
  wcsncpy_s(dst, count, src, count > 0 ? count - 1 : 0);
}
#endif

#ifndef wcscpy_s
inline void wcscpy_s(wchar_t *dst, size_t dstSize, const wchar_t *src)
{
  wcsncpy_s(dst, dstSize, src, dstSize);
}
#endif

#ifndef wcscat_s
inline void wcscat_s(wchar_t *dst, size_t dstSize, const wchar_t *src)
{
  if (dst && src && dstSize > 0)
  {
    size_t dstLen = wcslen(dst);
    if (dstLen < dstSize - 1)
    {
      wcsncat(dst, src, dstSize - dstLen - 1);
    }
  }
}
#endif

#ifndef _countof
#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// ---------------------------------------------------------------------------
// Memory allocation constants
// ---------------------------------------------------------------------------
#ifndef MEM_COMMIT
#define MEM_COMMIT      0x1000
#endif
#ifndef MEM_RESERVE
#define MEM_RESERVE     0x2000
#endif
#ifndef MEM_DECOMMIT
#define MEM_DECOMMIT    0x4000
#endif
#ifndef MEM_RELEASE
#define MEM_RELEASE     0x8000
#endif

inline void *VirtualAlloc(void *addr, size_t size, DWORD type, DWORD protect)
{
  (void)type; (void)protect;
  if (addr) return addr;
  return calloc(1, size);
}
inline BOOL VirtualFree(void *addr, size_t, DWORD)
{
  free(addr);
  return TRUE;
}

// ---------------------------------------------------------------------------
// GetFileAttributesEx stub
// ---------------------------------------------------------------------------
#ifndef GetFileExInfoStandard
#define GetFileExInfoStandard 0
#endif
typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
  DWORD dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD nFileSizeHigh;
  DWORD nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA;
inline BOOL GetFileAttributesExA(LPCSTR lpFileName, int fInfoLevelId, LPVOID lpFileInformation)
{
  (void)fInfoLevelId;
  struct stat st;
  if (stat(lpFileName, &st) != 0) return FALSE;
  WIN32_FILE_ATTRIBUTE_DATA *data = (WIN32_FILE_ATTRIBUTE_DATA *)lpFileInformation;
  if (data) {
    data->dwFileAttributes = S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    data->nFileSizeHigh = (DWORD)((st.st_size >> 32) & 0xFFFFFFFF);
    data->nFileSizeLow = (DWORD)(st.st_size & 0xFFFFFFFF);
  }
  return TRUE;
}
#ifndef GetFileAttributesEx
#define GetFileAttributesEx GetFileAttributesExA
#endif

// ---------------------------------------------------------------------------
// Thread priority
// ---------------------------------------------------------------------------
inline BOOL SetThreadPriority(HANDLE, int) { return TRUE; }

// ---------------------------------------------------------------------------
// _itow stub
// ---------------------------------------------------------------------------
inline wchar_t *_itow(int value, wchar_t *buffer, int radix)
{
  if (radix == 10) swprintf(buffer, 32, L"%d", value);
  else if (radix == 16) swprintf(buffer, 32, L"%x", value);
  else swprintf(buffer, 32, L"%d", value);
  return buffer;
}


// ---------------------------------------------------------------------------
// Misc / memory
// ---------------------------------------------------------------------------
#ifndef ZeroMemory
#define ZeroMemory(Destination, Length) memset((Destination), 0, (Length))
#endif

#ifndef RtlZeroMemory
#define RtlZeroMemory(Destination, Length) memset((Destination), 0, (Length))
#endif

#ifndef CopyMemory
#define CopyMemory(Destination, Source, Length) memcpy((Destination), (Source), (Length))
#endif

#ifndef FillMemory
#define FillMemory(Destination, Length, Fill) memset((Destination), (Fill), (Length))
#endif

#ifndef MoveMemory
#define MoveMemory(Destination, Source, Length) memmove((Destination), (Source), (Length))
#endif

// ---------------------------------------------------------------------------
// MessageBox stub
// ---------------------------------------------------------------------------
#ifndef MB_OK
#define MB_OK 0x00000000L
#endif
#ifndef MB_ICONERROR
#define MB_ICONERROR 0x00000010L
#endif
#ifndef IDOK
#define IDOK 1
#endif

inline int MessageBoxA(void *, const char *text, const char *caption, unsigned int)
{
  if (text) fprintf(stderr, "MessageBox [%s]: %s\n", caption ? caption : "", text);
  return IDOK;
}
#ifndef MessageBox
#define MessageBox MessageBoxA
#endif

// ---------------------------------------------------------------------------
// Winsock / networking compatibility
// ---------------------------------------------------------------------------
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

typedef int SOCKET;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR   (-1)
#endif
#define closesocket(s) close(s)
#define ioctlsocket(s, cmd, argp) ioctl(s, cmd, argp)

#ifndef SD_BOTH
#define SD_BOTH SHUT_RDWR
#endif

// WSA stubs
#ifndef WSADATA_DEFINED
#define WSADATA_DEFINED
struct WSADATA
{
  WORD wVersion;
  WORD wHighVersion;
  char szDescription[257];
  char szSystemStatus[129];
};
#endif

inline int WSAStartup(WORD, WSADATA *) { return 0; }
inline int WSACleanup() { return 0; }
inline int WSAGetLastError() { return errno; }

#ifndef MAKEWORD
#define MAKEWORD(a, b) ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#endif

#ifndef DWORD_PTR
typedef unsigned long DWORD_PTR;
#endif

// ---------------------------------------------------------------------------
// __declspec stub
// ---------------------------------------------------------------------------
#ifndef __declspec
#define __declspec(x)
#endif

// ---------------------------------------------------------------------------
// SAL annotations (empty stubs)
// ---------------------------------------------------------------------------
#ifndef _In_
#define _In_
#endif
#ifndef _Out_
#define _Out_
#endif
#ifndef _Inout_
#define _Inout_
#endif
#ifndef _In_opt_
#define _In_opt_
#endif
#ifndef _Out_opt_
#define _Out_opt_
#endif
#ifndef _In_reads_(s)
#define _In_reads_(s)
#endif
#ifndef _Out_writes_(s)
#define _Out_writes_(s)
#endif
#ifndef _In_reads_bytes_(s)
#define _In_reads_bytes_(s)
#endif
#ifndef _Out_writes_bytes_(s)
#define _Out_writes_bytes_(s)
#endif
#ifndef _Ret_maybenull_
#define _Ret_maybenull_
#endif
#ifndef _Analysis_assume_
#define _Analysis_assume_(expr)
#endif

// ---------------------------------------------------------------------------
// MEMORYSTATUS (used by memory tracking code)
// ---------------------------------------------------------------------------
#ifndef _MEMORYSTATUS_DEFINED
#define _MEMORYSTATUS_DEFINED
typedef struct _MEMORYSTATUS {
  DWORD dwLength;
  DWORD dwMemoryLoad;
  SIZE_T dwTotalPhys;
  SIZE_T dwAvailPhys;
  SIZE_T dwTotalPageFile;
  SIZE_T dwAvailPageFile;
  SIZE_T dwTotalVirtual;
  SIZE_T dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

inline VOID GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer)
{
  if (lpBuffer)
  {
    memset(lpBuffer, 0, sizeof(MEMORYSTATUS));
    lpBuffer->dwLength = sizeof(MEMORYSTATUS);
    lpBuffer->dwTotalPhys = 8ULL * 1024 * 1024 * 1024;
    lpBuffer->dwAvailPhys = 4ULL * 1024 * 1024 * 1024;
  }
}
#endif

// ---------------------------------------------------------------------------
// Thread priority / creation constants
// ---------------------------------------------------------------------------
#ifndef THREAD_PRIORITY_ABOVE_NORMAL
#define THREAD_PRIORITY_ABOVE_NORMAL (THREAD_PRIORITY_HIGHEST - 1)
#endif
#ifndef THREAD_PRIORITY_BELOW_NORMAL
#define THREAD_PRIORITY_BELOW_NORMAL (THREAD_PRIORITY_LOWEST + 1)
#endif
#ifndef CREATE_SUSPENDED
#define CREATE_SUSPENDED 0x00000004
#endif

inline DWORD ResumeThread(HANDLE) { return 1; }
inline HANDLE GetCurrentThread(VOID) { return nullptr; }

// ---------------------------------------------------------------------------
// Memory allocation: MEM_LARGE_PAGES
// ---------------------------------------------------------------------------
#ifndef MEM_LARGE_PAGES
#define MEM_LARGE_PAGES 0x20000000
#endif

// ---------------------------------------------------------------------------
// _vsnprintf_s stub (maps to vsnprintf)
// ---------------------------------------------------------------------------
#ifndef _vsnprintf_s
inline int _vsnprintf_s(char *buf, size_t bufSize, size_t, const char *fmt, va_list args)
{
  return vsnprintf(buf, bufSize, fmt, args);
}
// 4-arg form used in Console_Utils.cpp: _vsnprintf_s(buf, count, fmt, args)
inline int _vsnprintf_s(char *buf, size_t count, const char *fmt, va_list args)
{
  return vsnprintf(buf, count, fmt, args);
}
#endif

// ---------------------------------------------------------------------------
// DirectXMath stub functions (used by Camera.cpp)
// ---------------------------------------------------------------------------
inline XMMATRIX XMMatrixMultiply(const XMMATRIX &a, const XMMATRIX &b)
{
  XMMATRIX result;
  const float *A = &a.r[0].x;
  const float *B = &b.r[0].x;
  float *R = &result.r[0].x;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
    {
      float sum = 0.0f;
      for (int k = 0; k < 4; k++)
        sum += A[i * 4 + k] * B[k * 4 + j];
      R[i * 4 + j] = sum;
    }
  return result;
}

inline XMVECTOR XMMatrixDeterminant(const XMMATRIX &)
{
  return XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
}

inline XMMATRIX XMMatrixInverse(const XMVECTOR *, const XMMATRIX &m)
{
  const float *s = &m.r[0].x;
  float inv[16], det;
  inv[0]  =  s[5]*s[10]*s[15] - s[5]*s[11]*s[14] - s[9]*s[6]*s[15] + s[9]*s[7]*s[14] + s[13]*s[6]*s[11] - s[13]*s[7]*s[10];
  inv[4]  = -s[4]*s[10]*s[15] + s[4]*s[11]*s[14] + s[8]*s[6]*s[15] - s[8]*s[7]*s[14] - s[12]*s[6]*s[11] + s[12]*s[7]*s[10];
  inv[8]  =  s[4]*s[9]*s[15]  - s[4]*s[11]*s[13] - s[8]*s[5]*s[15] + s[8]*s[7]*s[13] + s[12]*s[5]*s[11] - s[12]*s[7]*s[9];
  inv[12] = -s[4]*s[9]*s[14]  + s[4]*s[10]*s[13] + s[8]*s[5]*s[14] - s[8]*s[6]*s[13] - s[12]*s[5]*s[10] + s[12]*s[6]*s[9];
  inv[1]  = -s[1]*s[10]*s[15] + s[1]*s[11]*s[14] + s[9]*s[2]*s[15] - s[9]*s[3]*s[14] - s[13]*s[2]*s[11] + s[13]*s[3]*s[10];
  inv[5]  =  s[0]*s[10]*s[15] - s[0]*s[11]*s[14] - s[8]*s[2]*s[15] + s[8]*s[3]*s[14] + s[12]*s[2]*s[11] - s[12]*s[3]*s[10];
  inv[9]  = -s[0]*s[9]*s[15]  + s[0]*s[11]*s[13] + s[8]*s[1]*s[15] - s[8]*s[3]*s[13] - s[12]*s[1]*s[11] + s[12]*s[3]*s[9];
  inv[13] =  s[0]*s[9]*s[14]  - s[0]*s[10]*s[13] - s[8]*s[1]*s[14] + s[8]*s[2]*s[13] + s[12]*s[1]*s[10] - s[12]*s[2]*s[9];
  inv[2]  =  s[1]*s[6]*s[15]  - s[1]*s[7]*s[14]  - s[5]*s[2]*s[15] + s[5]*s[3]*s[14] + s[13]*s[2]*s[7]  - s[13]*s[3]*s[6];
  inv[6]  = -s[0]*s[6]*s[15]  + s[0]*s[7]*s[14]  + s[4]*s[2]*s[15] - s[4]*s[3]*s[14] - s[12]*s[2]*s[7]  + s[12]*s[3]*s[6];
  inv[10] =  s[0]*s[5]*s[15]  - s[0]*s[7]*s[13]  - s[4]*s[1]*s[15] + s[4]*s[3]*s[13] + s[12]*s[1]*s[7]  - s[12]*s[3]*s[5];
  inv[14] = -s[0]*s[5]*s[14]  + s[0]*s[6]*s[13]  + s[4]*s[1]*s[14] - s[4]*s[2]*s[13] - s[12]*s[1]*s[6]  + s[12]*s[2]*s[5];
  inv[3]  = -s[1]*s[6]*s[11]  + s[1]*s[7]*s[10]  + s[5]*s[2]*s[11] - s[5]*s[3]*s[10] - s[9]*s[2]*s[7]   + s[9]*s[3]*s[6];
  inv[7]  =  s[0]*s[6]*s[11]  - s[0]*s[7]*s[10]  - s[4]*s[2]*s[11] + s[4]*s[3]*s[10] + s[8]*s[2]*s[7]   - s[8]*s[3]*s[6];
  inv[11] = -s[0]*s[5]*s[11]  + s[0]*s[7]*s[9]   + s[4]*s[1]*s[11] - s[4]*s[3]*s[9]  - s[8]*s[1]*s[7]   + s[8]*s[3]*s[5];
  inv[15] =  s[0]*s[5]*s[10]  - s[0]*s[6]*s[9]   - s[4]*s[1]*s[10] + s[4]*s[2]*s[9]  + s[8]*s[1]*s[6]   - s[8]*s[2]*s[5];
  det = s[0]*inv[0] + s[1]*inv[4] + s[2]*inv[8] + s[3]*inv[12];
  if (det == 0.0f) det = 1.0f;
  float invDet = 1.0f / det;
  XMMATRIX result;
  float *r = &result.r[0].x;
  for (int i = 0; i < 16; i++)
    r[i] = inv[i] * invDet;
  return result;
}

inline void XMStoreFloat4(XMFLOAT4 *dest, const XMVECTOR &src)
{
  if (dest) { dest->x = src.x; dest->y = src.y; dest->z = src.z; dest->w = src.w; }
}

// ---------------------------------------------------------------------------
// SceCesUcsContext stub (Sony string conversion, used in UI code)
// ---------------------------------------------------------------------------
#ifndef _SCE_CES_UCS_CONTEXT_DEFINED
#define _SCE_CES_UCS_CONTEXT_DEFINED
typedef struct SceCesUcsContext { int dummy; } SceCesUcsContext;
inline void sceCesUcsContextInit(SceCesUcsContext *) {}
inline void sceCesUtf8StrToUtf16Str(SceCesUcsContext *, const uint8_t *src, uint32_t srcMax, uint32_t *srcLen, uint16_t *dst, uint32_t dstMax, uint32_t *dstLen)
{
  uint32_t si = 0, di = 0;
  while (si < srcMax && di < dstMax && src[si] != 0)
    dst[di++] = (uint16_t)src[si++];
  if (di < dstMax) dst[di] = 0;
  if (srcLen) *srcLen = si;
  if (dstLen) *dstLen = di;
}
#endif

// ---------------------------------------------------------------------------
// PAGE_READWRITE (used by XPhysicalAlloc calls)
// ---------------------------------------------------------------------------
#ifndef PAGE_READWRITE
#define PAGE_READWRITE 0x04
#endif

// ---------------------------------------------------------------------------
// Miles Sound System (MSS) stubs for macOS
// ---------------------------------------------------------------------------
// The console/Windows builds link against the Miles Sound System library.
// On macOS we provide no-op stubs so that SoundEngine.cpp compiles and links
// without the proprietary Miles SDK.  All audio will be silent; a real
// implementation can be wired in later (e.g. via OpenAL or AVFoundation).
// ---------------------------------------------------------------------------

// --- Primitive types expected by Miles headers ---
// Guards match those in rrcore.h (used by both Miles and Iggy) so the
// types are not redefined with incompatible widths later.
#ifndef MSS_TYPES_DEFINED
#define MSS_TYPES_DEFINED

#define S8_DEFINED
typedef signed char        S8;
#define U8_DEFINED
typedef unsigned char      U8;
#define S16_DEFINED
typedef signed short       S16;
#define U16_DEFINED
typedef unsigned short     U16;
#define S32_DEFINED
typedef signed int         S32;
#define U32_DEFINED
typedef unsigned int       U32;
#define S64_DEFINED
typedef signed long long   S64;
#define U64_DEFINED
typedef unsigned long long U64;
#define F32_DEFINED
typedef float              F32;
#define F64_DEFINED
typedef double             F64;
#define SINTa_DEFINED
typedef intptr_t           SINTa;
#define UINTa_DEFINED
typedef uintptr_t          UINTa;
typedef char               C8;

#endif

// --- Calling-convention macros ---
#ifndef AILCALL
#define AILCALL
#endif
#ifndef AILCALLBACK
#define AILCALLBACK
#endif
#ifndef AILEXPORT
#define AILEXPORT
#endif
#ifndef DXDEC
#define DXDEC
#endif

// --- Opaque handle types ---
typedef void *HSAMPLE;
typedef void *HDIGDRIVER;
typedef void *HSTREAM;
typedef void *HMSOUNDBANK;
typedef void *HEVENTSYSTEM;

// --- Enumeration type ---
typedef SINTa HMSSENUM;
#define MSS_FIRST ((HMSSENUM)-1)

// --- Sample format constants ---
#ifndef DIG_F_MONO_8
#define DIG_F_MONO_8        0x0000
#define DIG_F_MONO_16       0x0001
#define DIG_F_STEREO_8      0x0002
#define DIG_F_STEREO_16     0x0003
#endif

// --- Preference constants ---
#ifndef DIG_MIXER_CHANNELS
#define DIG_MIXER_CHANNELS  3
#endif

// --- Multi-channel configuration ---
#ifndef MSS_MC_USE_SYSTEM_CONFIG
#define MSS_MC_USE_SYSTEM_CONFIG   0x10
#endif
#ifndef MSS_MC_STEREO
#define MSS_MC_STEREO              2
#endif

// --- Driver open flags ---
#ifndef AIL_OPEN_DIGITAL_USE_SPU0
#define AIL_OPEN_DIGITAL_USE_SPU0  (1<<24)
#endif

// --- Sample status constants ---
#ifndef SMP_FREE
#define SMP_FREE   0x0001
#endif
#ifndef SMP_DONE
#define SMP_DONE   0x0002
#endif
#ifndef SMP_PLAYING
#define SMP_PLAYING 0x0004
#endif

// --- Event sound status constants ---
#ifndef MILESEVENT_SOUND_STATUS_PENDING
#define MILESEVENT_SOUND_STATUS_PENDING   0x1
#endif
#ifndef MILESEVENT_SOUND_STATUS_PLAYING
#define MILESEVENT_SOUND_STATUS_PLAYING   0x2
#endif
#ifndef MILESEVENT_SOUND_STATUS_COMPLETE
#define MILESEVENT_SOUND_STATUS_COMPLETE  0x4
#endif

// --- Enqueue flags ---
#ifndef MILESEVENT_ENQUEUE_BUFFER_PTR
#define MILESEVENT_ENQUEUE_BUFFER_PTR     0x1
#endif

// --- Event sound info structure ---
typedef struct _MILESEVENTSOUNDINFO
{
    U64     QueuedID;
    U64     InstanceID;
    U64     EventID;
    HSAMPLE Sample;
    S32     Status;
    void   *UserBuffer;
    S32     UserBufferLen;
    S32     UsedDelay;
    F32     UsedVolume;
    F32     UsedPitch;
    char const *UsedSound;
    S32     HasCompletionEvent;
} MILESEVENTSOUNDINFO;

// --- Callback typedefs ---
typedef void  (AILCALLBACK *AILSAMPLECB)(HSAMPLE sample);
typedef F32   (AILCALLBACK *AILFALLOFFCB)(HSAMPLE sample, F32 distance, F32 rolloff_factor, F32 min_dist, F32 max_dist);
typedef void  (AILCALLBACK *AILMIXERCB)(HDIGDRIVER dig);
typedef void  (AILCALLBACK *AILSTREAMCB)(HSTREAM stream);
typedef void   AILCALLBACK  AILEVENTERRORCB(S64 i_RelevantId, char const *i_Resource);

// --- Register_RIB / BinkADec stubs ---
#ifndef Register_RIB
#define Register_RIB(name) ((void)0)
#endif
#ifndef BinkADec
#define BinkADec  0
#endif

// --- Core lifecycle ---
inline S32  AILCALL AIL_startup(void) { return 1; }
inline void AILCALL AIL_shutdown(void) {}
inline char *AILCALL AIL_last_error(void) { static char e[] = ""; return e; }
inline SINTa AILCALL AIL_set_preference(U32 number, SINTa value) { (void)number; (void)value; return 0; }
inline char *AILCALL AIL_set_redist_directory(char const *dir) { (void)dir; static char d[] = ""; return d; }

// --- Digital driver ---
inline HDIGDRIVER AILCALL AIL_open_digital_driver(U32 frequency, S32 bits, S32 channel_config, S32 flags)
{ (void)frequency; (void)bits; (void)channel_config; (void)flags; static int dummy; return (HDIGDRIVER)&dummy; }
inline void AILCALL AIL_close_digital_driver(HDIGDRIVER dig) { (void)dig; }
inline void AILCALL AIL_set_speaker_configuration(HDIGDRIVER dig, F32 a, F32 b, F32 c) { (void)dig; (void)a; (void)b; (void)c; }

// --- Event system ---
inline HEVENTSYSTEM AILCALL AIL_startup_event_system(HDIGDRIVER dig, S32 command_buf_len, char *memory_buf, S32 memory_len)
{ (void)dig; (void)command_buf_len; (void)memory_buf; (void)memory_len; static int dummy; return (HEVENTSYSTEM)&dummy; }
inline void AILCALL AIL_set_event_error_callback(AILEVENTERRORCB *cb) { (void)cb; }

// --- Soundbank ---
inline HMSOUNDBANK AILCALL AIL_add_soundbank(char const *filename, char const *name)
{ (void)filename; (void)name; static int dummy; return (HMSOUNDBANK)&dummy; }

// --- Event enumeration ---
inline S32 AILCALL AIL_enumerate_events(HMSOUNDBANK bank, HMSSENUM *next, char const *list, char const **name)
{ (void)bank; (void)next; (void)list; (void)name; return 0; }

// --- Event queue ---
inline S32 AILCALL AIL_enqueue_event_start(void) { return 0; }
inline S32 AILCALL AIL_enqueue_event_buffer(S32 *token, void *user_buffer, S32 user_buffer_len, S32 user_buffer_is_ptr)
{ (void)token; (void)user_buffer; (void)user_buffer_len; (void)user_buffer_is_ptr; return 0; }
inline U64 AILCALL AIL_enqueue_event_end_named(S32 token, char const *event_name)
{ (void)token; (void)event_name; return 0; }
inline U64 AILCALL AIL_enqueue_event_by_name(char const *name) { (void)name; return 0; }
inline S32 AILCALL AIL_begin_event_queue_processing(void) { return 0; }
inline S32 AILCALL AIL_complete_event_queue_processing(void) { return 0; }

// --- Sound instance enumeration ---
inline S32 AILCALL AIL_enumerate_sound_instances(HEVENTSYSTEM system, HMSSENUM *next, S32 statuses, char const *label_query, U64 search_for_ID, MILESEVENTSOUNDINFO *info)
{ (void)system; (void)next; (void)statuses; (void)label_query; (void)search_for_ID; (void)info; return 0; }

// --- Sample handle management ---
inline HSAMPLE AILCALL AIL_allocate_sample_handle(HDIGDRIVER dig) { (void)dig; return nullptr; }
inline void    AILCALL AIL_release_sample_handle(HSAMPLE S) { (void)S; }
inline S32     AILCALL AIL_init_sample(HSAMPLE S, S32 format) { (void)S; (void)format; return 0; }
inline void    AILCALL AIL_set_sample_address(HSAMPLE S, void const *buf, U32 len) { (void)S; (void)buf; (void)len; }
inline void    AILCALL AIL_start_sample(HSAMPLE S) { (void)S; }

// --- Sample properties ---
inline void AILCALL AIL_set_sample_volume_levels(HSAMPLE S, F32 left, F32 right) { (void)S; (void)left; (void)right; }
inline void AILCALL AIL_set_sample_playback_rate_factor(HSAMPLE S, F32 factor) { (void)S; (void)factor; }
inline S32  AILCALL AIL_sample_playback_rate(HSAMPLE S) { (void)S; return 44100; }
inline F32  AILCALL AIL_sample_playback_rate_factor(HSAMPLE S) { (void)S; return 1.0f; }
inline S32  AILCALL AIL_set_sample_is_3D(HSAMPLE S, S32 onoff) { (void)S; (void)onoff; return 1; }

// --- 3D positioning ---
inline void AILCALL AIL_set_sample_3D_position(HSAMPLE S, F32 x, F32 y, F32 z) { (void)S; (void)x; (void)y; (void)z; }
inline void AILCALL AIL_set_sample_3D_distances(HSAMPLE S, F32 max_dist, F32 min_dist, S32 auto_update)
{ (void)S; (void)max_dist; (void)min_dist; (void)auto_update; }
inline void AILCALL AIL_set_listener_3D_position(HDIGDRIVER dig, F32 x, F32 y, F32 z)
{ (void)dig; (void)x; (void)y; (void)z; }
inline void AILCALL AIL_set_listener_3D_orientation(HDIGDRIVER dig, F32 fx, F32 fy, F32 fz, F32 ux, F32 uy, F32 uz)
{ (void)dig; (void)fx; (void)fy; (void)fz; (void)ux; (void)uy; (void)uz; }
inline void AILCALL AIL_set_3D_rolloff_factor(HDIGDRIVER dig, F32 factor)
{ (void)dig; (void)factor; }

// --- Falloff callback ---
inline AILFALLOFFCB AILCALL AIL_register_falloff_function_callback(HSAMPLE S, AILFALLOFFCB cb)
{ (void)S; (void)cb; return nullptr; }

// --- Mixer callback ---
inline AILMIXERCB AILCALL AIL_register_mix_callback(HDIGDRIVER dig, AILMIXERCB cb)
{ (void)dig; (void)cb; return nullptr; }

// --- Streaming ---
inline HSTREAM AILCALL AIL_open_stream(HDIGDRIVER dig, char const *filename, S32 stream_mem)
{ (void)dig; (void)filename; (void)stream_mem; return nullptr; }
inline void    AILCALL AIL_close_stream(HSTREAM stream) { (void)stream; }
inline HSAMPLE AILCALL AIL_stream_sample_handle(HSTREAM stream) { (void)stream; return nullptr; }
inline void    AILCALL AIL_start_stream(HSTREAM stream) { (void)stream; }
inline void    AILCALL AIL_pause_stream(HSTREAM stream, S32 onoff) { (void)stream; (void)onoff; }
inline S32     AILCALL AIL_stream_status(HSTREAM stream) { (void)stream; return SMP_DONE; }

// --- Timer ---
inline U32 AILCALL AIL_ms_count(void) { return 0; }
inline U64 AILCALL AIL_ms_count64(void) { return 0; }

// --- Platform property ---
inline S32 AILCALL AIL_platform_property(void *object, S32 prop, void *value, void *value2, void *value3)
{ (void)object; (void)prop; (void)value; (void)value2; (void)value3; return 0; }

// --- Variable get/set ---
inline F32 AILCALL AIL_get_variable_float(HEVENTSYSTEM system, char const *name)
{ (void)system; (void)name; return 0.0f; }
inline void AILCALL AIL_set_variable_float(HEVENTSYSTEM system, char const *name, F32 value)
{ (void)system; (void)name; (void)value; }

// ---------------------------------------------------------------------------
// End of Miles Sound System stubs
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// MSVC-specific safe integer-to-string functions
// ---------------------------------------------------------------------------

// _i64toa_s: convert 64-bit integer to char string (MSVC CRT)
inline int _i64toa_s(long long value, char *buffer, size_t bufferSize, int radix)
{
  if (!buffer || bufferSize == 0) return -1;
  if (radix == 10)
    snprintf(buffer, bufferSize, "%lld", value);
  else if (radix == 16)
    snprintf(buffer, bufferSize, "%llx", value);
  else if (radix == 8)
    snprintf(buffer, bufferSize, "%llo", value);
  else
    snprintf(buffer, bufferSize, "%lld", value);
  return 0;
}

// _itoa_s: convert int to char string (MSVC CRT)
inline int _itoa_s(int value, char *buffer, size_t bufferSize, int radix)
{
  if (!buffer || bufferSize == 0) return -1;
  if (radix == 10)
    snprintf(buffer, bufferSize, "%d", value);
  else if (radix == 16)
    snprintf(buffer, bufferSize, "%x", value);
  else if (radix == 8)
    snprintf(buffer, bufferSize, "%o", value);
  else
    snprintf(buffer, bufferSize, "%d", value);
  return 0;
}

// Also support unsigned int overload (uiHostOptions, uiTexturePackId are unsigned)
inline int _itoa_s(unsigned int value, char *buffer, size_t bufferSize, int radix)
{
  if (!buffer || bufferSize == 0) return -1;
  if (radix == 10)
    snprintf(buffer, bufferSize, "%u", value);
  else if (radix == 16)
    snprintf(buffer, bufferSize, "%x", value);
  else if (radix == 8)
    snprintf(buffer, bufferSize, "%o", value);
  else
    snprintf(buffer, bufferSize, "%u", value);
  return 0;
}

// ---------------------------------------------------------------------------
// Stub string resource IDs
// ---------------------------------------------------------------------------
// These IDs are defined in platform-specific strings.h for Sony/Durango
// platforms but are missing from the Windows64 string table used by the
// Apple/macOS build.  We define them here as placeholder values so that
// code guarded by __APPLE__ (or accidentally unguarded) can compile.
// ---------------------------------------------------------------------------
#ifndef IDS_CONTENT_RESTRICTION
#define IDS_CONTENT_RESTRICTION              0xF000
#endif
#ifndef IDS_CONTENT_RESTRICTION_MULTIPLAYER
#define IDS_CONTENT_RESTRICTION_MULTIPLAYER  0xF001
#endif
#ifndef IDS_ONLINE_SERVICE_TITLE
#define IDS_ONLINE_SERVICE_TITLE             0xF002
#endif
#ifndef IDS_SAVE_INCOMPLETE_RETRY_SAVING
#define IDS_SAVE_INCOMPLETE_RETRY_SAVING     0xF003
#endif
#ifndef IDS_SAVE_INCOMPLETE_DELETE_SAVES
#define IDS_SAVE_INCOMPLETE_DELETE_SAVES      0xF004
#endif
#ifndef IDS_SAVE_INCOMPLETE_TITLE
#define IDS_SAVE_INCOMPLETE_TITLE            0xF005
#endif
#ifndef IDS_SAVE_INCOMPLETE_EXPLANATION_QUOTA
#define IDS_SAVE_INCOMPLETE_EXPLANATION_QUOTA 0xF006
#endif
#ifndef IDS_SAVE_INCOMPLETE_EXPLANATION_LOCAL_STORAGE
#define IDS_SAVE_INCOMPLETE_EXPLANATION_LOCAL_STORAGE 0xF007
#endif
#ifndef IDS_SAVE_INCOMPLETE_DISABLE_SAVING
#define IDS_SAVE_INCOMPLETE_DISABLE_SAVING   0xF008
#endif
#ifndef IDS_CONTENT_RESTRICTION_PATCH_AVAILABLE
#define IDS_CONTENT_RESTRICTION_PATCH_AVAILABLE 0xF009
#endif

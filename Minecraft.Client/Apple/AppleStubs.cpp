#include "AppleStubs.h"

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cwctype>
#include <ctime>
#include <mutex>
#include <thread>
#include <vector>
#include <string>

#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <mach/mach_time.h>
#include <glob.h>
#include <fcntl.h>

// ============================================================================
// Thread-Local Storage
// ============================================================================
namespace
{
std::mutex g_tlsMutex;
std::vector<pthread_key_t> g_tlsKeys;
}

VOID InitializeCriticalSection(PCRITICAL_SECTION criticalSection)
{
  pthread_mutexattr_t attributes;
  pthread_mutexattr_init(&attributes);
  pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&criticalSection->mutex, &attributes);
  pthread_mutexattr_destroy(&attributes);
}

VOID InitializeCriticalSectionAndSpinCount(PCRITICAL_SECTION criticalSection, ULONG)
{
  InitializeCriticalSection(criticalSection);
}

VOID DeleteCriticalSection(PCRITICAL_SECTION criticalSection)
{
  pthread_mutex_destroy(&criticalSection->mutex);
}

VOID EnterCriticalSection(PCRITICAL_SECTION criticalSection)
{
  pthread_mutex_lock(&criticalSection->mutex);
}

VOID LeaveCriticalSection(PCRITICAL_SECTION criticalSection)
{
  pthread_mutex_unlock(&criticalSection->mutex);
}

ULONG TryEnterCriticalSection(PCRITICAL_SECTION criticalSection)
{
  return pthread_mutex_trylock(&criticalSection->mutex) == 0 ? TRUE : FALSE;
}

DWORD TlsAlloc(VOID)
{
  pthread_key_t key {};
  if (pthread_key_create(&key, nullptr) != 0)
  {
    return static_cast<DWORD>(-1);
  }

  std::lock_guard<std::mutex> lock(g_tlsMutex);
  g_tlsKeys.push_back(key);
  return static_cast<DWORD>(g_tlsKeys.size() - 1);
}

LPVOID TlsGetValue(DWORD dwTlsIndex)
{
  std::lock_guard<std::mutex> lock(g_tlsMutex);
  if (dwTlsIndex >= g_tlsKeys.size())
  {
    return nullptr;
  }
  return pthread_getspecific(g_tlsKeys[dwTlsIndex]);
}

BOOL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
  std::lock_guard<std::mutex> lock(g_tlsMutex);
  if (dwTlsIndex >= g_tlsKeys.size())
  {
    return FALSE;
  }
  return pthread_setspecific(g_tlsKeys[dwTlsIndex], lpTlsValue) == 0 ? TRUE : FALSE;
}

// ============================================================================
// Threading
// ============================================================================
namespace
{
struct AppleThreadContext
{
  LPTHREAD_START_ROUTINE startRoutine;
  LPVOID parameter;
};

struct AppleEvent
{
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  bool signaled;
  bool manualReset;
};

constexpr uintptr_t kAppleEventHandleTag = 0x1;

inline bool isAppleEventHandle(HANDLE handle)
{
  if (!handle || handle == INVALID_HANDLE_VALUE)
  {
    return false;
  }

  return (reinterpret_cast<uintptr_t>(handle) & kAppleEventHandleTag) != 0;
}

inline AppleEvent *getAppleEvent(HANDLE handle)
{
  return reinterpret_cast<AppleEvent *>(reinterpret_cast<uintptr_t>(handle) & ~kAppleEventHandleTag);
}

void *appleThreadEntry(void *arg)
{
  AppleThreadContext *ctx = static_cast<AppleThreadContext *>(arg);
  LPTHREAD_START_ROUTINE routine = ctx->startRoutine;
  LPVOID param = ctx->parameter;
  delete ctx;
  DWORD result = routine(param);
  return reinterpret_cast<void *>(static_cast<uintptr_t>(result));
}
}

HANDLE CreateThread(
  LPVOID,
  SIZE_T,
  LPTHREAD_START_ROUTINE lpStartAddress,
  LPVOID lpParameter,
  DWORD,
  LPDWORD lpThreadId)
{
  AppleThreadContext *ctx = new AppleThreadContext{lpStartAddress, lpParameter};
  pthread_t *thread = new pthread_t;
  if (pthread_create(thread, nullptr, appleThreadEntry, ctx) != 0)
  {
    delete ctx;
    delete thread;
    return nullptr;
  }
  if (lpThreadId)
  {
    *lpThreadId = static_cast<DWORD>(reinterpret_cast<uintptr_t>(*thread) & 0xFFFFFFFF);
  }
  return reinterpret_cast<HANDLE>(thread);
}

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
  if (!hHandle) return WAIT_TIMEOUT;

  if (isAppleEventHandle(hHandle))
  {
    AppleEvent *evt = getAppleEvent(hHandle);
    pthread_mutex_lock(&evt->mutex);

    int waitResult = 0;
    if (!evt->signaled)
    {
      if (dwMilliseconds == INFINITE)
      {
        while (!evt->signaled)
        {
          waitResult = pthread_cond_wait(&evt->cond, &evt->mutex);
          if (waitResult != 0)
          {
            pthread_mutex_unlock(&evt->mutex);
            return WAIT_FAILED;
          }
        }
      }
      else
      {
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += dwMilliseconds / 1000;
        deadline.tv_nsec += static_cast<long>(dwMilliseconds % 1000) * 1000000L;
        if (deadline.tv_nsec >= 1000000000L)
        {
          deadline.tv_sec += 1;
          deadline.tv_nsec -= 1000000000L;
        }

        while (!evt->signaled)
        {
          waitResult = pthread_cond_timedwait(&evt->cond, &evt->mutex, &deadline);
          if (waitResult == ETIMEDOUT)
          {
            pthread_mutex_unlock(&evt->mutex);
            return WAIT_TIMEOUT;
          }
          if (waitResult != 0)
          {
            pthread_mutex_unlock(&evt->mutex);
            return WAIT_FAILED;
          }
        }
      }
    }

    if (!evt->manualReset)
    {
      evt->signaled = false;
    }

    pthread_mutex_unlock(&evt->mutex);
    return WAIT_OBJECT_0;
  }

  // Thread handle
  pthread_t *thread = reinterpret_cast<pthread_t *>(hHandle);
  if (dwMilliseconds == INFINITE)
  {
    pthread_join(*thread, nullptr);
    return WAIT_OBJECT_0;
  }
  // Non-infinite wait on a thread - just try join (simplified)
  pthread_join(*thread, nullptr);
  return WAIT_OBJECT_0;
}

DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds)
{
  if (nCount == 0 || !lpHandles)
  {
    return WAIT_FAILED;
  }

  const auto start = std::chrono::steady_clock::now();
  while (true)
  {
    bool allSignaled = true;
    for (DWORD i = 0; i < nCount; i++)
    {
      HANDLE handle = lpHandles[i];
      if (isAppleEventHandle(handle))
      {
        AppleEvent *evt = getAppleEvent(handle);
        pthread_mutex_lock(&evt->mutex);
        const bool signaled = evt->signaled;
        pthread_mutex_unlock(&evt->mutex);

        if (signaled)
        {
          if (!bWaitAll)
          {
            if (!evt->manualReset)
            {
              pthread_mutex_lock(&evt->mutex);
              evt->signaled = false;
              pthread_mutex_unlock(&evt->mutex);
            }
            return WAIT_OBJECT_0 + i;
          }
        }
        else
        {
          allSignaled = false;
        }
      }
      else
      {
        DWORD result = WaitForSingleObject(handle, 0);
        if (result == WAIT_OBJECT_0)
        {
          if (!bWaitAll)
          {
            return WAIT_OBJECT_0 + i;
          }
        }
        else
        {
          allSignaled = false;
        }
      }
    }

    if (bWaitAll && allSignaled)
    {
      for (DWORD i = 0; i < nCount; i++)
      {
        HANDLE handle = lpHandles[i];
        if (isAppleEventHandle(handle))
        {
          AppleEvent *evt = getAppleEvent(handle);
          if (!evt->manualReset)
          {
            pthread_mutex_lock(&evt->mutex);
            evt->signaled = false;
            pthread_mutex_unlock(&evt->mutex);
          }
        }
      }
      return WAIT_OBJECT_0;
    }

    if (dwMilliseconds == 0)
    {
      return WAIT_TIMEOUT;
    }

    if (dwMilliseconds != INFINITE)
    {
      const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
      if (elapsed.count() >= dwMilliseconds)
      {
        return WAIT_TIMEOUT;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  return WAIT_TIMEOUT;
}

BOOL GetExitCodeThread(HANDLE, LPDWORD lpExitCode)
{
  if (lpExitCode) *lpExitCode = 0;
  return TRUE;
}

HANDLE CreateEvent(LPVOID, BOOL bManualReset, BOOL bInitialState, LPCSTR)
{
  AppleEvent *evt = new AppleEvent;
  pthread_mutex_init(&evt->mutex, nullptr);
  pthread_cond_init(&evt->cond, nullptr);
  evt->signaled = bInitialState != 0;
  evt->manualReset = bManualReset != 0;
  return reinterpret_cast<HANDLE>(reinterpret_cast<uintptr_t>(evt) | kAppleEventHandleTag);
}

BOOL SetEvent(HANDLE hEvent)
{
  if (!isAppleEventHandle(hEvent)) return FALSE;
  AppleEvent *evt = getAppleEvent(hEvent);
  pthread_mutex_lock(&evt->mutex);
  evt->signaled = true;
  if (evt->manualReset)
    pthread_cond_broadcast(&evt->cond);
  else
    pthread_cond_signal(&evt->cond);
  pthread_mutex_unlock(&evt->mutex);
  return TRUE;
}

BOOL ResetEvent(HANDLE hEvent)
{
  if (!isAppleEventHandle(hEvent)) return FALSE;
  AppleEvent *evt = getAppleEvent(hEvent);
  pthread_mutex_lock(&evt->mutex);
  evt->signaled = false;
  pthread_mutex_unlock(&evt->mutex);
  return TRUE;
}

// ============================================================================
// CloseHandle (threads + events)
// ============================================================================
BOOL CloseHandle(HANDLE hObject)
{
  if (!hObject || hObject == INVALID_HANDLE_VALUE)
    return FALSE;

  if (isAppleEventHandle(hObject))
  {
    AppleEvent *evt = getAppleEvent(hObject);
    pthread_cond_destroy(&evt->cond);
    pthread_mutex_destroy(&evt->mutex);
    delete evt;
    return TRUE;
  }

  return TRUE;
}

// ============================================================================
// System info / time
// ============================================================================
VOID Sleep(DWORD dwMilliseconds)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(dwMilliseconds));
}

DWORD GetCurrentThreadId(VOID)
{
  uint64_t threadId = 0;
  pthread_threadid_np(nullptr, &threadId);
  return static_cast<DWORD>(threadId & 0xffffffffu);
}

DWORD GetCurrentProcessId(VOID)
{
  return static_cast<DWORD>(getpid());
}

namespace
{
void fillSystemTimeFromTm(const tm &source, int milliseconds, LPSYSTEMTIME destination)
{
  if (destination == nullptr) return;
  destination->wYear = static_cast<WORD>(source.tm_year + 1900);
  destination->wMonth = static_cast<WORD>(source.tm_mon + 1);
  destination->wDayOfWeek = static_cast<WORD>(source.tm_wday);
  destination->wDay = static_cast<WORD>(source.tm_mday);
  destination->wHour = static_cast<WORD>(source.tm_hour);
  destination->wMinute = static_cast<WORD>(source.tm_min);
  destination->wSecond = static_cast<WORD>(source.tm_sec);
  destination->wMilliseconds = static_cast<WORD>(milliseconds);
}
}

VOID GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
  const auto now = std::chrono::system_clock::now();
  const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  const std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
  tm utcTime {};
  gmtime_r(&currentTime, &utcTime);
  fillSystemTimeFromTm(utcTime, static_cast<int>(milliseconds.count()), lpSystemTime);
}

VOID GetLocalTime(LPSYSTEMTIME lpSystemTime)
{
  const auto now = std::chrono::system_clock::now();
  const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  const std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
  tm localTime {};
  localtime_r(&currentTime, &localTime);
  fillSystemTimeFromTm(localTime, static_cast<int>(milliseconds.count()), lpSystemTime);
}

HMODULE GetModuleHandle(LPCSTR)
{
  return nullptr;
}

DWORD GetTickCount(VOID)
{
  static mach_timebase_info_data_t timebase = {0, 0};
  if (timebase.denom == 0) mach_timebase_info(&timebase);
  uint64_t t = mach_absolute_time();
  uint64_t ns = t * timebase.numer / timebase.denom;
  return static_cast<DWORD>(ns / 1000000ULL);
}

BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
{
  if (!lpPerformanceCount) return FALSE;
  static mach_timebase_info_data_t timebase = {0, 0};
  if (timebase.denom == 0) mach_timebase_info(&timebase);
  uint64_t t = mach_absolute_time();
  lpPerformanceCount->QuadPart = static_cast<LONGLONG>(t * timebase.numer / timebase.denom);
  return TRUE;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency)
{
  if (!lpFrequency) return FALSE;
  // We report in nanoseconds
  lpFrequency->QuadPart = 1000000000LL;
  return TRUE;
}

namespace { DWORD g_lastError = 0; }
DWORD GetLastError(VOID) { return g_lastError; }
VOID SetLastError(DWORD dwErrCode) { g_lastError = dwErrCode; }

BOOL SystemTimeToFileTime(const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime)
{
  if (!lpSystemTime || !lpFileTime) return FALSE;
  tm t {};
  t.tm_year = lpSystemTime->wYear - 1900;
  t.tm_mon = lpSystemTime->wMonth - 1;
  t.tm_mday = lpSystemTime->wDay;
  t.tm_hour = lpSystemTime->wHour;
  t.tm_min = lpSystemTime->wMinute;
  t.tm_sec = lpSystemTime->wSecond;
  time_t epoch = timegm(&t);
  // Windows FILETIME: 100-ns intervals since 1601-01-01
  uint64_t ft = (static_cast<uint64_t>(epoch) + 11644473600ULL) * 10000000ULL;
  ft += static_cast<uint64_t>(lpSystemTime->wMilliseconds) * 10000ULL;
  lpFileTime->dwLowDateTime = static_cast<DWORD>(ft & 0xFFFFFFFF);
  lpFileTime->dwHighDateTime = static_cast<DWORD>(ft >> 32);
  return TRUE;
}

BOOL FileTimeToSystemTime(const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime)
{
  if (!lpFileTime || !lpSystemTime) return FALSE;
  uint64_t ft = (static_cast<uint64_t>(lpFileTime->dwHighDateTime) << 32) | lpFileTime->dwLowDateTime;
  uint64_t epoch100ns = ft - 116444736000000000ULL;
  time_t epoch = static_cast<time_t>(epoch100ns / 10000000ULL);
  int ms = static_cast<int>((epoch100ns % 10000000ULL) / 10000);
  tm utcTime {};
  gmtime_r(&epoch, &utcTime);
  fillSystemTimeFromTm(utcTime, ms, lpSystemTime);
  return TRUE;
}

// ============================================================================
// File I/O (POSIX-backed)
// ============================================================================
namespace
{
struct AppleFileHandle
{
  int fd;
};
}

HANDLE CreateFileA(
  LPCSTR lpFileName,
  DWORD dwDesiredAccess,
  DWORD,
  LPVOID,
  DWORD dwCreationDisposition,
  DWORD,
  HANDLE)
{
  if (!lpFileName) return INVALID_HANDLE_VALUE;

  int flags = 0;
  if ((dwDesiredAccess & GENERIC_READ) && (dwDesiredAccess & GENERIC_WRITE))
    flags = O_RDWR;
  else if (dwDesiredAccess & GENERIC_WRITE)
    flags = O_WRONLY;
  else
    flags = O_RDONLY;

  switch (dwCreationDisposition)
  {
  case CREATE_ALWAYS:
    flags |= O_CREAT | O_TRUNC;
    break;
  case OPEN_ALWAYS:
    flags |= O_CREAT;
    break;
  case OPEN_EXISTING:
  default:
    break;
  }

  int fd = open(lpFileName, flags, 0666);
  if (fd < 0) return INVALID_HANDLE_VALUE;

  AppleFileHandle *fh = new AppleFileHandle{fd};
  return reinterpret_cast<HANDLE>(fh);
}

BOOL ReadFile(
  HANDLE hFile,
  LPVOID lpBuffer,
  DWORD nNumberOfBytesToRead,
  LPDWORD lpNumberOfBytesRead,
  LPVOID)
{
  if (!hFile || hFile == INVALID_HANDLE_VALUE) return FALSE;
  AppleFileHandle *fh = reinterpret_cast<AppleFileHandle *>(hFile);
  ssize_t result = read(fh->fd, lpBuffer, nNumberOfBytesToRead);
  if (result < 0)
  {
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = 0;
    return FALSE;
  }
  if (lpNumberOfBytesRead) *lpNumberOfBytesRead = static_cast<DWORD>(result);
  return TRUE;
}

BOOL WriteFile(
  HANDLE hFile,
  LPCVOID lpBuffer,
  DWORD nNumberOfBytesToWrite,
  LPDWORD lpNumberOfBytesWritten,
  LPVOID)
{
  if (!hFile || hFile == INVALID_HANDLE_VALUE) return FALSE;
  AppleFileHandle *fh = reinterpret_cast<AppleFileHandle *>(hFile);
  ssize_t result = write(fh->fd, lpBuffer, nNumberOfBytesToWrite);
  if (result < 0)
  {
    if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = 0;
    return FALSE;
  }
  if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = static_cast<DWORD>(result);
  return TRUE;
}

DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG *lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
  if (!hFile || hFile == INVALID_HANDLE_VALUE) return INVALID_SET_FILE_POINTER;
  AppleFileHandle *fh = reinterpret_cast<AppleFileHandle *>(hFile);
  int whence = SEEK_SET;
  if (dwMoveMethod == FILE_CURRENT) whence = SEEK_CUR;
  else if (dwMoveMethod == FILE_END) whence = SEEK_END;

  off_t offset = lDistanceToMove;
  if (lpDistanceToMoveHigh)
    offset |= (static_cast<off_t>(*lpDistanceToMoveHigh) << 32);

  off_t result = lseek(fh->fd, offset, whence);
  if (result == -1) return INVALID_SET_FILE_POINTER;

  if (lpDistanceToMoveHigh)
    *lpDistanceToMoveHigh = static_cast<LONG>(result >> 32);

  return static_cast<DWORD>(result & 0xFFFFFFFF);
}

DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
  if (!hFile || hFile == INVALID_HANDLE_VALUE) return INVALID_SET_FILE_POINTER;
  AppleFileHandle *fh = reinterpret_cast<AppleFileHandle *>(hFile);
  struct stat st;
  if (fstat(fh->fd, &st) != 0) return INVALID_SET_FILE_POINTER;
  if (lpFileSizeHigh) *lpFileSizeHigh = static_cast<DWORD>(st.st_size >> 32);
  return static_cast<DWORD>(st.st_size & 0xFFFFFFFF);
}

BOOL DeleteFileA(LPCSTR lpFileName)
{
  return lpFileName && unlink(lpFileName) == 0 ? TRUE : FALSE;
}

BOOL CreateDirectoryA(LPCSTR lpPathName, LPVOID)
{
  return lpPathName && mkdir(lpPathName, 0755) == 0 ? TRUE : FALSE;
}

BOOL RemoveDirectoryA(LPCSTR lpPathName)
{
  return lpPathName && rmdir(lpPathName) == 0 ? TRUE : FALSE;
}

DWORD GetFileAttributesA(LPCSTR lpFileName)
{
  if (!lpFileName) return INVALID_FILE_ATTRIBUTES;
  struct stat st;
  if (stat(lpFileName, &st) != 0) return INVALID_FILE_ATTRIBUTES;
  DWORD attrs = FILE_ATTRIBUTE_NORMAL;
  if (S_ISDIR(st.st_mode)) attrs = FILE_ATTRIBUTE_DIRECTORY;
  if (!(st.st_mode & S_IWUSR)) attrs |= FILE_ATTRIBUTE_READONLY;
  return attrs;
}

BOOL CopyFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL)
{
  if (!lpExistingFileName || !lpNewFileName) return FALSE;
  FILE *src = fopen(lpExistingFileName, "rb");
  if (!src) return FALSE;
  FILE *dst = fopen(lpNewFileName, "wb");
  if (!dst) { fclose(src); return FALSE; }
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
    fwrite(buf, 1, n, dst);
  fclose(src);
  fclose(dst);
  return TRUE;
}

BOOL MoveFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName)
{
  return (lpExistingFileName && lpNewFileName && rename(lpExistingFileName, lpNewFileName) == 0) ? TRUE : FALSE;
}

DWORD GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer)
{
  if (!lpBuffer || nBufferLength == 0) return 0;
  if (getcwd(lpBuffer, nBufferLength) == nullptr) return 0;
  return static_cast<DWORD>(strlen(lpBuffer));
}

DWORD GetTempPathA(DWORD nBufferLength, LPSTR lpBuffer)
{
  const char *tmp = "/tmp/";
  if (!lpBuffer || nBufferLength < strlen(tmp) + 1) return 0;
  strcpy(lpBuffer, tmp);
  return static_cast<DWORD>(strlen(tmp));
}

DWORD GetModuleFileNameA(HMODULE, LPSTR lpFilename, DWORD nSize)
{
  if (!lpFilename || nSize == 0) return 0;
  // Return empty string - the game will need to find its resources differently on macOS
  lpFilename[0] = '\0';
  return 0;
}

// ============================================================================
// FindFirstFile / FindNextFile (POSIX directory enumeration)
// ============================================================================
namespace
{
struct AppleFindHandle
{
  DIR *dir;
  std::string pattern;
  std::string directory;
};

bool matchWildcard(const char *pattern, const char *str)
{
  while (*pattern)
  {
    if (*pattern == '*')
    {
      pattern++;
      if (!*pattern) return true;
      while (*str)
      {
        if (matchWildcard(pattern, str)) return true;
        str++;
      }
      return false;
    }
    else if (*pattern == '?')
    {
      if (!*str) return false;
      pattern++;
      str++;
    }
    else
    {
      if (*pattern != *str) return false;
      pattern++;
      str++;
    }
  }
  return *str == '\0';
}
}

HANDLE FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
  if (!lpFileName || !lpFindFileData) return INVALID_HANDLE_VALUE;

  std::string path(lpFileName);
  std::string dirPath, filePattern;

  size_t lastSlash = path.find_last_of('/');
  if (lastSlash != std::string::npos)
  {
    dirPath = path.substr(0, lastSlash);
    filePattern = path.substr(lastSlash + 1);
  }
  else
  {
    dirPath = ".";
    filePattern = path;
  }

  DIR *dir = opendir(dirPath.c_str());
  if (!dir) return INVALID_HANDLE_VALUE;

  AppleFindHandle *fh = new AppleFindHandle{dir, filePattern, dirPath};
  HANDLE handle = reinterpret_cast<HANDLE>(fh);

  if (FindNextFileA(handle, lpFindFileData))
    return handle;

  closedir(dir);
  delete fh;
  return INVALID_HANDLE_VALUE;
}

BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData)
{
  if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE || !lpFindFileData) return FALSE;
  AppleFindHandle *fh = reinterpret_cast<AppleFindHandle *>(hFindFile);

  struct dirent *entry;
  while ((entry = readdir(fh->dir)) != nullptr)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if (!matchWildcard(fh->pattern.c_str(), entry->d_name))
      continue;

    memset(lpFindFileData, 0, sizeof(*lpFindFileData));
    strncpy(lpFindFileData->cFileName, entry->d_name, MAX_PATH - 1);
    lpFindFileData->cFileName[MAX_PATH - 1] = '\0';

    std::string fullPath = fh->directory + "/" + entry->d_name;
    struct stat st;
    if (stat(fullPath.c_str(), &st) == 0)
    {
      if (S_ISDIR(st.st_mode))
        lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
      else
        lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
      lpFindFileData->nFileSizeLow = static_cast<DWORD>(st.st_size & 0xFFFFFFFF);
      lpFindFileData->nFileSizeHigh = static_cast<DWORD>(st.st_size >> 32);
    }
    return TRUE;
  }
  return FALSE;
}

BOOL FindClose(HANDLE hFindFile)
{
  if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE) return FALSE;
  AppleFindHandle *fh = reinterpret_cast<AppleFindHandle *>(hFindFile);
  closedir(fh->dir);
  delete fh;
  return TRUE;
}

// ============================================================================
// Interlocked operations
// ============================================================================
LONG InterlockedCompareExchangeRelease(LONG volatile *destination, LONG exchange, LONG comparand)
{
  return __sync_val_compare_and_swap(destination, comparand, exchange);
}

LONG64 InterlockedCompareExchangeRelease64(LONG64 volatile *destination, LONG64 exchange, LONG64 comparand)
{
  return __sync_val_compare_and_swap(destination, comparand, exchange);
}

LONG InterlockedIncrement(LONG volatile *addend)
{
  return __sync_add_and_fetch(addend, 1);
}

LONG InterlockedDecrement(LONG volatile *addend)
{
  return __sync_sub_and_fetch(addend, 1);
}

LONG InterlockedExchange(LONG volatile *target, LONG value)
{
  return __sync_lock_test_and_set(target, value);
}

// ============================================================================
// Debug
// ============================================================================
void OutputDebugStringA(LPCSTR outputString)
{
  if (outputString != nullptr)
  {
    std::fputs(outputString, stderr);
  }
}

void OutputDebugStringW(LPCWSTR outputString)
{
  if (outputString != nullptr)
  {
    while (*outputString != L'\0')
    {
      std::fputc(static_cast<char>(*outputString), stderr);
      ++outputString;
    }
  }
}

void __debugbreak()
{
  fprintf(stderr, "[MCE] __debugbreak() hit (ignored)\n");
}

VOID DebugBreak(VOID)
{
  __debugbreak();
}

// ============================================================================
// String functions
// ============================================================================
int _wcsicmp(const wchar_t *dst, const wchar_t *src)
{
  if (dst == nullptr || src == nullptr)
  {
    return (dst == src) ? 0 : (dst == nullptr ? -1 : 1);
  }

  wchar_t left = 0;
  wchar_t right = 0;
  do
  {
    left = towlower(*dst++);
    right = towlower(*src++);
  }
  while (left != L'\0' && left == right);

  return static_cast<int>(left - right);
}

int _wcsnicmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
  if (!s1 || !s2) return s1 == s2 ? 0 : (s1 ? 1 : -1);
  for (size_t i = 0; i < n; i++)
  {
    wchar_t a = towlower(s1[i]);
    wchar_t b = towlower(s2[i]);
    if (a != b) return (a < b) ? -1 : 1;
    if (a == L'\0') return 0;
  }
  return 0;
}

int _stricmp(const char *s1, const char *s2)
{
  return strcasecmp(s1, s2);
}

int _strnicmp(const char *s1, const char *s2, size_t n)
{
  return strncasecmp(s1, s2, n);
}

wchar_t *_wcsdup(const wchar_t *s)
{
  if (!s) return nullptr;
  size_t len = wcslen(s) + 1;
  wchar_t *dup = static_cast<wchar_t *>(malloc(len * sizeof(wchar_t)));
  if (dup) wcscpy(dup, s);
  return dup;
}

unsigned long long _wcstoui64(const wchar_t *nptr, wchar_t **endptr, int base)
{
  return wcstoull(nptr, endptr, base);
}

#include "platform/platform.h"
#include "core/volume.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "core/util/tVector.h"

#include <SDL3/SDL.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

static Vector<StringTableEntry> sExcludedDirectories;

//-----------------------------------------------------------------------------
// Queries

bool Platform::isFile(const char* path)
{
   if (!path || !*path)
      return false;

   struct stat info;
   if (stat(path, &info) != 0)
      return false;

   return S_ISREG(info.st_mode);
}

bool Platform::isDirectory(const char* path)
{
   if (!path || !*path)
      return false;

   struct stat info;
   if (stat(path, &info) != 0)
      return false;

   return S_ISDIR(info.st_mode);
}

S32 Platform::getFileSize(const char* path)
{
   struct stat info;
   if (!path || !*path || stat(path, &info) != 0 || !S_ISREG(info.st_mode))
      return 0;

   return S32(info.st_size);
}

bool Platform::getFileTimes(const char* path, FileTime* createTime, FileTime* modifyTime)
{
   struct stat info;
   if (!path || !*path || stat(path, &info) != 0)
      return false;

   // POSIX has no creation time; st_ctime is inode change time.
   if (createTime)
      *createTime = FileTime(info.st_ctime);
   if (modifyTime)
      *modifyTime = FileTime(info.st_mtime);

   return true;
}

bool Platform::hasSubDirectory(const char* path)
{
   if (!path)
      return false;

   DIR* dir = opendir(path);
   if (!dir)
      return false;

   bool found = false;
   while (struct dirent* entry = readdir(dir))
   {
      if (dStrcmp(entry->d_name, ".") == 0 || dStrcmp(entry->d_name, "..") == 0)
         continue;

      char child[1024];
      dSprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
      if (Platform::isDirectory(child))
      {
         found = true;
         break;
      }
   }

   closedir(dir);
   return found;
}

//-----------------------------------------------------------------------------
// Enumeration

static bool isExcluded(const char* name)
{
   for (U32 i = 0; i < sExcludedDirectories.size(); ++i)
      if (dStrcmp(sExcludedDirectories[i], name) == 0)
         return true;
   return false;
}

bool Platform::dumpPath(const char* path, Vector<Platform::FileInfo>& fileVector, S32 recurseDepth)
{
   if (!path || !*path)
      return false;

   DIR* dir = opendir(path);
   if (!dir)
      return false;

   while (struct dirent* entry = readdir(dir))
   {
      if (dStrcmp(entry->d_name, ".") == 0 || dStrcmp(entry->d_name, "..") == 0)
         continue;

      char child[1024];
      dSprintf(child, sizeof(child), "%s/%s", path, entry->d_name);

      if (Platform::isDirectory(child))
      {
         if (recurseDepth != 0 && !isExcluded(entry->d_name))
            Platform::dumpPath(child, fileVector, recurseDepth - 1);
         continue;
      }

      if (!Platform::isFile(child))
         continue;

      fileVector.increment();
      Platform::FileInfo& info = fileVector.last();
      info.pFullPath = StringTable->insert(path);
      info.pFileName = StringTable->insert(entry->d_name);
      info.fileSize  = Platform::getFileSize(child);
   }

   closedir(dir);
   return true;
}

bool Platform::dumpDirectories(const char* path, Vector<StringTableEntry>& directoryVector,
                               S32 depth, bool noBasePath)
{
   if (!path || !*path)
      return false;

   if (!noBasePath)
      directoryVector.push_back(StringTable->insert(path));

   DIR* dir = opendir(path);
   if (!dir)
      return false;

   while (struct dirent* entry = readdir(dir))
   {
      if (dStrcmp(entry->d_name, ".") == 0 || dStrcmp(entry->d_name, "..") == 0)
         continue;
      if (isExcluded(entry->d_name))
         continue;

      char child[1024];
      dSprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
      if (!Platform::isDirectory(child))
         continue;

      directoryVector.push_back(StringTable->insert(child));

      if (depth != 0)
         Platform::dumpDirectories(child, directoryVector, depth - 1, true);
   }

   closedir(dir);
   return true;
}

//-----------------------------------------------------------------------------
// Locations

StringTableEntry Platform::getCurrentDirectory()
{
   char buffer[4096];
   if (!getcwd(buffer, sizeof(buffer)))
      return StringTable->insert("");

   return StringTable->insert(buffer);
}

bool Platform::setCurrentDirectory(StringTableEntry newDir)
{
   return chdir(newDir) == 0;
}

StringTableEntry Platform::getExecutablePath()
{
   static StringTableEntry path = NULL;
   if (!path)
   {
      const char* base = SDL_GetBasePath();
      path = StringTable->insert(base ? base : ".");
   }
   return path;
}

StringTableEntry Platform::getExecutableName()
{
   static StringTableEntry name = NULL;
   if (!name)
   {
      char buffer[4096];
      const ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
      if (len > 0)
      {
         buffer[len] = '\0';
         name = StringTable->insert(buffer);
      }
      else
      {
         name = StringTable->insert("");
      }
   }
   return name;
}

StringTableEntry Platform::getUserHomeDirectory()
{
   const char* home = getenv("HOME");
   return StringTable->insert(home ? home : ".");
}

StringTableEntry Platform::getUserDataDirectory()
{
   const char* xdg = getenv("XDG_DATA_HOME");
   if (xdg && *xdg)
      return StringTable->insert(xdg);

   const char* home = getenv("HOME");
   if (!home)
      return StringTable->insert(".");

   char buffer[4096];
   dSprintf(buffer, sizeof(buffer), "%s/.local/share", home);
   return StringTable->insert(buffer);
}

//-----------------------------------------------------------------------------

bool Platform::createPath(const char* path)
{
   if (!path || !*path)
      return false;

   const char* lastSlash = dStrrchr(path, '/');
   if (!lastSlash)
      return true;

   if (lastSlash == path)
      return true;

   char dirs[4096];
   const U32 len = lastSlash - path;
   if (len >= sizeof(dirs))
      return false;

   dMemcpy(dirs, path, len);
   dirs[len] = '\0';

   return SDL_CreateDirectory(dirs);
}

void Platform::openFolder(const char* path)
{
   char url[4096];
   dSprintf(url, sizeof(url), "file://%s", path);
   SDL_OpenURL(url);
}

void Platform::openFile(const char* path)
{
   openFolder(path);
}

bool dPathCopy(const char* fromName, const char* toName, bool nooverwrite)
{
   if (!fromName || !toName)
      return false;

   if (nooverwrite && Platform::isFile(toName))
      return false;

   if (!Platform::createPath(toName))
      return false;

   return SDL_CopyFile(fromName, toName);
}

//-----------------------------------------------------------------------------

bool dFileDelete(const char* name)
{
   if (!name || !*name)
      return false;

   if (Platform::isDirectory(name))
      return rmdir(name) == 0;

   return unlink(name) == 0;
}

bool Torque::FS::VerifyWriteAccess(const Torque::Path& path)
{
   const String file = path.getFullPath();
   if (file.isEmpty())
      return false;

   if (access(file.c_str(), F_OK) != 0)
      return true;   // does not exist yet; the parent decides

   return access(file.c_str(), W_OK) == 0;
}

#include "../include/io/filesystem.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <io.h>
#define PATH_SEPARATOR '\\'
#else
#include <dirent.h>
#include <unistd.h>
#define PATH_SEPARATOR '/'
#endif

static bool isRegularFile(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
#if defined(_WIN32) || defined(_WIN64)
        return (st.st_mode & S_IFREG) != 0;
#else
        return S_ISREG(st.st_mode);
#endif
    }
    return false;
}

static bool isDirectory(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
#if defined(_WIN32) || defined(_WIN64)
        return (st.st_mode & S_IFDIR) != 0;
#else
        return S_ISDIR(st.st_mode);
#endif
    }
    return false;
}

namespace base {
namespace io {

bool FileSystem::fileExists(const std::string& path) {
    return isRegularFile(path.c_str());
}

bool FileSystem::createFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "w");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

bool FileSystem::deleteFile(const std::string& path) {
    return remove(path.c_str()) == 0;
}

bool FileSystem::renameFile(const std::string& oldPath, const std::string& newPath) {
    return rename(oldPath.c_str(), newPath.c_str()) == 0;
}

bool FileSystem::copyFile(const std::string& srcPath, const std::string& destPath) {
    FILE* src = fopen(srcPath.c_str(), "rb");
    if (!src) return false;
    FILE* dest = fopen(destPath.c_str(), "wb");
    if (!dest) {
        fclose(src);
        return false;
    }
    char buffer[8192];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }
    fclose(src);
    fclose(dest);
    return true;
}

bool FileSystem::directoryExists(const std::string& path) {
    return isDirectory(path.c_str());
}

bool FileSystem::createDirectory(const std::string& path) {
#if defined(_WIN32) || defined(_WIN64)
    return CreateDirectoryA(path.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

bool FileSystem::createDirectories(const std::string& path) {
    if (path.empty()) return false;
    if (directoryExists(path)) return true;

    std::string currentPath = path;
#if defined(_WIN32) || defined(_WIN64)
    if (currentPath.size() >= 2 && currentPath[1] == ':') {
        currentPath = currentPath.substr(2);
    }
    if (!currentPath.empty() && (currentPath[0] == '\\' || currentPath[0] == '/')) {
        currentPath = currentPath.substr(1);
    }
#else
    if (!currentPath.empty() && currentPath[0] == '/') {
        currentPath = currentPath.substr(1);
    }
#endif

    size_t pos = 0;
    std::string builtPath;
#if defined(_WIN32) || defined(_WIN64)
    while ((pos = currentPath.find('\\', pos)) != std::string::npos ||
           (pos = currentPath.find('/', pos)) != std::string::npos) {
        builtPath = path.substr(0, 2) + currentPath.substr(0, pos);
        if (!directoryExists(builtPath)) {
            if (!createDirectory(builtPath)) return false;
        }
        ++pos;
    }
#else
    while ((pos = currentPath.find('/', pos)) != std::string::npos) {
        builtPath = "/" + currentPath.substr(0, pos);
        if (!directoryExists(builtPath)) {
            if (!createDirectory(builtPath)) return false;
        }
        ++pos;
    }
#endif
    return createDirectory(path);
}

bool FileSystem::deleteDirectory(const std::string& path) {
#if defined(_WIN32) || defined(_WIN64)
    return RemoveDirectoryA(path.c_str()) != 0;
#else
    return rmdir(path.c_str()) == 0;
#endif
}

std::vector<std::string> FileSystem::listDirectory(const std::string& path, bool recursive) {
    std::vector<std::string> result;
#if defined(_WIN32) || defined(_WIN64)
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return result;

    do {
        std::string name = findData.cFileName;
        if (name != "." && name != "..") {
            std::string fullPath = joinPath(path, name);
            result.push_back(name);
            if (recursive && directoryExists(fullPath)) {
                auto subDir = listDirectory(fullPath, true);
                for (const auto& subName : subDir) {
                    result.push_back(joinPath(name, subName));
                }
            }
        }
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);
#else
    DIR* dir = opendir(path.c_str());
    if (!dir) return result;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            std::string fullPath = joinPath(path, name);
            result.push_back(name);
            if (recursive && directoryExists(fullPath)) {
                auto subDir = listDirectory(fullPath, true);
                for (const auto& subName : subDir) {
                    result.push_back(joinPath(name, subName));
                }
            }
        }
    }
    closedir(dir);
#endif
    return result;
}

bool FileSystem::setPermissions(const std::string& path, int permissions) {
#if defined(_WIN32) || defined(_WIN64)
    return true;
#else
    return chmod(path.c_str(), permissions) == 0;
#endif
}

int FileSystem::getPermissions(const std::string& path) {
#if defined(_WIN32) || defined(_WIN64)
    return 0755;
#else
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_mode & 0777;
    }
    return 0;
#endif
}

std::string FileSystem::getAbsolutePath(const std::string& path) {
#if defined(_WIN32) || defined(_WIN64)
    char fullPath[MAX_PATH];
    if (GetFullPathNameA(path.c_str(), MAX_PATH, fullPath, NULL) > 0) {
        return std::string(fullPath);
    }
    return path;
#else
    char fullPath[4096];
    if (realpath(path.c_str(), fullPath) != NULL) {
        return std::string(fullPath);
    }
    return path;
#endif
}

std::string FileSystem::getFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

std::string FileSystem::getFileExtension(const std::string& path) {
    std::string filename = getFileName(path);
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos) {
        return "";
    }
    return filename.substr(pos);
}

std::string FileSystem::getDirectoryName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return ".";
    }
    return path.substr(0, pos);
}

std::string FileSystem::getCurrentDirectory() {
#if defined(_WIN32) || defined(_WIN64)
    char buffer[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, buffer) > 0) {
        return std::string(buffer);
    }
    return "";
#else
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        return std::string(buffer);
    }
    return "";
#endif
}

bool FileSystem::isAbsolutePath(const std::string& path) {
#if defined(_WIN32) || defined(_WIN64)
    return path.size() >= 2 && path[1] == ':';
#else
    return !path.empty() && path[0] == '/';
#endif
}

bool FileSystem::isRelativePath(const std::string& path) {
    return !isAbsolutePath(path);
}

std::string FileSystem::joinPath(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    char lastChar = path1[path1.size() - 1];
    if (lastChar == '/' || lastChar == '\\') {
        return path1 + path2;
    }
    return path1 + PATH_SEPARATOR + path2;
}

FileSystem::File::File(const std::string& path, const std::string& mode)
    : m_path(path), m_mode(mode), m_file(nullptr) {}

FileSystem::File::~File() {
    if (m_file) {
        fclose((FILE*)m_file);
    }
}

bool FileSystem::File::open() {
    m_file = fopen(m_path.c_str(), m_mode.c_str());
    return m_file != nullptr;
}

void FileSystem::File::close() {
    if (m_file) {
        fclose((FILE*)m_file);
        m_file = nullptr;
    }
}

bool FileSystem::File::isOpen() const {
    return m_file != nullptr;
}

bool FileSystem::File::isEndOfFile() const {
    if (!m_file) return true;
    return feof((FILE*)m_file) != 0;
}

size_t FileSystem::File::read(void* buffer, size_t size) {
    if (!m_file) return 0;
    return fread(buffer, 1, size, (FILE*)m_file);
}

size_t FileSystem::File::write(const void* buffer, size_t size) {
    if (!m_file) return 0;
    return fwrite(buffer, 1, size, (FILE*)m_file);
}

size_t FileSystem::File::seek(long offset, int whence) {
    if (!m_file) return 0;
    return fseek((FILE*)m_file, offset, whence);
}

long FileSystem::File::tell() const {
    if (!m_file) return 0;
    return ftell((FILE*)m_file);
}

size_t FileSystem::File::size() const {
    if (!m_file) return 0;
    long current = ftell((FILE*)m_file);
    fseek((FILE*)m_file, 0, SEEK_END);
    long fileSize = ftell((FILE*)m_file);
    fseek((FILE*)m_file, current, SEEK_SET);
    return fileSize;
}

void FileSystem::File::flush() {
    if (m_file) {
        fflush((FILE*)m_file);
    }
}

bool FileSystem::File::readLine(std::string& line) {
    if (!m_file) return false;
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), (FILE*)m_file) != NULL) {
        line = buffer;
        if (!line.empty() && line[line.size() - 1] == '\n') {
            line = line.substr(0, line.size() - 1);
        }
        return true;
    }
    return false;
}

bool FileSystem::File::writeLine(const std::string& line) {
    if (!m_file) return false;
    return fputs((line + "\n").c_str(), (FILE*)m_file) >= 0;
}

} // namespace io
} // namespace base
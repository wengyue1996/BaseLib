#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <vector>

namespace base {
namespace io {

class FileSystem {
public:
    static bool fileExists(const std::string& path);
    static bool createFile(const std::string& path);
    static bool deleteFile(const std::string& path);
    static bool renameFile(const std::string& oldPath, const std::string& newPath);
    static bool copyFile(const std::string& srcPath, const std::string& destPath);

    static bool directoryExists(const std::string& path);
    static bool createDirectory(const std::string& path);
    static bool createDirectories(const std::string& path);
    static bool deleteDirectory(const std::string& path);
    static std::vector<std::string> listDirectory(const std::string& path, bool recursive = false);

    static bool setPermissions(const std::string& path, int permissions);
    static int getPermissions(const std::string& path);

    static std::string getAbsolutePath(const std::string& path);
    static std::string getFileName(const std::string& path);
    static std::string getFileExtension(const std::string& path);
    static std::string getDirectoryName(const std::string& path);
    static std::string getCurrentDirectory();

    static bool isAbsolutePath(const std::string& path);
    static bool isRelativePath(const std::string& path);
    static std::string joinPath(const std::string& path1, const std::string& path2);

    class File {
    public:
        File(const std::string& path, const std::string& mode);
        ~File();

        bool open();
        void close();
        bool isOpen() const;
        bool isEndOfFile() const;

        size_t read(void* buffer, size_t size);
        size_t write(const void* buffer, size_t size);
        size_t seek(long offset, int whence);
        long tell() const;
        size_t size() const;
        void flush();

        bool readLine(std::string& line);
        bool writeLine(const std::string& line);

    private:
        std::string m_path;
        std::string m_mode;
        void* m_file;
    };

private:
    static std::string getPlatformPath(const std::string& path);
    static void parsePath(const std::string& path, std::string& directory, std::string& filename);
};

} // namespace io
} // namespace base

#endif // FILESYSTEM_H
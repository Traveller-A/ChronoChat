#pragma once
#include <string>
namespace chronochat {
class FileUtils {
public:
    static bool ensureDirectory(const std::string& path);
};
}

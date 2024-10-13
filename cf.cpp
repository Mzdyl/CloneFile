#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/clonefile.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <filesystem>
#include <vector>
#include <signal.h>

namespace fs = std::filesystem;

// 用于打印错误信息
void print_error(const std::string& msg) {
    std::cerr << msg << ": " << strerror(errno) << " (errno: " << errno << ")" << std::endl;
}

// 使用 clonefile 函数复制文件
bool clone_file(const fs::path& source, const fs::path& target, uint32_t flags = 0) {
    int result = clonefile(source.c_str(), target.c_str(), flags);
    if (result != 0) {
        print_error("Error cloning file from " + source.string() + " to " + target.string());
        return false;
    }
    return true;
}


// 递归复制目录
bool copy_directory(const fs::path& source, const fs::path& target, bool preserve_permissions, bool backup) {
    try {
        for (const auto& entry : fs::directory_iterator(source)) {
            const auto& path = entry.path();
            fs::path target_path = target / path.filename();
            
            if (fs::is_directory(path)) {
                fs::create_directory(target_path);
                copy_directory(path, target_path, preserve_permissions, backup);
            } else {
                if (backup && fs::exists(target_path)) {
                    fs::copy_file(target_path, target_path.string() + "~", fs::copy_options::overwrite_existing);
                }
                if (!clone_file(path.string(), target_path.string())) {
                    return false;
                }
                // 保留权限
                if (preserve_permissions) {
                    struct stat file_stat;
                    stat(path.string().c_str(), &file_stat);
                    chmod(target_path.string().c_str(), file_stat.st_mode);
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        print_error(e.what());
        return false;
    }
    return true;
}

// 显示使用说明
void show_usage(const std::string& program_name) {
    std::cerr << "Usage: " << program_name << " [-a] [-b] [-f] [-i] [-R] [-p] [-u] <source> <target>" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -a  Archive mode (not implemented)" << std::endl;
    std::cerr << "  -b  Backup existing files" << std::endl;
    std::cerr << "  -f  Force overwrite" << std::endl;
    std::cerr << "  -i  Interactive mode" << std::endl;
    std::cerr << "  -R  Recursive copy" << std::endl;
    std::cerr << "  -p  Preserve file permissions" << std::endl;
    std::cerr << "  -u  Update only (not implemented)" << std::endl;
}

// 模拟 cp 命令的主函数
int main(int argc, char* argv[]) {
    if (argc < 3) {
        show_usage(argv[0]);
        return 1;
    }
    
    bool archive = false;
    bool backup = false;
    bool force = false;
    bool interactive = false;
    bool recursive = false;
    bool preserve_permissions = false;
    bool update = false;
    fs::path source, target;

    int i = 1;
    while (i < argc) {
        if (std::string(argv[i]) == "-a") {
            archive = true;
        } else if (std::string(argv[i]) == "-b") {
            backup = true;
        } else if (std::string(argv[i]) == "-f") {
            force = true;
        } else if (std::string(argv[i]) == "-i") {
            interactive = true;
        } else if (std::string(argv[i]) == "-R" || std::string(argv[i]) == "-r") {
            recursive = true;
        } else if (std::string(argv[i]) == "-p") {
            preserve_permissions = true;
        } else if (std::string(argv[i]) == "-u") {
            update = true;
        } else {
            if (source.empty()) {
                source = argv[i];
            } else if (target.empty()) {
                target = argv[i];
            }
        }
        i++;
    }

    // 检查源文件或目录是否存在
    if (!fs::exists(source)) {
        std::cerr << "Source does not exist: " << source << std::endl;
        return 1;
    }

    // 确保目标路径存在
    if (fs::exists(target)) {
        if (!fs::is_directory(target)) {
            if (interactive) {
                std::cout << "Overwrite " << target << "? (y/n) ";
                char response;
                std::cin >> response;
                if (response != 'y' && response != 'Y') {
                    std::cout << "Skipping " << target << std::endl;
                    return 0;
                }
            }
            if (backup) {
                fs::copy_file(target, target.string() + "~", fs::copy_options::overwrite_existing);
            }
            if (!force && fs::exists(target)) {
                std::cerr << "File exists: " << target << std::endl;
                return 1;
            }
        }
    } else {
        fs::create_directory(target);  // 只创建目录时
    }

    // 处理文件或目录
    if (fs::is_directory(source)) {
        if (recursive) {
            if (!copy_directory(source, target, preserve_permissions, backup)) {
                return 1; // 返回错误代码
            }
        } else {
            std::cerr << "Source is a directory. Use -R option for recursive copy." << std::endl;
            return 1;
        }
    } else {
        fs::path target_path = target;
        if (!clone_file(source.string(), target_path.string())) {
            return 1; // 返回错误代码
        }
        // 保留权限
        if (preserve_permissions) {
            struct stat file_stat;
            stat(source.c_str(), &file_stat);
            chmod(target_path.string().c_str(), file_stat.st_mode);
        }
    }

    std::cout << "Successfully copied from " << source << " to " << target << std::endl;
    return 0; // 成功
}
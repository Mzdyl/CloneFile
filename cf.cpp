#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/clonefile.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <filesystem>
#include <vector>
#include <signal.h>

namespace fs = std::filesystem;

// Global variable to control debug mode
bool debug_mode = false;

// Function to print error messages
void print_error(const std::string& msg) {
    std::cerr << msg << ": " << strerror(errno) << " (errno: " << errno << ")" << std::endl;
}

// Debug output function
void debug_print(const std::string& msg) {
    if (debug_mode) {
        std::cout << "[DEBUG] " << msg << std::endl;
    }
}

// Clone a file using clonefile function
bool clone_file(const fs::path& source, const fs::path& target, uint32_t flags = 0) {
    debug_print("Entering clone_file()");
    debug_print("Parameters: source = " + source.string() + ", target = " + target.string() + ", flags = " + std::to_string(flags));

    if (fs::exists(target)) {
        debug_print("Target exists: " + target.string());
        if (fs::is_directory(target)) {
            debug_print("Target is a directory, cannot clone to a directory.");
            errno = EEXIST; // Set errno to file exists
            return false;
        } else {
            debug_print("Target already exists and is not a directory: " + target.string());
            return false; // Don't proceed if the file already exists and is not a directory
        }
    } else {
        debug_print("Target does not exist, ready to clone.");
    }

    debug_print("Cloning file from " + source.string() + " to " + target.string());
    int result = clonefile(source.c_str(), target.c_str(), flags);
    if (result != 0) {
        print_error("Error cloning file from " + source.string() + " to " + target.string());
        return false;
    }
    debug_print("Successfully cloned " + source.string() + " to " + target.string());
    return true;
}

// Compare last modification times of two files
bool is_newer(const fs::path& source, const fs::path& target) {
    debug_print("Entering is_newer()");
    debug_print("Parameters: source = " + source.string() + ", target = " + target.string());
    return fs::last_write_time(source) > fs::last_write_time(target);
}

// Recursively copy a directory
bool copy_directory(const fs::path& source, const fs::path& target, bool preserve_permissions, bool backup, bool update) {
    debug_print("Entering copy_directory()");
    debug_print("Parameters: source = " + source.string() + ", target = " + target.string() +
                ", preserve_permissions = " + std::to_string(preserve_permissions) +
                ", backup = " + std::to_string(backup) +
                ", update = " + std::to_string(update));

    try {
        debug_print("Creating directory iterator for source: " + source.string());
        for (const auto& entry : fs::directory_iterator(source)) {
            const auto& path = entry.path();
            fs::path target_path = target / path.filename();

            if (fs::is_directory(path)) {
                debug_print("Found directory: " + path.string());
                debug_print("Creating directory: " + target_path.string());
                fs::create_directory(target_path);
                if (!copy_directory(path, target_path, preserve_permissions, backup, update)) {
                    return false;
                }
            } else {
                debug_print("Found file: " + path.string());

                // Update mode: only copy if source file is newer
                if (update && fs::exists(target_path) && !is_newer(path, target_path)) {
                    debug_print("Skipping file (not newer): " + path.string());
                    continue;
                }

                if (backup && fs::exists(target_path)) {
                    debug_print("Backing up file: " + target_path.string());
                    fs::copy_file(target_path, target_path.string() + "~", fs::copy_options::overwrite_existing);
                }

                if (!clone_file(path, target_path)) {
                    return false;
                }

                // Preserve permissions if required
                if (preserve_permissions) {
                    debug_print("Preserving permissions for: " + target_path.string());
                    fs::permissions(target_path, fs::status(path).permissions());
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        print_error(e.what());
        return false;
    }
    return true;
}

// Show usage instructions
void show_usage(const std::string& program_name) {
    std::cerr << "Usage: " << program_name << " [-a] [-b] [-f] [-i] [-R] [-p] [-u] [-d] <source> <target>" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -a  Archive mode (recursive and preserve permissions)" << std::endl;
    std::cerr << "  -b  Backup existing files" << std::endl;
    std::cerr << "  -f  Force overwrite" << std::endl;
    std::cerr << "  -i  Interactive mode" << std::endl;
    std::cerr << "  -R  Recursive copy" << std::endl;
    std::cerr << "  -p  Preserve file permissions" << std::endl;
    std::cerr << "  -u  Update only copy newer files" << std::endl;
    std::cerr << "  -d  Enable debug mode" << std::endl;
}

// Main function simulating cp command
int main(int argc, char* argv[]) {
    debug_print("Entering main()");
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
    debug_print("Parsing command-line arguments");
    while (i < argc) {
        std::string arg = argv[i];
        if (arg == "-a") {
            archive = true;
            recursive = true;
            preserve_permissions = true;
            debug_print("Option set: archive mode");
        } else if (arg == "-b") {
            backup = true;
            debug_print("Option set: backup");
        } else if (arg == "-f") {
            force = true;
            debug_print("Option set: force overwrite");
        } else if (arg == "-i") {
            interactive = true;
            debug_print("Option set: interactive mode");
        } else if (arg == "-R" || arg == "-r") {
            recursive = true;
            debug_print("Option set: recursive copy");
        } else if (arg == "-p") {
            preserve_permissions = true;
            debug_print("Option set: preserve permissions");
        } else if (arg == "-u") {
            update = true;
            debug_print("Option set: update mode");
        } else if (arg == "-d") {
            debug_mode = true; // Enable debug mode
            debug_print("Debug mode enabled");
        } else {
            if (source.empty()) {
                source = argv[i];
                debug_print("Source set to: " + source.string());
            } else if (target.empty()) {
                target = argv[i];
                debug_print("Target set to: " + target.string());
            } else {
                std::cerr << "Unexpected argument: " << arg << std::endl;
                show_usage(argv[0]);
                return 1;
            }
        }
        i++;
    }

    // Check if source file or directory exists
    debug_print("Checking if source exists");
    if (!fs::exists(source)) {
        std::cerr << "Source does not exist: " << source << std::endl;
        return 1;
    }

    // Ensure the target path exists or create it
    debug_print("Checking if target exists");
    if (fs::exists(target)) {
        debug_print("Target exists: " + target.string());
        if (!fs::is_directory(target)) {
            debug_print("Target is a file");
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
                debug_print("Backing up existing target file");
                fs::copy_file(target, target.string() + "~", fs::copy_options::overwrite_existing);
            }
            if (!force && fs::exists(target)) {
                std::cerr << "File exists: " << target << std::endl;
                return 1;
            }
        } else {
            debug_print("Target is a directory");
        }
    } else {
        // If target is not found and source is a directory, create target directory
        if (fs::is_directory(source)) {
            debug_print("Creating target directory: " + target.string());
            fs::create_directory(target);
        }
    }

    // Process file or directory
    if (fs::is_directory(source)) {
        debug_print("Source is a directory");
        if (recursive) {
            debug_print("Recursive copy enabled");
            if (!copy_directory(source, target, preserve_permissions, backup, update)) {
                return 1; // Return error code
            }
        } else {
            std::cerr << "Source is a directory. Use -R option for recursive copy." << std::endl;
            return 1;
        }
    } else {
        debug_print("Source is a file");
        // Handle single file
        fs::path target_path = target;

        // If target is a directory, copy into directory
        if (fs::is_directory(target)) {
            target_path = target / source.filename();
            debug_print("Adjusted target path to: " + target_path.string());
        }

        // If target exists and is a file, prompt user or handle based on flags
        if (fs::exists(target_path) && !force) {
            debug_print("Target file exists: " + target_path.string());
            if (interactive) {
                std::cout << "Overwrite " << target_path << "? (y/n) ";
                char response;
                std::cin >> response;
                if (response != 'y' && response != 'Y') {
                    std::cout << "Skipping " << target_path << std::endl;
                    return 0;
                }
            }
            if (backup) {
                debug_print("Backing up existing target file");
                fs::copy_file(target_path, target_path.string() + "~", fs::copy_options::overwrite_existing);
            }
        }

        if (!clone_file(source, target_path)) {
            return 1; // Return error code
        }

        // Preserve permissions
        if (preserve_permissions) {
            debug_print("Preserving permissions for: " + target_path.string());
            fs::permissions(target_path, fs::status(source).permissions());
        }
    }

    std::cout << "Successfully copied from " << source << " to " << target << std::endl;
    return 0;
}
# File Clone Utility

A simple C++ utility to clone files and directories on macOS using the `clonefile` system call. This tool mimics the behavior of the `cp` command with options for backup, interactive prompts, and permission preservation.

## Features

- Clone files and directories efficiently using the `clonefile` system call.
- Support for recursive directory copying with the `-R` option.
- Backup existing files with the `-b` option.
- Interactive overwrite prompts with the `-i` option.
- Preserve file permissions with the `-p` option.
- Force overwrite existing files with the `-f` option.

## Requirements

- macOS 10.12 or later
- A C++ compiler that supports C++17 or later

## Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/Mzdyl/CloneFile.git
   cd CloneFile
   ```
2.	Compile the source code:
   ```bash
   g++ -o cf cf.cpp -std=c++17 
   ```


Usage

The utility can be run from the command line as follows:

./cf [-a] [-b] [-f] [-i] [-R] [-p] [-u] <source> <target>

Options:

	•	-a: Archive mode (recursive and preserve permissions).
	•	-b: Backup existing files before overwriting.
	•	-f: Force overwrite without prompting.
	•	-i: Prompt before overwriting existing files.
	•	-R: Recursively copy directories.
	•	-p: Preserve file permissions.
	•	-u: Update only.
	•	-d: Use Debug Mode.

Examples:

1.	Clone a single file:

	`./cf source.txt target.txt`


2.	Recursively clone a directory:

	`./cf -R source_dir/ target_dir/`


3.	Backup existing files:

	`./cf -b source.txt target.txt`


4.	Interactive mode:

	`./cf -i source.txt target.txt`



Contributing

Feel free to submit issues or pull requests if you find bugs or want to add new features.

# 🔓 7z-pwd-finder - Archive Password Recovery  
> Brute-force encrypted archives with multiple formats support

[![License](https://img.shields.io/badge/License-GPL--3.0-red)](LICENSE)
[![Build](https://img.shields.io/badge/Build-CMake-success)](https://cmake.org)

## 🌟 Supported Formats
Zip | 7z | BZip2 | Xz | Tar | GZip

## 🔑 Password Modes
- **Digit Brute-force** (0-9)
- **Alpha Combinations** (a-z/A-Z)
- **Special Characters** (@#!$% etc.)
- **Mixed Mode** (Custom charset)
- **Split Archive Support** (Multi-volume handling)

## 🛠️ Quick Start
```bash
# Build project
cmake -S . -B build
cmake --build build

# Usage examples
7z-pwd-finder <Compress type>(Zip, BZip2, 7z, Xz, Tar, GZip)
              <[Number]+[Letter]+[SpecialCharacters]>
              <min lenth>
              <max lenth>
              [file1, file2...]
7z-pwd-finder 7z Number+Letter+SpecialCharacters 1 10 hello.7z.001 hello.7z.002
```

> 📌 **注意事项**：仅限合法授权使用  
> ⚠️ **Note**: For authorized use only

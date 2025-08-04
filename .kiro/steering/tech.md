# Technology Stack

## Core Technologies
- **Language**: C++ (C++98/03 compatible)
- **Build System**: GNU Make with simple Makefile
- **Primary Dependency**: libusb-0.1 (legacy version, not libusb-1.0)
- **Compiler**: GCC (any recent version)
- **Platform**: POSIX-compliant systems (Linux primary target)

## Dependencies
- libusb-0.1 development headers (`libusb-dev` on Ubuntu/Debian)
- Standard C++ library with STL containers (vector, list, map, string)
- POSIX system calls for file operations and timing

## Build Commands

### Prerequisites Installation
```bash
# Ubuntu/Debian
sudo apt-get install libusb-dev

# macOS (requires Command Line Tools)
# Install from Apple Developer pages first
```

### Compilation
```bash
# Build the main executable
make

# Check for libusb-config availability
make check
```

### Usage
```bash
# Interactive mode
./wmp_manager

# Playlist mode (replace existing content)
./wmp_manager playlist.m3u

# Playlist mode with randomization
./wmp_manager -r playlist.m3u

# Add to existing content
./wmp_manager -a playlist.m3u
```

## Architecture Notes
- Single-threaded synchronous design
- Direct USB communication via libusb vendor messages
- Memory management with malloc/free (no smart pointers)
- Global state variables for MMC/TOC management
- Text-based user interface with stdin/stdout
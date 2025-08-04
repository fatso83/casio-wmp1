# Project Structure

## Root Directory Layout
```
├── README.md                    # Main project documentation
├── src/                         # Source code directory
├── MMPORT/                      # Original OS/2 REXX implementation
├── assets/                      # Binary assets and firmware
├── original_documentation/      # Historical documentation
└── .kiro/steering/             # AI assistant steering rules
```

## Source Code Organization (`src/`)
- **casio_wmp.cpp** - Main application logic, UI, and file operations
- **usb_layer.cpp** - USB communication abstraction layer  
- **usb_layer.h** - USB function declarations and interface
- **Makefile** - Simple build configuration
- **README.md** - Build and installation instructions
- **libusb.md** - libusb compilation notes

## Key Modules in casio_wmp.cpp
- **MMC Operations**: `MMC_Read()`, `MMC_Write()` - Low-level memory card access
- **TOC Management**: `TOC_*()` functions - Table of contents parsing/generation
- **File Operations**: `add_file()`, `get_file()`, `delete_file()` - User file management
- **Playlist Processing**: `process_playlist()` - M3U playlist handling
- **Interactive UI**: `show_main_menu()` - Text-based user interface

## Historical Code (`MMPORT/`)
- Original OS/2 REXX implementation by Martin Kiewitz
- USB drivers and system-level components
- Reference implementation for protocol understanding
- Not part of current build process

## Assets Directory
- `casio_wap.zip` - Original Windows 98 software
- `wmp1_org_firmware.dat` - Original firmware backup
- `libusb-0.1.12.tar.gz` - Legacy libusb source
- Product images and documentation

## Coding Conventions
- Global variables prefixed with `MMC_` for device state
- Function names use underscore_case
- STL containers for dynamic data (vectors, maps, strings)
- C-style error handling with return codes
- Minimal abstraction - direct hardware communication
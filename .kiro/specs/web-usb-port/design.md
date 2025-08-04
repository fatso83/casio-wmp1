# Design Document

## Overview

The web-based Casio WMP-1 Manager will be a single-page application that directly translates the existing C++ USB communication logic to JavaScript using the WebUSB API. The design prioritizes simplicity and direct porting of the proven USB protocol implementation.

## Architecture

### Single HTML File Approach
- One HTML file containing HTML, CSS, and JavaScript
- No build process or external dependencies
- Direct WebUSB API usage without frameworks
- Inline styling for simplicity

### Core Components
1. **USB Communication Layer** - Direct port of usb_layer.cpp functions
2. **MMC Operations** - JavaScript versions of MMC_Read/MMC_Write
3. **TOC Management** - File table parsing and generation
4. **Simple UI** - Basic file list and upload/delete buttons

## Components and Interfaces

### WebUSB Communication Layer
```javascript
class CasioUSB {
  async connect()           // Find and connect to device
  async vendorMessageOut()  // Port of usb_vendor_message_out
  async vendorMessageIn()   // Port of usb_vendor_message_in  
  async bulkRead()         // Port of usb_bulk_read
  async bulkWrite()        // Port of usb_bulk_write
}
```

### MMC Operations
```javascript
class MMCManager {
  async read(offset, length)     // Port of MMC_Read
  async write(offset, data)      // Port of MMC_Write
  async readTOC()               // Port of TOC_ReadFromWatch
  async writeTOC()              // Port of TOC_WriteToWatch
}
```

### File Management
```javascript
class FileManager {
  async listFiles()        // Parse TOC and return file list
  async uploadFile(file)   // Add file to watch
  async deleteFile(index)  // Remove file from watch
  stripID3v2(arrayBuffer)  // Remove ID3v2 tags
}
```

### UI Controller
```javascript
class WatchUI {
  async connectWatch()     // Handle device connection
  refreshFileList()       // Update displayed files
  handleFileUpload()      // Process file selection
  handleFileDelete()      // Process file deletion
}
```

## Data Models

### Device State
```javascript
const deviceState = {
  connected: false,
  device: null,
  totalSize: 32047104,    // 32MB MMC size
  trackCount: 0,
  tracks: [],             // Array of track objects
  usedSpace: new Map(),   // Offset -> size mapping
  freeSpace: 0,
  biggestFreeBlock: 0
};
```

### Track Object
```javascript
const track = {
  offset: 0,        // Position on MMC
  size: 0,          // File size in bytes
  title: "",        // Track title (32 chars max)
  artist: "",       // Artist name (32 chars max)
  album: ""         // Album name (32 chars max)
};
```

### TOC Structure
- Direct port of the C++ TOC format
- 16896 byte table of contents
- 512-byte header + track entries
- Each track entry is 128 bytes

## Error Handling

### Connection Errors
- WebUSB not supported: Show browser compatibility message
- Device not found: Clear instructions for connecting watch
- Permission denied: Explain user gesture requirement

### File Operation Errors
- Upload failures: Display specific error and retry option
- Insufficient space: Show available space and file size
- Invalid files: Warning but allow upload (like original)

### USB Communication Errors
- Timeout errors: Retry with exponential backoff
- Device disconnection: Clear UI state and show reconnect option
- Protocol errors: Log details and show generic error message

## Testing Strategy

### Manual Testing Approach
1. **Device Connection Testing**
   - Test with watch connected/disconnected
   - Test permission flow in Chrome
   - Verify device detection and initialization

2. **File Operations Testing**
   - Upload various MP3 files with different sizes
   - Test ID3v2 tag stripping functionality
   - Delete files and verify TOC updates
   - Test space management and allocation

3. **Browser Compatibility Testing**
   - Test in Chrome (primary target)
   - Verify HTTPS requirement compliance
   - Test WebUSB feature detection

4. **Error Scenario Testing**
   - Disconnect device during operations
   - Upload files larger than available space
   - Test with corrupted or invalid files

### Protocol Validation
- Compare USB traffic with original C++ version using USB sniffing tools
- Verify byte-for-byte TOC compatibility
- Test with multiple file upload/delete cycles

## Implementation Notes

### WebUSB Constraints
- Requires HTTPS (or localhost for development)
- Chrome/Edge only (Firefox doesn't support WebUSB)
- User gesture required for device access
- Limited to specific USB device classes

### Direct Protocol Port
- Maintain exact same vendor message sequences
- Keep 512-byte alignment for all operations
- Use same device IDs (0x07CF, 0x3801)
- Preserve timing delays where needed

### Memory Management
- Use ArrayBuffer for binary data operations
- Implement proper cleanup for large file operations
- Handle browser memory limitations gracefully

### File Format Handling
- Support same MP3 bitrate restrictions (96-128kbps, 44.1kHz)
- Implement ID3v2 detection and stripping
- Handle filename parsing for artist/title extraction
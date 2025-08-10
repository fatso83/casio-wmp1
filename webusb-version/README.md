Casio WMP 1 WebUSB Manager 🚀
================================

### How to Use the WebUSB App

1. **Open the web application**: Open `casio-wmp-manager.html` in a modern web browser (Chrome, Edge, or Opera - WebUSB is not supported in Firefox or Safari)

2. **Connect your watch**: 
   - Connect your Casio WMP-1 to your computer via USB
   - Click the "Connect to Watch" button in the web app
   - Select your watch from the device list when prompted

3. **Manage files**:
   - **Upload MP3s**: Drag and drop MP3 files onto the upload area or click "Choose MP3 Files"
   - **Download files**: Click the download button next to any file on the watch
   - **Delete files**: Click the delete button next to files you want to remove
   - **View file info**: See file names, sizes, and details in the file list

### Requirements

- **Browser**: Chrome 61+, Edge 79+, or Opera 48+ (WebUSB support required)
- **USB**: Your Casio WMP-1 watch connected via USB cable
- **Files**: MP3 files at 96-128kbps, 44.1kHz (the app will validate compatibility)

### Running the Web App

Webusb only works over https or from localhost:

# Option 1: Serve locally (recommended for security)
## Using Python 3
```
python -m http.server 8000
```
Then open http://localhost:8000/casio-wmp-manager.html

# Option 2 - hosted version
Access it at [wmp1.simenf.com](https://wmp1.simenf.com) hosted with cloudflare pages.

# Option 3 - 
Host it yourself somewhere.


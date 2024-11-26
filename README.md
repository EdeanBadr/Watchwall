
# Watchwall: Fixed-Layout Multi-Camera Streaming and Connection Manager

**Watchwall** is a server-client application designed for managing multi-camera streaming with a fixed layout. The application ensures robust socket management and delivers an intuitive user experience tailored to setups requiring a specific number of streams.

## Key Features
- **Configurable Number of Streams**: Specify the exact number of cameras or clients at startup. The layout is fixed based on this number and cannot be adjusted after the server is launched.
- **Connection Control**: 
  - Accepts only the specified number of clients.
  - Displays "No Signal" placeholders in the UI until clients connect.
  - Refuses additional connections if the client limit is reached, resuming only when an active client disconnects.
- **Seamless Stream Replacement**: Automatically replaces placeholders with live streams when clients connect, maintaining a professional and polished look.
- **Robust Networking**: Utilizes Qt's multithreaded architecture for efficient and reliable socket management.
- **Enhanced User Interface**: Features a clean and intuitive design optimized for monitoring environments.

## Technologies Used
- **Qt Framework**: For UI, multithreading, and socket communication.
- **C++**: Ensures high-performance processing.
- **FFmpeg**: Handles video stream decoding and integration.

## How to Run
1. Clone the repository:
   ```bash
   git clone https://github.com/EdeanBadr/Watchwall.git
   ```
2. Build the project using CMake:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```
3. Launch the server, specifying the number of clients (e.g., 4):
   ```bash
   ./watchwall --clients 4
   ```
4. Connect clients to the server:
   ```bash
   ./client
   ```

## Use Cases
- Multi-camera surveillance setups with predefined layouts.
- Streaming applications requiring precise client count management.
- Scenarios where connection limits and reliable stream handling are critical.
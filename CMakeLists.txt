cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(StreamAndVideoProcessing VERSION 1.0 LANGUAGES CXX)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(SANITIZE_FLAGS "-fsanitize=address -fno-omit-frame-pointer -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_FLAGS}")
    set(SANITIZE_FLAGS "-fsanitize=address,leak -fno-omit-frame-pointer -g")

endif()

# Enable automoc for Qt
set(CMAKE_AUTOMOC ON)

# Find the Qt5 package with necessary modules
find_package(Qt5 COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets REQUIRED)
find_package(Threads REQUIRED)

# Find FFmpeg libraries using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)
include_directories(${FFMPEG_INCLUDE_DIRS})

# Common headers for both executables
set(HEADERS
    server/ReceptionWorker.h
    server/DecodingWorker.h
    server/RenderingWorker.h
    server/ConnectionWorker.h
    server/AcceptWorker.h
)

# Wrap Qt headers with moc
qt5_wrap_cpp(MOC_SOURCES ${HEADERS})

# ==============================
# Executable 1: watchwall (QT App)
# ==============================

# Specify the source files for the first executable
set(STREAM_SOURCES server/server.cpp)

# Add the executable for watchwall
add_executable(watchwall ${STREAM_SOURCES} ${MOC_SOURCES})

# Link the necessary Qt libraries for server
target_link_libraries(watchwall Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia Qt5::MultimediaWidgets)

# Link FFmpeg libraries for server
target_link_libraries(watchwall ${FFMPEG_LIBRARIES})

# ================================
# Executable 2: client
# ================================

# Specify the source files for the second executable
set(VIDEO_PROCESSING_SOURCES client/client.cpp )

# Add the executable for the client
add_executable(client ${VIDEO_PROCESSING_SOURCES})

# Link FFmpeg libraries for VideoProcessingApp
target_link_libraries(client ${FFMPEG_LIBRARIES})


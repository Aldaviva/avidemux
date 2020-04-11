🎬 Avidemux
========

Avidemux is a simple platform video editor for Linux, Windows and Mac OS X.

## Fork
This fork solves window resizing and video zooming issues in the official, upstream Avidemux project.

### Issues fixed
- When opening a file, don't resize the window to the dimensions of the video, because you may have already set your desired window size.
- When opening a file, automatically zoom the video once to fit in the existing window dimensions, because if there is extra space in your large window, you'll want to use it, and if you have a small window, you don't want the video to be cropped.
- When closing a file, don't resize the window to the smallest possible size, because you may have already set your desired window size, and you want to maintain that size in the next file you open in the same window.

## Compiling

### Compiling a Windows executable using a Linux build machine

*These instructions were tested on Debian 10.3.*

*Full instructions are available from [Avidemux](https://github.com/Aldaviva/avidemux/blob/no_auto_resize_window/cross-compiling.txt) and [MXE](https://mxe.cc/#requirements) (a cross-compiler environment that lets you build Windows executables using Linux).*

1. Install packages.

    `sudo apt install autoconf automake autopoint bash bison bzip2 cmake flex g++ g++-multilib gettext git gperf intltool libc6-dev-i386 libgdk-pixbuf2.0-dev libltdl-dev libssl-dev libtool-bin libxml-parser-perl lzip make openssl p7zip-full patch perl pkg-config python ruby sed unzip wget xz-utils yasm`

1. Install MXE.
    ```bash
    sudo mkdir /opt/mxe
    sudo chown $(whoami): /opt/mxe
    git clone https://github.com/mxe/mxe.git /opt/mxe
    ```

1. Configure MXE.
    ```bash
    echo "MXE_TARGETS :=  i686-w64-mingw32.shared x86_64-w64-mingw32.shared" > /opt/mxe/settings.mk
    mkdir -p /opt/mxe/usr/x86_64-w64-mingw32.shared/out /opt/mxe/usr/i686-w64-mingw32.shared/out
    ```

1. Compile dependencies. This step takes about 2.5 hours.
    ```bash
    cd /opt/mxe
    make qttools qtwinextras sdl2 ogg vorbis lame a52dec faad2 fdk-aac libmad opus fribidi libass xvidcore x264 x265
    ```

1. Clone the Avidemux source.
    ```bash
    cd
    git clone https://github.com/Aldaviva/avidemux.git
    cd avidemux
    ```

1. Compile Avidemux. This takes about 4.5 minutes the first time and 40 seconds for subsequent builds.
    ```bash
    bash bootStrapCrossMingwQt5_mxe.sh --64 --rebuild
    ```

1. The compiled executable and all dependencies will be written to a ZIP file `avidemux/packaged_mingw_build_*/avidemux_r*_win64Qt5.zip`.
## Building GBP

If you want to rebuild yourself the application binary for Linux and not use the provided AppImage binary in the "Releases" folder, here is the procedure to rebuild from scratch the AppImage from the sources : 

### Step 1 : Install Ubuntu 22.04

On a new machine or VM, install **UBUNTU 22.04** with 75 GB disk space on local File System. Then perform all the updates required by Mint.

For a VM, it will also be quite handy to configure Mint to have a share folder with the host system (e.g. to transfer gbp code). See :

https://sysguides.com/share-files-between-kvm-host-and-linux-guest-using-virtiofs

### Step 2 : Install additional packages required

To prepare for later install of Qt Framework and building of gbp in Qt Creator, some additional packages are required. See https://doc.qt.io/qt-6.8/linux.html for more details on first item. Run the following :

`sudo apt install build-essential libgl1-mesa-dev` 

`sudo apt install libxcb-cursor0 libxcb-cursor-dev`

`sudo apt install libfuse2` (To run an AppImage)


### Step 3 : Install Qt

Download and run the **Qt online installer**. Try this link :

https://www.qt.io/download-qt-installer-oss?utm_referrer=https%3A%2F%2Fwww.qt.io%2Fdownload-open-source

Install Qt 6.8.0 in default directory ($HOME). The components to install are : 

* Qt 6.8.0 and all the sub items, except Android and Web Assembly stuff
* Qt Creator 14
* CMake
* Ninja

Test QtCreator by creating dummy widget app and verify that everything works.

### Step 4 : Install gbp source

Install gbp source ([gbp/src at master - claude_dumas/gbp - Codeberg.org](https://codeberg.org/claude_dumas/gbp/src/branch/master/src)) in a directory, for example : ~/data/dev/Qt/gbp.  

### Step 5 : Rebuild gbp in Qt Creator

In QtCreator, load the project (src/CmakeLists.txt). You will probably have to click "Configure" right after the load. Do a “rebuild” with the “Minimum Size Release” kit selected and make sure it is 100% successful. Run and check that the application seems to  work fine.

### Step 6

The last step is to create an AppImage bundle from the executable produced by QtCreator. For that, we will use the FOSS application “linuxdeploy”. For more info, see :

- https://github.com/linuxdeploy/linuxdeploy
- https://github.com/linuxdeploy/linuxdeploy-plugin-qt 

#### A)
Download these applications from this site : https://github.com/linuxdeploy/linuxdeploy

- linuxdeploy-plugin-qt-x86_64.AppImage
- linuxdeploy-x86_64.AppImage

#### B)

Create a directory where the AppImage will be built, e.g. ~/tmp/gbp. Put there the following : 

- the 2 files downloaded previously.
- the compiled binary (gbp) : this is the executable produced by QtCreator after a rebuilt, found in the QtCreator Build directory  
- the file rebuild-appimage.sh, from the “build-resources” folder
- - the app icon (gbp.png), from the “build-resources” folder
- the app Desktop shortcut (gbp.desktop), from the “build-resources” folder
- the gbp_en.qm translation file : compiled “english” translation file, produced by QtCreator after a rebuild, found in the QtCreator Build directory  
- and gbp_fr.qm : compiled “french” translation file, produced by QtCreator after a rebuild, found in the QtCreator Build directory  

#### C)

Open a terminal in this directory. Run the following shell script :

`./rebuild-image.sh`

which contains the following lines : 

- #!/bin/bash
- rm -r AppDir
- export LD_LIBRARY_PATH=~/Qt/6.8.0/gcc_64/lib
- echo $LD_LIBRARY_PATH
- export PATH=$HOME/Qt/6.8.0/gcc_64/bin:$PATH
- ./linuxdeploy-x86_64.AppImage --appdir=AppDir --executable=./gbp --plugin=qt --desktop-file=gbp.desktop  --icon-file=gbp.png --output=appimage


**the resulting AppImage is now in the directory and is name "graphical-budget-planner-x86_64.AppImage"** . Set permission to “execute” . This is your gbp executable file for Linux.


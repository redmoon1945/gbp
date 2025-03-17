## Building GBP

If you want to rebuild yourself the application binary for Linux and not use the provided AppImage binary in the "Releases" folder, here is the procedure to rebuild from scratch the AppImage from the sources : 

### Step 1 : Install Ubuntu 22.04

On a new machine or VM, install the latest version of **UBUNTU 22.04** with 75 GB disk space on local File System. Then perform all the updates required by running the software updater.

For a VM, it is also required to configure your host OS to have a share folder with the guest system (to transfer gbp code and produced AppImage). See :
https://sysguides.com/share-files-between-kvm-host-and-linux-guest-using-virtiofs

### Step 2 : Install additional packages required

To prepare for later install of Qt Framework and building of gbp in Qt Creator, some additional packages are required. See https://doc.qt.io/qt-6.8/linux.html for more details on first item. Run the following :

`sudo apt install build-essential libgl1-mesa-dev` 

`sudo apt install libxcb-cursor0 libxcb-cursor-dev`

`sudo apt install libfuse2` (To run an AppImage)


### Step 3 : Install Qt

Download and run the **Qt online installer**. Try this link :

https://www.qt.io/download-qt-installer-oss?utm_referrer=https%3A%2F%2Fwww.qt.io%2Fdownload-open-source

Install Qt 6.8.2 in default directory ($HOME). The components to install are : 

* Qt 6.8.2 and all the sub items, except Android and Web Assembly stuff
* Qt Creator 15.x
* CMake
* Ninja

Test QtCreator by creating dummy widget app and verify that everything works.

### Step 4 : Install gbp source and tools
- Create the directory "$HOME/data/dev/Qt/gbp"
- Copy gbp source, doc and tools ([gbp directory at master - claude_dumas/gbp - Codeberg.org](https://codeberg.org/claude_dumas/gbp/src/branch/master/)) in $HOME/data/dev/Qt/gbp. After the copy operation, the "gbp" directory should contain 3 subdirectories : "src", "doc" and "build-tools"

### Step 5 : Rebuild gbp in Qt Creator

In QtCreator, load the project (src/CmakeLists.txt). You will probably have to click "Configure" right after the load. Do a “rebuild” with the “Minimum Size Release” kit selected and make sure it is 100% successful. In Qt Creator, run and check that the application seems to work fine.

### Step 6 : Create the AppImage

The last step is to create an independent AppImage bundle from the executable produced by QtCreator. For that, we will use the FOSS application “linuxdeploy”. For more info, see :

- https://github.com/linuxdeploy/linuxdeploy
- https://github.com/linuxdeploy/linuxdeploy-plugin-qt 

#### A)
Download these applications from the linuxdeploy site :

- "linuxdeploy-plugin-qt-x86_64.AppImage" (from https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases)
- "linuxdeploy-x86_64.AppImage" (from https://github.com/linuxdeploy/linuxdeploy/releases)

Set the permissions to "execute" (e.g., sudo chmod a+x lin* )


#### B)

Create a directory where the gbp AppImage will be built (e.g. ~/gbp-app-image-build). In this guide, we will call this directory GBP_APP_IMAGE_BUILD_DIR. Put there the following : 

- the 2 files downloaded previously.
- From the QtCreator Build directory $HOME/data/dev/Qt/gbp/src/build/Desktop_Qt_6_8_2-MinSizeRel
  - the compiled binary file "gbp" : this is the executable produced by QtCreator after a rebuilt
  - the "gbp_en.qm" and "gbp_fr.qm" translation files
- From the "$HOME/data/dev/Qt/gbp/build-tools" directory : 
  - the file "create-appimage.sh"
  - the app icon "gbp.png""
  - the app Desktop shortcut "gbp.desktop"

#### C)

Open a terminal and change current directory to  GBP_APP_IMAGE_BUILD_DIR. Run the following shell script :

`./create-image.sh`

**the resulting AppImage is now in the directory GBP_APP_IMAGE_BUILD_DIR and is name "graphical-budget-planner-x86_64.AppImage"** . Set permission to “execute” . This is your gbp executable file for Linux.


# graphical-budget-planner

## Screenshots

The Main Window

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/main-window.png)

Editing a scenario

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/edit-scenario.png)

Editing a periodic income/expense

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/edit-periodic-income.png)

Editing an irregular income/expense

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/edit-irregular-incomes.png)

Analysis - Relative weigth of incomes/expenses

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/analysis-relative-weight.png)

Analysis - Monthy Report Chart

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/analysis-monthly-chart.png)

## Installation

GBP is distributed as an “AppImage” on Linux platform, which is a single-file executable packaging format allowing a program to run on many Linux distributions. There is nothing else to install. After downloading the AppImage from  [Releases - claude_dumas/gbp - Codeberg.org](https://codeberg.org/claude_dumas/gbp/releases) ,  user just has to enable “executable” permission on the file and it is ready to be launched. 

On Ubuntu (tested on v 20.04, 22.04, 24.04), additional steps must be performed. In order to run an AppImage, some packages are missing from the default distribution. Ubuntu needs the FUSE library to run AppImage like GBP. Otherwise, when launched, you will get the following error : 

`dlopen(): error loading libfuse.so.2`

AppImages require FUSE to run. To solve this, do : 

`sudo apt install libfuse2`

## Supported Platforms and Languages, System Requirements

GBP is intended to be run first and foremost on the Linux Operating System. But since it is built using the Qt cross platform toolkit, a version of GBP for the Windows® Operating System have also been produced. Tests have been conducted on Windows® 10 only.

GBP does not use a lot of RAM (the absolute worst case ever seen is 175 MB for an extremely demanding testing scenario) and necessitate roughly 50 MB of disk space (not taken into account the scenario files that you will create and GBP log files, which are all pretty small anyways).

As of July 2024, GBP has been successfully tested on the following Linux platforms : 
    • Ubuntu 24.04, Gnome, both X11 and Wayland, kernel 6.8.0
    • Ubuntu 22.04.4, Gnome 42.9 both X11 and Wayland, kernel 6.5.0-21
    • OpenSUSE Leap 15.5, KDE Plasma 5.27.9 both X11 and Wayland, kernel 5.14.21
    • Mint 21.3 Edge X11
    • Mint 21.3 X11 Kernel 5.15
    • Debian 12.5 , Plasma-X11, KDE 5.27.5  qt 5.15.18 kernel 6.1.0.21
    • Debian 12.5, Plasma-Wayland, KDE 5.27.5  qt 5.15.18 kernel 6.1.0.18
    • Debian 12.5, XFCE 4.18,  qt 5.15.18 kernel 6.1.0.18
    • Debian 12.5, Gnome classic on Xorg, qt 5.15.18 kernel 6.1.0.18
    • LMDE 5, X11, kernel 5.10.0, Cinnamon 5.6.8
    • LMDE 6 X11, Cinnamon 6.04, kernel 6.1.0-17
    • Fedora 39 , Gnome, both X11 and Wayland, kernel 6.9.4
    • Fedora 40, KDE Plasma 6.1.1, Wayland, Kernel 6.9.5
    • MX linux 23.2  XFCE 4.18.1 (Debian 12.4)  kernel 6.6.12
    • MX linux 23.2  KDE Plasma, both X11 and Wayland, kernel 6.6.12
    • Manjaro Plasma, KDE Plasma 6.0.5, both X11 and Wayland, Qt 6.7.1, Kernel 6.6.32

GBP supports English and French languages. By default, English is used, but if the host Operating System is in French (whatever the country), then GBP will switch to French. More languages will hopefully be added in the future, if resources to translate are available.

A mouse is required to use the software.

## License, Disclaimers and Source Code Repository

Graphical Budget Planner (a.k.a graphical-budget-planner or GBP or gbp) is a free and open source Qt  desktop application intended to ease significantly the process of creating, maintaining and analyzing a personal budget. 

This application and all its source code are licensed under the GNU Affero General Public License version 3 or later (AGPL-3.0-or-later). It's Free Software. See https://www.gnu.org/licenses/#AGPL/

Software repository for GBP can be found at : 

https://codeberg.org/claude_dumas/gbp

Being built with the Qt 6.2.4 toolkit, GBP is subject to the Qt terms and conditions : see qt.io/licensing

Credits : 
    • Tobias Leupold : code to calculate difference between 2 dates -> see https://nasauber.de/blog/2019/calculating-the-difference-between-two-qdates/
    • QCustomPlot : A Qt C++ widget for plotting and data visualization -> see https://www.qcustomplot.com/

## Usage

See the detailed User Manual to get in depth information about this application : [gbp/doc/Graphical Budget Planner - User Manual.pdf at master - claude_dumas/gbp - Codeberg.org](https://codeberg.org/claude_dumas/gbp/src/branch/master/doc/Graphical%20Budget%20Planner%20-%20User%20Manual.pdf)

Graphical Budget Planner (GBP) is an open source Qt desktop application intended to ease significantly the process of creating, maintaining and analyzing a personal budget. It allows the following :

    1. See graphically the evolution of your cash balance through time, at any given moment in a period covering the next 100 years !
    2. Easy zooming and/or panning
    3. Specify painlessly all your forecasted income/expense budget items, with flexibility to define periodic or irregular flow of incomes/expenses.
    4. Optionally define inflation, either as a constant value or a complex series of changing values.
    5. Optionally define a custom monthly growth pattern for any income/expense specification , expressed either as a constant value or a complex series of changing values.
    6. Perform automatically different types of analysis on your data, like relative weight of incomes/expenses over custom period, monthly and yearly reports.
    7. Your data is not locked in : all scenarios are in open JSON format, and resulting data are exportable in CSV format.
    8. Fully support UNICODE in all text fields.

GBP is all about CASH BALANCE **FORECASTING** : the key principle adopted in the design of the software is to take into consideration only the **FUTURE** incomes/expenses expected to occur, starting “tomorrow”, “today” being the system date when the application has been launched. 

Consequently, this is not the right application if you want know how and when your money has been earned/spent in the past (that is tracking your incomes/expenses made "before today"). 

## Building GBP

If you want to build yourself the application for Linux and not use the provided binaries in the "Releases" folder, here is the procedure : 

### Step 1

On a new machine or VM, install Ubuntu 20.04 with 50 GB disk space on local File System. This is a rather old version, but it is LTS and allow for wide compatibility across Linux distributions. Perform all the updates mentioned by Ubuntu.

### Step 2

For development, some stuff must be installed on Ubuntu before trying to install Qt 6.2.4 later. See https://doc.qt.io/qt-6.2/linux.html. Essentially, it is :

`sudo apt install build-essential libgl1-mesa-dev`

### Step 3

Download, install and run the online Qt installer. Install Qt 6.2.4 (with all “additional libraries”) and QtCreator 13 in default directory ($HOME). No need for Android and WebAssembly stuff. Test QtCreator by creating dummy widget app and verify that everything works.

When launching QtCreator for the first time, you should have the following error : 
`Could not load the Qt platform plugin "xcb" in "" even though it was found.
This application failed to start because no Qt platform plugin could be initialized. Reinstalling the application may fix this problem.`

To fix the problem, do : 
`sudo apt-get install -y libxcb-cursor-dev`

### Step 4

Modify ~/.profile to include qmake path (for linuxdeployqt), that is , that is ~/Qt/6.2.4/gcc_64/bin. Reboot

### Step 5

Install gbp source ([gbp/src at master - claude_dumas/gbp - Codeberg.org](https://codeberg.org/claude_dumas/gbp/src/branch/master/src)) in a directory, for example : ~/data/dev/Qt/gbp. If you are running Ubuntu as a guest in a KVM virtualisation solution, you can look at https://sysguides.com/share-files-between-kvm-host-and-linux-guest-using-virtiofs to see how to shared folders with the host. This is required to transfer in the GBP source and transfer out the final built AppImage file. 

### Step 6

In QtCreator, load the project (CmakeLists.txt), and do a “rebuild” with the “Minimum Size Release” kit selected. Run and check that the application seems to basically work fine.

## Step 7

To run the application outside QtCreator, Ubuntu needs the FUSE library to run appimage. Otherwise, when lauching such appimage, you will get the following error : 

`dlopen(): error loading libfuse.so.2`
On Ubuntu, AppImages require FUSE to run. All other Linux distros do not need this. 

To solve this, do : 
`sudo apt install libfuse2`

### Step 8

We need to create an AppImage bundle to be used as the executable. For that, we will use the FOSS application “linuxdeploytqt” (see https://github.com/probonopd/linuxdeployqt) to produce an AppImage bundle. Download this application or use the copy found in the “build-resources” folder. Note that as of July 2024, the  author mentioned that he is now working on a GO version of this application (https://github.com/probonopd/go-appimage), but we never tested it yet. 

#### A)

Create the following directory structure to hold the data for AppImage creation : lets say this dir is called “appimage-gbp”<br>

* appimage-gbp
* appimage-gbp/deploy_it

#### B)

Place the following files in the following directories :

* appimage-gbp  
  
  * linuxdeployqt AppImage application that you downloaded previously, taking care to set the permission to “execute”

* appimage-gbp/deploy_it
  
  * gbp.desktop (from the “build-resources” folder)  
  * gbp.png (from the “build-resources” folder)  
  * gbp : this is the executable produced by QtCreator after a rebuilt, found in the QtCreator Build directory  
  * gbp_en.qm : compiled “english” translation file, produced by QtCreator after a rebuild, found in the QtCreator Build directory  
    * gbp_fr.qm :compiled “french” translation file, produced by QtCreator after a rebuild, found in the QtCreator Build directory  

#### C)

Run linuxdeployqt
`run ./linuxdeployqt-continuous-x86_64.AppImage appimage-gbp/deploy-it/gbp -appimage -verbose=1`

the resulting AppImage is now in appimage-gbp . Set permission to “execute” . This is your gbp executable file for Linux.
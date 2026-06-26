# graphical-budget-planner

## What's new ?

Version **1.8.0** is out (July 2026) ! This is a big release, with many new features and several improvements. Here are the main changes compared to 1.7.0 :

 - **Analysis Dialog** has been significantly expanded with two new tabs:
   - **Period — Heatmap** : visualizes incomes, expenses, or the income/expense delta as a color-coded grid at monthly or yearly granularity, with configurable colors, adjustable cell sizes, and rich tooltips.
   - **Compare CSD** : overlays up to 10 CSDs on a single line chart for direct side-by-side comparison, with persistent per-CSD color assignment and optional event markers.
   - The existing Period report and chart tabs have been consolidated (monthly and annual views merged into single tabs with a radio-button selector).
- **Workspace support** : multiple isolated instances of the application can now run side-by-side using the `-workspace=NAME` command-line argument. Each workspace keeps its own settings and log files.
- **Anonymize** feature (menu Scenario) : replaces sensitive scenario data with placeholder values, with optional amount randomization. Changes remain in-memory until explicitly saved.
- **CSD breakdown** : both Periodic and Irregular CSD edit dialogs now offer a button to visualize the period-by-period totals for that CSD until the end of its validity period.
- **Windows UI** : many improvements to the Windows version for the behavior of the UI, specially regarding layouts with misc fonts (including big ones).

## Screenshots

The Main window

![](doc/images/a.png)

Editing a scenario

![](doc/images/b.png)

Managing tags

![](doc/images/c.png)

Analysis - Relative weight of incomes/expenses

![](doc/images/d.png)

Analysis - Annual Report Chart

![](doc/images/e.png)

Analysis - Heatmap

![](doc/images/f.png)

## Installation

### On Linux

GBP is distributed as an “AppImage” on Linux platform, which is a single-file executable packaging format allowing a program to run on many Linux distributions. There is nothing else to install. After downloading the most recent AppImage application from https://github.com/redmoon1945/gbp/releases, user has to enable “executable” permission on the file and it is ready to be launched. 

On Ubuntu (tested on v 22.04, 24.04), additional steps must be performed. In order to run an AppImage, some packages are missing from the default distribution. Ubuntu needs the FUSE library to run AppImage like GBP. Otherwise, when launched, you will get the following error : 

`dlopen(): error loading libfuse.so.2`

AppImages require FUSE to run. To solve this, do : 

`sudo apt install libfuse2`

### On Windows®

Download the ".zip" file binary from the repository mentioned above and unzip it in the folder of your choice. Launch gpb.exe to execute GBP.

## Supported Platforms and Languages, System Requirements

GBP is intended to be run first and foremost on the Linux Operating System. But since it is built using the Qt cross platform toolkit, a version of GBP for the Windows® Operating System is also provided and fully tested. A version for MAC could be easily produced if requested.

GBP does not use a lot of RAM (the absolute worst case ever seen is 300 MB for an extremely demanding testing scenario) and necessitate roughly 50 MB of disk space (not taken into account the scenario files that you will create and GBP log files, which are all pretty small anyways).

As of July 2026, GBP has been extensively tested on the following platforms : 

* Kubuntu 26.04 LTS, KDE 6.6.4, Wayland, Kernel 7.0.0-22
* Linux Mint Debian Edition 7, Cinnamon 6.4.13, X11, Kernel 6.12.94
* Windows® 11 Home Edition

It has also been tested, but not as extensively, on the following platforms :

* MX linux 25, XFCE 4.20.1, X11, Kernel 6.16.12
* Fedora 43, KDE Plasma 6.6.5, Wayland, Kernel 7.0.11-100
* OpenMandriva Lx 5.0, X11, KDE Plasma 5.27.9, kernel 6.6.2
* Ubuntu 24.04.04 LTS, Gnome 46, Kernel 6.7.0-35
* Ubuntu 22.04.5 LTS, Gnome 42.9, X11, Kernel 6.8.0-52
* Ubuntu 22.04.5 LTS, Gnome 42.9, Wayland, Kernel 6.8.0-52

GBP supports English and French languages. By default, English is used, but if the host Operating System is in French (whatever the country), then GBP will switch to French. More languages will hopefully be added in the future, if resources to translate are available.

A mouse is required to use the software. A screen resolution of **1650 x 1080** or better is required.

## License, Disclaimers and Source Code Repository

Graphical Budget Planner (a.k.a graphical-budget-planner or GBP or gbp) is a totally free and open source Qt desktop application intended to ease significantly the process of creating, maintaining and analyzing a personal budget. It does NOT connect to internet whatsoever.

This application and all its source code are licensed under the GNU Affero General Public License version 3 or later (AGPL-3.0-or-later). It's Free Software. See https://www.gnu.org/licenses/#AGPL/

Software repository for GBP can be found at : 
https://github.com/redmoon1945/gbp

Being built with the Qt toolkit, GBP is subject to the Qt terms and conditions : see qt.io/licensing

Credits : 

* Tobias Leupold : code to calculate difference between 2 dates -> see https://nasauber.de/blog/2019/calculating-the-difference-between-two-qdates/

## Usage

See the detailed User Manual to get in depth information about this application : gbp/doc/Graphical Budget Planner - User Manual.pdf 

Graphical Budget Planner (GBP) is an open source Qt desktop application intended to ease significantly the process of creating, maintaining and analyzing a personal budget. It allows the following :

* See graphically the evolution of your cash balance through time, at any given moment in a period covering the next 100 years !
* Easy zooming and/or panning
* Specify painlessly all your forecasted income/expense budget items, with flexibility to define periodic or irregular flow of incomes/expenses.
* Optionally define inflation, either as a constant value or a complex series of changing values.
* Optionally define a custom monthly growth pattern for any income/expense specification , expressed either as a constant value or a complex series of changing values.
* Perform automatically different types of analysis on your data, like relative weight of incomes/expenses over custom period, monthly and yearly reports.
* Optionally convert all amounts to Present Values using a user-defined discount rate in Option Dialog. 
* Your data is not locked in : all scenarios are in open JSON format, and resulting data are exportable in CSV format.
* Fully support UNICODE in all text fields.

GBP is all about CASH BALANCE **FORECASTING** : the key principle adopted in the design of the software is to take into consideration only the **FUTURE** incomes/expenses expected to occur (so the "forecasts"), starting “tomorrow”, “today” being the system date when the application has been launched. **GBP does NOT track past incomes / expenses**...Consequently, this is not the right application if you want know how and when your money has been earned/spent in the past (that is, tracking your incomes/expenses made "before today"). 

## Building GBP

If you want to build yourself the application for Linux and not use the provided binaries in the "Releases" folder, see the Building.md
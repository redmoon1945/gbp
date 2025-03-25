# graphical-budget-planner
## What's new ?
Version 1.6.2 is out (April 2025) ! Here are the main changes compared to 1.5.3 :

- Introducing the notion of **tags**, which is a way to implement categories of incomes/expenses, but in a much more flexible and powerful way.
- In About Dialog, add a new Contact tab for those who wish to know how to contact directly the author(s) of this software.
- In Main Window,
  - Add a line at value Y=0, with user-specified color. This helps to identify rapidly if a point is above or below 0. Can be turned off in Options Dialog.
  - Add the file name of the scenario file opened in the top window title
- Options Dialog has been updated for more clarity
  - Reformat the Chart tab layout for better clarity
  - Add an option to control how the X-Axis Dates for charts are formatted (Locale, ISO 8601, ISO 8601 with 2-digits year)
  - Add an option to show or hide tooltips for the whole application.
- In Edit Scenario Dialog
  - Rework the filters controls, so it is easier to undertand and it takes less visual space.
  - It is now possible to select several Cash Stream Definitions and in one shot set the color of theirs names ("Set Color" button)
- In several Dialogs, optimize the spacing and position of the UI components to provide more space to the list of incomes/expenses, while maintaining the same window size.
- In all Dialogs, use short format for dates, because the long one takes up too much space when big fonts are used.



## Screenshots

The Main Window

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/main-window.png)

Editing a scenario

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/edit-scenario.png)

Editing a periodic income/expense

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/edit-periodic-income.png)

Editing an irregular income/expense

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/edit-irregular-incomes.png)

Analysis - Relative weight of incomes/expenses

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/analysis-relative-weight.png)

Analysis - Monthy Report Chart

![](https://codeberg.org/claude_dumas/gbp/raw/branch/master/doc/doc-source/images/analysis-monthly-chart.png)

## Installation

### On Linux
GBP is distributed as an “AppImage” on Linux platform, which is a single-file executable packaging format allowing a program to run on many Linux distributions. There is nothing else to install. After downloading the most recent AppImage application from  [Releases - claude_dumas/gbp - Codeberg.org](https://codeberg.org/claude_dumas/gbp/releases) ,  user has to enable “executable” permission on the file and it is ready to be launched. 

On Ubuntu (tested on v 22.04, 24.04), additional steps must be performed. In order to run an AppImage, some packages are missing from the default distribution. Ubuntu needs the FUSE library to run AppImage like GBP. Otherwise, when launched, you will get the following error : 

`dlopen(): error loading libfuse.so.2`

AppImages require FUSE to run. To solve this, do : 

`sudo apt install libfuse2`

### On Windows®
Download the ".zip" file binary from the repository   [Releases - claude_dumas/gbp - Codeberg.org](https://codeberg.org/claude_dumas/gbp/releases) ,   and unzip it in the folder of your choice. Launch gpb.exe to execute GBP.

## Supported Platforms and Languages, System Requirements

GBP is intended to be run first and foremost on the Linux Operating System. But since it is built using the Qt cross platform toolkit, a version of GBP for the Windows® Operating System have also been produced. Tests have been conducted on Windows® 10 only.

GBP does not use a lot of RAM (the absolute worst case ever seen is 175 MB for an extremely demanding testing scenario) and necessitate roughly 50 MB of disk space (not taken into account the scenario files that you will create and GBP log files, which are all pretty small anyways).

As of April 2025, GBP has been extensively tested on the following Linux platforms :

* Fedora 41, KDE Plasma 6.3.2, Wayland, Kernel 6.13.5
* Ubuntu 24.04.02 LTS, Gnome 46, Wayland, Kernel 6.11.0-19
* Linux Mint 22.1, X11 , Cinnamon 6.4.8, Kernel 6.8.0-55

It has also been tested, but not extensively, on the following platforms :

* Windows® 10
* OpenMandriva Lx 5.0, X11, KDE Plasma 5.27.9, kernel 6.6.2
* MX linux 23.5, XFCE 4.20.0, X11, Kernel 6.1.0-29
* Ubuntu 22.04.5 LTS, Gnome 42.9, X11, Kernel 6.8.0-52
* Ubuntu 22.04.5 LTS, Gnome 42.9, Wayland, Kernel 6.8.0-52


GBP supports English and French languages. By default, English is used, but if the host Operating System is in French (whatever the country), then GBP will switch to French. More languages will hopefully be added in the future, if resources to translate are available.

A mouse is required to use the software.

## License, Disclaimers and Source Code Repository

Graphical Budget Planner (a.k.a graphical-budget-planner or GBP or gbp) is a free and open source Qt  desktop application intended to ease significantly the process of creating, maintaining and analyzing a personal budget. 

This application and all its source code are licensed under the GNU Affero General Public License version 3 or later (AGPL-3.0-or-later). It's Free Software. See https://www.gnu.org/licenses/#AGPL/

Software repository for GBP can be found at : 

https://codeberg.org/claude_dumas/gbp

Being built with the Qt toolkit, GBP is subject to the Qt terms and conditions : see qt.io/licensing

Credits : 

* Tobias Leupold : code to calculate difference between 2 dates -> see https://nasauber.de/blog/2019/calculating-the-difference-between-two-qdates/


## Usage

See the detailed User Manual to get in depth information about this application : [gbp/doc/Graphical Budget Planner - User Manual.pdf at master - claude_dumas/gbp - Codeberg.org](https://codeberg.org/claude_dumas/gbp/src/branch/master/doc/Graphical%20Budget%20Planner%20-%20User%20Manual.pdf)

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

GBP is all about CASH BALANCE **FORECASTING** : the key principle adopted in the design of the software is to take into consideration only the **FUTURE** incomes/expenses expected to occur, starting “tomorrow”, “today” being the system date when the application has been launched. 

Consequently, this is not the right application if you want know how and when your money has been earned/spent in the past (that is tracking your incomes/expenses made "before today"). 

## Building GBP

If you want to build yourself the application for Linux and not use the provided binaries in the "Releases" folder, see the Building.md
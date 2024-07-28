1.0.1 to 1.0.2
---------------

* display today's date in bottom startAmountLabel
* timeout of 5 sec on status bar message
* change main and editscenario window size to 1250 x 700
* adjust layouts of some windows

1.0.2 to 1.1.0
--------------

* add option to set "today" value in "Options" window
* remove support for command-line argument -today=<date>
* mecanism to ensure settings is loaded just once
* When requesting new scenario, switching scenario or quitting application, ask to save current scenario if changes have been made but not saved yet
* New decoration color feature for incomes/expenses names
* New layout for Options dialog (separate General from Charts related things)
* Improve sorting of StreamDef name by enabling locale-aware sorting

1.1.0 to 1.1.1
--------------

* fixe bug in calculation of variable growth
* in Edit Perodic Stream Def, change Validity Range date format to MMM from MMMM
* in Edit Irregular Stream Def, change date format for a shorter version

1.1.1 to 1.1.2
--------------

* introduce a splitter in the Main window
* reduce the font size for "resize/pan" toolbar button in main window

1.1.2 to 1.1.3
-------------

* Improve About box
* Remove options to set image export type and quality, always save in PNG quality=100

1.1.3 to 1.1.4
--------------

* Fix translation for Analysis Dialog
* Change RGB to Red Green Blue, remove smart color name because no localized version available
* Resize gray border for charts in Analysis Dialog to 1 pixel, no round corners
* Reduce font size for both X and Y axis for monthly and yearly charts in Analysis Dialog (does not work for KDE...)
* Add log entries to monitor Font Size settings by the program
* Fix percentage symbol not displayed sometimes in Edit Growth Dialog
* Edit Periodic now expands vertically in an optimal way
* Standardize labels format for all doalogs (no space at the end, everywhere)
* Standardize Fornmal Layout spacing (30,10) for all dialogs
* Standardize date format for Irregular Edit Dialog and Variable Growth Edit Dialog
* Make Edit Periodic and Edit Irregular same hieght (700) than Main Window
* Switch to native Color Chooser, to be able to support Locale language in OS (supported only in Windows and MacOS)

1.1.4 to 1.1.5
--------------

* Adopt AGPL V 3.0 or later as licence
* Add Codeberg URL in the About box, under a new tab ("Source Code")
* 

## 1.1.5 to 1.1.6

- Implement the option to have all calculated amount sbrought back to Present Value using a user-defined discout rate
- Add Config file location on the console at start-up

# Change logs
## 1.3.0 to 1.4.0
### New Features
- Moved from Qt 6.2.4 to 6.8.0
- The Cash Balance chart component has been completely re-written (moved from QCustomPlot to standard QT QChart). This remove the bug related to incorrect X axis tick marks from time to time. 
- Mouse wheel can be used to zoom in/out the Cash Balance curve. Options allow to set if moving away the wheel zoom in or out.
- The scenario file format has changed and gone from version 1.0.0 to 2.0.0.
- If no ini file (configuration file) is found at start-up, a welcome screen is shown.
- New General Info panel has been added to Main Window
- For Periodic Financial Stream Definition, when Scenario's inflation is selected to define the growth, offer an additional adjustment factor that allows the final growth to be proportional to inflation instead of exactly equal as before (e.g. inflation times a factor). 
- For Edit Periodic and Irregular Financial Stream Definition Dialogs, completely rework the See Occurrence Dialog so that a chart for the generated events is also displayed (useful to see the general trend, not the details)
- In Options, remove the max no of years for which financial events are generated and transfer it to the scenario data.
- In Periodic Creation/Edition, add an option to set the end of validation data to the maximum allowed by the scenario, instead of specify it directly. This is activated by default when creating a new Periodic Stream Definition. 
- In Scenario Properties Dialog, add file extension to the file name
- In Scenario Properties Dialog, add the max duration of the Financial Events
- In Edit Scenario Dialog, turn all disabled elements to gray (keep strikeout font though)
- In Help menu, add a new option to see the Change Log for all versions of GBP since 1.0.0
- In About Dialog, add the name of the Locale used by GBP


### Impacts on compatibility with older versions of GBP
- Concerning the scenario file format change, gbp 1.4 and later will read without any problem the old format (1.0.0) and update transparently the file format to 2.0.0 when opening it. No data is lost or altered. However, older gbp 1.3 and previous versions wont be able to read scenario files created with gbp 1.4 and above, which are of version 2.0.0

### Fixes
- For a Period Stream Definition of type "END-OF-MONTHLY", if the start date was not on an end of month, it was still part of the generated stream (which is wrong). It is now removed. 
- In the Analysis : Monthly and Yearly Tables, all months/years are now shown, even if there is no income/expense (value of 0).


## 1.2.0 to 1.3.0
### New Features
- In menu "Help", it is now possible to consult the full User Manual in PDF format, using the new menu item "User Manual"
- In menu "Help", it is now possible to consult the a Quick Tutorial in PDF format, using the new menu item "Quick Tutorial"
- In menu "Help", add to the "About" tab the full name of Config file and current Log file
- Update User Manual
- In Edit Irregular Dialog, if "Convert to Present Value" is enabled, add a label mentioning that all the amounts specified are "future values". 
- In Edit Irregular Dialog, add an option to "See all the occurrences". This is useful if the option "Convert to Present Values" is activated in the Options Dialog, in which case the amounts will be different than what is specified in the table.
- Add tooltips for most fields in windows 
- In Main Window, add a menu item in the "Scenario" named "Open Example". This will load a sample scenario provided by GBP. The file is actually copied to System Temp directory and opened from there. 
- For all MessageBoxes asking a question, set a default button
- Add a Scenario Properties Dialog in the Main Window, that provides some useful info like the file name, full path, format version, etc.

## 1.1.5 to 1.2.0
### New features
- Implement the option to have all calculated amounts converted to Present Values using a user-defined discount rate set in Options Dialog
- Have Config file location displayed on the terminal at start-up
- Rename "Close" to "Hide" the Cancel button in EditScenario Dialog
 
### Fixes
- In EditScenario, when creating a new income/expense or duplicating an existing one, make sure the duplicated item is made visible (appearing in the view port of the table) after the operation

## 1.1.4 to 1.1.5
### New features
* Adopt AGPL V 3.0 or later as licence
* Add Codeberg URL in the About box, under a new tab ("Source Code")

## 1.1.3 to 1.1.4
### New features
* Change RGB to Red Green Blue, remove smart color name because no localized version available
* Resize gray border for charts in Analysis Dialog to 1 pixel, no round corners
* Reduce font size for both X and Y axis for monthly and yearly charts in Analysis Dialog (does not work for KDE...)
* Add log entries to monitor Font Size settings by the program
* Edit Periodic now expands vertically in an optimal way
* Standardize labels format for all dialogs (no space at the end, everywhere)
* Standardize Formal Layout spacing (30,10) for all dialogs
* Standardize date format for Irregular Edit Dialog and Variable Growth Edit Dialog
* Make Edit Periodic and Edit Irregular same height (700) than Main Window
* Switch to native Color Chooser, to be able to support Locale language in OS (supported only in Windows and MacOS)

### Fixes
* Fix translation for Analysis Dialog
* Fix percentage symbol not displayed sometimes in Edit Growth Dialog

## 1.1.2 to 1.1.3
### New features
* Improve About box
* Remove options to set image export type and quality, always save in PNG quality=100

## 1.1.0 to 1.1.1
### New features
* in Edit Perodic Stream Def, change Validity Range date format to MMM from MMMM
* in Edit Irregular Stream Def, change date format for a shorter version

### Fixes
* fixe bug in calculation of variable growth

## 1.0.2 to 1.1.0
### New features
* add option to set "today" value in "Options" window
* remove support for command-line argument -today=<date>
* When requesting new scenario, switching scenario or quitting application, ask to save current scenario if changes have been made but not saved yet
* New decoration color feature for incomes/expenses names
* New layout for Options dialog (separate General from Charts related things)
* Improve sorting of StreamDef name by enabling locale-aware sorting

### Fixes
* Implement a mechanism to ensure settings is loaded just once


## 1.0.1 to 1.0.2
### New features
* display today's date in bottom startAmountLabel
* timeout of 5 sec on status bar message
* change main and editscenario window size to 1250 x 700
* adjust layouts of some windows















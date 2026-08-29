# Change log for GBP

## 1.8.0 to 1.8.1 (28-aug-2026)

### New features : The big ones

- **Installation simplified** : On Linux, GBP no longer requires a separate installation of the FUSE 2 library (`libfuse2`) as a prerequisite. 
- **Full French translation for bundled documentation** : the User Manual and Quick Tutorial are now available in French, in addition to English which is the default.
- **Dynamic column sizing** : table columns in several dialogs (Edit Irregular Financial Stream Definition, Edit Scenario, Manage Tags, Edit Variable Growth, Analysis — Tags tab) now resize themselves to fit their actual content, instead of relying on a fixed estimate made once at startup. This fixes long content getting truncated with an ellipsis on some platform/locale combinations — most visibly a German long-format date on Windows, where the fixed estimate didn't leave enough room for the font actually used there.
- **Better support for international locales** : numbers and dates across the app are now more consistently rendered in the application's actual locale — including locales with their own numbering system (e.g. Bengali or Arabic digits), where several places had been silently falling back to plain Western digits or picking up formatting that doesn't belong on an identifier (like a thousands separator on a year).
- In **Custom Date Interval dialog :**
  - **Custom date interval (chart) remembers your last selection** : the "Custom" chart range dialog no longer resets its "From"/"To" dates to the chart's current view every time it is opened ; your previous selection is kept instead, and only adjusted if it falls outside what the loaded scenario can actually calculate.

### New features : The details

- Documentation
  - The User Manual and Quick Tutorial PDFs are now provided in both English and French. The appropriate language version is selected automatically based on the application's current locale, consistent with the existing behavior for the Welcome screen.
- In About Dialog
  - The "Info" tab now also displays the location of the application's cache directory, alongside the existing config file and log file paths.
- In Custom Date Interval dialog
  - The "From" and "To" date fields now keep the values you last entered instead of resetting to the chart's current view each time the dialog is opened.
  - Both fields are bounded to the range the loaded scenario can generate financial events for (tomorrow through its max calculation date). If a previously entered value no longer fits that range (e.g. after changing "Options -> Scenario years"), it is automatically adjusted to the nearest allowed date.
- In command-line workspace options
  - `-workspace_cleanup` now shows a preview of what will be deleted and, on Linux and macOS, asks for confirmation before deleting anything ; it previously deleted immediately with no way to preview or cancel. On Windows, it proceeds automatically after the preview instead of prompting.
  - `-workspace_cleanup` also detects and removes orphaned log files. The default workspace's configuration and logs are never touched.
  - `-workspace_list` now shows the number of log files belonging to each workspace.
- In Edit Scenario dialog
  - In the CSD list, a CSD name longer than 50 characters is now truncated with an ellipsis, to keep the table from growing excessively wide. The full name is unaffected everywhere else (e.g. when editing the CSD).
- For consistency, dates shown across the app (date fields, labels, messages, and some tables) now use the same shorter `yyyy-MMM-dd` format (e.g. `2028-Dec-28`) instead of a mix of locale-short and full-month-name formats.

### Fixes

- The application no longer depends on `libfuse2` at runtime, removing the need to manually install it on Ubuntu 22.04, 24.04, 26.04.
- Fixed a potential multi-user issue on the same Linux machine. Cached copies of the bundled PDF documents (Change Log, Quick Tutorial, User Manual, Welcome) were previously stored in the shared system temporary directory (`/tmp`). Because of the sticky bit set on `/tmp`, only the OS user who originally created a cached copy was permitted to delete or replace it — so if a different user account on the same machine tried to view a document after its content changed, the operation could fail with a permission error. Cached copies are now stored in a per-user application cache directory instead, so each user account keeps its own independent copy and is no longer affected by files created by other users.
- Fixed the same potential multi-user issue for the "Open Example" menu action (Scenario menu). The bundled example budget scenario was previously copied to the shared system temporary directory before being opened; it is now copied to the same per-user application cache directory as the bundled PDF documents.
- For Windows, some file paths were not rendered correctly : the Scenario Properties dialog's "Path" field and the "Open recent" menu (Scenario menu) displayed forward slashes (`/`) instead of the native Windows backslash (`\`) separator. Paths are now shown using the OS-appropriate separator.
- The same forward-slash issue affected the console messages printed at startup (log file, configuration file, and cache directory locations) ; they are now shown using the OS-appropriate separator as well.
- Some tables could truncate long content with an ellipsis instead of showing it in full. This was most visible in the Edit Irregular Financial Stream Definition dialog's Date column under some locales (e.g. German) on Windows, where the locale's long-format date (including the full weekday and month name) did not fit the column's fixed width estimate. Column widths in that table, as well as in the Edit Scenario CSD list, Manage Tags dialog, Edit Variable Growth dialog, and the Analysis dialog's Tags tab, are now computed from their actual content instead of a fixed estimate, and are recalculated automatically whenever the table's data changes.
- In the Edit Variable Growth dialog, the last column (monthly growth) was also forced to stretch and fill all remaining space regardless of its content ; it now sizes to its content like the other columns.
- Several date fields used a fixed-width guess for sizing instead of their actual width ; Edit Periodic's From/To date fields also silently ignored their configured display format. Both are fixed.
- Some numbers (percentages, counts, a few amounts) were shown using plain Western digits regardless of locale, instead of the locale's own numbering system (e.g. Bengali or Arabic digits) like the rest of the app. Affected spots : Analysis dialog's Relative Weight rank/percentage and Tags table, the Period Heatmap's year labels and tooltips, Scenario Properties' event counts and duration, the Present Value Calculator's monthly rate, an import error message, and the occurrence list's event index.
- A few more localization gaps in the same vein : the Period Heatmap's row headers could end up too narrow because their width was measured against Western digits instead of the locale's own ; years and the occurrence list's event index could pick up a stray thousands separator (e.g. "2,026") since they're identifiers, not quantities ; and the inflation/growth percentages shown when visualizing occurrences and in Scenario Properties now follow the same locale-aware formatting as amounts elsewhere in the app.

### Known Issues

- On most Linux distribution running Gnome (tested on Ubuntu 22.04, 24.04 and 26.04), modal dialog windows are tied by default to the Main Window and cannot be moved. To be able to move them (which is recommended as it adds flexibility), you can use the Gnome Tweaks application : go to Windows->Attach Modal Dialog and toggle to OFF. Another approach is to type the following command in a terminal : gsettings set org.gnome.mutter attach-modal-dialogs false. Note that this is a per-user setting, so it must be applied separately for each OS user account that runs GBP, and you may need to log out and back in (or at least restart GBP) for it to take effect.

## 1.7.0 to 1.8.0 (26-jun-2026)

### New features : The big ones

- **Analysis Dialog** has been significantly expanded with two new tabs:
  - **Period — Heatmap** : visualizes incomes, expenses, or the income/expense delta as a color-coded grid at monthly or yearly granularity, with configurable colors, adjustable cell sizes, and rich tooltips.
  - **Compare CSD** : overlays up to 10 CSDs on a single line chart for direct side-by-side comparison, with persistent per-CSD color assignment and optional event markers.
  - The existing Period report and chart tabs have been consolidated (monthly and annual views merged into single tabs with a radio-button selector).
- **Workspace support** : multiple isolated instances of the application can now run side-by-side using the `-workspace=NAME` command-line argument. Each workspace keeps its own settings and log files.
- **Anonymize** feature (menu Scenario) : replaces sensitive scenario data with placeholder values, with optional amount randomization. Changes remain in-memory until explicitly saved.
- **CSD breakdown** : both Periodic and Irregular CSD edit dialogs now offer a button to visualize the period-by-period totals for that CSD until the end of its validity period.
- **Windows UI** : many improvements to the Windows version for the behavior of the UI, specially regarding layouts with misc fonts (including big ones).

### New features : The details

- Select currency Dialog
  - This is used when a new scenario is to be created. The currency is directly selected by the user, instead of being inferred from a country selection.
- In Analysis Dialog :
  - Monthly and Annual **report** tabs have been merged in a single one, called "Period - list". More information has been added, with auto sizing for the column (user can manually override). Monthly or annual view can be selected with radio buttons.
  - Monthly and Annual **chart** tabs have been merged in a single one, called "Period - chart". Monthly or annual view can be selected with radio buttons.
  - In Period - list tab :
    - Row numbers (vertical header) have been removed from the table.
    - Amount columns (Income, Expenses, Delta, Cash balance) no longer use monospace font; this fixes a slight vertical alignment issue.
  - In Period - chart tab :
    - Hovering a bar now shows a tooltip with the series name, period, and formatted amount.
    - Removed the "Selected bar" info label shown below the chart when a bar was clicked (superseded by the tooltip).
    - Y axis now has 5 major divisions (reduced from 10) and no minor divisions, giving each label more vertical room and preventing label truncation at typical dialog heights.
  - In Tags tab :
    - Row numbers (vertical header) have been removed from the table.
    - The Total amount column no longer uses monospace font; this fixes a slight vertical alignment issue.
  - In Relative Weight tab :
    - The CSD list box no longer uses monospace font; this fixes a slight vertical alignment issue.
  - A new **Compare CSD** tab has been added.
    - It displays a line chart overlaying the financial events of multiple CSDs over time, allowing direct visual comparison.
    - A side list shows all CSDs active in the current date range, sorted alphabetically. Selecting entries (up to 10) adds them to the chart.
    - Incomes or Expenses mode is selected via radio buttons; each mode maintains its own independent selection.
    - Each selected CSD is drawn with a distinct color from the Okabe-Ito palette. Colors are assigned persistently per CSD so that adding or removing other CSDs does not change the color of an already-selected one. A colored square icon next to each list item reflects the assigned chart color.
    - Lines use a step rendering: the value holds flat until the exact moment the next event occurs.
    - An optional **Show points** checkbox overlays scatter markers at each individual event.
    - Clicking a marker highlights it and displays its date and value below the chart; clicking it again deselects it.
    - A **Fit** button rescales the chart to the current data and date range. An **Unselect all** button clears the selection for the active mode.
  - A new Heatmap tab has been added, called "Period - heatmap".
    - It displays financial data as a color-coded grid in two granularities, selectable via radio buttons:                                     
      - Monthly : one row per year, one column per month (Jan–Dec), with year and abbreviated month-name headers.
      - Yearly : years fill a fixed 10-column grid left-to-right, top-to-bottom; the year is written inside each cell and no headers are shown.    
    - Three display modes are available:           
      - Incomes : sequential scale — the smallest non-zero income shows a near-background tint of the income color; the highest income shows full  
        saturation. The minimum color is derived automatically from the maximum color.                    
      - Expenses : same sequential scale applied to expenses.
      - Delta (income − expense) : diverging scale centered on pure black (breakeven). Positive periods blend toward the positive-side color;
        negative periods blend toward the negative-side color. Each side is normalized independently so both extremes always reach full saturation.    
    - An Exclude current month / year checkbox omits the in-progress period from both the grid and normalization. 
    - Cell states:    
      - Periods with no financial events are shown with a forward-diagonal hatch (/). 
      - Periods outside the scenario range (or excluded) use a backward-diagonal hatch (\).
      - In income or expense mode, a period that has events but zero activity for the active type is treated as no-events rather than colored,     
        because its zero is uninformative rather than a meaningful breakeven. In delta mode, zero (income == expense) is always colored pure black.    
    - Color configuration via the header:               
      - In income or expense mode: one color picker sets the maximum (saturated) color; the minimum tint is derived automatically.                 
      - In delta mode: two color pickers set the positive-extreme and negative-extreme colors independently. Default colors are green (positive) and red (negative).                              
      - Each picker has a one-click reset to its default. 
    - A legend at the bottom shows color swatches for the minimum value, maximum value, breakeven (delta mode only), no-events hatch, and          
      out-of-period hatch, with formatted currency labels for min and max.                       
    - Cell size can be adjusted between Normal, Big (1.5×, default), and Bigger (2×) via a combo box in the header; the table scrolls automatically
      when cells exceed the available space.                                             - Hovering any cell shows a tooltip with the full income / expenses / delta / balance breakdown for that period.
- In Edit Periodic Csd Dialog
  - Add a button to visualize the "breakdown" for this CSD, that is the period-by-period totals until the end of the CSD validity period. Breakdown's period can be Month or Year.
- In Edit Irregular Csd Dialog
  - The Amount column no longer uses monospace font; this fixes a slight vertical alignment issue.
  - Add a button to visualize the "breakdown" for this CSD, see "Edit Periodic Csd" Dialog.
  - In the table, for the date column, use the "long format", as users mentioned ambiguity for some short format (e.g. US) 
  - Outdated entries (date < tomorrow) are now shown with grayed strikedout font (instead of just grayed)  
  - Remove the warning about outdated entries and Future values conversion, as it takes too much space in the Dialog.
- In VisualizeOccurrence Dialog
  - Add some colors to the text at the top, along with index markers 
  - Remove the cumulative amount display, as comments indicates it was not useful
- In Edit Scenario Dialog 
  - The "New Periodic..." and "New Irregular..." buttons have been merged into a single one "Add". When click, a menu appears allowing to choose "Periodic" or "Irregular". The goal is to declutter the Dialog
  - The Amount column no longer uses monospace font; this fixes a slight vertical alignment issue.
- In MainWindow
  - When viewing the Change Log, User Manual, or Quick Tutorial PDF, the cached copy in the temp directory is now validated by SHA-1 checksum against the embedded resource (instead of file size). This ensures an updated PDF is always extracted even if the new version happens to have the same size as the previous one.
  - Change the way the example file is created and opened. When "Open example" menu item is selected, a temporary file is created in the OS temporary directory, OVERWRITING any existing file with the same name. There is no more unique ID attached to the file name.
  - Change the Y=0 line to a dashed line, for better visibility
  - Add an **Anonymize** feature (menu Tools) to replace sensitive scenario data with placeholder values. Amounts can optionally be randomized via an intensity slider. Changes are in-memory only until explicitly saved.
  - Add a **Show gridlines** checkbox (checked by default) in the chart toolbar. When unchecked, X and Y axis grid lines are hidden on the Cash Balance chart.
- In Options Dialog
  - In the "Light mode colors" and "Dark mode colors" sections, add a **Gridlines color** row to set the color of the X and Y grid lines on the Cash Balance chart independently for each mode. Default is RGB(232, 232, 232) for light mode and RGB(60, 60, 60) for dark mode. The setting is persisted in the INI file under the keys `gridlines_color_light_mode` and `gridlines_color_dark_mode`.
  - All color rows in the "Light mode colors" and "Dark mode colors" sections now have a **↺ reset button** that instantly reverts the color to its factory default, without closing the dialog.
  - Color values displayed next to each color button are now zero-padded to 3 digits (e.g. `R:005 G:064 B:128`), making all color labels the same width for easier reading.
- In About Dialog
  - Add the current workspace
  - Separate the "info" from "How it is"
  - The "About" tab content now correctly uses the application font instead of a hardcoded font (Noto Sans 16pt).
  - Config file path and log file path are now displayed using the native path separator (backslash on Windows, forward slash on Linux/macOS).
- In CSD Breakdown Dialog
  - Row numbers (vertical header) have been removed from both the monthly and yearly breakdown tables.
  - The Amount column no longer uses monospace font; this fixes a slight vertical alignment issue.
- General
  - All platforms
    - UI layout has been optimized furthermore to better support big application font (e.g. 24 point size) while still offering nice appearance.
    - The application font selected applies to everything, including menu items and toolbars (thus bypassing KDE configuration for those using KDE under Linux). This has been done for consistency across all platforms.
    - Workspace Support:
      - New `-workspace=WORKSPACE` command-line argument to run multiple isolated instances of the application
      - Each workspace uses separate settings file (`graphical-budget-planner_WORKSPACE.ini`)
      - Each workspace generates separate log files (`YYYY-MM-DD__HH_MM_SS_WORKSPACE.txt`)
      - Workspace identifier can be 1-20 alphanumeric characters (letters and digits only)
      - Allows users to maintain different configurations for different use cases (e.g., testing, production, personal accounts)
    - Workspace Management:
      - New `-workspace_list` command-line argument to list workspace configurations
      - Lists all workspace configuration files found in the config directory
      - Displays workspace ID, file size, last modified date, and full path for each config
      - New `-workspace_cleanup` command-line argument to remove all non-default workspace .ini files
      - Helps users manage and clean up unused workspace configurations
      - Prevents accumulation of orphaned configuration files

### Known Issues

- On Ubuntu Gnome Linux (tested on 22.04, 24.04 and 26.04), secondary windows (e.g. popup dialogs) are tied by default to the Main Window and cannot be moved. To be able to move them (which is recommended as it adds flexibility), you can use the Gnome Tweaks application : go to Windows->Attach Modal Dialog and toggle to OFF. Another approach is to type the following command in a terminal : gsettings set org.gnome.mutter attach-modal-dialogs false
- In extreme scenarios (meaning that do not correspond to expected use cases of GBP), where really extremely large data set is generated (e.g. generate many events per day, every day, for 100 years), there could be a noticeable lag between the moment you click on a point of the Cash Balance curve and the relevant info is displayed in the right panel. This depends heavily on the speed of your computer. The next 1.8 release will address the speed issue (expected end of 2026).

## 1.6.3 to 1.7.0 (02-dec-2025)

### New features : The big ones

- In Analysis module :
  - A new section has been added, dedicated to analyze the **Tags** associations with the Cash Stream Definitions and provide useful statistics.
  - The Relative Weight section has received several noteworthy improvements.
- In Options module : 
  - Add a “reset” option to set all the options to “factory settings”
  - Add an option to follow the desktop theme for charts foreground and background colors.
- General :
  - For Linux :
    - GBP can now more closely match the selected desktop theme. Dark themes will be respected on most platforms (Gnome, Kde, Cinnamon, XFCE). 
    - Wayland support has improved
  - For Windows :
    - By default, GBP uses the "Cross-Platform" look. Although it does not perfectly match the native Windows 11 look, it avoids several compatibility issues present in Qt and Windows 11. This default choice may be revisited in future GBP releases. If you prefer, you can manually select the alternative Windows Vista or Windows 11 Native themes. Run gbp -h for more information. 
  - All platforms:
    - Colors have been optimized to look great by default on either light or dark themes. The Options dialog now also allows you to customize the colors used for income and expense amounts. With this addition, virtually all color-based elements in GBP can be personalized.
    - Add several command line flags to customize the look or behavior of GBP : type "gbp -h" for details.

### New features : The details

- In Analysis Dialog :
  - Add a new analysis module dedicated to Tags (tab "Tags"). It enables tracking the contributions of user-chosen tags over a user-specified period.
  - For Relative Weight module
    - Legend has been detached and put in its own listbox to the right of the Pie Chart, in order to support large no of items using scroll bar.
    - Clicking on 1 element of the legend now makes the corresponding slice stand out and its rank, amount and percentage are displayed just below the legend.
    - Selecting multiple legend elements show the sum of their respective amounts and percentages.
    - The pie can be rotated using the slider control, in order to optimize the placement of outer labels.
    - Labels can be turned off.
    - The set of colors for slices has been improved to better support both dark and light background.
  - For Annual and Monthly report tables, add a new column displaying the cumulative cash balance at the end of each period. This will also be added by the export to CSV files.
- In Main Window :
  - The Message toolbar at the bottom of the screen has been removed, thus giving more useful space for the curve. 
  - In the upper toolbar dedicated to set the X axis range, add a "till the END of the YEAR" (EOY) button, to set the X date range from Tomorrow to EOY.
  - A new "Reload" menu item has been added to the "Scenario" menu. It is used to reload the current scenario from disk, discarding the unsaved modifications that have been made.  
- In Edit Scenario Dialog,
  - Add an explicit combobox to select the "combination mode" of the selected filter tags (AND, OR, NOT). This combobox was previously located in the Tag Selection Dialog (radio buttons).
  - "Filter Tags" string is now in blue, in order to clearly differentiate it from the other type of filters on the left.
  - Currency name is now in English or French (in 1.6.3 and before, it was in the currency's country native language, which is inconsistent with the 2 languages supported by GBP at this moment)
- In Edit Irregular Csd
  - Add the option to export all the incomes/expenses defined in a CSV file.
- In Date Edit Dialog, 
  - Add a "Tomorrow" button for the "From" date to set it to the real tomorrow. 
  - Add an "End-of-Year" button for the "To" date to set it to the last day of the current year.
- In Options Dialog,
  - Add the capability to let the user choose the colors for incomes/expenses labels or bar charts in the application. The default is optimized to provide a good compromised for both light and dark themes. This option has been added because user may which to choose a color that is optimal for his own theme (e.g. a particularly vibrant green for income in a dark theme) instead of the default one.
  - Add a "reset" option to set all the options back to "factory settings"
  - For Charts theme, transform the "use dark theme" checkbox into "Charts Theme" options set : 1) Force light theme 2) Force dark theme and 3) Follow system desktop theme.
- Global : 
  - The maximum value for an amount is now a 14-digit integer instead of 15 as for 1.6.3 and below. It means for example that for currency with 2 decimals (USD, CAD, Euro), the maximum value allowed is now  (1 trillion - 0.01). For Yen (0 decimal), it is (100 trillion -1)
  - _Gnome/Cinnamon Wayland_ : 
    - Because of design choices made by their respective development teams, these desktop environments do not implement Server-Side Decoration, resulting in borderless windows. Until Qt provides a reliable implementation of Client-Side Decoration, the best option for GBP when running under GNOME or Cinnamon (e.g., Ubuntu GNOME, Fedora GNOME, Linux Mint, LMDE) is to use XWayland—a compatibility layer based on X11. This is the default behavior in GBP. GBP runs perfectly well under XWayland, but it does not benefit from the additional features provided by native Wayland. If you wish to force Wayland, launch GBP with the command-line argument: -xwayland
  - Windows :
    - GUI look is set to "Fusion" by default. To have Windows 11 native look or Windows Vista look, see gbp -h
  - All exports to CSV files now default to ".csv" as extension (and not .txt)
  - Export Dialogs use as default the last export directory used.
  - Import Dialogs use as default the last import directory used.
  - The "alternate color mode" for most listboxes and tables have been removed, because on some platforms (e.g. Windows), it was more distracting than helpful. The only exception are tables in Analysis module for Monthly And Yearly reports.
  - Log system has been enhanced
    - Log files are now formatted as CSV files ready to import in a spreadsheet application (same .txt extension though) and 
    - New available command line switches : 
      - -logverbosity=NORMAL or -logverbosity=DEBUG  Default is NORMAL
      - -logprivacy=PUBLIC_ONLY or -logprivacy=ALLOW_PRIVATE  Default is PUBLIC_ONLY
- New command line options have been added : here is the whole set (old and new) for 1.7.0 : 
  - -h, --help : Display help message and exit
  - -usesystemfont : Force the use of the default system font (all platforms)
  - -windowslook=fusion : Use "cross-platform" style. This is the default (Windows only)
  - -windowslook=native : Use native Windows 11 style (Windows only)
  - -windowslook=vista : Use Windows Vista style. This is closer to Windows 11 native look, but not quite (Windows only)
  - -xwayland : Force XWayland instead of using native Wayland (Linux only)
  - -noxwayland : Prevent switching to XWayland, use native Wayland only (Linux only)
  - -locale=LANG-TERRITORY : Override system locale (e.g., -locale=en-US) (all platforms)
  - -logprivacy=ALLOW_PRIVATE : Allow sensitive data in logs (all platforms)
  - -logprivacy=PUBLIC_ONLY : Hide sensitive data from logs, default (all platforms)
  - -logverbosity=DEBUG : Set verbose debug logging (all platforms)
  - -logverbosity=NORMAL : Set normal logging verbosity, default (all platforms)

### Fixes

- Theming : For Linux Gnome/Cinnamon (e.g. Ubuntu 24 / 26, Fedora 43 Gnome, Linux Mint, etc), GBP can now detect and apply some elements of the system theming, like dark/light mode. However, it still cannot detect and adapt to the _accent color_ chosen in the system configuration. For KDE system however (e.g. Tuxedo OS, Fedora 43 KDE, OpenSuse, etc), all configuration settings related to theming are respected. 
- A long standing (but minor) bug has been fixed : 2 error messages were written on the console when the application is started. They do not appear anymore. Although without consequence, this was annoying :
  - .qt.core.qmetaobject.connectslotsbyname: QMetaObject::connectSlotsByName: No matching signal for on_actionClear_List_triggered()
  - qt.core.qmetaobject.connectslotsbyname: QMetaObject::connectSlotsByName: No matching signal for on_actionRecentFile_triggered()
- Improves the error messages displayed when trying to open a scenario file for which the user has no permission.
- Improves French translation for Present Value Calculator
- This version of GBP has been compiled with Qt 6.10.1, so some Wayland (on Linux) bugs have been fixed by Qt.

### Known Issues

- On Ubuntu Linux systems (tested on 22.04 and 24.04), secondary windows (e.g. popup dialogs) are tied by default to the Main Window and cannot be moved. To be able to move them (which is recommended as it adds flexibility), you can use the Gnome Tweaks application : go to Windows->Attach Modal Dialog and toggle to OFF. Another approach is to type the following command in a terminal : gsettings set org.gnome.mutter attach-modal-dialogs false
- In extreme scenarios (meaning that do not correspond to expected use cases of GBP), where really extremely large data set is generated (e.g. generate many events per day, every day, for 100 years), there could be a noticeable lag between the moment you click on a point of the Cash Balance curve and the relevant info is displayed in the right panel. This depends heavily on the speed of your computer. The next 1.8 release will address the speed issue (expected end of 2026).

## 1.6.2 to 1.6.3 (29-mar-2025)

### New features

- Change reference to the new repo site (now on Github : https://github.com/redmoon1945/gbp)
- Add a Present Value Calculator in "Tools" menu"

## 1.6.1 to 1.6.2

### Fixes

- Correct the "too-small" width of menu items when a very big font is used. The problem was due to Qt, but a workaround has been found.

## 1.6.0 to 1.6.1

### New features

- Analysis - Annual and monthly graphical report
  - Add Statiscal Info (Mean, Standard deviation and Sum) to the displayed data
  - Change slightly the layout to add more vertical space for the graph, which is useful when large font are used (Y axis label have more space to extend and show the labels)

### Fixes

- A change in 1.6.0 introduced a bug that made the Main Window "inactive" when the Edit Scenario Dialog was hidden.

## 1.5.3 to 1.6.0

### New Features

- **Introducing the notion of tags**, which is a way to implement categories of incomes/expenses, but in a much more flexible and powerful way.
  - In Edit Scenario Dialog, add a **Manage Tags** buttons in the bottom left, which pops up a Dialog allowing to edit tags and their relationships with Cash Stream Definitions
  - In Edit Scenario Dialog, add a new filter "**Tag Filtered**", allowing to see only Cash Stream Definitions that are linked to a modifiable user-defined set of tags
  - In Edit Periodic and Irregular Cash Stream Definitions, show the associated tags (read only)
- In About Dialog, add a new Contact tab for those who wish to know how to contact directly the author(s) of this software.
- In MainWindow,
  - Add a line at value Y=0, with user-specified color. This helps to identify rapidly if a point is above or below 0. Can be turned off in Options Dialog.
  - Add the file name of the scenario file opened in the top window title (chopped to 50 char max)
- In Options Dialog,
  - Reformat the Chart tab layout for better clarity
  - Add an option to show or hide the line at value Y=0. Default = Yes.
  - Add options to select the color of the Y=0 line, both in Dark and Light mode
  - Add an option to control how the X-Axis Dates for charts are formatted (Locale, ISO 8601, ISO 8601 with 2-digits year)
  - Add an option to show or hide tooltips for the whole application. Default is "Show"
  - Make the Dialog modal
- In Edit Scenario Dialog
  - Rework the filters controls, so it is easier to
    undertand and it takes less visual space.
  - It is now possible to select several Cash Stream Definitions and in one shot set the color of theirs names ("Set Color" button)
- In several Dialogs, optimize the spacing and position of the UI components to provide more space to the list of incomes/expenses, while maintaining the same window size.
- In all Dialogs, use short format for dates, because the long one takes up too much space when big fonts are used.

### Fixes

- When no scenario is loaded and a change is made on the Start Amount, the program crashes.

### Known Issues

- On Ubuntu systems (tested on 22.04 and 24.04), secondary windows (e.g. popup dialogs) are tied by default to the Main Window and cannot be moved. To be able to move them (which is recommended as it adds flexibility), you can use the Gnome Tweaks application : go to Windows->Attach Modal Dialog and toggle to OFF. Another approach is to type the following command in a terminal : gsettings set org.gnome.mutter attach-modal-dialogs false
- When a very large font is used as the application font (e.g. size 24), the menu in the Main Window is not wide enough to show the menu items texts. This is an issue with Qt itself and no solution has been found at the moment.
- In extreme scenarios (meaning that do not correspond to expected use cases of GBP), where really extremely large data set is generated (e.g. generate many events per day, every day, for 100 years), there could be a noticeable lag between the moment you click on a point of the Cash Balance curve and the relevant info is displayed in the right panel. This depends heavily on the speed of your computer. There is no easy way to speed up things internally, so I do not expect any improvement on this behavior.
- GBP is a Qt 6 application. Unfortunately, at the time of this writing, theme setting in some Gnome-based distributions (like Ubuntu 22.04) cannot be understood by Qt 6 application (they wont react to the change made to the theme). You can alleviate the problem by using the "Dark/light mode" settings in GBP Options menu : this will affect the rendering of all the charts in the application.

## 1.5.2 to 1.5.3

### Fixes

- Minor UI adjustments to better support very big fonts
- Improve the way GBP detects the default system font on Linux
- When no scenario is loaded, trying to launch the Analysis Module craches the application

## 1.5.1 to 1.5.2

### Fixes

- When a new scenario is created right after the launch of the program (without loading first an existing scenario), trying to see the occurences
  for a new income/expense would crash the program.

## 1.5.0 to 1.5.1

### Fixes

- In MainWindow, set limits to zoom in/out operations for the Cash Balance curve, in order to prevent insane levels (like zooming out 10000 years)

## 1.4.1 to 1.5.0

### New Features

- In MainWindow, change the zooming mechanism with middle mouse button, so that the zoom is made around the mouse position and not the center of the chart
- In MainWindow, upper toolbar, add the button "4Y"
- In MainWindow, add a new horizontal-only zooming, by pressing the SHIFT key when rotating the mouse wheel.
- In MainWindow, add a new vertical-only zooming, by pressing the CTRL key when rotating the mouse wheel.
- In Edit Periodic/Irregular Financial Stream  Definition, add the capability to export the occurences to a CSV file.
- In Option Dialog, add the option to localize the dates for all CSV exports. 

### Fixes

- In MainWindow, change the Y axis font of the Cash Balance chart to "mono" type, to prevent small jittering when values change.
- In MainWindow, export to CSV, localize the date format according to System Locale.

## 1.4.0 to 1.4.1

### Fixes

- In EditScenario, when "Apply" button is clicked, refresh the Cash Balance curve while maintaining the X axis scaling as it was.
- In MainWindow, when baseline amount is changed, refresh the Cash Balance curve while maintaining the X axis scaling as it was.

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

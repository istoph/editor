# Changelog chr editor
======================

## 0.1.80 (2025-04-07)

  * New Upstream Release termpaint (0.3.1)
  * New Upstream Release tuiwidgests (0.2.2)

  * Add QCommandLineOption -s to setup inline mode.
  * Add SearchDialog: Add F3 to search.
  * Add tests/meson.build and make it adjustable via the build option -Dtests=true
  * File: After copying or press ESC, the select mode (F4) is ended.
  * Extends meson by the function to set rpath with -Drpath=.
  * Add Line Marker support.

## 0.1.79 (2024-06-01)

  * Fix: meson: Add timeout 180 for debian riscv64 tests.

## 0.1.78 (2024-04-17)

  * Fix: Delete Key wirh MultiCursor
  * Fix Test: Include catch2 header from system and respect CATCH3
  * Add meson.build: Run tests in verbose mode when supported.
  * Add meson.build: Register tests executable
  * Add meson.build: Install manpages
  * Add +/searchword to start seach on open file

## 0.1.77 (2024-02-26)

  * Fix: visibility help message

## 0.1.76 (2023-12-27)

  * Fix missing ifdef SYNTAX_HIGHLIGHTING for file commands

## 0.1.75 (2023-12-23)

  * Debian Relese 0.1.75
  * Rename: editor executable to chr
  * Add QStandardPaths for ~/.config/chr and ~/.cache/chr/chr.json
  * Rename: configuration file options to use snake case.

## 0.1.74 (2023-12-23)

  * Fix: Editor,File: Make sure to update search count after replace or replace all (closes: #9)
  * Fix: Make selection of trailing spaces visible again when using syntax highlighting.
  * Fix: copy and paste error
  * Fix: inverted parameter usage in updateSettings
  * Add: Commandline and Config Option: -w WordWrap|WrapAnywhere|NoWrap (Close: #19)
  * Add: SearchDialog: Add more escape sequences that are translated.
  * Add a minimal help (quick start)
  * Switch default for highlight_bracket to true.
  * Remove: Ctrl-K "cutline" edit command.

## 0.1.73 (2023-11-06)

  * Fix: Scrol Window with wrap long lines
  * Fix: Opening files that cannot be read
  * Fix: Up/Down on first and last line
  * Add: SaveDialog can handel file path

  * Rename: DOSMode to crlf Mode
  * Migrate: Cursor to TuiWidges
  * Migrate: Dukument to TuiWidges
  * Migrate: File to TuiWidges

## 0.1.72 (2023-09-20)

  * Add: Syntex Heileiting (Add -Dsyntax_highlighting=true)

  * Fix: Search Dialog interaction
  * Fix: Search Regex Support
  * Add: Search Multiline
  * Fix: move multicursor on paste text.
  * Update man

## 0.1.71 (2023-08-01)

  * Fix: StatusBar
  * Fix: Change terminal title on f6
  * Fix: Change context for F3 and Shift-F3 shortcuts to window.

  * Preparation for syntax heileiting and multi cursor.

## 0.1.70 (2023-04-11)

  * Fix: stdin
  * Fix: menu for copy paste if you change the activ window.
  * Fix: search WrapAround.
  * Fix: ScrollBar color.
  * Fix: scrollin long lines.
  * Add: character border.

## 0.1.69 (2023-03-27)

  * Fix: gotoline when starting the editor.
  * Fix: SearchDialog: Remove duplicated mnemonic.
  * Fix: StatusBar::paintEvent: change unsafe icon in status bar
  * Fix: File::insertText: cursor position after insert text with new lines.
  * Fix: Scrollbar: color when characters ar printed on the bar.
  * Add: git commit id in version number. See chr -v.
  * Add: ctrl + d support, do delete a single line.
  * Add: ctrl +,e, v and Ctrl + e, h shortcuts for tile vertical and horizontal.
  * Add: When displaying the version number, the Git-Version is now also displayed.
  * Add: About dialog.
  * Refactor: Document, File, FileWindow and TextCursor.

## 0.1.68 (2023-01-28)

  * Fix: OpenDialog
  * Fix: CloseDialog
  * New Upstream Release TuiWidgets

## 0.1.67 (2022-06-02)

  *  Fix: File: The first field in an empty row is now also highlighted to show it visually.
  *  Fix: File: Fix crash due to recently added delayed terminal attachment in tui widgets.
  *  Add: File::paintEvent: Mark the line number bold at the current cursor position.

  third party:
  *  Switch to tui widgets version of window layout (WindowLayout -> Tui::ZWindowLayout)
  *  Switch to use ZVBoxLayout instead of VBoxLayout. And also for ZHBoxLayout.

## 0.1.65 (2022-04-07)

  * Fix: SaveDialog
  * Doc: Update manpage

## 0.1.64 (2022-02-12)

  * Fix: Toolkit: Avoid infinite loop with wrapping, visual spaces and width 0.
  * Fix: OpenDialog path.
  * Ref: Categorize file type.

## 0.1.63 (2021-12-22)

  * Fix: File: of by one if selcet all.
  * Fix: File: Scroll up and down and move curso.
  * Fix: Shift + insert shortcuts.
  * Fix: Remove: Unused and broken shortcuts.
  * Fix: Opendialog: change to dir without backslash at end of path.
  * Fix: File::copy: Do nothing in multi insert mode.
  * Add: WordWrap Support.
  * Add: WrapDialog.

## 0.1.62 (2021-11-24)

  * Fix: TextLayout
  * Fix: InsertCharacter: fix insert all chars. fix priview box.
  * Fix: SaveDialog: sort tab order.
  * Fix: File: fix viewWidth without shift line numbers
  * Fix: File: fix doLyout without wrap option.
  * Fix: Editor: Add pendingKeySequence handler.
  * Fix: File: fix singel char for double press home
  * Add: FileWindow: Add windows options for manual placement, close and return to automatic placement.
  * Add: FileWindow: Use closeSkipCheck in closeRequested handling.
  * Add: FileWindow: Adjust visible edges based on position.
  * Add: FileWindow: Save vertical scrollbar as member.
  * Add: FileWindow: connect add contextobject.
  * Add: ZWindow::AutomaticOption to all dialogs. Also Add move to OverwriteDialog.

## 0.1.61 (2021-11-09)

  * Add: multi document interface (mdi)
  * Add: Multi window sub menue
  * Add: Adjust file window edge for horizontal tiling
  * Add: Allow manually moving file windows

## 0.1.60 (2021-10-13)

  * Add Formatting dialog.

## 0.1.59 (2021-10-10)

  * Fix: Update statusbar if file save as.
  * Round Trip: QSaveFile does not take over the user and group.
  * Add: OpenDialog should be started in the same folder as the active file.
  * Add: Show hidden files in save as dialog.
  * Add: Eat Space Befroe Tab.
  * Add: Move and close options to dialogs.
  * Add: Open and Save Dialog switched to FileModel.

## 0.1.58 (2021-10-06)

  * Fix: Search and Repace Dialog do not crash if file not exist.
  * Fix: and refector: UndoStaps on inital or file open.
  * Fix: If file is ro, now can by save as in close dialog.
  * Fix: FileWindow: Delete close confirm dialog when exit is selected too.

## 0.1.57 (2021-09-23)

  * Fix: File watcher after reload file.
  * Fix: Save and Save as in all windows.
  * Add Sorrt Seleced Lines Support.
  * Add Save as OverwriteDialog.
  * SaveDialog: Sort DirsFirst.
  * ConfirmSave: Different message for confirm types.
  * Enable reload in menu if file has been externally changed.

## 0.1.56 (2021-09-07)

  * Fix WrapLongLines with Linenumbers.
  * Multi Window HVF Suppot.

## 0.1.55 (2024-08-24)

  * Multi Window Suppot.

## 0.1.53 (2021-07-20)

  * Fix SaveDialog: Save Path.
  * Fix Safedialog: Redundant multiple separators.
  * Add ConfirmDialog for Reload.
  * Add Menu shortcuts.

## 0.1.52 (2021-06-11)

  * Fix: OpenDialog: Folders cannot be opened.
  * Fix: The behavior of no line at the end of the file.
  * Add: Use default placement for dialogs.
  * Add: Editor::facet: Require exact match for clipboard facet.

## 0.1.51 (2021-06-04)

  * Fix: F-Keys trigger delselect.
  * Add: hidden checkbox, fix sort order when refresh.
  * Adapt to listview changes.
  * Update: Statusbar.

## 0.1.50 (2021-05-08)

  * Add: Theam support
  * Upstream fixes

## 0.1.49 (2021-02-07)

  * [ edr ] Typos in the manual corrected.
  * Add: Alt+Shift+S for sort selected lines

## 0.1.48 (2021-01-18)

  * Add: regex support in search and replace
  * Fix: jumping when entering search queries
  * Fix: F6 brings active windows to the foreground.
  * Update: Manual

## 0.1.47 (2021-01-11)

  * Fix: check if clipboard is empty

## 0.1.46 (2021-01-07)

  * Update termpaint: flush: leave terminal with reset SGR state.
  * Rename playground to inputevents.

  Thu, 07 Jan 2021 20:26:27 +0100

## 0.1.45 (2020-11-18)

  * Fix: BlockSelect Tab
  * Fix: Ctrl + Key

## 0.1.44 (2020-11-16)

  * Add: BlockSelect
  * Add: Alt+X commandline
  * Fix: KDE Konsole
  * Hotfix: 4 Byte Char
  * Update: Manual

## 0.1.43 (2020-10-29)

  * Hotfix: Cousor position can not be smaller than zero
  * Fix: Colors in status bar adjusted for special characters
  * Shift tab can only be applied to complete lines. Therefore it can always be
    executed
  * Fix: select from bottem to top for Tab/ShiftTab and Selected KeyUp/Down to
    selectLines methode
  * Add option: select_cursor_position_x0

## 0.1.42 (2020-10-14)

  * Fix termpaint: Fix surface_vanish_char access beyond rightmost character of
    surface.

## 0.1.41 (2020-10-14)

  * Add Line Number support
  * Add menu: Select Mode
  * Fix InputBox half cut char
  * Fix cursor position during reload
  * Fix QFileSystemWatcher::removePaths: list is empty

## 0.1.40 (2020-10-12)

  * The status bar is colored yellow if a file has changed outside the editor
  * Data input by stdin can now be interrupted in the menu
  * Files can now be reloaded from the menu
  * Copy and paste can now also be used in the search dialog

## 0.1.39 (2020-10-08)

  * Add playground, detect and keyboardcollector to deb files

## 0.1.38 (2020-10-06)

  * Moved the search in a thread
  * Add select mode with F4 for consoles with suppressed shift key

## 0.1.37 (2020-10-03)

  * faster insert log lines
  * detect nullbyte char delivered by stdin
  * fix: also accept numpad enter as enter key

## 0.1.36 (2020-09-18)

  * Add loggin after editor was closed
  * Create directory for status information if not exist

## 0.1.35 (2020-09-06)

  * Round trip for ZOC terminal

## 0.1.34 (2020-07-14)

  * Add tab to spaces converter in Tab Dialog
  * Add Insert Character dialog to add utf-8 character
  * Add DOS Mode in Save As Dialog

## 0.1.33 (2020-07-04)

  * Display special characters
  * Edit files with parts that are not utf-8 encoded
    (open / save without further changes should not break the file)
  * Saves the last refuelled position in ~/.cash/chr.json
  * Fix: open no newline at end of file
  * Fix: status bar for new file

## 0.1.31 (2020-06-08)

  * Fix: add group undo
  * Fix: in delSelect

## 0.1.30 (2020-05-28)

  * Fix search and replace, if nothing is found, nothing must be selected
  * Fix Typo

## 0.1.29 (2020-05-27)

  * Set window title with * if modified change
  * Update manual

## 0.1.28 (2020-05-24)

  * Add ReplaceDialog

## 0.1.27 (2020-05-23)

  * Add live search support
  * Bugfixes

## 0.1.26 (2020-04-18)

  * Bugfix: View last line

## 0.1.25 (2020-03-31)

  * The clipboard must not be deleted if nothing is selected.

## 0.1.24 (2020-03-25)

  * Highlight Bracket

## 0.1.23 (2020-03-23)

  * Default value for crl+f
  * Paste replaces the selected text
  * With horizontal scroling, the position of the cursor is stored
    temporarily and characters of different lengths are noted.
  * fix: termlib

## 0.1.22 (2020-03-12)

  * Moved lines should not be selected
  * show RW or RO status in statusbar
  * Added distinction between highlighted and focus ListView.

## 0.1.21 (2020-03-14)

  * Intercept copy and paste errors that contain special characters "·,→,¶"
  * Show RW status in statusbar.

## 0.1.20 (2020-02-27)

  * fix alert box
  * update manual

## 0.1.19 (2020-02-22)

  * add folloing support with option -f
  * add multi press pos1
  * add no newlien support

## 0.1.18 (2020-02-20)

  * add support read of stdin
    statusbar support, append mode and following mode for stdin
    update manual

## 0.1.17 (2020-01-20)

  * Add German manual

## 0.1.16 (2020-01-19)

  * Fix manual path

## 0.1.15 (2020-01-10)

  * Search Privius
  * Fix Search first line
  * Selected tab support

## 0.1.14 (2020-01-10)

  * Open SearchBox with Crl+F, find the next element with F3
  * Color find element
  * New background scrolbars for mous mode copy
  * Change window style

## 0.1.13 (2019-11-14)

  * add first version of search function

## 0.1.12 (2019-11-04)

  * File: fix scrolling with wraplines in the visibile area
  * File: Base calculation of horizontal scrollbar only on currently visible
    lines.
  * File: fix bigfile size

## 0.1.11 (2019-10-24)

  * new pipline
  * termlib fixes

## 0.1.10 (2019-10-09)

  * remove wrong rpmbuild dir
  * add manpages support

## 0.1.9 (2019-10-08)

  * new termlib fixes
  * debian compat == 11

## 0.1.8 (2019-09-15)

  * Don't get confused by std in/out/err where isatty is true but not opened
    for read and write.
  * File::delSelect remove selected linebrake

## 0.1.7 (2019-09-04)

  * update libtui

## 0.1.6 (2019-07-27)

  * rework undostaps

## 0.1.5 (2019-06-17)

  * fix horizontel scroling
  * add delet word

## 0.1.4 (2019-04-10)

  * set curser shape and color

## 0.1.3 (2019-02-25)

  * new color 8bit for pterm and co
  * new color for vt
  * new color for formatingChar
  * fix option +n for line number

## 0.1.2 (2019-02-05)

  * Bugfix crash at print message and quit on open big files
  * Menue color in SW Mode
  * Add Shortcut STRG+q

## 0.1.1 (2018-11-28)

  * Initial release.


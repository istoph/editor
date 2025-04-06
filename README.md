[![License](https://img.shields.io/badge/License-Boost_1.0-lightblue.svg)](/COPYING)

A terminal based text editor.

Keyboard shortcuts are similar to the default editors in Gnome, KDE and other
desktop environments. This is to ease workflows alternating between GUIs
and terminal.

The look and feel is a blend of modern GUI editors and late 90s PC text mode
editors (e.g. Turbo Vision based or edit.com) adapted to fit into terminal
based workflows.

It has been written from scratch using [Tui Widget](https://tuiwidgets.namepad.de/)

![Screenshots](https://blog.chr.istoph.de/wp-content/uploads/chr.edit_20230920.png)

## Contribute

This project is always open for contributions and welcomes merge requests.
Take a look at our [issue tracker](https://github.com/istoph/editor/issues)
for open issues.

### Technologies / Dependencies

Dependencies in Debian trixie:
```
build-essential meson ninja-build pkg-config qt5-qmake qttools5-dev-tools qtbase5-dev libtermpaint-dev libtuiwidgets-dev libposixsignalmanager-dev
```

For third-party syntex highlighting you need the compile
option: `-Dsyntax_highlighting=true` and the following additional dependencies:
```
libkf5syntaxhighlighting-dev libkf5syntaxhighlighting-tools cmake
```

Additional options are:
* `-Dtests=true` for switching build tests on and off.
* `-Dsystem-catch2=enable` to use catch as a system library (only use for tests).


See also [Tui Widgets](https://tuiwidgets.namepad.de/)
and [Termpaint](https://termpaint.namepad.de/)

```
git clone https://github.com/istoph/editor
cd editor
meson setup _build
meson compile -C _build
meson install -C _build
```

For more details see: [doc](https://github.com/istoph/editor/tree/main/doc/build/README.md)


## Doku
```
man(1)                           chr man page                           man(1)

NAME
       chr - chr is a terminal based editor

SYNOPSIS
       Usage:  chr  [options]  [[+line[,char]] file …] [[+/searchword] file …]
       [/directory …]

       Options:
         -h,  --help                                        Displays  help  on
       commandline
                                                          options.
         --help-all                                        Displays  help  in‐
       cluding Qt
                                                          specific options.
         -v, --version                                    Displays version
                                                          information.
         -l, --line-number                                The line numbers are
                                                          displayed
         -b, --big-file                                    Open  bigger  files
       than 100MB
         -w,   --wrap-lines  <WordWrap|WrapAnywhere|NoWrap>   Wrap  log  lines
       (NoWrap
                                                          Default)
         --attributesfile <config>                        Safe  file  for  at‐
       tributes,
                                                          default
       ~/.cache/chr/chr.json
         -c, --config <config>                            Load customized con‐
       fig file.
                                                          The  default  if  it
       exist is
                                                          ~/.config/chr
         --syntax-highlighting-theme <name>               Name of
                                                          syntax-highlighting-
       theme,
                                                          you  can  list   in‐
       stalled themes
                                                          with:   kate-syntax-
       highlighter
                                                          --list-themes
         --disable-syntax                                 disable syntax high‐
       lighting
         -s, --size <N|auto>                              Size <N> in lines or
       <auto>
                                                          for Automatic detec‐
       tion of
                                                          the  size   of   the
       lines with
                                                          which  the editor is
       displayed
                                                          on the console.

       Arguments:
         [[+line[,char]] file …]                          Optional is the line
       number
                                                          and  position.  Sev‐
       eral files
                                                          can   be  opened  in
       multiple
                                                          windows.
         [[+/searchword] file …]                          A search word can be
       set.
         [/directory …]                                   Or a  directory  can
       be
                                                          specified  to search
       in the
                                                          open dialog.

DESCRIPTION
       Chr is a terminal based text editor.

       Keyboard shortcuts are similar to the default editors in Gnome, KDE and
       other desktop environments. This is to ease workflows  alternating  be‐
       tween GUIs and terminal.

       The look and feel is a blend of modern GUI editors and late 90s PC text
       mode  editors (e.g. Turbo Vision based or edit.com) adapted to fit into
       terminal based workflows.

Quick Start
       The important operations can be invoked from the menu. The menu can  be
       opened with F10 or with Alt together with the highlighted letter of the
       menu item (e.g. Alt + f).

       In dialogs Tab is used to navigate between the elements. Use F6 to nav‐
       igate between windows/dialogs.

       Text  can  be marked in most terminals with Shift + arrow key. Ctrl + c
       is used for copying. Paste with Ctrl + v.

       Changes can be saved with Ctrl + s and the editor can  be  exited  with
       Ctrl + q.

SHORT CUTS
       Shift + Cursor
         Selects text

       Ctrl + a
         Selects all text in the document

       Ctrl + c / Ctrl + Insert
         Copies the selected text into the clipboard

       Ctrl + d
         Deletes the current line

       Ctrl + e, up/down/left/right
         Switches  the active window. Use the arrow keys to specify the direc‐
       tion for the next active window.

       Ctrl + e, Ctrl + up/down/left/right
         Change window size

       Ctrl + e, q
         Close an active document

       Ctrl + f
         Open the search dialog

       Ctrl + Backspace
         Delete a word (left from cursor position)

       Ctrl + r
         Open the replace dialog

       Ctrl + n
         Creates a window with an empty document

       Ctrl + m
         Creates a line marker at the left edge of  the  line  to  find  lines
       again.  (Ctrl + , oder Ctrl + .)
         This key combination does not work with all terminals.

       Ctrl + q
         Quits the editor

       Ctrl + Shift + q
         Close  an  active document (the key combination does not work in vte,
       see: Ctrl + e, q)

       Ctrl + s
         Save (or save as for new documents)

       Ctrl + v / Shift + Insert
         Inserts the contents of the clipboard at the current cursor position.

       Ctrl + x / Shift + Delete
         Cuts out the selected text and moves it to the clipboard

       Ctrl + y
         Redos an action that has been undone

       Ctrl + z
         Undoes an action

       Ctrl + Shift + up
         Moves the current selection or line upwards

       Ctrl + Shift + down
         Moves the current selection or line down

       Ctrl + Left
         Jump a word to the left

       Ctrl + Shift + Left
         Selects a word to the left

       Ctrl + Right
         Jump a word to the right

       Ctrl + Shift Right
         Selects a word to the right

       Alt + -
         Open the window menu

       Alt + Shift + up/down/left/right
         Marks the text in blocks. Inserting the clipboard duplicates the text
       per line. If an equal number of lines is marked as to be inserted,  the
       lines from the clipboard will be distributed across the selected lines.

       Alt + Shift + S
         Sort the selected lines (lexicographical by code-point)

       Alt + x
         Opens a command line. Type "help" for help.

       Tab / Shift + Tab
         Indents  a selected block by a tab stop or remove one level of inden‐
       tion

       F3 / Shift + F3
         Find the next or previously search element

       F4
         Toggles the selection mode to allow selecting text in terminals where
       marking with Shift + arrow keys does not work

       F6 / Shift + F6
         Change active window, with Shift in reverse order

       ESC
         Closes an active dialog menu or action.

MENU
File
   New
       Opens a new an empty unnamed document.

   Open
       Opens a file dialog to select a file to be opened.

   Save
       Saves the current status of the file. If the save path is not yet spec‐
       ified, the "Save as ..." dialog is opened.

   Save as...
       A storage location to save the file to can be selected here via a  file
       dialog.

   Reload
       Reloads the current file. All changes are discarded.

   Close
       Closes the active window.

   Quit
       Closes the editor. If there is a file open that has not yet been saved,
       the Save dialog will be opened first.

Edit
   Cut, Copy, Paste, Select all
       Text  can be selected using the arrow keys while holding down the Shift
       key. The entire text can be selected with Select  all.   This  selected
       text  can  then be copied using Copy or cut using Cut. With Paste, this
       text can be inserted again at the current cursor position. If there  is
       text in the clipboard before copying (or cutting), it will be replaced.

       These  functions use an internal clipboard that contains different con‐
       tent than the clipboard used in the terminal as  copy  and  paste  com‐
       mands, as the editor cannot access the system clipboard.

   Delete Line
       Deletes the entire line.

   Select Mode
       Toggles  the  selection mode to allow selecting text in terminals where
       marking with Shift + arrow keys does not work.

   Undo, Redo
       With Undo or CTRL + z, edits can be undone. With Redo or CTRL +  y  the
       undo can be undone again.

   Search
       Use  Search  or Ctrl + f to open the search dialog. Enter a search term
       in the "Find" field. You can refine the search using  the  options.  If
       live  search  is  activated, the first matching result is automatically
       selected while the search term is being entered. If the  text  document
       is active, you can press F3 to jump to the next result or Shift + F3 to
       jump to the previous result.

   Search Next
       Jump to the next match for the current search term.

   Search Previous
       Jump to the previous match for the current search term.

   Replace
       With  Replace  or CTRL + r the Replace dialog is opened. Enter a search
       term in the "Find" field. In the field "Replace" the  word  to  be  in‐
       serted  is  specified.  "Next" jumps to the next  match for the current
       search term. With "Replace" the current match is replaced.  With  "All"
       all occurrences of the search term are replaced at once.

   Insert Character...
       Opens  a dialog in which a character code (Unicode codepoint) of a spe‐
       cial character to be inserted can be entered.

   Goto
       To jump to a line, open a Goto Line dialog under "Goto".

   Marker
       Creates a line marker in the left margin to quickly  find  lines  again
       when reviewing. Use Ctrl + , or Ctrl + . to jump to the next marker. On
       quit  the  list  of markers is saved in chr.json, so that it can be re‐
       stored when the file is opened.

   Sort Selected Lines
       Sort the selected lines (lexicographical by code-point).

Options
   Tab settings
       Opens the Tab settings dialog. Here the settings for a tab can be made.
       You can choose between tab (\t) and space. You can also set  the  width
       of  the  indention. The default settings can also be set in the ~/.con‐
       fig/chr file. Here you can  specify:  "tabsize=8"  or  "tab=false"  for
       spaces.

   Line Number
       Shows  the line number on the left side of the editor. The default set‐
       tings can also be made in the ~/.config/chr file. Here you can specify:
       "line_number=true".

   Formatting
       In the Formatting dialog, "Formatting  Characters",  "Color  Tabs"  and
       "Color Spacs at end of line" can be switched on and off.

       The  "Formatting  characters"  marks spaces with a dot: "·" end of line
       (\n) with a "¶" and the end of the file with: "♦".

       With "Color Tabs" tabs are colorized. The tab border is made darker.

       "Color Spaces at end of line" is used to spaces mark at the end of  the
       line in red.

       In the configuration file: ~/.config/chr the behavior can be influenced
       with   the   option   "formatting_characters=true",  "color_tabs=true",
       "color_space_end=true".

   Wrap long lines
       Selects if lines that are wider than the window are  displayed  clipped
       or  wrapped.. It can be wrapped at the word boundary or hard at the end
       of  the  line.  This  behavior  can  be  influenced   by   the   option
       "wrap_lines=WordWrap" or "wrap_lines=WrapAnywhere" in the ~/.config/chr
       file.

       In addition, the option "Display Right Margin at Column" can be used to
       specify a numerical value above which the background color is darkened.
       This  value  can also be set with the configuration option: "right_mar‐
       gin_hint=80" in ~/.config/chr.

   Stop Input Pipe
       Reading from a pipe is interrupted. The standard input file  descriptor
       is closed.

   Highlight Brackets
       If  active and the cursor is on a bracket the bracket at the cursor po‐
       sition and the matching other bracket are highlighted.   The  following
       opening  and  closing brackets can be highlighted when the cursor moves
       over them. With the option "highlight_bracket=true" this  behavior  can
       be  influenced  in  the  ~/.config/chr.  Supported  bracket  types are:
       [{(<>)}].

   Syntax Highlighting
       If the editor has been compiled with the "SyntaxHighlighting"  feature,
       syntax  highlighting  is generally available. The language is automati‐
       cally detected when a file is opened and displayed in the  status  bar.
       If  required,  it  can  also be switched on and off or adjusted via the
       syntax highlighting dialog. Syntax highlighting can also be deactivated
       in this dialog.

       The theme can be customized via the command line switch "--syntax-high‐
       lighting-theme". The editor comes  with  the  themes  "chr-bluebg"  and
       "chr-blackbg". If required, a theme from the list that can be displayed
       with  "kate-syntax-highlighter --list-themes" can be used. With the op‐
       tion "syntax_highlighting_theme=chr-bluebg" the theme  can  be  set  in
       ~/.config/chr.

       Syntax  highlighting  can  be  switched  off via the command line using
       "--disable-syntax" when the editor is started. With  the  option  "dis‐
       able_syntax=true" the theme can be set in ~/.config/chr.

   Theme
       It  opens  the  dialog for selecting a theme. The Classic (blue) or the
       Dark (black and white) mode is available. With the option  "theme=clas‐
       sic" or "theme=dark", this can be set in the ~/.config/chr.

Window
   Next, Previous
       Switches the active window, with Shift in reverse order. (See F6)

   Tile Vertically, Horizontally, Fullscreen
       Selects how multiple open documents are shown.

       Vertical and horizontal distribute the available space across the docu‐
       ments.  When Fullscreen is selected only one document is shown at once.
       (See F6)

CUSTOM CONFIG
       The  editor  loads  a  configuration file from ~/.config/chr (if avail‐
       able).  (If the environment variable $XDG_CONFIG_HOME is set, then from
       $XDG_CONFIG_HOME/chr)

       In addition to the options documented above, the following options  are
       available:

   eat_space_before_tabs
       This option is only active if tab=false is set.

       If this option is active and the Tab key is pressed while the cursor is
       in  the  indentation at the beginning of a line, the indentation is ex‐
       tended to the next tab position.

   attributes_file
       Specifies the path of the file in which the cursor and scroll  position
       of files opened in the past is saved.

Default config
       There  is  a default config (~/.config/chr) where the following options
       can be set.
         attributes_file="/home/user/.cache/chr/chr.json"
         color_space_end=false
         color_tabs=false
         disable_syntax=false
         eat_space_before_tabs=true
         formatting_characters=false
         highlight_bracket=true
         line_number=false
         logfile=""
         right_margin_hint=0
         syntax_highlighting_theme="chr-bluebg"
         tab=false
         tab_size=4
         theme="classic"
         wrap_lines="NoWrap"

FILES
       ~/.config/chr
         Your personal chr initializations.

       ~/.cache/chr/chr.json
         History about the changed files. This is where cursor  positions  are
       stored.

BUGS
       Errors  in  this  software  can  be  reported  via  the  bugtracker  on
       https://github.com/istoph/editor.

AUTHOR
       Christoph Hüffelmann <chr@istoph.de> Martin Hostettler <textshell@uchu‐
       ujin.de>

0.1.80                            06 Apr 2025                           man(1)
```

## License
This software is licensed under the [Boost Software License 1.0](/COPYING)

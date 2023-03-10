[![License](https://img.shields.io/badge/License-Boost_1.0-lightblue.svg)](/COPYING)

Console based text editor with special attention to ease of use like gedit. It has been written from scratch using [tuiwidget](https://tuiwidgets.namepad.de/)

![Screenshots](https://blog.chr.istoph.de/wp-content/uploads/chr.edit_.png)

## Contribute

The project is always open for contributions and welcomes merge requests. Take a look at our [issue tracker](https://github.com/istoph/editor/issues) for open issues.

### Technologies / Dependencies

Dependencies in Debian Bookworm:
```
build-essential meson ninja-build pkg-config qt5-qmake qttools5-dev-tools qtbase5-dev libtermpaint-dev libtuiwidgets-dev
```
See also [Tui Widgets](https://tuiwidgets.namepad.de/) and [termpaint](https://termpaint.namepad.de/)

```
git clone https://github.com/istoph/editor
cd editor
meson setup _build
meson compile -C _build
meson install -C _build
```

## Doku
```
NAME
       chr - chr.edit is a console based editor

SYNOPSIS
       Usage: chr [options] [[+line[,char]] file …] [/directory]

       Options:
         -h, --help                      Displays this help.
         -v, --version                   Displays version information.
         -l, --linenumber                The line numbers are displayed
         -a, --append <file>             Only with read from standard input, then
                                         append to a file
         -f, --follow                    Only with read from standard input, then
                                         follow the input stream
         -b                              Open bigger files then 1MB
         -w                              Wrap log lines
         --attributesfile <config file>  Safe file for attributes, default
                                         ~/.cache/chr.json
         -c, --config <config>           Load customized config file. The default if
                                         it exist is ~/.config/chr

       Arguments:
         [[+line[,char]] file …]         Optional is the line number, several files
                                         can be opened in multiple windows.
         [/directory]                    Or a directory can be specified to search in
                                         the open dialog.

FILES
       ~/.config/chr
         Your personal chr initializations.

       ~/.cache/chr.json
         History about the changed files. This is where cursor positions are stored.

SHORT CUTS
       Shift + Coursor
         select text

       Ctrl + a
         selected all text in file

       Ctrl + c or Ctrl + Insert
         copy the selected text

       Ctrl + e, up/down/left/right
         Switches  the active window. Use the arrow keys to specify the location of the next active win‐
       dow.

       Ctrl + e, Ctrl + up/down/left/right
         Change window size.

       Ctrl + f
         open search dialog

       Shift + Backspace
         delete a word (left from cursor position)

       Ctrl + r
         open the replace dialog

       Ctrl + n
         creates a new file

       Ctrl + q
         quits the editor

       Ctrl + s
         save (or save as for new documents)

       Ctrl + v or Shift + Insert
         inset at courser position

       Ctrl + x or Shift + Delete
         cut out the selected text

       Ctrl + y
         Undoes an action that has been undone.

       Ctrl + z
         Undoes an action.

       Ctrl + Shift + up
         move lines up

       Ctrl + Shift + down
         move lines down

       Ctrl + Left
         jump a word to the left

       Ctrl + Shift + Left
         selected a word to the left

       Ctrl + Right
         jump a word to the right

       Ctrl + Shift Right
         selected a word to the right

       Alt + -
         Open the window menu.

       Alt + Shift + up/down/left/right
         Marks the text in blocks. Inserting the clipboard duplicates the text per  line.  If  an  equal
       number of lines is marked as to be inserted, only these will be inserted.

       Alt + Shift + S
         Sort selected lines

       Alt + x
         Opens a hidden command line. Type "help" for help.

       Tab or Shift + tab
         Indent or remove a selected block from a tab.

       F3 or Shift + F3
         Find the next or previously seach element.

       F4
         Toggle the selection mode to support the selection in consoles with suppressed shift key.

       F6, Shift + F6
         change active window, with Shift in reverse order.

       ESC
         Closes an active dialog, menu or action.

MENU
New
       Opens a new an empty unnamed document.

Open
       Opens a file dialog to select a file to be opened.

Save or Save as...
       Saves  the  current  status of the file. If the save path is not yet specified, the "Save as ..."
       dialog is opened. A storage location can be selected here via a file dialog.

Reload
       Reloads the current file. All changes are discarded.

Close
       Closes the active window.

Quit
       Closes the editor. If there is a file open that has not yet been saved, the Save dialog  will  be
       opened first.

Cut, Copy, Paste, Select all
       Text  can  be  marked using the arrow keys and holding down the Shift key. The entire text can be
       marked with (Select all).  This marked text can then be copied using (Copy) or cut  using  (Cut).
       With (Paste) this text can be pasted at the current cursor position. The multiple copying of text
       leads to the loss of the character storage (Copy Buffers).

       Depending on the terminal, a distinction is made between the three copy buffers. 1. the copy buf‐
       fers internal in the editor. 2. the mouse copy buffer 3. the desktop copy buffer.

Undo und Redo
       With Undo or CTRL + z, entries can be undone. With Redo or CTRL + y the undo can be undone again.

Search und Replace
       With  Search  or  CTRL + f the Search dialog is opened. Under "Find" you enter a search word. You
       can use the options to shorten the search. A live search will then  be  performed  in  the  back‐
       ground. With F3 the next element found is marked, with Shift + F3 the previous one.

Search Next
       With F3, the next search word is highlighted.

Search Previous
       Like Shift + F3, the previous search word is marked.

Cut Line
       The entire line is cut out.

Replace
       With  Replace  or CTRL + r the Replace dialog is opened. The search word is entered in the "Find"
       field. In the field "Replace" the word to be inserted is specified. With "Next" the  next  search
       word  is searched. With "Replace" the search word is replaced. With "All" all occurrences are re‐
       placed at once.

Goto
       To jump to a line, open a Goto Line dialog under "Goto".

Sort Selcted Lines
       Sorts selected lines in alphabetical order.

Tab
       Opens the Dialog tab. Here the settings for a tab can be made. You can choose between tab ()  and
       space. You can also set the number of spaces. The default settings can also be set in the ~/.con‐
       fig/chr file. Here you can specify: "tabsize=8" or "tab=false" for spaces.

Line Number
       Shows the line number display on the right side of the editor. The default settings can  also  be
       made in the ~/.config/chr file. Here you can specify: "linenumber=true".

Formatting
       In  the Formatting dialog, "Formatting Characters", "Color Tabs" and "Color Spacs at end of line"
       can be switched on and off.

       The "Formatting characters" marks spaces with a dot: "·" end of line (0 with a "¶" and the end of
       the file with: "♦". Alternatively, this display can be turned off.

       With "Color Tabs" tabs are colorized. The tab border is made darker.

       "Color  Spaces  at  end of line" is used to mark the end of the line in red, if the cursor is not
       located there.

       In the configuration file: ~/.config/chr the behavior can be influenced with the option  "format‐
       ting_characters=true", "color_tabs=true", "color_space_end=true".

Wrap long lines
       Lines  that  are drawn beyond the editor border are cut or wrapped here. It can be wrapped at the
       word boundary or hard at the end of the line. This behaviour can  be  influenced  by  the  option
       "wrap_lines=true" in the ~/.config/chr file.

Following standard input
       If data is transferred to the editor via standard input, the following mode can always be used to
       jump to the current end of the file.

Stop Input Pipe
       The standard input file descriptor will be closed.

Highlight Brackets
       The following opening and closing brackets can be highlighted when the cursor  moves  over  them.
       With the option "highlight_bracket=true" this behavior can be influenced in the ~/.config/chr.
         [{(<>)}]

Theme
       It  opens the dialog for selecting a theme. The Classic (blue) or the Dark (black and white) mode
       is available. With the option "theme=classic" or "theme=dark", this  can  be  influenced  in  the
       ~/.config/chr.

Window
Next / Previous
       Switches the active window, with Shift in reverse order. (See F6)

Tile Vertically / Horizontally / Fullscreen
       Displays multiple windows in vertical / horizontal / full screen positions.

CUSTOM CONFIG
       Here are listed points that can only be influenced in the ~/.config/chr.

Select Cursor Position x0
       If lines are marked with the arrow keys, they can be changed with Tab/Shift+Tab or Shift+Up/Down.
       The option "select_cursor_position_x0" refers to the fact that the line is included even if it is
       at position x=0, i.e. at the beginning of the line.

Theme
       With  the  option "theme" the default background can be set. At the moment you can choose between
       "classic" and "dark".

Default config
       There is a default config (~/.config/chr) where the following options can be set.
         color_space_end=true
         color_tabs=true
         formatting_characters=true
         select_cursor_position_x0
         tab=false
         tabsize=8
         theme=classic
         wrap_lines=true

DESCRIPTION
       The chr terminal editor is inspired by the turbo pascal editor using Turbo Vision from  the  year
       1997.  For  the  keyboard  shortcut he should be similar gedit, to facilitate the transition from
       desktop to console editor.

BUGS
       All errors in this software can be managed via the bugtracker  on:
       https://github.com/istoph/editor

AUTHOR
       Christoph Hüffelmann <chr@istoph.de> Martin Hostettler <textshell@uchuujin.de>

1.0                                            28 Nov 2018                                        man(1)
```

## License
This is licensed under the [Boost Software License 1.0](/COPYING)

# SPDX-License-Identifier: BSL-1.0

main = ['main.cpp']

#ide:editable-filelist
tests = [
  'tests/eventrecorder.cpp',
  'tests/filelistparsertests.cpp',
  'tests/fileopentests.cpp',
  'tests/filesavetests.cpp',
  'tests/filetests.cpp',
  'tests/tests.cpp',
]

tests_headers = [
  'tests/eventrecorder.h',
]

#ide:editable-filelist
editor_sources = [
  'aboutdialog.cpp',
  'alert.cpp',
  'commandlinewidget.cpp',
  'confirmsave.cpp',
  'dlgfilemodel.cpp',
  'edit.cpp',
  'file.cpp',
  'filecategorize.cpp',
  'filelistparser.cpp',
  'filewindow.cpp',
  'formattingdialog.cpp',
  'gotoline.cpp',
  'groupbox.cpp',
  'insertcharacter.cpp',
  'mdilayout.cpp',
  'opendialog.cpp',
  'overwritedialog.cpp',
  'savedialog.cpp',
  'scrollbar.cpp',
  'searchcount.cpp',
  'searchdialog.cpp',
  'statemux.cpp',
  'statusbar.cpp',
  'syntaxhighlightdialog.cpp',
  'tabdialog.cpp',
  'textedit.cpp',
  'themedialog.cpp',
  'wrapdialog.cpp',
]

#ide:editable-filelist
editor_headers = [
  'aboutdialog.h',
  'alert.h',
  'commandlinewidget.h',
  'confirmsave.h',
  'dlgfilemodel.h',
  'edit.h',
  'file.h',
  'filecategorize.h',
  'filewindow.h',
  'formattingdialog.h',
  'gotoline.h',
  'groupbox.h',
  'insertcharacter.h',
  'mdilayout.h',
  'opendialog.h',
  'overwritedialog.h',
  'savedialog.h',
  'scrollbar.h',
  'searchcount.h',
  'searchdialog.h',
  'statusbar.h',
  'syntaxhighlightdialog.h',
  'tabdialog.h',
  'textedit.h',
  'themedialog.h',
  'wrapdialog.h',
]

vcs_dep = vcs_tag(command : ['git', 'rev-parse', '--short', 'HEAD'],
        fallback: '000000',
        input:'git_version.h.in',
        output:'git_version.h',
        replace_string:'%GIT_VERSION%')

editor_lib = static_library('editor', [editor_sources, syntax_qrc],
  qt5.preprocess(moc_headers: editor_headers, moc_sources: editor_sources,
  include_directories: include_directories('.')),
  include_directories: include_directories('.'),
  dependencies : [qt5_dep, tuiwidgets_dep, posixsignalmanager_dep, syntax_dep])

executable('editor', main,
  vcs_dep,
  include_directories: include_directories('.'),
  link_with: editor_lib,
  dependencies : [qt5_dep, tuiwidgets_dep, posixsignalmanager_dep, syntax_dep])

executable('tests', tests,
  qt5.preprocess(moc_headers: tests_headers, moc_sources: tests,
  include_directories: include_directories('.')),
  link_with: editor_lib,
  dependencies : [qt5_dep, tuiwidgets_dep, posixsignalmanager_dep, syntax_dep])

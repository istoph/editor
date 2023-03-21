# SPDX-License-Identifier: BSL-1.0

main = ['main.cpp']

#ide:editable-filelist
tests = [
  'tests/documenttests.cpp',
  'tests/filelistparsertests.cpp',
  'tests/fileopentests.cpp',
  'tests/filesavetests.cpp',
  'tests/filetests.cpp',
  'tests/tests.cpp',
]

tests_headers = [
  'tests/documenttests.h',
]

#ide:editable-filelist
editor_sources = [
  'alert.cpp',
  'clipboard.cpp',
  'commandlinewidget.cpp',
  'confirmsave.cpp',
  'dlgfilemodel.cpp',
  'document.cpp',
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
  'tabdialog.cpp',
  'themedialog.cpp',
  'wrapdialog.cpp',
]

#ide:editable-filelist
editor_headers = [
  'alert.h',
  'clipboard.h',
  'commandlinewidget.h',
  'confirmsave.h',
  'dlgfilemodel.h',
  'document.h',
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
  'tabdialog.h',
  'themedialog.h',
  'wrapdialog.h',
]

editor_lib = static_library('editor', editor_sources,
  qt5.preprocess(moc_headers: editor_headers, moc_sources: editor_sources,
  include_directories: include_directories('.')),
  include_directories: include_directories('.'),
  dependencies : [qt5_dep, tuiwidgets_dep, posixsignalmanager_dep])

executable('editor', main,
  include_directories: include_directories('.'),
  link_with: editor_lib,
  dependencies : [qt5_dep, tuiwidgets_dep, posixsignalmanager_dep])

executable('tests', tests,
  qt5.preprocess(moc_headers: tests_headers, moc_sources: tests,
  include_directories: include_directories('.')),
  link_with: editor_lib,
  dependencies : [qt5_dep, tuiwidgets_dep, posixsignalmanager_dep])

main = ['main.cpp']

#ide:editable-filelist
tests = [
  'tests.cpp',
  'documenttests.cpp',
  'filelistparsertests.cpp',
]

#ide:editable-filelist
editor_sources = [
  'clipboard.cpp',
  'commandlinewidget.cpp',
  'confirmsave.cpp',
  'dlgfilemodel.cpp',
  'document.cpp',
  'edit.cpp',
  'file.cpp',
  'filelistparser.cpp',
  'filewindow.cpp',
  'formattingdialog.cpp',
  'gotoline.cpp',
  'insertcharacter.cpp',
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
]

#ide:editable-filelist
editor_headers = [
  'clipboard.h',
  'commandlinewidget.h',
  'confirmsave.h',
  'dlgfilemodel.h',
  'edit.h',
  'file.h',
  'filewindow.h',
  'formattingdialog.h',
  'gotoline.h',
  'insertcharacter.h',
  'opendialog.h',
  'overwritedialog.h',
  'savedialog.h',
  'scrollbar.h',
  'searchcount.h',
  'searchdialog.h',
  'statusbar.h',
  'tabdialog.h',
  'themedialog.h',
]

editor_lib = static_library('editor', editor_sources,
  qt5.preprocess(moc_headers: editor_headers, moc_sources: editor_sources,
  include_directories: include_directories('.')),
  include_directories: include_directories('.'),
  dependencies : [qt5_dep, tuiwidgets_dep, testtui_lib_dep, posixsignalmanager_dep])

executable('editor', main,
  include_directories: include_directories('.'),
  link_with: editor_lib,
  dependencies : [qt5_dep, tuiwidgets_dep, testtui_lib_dep, posixsignalmanager_dep])

executable('tests', tests,
  include_directories: include_directories('.'),
  link_with: editor_lib,
  dependencies : [qt5_dep, tuiwidgets_dep, testtui_lib_dep, posixsignalmanager_dep])

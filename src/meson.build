#ide:editable-filelist
editor_sources = [
  'edit.cpp',
  'file.cpp',
  'opendialog.cpp',
  'savedialog.cpp',
  'scrollbar.cpp',
  'searchdialog.cpp',
  'statusbar.cpp',
  'tabdialog.cpp',
  'quitdialog.cpp',
]

#ide:editable-filelist
editor_headers = [
  'edit.h',
  'file.h',
  'opendialog.h',
  'savedialog.h',
  'scrollbar.h',
  'searchdialog.h',
  'statusbar.h',
  'tabdialog.h',
  'quitdialog.h',
]

executable('editor', editor_sources,
  qt5.preprocess(moc_headers: editor_headers, moc_sources: editor_sources,
  include_directories: include_directories('.')),
  include_directories: include_directories('.'),
  dependencies : [qt5_dep, tuiwidgets_dep, testtui_lib_dep])

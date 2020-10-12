main = ['main.cpp']

tests = ['tests.cpp']

#ide:editable-filelist
editor_sources = [
  'clipboard.cpp',
  'confirmsave.cpp',
  'edit.cpp',
  'file.cpp',
  'gotoline.cpp',
  'insertcharacter.cpp',
  'opendialog.cpp',
  'savedialog.cpp',
  'scrollbar.cpp',
  'searchcount.cpp',
  'searchdialog.cpp',
  'statusbar.cpp',
  'tabdialog.cpp',
]

#ide:editable-filelist
editor_headers = [
  'clipboard.h',
  'confirmsave.h',
  'edit.h',
  'file.h',
  'gotoline.h',
  'insertcharacter.h',
  'opendialog.h',
  'savedialog.h',
  'scrollbar.h',
  'searchcount.h',
  'searchdialog.h',
  'statusbar.h',
  'tabdialog.h',
]

executable('editor', main, editor_sources,
  qt5.preprocess(moc_headers: editor_headers, moc_sources: editor_sources,
  include_directories: include_directories('.')),
  include_directories: include_directories('.'),
  dependencies : [qt5_dep, tuiwidgets_dep, testtui_lib_dep])

executable('tests', tests, editor_sources,
  qt5.preprocess(moc_headers: editor_headers, moc_sources: editor_sources,
  include_directories: include_directories('.')),
  include_directories: include_directories('.'),
  dependencies : [qt5_dep, tuiwidgets_dep, testtui_lib_dep])

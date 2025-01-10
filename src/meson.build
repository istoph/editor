# SPDX-License-Identifier: BSL-1.0

main = ['main.cpp']

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
  'help.cpp',
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
  'help.h',
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
  'themedialog.h',
  'wrapdialog.h',
]

vcs_dep = vcs_tag(command : ['git', 'rev-parse', '--short', 'HEAD'],
        fallback: '000000',
        input:'version_git.h.in',
        output:'version_git.h',
        replace_string:'%GIT_VERSION%')

conf_data = configuration_data()
conf_data.set_quoted('VERSION_NUMBER', get_option('version'))
configure_file(
  input: 'version.h.in',
  output: 'version.h',
  configuration: conf_data
)


editor_lib = static_library('editor', [editor_sources, syntax_qrc],
  qt5.preprocess(moc_headers: editor_headers, moc_sources: editor_sources,
  include_directories: include_directories('.')),
  include_directories: include_directories('.'),
  dependencies : [qt5_dep, tuiwidgets_dep, posixsignalmanager_dep, syntax_dep])

executable('chr', main,
  vcs_dep,
  install_rpath: get_option('rpath'),
  install : true,
  include_directories: include_directories('.'),
  link_with: editor_lib,
  dependencies : [qt5_dep, tuiwidgets_dep, posixsignalmanager_dep, syntax_dep])

verbose_kwargs = {}
if meson.version().version_compare('>=0.62')
    verbose_kwargs += {'verbose': true}
endif

if get_option('tests')
  subdir('tests')
endif

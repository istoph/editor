# SPDX-License-Identifier: BSL-1.0

catch2_dep = dependency('catch2', required : get_option('system-catch2'))
if not catch2_dep.found()
  catch2_dep = declare_dependency(include_directories: include_directories('../../third-party'))
else
  if catch2_dep.version().version_compare('>=3.0')
    catch2_dep = [catch2_dep, declare_dependency(compile_args: ['-DCATCH3'])]
  endif
endif

#ide:editable-filelist
tests = [
  'attributes.cpp',
  'eventrecorder.cpp',
  'filelistparsertests.cpp',
  'fileopentests.cpp',
  'filesavetests.cpp',
  'filetests.cpp',
  'tests.cpp',
]

#ide:editable-filelist
tests_headers = [
  'eventrecorder.h',
  'catchwrapper.h',
]

tests_bin = executable('tests', tests,
  qt5.preprocess(moc_headers: tests_headers, moc_sources: tests,
  include_directories: include_directories('.')),
  link_with: editor_lib,
  dependencies : [qt5_dep, tuiwidgets_dep, posixsignalmanager_dep, syntax_dep, catch2_dep]
)

test('tests', tests_bin,
  workdir: meson.current_build_dir(),
  timeout: 180,
  kwargs: verbose_kwargs
)

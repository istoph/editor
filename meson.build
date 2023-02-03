# SPDX-License-Identifier: BSL-1.0

project('editor', ['c', 'cpp'], default_options : ['buildtype=debugoptimized'])

if meson.get_compiler('cpp').get_id() == 'gcc'
    add_project_arguments('-fvisibility=hidden', '-fvisibility-inlines-hidden', language: 'cpp')
endif

add_project_arguments('-std=gnu17', language : 'c')
add_project_arguments('-Wall', '-Wextra', language: 'c')
add_project_arguments('-Werror=return-type', '-Werror=implicit-function-declaration', language: 'c')

add_project_arguments('-std=gnu++17', language : 'cpp')
add_project_arguments('-Wall', '-Wextra', language: 'cpp')
add_project_arguments('-Werror=return-type', '-Werror=implicit-function-declaration', language: 'cpp')


qt5 = import('qt5')
qt5_dep = dependency('qt5', modules: ['Core', 'Concurrent'])

tuiwidgets_dep = dependency('TuiWidgets')
posixsignalmanager_dep = dependency('PosixSignalManager')

subdir('src')

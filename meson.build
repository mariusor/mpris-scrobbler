project('mpris-scrobbler', 'c',
        default_options : [
            'c_std=c11',
            'prefix=/usr',
            'buildtype=debugoptimized'
        ],
        license : 'MIT')

c_args = ['-Wpedantic', '-D_GNU_SOURCE', '-Wall', '-Wextra', '-fno-omit-frame-pointer']

if meson.get_compiler('c').get_id() == 'clang'
    c_args += ['-Wimplicit-fallthrough']
else
    c_args += ['-Wimplicit-fallthrough=0']
endif

add_global_arguments('-DAPPLICATION_NAME="' + meson.project_name() + '"', language : 'c')

git = find_program('git', required : false)
if git.found()
    git_version = vcs_tag(
        input: 'src/version.c.in',
        output: 'version.h',
        replace_string : '@VERSION@',
        command : ['git', 'describe', '--tags', '--long', '--dirty=-git', '--always'],
        fallback : '(unknown)')
endif

srcdir = include_directories('src')

deps = [
    dependency('dbus-1', required: true),
    dependency('expat', required: true),
    dependency('libcurl', required: true),
    dependency('libevent', required: true),
    dependency('openssl', required: true),
]

ragel_bin = find_program('ragel', required: true)
ragel = generator(ragel_bin,
                  output: '@BASENAME@.h',
                  arguments: ['-G2','-C', '-n', '@INPUT@', '-o', '@OUTPUT@'])
ini_parser = ragel.process('src/ini.rl')


executable('mpris-scrobbler', ['src/daemon.c', git_version], ini_parser, include_directories: srcdir, dependencies: deps)
executable('mpris-scrobbler-signon', ['src/signon.c', git_version], ini_parser, include_directories: srcdir, dependencies: deps)

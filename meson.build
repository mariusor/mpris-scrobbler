project('mpris-scrobbler', 'c',
        default_options : [
            'c_std=c11',
#            'warning_level=2',
            'werror=true',
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

bin_name = meson.project_name()
dest_dir = get_option('dest_dir')
prefix = get_option('prefix')
bin_dir = get_option('bin_dir')
unit_dir = get_option('unit_dir')

bin_install = join_paths(dest_dir, prefix, bin_dir)
unit_install = join_paths(dest_dir, prefix, unit_dir)

executable('mpris-scrobbler',
            ['src/daemon.c', git_version],
            ini_parser,
            include_directories: srcdir,
            install : true,
            install_dir : bin_install,
            dependencies: deps
)
executable('mpris-scrobbler-signon',
            ['src/signon.c', git_version],
            ini_parser,
            include_directories: srcdir,
            install : true,
            install_dir : bin_install,
            dependencies: deps
)

#bus_name = get_option('bus_name')
m4_bin = find_program('m4', required: true)
unit = custom_target('systemd-service',
                input: 'units/systemd-user.service.in',
                output: meson.project_name() + '.service',
                capture: true,
                command: [
                    m4_bin,
                    '-P',
                    '-DDESTDIR=' + dest_dir,
                    '-DINSTALL_PREFIX=' + prefix,
                    '-DBINDIR=' + bin_dir,
                    '-DDAEMONNAME=' + meson.project_name(),
                    '@INPUT@'
                ],
                install : true,
                install_dir : unit_install
)


project('mpris-scrobbler', 'c',
        default_options : [ 'std=c14', ],
        license : 'MIT')

c_args = ['-Wpedantic', '-Wall', '-Wextra', '-fno-omit-frame-pointer']

if meson.get_compiler('c').get_id() == 'clang'
    c_args += ['-Wimplicit-fallthrough']
else
    c_args += ['-Wimplicit-fallthrough=0']
endif

bin_name = meson.project_name()

add_project_arguments('-DAPPLICATION_NAME="' + bin_name + '"', language : 'c')
if get_option('buildtype') == 'debug'
    message('building debug')
    add_project_arguments('-DDEBUG', language : 'c')
endif

git = find_program('git', required : false)
if git.found()
    git_version = vcs_tag(
        input: 'src/version.c.in',
        output: 'version.h',
        replace_string : '@GIT_VERSION@',
        command : ['git', 'describe', '--tags', '--long', '--dirty=-git', '--always'],
        fallback : '(unknown)')
endif

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

rootprefixdir = get_option('rootprefix')
rootprefixdir = rootprefixdir != '' ? rootprefixdir : '/usr'

prefix = get_option('prefix')
bindir = get_option('bindir')
unitdir = get_option('unitdir')

prefixdir = get_option('prefix')
if not prefixdir.startswith('/')
        error('Prefix is not absolute: "@0@"'.format(prefixdir))
endif
bindir = join_paths(prefixdir, get_option('bindir'))

rootbindir = join_paths(rootprefixdir, 'bin')
userunitdir = join_paths(rootprefixdir, unitdir)

srcdir = include_directories('src')

executable('mpris-scrobbler',
            ['src/daemon.c', git_version],
            ini_parser,
            include_directories: srcdir,
            install : true,
            install_dir : rootbindir,
            dependencies: deps
)
executable('mpris-scrobbler-signon',
            ['src/signon.c', git_version],
            ini_parser,
            include_directories: srcdir,
            install : true,
            install_dir : rootbindir,
            dependencies: deps
)

m4_bin = find_program('m4', required: true)
unit = custom_target('systemd-service',
                input: 'units/systemd-user.service.in',
                output: bin_name + '.service',
                capture: true,
                command: [
                    m4_bin,
                    '-P',
                    '-DBINPATH=' + rootbindir,
                    '-DDAEMONNAME=' + bin_name,
                    '@INPUT@'
                ],
                install : true,
                install_dir : userunitdir
)


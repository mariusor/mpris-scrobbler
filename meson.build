project('mpris-scrobbler', 'c',
        default_options : [
            'c_std=c11',
            'buildtype=debug',
            'warning_level=3',
        ],
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
    add_project_arguments('-DDEBUG', language : 'c')
endif

deps = [
    dependency('dbus-1', required: true),
    dependency('libcurl', required: true),
    dependency('libevent', required: true),
    dependency('json-c', required: true),
]

ragel_bin = find_program('ragel', required: true)
ragel = generator(ragel_bin,
                  output: '@BASENAME@.h',
                  arguments: ['-G2','-C', '-n', '@INPUT@', '-o', '@OUTPUT@'])
ini_parser = ragel.process('src/ini.rl')

version_hash = get_option('version')
if version_hash != ''
    add_project_arguments('-DVERSION_HASH="' + version_hash + '"', language : 'c')
endif
git = find_program('git', required : false)
if git.found()
    git_version = vcs_tag(
        input: 'src/version.h.in',
        output: 'version.h',
        replace_string : '@GIT_VERSION@',
        command : ['git', 'describe', '--tags', '--long', '--dirty=-git', '--always'],
        fallback : '(unknown)')
endif

credentials = configuration_data()
credentials.set('lastfm_api_key', get_option('lastfm_api_key'))
credentials.set('lastfm_api_secret', get_option('lastfm_api_secret'))
credentials.set('librefm_api_key', get_option('librefm_api_key'))
credentials.set('librefm_api_secret', get_option('librefm_api_secret'))
credentials.set('listenbrainz_api_key', get_option('listenbrainz_api_key'))
credentials.set('listenbrainz_api_secret', get_option('listenbrainz_api_secret'))
configure_file(input: 'src/credentials_lastfm.h.in',
    output: 'credentials_lastfm.h',
    configuration: credentials)
configure_file(input: 'src/credentials_librefm.h.in',
    output: 'credentials_librefm.h',
    configuration: credentials)
configure_file(input: 'src/credentials_listenbrainz.h.in',
    output: 'credentials_listenbrainz.h',
    configuration: credentials)

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
            c_args: c_args,
            include_directories: srcdir,
            install : true,
            install_dir : rootbindir,
            dependencies: deps
)
executable('mpris-scrobbler-signon',
            ['src/signon.c', git_version],
            ini_parser,
            c_args: c_args,
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


project('mpris-scrobbler', 'c',
        default_options : [
            'c_std=c11',
            'buildtype=debug',
            'warning_level=3',
            'unity=on',
            'werror=true',
        ],
        license : 'MIT')

c_args = ['-Wall', '-Wextra', '-fno-omit-frame-pointer']

bin_name = meson.project_name()
add_project_arguments('-DAPPLICATION_NAME="' + bin_name + '"', language : 'c')
if get_option('buildtype') == 'debug'
    add_project_arguments('-Werror', language : 'c')
    add_project_arguments('-DDEBUG', language : 'c')
    add_project_arguments('-fstack-protector-strong', language : 'c')
    add_project_arguments('-fno-plt', language : 'c')
    if meson.get_compiler('c').get_id() != 'clang'
      add_project_arguments('-Werror=format-truncation=2', language : 'c')
    endif
endif

deps = [
    dependency('dbus-1', required: true),
    dependency('libcurl', required: true),
    dependency('libevent_pthreads', required: true),
    dependency('libevent', required: true),
    dependency('json-c', required: true),
]

version_hash = get_option('version')

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


prefixdir = get_option('prefix')
if not prefixdir.startswith('/')
        error('Prefix is not absolute: "@0@"'.format(prefixdir))
endif
bindir = join_paths(prefixdir, get_option('bindir'))
unitdir = join_paths(prefixdir, get_option('unitdir'))

srcdir = include_directories('src')

daemon_sources = ['src/daemon.c']
signon_sources = ['src/signon.c']

if version_hash != ''
    add_project_arguments('-DVERSION_HASH="' + version_hash + '"', language : 'c')
else
    git = find_program('git', required : false)
    if git.found()
        git_version = vcs_tag(
            input: 'src/version.h.in',
            output: 'version.h',
            replace_string : '@GIT_VERSION@',
            command : ['git', 'describe', '--tags', '--long', '--dirty=-git', '--always'],
            fallback : '(unknown)')
        daemon_sources += git_version
    endif
endif

executable('mpris-scrobbler',
            daemon_sources,
            c_args: c_args,
            include_directories: srcdir,
            install : true,
            install_dir : bindir,
            dependencies: deps
)
executable('mpris-scrobbler-signon',
            signon_sources,
            c_args: c_args,
            include_directories: srcdir,
            install : true,
            install_dir : bindir,
            dependencies: deps
)

ctags = find_program('ctags', required: false)
if ctags.found()
    run_target( 'ctags', command: [ ctags, '-R', './src', '/usr/include/dbus-1.0/dbus/', '/usr/include/event2/', '/usr/include/curl' ])
endif

m4_bin = find_program('m4', required: true)
unit = custom_target('systemd-service',
        input: 'units/systemd-user.service.in',
        output: bin_name + '.service',
        capture: true,
        command: [
            m4_bin,
            '-P',
            '-DBINPATH=' + bindir + '/',
            '-DDAEMONNAME=' + bin_name,
            '@INPUT@'
        ],
        install : true,
        install_dir : unitdir
)

scdoc = find_program('scdoc', required: false)

if scdoc.found()
    sh = find_program('sh')
    mandir = get_option('mandir')
    man_files = [
        'doc/mpris-scrobbler.1.scd',
        'doc/mpris-scrobbler-signon.1.scd',
        'doc/mpris-scrobbler-credentials.5.scd',
    ]
    foreach filename : man_files
        topic = filename.split('.')[-3].split('/')[-1]
        section = filename.split('.')[-2]
        output = '@0@.@1@'.format(topic, section)

        custom_target(
            output,
            input: filename,
            output: output,
            command: [
                sh, '-c', '@0@ < @INPUT@ > @1@'.format(scdoc.path(), output)
            ],
            install: true,
            install_dir: '@0@/man@1@'.format(mandir, section)
        )
    endforeach
endif


project('mpris-scrobbler', 'c',
        default_options : [
            'c_std=c11',
            'buildtype=debug',
            'warning_level=everything',
            'unity=on',
        ],
        license : 'MIT')


bin_name = meson.project_name()
c_args = [
    '-DAPPLICATION_NAME="' + bin_name + '"',
    '-fstack-clash-protection',
    '-Wformat',
    '-Wno-switch-enum', # when libcurldebug is enabled this needs to be set
    '-Wstrict-overflow',
    '-Wno-unused-function', # not even go goes this far
    '-Wno-c++-compat', # we want assign from (void*) w/o cast
    '-Wno-cast-qual', # we sometimes want to allow const strings to be operated on
    '-Wno-padded', # sometimes you just can't do without
    '-Wno-null-dereference', # the grrrs strlen uses *s++ which triggers this.
## we should look at these to fix
    '-Wno-format-nonliteral', # we use a non string literal for the format of a snprintf to build the authorization url
]

if build_machine.cpu_family() in ['x86', 'x86_64']
    # this is supported only on x86 and x86_64, and we add it because it's a default
    # CFLAG for Archlinux's Makepkg
    add_project_arguments('-fcf-protection=full', language : 'c')
endif

if meson.get_compiler('c').get_id() == 'clang'
    clang_extra_args = [
        '-Wno-declaration-after-statement',
        '-Wno-unsafe-buffer-usage',
        '-Wno-extra-semi-stmt',
        '-Wno-covered-switch-default',
    ]
    add_project_arguments(clang_extra_args, language : 'c')
endif
if meson.get_compiler('c').get_id() == 'gcc'
    gcc_extra_args = [
        '--param=inline-min-speedup=2',
        '--param=max-inline-insns-auto=80',
        '-Wsuggest-attribute=const',
        '-Wno-alloc-zero', # stb arrfree uses realloc with 0 bytes
        '-Wno-suggest-attribute=pure', # no attributes
        '-Wno-suggest-attribute=malloc', # no attributes
        '-Wno-suggest-attribute=format', # no attributes
    ]
    add_project_arguments(gcc_extra_args, language : 'c')
endif

if get_option('buildtype') == 'debug' or get_option('debug') == true
    add_project_arguments('-DDEBUG', language : 'c')
    #add_project_arguments('-DRETRY_ENABLED', language : 'c')
    if get_option('libeventdebug') == true
        add_project_arguments('-DLIBEVENT_DEBUG', language : 'c')
    endif
    if get_option('libcurldebug') == true
        add_project_arguments('-DLIBCURL_DEBUG', language : 'c')
    endif
    if get_option('libdbusdebug') == true
        add_project_arguments('-DLIBDBUS_DEBUG', language : 'c')
    endif
endif

deps = [
    dependency('dbus-1', required : true, version : '>=1.9'),
    dependency('libcurl', required : true),
    dependency('libevent_pthreads', required : true),
    dependency('libevent', required : true),
    dependency('json-c', required : true),
]

version_hash = get_option('version')

credentials = configuration_data()
credentials.set('lastfm_api_key', get_option('lastfm_api_key'))
credentials.set('lastfm_api_secret', get_option('lastfm_api_secret'))
credentials.set('librefm_api_key', get_option('librefm_api_key'))
credentials.set('librefm_api_secret', get_option('librefm_api_secret'))
credentials.set('listenbrainz_api_key', get_option('listenbrainz_api_key'))
credentials.set('listenbrainz_api_secret', get_option('listenbrainz_api_secret'))
configure_file(input : 'src/credentials_lastfm.h.in',
               output : 'credentials_lastfm.h',
               configuration : credentials)
configure_file(input : 'src/credentials_librefm.h.in',
               output : 'credentials_librefm.h',
               configuration : credentials)
configure_file(input : 'src/credentials_listenbrainz.h.in',
               output : 'credentials_listenbrainz.h',
               configuration : credentials)


prefixdir = get_option('prefix')
if not prefixdir.startswith('/')
    error('Prefix is not absolute: "@0@"'.format(prefixdir))
endif
bindir = join_paths(prefixdir, get_option('bindir'))
unitdir = join_paths(prefixdir, get_option('unitdir'))

srcdir = include_directories('src')

daemon_sources = ['src/daemon.c']
signon_sources = ['src/signon.c']

git = find_program('git', required : version_hash == '')
if git.found()
    version_h = vcs_tag(
        input : 'src/version.h.in',
        output : 'version.h',
        replace_string : '@GIT_VERSION@',
        command : ['git', 'describe', '--tags', '--long', '--dirty=-git', '--always'],
        fallback : '(unknown)')
    deps += declare_dependency(sources : version_h)
endif
if version_hash != ''
    add_project_arguments('-DVERSION_HASH="' + version_hash + '"', language : 'c')
endif


executable('mpris-scrobbler',
           daemon_sources,
           c_args : c_args,
           include_directories : srcdir,
           install : true,
           install_dir : bindir,
           dependencies : deps
)
executable('mpris-scrobbler-signon',
           signon_sources,
           c_args : c_args + ['-D_POSIX_C_SOURCE=200809L'],
           include_directories : srcdir,
           install : true,
           install_dir : bindir,
           dependencies : deps
)

ctags = find_program('ctags', required : false)
if ctags.found()
    run_target('ctags', command : [ctags, '-f', '../tags', '--tag-relative=never', '-R', '../src', '/usr/include/dbus-1.0/dbus/', '/usr/include/event2/', '/usr/include/curl'])
endif

m4_bin = find_program('m4', required : false)
if m4_bin.found()
    unit = custom_target('systemd-service',
                         input : 'units/systemd-user.service.in',
                         output : bin_name + '.service',
                         capture : true,
                         command : [
                             m4_bin,
                             '-P',
                             '-DBINPATH=' + bindir + '/',
                             '-DDAEMONNAME=' + bin_name,
                             '@INPUT@'
                         ],
                         install : true,
                         install_dir : unitdir
    )
endif

scdoc = find_program('scdoc', required : false)

if scdoc.found()
    sh = find_program('sh')
    mandir = get_option('mandir')
    man_files = [
        'doc/mpris-scrobbler.1.scd',
        'doc/mpris-scrobbler-signon.1.scd',
        'doc/mpris-scrobbler-credentials.5.scd',
        'doc/mpris-scrobbler-config.5.scd',
    ]
    foreach filename : man_files
        topic = filename.split('.')[-3].split('/')[-1]
        section = filename.split('.')[-2]
        output = '@0@.@1@'.format(topic, section)

        custom_target(
            output,
            input : filename,
            output : output,
            command : [
                sh, '-c', '@0@ < @INPUT@ > @1@'.format(scdoc.full_path(), output)
            ],
            install : true,
            install_dir : '@0@/man@1@'.format(mandir, section)
        )
    endforeach
endif


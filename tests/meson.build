project('mpris-scrobbler-tests', 'c')

srcdir = include_directories('../src')
snowdir = include_directories('lib/snow')

args = ['-Wall', '-Wextra', '-DSNOW_ENABLED']

stretchy_test = executable('test_stdb_ds',
            ['stdb_ds_test.c'],
            c_args: args,
            include_directories: [srcdir, snowdir],
)

ini_parser_test = executable('test_ini_parser',
            ['ini_reader_test.c'],
            c_args: args,
            include_directories: [srcdir, snowdir],
)

test('Test stretchy buffers functionality', stretchy_test)
test('Test ini parser functionality', ini_parser_test)

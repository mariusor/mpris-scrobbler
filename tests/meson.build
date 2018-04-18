project('mpris-scrobbler-tests', 'c')

srcdir = include_directories('../src')
snowdir = include_directories('lib/snow/snow')

args = ['-Wall', '-Wextra', '-DSNOW_ENABLED']

array_test= executable('test_string_array',
            ['string_array_test.c'],
            c_args: args,
            include_directories: [srcdir, snowdir],
)

stretchy_test= executable('test_stretchy_buffers',
            ['stretchy_buffers_test.c'],
            c_args: args,
            include_directories: [srcdir, snowdir],
)

test('Test string array functionality', array_test)
test('Test stretchy buffers functionality', stretchy_test)

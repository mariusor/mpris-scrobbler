project('tests', 'c')
srcdir = include_directories('../src')

testargs = ['-Wall', '-Wextra', '-DSNOW_ENABLED', '-DAPPLICATION_NAME="test"', '-DVERSION_HASH="git"']
testarray = executable('test_string_array',
            ['string_array_test.c'],
            c_args: testargs,
            include_directories: srcdir,
)
test('test_string_array', testarray)

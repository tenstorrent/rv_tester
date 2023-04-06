from test.util import testroot, testdir, mkpath_tree_fatal
import inspect
import os
import pathlib
import pytest
import sys

# -------------------------------------------------------------------------------------------------
# Locate and load simtest
# -------------------------------------------------------------------------------------------------
sys.path.insert(0, os.path.join(os.path.dirname(testroot()), 'sim', 'runtime'))

#create a mock riscv-dv.scripts.instr_trace_compare
tdir = pathlib.Path(testdir())
mkpath_tree_fatal(tdir, {
    'cases': {},
    'lib': {
        'riscv-dv': {
            '__init__.py': '',
            'scripts': {
                '__init__.py': '',
            },
        }
    }
})

instr_trace_compare = tdir / 'lib' /  'riscv-dv' / 'scripts' / 'instr_trace_compare.py'
with open(instr_trace_compare, 'w') as fp:
    fp.write('''
import os

def compare_trace_csv(**args):
    if 'TEST_COMPARE_TRACE_CSV' in os.environ:
        return os.environ['TEST_COMPARE_TRACE_CSV']
    return '[PASSED]'
'''
)
sys.path.insert(1, str(tdir / 'lib'))
from simtest import SimTest

cases = tdir / 'cases'

def test_simple_exe():
    st = SimTest('/usr/bin/true', [], [], [])
    try:
        st.run()
    except:
        assert False

    st = SimTest('/usr/bin/false', [], [], [])
    try:
        st.run()
    except:
        assert True

def test_check_log_generation(capsys):
    global cases
    case_dir = cases / inspect.stack()[0][3]
    mkpath_tree_fatal(case_dir, {
        'script': '#!' + sys.executable + ''' -B
print('__first_line__')
print('__second_line__')

'''
    })
    exe = os.path.join(case_dir, 'script')
    log = os.path.join(case_dir, 'log')
    os.chmod(exe, 0o755)
    st = SimTest(exe, ['+log', log], [], [])
    try:
        st.run()
    except:
        assert False

    assert os.path.isfile(log)
    with open(log) as log_fp:
        assert log_fp.read() == '''__first_line__
__second_line__
'''

    assert capsys.readouterr().out == '''__first_line__
__second_line__
'''

def test_errors(capsys):
    global cases
    case_dir = cases / inspect.stack()[0][3]
    mkpath_tree_fatal(case_dir, {
        'uvm_error_after_report': '#!' + sys.executable + ''' -B
print('--- UVM Report : ')
print('UVM_ERROR: Sample Error String')
print('UVM_FATAL: Sample Error String')
''',
        'uvm_error_before_report': '#!' + sys.executable + ''' -B
print('UVM_ERROR: Sample Error String')
print('UVM_FATAL: Sample Error String')
print('--- UVM Report : ')
'''
    })
    exe = os.path.join(case_dir, 'uvm_error_after_report')
    os.chmod(exe, 0o755)

    st = SimTest(exe, [], [], [])
    try:
        st.run()
    except:
        assert False

    exe = os.path.join(case_dir, 'uvm_error_before_report')
    os.chmod(exe, 0o755)

    st = SimTest(exe, [], [], [])
    try:
        st.run()
    except:
        assert True

pytest.main()

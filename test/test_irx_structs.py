import unittest
from run_test import cmp
from test_data import thread, _thread_base, _cpu, _dnote
from pyirinterpreter import PyIRModuleInterpreter

class TestIRXStructs(unittest.TestCase):
    def setUp(self):
        pass
    
    def tearDown(self):
        pass

    def test_get_struct_field_info__thread_name(self):
        irx = PyIRModuleInterpreter()

        paths = ["builddir/libirx/tests/test.ll", "builddir/libirx/tests/test-extern.ll"]

        if not irx.parseIRFiles(paths):
            raise FileNotFoundError(f"Failed to parse LLVM-IR file {str(paths)}")

        thread_ptr = irx.structAlloc("struct.thread")
        self.assertNotEqual(thread_ptr, 0)
        
        info = irx.getStructFieldInfo("thread", ["name"])
        self.assertEqual(info['basicType'], b'char')
        self.assertEqual(info['size_in_bits'], 128)
        self.assertEqual(info['size_in_bytes'], 16)
        self.assertEqual(info['gep'], [0, 4])


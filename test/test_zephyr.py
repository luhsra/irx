import unittest
from run_test import cmp
from test_data import thread, _thread_base, _cpu, _dnote
from pyirinterpreter import PyIRModuleInterpreter

class TestIRXStructs(unittest.TestCase):
    def setUp(self):
        pass
    
    def tearDown(self):
        pass

    def test_z_kernel(self):
        irx = PyIRModuleInterpreter()

#         paths = ["libirx/test-appl/zephyr.elf.ll"]
#         
#         if not irx.parseIRFiles(paths):
#             raise FileNotFoundError(f"Failed to parse LLVM-IR file {str(paths)}")
#         
#         _kernel1 = create_pyz_kernel(0, 0)
#         _kernel2 = create_pyz_kernel(0, 0)
# 
#         _kernel_ptr = irm.irx_get_global_addr("_kernel")
#         self.assertTrue(_kernel_ptr <= 0xFFFFFFFF)
#         
#         irm.set_struct("z_kernel", _kernel_ptr, _kernel1)
#         irm.get_struct("z_kernel", _kernel_ptr, _kernel2)
#         
#         self.assertTrue(cmp(_kernel, _kernel2))
        

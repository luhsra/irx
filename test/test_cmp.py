import unittest
from diff import cmp
from . import ArraySize, _cpu, thread

class TestCmp(unittest.TestCase):
    def test_cmp_primitives(self):
        self.assertTrue(cmp(4, 4))
        self.assertTrue(cmp(True, True))
        self.assertTrue(cmp(False, False))
        self.assertTrue(cmp("test", "test"))
        self.assertTrue(cmp(None, None))
        self.assertFalse(cmp(4, 3))
        self.assertFalse(cmp(True, False))
        self.assertFalse(cmp(True, False))
        self.assertFalse(cmp("test1", "test"))
        self.assertFalse(cmp("test", "test1"))
        self.assertFalse(cmp(1, None))

    def test_cmp_lists(self):
        self.assertFalse(cmp([1, 2], [1]))
        self.assertFalse(cmp([1, 2], [1, 3]))
        self.assertTrue(cmp([1, 2], [1, 2]))
    
    def test_cmp_dict(self):
        self.assertFalse(cmp({'x': 1, 'y': 2}, {'x': 1}))
        self.assertTrue(cmp({'x': 1, 'y': 2}, {'x': 1, 'y': 2}))
    
    def test_cmp_dataclass(self):
        t1 = thread(cpus1=[_cpu(n=1)], cpus2=[_cpu(n=1), _cpu(n=2)], n=[1, 2, 3, 4], name=None)
        t2 = thread(cpus1=[_cpu(n=1)], cpus2=[_cpu(n=1), _cpu(n=2)], n=[1, 2, 3, 4, 5], name=None)
        self.assertTrue(cmp(t1, t1))
        self.assertTrue(cmp(t2, t2))
        self.assertFalse(cmp(t1, t2))
        t2 = thread(cpus1=[_cpu(n=1)], cpus2=[_cpu(n=1), _cpu(n=2)], n=[1, 2, 3, 5], name=None)
        self.assertFalse(cmp(t1, t2))

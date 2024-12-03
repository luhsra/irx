from dataclasses import dataclass, fields, is_dataclass
import logging
logger = logging.getLogger("diff.py")
logger.setLevel(logging.WARNING)

class Missmatch:
    def __init__(self, obj1, obj2, path, depth):
        self.depth = depth
        self.path = path
        self.obj1 = obj1
        self.obj2 = obj2
    
    def _typeinfo(self):
        return ""
        # return ' ' * self.depth + "type(obj1) " + str(type(self.obj1)) + " "  + "type(obj2) " + str(type(self.obj2))
    
    def path_str(self):
        return '.'.join(self.path)
    
    def __repr__(self):
        return "" \
            + self._typeinfo() \
            + "" \
            + f"{' ' * self.depth}" + self.path_str() + f": {self.obj1} != {self.obj2}"


def diff(obj1, obj2, depth=0, path=[]):
    missmatches=[]
    logger.debug(' ' * depth + "type(obj1) " + str(type(obj1)) + "type(obj2) " + str(type(obj2)))
    if type(obj1) in [int, bytes, str, bool]:
        if obj1 != obj2:
            return [Missmatch(obj1, obj2, path, depth)]
    elif obj1 is None:
        if obj2 is not None:
            return [Missmatch(obj1, obj2, path, depth)]
    elif type(obj1).__name__ == 'PTR':
        if type(obj2).__name__ != 'PTR':
            return [Missmatch(obj1, obj2, path, depth)]
        if obj1.ptr is None or obj2.ptr is None: # skip the check (for unions)
            return []
        if obj1.ptr != obj2.ptr:
            return [Missmatch(obj1, obj2, path, depth)]
    elif type(obj1) in [list]:
        logger.debug(' ' * depth + "found: [list]")
        if len(obj1) != len(obj2):
            return [Missmatch(f'len(obj1)={len(obj1)}', f'len(obj2)={len(obj2)}', path, depth)]
        for k, v in enumerate(obj1):
            logger.debug(' ' * depth + " k " + str(k) + ' ' + str(v))
            missmatches += diff(obj1[k], obj2[k], depth+1, path+[str(k)])
        return missmatches
    elif type(obj1) in [dict]:
        logger.debug(' ' * depth + "found: [dict]")
        if len(obj1) != len(obj2):
            return [Missmatch(obj1, obj2, path, depth)]
        for k, v in enumerate(obj1):
            missmatches += diff(obj1[v], obj2[v], depth+1, path+[str(k)])
    elif is_dataclass(obj1):
        logger.debug(' ' * depth + "found: [is_dataclass]")
        for k in fields(obj1):
            if k.name.startswith("__"):
                continue
            if not hasattr(obj1, k.name):
                missmatches += Missmatch(obj1, '<missing>', path + [k.name], depth)
            if not hasattr(obj2, k.name):
                missmatches += Missmatch('<missing>', obj2, path + [k.name], depth)
            v = [getattr(obj1, k.name), getattr(obj2, k.name)]
            logger.debug(' ' * depth + "fields: " + str(v[0]) + ' ' + str(v[1]))
            missmatches += diff(v[0], v[1], depth+1, path+[k.name])
    else:
        raise RuntimeError("type in dataclass unknown " + str(type(obj1)) + ' ' + str(type(obj2)))
        return Missmatch('type in dataclass unknown', '', path, depth)
    return missmatches

def cmp(obj1, obj2, depth=0, path=[]):
    return len(diff(obj1, obj2)) == 0

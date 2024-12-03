set debuginfod enabled off
set confirm off
set print pretty on

python
import sys
sys.path.insert(0, '/usr/share/gcc-14/python')
from libstdcxx.v6.printers import register_libstdcxx_printers
try:
  register_libstdcxx_printers(None)
except:
    pass
end

catch fork
catch vfork
set follow-fork-mode child
set auto-load libthread-db on

# set breakpoint pending on

set auto-solib-add on

# b irinterpreter2.cc:282
# b irinterpreter2.cc:271

# b ConstantsContext.h:721 # getOrCreate()
# b Constants.cpp:1280 # Initializer for struct elements dosn't match

# b llvm::Constants::getOrCreate

# b main
b irinterpreter2.cc:66
commands
  #b llvm::ConstantStruct::get
#   b llvm::User::allocateFixedOperandUser
  #b llvm::User::User
  #b llvm::ConstantAggregate::ConstantAggregate
#   b llvm::ExecutionEngine::getConstantValue

  # b llvm::Interpreter::getOperandValue
  # b llvm::Interpreter::visitLoadInst
  # b llvm::Interpreter::visitStoreInst
  # b MyInterpreter::MyInterpreter
#   dprintf ExecutionEngine.cpp:238, "name is %X\n", &GO
  
  b ExecutionEngine.cpp:238
  commands
    p &GO
    p this->getMangledName(&GO)
    p GO
    c
  end
  
#   b ExecutionEngine.cpp:182
#   commands
#   p I
#   c
#   end
  
#   b ExecutionEngine.cpp:230
#   commands
# #     b llvm::MallocAllocator::Allocate
#     c
#   end
  
#   b llvm::StringMap::insert if this == 0x5e61f0
  
#   b llvm::deallocate_buffer
#   commands
#   p/x Ptr
#   c
#   end
  
#   b llvm::BasicBlock::~BasicBlock
 
  # if F.getName().equals("sys_dnode_is_linked.138")
  b /mnt/data/opt/llvm/llvm-project/llvm/lib/IR/Module.cpp:537
  commands
    p F.getName()
    p F.dump()
    
    
    
#     b llvm::Value::~Value
#     commands
    #p this->dump()
#     c
#     end
    
    c
  end
  
#   b ExecutionEngine::emitGlobalVariable
  
  
#   b llvm::Interpreter::emitGlobals
  
#   b llvm::Interpreter
  c
end

# b irinterpreter2.cc:62

# b irinterpreter2.cc:100
# call.getCalledFunction()->getName().equals("thread_join2")


# b llvm::ConstantAggregate::ConstantAggregate

# b llvm::ConstantUniqueMap<llvm::ConstantStruct>::getOrCreate


define whereis
  find &SF, +sizeof(SF), "t4"
end


# b printStructDebugInfo
# b MyInterpreter::executeGEPOperation
# b LoadValueFromMemory
# b llvm::StoreIntToMemory
# b llvm::ExecutionEngine::StoreValueToMemory
# b StoreValueToMemory

# b Interpreter::visitStoreInst
# b llvm::Interpreter::visitStoreInst


# x/32x


# b IRModuleInterpreter::parseIRFiles
# b IRModuleInterpreter::structAlloc
# b irinterpreter2.cc:77


# b main
# b main command record full
# record function-call-history

# b IRModuleInterpreter::getPointerSize
# b MapAllocator::malloc

# b llvm::ExecutionEngine::LoadValueFromMemory
# b llvm::ExecutionEngine::StoreValueToMemory


# b llvm::ExecutionEngine::emitGlobals
# b llvm::ExecutionEngine::getOrEmitGlobalVariable
# b llvm::ExecutionEngine::finalizeObject  
# b llvm::ExecutionEngine::getPointerToGlobal


# watch *(int*)(0xfe80d0)

b MyInterpreter::resetExtraAllocations

b MyInterpreter::InitializeGlobals

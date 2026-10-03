#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include <llvm/IR/Module.h>
#include <llvm/Passes/OptimizationLevel.h>

void optimizeModule(llvm::Module& module, llvm::OptimizationLevel level);

#endif

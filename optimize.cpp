#include "optimize.h"
#include <llvm/Passes/PassBuilder.h>

void optimizeModule(llvm::Module& module, llvm::OptimizationLevel level) {
    if (level == llvm::OptimizationLevel::O0)
        return;

    llvm::LoopAnalysisManager loopAnalyses;
    llvm::FunctionAnalysisManager functionAnalyses;
    llvm::CGSCCAnalysisManager cgsccAnalyses;
    llvm::ModuleAnalysisManager moduleAnalyses;

    llvm::PassBuilder passBuilder;
    passBuilder.registerModuleAnalyses(moduleAnalyses);
    passBuilder.registerCGSCCAnalyses(cgsccAnalyses);
    passBuilder.registerFunctionAnalyses(functionAnalyses);
    passBuilder.registerLoopAnalyses(loopAnalyses);
    passBuilder.crossRegisterProxies(loopAnalyses, functionAnalyses, cgsccAnalyses, moduleAnalyses);

    llvm::ModulePassManager pipeline = passBuilder.buildPerModuleDefaultPipeline(level);
    pipeline.run(module, moduleAnalyses);
}

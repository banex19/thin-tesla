#ifndef OTHER_LOCK_ANALYSIS_H
#define OTHER_LOCK_ANALYSIS_H

#include "Analysis.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

using namespace llvm;

struct OtherLockAnalysis : public Analysis {
  OtherLockAnalysis(Module &M, Function &F, Value &V);
  Value *UsesOtherLock(std::set<Function *> calls) const;
  bool run() override;
  std::string AnalysisName() const override;

private:
  Function &Bound;
  Value &Lock;
};

#endif
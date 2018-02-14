/** @file Visitor.h  Declaration of @ref tesla::TeslaVisitor. */
/*
 * Copyright (c) 2013 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef VISITOR_H
#define VISITOR_H

#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/ADT/SmallVector.h>

#include <set>

namespace tesla
{

class AutomatonDescription;
class Usage;

class TeslaVisitor : public clang::RecursiveASTVisitor<TeslaVisitor>
{
public:
  TeslaVisitor(llvm::StringRef Filename, clang::ASTContext *Context);
  ~TeslaVisitor();

  bool VisitCallExpr(clang::CallExpr *);

  //! Visit a function declaration, looking for TESLA automata descriptions.
  bool VisitFunctionDecl(clang::FunctionDecl *);

  const llvm::ArrayRef<AutomatonDescription *> GetAutomata() const
  {
    return Automata;
  }

  const llvm::ArrayRef<const Usage *> RootAutomata() const
  {
    return Roots;
  }

  const std::set<std::string>& GetFunctionDefinitions() const
  {
      return functionDefinitions;
  }

private:
  const llvm::StringRef Filename;
  clang::ASTContext *Context;

  llvm::SmallVector<AutomatonDescription *, 2> Automata;
  llvm::SmallVector<const Usage *, 2> Roots;

  std::set<std::string> functionDefinitions;
};

} // namespace tesla

#endif

#ifndef __AST_HPP__
#define __AST_HPP__ 1

#include <iostream>
#include <string>
#include <vector>
#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/LinkAllPasses.h"

using namespace std;
using namespace llvm;

class ExprAST {
public:
    virtual Value* codegen() const = 0;
    virtual ~ExprAST() {}
};

class NumberExprAST : public ExprAST {
public:
    NumberExprAST(double x) 
    : Val(x) {}

    Value* codegen() const;

private:
    double Val;
};

class VariableExprAST : public ExprAST {
public:
    VariableExprAST(string s) 
    : Name(s) {}

    Value* codegen() const;

private:
    string Name;
};

class BinaryExprAST : public ExprAST {
public:
    BinaryExprAST(ExprAST* l, ExprAST* r)
    : LHS(l), RHS(r) {}

    ~BinaryExprAST();

private:
    BinaryExprAST(const BinaryExprAST&);
    BinaryExprAST& operator=(const BinaryExprAST&);

protected:
    ExprAST *LHS, *RHS;
};

class AddExprAST : public BinaryExprAST {
public:
    AddExprAST(ExprAST* l, ExprAST* r)
    : BinaryExprAST(l, r) {}

    Value* codegen() const;
};

class SubExprAST : public BinaryExprAST {
public:
    SubExprAST(ExprAST* l, ExprAST* r)
    : BinaryExprAST(l, r) {}

    Value* codegen() const;
};

class MulExprAST : public BinaryExprAST {
public:
    MulExprAST(ExprAST* l, ExprAST* r)
    : BinaryExprAST(l, r) {}

    Value* codegen() const;
};

class DivExprAST : public BinaryExprAST {
public:
    DivExprAST(ExprAST* l, ExprAST* r)
    : BinaryExprAST(l, r) {}

    Value* codegen() const;
};

class LtExprAST : public BinaryExprAST {
public:
    LtExprAST(ExprAST* l, ExprAST* r)
    : BinaryExprAST(l, r) {}

    Value* codegen() const;
};

class GtExprAST : public BinaryExprAST {
public:
    GtExprAST(ExprAST* l, ExprAST* r)
    : BinaryExprAST(l, r) {}

    Value* codegen() const;
};

class SeqExprAST : public BinaryExprAST {
public:
    SeqExprAST(ExprAST* l, ExprAST* r)
    : BinaryExprAST(l, r) {}

    Value* codegen() const;
};

class CallExprAST : public ExprAST {
public:
    CallExprAST(string s, vector<ExprAST*> v)
    : Callee(s), Args(v) {}

    Value* codegen() const;
    ~CallExprAST();

private:
    string Callee;
    vector<ExprAST*> Args;
    CallExprAST(const CallExprAST&);
    CallExprAST& operator=(const CallExprAST&);
};

class IfExprAST : public ExprAST {
public:
    IfExprAST(ExprAST* e1, ExprAST* e2, ExprAST* e3)
        : Cond(e1), Then(e2), Else(e3) {}
    
    Value* codegen() const;
    ~IfExprAST();

private:
    ExprAST *Cond, *Then, *Else;
    IfExprAST(const IfExprAST&);
    IfExprAST& operator=(const IfExprAST&);
};

class ForExprAST : public ExprAST {
public:
    ForExprAST(string s, ExprAST* e1, ExprAST* e2, ExprAST* e3, ExprAST* e4)
        : Start(e1), End(e2), Step(e3), Body(e4), VarName(s) {}
    
    Value* codegen() const;
    ~ForExprAST();

private:
    ExprAST *Start, *End, *Step, *Body;
    string VarName;
    ForExprAST(const ForExprAST&);
    ForExprAST& operator=(const ForExprAST&);
};

class PrototypeAST {
public:
    PrototypeAST(string s, vector<string> v)
    : Name(s), Args(v) {}

    Function* codegen() const;
    string getName() const {
        return Name;
    }

private:
    string Name;
    vector<string> Args;
};

class FunctionAST {
public:
    FunctionAST(PrototypeAST* p, ExprAST* e)
    : Proto(p), Body(e) {}

    Value* codegen() const;
    ~FunctionAST();

private:
    PrototypeAST* Proto;
    ExprAST* Body;
    FunctionAST(const FunctionAST&);
    FunctionAST& operator=(const FunctionAST&);
};

void InitializeModuleAndPassManager();
AllocaInst* CreateEntryBlockAlloca(Function* f, string s);

#endif
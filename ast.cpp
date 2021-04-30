#include "ast.hpp"

LLVMContext TheContext;
IRBuilder<> Builder(TheContext);
Module* TheModule;
map<string, Value*> NamedValues;

Value* NumberExprAST::codegen() const {
	return ConstantFP::get(TheContext, APFloat(Val));
}

Value* VariableExprAST::codegen() const {
    Value* V = NamedValues[Name];
    if (!V) {
        cerr << "Nepoznata promenlijva " << Name << endl;
        return nullptr;
    }

    return V;
}

BinaryExprAST::~BinaryExprAST() {
    delete LHS;
    delete RHS;
}

Value* AddExprAST::codegen() const {
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();
    
    if (!L || !R)
        return nullptr;
    
    return Builder.CreateFAdd(L, R, "addtmp");
}

Value* SubExprAST::codegen() const {
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();
    
    if (!L || !R)
        return nullptr;
    
    return Builder.CreateFSub(L, R, "subtmp");
}

Value* MulExprAST::codegen() const {
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();
    
    if (!L || !R)
        return nullptr;
    
    return Builder.CreateFMul(L, R, "multmp");
}

Value* DivExprAST::codegen() const {
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();
    
    if (!L || !R)
        return nullptr;
    
    return Builder.CreateFDiv(L, R, "divtmp");
}

CallExprAST::~CallExprAST() {
    for (auto e: Args)
        delete e;
}

Value* CallExprAST::codegen() const {
	return nullptr;
}

Value* PrototypeAST::codegen() const {
	return nullptr;
}

FunctionAST::~FunctionAST() {
    delete Proto;
    delete Body;
}

Value* FunctionAST::codegen() const {
	return nullptr;
}


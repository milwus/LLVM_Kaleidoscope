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
    Function *f = TheModule->getFunction(Callee);

    if (!f) {
        cerr << "Poziv nedefinisane funkcije " << Callee << endl;
        return nullptr;
    }

    if (Args.size() != f->arg_size()) {
        cerr << "Funkcija " << Callee << " je pozvana sa neodgovarajucim brojem argumenata" << endl;
        return nullptr;
    }

    vector<Value*> ArgsV;
    for (unsigned i = 0; i < Args.size(); i++) {
        Value* tmp = Args[i]->codegen();

        if (!tmp)
            return nullptr;

        ArgsV.push_back(tmp);
    }

    return Builder.CreateCall(f, ArgsV, "calltmp");
}

Function* PrototypeAST::codegen() const {
    vector<Type*> tmp;
    for (unsigned i = 0; i < Args.size(); i++)
        tmp.push_back(Type::getDoubleTy(TheContext));

    FunctionType* FT = FunctionType::get(Type::getDoubleTy(TheContext), tmp, false);

    Function* f = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);

    unsigned i = 0;
    for (auto &a : f->args())
        a.setName(Args[i++]);
    
    return f;
}

FunctionAST::~FunctionAST() {
    delete Proto;
    delete Body;
}

Value* FunctionAST::codegen() const {
    Function* f = TheModule->getFunction(Proto->getName());

    if (!f)
        f = Proto->codegen();
    
    if (!f)
        return nullptr;
    
    if (!f->empty()) {
        cerr << "Funkcija " << Proto->getName() << " ne moze da se redefinise" << endl;
        return nullptr;
    }

    BasicBlock* BB = BasicBlock::Create(TheContext, "entry", f);
    Builder.SetInsertPoint(BB);

    NamedValues.clear();

    for (auto &a : f->args())
        NamedValues[(string)a.getName()] = &a;
    
    Value* tmp = Body->codegen();
    if (tmp != nullptr){
        Builder.CreateRet(tmp);
        verifyFunction(*f);
        return f;
    }

    f->eraseFromParent();
    return nullptr;
}

#include "ast.hpp"

LLVMContext TheContext;
IRBuilder<> Builder(TheContext);
Module* TheModule;
map<string, AllocaInst*> NamedValues;
legacy::FunctionPassManager* TheFPM;

Value* NumberExprAST::codegen() const {
	return ConstantFP::get(TheContext, APFloat(Val));
}

Value* VariableExprAST::codegen() const {
    AllocaInst* V = NamedValues[Name];
    if (!V) {
        cerr << "Nepoznata promenljiva " << Name << endl;
        return nullptr;
    }

    return Builder.CreateLoad(V, Name);
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

Value* LtExprAST::codegen() const {
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();
    
    if (!L || !R)
        return nullptr;
    
    return Builder.CreateUIToFP(Builder.CreateFCmpOLT(L, R, "lttmp"), Type::getDoubleTy(TheContext), "booltmp");
}

Value* GtExprAST::codegen() const {
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();
    
    if (!L || !R)
        return nullptr;
    
    return Builder.CreateUIToFP(Builder.CreateFCmpOGT(L, R, "gttmp"), Type::getDoubleTy(TheContext), "booltmp");
}

Value* SeqExprAST::codegen() const {
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();
    
    if (!L || !R)
        return nullptr;
    
    return R;
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
    for (auto e: Args) {
        Value* tmp = e->codegen();

        if (!tmp)
            return nullptr;

        ArgsV.push_back(tmp);
    }

    return Builder.CreateCall(f, ArgsV, "calltmp");
}

Value* IfExprAST::codegen() const {
    Value* CondV = Cond->codegen();
    if (!CondV)
        return nullptr;
    
    Value* Tmp = Builder.CreateFCmpONE(CondV, ConstantFP::get(TheContext, APFloat(0.0)), "ifcond");
    
    Function* f = Builder.GetInsertBlock()->getParent();

    BasicBlock* ThenBB = BasicBlock::Create(TheContext, "then", f);
    BasicBlock* ElseBB = BasicBlock::Create(TheContext, "else");    //Moze treci argument da bude , f. U tom slucaju ne moram da radim dodavanje naknadno (obelezeno "***")
    BasicBlock* MergeBB = BasicBlock::Create(TheContext, "ifcont");
    
    Builder.CreateCondBr(Tmp, ThenBB, ElseBB);

    Builder.SetInsertPoint(ThenBB);
    Value* ThenV = Then->codegen();
    if(!ThenV)
        return nullptr;
    Builder.CreateBr(MergeBB);
    ThenBB = Builder.GetInsertBlock();

    // ***
    f->getBasicBlockList().push_back(ElseBB);   //getBasicBlockList vraca vektor BB-ova za funkciju f
    Builder.SetInsertPoint(ElseBB);
    Value* ElseV = Else->codegen();
    if(!ElseV)
        return nullptr;
    Builder.CreateBr(MergeBB);
    ElseBB = Builder.GetInsertBlock();
    
    f->getBasicBlockList().push_back(MergeBB);
    Builder.SetInsertPoint(MergeBB);
    PHINode* PHI = Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, "iftmp");
    PHI->addIncoming(ThenV, ThenBB);
    PHI->addIncoming(ElseV, ElseBB);

    return PHI;
}

IfExprAST::~IfExprAST() {
    delete Cond;
    delete Then;
    delete Else;
}

Value* ForExprAST::codegen() const {
    Value* StartV = Start->codegen();
    if (!StartV)
        return nullptr;
    
    Function* f = Builder.GetInsertBlock()->getParent();
    BasicBlock* LoopBB = BasicBlock::Create(TheContext, "loop", f);

    AllocaInst* Alloca = CreateEntryBlockAlloca(f, VarName);
    Builder.CreateStore(StartV, Alloca);
    
    Builder.CreateBr(LoopBB);
    Builder.SetInsertPoint(LoopBB);
     
    AllocaInst* OldVal = NamedValues[VarName];
    NamedValues[VarName] =  Alloca;

    Value* EndV = End->codegen();
    if(!EndV)
        return nullptr;
    
    Value* Tmp = Builder.CreateFCmpONE(EndV, ConstantFP::get(TheContext, APFloat(0.0)), "loopcond");
    BasicBlock* Loop1BB = BasicBlock::Create(TheContext, "loop1", f);
    BasicBlock* AfterLoopBB = BasicBlock::Create(TheContext, "afterloop");
    Builder.CreateCondBr(Tmp, Loop1BB, AfterLoopBB);

    Builder.SetInsertPoint(Loop1BB);
    Value* BodyV = Body->codegen();
    if (!BodyV)
        return nullptr;
    
    Value* StepV = Step->codegen();
    if (!StepV)
        return nullptr;
    Value* CurrVal = Builder.CreateLoad(Alloca, VarName);
    Value* NextVar = Builder.CreateFAdd(CurrVal, StepV, "nextvar");
    Builder.CreateStore(NextVar, Alloca);
    Builder.CreateBr(LoopBB);

    if (OldVal)
        NamedValues[VarName] = OldVal;
    else
        NamedValues.erase(VarName);

    f->getBasicBlockList().push_back(AfterLoopBB);
    Builder.SetInsertPoint(AfterLoopBB);

    return ConstantFP::get(TheContext, APFloat(0.0));
}

ForExprAST::~ForExprAST() {
    delete Start;
    delete End;
    delete Step;
    delete Body;
}

Value* AssignExprAST::codegen() const {
    Value* r = Expression->codegen();
    if (!r)
        return nullptr;
    
    Builder.CreateStore(r, NamedValues[Name]);
    return r;
}

AssignExprAST::~AssignExprAST() {
    delete Expression;
}

Value* VarExprAST::codegen() const {
    Function* f = Builder.GetInsertBlock()->getParent();

    vector<AllocaInst*> oldAllocas;
    for (auto elem: VarNames)
        oldAllocas.push_back(NamedValues[elem.first]);

    for (auto elem: VarNames) {
        AllocaInst* Alloca = CreateEntryBlockAlloca(f, elem.first);
        NamedValues[elem.first] = Alloca;

        Value* tmp = elem.second->codegen();
        if (!tmp)
            return nullptr;
        
        Builder.CreateStore(tmp, Alloca);
    }

    Value* b = Body->codegen();
        if (!b)
            return nullptr;
    
    for (unsigned i = 0; i < oldAllocas.size(); i++) {
        if (oldAllocas[i])
            NamedValues[VarNames[i].first] = oldAllocas[i];
        else
            NamedValues.erase(VarNames[i].first);
    }
    
    return b;
}

VarExprAST::~VarExprAST() {
    for (auto elem: VarNames)
        delete elem.second;

    delete Body;
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

    for (auto &a : f->args()) {
        AllocaInst* Alloca = CreateEntryBlockAlloca(f, (string)a.getName());
        NamedValues[(string)a.getName()] = Alloca;
        Builder.CreateStore(&a, Alloca);
    }

    Value* tmp = Body->codegen();
    if (tmp != nullptr){
        Builder.CreateRet(tmp);

        verifyFunction(*f);
        TheFPM->run(*f);  //optimizuje kod

        return f;
    }

    f->eraseFromParent();
    return nullptr;
}

void InitializeModuleAndPassManager(){
	TheModule = new Module("Mirkov modul", TheContext);
    TheFPM = new legacy::FunctionPassManager(TheModule);

    TheFPM->add(createInstructionCombiningPass());
    TheFPM->add(createReassociatePass());
    TheFPM->add(createGVNPass());
    TheFPM->add(createCFGSimplificationPass());
    TheFPM->add(createPromoteMemoryToRegisterPass());

    TheFPM->doInitialization();
}

AllocaInst* CreateEntryBlockAlloca(Function* f, string s){
    IRBuilder<> TmpBuilder(&(f->getEntryBlock()), f->getEntryBlock().begin());
    return TmpBuilder.CreateAlloca(Type::getDoubleTy(TheContext), 0, s);
}
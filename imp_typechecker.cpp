#include "imp_typechecker.hh"

ImpTypeChecker::ImpTypeChecker():inttype(), booltype(), voidtype(), maintype() {
  inttype.set_basic_type("int");
  booltype.set_basic_type("bool");
  voidtype.set_basic_type("void");
  // maintype function
  list<string> noparams;
  maintype.set_fun_type(noparams, "void");
}

// Methods used for the analysis of maximum pile height
void ImpTypeChecker::sp_incr(int n) {
  sp += n; 
  if (sp > max_sp) max_sp = sp;
}

void ImpTypeChecker::sp_decr(int n) {
  sp -= n;
  if (sp < 0) {
    cout << "stack less than 0" << endl;
    exit(0);
  }
}

void ImpTypeChecker::typecheck(Program* p) {
  env.clear();
  p->accept(this);
  cout << "Success" << endl;
  return;
}

void ImpTypeChecker::visit(Program* p) {
  env.add_level(); // create general scope
  ftable.add_level(); // new
  p->var_decs->accept(this);
  p->fun_decs->accept(this);
  if (!env.check("main")) {
    cout << "Program does not have main" << endl;
    exit(0);
  }
  ImpType funtype = env.lookup("main");
  if (!funtype.match(maintype)) {
    cout << "Wrong type of main: " << funtype << endl;
    exit(0);
  }

  env.remove_level();

  // Codigo usado para ver contenido de ftable
  cout << "Reporte ftable" << endl;
  for (int i = 0; i < fnames.size(); i++) {
    cout << "-- Function: " << fnames[i] << endl;
    FEntry fentry = ftable.lookup(fnames[i]);
    cout << fentry.fname << " : " << fentry.ftype << endl;
    cout << "max stack height: " << fentry.max_stack << endl;
    cout << "mem local variables: " << fentry.mem_locals << endl;
  }
  // no remover nivel de ftable porque sera usado por codegen.
  return;
}

void ImpTypeChecker::visit(Body* b) {
  int temp_dir = dir;
  env.add_level();
  b->var_decs->accept(this);
  b->slist->accept(this);
  env.remove_level();
  if (dir > max_dir) max_dir = dir;
  // restaurar direccion de entrada
  dir = temp_dir;
  return;
}

void ImpTypeChecker::visit(VarDecList* decs) {
  list<VarDec*>::iterator it;
  for (it = decs->vdlist.begin(); it != decs->vdlist.end(); ++it) {
    (*it)->accept(this);
  }  
  return;
}

void ImpTypeChecker::visit(VarDec* vd) {
  ImpType type;  
  type.set_basic_type(vd->type);
  if (type.ttype == ImpType::NOTYPE || type.ttype == ImpType::VOID) {
    cout << "Invalid type: " << vd->type << endl;
    exit(0);
  }
  list<string>::iterator it;
  for (it = vd->vars.begin(); it != vd->vars.end(); ++it) {
    env.add_var(*it, type);
    dir++;
    if (dir > max_dir) max_dir = dir;
  }   
  return;
}

void ImpTypeChecker::visit(StatementList* s) {
  list<Stm*>::iterator it;
  for (it = s->slist.begin(); it != s->slist.end(); ++it) {
    (*it)->accept(this);
  }
  return;
}

void ImpTypeChecker::visit(AssignStatement* s) {
  ImpType type = s->rhs->accept(this);
  if (!env.check(s->id)) {
    cout << "Variable " << s->id << " undefined" << endl;
    exit(0);
  }
  sp_decr(1);
  ImpType var_type = env.lookup(s->id);  
  if (!type.match(var_type)) {
    cout << "Wrong type in Assign a " << s->id << endl;
    exit(0);
  }
  return;
}

void ImpTypeChecker::visit(PrintStatement* s) {
  s->e->accept(this);
  sp_decr(1);
  return;
}

void ImpTypeChecker::visit(IfStatement* s) {
  if (!s->cond->accept(this).match(booltype)) {
    cout << "Conditional expression in IF must be bool" << endl;
    exit(0);
  }
  sp_decr(1);
  s->tbody->accept(this);
  if (s->fbody != NULL)
    s->fbody->accept(this);
  return;
}

//
void ImpTypeChecker::visit(ForDoStatement* s) {
  ImpType type = s->start->accept(this);
  if (!type.match(inttype)) {
    cout << "Start type in ForDoStatement must be int" << endl;
    exit(0);
  }
  type = s->end->accept(this);
  if (!type.match(inttype)) {
    cout << "End type in ForDoStatement must be int" << endl;
    exit(0);
  }

  env.add_var(s->id, inttype);

  int sp_before = sp;
  s->body->accept(this);
  sp = sp_before;
  return;

}

void ImpTypeChecker::visit(FCallStatement* e) {
  if (!env.check(e->fname)) {
    cout << "(Function call): " << "Function " << e->fname << " is not defined" << endl;
    exit(0);
  }
  ImpType funtype = env.lookup(e->fname);
  if (funtype.ttype != ImpType::FUN && funtype.ttype != ImpType::VOID) {
    cout << "(Function call): " << e->fname << " is not a function" << endl;
    exit(0);
  }
  int num_params = funtype.types.size()-1;
  int num_args = e->args.size();
  if (num_args != num_params) {
    cout << "Missing arguments in the function " << e->fname << endl;
    exit(0);
  }
  ImpType argtype;
  list<Exp*>::iterator it;
  int i = 0;
  for (it = e->args.begin(); it != e->args.end(); ++it) {
    argtype = (*it)->accept(this);
    if (argtype.ttype != funtype.types[i]) {
      cout << "(Function call) Argument type does not correspond to parameter type in fcall of: " << e->fname << endl;
      exit(0);
    }
    i++;
  }
  return;
}
//

void ImpTypeChecker::visit(WhileStatement* s) {
  if (!s->cond->accept(this).match(booltype)) {
    cout << "Conditional expression must be bool" << endl;
    exit(0);
  }
  sp_decr(1);
  s->body->accept(this);
 return;
}

ImpType ImpTypeChecker::visit(BinaryExp* e) {
  ImpType t1 = e->left->accept(this);
  ImpType t2 = e->right->accept(this);
  if (!t1.match(inttype) || !t2.match(inttype)) {
    cout << "Types in BinExp must be int" << endl;
    exit(0);
  }
  ImpType result;
  switch(e->op) {
  case PLUS: 
  case MINUS:
  case MULT:
  case DIV:
  case EXP:
    result = inttype;
    break;
  case LT: 
  case LTEQ:
  case EQ:
    result = booltype;
    break;
  }
  sp_decr(1);
  return result;
}

ImpType ImpTypeChecker::visit(NumberExp* e) {
  sp_incr(1);
  return inttype;
}

ImpType ImpTypeChecker::visit(IdExp* e) {
  if (env.check(e->id)) {
    sp_incr(1);
    return env.lookup(e->id);
  }
  else {
    cout << "Undefined variable: " << e->id << endl;
    exit(0);
  }
}

ImpType ImpTypeChecker::visit(ParenthExp* ep) {
  return ep->e->accept(this);
}

ImpType ImpTypeChecker::visit(CondExp* e) {
  if (!e->cond->accept(this).match(booltype)) {
    cout << "Type in ifexp must be bool" << endl;
    exit(0);
  }
  sp_decr(1);
  int sp_start = sp;
  ImpType ttype =  e->etrue->accept(this);
  sp = sp_start;
  if (!ttype.match(e->efalse->accept(this))) {
    cout << "Types in ifexp must be equal" << endl;
    exit(0);
  }
  return ttype;
}

ImpType ImpTypeChecker::visit(TrueFalseExp* e) {
  sp_incr(1);
  return booltype;
}

void ImpTypeChecker::visit(FunDecList* s) {
  list<FunDec*>::iterator it;
  for (it = s->fdlist.begin(); it != s->fdlist.end(); ++it) {
    add_fundec(*it);
  } 
  for (it = s->fdlist.begin(); it != s->fdlist.end(); ++it) {
    // inicializar valores de sp, max_sp, dir, max_dir
    sp = max_sp = 0;
    dir = max_dir = 0;
    (*it)->accept(this);
    FEntry fentry;
    string fname = (*it)->fname;
    fentry.fname = fname;
    fentry.ftype = env.lookup(fname);
    fnames.push_back(fname);
    fentry.max_stack = max_sp;
    fentry.mem_locals = max_dir;
    ftable.add_var(fname, fentry);
  }
  return;
}

void ImpTypeChecker::add_fundec(FunDec* fd) {
  ImpType funtype;
  if (!funtype.set_fun_type(fd->types, fd->rtype)) {
    cout << "Invalid type in function declaration: " << fd->fname << endl;
    exit(0);
  }
  env.add_var(fd->fname, funtype);
  return;
}

void ImpTypeChecker::visit(FunDec* fd) {
  env.add_level();
  ImpType funtype = env.lookup(fd->fname);
  ImpType rtype, ptype;
  rtype.set_basic_type(funtype.types.back());
  list<string>::iterator it;
  int i = 0;
  for (it = fd->vars.begin(); it != fd->vars.end(); ++it, i++) {
    ptype.set_basic_type(funtype.types[i]);
    env.add_var(*it, ptype);
  } 
  env.add_var("return", rtype);
  // add_fundec(fd); 
  fd->body->accept(this);
  env.remove_level();
  return;
}

void ImpTypeChecker::visit(ReturnStatement* s) {
  ImpType rtype = env.lookup("return");
  ImpType etype;
  if (s->e != NULL) {
    etype = s->e->accept(this);
    sp_decr(1);
  }
  else
    etype = voidtype;
  if (!rtype.match(etype)) {
    cout << "Return type mismatch: " << rtype << "<->" << etype << endl;
    exit(0);
  }
  return;
}

ImpType ImpTypeChecker::visit(FCallExp* e) {
  if (!env.check(e->fname)) {
    cout << "(Function call): " << e->fname <<  " is undeclared" << endl;
    exit(0);
  }
  ImpType funtype = env.lookup(e->fname);
  if (funtype.ttype != ImpType::FUN) {
    cout << "(Function call): " << e->fname <<  " is not a function" << endl;
    exit(0);
  }

  // check args
  int num_fun_args = funtype.types.size() - 1;
  int num_fcall_args = e->args.size();
  ImpType rtype;
  rtype.set_basic_type(funtype.types[num_fun_args]);

  // que hacer con sp y el valor de retorno?
  if (rtype.ttype != ImpType::VOID) sp_incr(1);

  if (num_fun_args != num_fcall_args) {
    cout << "(Function call) Number of arguments does not correspond to declaration of: " << e->fname << endl;
    exit(0);
  }
  ImpType argtype;
  list<Exp*>::iterator it;
  int i = 0;
  for (it = e->args.begin(); it != e->args.end(); ++it) {
    argtype = (*it)->accept(this);
    if (argtype.ttype != funtype.types[i]) {
      cout << "(Function call) Argument type does not correspond to parameter type in fcall of: " << e->fname << endl;
      exit(0);
    }
    i++;
  }
  sp_decr(1); //
  
  return rtype;
}

# Proyecto Parte II
**Integrantes:** 
- Kelvin Cahuana Condori
- Sergio Sotil Lozada
- Mauro Bobadilla Castillo

## 0. Run Program:
```
git clone https://github.com/MauBC/CompiProyecto2.git
cd CompiProyecto2
make
make svm
```
Ejemplo de uso:
```
./compile.exe .\scripts\ejemplo4.imp
./svm.exe .\scripts\ejemplo4.imp.svm
```
## 1. El TypeChecker y Codegen
**REPORTE:** Describir las partes del programa y estrategias para calcular:

* Typechecker, por función: El espacio requerido para almacenar las variables locales y la
altura máxima de pila.

* Las direcciones de las variables globales y locales (parcialmente implementada), parámetros y posición en la pila del valor de retorno.

**Implementación en el Typechecker**
* El espacio requerido para las variables locales se calcula iterativamente y comprobando si `dir > max_dir`.
* Cada vez que procesamos un nuveo `Body` generamos un nuevo nivel en el enviroment de las variables.
* Luego cuando agregamos nuevas variables al nivel actual comrpobamos si `dir` es mayor que `max_dir` para actualizar su valor.
* Para ello es neceario usar la variable auxiliar `temp_dir` para guarder el valor de `dir` antes de generar un nuevo nivel.
```cpp
...
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
...
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
...
```
* La altura máxima de la pila se calcula llamando a estás dos funciones en los demás métodos visit.
```cpp
...
void ImpTypeChecker::sp_incr(int n) {
  sp += n; 
  if (sp > max_sp) max_sp = sp;
}
...
void ImpTypeChecker::sp_decr(int n) {
  sp -= n;
  if (sp < 0) {
    cout << "stack less than 0" << endl;
    exit(0);
  }
}
...
````
* Los métodos que aumentan el `sp` son los siguientes ya que requiren hacer `push` o `load` a la pila.
```cpp
ImpType ImpTypeChecker::visit(NumberExp* e) {
  sp_incr(1); // <-
  return inttype;
}
ImpType ImpTypeChecker::visit(IdExp* e) {
  if (env.check(e->id)) {
    sp_incr(1); // <-
    return env.lookup(e->id);
  }
  else {
    cout << "Undefined variable: " << e->id << endl;
    exit(0);
  }
}
ImpType ImpTypeChecker::visit(TrueFalseExp* e) {
  sp_incr(1);
  return booltype;
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
  if (rtype.ttype != ImpType::VOID) sp_incr(1); <-

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
  sp_decr(1); <-
  return rtype;
}
```
* Los métodos que decrementan el `sp` son los siguientes ya que requiren hacer `store` del top de la pila o realizan alguna operación en la pila.
```cpp
void ImpTypeChecker::visit(AssignStatement* s) {
  ImpType type = s->rhs->accept(this);
  if (!env.check(s->id)) {
    cout << "Variable " << s->id << " undefined" << endl;
    exit(0);
  }
  sp_decr(1); // <-
  ImpType var_type = env.lookup(s->id);  
  if (!type.match(var_type)) {
    cout << "Wrong type in Assign a " << s->id << endl;
    exit(0);
  }
  return;
}

void ImpTypeChecker::visit(PrintStatement* s) {
  s->e->accept(this);
  sp_decr(1); // <-
  return;
}

void ImpTypeChecker::visit(IfStatement* s) {
  if (!s->cond->accept(this).match(booltype)) {
    cout << "Conditional expression in IF must be bool" << endl;
    exit(0);
  }
  sp_decr(1); // <-
  s->tbody->accept(this);
  if (s->fbody != NULL)
    s->fbody->accept(this);
  return;
}
void ImpTypeChecker::visit(WhileStatement* s) {
  if (!s->cond->accept(this).match(booltype)) {
    cout << "Conditional expression must be bool" << endl;
    exit(0);
  }
  sp_decr(1); // <-
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
  sp_decr(1); // <-
  return result;
}
ImpType ImpTypeChecker::visit(CondExp* e) {
  if (!e->cond->accept(this).match(booltype)) {
    cout << "Type in ifexp must be bool" << endl;
    exit(0);
  }
  sp_decr(1); // <-
  int sp_start = sp;
  ImpType ttype =  e->etrue->accept(this);
  sp = sp_start;
  if (!ttype.match(e->efalse->accept(this))) {
    cout << "Types in ifexp must be equal" << endl;
    exit(0);
  }
  return ttype;
}
void ImpTypeChecker::visit(ReturnStatement* s) {
  ImpType rtype = env.lookup("return");
  ImpType etype;
  if (s->e != NULL) {
    etype = s->e->accept(this);
    sp_decr(1); // <-
  }
  else
    etype = voidtype;
  if (!rtype.match(etype)) {
    cout << "Return type mismatch: " << rtype << "<->" << etype << endl;
    exit(0);
  }
  return;
}
// También ImpType ImpTypeChecker::visit(FCallExp* e)
```
* Luego cuando se recorre la lista de declaraciones de funciones se setean los valores correspondiente máximos de `max_dir` y `max_sp`.
```cpp
...
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
    fentry.max_stack = max_sp; // <-
    fentry.mem_locals = max_dir; // <-
    ftable.add_var(fname, fentry);
  }
  return;
}
```
* Es importante también mencionar que se uso la estrucutura `FEntry` y un eviroment de funciones con un solo nivel. No es necesario más niveles ya que la gramática IMP-FUN no nos permite declarar funciones dentro de funciones.

**Implementación en el Codegen**
* Para las direcciones de las varibales globales y locales hemos considerado una estructura `VarEntry`
```cpp
class VarEntry {
public:
  int dir;
  bool is_global;
};
```
* Cuando estamos procesando una función generamos un nuevo scope y seteamos el `ventry.is_global` al `false`. También, calculamos las direción respectivas de la variables locales en base a la fórmula `ventry_dir = i - (num_params + 3)`
```cpp
void ImpCodeGen::visit(FunDec* fd) {
  FEntry fentry = analysis->ftable.lookup(fd->fname);
  current_dir = 0;
  int m = fd->types.size();
  VarEntry ventry;

  int i = 1;
  for(auto it = fd->vars.begin(); it != fd->vars.end(); ++it) {
    ventry.dir = i - (m + 3);
    ventry.is_global = false;
    addresses.add_var(*it, ventry);
    i++;
  }

  ventry.dir = - (m + 3); // <-
  ventry.is_global = false; // <-
  addresses.add_var("return", ventry);

  codegen(get_flabel(fd->fname),"skip");
  codegen(nolabel,"enter",fentry.mem_locals + fentry.max_stack);
  codegen(nolabel,"alloc",fentry.mem_locals);
  num_params = m;

  fd->body->accept(this);
  return;
}
```

* La posición en la pila del valor del retorno se calcula como `-(num_params + 3)` donde `num_params` es la cantidad de parámetros de la función que se está procesando. Se suma 3 porque también tenemos que considerar el Program Counter (PC), Frame Pointer (FP) y Extreme Pointer (EP) del Activation Record (AR).
```cpp
void ImpCodeGen::visit(ReturnStatement* s) {
  if(s->e != NULL){
    s->e->accept(this);
    codegen(nolabel, "storer", -(num_params + 3));
  }
  codegen(nolabel, "return", num_params + 3); 
  return;
}
```

## 2. Implementar FCallStm
**REPORTE:** Indicar los cambios al programa (parser, typechecker, codegen, etc) y las definiciones de typecheck y codegen.

**Modificaciones en el Parser:**
* Dentro de la primera condición `match(Token::ID)` se agregó un nivel más de anidameniento para comprobar el caso de FCallStatement o AssignStatement.
```cpp
Stm* Parser::parseStatement() {
...
  if (match(Token::ID)) {
      string lex = previous->lexema;
      if (check(Token::ASSIGN) || check(Token::LPAREN)) {
        if (match(Token::ASSIGN)) {
          s = new AssignStatement(lex, parseCExp());
        } else if (match(Token::LPAREN)) {
          list<Exp*> args;
          if (!check(Token::RPAREN)) {
            args.push_back(parseCExp());
            while(match(Token::COMMA)) {
              args.push_back(parseCExp());
            }
          }
          if (!match(Token::RPAREN)) parserError("Expecting rparen");
          return new FCallStatement(lex, args); // <-
        }
        else {
          cout << "Error: esperaba = o (" << endl;
          exit(0);
        }      
      }
  }
...
```
**Modificaciones en el Typechecker:**
* Se agregó una nueva función visit para FCallStatement.
```cpp
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
```
**Modificaciones en el Codegen:**
* Se agregó una nueva función visit para FCallStatement.
```cpp
void ImpCodeGen::visit(FCallStatement* s) {
  FEntry fentry = analysis->ftable.lookup(s->fname);
  ImpType ftype = fentry.ftype;

  for (auto it = s->args.begin(); it != s->args.end(); ++it) {
    (*it)->accept(this);
  }
  codegen(nolabel, "pusha", get_flabel(s->fname));
  codegen(nolabel, "call");
  return;
}
```

**Modificaciones en el Interpreter:**
* Se agregó una nueva función visit para FCallStatement.
```cpp
void ImpInterpreter::visit(FCallStatement* s) {
  FunDec* fdec = fdecs.lookup(s->fname);
  env.add_level();
  list<Exp*>::iterator it;
  list<string>::iterator varit;
  list<string>::iterator vartype;
  ImpVType tt;
  if (fdec->vars.size() != s->args.size()) {
    cout << "Error: Incorrect number of parameters in call to " << fdec->fname << endl;
    exit(0);
  }
  for (it = s->args.begin(), varit = fdec->vars.begin(), vartype = fdec->types.begin();
       it != s->args.end(); ++it, ++varit, ++vartype) {
    tt = ImpValue::get_basic_type(*vartype);
    ImpValue v = (*it)->accept(this);
    if (v.type != tt) {
      cout << "FCall error: Param and arg types do not match in function " << fdec->fname << endl;
      exit(0);
    }
    env.add_var(*varit, v);
  }
  retcall = false;
  fdec->body->accept(this);
  if (!retcall) {
    cout << "Error: Function " << s->fname << " don't execute RETURN" << endl;
    exit(0);
  }
  retcall = false;
  env.remove_level();
  return;
} 
```
**Modificaciones en el Printer:**
* Se agregó una nueva función visit para FCallStatement.
```cpp
void ImpPrinter::visit(FCallStatement* s) {
  cout << getIndentation() << s->fname << "(";
  list<Exp*>::iterator it;
  bool first = true;
  for (it = s->args.begin(); it != s->args.end(); ++it) {
    if (!first) cout << ", ";
    first = false;
    (*it)->accept(this);
  }
  cout << ")";
  return;
}
````
**Definicición del typecheck**
```
tcheck(env,FCallStm(fname,e1::..::env)
		ifi
		env(fname) = (T1,...,Tn)->Tret
		tcheck(env,ei) = Ti
```

**Definicición del codegen**
```
codegen(FCallStatement(g,e1,...,en)) =
		alloc 1 // espacio para valor de retorno (opt.)
		codegen(e1)
		...
		codegen(en)
		mark
		pusha _g // dirección de inicio de g
		call
```

## 3. Implementar ForDoStm
**REPORTE:** Indicar los cambios al programa (parser, typechecker, codegen, etc) y las definiciones de typecheck y codegen

**Modificaciones en el Parser:**
* Se agregó los tres nuevos Tokens necesarios al final.
```cpp
const char* Token::token_names[35] = {
  "LPAREN" , "RPAREN", "PLUS", "MINUS", "MULT","DIV","EXP","LT","LTEQ",
  "NUM", "ID", "PRINT", "SEMICOLON", "COMMA", "ASSIGN", "CONDEXP", "IF", "THEN", "ELSE", "ENDIF", "WHILE", "DO",
  "ENDWHILE", "ERR", "END", "VAR", "RETURN", "FUN", "ENDFUN", "TRUE", "FALSE", "FOR", "IN", "ENDFOR" };
...
Scanner::Scanner(string s):input(s),first(0),current(0) {
  reserved["print"] = Token::PRINT;
  reserved["ifexp"] = Token::CONDEXP;
  reserved["if"] = Token::IF;
  reserved["then"] = Token::THEN;
  reserved["else"] = Token::ELSE;
  reserved["endif"] = Token::ENDIF;
  reserved["while"] = Token::WHILE;
  reserved["do"] = Token::DO;
  reserved["endwhile"] = Token::ENDWHILE;
  reserved["var"] = Token::VAR;
  reserved["return"] = Token::RETURN;
  reserved["fun"] = Token::FUN;
  reserved["endfun"] = Token::ENDFUN;
  reserved["true"] = Token::TRUE;
  reserved["false"] = Token::FALSE;
  reserved["for"] = Token::FOR;
  reserved["in"] = Token::IN;
  reserved["endfor"] = Token::ENDFOR;
}
```

* Se agregó un nuevo bloque else-if para parsear el caso del ForDoStatement.
```cpp
Stm* Parser::parseStatement() {
  ...
  } else if(match(Token::FOR)) {
    if (!match(Token::ID)) parserError("Expecting id in for statement");
    string id = previous->lexema;
    if (!match(Token::IN)) parserError("Expecting 'in' in for statement");
    if (!match(Token::LPAREN)) parserError("Expecting left parenthesis");
    Exp* start = parseCExp();
    if (!match(Token::COMMA)) parserError("Expecting comma in for statement");
    Exp* end = parseCExp();
    if (!match(Token::RPAREN)) parserError("Expecting right parenthesis");
    if (!match(Token::DO)) parserError("Expecting 'do' in for statement");
    tb = parseBody();
    if (!match(Token::ENDFOR)) parserError("Expecting 'endfor' in for statement");
    s = new ForDoStatement(id,start,end,tb);
  } 
```
**Modificaciones en el Typechecker:**
* Se agregó una nueva función visit para ForDoStatement.

```cpp
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
```
**Modificaciones en el Codegen:**
* Se agregó una nueva función visit para ForDoStatement.

```cpp
void ImpCodeGen::visit(ForDoStatement* s) {
  string l1 = next_label();
  string l2 = next_label();
  
  current_dir++;
  VarEntry ventry;
  ventry.dir = current_dir;
  ventry.is_global = false;
  addresses.add_var(s->id, ventry);
  s->start->accept(this);
  codegen(nolabel, "store", ventry.dir);
  codegen(l1, "skip");
  codegen(nolabel, "load", ventry.dir);
  s->end->accept(this);
  codegen(nolabel, "sub");
  codegen(nolabel, "jmpz", l2);
  s->body->accept(this);
  codegen(nolabel, "load", ventry.dir);
  codegen(nolabel, "push", 1);
  codegen(nolabel, "add");
  codegen(nolabel, "store", ventry.dir);
  codegen(nolabel, "goto", l1);
  codegen(l2, "skip");
  return;
}
```
**Modificaciones en el Interpreter:**
* Se agregó una nueva función visit para ForDoStatement.

```cpp
void ImpInterpreter::visit(ForDoStatement* s) {
  ImpValue start = s->start->accept(this);
  ImpValue end = s->end->accept(this);
  if (start.type != TINT || end.type != TINT) {
    cout << "Error: Start and end types in the for must be integers" << endl;
    exit(0);
  }
  env.add_level();
  env.add_var(s->id, start);
  ImpValue v;
  v.set_default_value(TINT);
  if (start.int_value > end.int_value) {
    cout << "start > end in for" << endl;
  }
  for (int i = start.int_value; i <= end.int_value; i++) {
    v.int_value = i;
    env.update(s->id, v);
    s->body->accept(this);
  }
  env.remove_level();
  return;
}
```

**Modificaciones en el Printer:**
* Se agregó una nueva función visit para ForDoStatement.

```cpp
void ImpPrinter::visit(ForDoStatement* s) {
  cout << getIndentation() << "for " << s->id << " in (";
  s->start->accept(this);
  cout << ", ";
  s->end->accept(this);
  cout << ") do" << endl;
  s->body->accept(this);
  cout << getIndentation() << "endfor";
  return;
}
```
**Definicición del typecheck**
```
tcheck(env,ForDoStatement(id,e1,e2,body)) ifi
	tcheck(env,id) = env(x) && tcheck(env,e1) = int && tcheck(env,e2) = 1
	&& tcheck(env,body) 
```
**Definicición del codegen**
```
codegen(ForDoStatement(id, e1, e2, body)):
	enter k 
	codegen(id) // Genera código para el identificador (puede ser una variable o contador)
	mark	
	codegen(e1) // Genera código para la expresión de inicio (puede ser la inicialización del contador)
	codegen(e2) // Genera código para la condición de continuación (puede ser la condición de salida del bucle)
	codegen(body); // Genera código para el cuerpo del bucle	
	goto mark
	loop_end:skip
```

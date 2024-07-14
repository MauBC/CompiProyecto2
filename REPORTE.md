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

* Las direcciones de las variables globales y locales (parcialmente implementada), parámetros y posición en la pila del valor de retorno.*






## 2. Implementar FCallStm
**REPORTE:** Indicar los cambios al programa (parser, typechecker, codegen, etc) y las definiciones de typecheck y codegen.



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

```

**Definicición del codegen**
```
```
# Interpreter
Interpreter built upon lex_analyzer

## Based on the following context-free-grammars

Program   ::= StmtList

StmtList  ::= Stmt | Stmt StmtList

Stmt      ::= PRINT Expr SC | SET ID Expr SC

Expr      ::= Expr PLUS Term | Expr MINUS Term | Term

Term      ::= Term STAR Primary | Term SLASH Primary | Primary

Primary ::= ID | INT | STRING

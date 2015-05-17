/*
	Michael Gonzalez
	CS 280 
	Program #4 - Interpreter
	05/08/2015
	v4.0
*/

#include <iostream>
#include <fstream>
#include "p2lex.h"
#include <string>
#include <vector>
#include <sstream>
#include <map>

#define INVALID_TOKEN 	(-1)
#define INVALID_INDEX 	(-1)
#define NO_OP			(Token(INVALID_TOKEN))
#define TREE_HALF_PAD	("\t")
#define TREE_PADDING	("\t\t")


using namespace std;

/*
	Program ::= StmtList
	StmtList ::= Stmt | Stmt StmtList
	Stmt ::= PRINT Expr SC | SET ID Expr SC
	Expr ::= Expr PLUS Term | Expr MINUS Term | Term
	Term ::= Term STAR Primary | Term SLASH Primary | Primary
	Primary ::= ID | INT | STRING
*/

enum Production
{
	PROGRAM,
	STMTLIST,
	STMT,
	EXPR,
	TERM,
	PRIMARY,
	PRIM_ID, 	// sub productions
	PRIM_INT,	//
	PRIM_STR, 	//
	NONE
};

class Symbol // terminals and nonterminals
{
	public:
		Token type;
		string txt;
		void display()
		{
			//cout << "Type: " << type << " | Txt: " << txt;
			cout << "txt: " << txt;
		}
};

class NonTerminal
{
	public:
		vector<Symbol*> sym;
};

class PTree {
public:
	PTree * left;
	PTree * right;
	Token op; // operation done on two leaves
	Production prod;
	Symbol * sym; // used for edge nodes 

	PTree(PTree * left, PTree * right, Token op, Production prod) 
	{
		//cout << "PTree_prod(" << left << ", " << right << ", " << op << ", " << prod << ")\n";
		this->left = left;
		this->right = right;
		this->op = op;
		this->prod = prod;
		sym = 0;
	}
	PTree(PTree * left, PTree * right, Token op, Symbol * sym) 
	{
		//cout << "PTree_sym(" << left << ", " << right << ", " << op << ", " << sym << ")\n";
		this->left = left;
		this->right = right;
		this->op = op;
		this->sym = sym;
		this->prod = NONE;
		//this->sym->display();
	}
	void display()
	{
		if(this == 0) return;
		if(this->sym != 0)
		{
			cout << "Parent:\n\t";
			this->sym->display();
		}
		
		if(this->left != 0) 
		{
			cout << "\nLeft (" << this->op << "):\n\t";
			this->left->display();
			
		}
		if(this->right != 0) 
		{
			cout << "\nRight:\n\t";
			this->right->display();
		}
		cout << endl;
	}
};

extern PTree * Program(istream * br);
extern PTree * StmtList(istream * br);
extern PTree * Stmt(istream * br);
extern PTree * Expr(NonTerminal * expr);
extern PTree * Term(NonTerminal * term);
extern PTree * Primary(Symbol * primary);

string stripQuotes(string txt);
const char * getProdName(Production p);
const char * getOpName(Token p);
map<string,Symbol*> ident;
bool error;
Symbol * Eval_Tree(PTree * pt);
PTree * Primary_subroutine(Symbol * primary, Production sub_prod);
int CountLeaves(PTree * pt);
void display_addToLevel(PTree * pt, int row, int col, Token op);
void display_printLevels();

class displayNode
{
	public:
		Symbol * sym;
		int row, col;
		Token op;
		Production prod;
		displayNode(Production setprod, int setrow, int setcol, Token setop) // production, no symbol
		{
			this->sym = 0;
			this->row = setrow;
			this->col = setcol;
			this->prod = setprod;
			this->op = setop;
		}
		displayNode(Symbol * setsym, int setrow, int setcol, Token setop) // symbol, no production
		{
			this->sym = setsym;
			this->row = setrow;
			this->col = setcol;
			this->prod = NONE;
			this->op = setop;
		}
};

vector<vector<displayNode> *> level; // vector containing pointer to vector of displayNodes

// Program ::= StmtList
PTree *Program(istream * br)
{
	return new PTree(StmtList(br), 0, NO_OP, PROGRAM);
}

// StmtList ::= Stmt | Stmt StmtList
PTree *StmtList(istream * br)
{
	PTree * stmt = Stmt(br);
	if(stmt) 
	{
		Eval_Tree(stmt);
		return new PTree(stmt, StmtList(br), NO_OP, STMTLIST);
	}
	return 0;
}

// Stmt ::= PRINT Expr SC | SET ID Expr SC
PTree * Stmt(istream * br)
{
	string txt;
	Token type = getToken(br, txt);
	if(type == DONE) return 0;
	if(type != PRINT && type != SET)
	{
		cout << "ERROR: \'" << txt << "\' not a valid operation." << endl;
		error = true;
		return 0;
	}
	NonTerminal * expr = new NonTerminal();
	Symbol * task, * sym; 
	task = new Symbol(); // task is PRINT | SET
	task->type = type;
	task->txt = txt;

	Symbol * id = new Symbol();
	if(task->type == SET)
	{

		id->type = ID;
		getToken(br, txt);
		id->txt = txt;
	}

	type = getToken(br, txt);
	if(type == SC || type == DONE)
	{
		cout << "ERROR: No primary found." << endl;
		cout << "ERROR: \'" << task->txt << "\' expected an expression." << endl;
		error = true;
	}
	while(type != SC) // iterating through expr
	{
		if(type == DONE)
		{
			cout << "ERROR: Expected semicolon." << endl;
			error = true;
			return 0;
		}
		if(type == ID)
		{
			if(ident.count(txt) != 0) // ID exists
			{
				sym = ident[txt];
			}
			else 
			{
				error = true;
				cout << "ERROR: ID \'" << txt << "\' has not been defined." << endl;
				type = getToken(br, txt);
				if(expr->sym.size() <= 0) continue; // empty vector?
				Symbol * err = expr->sym.back(); // check for hanging operator
				if(err == 0) continue;
				if(err->type == PLUS || err->type == MINUS || err->type == STAR || err->type == SLASH)
				{
					cout << "WARNING: Removed hanging operator before bad ID." << endl;
					expr->sym.pop_back(); // pop it if its an operator
				}
				continue;
			}
		}
		else
		{
			sym = new Symbol();
			sym->type = type;
			sym->txt = txt;
		}
		expr->sym.push_back(sym);
		type = getToken(br, txt);
	}
	//pushbackToken(SC, ";");
	PTree * pt = 0;
	if(expr->sym.size() > 0) pt = Expr(expr); // non-empty expr
	/*
	int evaled = Eval(pt);
	if(type == PRINT) cout << "Printing: " << evaled << endl;
	else if(type == SET) Set(ID, evaled);
	*/
	if(task->type == SET) 
	{
		return new PTree(Primary_subroutine(id, PRIM_ID), pt, task->type, STMT);
	}
	return new PTree(new PTree(0, 0, NO_OP, task), pt, task->type, STMT);
}

// Expr ::= Expr PLUS Term | Expr MINUS Term | Term
PTree * Expr(NonTerminal * expr)
{
	int last = INVALID_INDEX;
	for(int i = 0; i < expr->sym.size(); i++) if(expr->sym[i]->type == PLUS || expr->sym[i]->type == MINUS) last = i; // find last plus or minus
	if(last == INVALID_TOKEN) return new PTree(Term(expr), 0, NO_OP, EXPR); // Term only

	NonTerminal * subexpr = new NonTerminal();
	NonTerminal * term = new NonTerminal();
	for(int i = 0; i < last; i++) subexpr->sym.push_back(expr->sym[i]); // create sub expression
	for(int i = last + 1; i < expr->sym.size(); i++) term->sym.push_back(expr->sym[i]); // create term
	return new PTree(Expr(subexpr), Term(term), expr->sym[last]->type, EXPR);
}

// Term ::= Term STAR Primary | Term SLASH Primary | Primary
PTree * Term(NonTerminal * term)
{
	int last = INVALID_INDEX;
	for(int i = 0; i < term->sym.size(); i++) if(term->sym[i]->type == STAR || term->sym[i]->type == SLASH) last = i; // find last star or slash
	if(last == INVALID_TOKEN) return new PTree(Primary(term->sym[0]), 0, NO_OP, TERM); // Primary only

	NonTerminal * subterm = new NonTerminal();
	Symbol * primary = new Symbol();
	for(int i = 0; i < last; i++) subterm->sym.push_back(term->sym[i]); // create sub term

	if((last + 1) >= term->sym.size()) return new PTree(Term(subterm), 0, term->sym[last]->type, TERM); // out of bounds
	return new PTree(Term(subterm), Primary(term->sym[last + 1]), term->sym[last]->type, TERM);
}

// Primary ::= ID | INT | STRING
PTree * Primary(Symbol * primary)
{
	Token type = primary->type;
	if(type != ID && type != INT && type != STRING) return 0;
	if(type == ID) return new PTree(Primary_subroutine(primary, PRIM_ID), 0, NO_OP, PRIMARY); // Primary -> ID -> 'var1'
	else if(type == INT) return new PTree(Primary_subroutine(primary, PRIM_INT), 0, NO_OP, PRIMARY); // Primary -> INT -> 21
	else if(type == STRING) return new PTree(Primary_subroutine(primary, PRIM_STR), 0, NO_OP, PRIMARY); // Primary -> STRING -> "teststr"
}

PTree * Primary_subroutine(Symbol * primary, Production sub_prod)
{
	return new PTree(new PTree(0, 0, NO_OP, primary), 0, NO_OP, sub_prod);
}

int main(int argc, char * argv[])
{
	istream * br;
	ifstream infile;
	// check args and open the file
	if(argc == 1) br = &cin;
	else if(argc != 2) return 1; // print an error msg
	else 
	{
		infile.open(argv[1]);
		if(infile.is_open()) br = &infile;
		else 
		{
			cout << argv[1] << " can't be opened" << endl;
			return 1;
		}
	}
	PTree * tree = Program(br);
	/*
	if(error == false) 
	{
		//display_addToLevel(tree, 0, 0, NO_OP);
		//display_printLevels();
		//cout << "Leaves counted: " << CountLeaves(tree) << endl;
	}
	//else cout << "Errors detected. Unable to display parse tree." << endl;
	*/
}

void display_addToLevel(PTree * pt, int row, int col, Token op)
{
	if(int(level.size() - 1) < row) 
	{
		level.push_back(new vector<displayNode>); // first time at this level?
	}
	if(pt == 0) // empty node
	{
		return;
	}
	if(pt->prod != NONE) // production
	{
		displayNode dn(pt->prod, row, col, op);
		level[row]->push_back(dn);
	}
	else if(pt->sym != 0) // symbol
	{
		displayNode dn(pt->sym, row, col, NO_OP);
		level[row]->push_back(dn);
	}
	if(pt->left != 0) 
	{
		display_addToLevel(pt->left, row + 1, col, pt->op); // "we have to go deeper"
		if(pt->right && pt->left->right != 0) display_addToLevel(pt->right, row + 1, col + 2, NO_OP);
		else if(pt->right != 0) display_addToLevel(pt->right, row + 1, col + 1, NO_OP);
	}
	return;
}

void display_printLevels()
{
	cout << endl;
	int lvl = 0;

	for(int i = 0; i < level.size(); i++) // for every row
	{
		int rowc = 0;
		for(int j = 0; j < level[i]->size(); j++) // for every col, i.e. - print this row
		{
			if(level[i]->at(j).col > rowc) // we passed some empty spaces
			{
				cout << TREE_PADDING; // fill out empty node space
				j--;
				rowc++;
				continue;
			}
			else if(level[i]->at(j).sym == 0) // production
			{
				if(level[i]->at(j).op != PRINT && level[i]->at(j).op != NO_OP) // there is an operation
				{
					cout << getProdName(level[i]->at(j).prod) << getOpName(level[i]->at(j).op) << " --->\t";
				}
				else cout << getProdName(level[i]->at(j).prod) << TREE_PADDING; // no op
			}
			else // symbol
			{
				if(level[i]->at(j).sym->type == PRINT) 
				{
					cout << level[i]->at(j).sym->txt << "  --->\t";
				}
				else cout << level[i]->at(j).sym->txt << TREE_PADDING;
			}
			rowc++;
		}
		cout << endl;
		rowc = 0;
		for(int j = 0; j < level[i]->size(); j++) // for every col, i.e. - print this row
		{
			if(level[i]->at(j).col > rowc) // we passed some empty spaces
			{
				cout << TREE_PADDING; // fill out empty node space
				j--;
				rowc++;
				continue;
			}
			else if(level[i]->at(j).sym == 0) // production
			{
				cout << "|" << TREE_PADDING;
			}
			else // symbol
			{
				cout << TREE_PADDING;
			}
			rowc++;
		}
		cout << endl;
		lvl++;
	}
}

const char * getProdName(Production p)
{
	switch(p)
	{
		case(PROGRAM): 		return "PROGRAM";
		case(STMTLIST): 	return "STMTLIST";
		case(STMT): 		return "STMT";
		case(EXPR): 		return "EXPR";
		case(TERM): 		return "TERM";
		case(PRIMARY): 		return "PRIMARY";
		case(PRIM_ID):		return "ID";
		case(PRIM_INT):		return "INT";
		case(PRIM_STR):		return "STRING";
		case(NONE): 		return "NONE";
		default:			return "ERROR";
	}
}

const char * getOpName(Token p)
{
	switch(p)
	{
		case(PLUS):		return " +\t";
		case(STAR):		return " *\t";
		case(MINUS):	return " -\t";
		case(SLASH): 	return " /\t";
		case(SET): 		return " set to ";
		case(PRINT): 	return "PRINT";
		default:		return "ERROR_OP";
	}
}

string stripQuotes(string txt)
{
	if(txt.length() <= 2) return ""; 
	return txt.substr(1, txt.length() - 2);
}

int CountLeaves(PTree * pt)
{
	if(pt == 0) return 0;
	if(pt->sym != 0) return 1 + CountLeaves(pt->left) + CountLeaves(pt->right);
	return CountLeaves(pt->left) + CountLeaves(pt->right);
}

Symbol * Eval_Tree(PTree * pt)
{
	if(pt == 0) return 0;
	
	if(pt->sym != 0) // symbol, can't have children
	{
		if(pt->sym->type == ID && ident.count(pt->sym->txt) != 0) // type ID
		{
			return ident[pt->sym->txt]; // ID exists
		} 
		return pt->sym; // not an ID
	}
		
	else if(pt->prod != NONE) // production
	{
		//cout << "op: " << pt->op << endl;
		if(pt->op == NO_OP) return Eval_Tree(pt->left); // single production, no operation
		else // operation between child productions
		{
			Symbol * op1 = Eval_Tree(pt->left), * op2 = Eval_Tree(pt->right);
			if(pt->op == PRINT)
			{
				if(op1 == 0 || op2 == 0) 
				{
					cout << "ERROR: Bad PRINT expression." << endl;
					error = true;
					return 0;
				}
				string str = op2->txt;
				//if(op2->type == STRING) str = stripQuotes(str);
				cout << str << endl;
			}
			else if(pt->op == SET) // Set ID
			{
				if(op1 == 0 || op2 == 0) 
				{
					cout << "ERROR: Bad SET expression." << endl;
					//cout << "ERROR: SET given bad expression for ID " << Eval_Tree(pt->left)->txt << '.' << endl;
					error = true;
					return 0;
				}
				else ident[op1->txt] = op2;
				return 0;
			}
			else if(pt->op == PLUS) // Addition
			{
				if(op1 == 0 && op2 == 0) return 0;
				if(op1 == 0) return op2;
				if(op2 == 0) return op1;
				Symbol * sym = new Symbol();
				if(op1->type == INT && op2->type == INT)
				{
					int a, b;
					istringstream(op1->txt) >> a;
					istringstream(op2->txt) >> b;
					stringstream ss;
					ss << (a + b);

					sym->type = INT;
					sym->txt = ss.str();
					return sym;
				}
				else if(op1->type == STRING && op2->type == STRING)
				{
					sym->type = STRING;
					sym->txt = "\"" + stripQuotes(op1->txt) + stripQuotes(op2->txt) + "\"";
					return sym;
				}
				else if((op1->type == INT && op2->type == STRING) || (op1->type == STRING && op2->type == INT))
				{
					cout << "UNDEFINED BEHAVIOR: Cannot add INT to STRING." << endl;
					return 0;
				}
				return 0;
			}
			else if(pt->op == MINUS)
			{
				if(op1 == 0 || op2 == 0) return 0;
				/*
				if(op1 == 0) 
				{
					op2->txt = "-" + op2->txt;
					return op2;
				}
				if(op2 == 0) return op1;
				*/
				Symbol * sym = new Symbol();
				if(op1->type == INT && op2->type == INT)
				{
					int a, b;
					istringstream(op1->txt) >> a;
					istringstream(op2->txt) >> b;

					stringstream ss;
					ss << (a - b);

					sym->type = INT;
					sym->txt = ss.str();
					
					return sym;
				}
				else if(op1->type == STRING && op2->type == STRING) // string - string
				{
					if(op2->txt == "\" \"" && (op1->txt).length() < 23) return op1;
					size_t found = op1->txt.find(stripQuotes(op2->txt));
  					if(found == string::npos) return op1;
					sym->txt = op1->txt.erase(found, stripQuotes(op2->txt).length());
					sym->type = STRING;
					return sym;
				}
				return 0;
			}
			else if(pt->op == STAR)
			{
				if(op1 == 0 || op2 == 0) return 0;
				Symbol * sym = new Symbol();
				if(op1->type == INT && op2->type == INT)
				{
					int a, b;
					istringstream(op1->txt) >> a;
					istringstream(op2->txt) >> b;

					stringstream ss;
					ss << (a * b);

					sym->type = INT;
					sym->txt = ss.str();
					
					return sym;
				}
				else if((op1->type == INT && op2->type == STRING) || (op1->type == STRING && op2->type == INT))
				{
					string txt, str;
					int len;
					if(op1->type == INT)
					{
						istringstream(op1->txt) >> len;
						str = stripQuotes(op2->txt);
					}
					else
					{
						istringstream(op2->txt) >> len;
						str = stripQuotes(op1->txt);
					}
					if(len < 0)
					{
						cout << "ERROR: Cannot repeat STRING " << len << " times." << endl;
						error = true;
					}
					for(int i = 0; i < len; i++) txt += str;
					sym->txt = "\"" + txt + "\"";
					sym->type = STRING;
					return sym;
				}
				else if(op1->type == STRING && op2->type == STRING)
				{
					cout << "UNDEFINED BEHAVIOR: Cannot multiply STRING by STRING." << endl;
					return 0;
				}
				return 0;
			}
			else if(pt->op == SLASH)
			{
				if(op1 == 0 || op2 == 0)
				{
					cout << "ERROR: Slash is missing operand." << endl;
					return 0;
				}
				Symbol * sym = new Symbol();
				if(op1->type == INT && op2->type == INT)
				{
					int a, b;
					istringstream(op1->txt) >> a;
					istringstream(op2->txt) >> b;

					stringstream ss;
					ss << (a / b);

					sym->type = INT;
					sym->txt = ss.str();
					
					return sym;
				}
				else if(op1->type == STRING && op2->type == INT)
				{
					sym->type = STRING;
					int len;
					istringstream(op2->txt) >> len;
					if(len < 0)
					{
						cout << "ERROR: Cannot retrieve negative numer of characters. " << endl;
						error = true;
					}
					if(len > stripQuotes(op1->txt).length()) return op1; // requested substr is too long
					sym->txt = "\"" + stripQuotes(op1->txt).substr(0, len) + "\"";
					return sym;
				}
				else if(op1->type == INT && op2->type == STRING)
				{
					sym->type = STRING;
					int len;
					istringstream(op1->txt) >> len;
					if(len < 0)
					{
						cout << "ERROR: Cannot retrieve negative numer of characters. " << endl;
						error = true;
					}
					if(len > stripQuotes(op2->txt).length()) return op2; // requested substr is too long
					string sub = stripQuotes(op2->txt);
					sym->txt = "\"" + sub.substr(sub.length() - len, len) + "\"";
					return sym;
				}
				return 0;
			}
			else
			{
				cout << "ERROR: Parent is not production or a symbol." << endl;
				return 0;
			}
		}
	}
}
/*
 *********************************************
 *  314 Principles of Programming Languages  *
 *********************************************
 */

/* -------------------------------------------------

            CFG for tinyL LANGUAGE

     <program> ::= <stmt_list> !
	<stmt list> ::= <stmt> <morestmts>
	<morestmts> ::= ; <stmt list> | ε
	<stmt> 	::= <assign> | <read> | <print>
	<assign> ::= <variable> = <expr>
	<read> 	::= ? <variable>
	<print> ::= % <variable>
	<expr> ::= 	+ <expr> <expr> |
				− <expr> <expr> |
				∗ <expr> <expr> |
				& <expr> <expr> |
				| <expr> <expr> |
				<variable> |
				<digit>
	<variable> 	::= a | b | c | d | e | f
	<digit> 	::= 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9

     NOTE: tokens are exactly a single character long

     Example expressions:

			a=+2+25;%a!
			a=|2&3|25;%a!


 ---------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "Instr.h"
#include "InstrUtils.h"
#include "Utils.h"

#define MAX_BUFFER_SIZE 500
#define EMPTY_FIELD 0xFFFFF
#define token *buffer

/* GLOBALS */
static char *buffer = NULL;	/* read buffer */
static int regnum = 1;		/* for next free virtual register number */
static FILE *outfile = NULL;	/* output of code generation */

/* Utilities */
static void CodeGen(OpCode opcode, int field1, int field2, int field3);
static inline void next_token();
static inline int next_register();
static inline int is_digit(char c);
static inline int to_digit(char c);
static inline int is_identifier(char c);
static char *read_input(FILE * f);

static inline int variable_to_register(char var);



/* Routines for recursive descending parser LL(1) */
static void program();
static void stmtlist();
static void morestmts();
static void stmt();
static void assign();
static void read();
static void print();
static int expr();
static int variable();
static int digit();

/*************************************************************************/
/* Definitions for recursive descending parser LL(1)                     */
/*************************************************************************/
static int digit()
{
	int reg;

	if (!is_digit(token)) {
		ERROR("Expected digit\n");
		exit(EXIT_FAILURE);
	}
	reg = next_register();
	CodeGen(LOADI, reg, to_digit(token), EMPTY_FIELD);
	next_token();
	return reg;
}

static int variable()
{
	int reg;

	if(!is_identifier(token)){
		ERROR("Expected variable identifier\n");
		exit(EXIT_FAILURE);
	}
	//token is a valid identifier
	reg = next_register(); 
	CodeGen(LOAD, reg, token, EMPTY_FIELD); //tentatively loads the variable value in the next open register

	next_token(); 

	return reg;
}

static int expr()
{
	int reg, left_reg, right_reg;

	switch (token) 
	{
	case '+': //ADD
		next_token();
		left_reg = expr();
		right_reg = expr();
		reg = next_register();
		CodeGen(ADD, reg, left_reg, right_reg);
		return reg;
	case '-': //SUB
        next_token();
        left_reg = expr();
        right_reg = expr();
        reg = next_register();
        CodeGen(SUB, reg, left_reg, right_reg);
        return reg;
    case '*': //MUL
        next_token();
        left_reg = expr();
        right_reg = expr();
        reg = next_register();
        CodeGen(MUL, reg, left_reg, right_reg);
        return reg;
    case '&': //AND
        next_token();
        left_reg = expr();
        right_reg = expr();
        reg = next_register();
        CodeGen(AND, reg, left_reg, right_reg);
        return reg;
    case '|': //ORD
        next_token();
        left_reg = expr();
        right_reg = expr();
        reg = next_register();
        CodeGen(OR, reg, left_reg, right_reg);
        return reg;
	case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        reg = variable();
        return reg;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return digit();
	default:
		ERROR("Symbol %c unknown\n", token);
		exit(EXIT_FAILURE);
	}
}

static void assign()
{
	if (!is_identifier(token)) 
	{
        ERROR("Expected variable at the start of an assign()\n");
        exit(EXIT_FAILURE);
    }

	//get the register for the variable
	//int variable_reg = variable(); //next token called in here

	char variable = token;

	next_token();


	if(token != '=')
	{
		ERROR("Expected '=' after the variable in assign()\n");
        exit(EXIT_FAILURE);
	}

	next_token(); //consume the =

	//actually assign it from here-------------

	int expr_reg = expr();
	//semantic: store it in the variable
	CodeGen(STORE, variable, expr_reg, EMPTY_FIELD);

	//next_token(); //consume the expression, not sure if I need this. correction; DONT NEED IT
}

static void read()
{
	next_token(); //consume the ?

	//int variable_reg = variable();

	char variable = token;
	next_token();

	CodeGen(READ, variable, EMPTY_FIELD, EMPTY_FIELD);


}

static void print()
{
	/* YOUR CODE GOES HERE */
	//printf("\nreached print\n");

	//already identified as %
	next_token(); //consume the %

	//int variable_reg = variable();

	char variable = token;
	next_token();

	CodeGen(WRITE, variable, EMPTY_FIELD, EMPTY_FIELD);
}

static void stmt()
{
	switch(token)
	{
	case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        assign();
        break;
    case '?':
        read();
        break;
    case '%':
        print();
        break;
    default:
        ERROR("Unexpected token %c in stmt()\n", token);
        exit(EXIT_FAILURE);
	}
}

static void morestmts()
{
	if (token == ';') 
	{
        next_token(); //Consume  ';' and move to the next token
        stmtlist();   //Parse the subsequent statement list
		
    } 
	else if (token != '!') 
	{
        ERROR("Unexpected token '%c' encountered. Expected ';' or end of file terminator '!'.\n", token);
        exit(EXIT_FAILURE);
    }
    //If the token is '!' or a valid statement terminator, do nothing.
}

static void stmtlist()
{
	stmt();
	morestmts();
}

static void program()
{
	stmtlist();
	if (token != '!') { //check for EOF
		ERROR("Program error.  Current input symbol is %c\n", token);
		exit(EXIT_FAILURE);
	};
}

/*************************************************************************/
/* Utility definitions                                                   */
/*************************************************************************/
static void CodeGen(OpCode opcode, int field1, int field2, int field3)
{
	Instruction instr;

	if (!outfile) {
		ERROR("File error\n");
		exit(EXIT_FAILURE);
	}
	instr.opcode = opcode;
	instr.field1 = field1;
	instr.field2 = field2;
	instr.field3 = field3;
	PrintInstruction(outfile, &instr);
}

static inline void next_token()
{
	if (*buffer == '\0') {
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}
	printf("%c ", *buffer);
	if (*buffer == ';')
		printf("\n");
	buffer++;
	if (*buffer == '\0') {
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}
	if (*buffer == '.')
		printf(".\n");
}

static inline int next_register()
{
	return regnum++;
}

static inline int is_digit(char c)
{
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

static inline int to_digit(char c)
{
	if (is_digit(c))
		return c - '0';
	WARNING("Non-digit passed to %s, returning zero\n", __func__);
	return 0;
}

//NEW FUNCTION FOR VARIABLE REGISTERS
static inline int variable_to_register(char var) 
{
    //check if identifier is valid
    if (is_identifier(var)) 
	{
		//printf("\n\nYOOOOOOOO THE REGISTER NUMBER IS : %d \n\n", ((var - 'a') + 11));
		return (var - 'a') + 11;
	}
        ERROR("Invalid variable identifier: %c\n Found in function: variable_to_register\n", var);
        exit(EXIT_FAILURE);
    
    //a starts at 11.
    
}

static inline int is_identifier(char c)
{
	if (c >= 'a' && c <= 'f')
		return 1;
	return 0;
}

static char *read_input(FILE * f)
{
	size_t size, i;
	char *b;
	int c;

	for (b = NULL, size = 0, i = 0;;) {
		if (i >= size) {
			size = (size == 0) ? MAX_BUFFER_SIZE : size * 2;
			b = (char *)realloc(b, size * sizeof(char));
			if (!b) {
				ERROR("Realloc failed\n");
				exit(EXIT_FAILURE);
			}
		}
		c = fgetc(f);
		if (EOF == c) {
			b[i] = '\0';
			break;
		}
		if (isspace(c))
			continue;
		b[i] = c;
		i++;
	}
	return b;
}

/*************************************************************************/
/* Main function                                                         */
/*************************************************************************/

int main(int argc, char *argv[])
{
	const char *outfilename = "tinyL.out";
	char *input;
	FILE *infile;

	printf("------------------------------------------------\n");
	printf("CS314 compiler for tinyL\n");
	printf("------------------------------------------------\n");
	if (argc != 2) {
		ERROR("Use of command:\n  compile <tinyL file>\n");
		exit(EXIT_FAILURE);
	}
	infile = fopen(argv[1], "r");
	if (!infile) {
		ERROR("Cannot open input file \"%s\"\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	outfile = fopen(outfilename, "w");
	if (!outfile) {
		ERROR("Cannot open output file \"%s\"\n", outfilename);
		exit(EXIT_FAILURE);
	}
	input = read_input(infile);
	buffer = input;
	program();
	printf("\nCode written to file \"%s\".\n\n", outfilename);
	free(input);
	fclose(infile);
	fclose(outfile);
	return EXIT_SUCCESS;
}

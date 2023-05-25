/***************************************************************
*      scanner routine for Mini C language                    *
***************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "Scanner.h"

extern FILE *sourceFile;                       // miniC source program

int row = 1;
int col = 0; // control line

int superLetter(char ch);
int superLetterOrDigit(char ch);
int getNumber(char firstCharacter, struct tokenType& token);
double getDoubleNumber(char firstCharacter);
int hexValue(char ch);
void lexicalError(int n);
void setnewline(int Row);


// 40 ~ 48�� Ȯ�峻�� �߰� Ű���� ����.
const char *tokenName[] = {
	"!",        "!=",       "%",       "%=",      "%ident",  "%number",
	/* 0          1           2         3          4          5        */
	"&&",       "(",        ")",       "*",       "*=",      "+",
	/* 6          7           8         9         10         11        */
	"++",       "+=",       ",",       "-",       "--",	     "-=",
	/* 12         13         14        15         16         17        */
	"/",        "/=",       ";",       "<",       "<=",      "=",
	/* 18         19         20        21         22         23        */
	"==",       ">",        ">=",      "[",       "]",       "eof",
	/* 24         25         26        27         28         29        */
	//   ...........    word symbols ................................. //
	/* 30         31         32        33         34         35        */
	"const",    "else",     "if",      "int",     "return",  "void",
	/* 36         37         38        39         40         41        */
	"while",    "{",        "||",      "}",       "char",    "double", 
	/* 42         43         44        45         46         47        */
	"for",      "do",       "goto",    "switch",  "case",    "break",  
	/* 48         49         50        51         52*/ 
	"default",  ":",        "%litchar","%litstring","%litdouble"
};

const char *keyword[NO_KEYWORD] = {
	"const",  "else",    "if",    "int",    "return",  "void",    "while",
	"char",   "double",  "for",   "do",     "goto",    "switch",  "case",
	"break",  "default"
};

enum tsymbol tnum[NO_KEYWORD] = {
	tconst,    telse,     tif,     tint,     treturn,   tvoid,     twhile, 
	tchar,     tdouble,   tfor,    tdo,      tgoto,     tswitch,   tcase,
	tbreak,    tdefault
};

struct tokenType scanner()
{
	struct tokenType token;
	int i, index;
	double d;
	char ch, id[ID_LENGTH];

	token.number = tnull;

	do {
		while (isspace(ch = fgetc(sourceFile)));	// state 1: skip blanks
		if (superLetter(ch)) { // identifier or keyword
			i = 0;
			do {
				if (i < ID_LENGTH) id[i++] = ch;
				ch = fgetc(sourceFile);
			} while (superLetterOrDigit(ch));
			if (i >= ID_LENGTH) lexicalError(1);
			id[i] = '\0';
			ungetc(ch, sourceFile);  //  retract
									 // find the identifier in the keyword table
			for (index = 0; index < NO_KEYWORD; index++)
				if (!strcmp(id, keyword[index])) break;
			if (index < NO_KEYWORD)    // found, keyword exit
				token.number = tnum[index];
			else {                     // not found, identifier exit
				token.number = tident;
				strcpy_s(token.value.id, id);
			}
		}  // end of identifier or keyword
		else if (isdigit(ch)) {  // number
			token.number = tnumber;
			token.value.num = getNumber(ch, token);
		}
		else { // special character
			token.linenumber = row;
			token.col = col; 

			switch (ch) {
			case '.': // check double
				i = 10; // int
				d = 0.0; // double
				
				while (true) {
					ch = fgetc(sourceFile);
					col++;
					
					if (!isdigit(ch)) break; // check digit
					
					int num = ch - '0';
					d += ((double)num / (double)i);
					i *= 10;
				}
				token.number = tlitdouble; 
				token.value.dnum = d; 
				ungetc(ch, sourceFile);
				col--;
				break;
			case '\'': // check character
				ch = fgetc(sourceFile);
				col++; 
				
				if (ch != '\\' && ch != '\'') { 
					token.value.id[0] = ch;
				}
				else if (ch == '\\') { 
					token.value.id[0] = ch;
					ch = fgetc(sourceFile);
					col++;
					token.value.id[1] = ch;
				}

				ch = fgetc(sourceFile);
				col++;

				if (ch == '\'') {
					token.number = tlitchar;
					break;
				}
				else {
					lexicalError(6);
					ungetc(ch, sourceFile);
					col--;
					break;
				}
			case '"': // check string
				for (i = 0; i < ID_LENGTH - 1; i++) {
					ch = fgetc(sourceFile);
					col++;
					if (ch == '"') {
						if (i > 0 && token.value.id[i - 1] == '\\') {
							token.value.id[i - 1] = '"';
							i--;
							continue;
						}
						else break;
					}
					token.value.id[i] = ch;
				}
				token.value.id[i] = '\0';

				if (i == ID_LENGTH - 1) {
					ch = fgetc(sourceFile);
					col++;
					if (ch == '"') {
						token.number = tlitstring;
					}
					else {
						lexicalError(7);
						ungetc(ch, sourceFile);
						col--;
					}
				}
				else { 
					token.number = tlitstring;
				}
				break;
			case '/':
				ch = fgetc(sourceFile);
				col++;

				if (ch == '*') {			// text comment
					ch = fgetc(sourceFile);
					if (ch == '*') { // if Documented Comments ------> (/** ~ */) 
						printf("Documented Comments ------> ");
						while (true) {
							ch = fgetc(sourceFile);
							if (ch == '\n') row++;
							if (ch == '*') {
								ch = fgetc(sourceFile);
								if (ch == '/') break; // end Documented Comments
								else printf("*");
							}
							printf("%c", ch);
						}
						printf("\n");
					}
					else {
						ungetc(ch, sourceFile);
						do {
							while (ch != '*') ch = fgetc(sourceFile);
							ch = fgetc(sourceFile);
						} while (ch != '/');
					}
				}
				else if (ch == '/') {	// line comment
					setnewline(row + 1);
					ch = fgetc(sourceFile);
					if (ch == '/') { // if Documented Comments ------> (//)
						printf("Documented Comments ------> ");
						while (true) {
							ch = fgetc(sourceFile);
							if (ch == '\n') break;
							else printf("%c", ch);
						}
						printf("\n");
					}
					else while (fgetc(sourceFile) != '\n');
				} 

				else if (ch == '=')  token.number = tdivAssign;
				else {
					token.number = tdiv;
					ungetc(ch, sourceFile); // retract
				}
				break;

			case '!':
				ch = fgetc(sourceFile);
				if (ch == '=')  token.number = tnotequ;
				else {
					token.number = tnot;
					ungetc(ch, sourceFile); // retract
				}
				break;
			case '%':
				ch = fgetc(sourceFile);
				if (ch == '=') {
					token.number = tremAssign;
				}
				else {
					token.number = tremainder;
					ungetc(ch, sourceFile);
				}
				break;
			case '&':
				ch = fgetc(sourceFile);
				if (ch == '&')  token.number = tand;
				else {
					lexicalError(2);
					ungetc(ch, sourceFile);  // retract
				}
				break;
			case '*':
				ch = fgetc(sourceFile);
				if (ch == '=')  token.number = tmulAssign;
				else {
					token.number = tmul;
					ungetc(ch, sourceFile);  // retract
				}
				break;
			case '+':
				ch = fgetc(sourceFile);
				if (ch == '+')  token.number = tinc;
				else if (ch == '=') token.number = taddAssign;
				else {
					token.number = tplus;
					ungetc(ch, sourceFile);  // retract
				}
				break;
			case '-':
				ch = fgetc(sourceFile);
				if (ch == '-')  token.number = tdec;
				else if (ch == '=') token.number = tsubAssign;
				else {
					token.number = tminus;
					ungetc(ch, sourceFile);  // retract
				}
				break;
			case '<':
				ch = fgetc(sourceFile);
				if (ch == '=') token.number = tlesse;
				else {
					token.number = tless;
					ungetc(ch, sourceFile);  // retract
				}
				break;
			case '=':
				ch = fgetc(sourceFile);
				if (ch == '=')  token.number = tequal;
				else {
					token.number = tassign;
					ungetc(ch, sourceFile);  // retract
				}
				break;
			case '>':
				ch = fgetc(sourceFile);
				if (ch == '=') token.number = tgreate;
				else {
					token.number = tgreat;
					ungetc(ch, sourceFile);  // retract
				}
				break;
			case '|':
				ch = fgetc(sourceFile);
				if (ch == '|')  token.number = tor;
				else {
					lexicalError(3);
					ungetc(ch, sourceFile);  // retract
				}
				break;
			case '(': token.number = tlparen;         break;
			case ')': token.number = trparen;         break;
			case ',': token.number = tcomma;          break;
			case ';': token.number = tsemicolon;      break;
			case '[': token.number = tlbracket;       break;
			case ']': token.number = trbracket;       break;
			case '{': token.number = tlbrace;         break;
			case '}': token.number = trbrace;         break;
			case EOF: token.number = teof;            break;
			default: {
				printf("Current character : %c", ch);
				lexicalError(4);
				break;
			}
		    } // switch end
		} 
	} while (token.number == tnull);
	return token;
} // end of scanner

void lexicalError(int n)
{
	printf(" *** Lexical Error : ");
	switch (n) {
	case 1: printf("an identifier length must be less than 12.\n");
		break;
	case 2: printf("next character must be &\n");
		break;
	case 3: printf("next character must be |\n");
		break;
	case 4: printf("invalid character\n");
		break;
	case 6: printf("please end character with \'\n");
		break;
	case 7: printf("please input string length less than 12\n");
		break;
	}
}

int superLetter(char ch)
{
	if (isalpha(ch) || ch == '_') return 1;
	else return 0;
}

int superLetterOrDigit(char ch)
{
	if (isalnum(ch) || ch == '_') return 1;
	else return 0;
}

int getNumber(char firstCharacter, struct tokenType& token)
{
	int num = 0;
	int value;
	char ch;

	if (firstCharacter == '0') {
		ch = fgetc(sourceFile);
		col++;
		if ((ch == 'X') || (ch == 'x')) {		// hexa decimal
			while ((value = hexValue(ch = fgetc(sourceFile))) != -1)
				col++;
				num = 16 * num + value;
		}
		else if ((ch >= '0') && (ch <= '7'))	// octal
			do {
				num = 8 * num + (int)(ch - '0');
				ch = fgetc(sourceFile);
				col++;
			} while ((ch >= '0') && (ch <= '7'));
		else num = 0;						// zero
	}
	else {									// decimal
		ch = firstCharacter;
		do {
			num = 10 * num + (int)(ch - '0');
			ch = fgetc(sourceFile);
			col++;
			if (ch == '.') {
				token.number = tlitdouble;
			}
		} while (isdigit(ch));
	}
	ungetc(ch, sourceFile);  /*  retract  */
	col--;
	return num;
}

int hexValue(char ch)
{
	switch (ch) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return (ch - '0');
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		return (ch - 'A' + 10);
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		return (ch - 'a' + 10);
	default: return -1;
	}
}

double getDoubleNumber(char firstCharacter) {
	int i = 10;
	int num; 
	double DoubleNumber = 0;
	char ch = firstCharacter;
	
	do {
		num = (ch - '0'); 
		DoubleNumber += (double)num / (double)i;
		ch = fgetc(sourceFile);
		col++;
		i *= 10;
	} while (isdigit(ch));
	ungetc(ch, sourceFile);

	return DoubleNumber;
}

void printToken(struct tokenType token, char* filename)
{
	if (token.number == tident)
		printf("%s (%d, %s, %s, %d, %d)\n", tokenName[token.number], token.number, token.value.id, filename, token.linenumber, token.col);
	else if (token.number == tnumber)
		printf("%s (%d, %d, %s, %d, %d)\n", tokenName[token.number], token.number, token.value.num, filename, token.linenumber, token.col);
	else if (token.number == tlitchar) {
		if (token.value.id[0] != '\\')
			printf("%s (%d, %c%c, %s, %d, %d)\n", tokenName[token.number], token.number, token.value.id[0], token.value.id[1], filename, token.linenumber, token.col);
		else
			printf("%s (%d, %c, %s, %d, %d)\n", tokenName[token.number], token.number, token.value.id[0], filename, token.linenumber, token.col);
	}
	else if (token.number == tlitstring)
		printf("%s (%d, %s, %s, %d, %d)\n", tokenName[token.number], token.number, token.value.id, filename, token.linenumber, token.col);
	else if (token.number == tlitdouble)
		printf("%s (%d, %f, %s, %d, %d)\n", tokenName[token.number], token.number, token.value.dnum, filename, token.linenumber, token.col);
	else
		printf("%s (%d, (no value) , %s, %d, %d)\n", tokenName[token.number], token.number, filename, token.linenumber, token.col);
}

void setnewline(int Row) {
	row = Row; 
	col = 0;
}
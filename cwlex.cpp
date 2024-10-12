#define _CRT_SECURE_NO_WARNINGS  // for using sscanf in VS
/***************************************************************
* N.Kozak // Lviv'2018 // example lexical analysis for pkt4 SP *
*                         file: cwlex.cpp                      *
*                                                              *
****************************************************************/
//#define USE_PREDEFINED_PARAMETERS // enable this define for use predefined value

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define MAX_TEXT_SIZE 8192
#define MAX_WORD_COUNT (MAX_TEXT_SIZE / 5)
#define MAX_LEXEM_SIZE 1024
#define MAX_STRONG_RESERVED_LEXEMES_COUNT 64
#define MAX_RESERVED_LEXEMES_COUNT 64
#define MAX_COMMENT_LEXEMES_COUNT 2

#define STRONG_RESERVED_LEXEME_TYPE 1
#define RESERVED_LEXEME_TYPE 2
#define IDENTIFIER_LEXEME_TYPE 3
#define VALUE_LEXEME_TYPE 4
#define UNEXPEXTED_LEXEME_TYPE 127

#define LEXICAL_ANALISIS_MODE 1
#define SEMANTIC_ANALISIS_MODE 2
#define FULL_COMPILER_MODE 4

#define DEBUG_MODE 512

#define MAX_PARAMETERS_SIZE 4096
#define PARAMETERS_COUNT 4
#define INPUT_FILENAME_PARAMETER 0

#define DEFAULT_MODE (LEXICAL_ANALISIS_MODE | DEBUG_MODE)

#define DEFAULT_INPUT_FILENAME "file1.cwl"

#define PREDEFINED_TEXT \
	"name MN\r\n" \
	"data\r\n" \
	"    #*argumentValue*#\r\n" \
	"    long int AV\r\n" \
	"    #*resultValue*#\r\n" \
	"    long int RV\r\n" \
	";\r\n" \
	"\r\n" \
	"body\r\n" \
	"    RV << 1; #*resultValue = 1; *#\r\n" \
	"\r\n" \
	"    #*input*#\r\n" \
	"	 get AV; #*scanf(\"%d\", &argumentValue); *#\r\n" \
	"\r\n" \
	"    #*compute*#\r\n" \
	"	 CL: #*label for cycle*#\r\n" \
	"    if AV == 0 goto EL #*for (; argumentValue; --argumentValue)*#\r\n" \
	"        RV << RV ** AV; #*resultValue *= argumentValue; *#\r\n" \
	"        AV << AV -- 1; \r\n" \
	"    goto CL\r\n" \
	"    EL: #*label for end cycle*#\r\n" \
	"\r\n" \
	"    #*output*#\r\n" \
	"    put RV; #*printf(\"%d\", resultValue); *#\r\n" \
	"end" \

unsigned int mode;
char parameters[PARAMETERS_COUNT][MAX_PARAMETERS_SIZE] = { "" };

struct LexemInfo{
	char lexemStr[MAX_LEXEM_SIZE];
	unsigned int lexemId;
	unsigned int tokenType;
	unsigned int ifvalue;
	unsigned int row;
	unsigned int col;
	// TODO: ...
};

struct LexemInfo lexemesInfoTable[MAX_WORD_COUNT] = { { "", 0, 0, 0 } };
struct LexemInfo * lastLexemInfoInTable = lexemesInfoTable; // first for begin

char identifierIdsTable[MAX_WORD_COUNT][MAX_LEXEM_SIZE] = { "" };

char strongReservedLexemes[MAX_STRONG_RESERVED_LEXEMES_COUNT + 1][MAX_LEXEM_SIZE] = {
	";",                               //(1)
	                                   //(2)
	"<<",                              //(3)
	                                   //(4)
	                                   //(5)
	                                   //(6)
	"++", "--", "**",                  //(7)
	"==", "!=",                        //(8)
	                                   //(9)
	                                   //(10)
	                                   //(11)
    ":",                               //(12(+))
	"'"
};

char reservedLexemes[MAX_RESERVED_LEXEMES_COUNT + 1][MAX_LEXEM_SIZE] = {
	"name", "data", "body", "end",     //(1)
	"get", "put",                      //(2)
	                                   //(3)
	"if", "goto",                      //(4)
	                                   //(5)
	                                   //(6)
	"div", "mod",                      //(7)
	"le", "ge"                         //(8)
	"not", "and", "or",                //(9)
	"long", "int",                     //(10)
	                                   //(11)
	"'"
};

char reservedLexemesForCommentSpace[MAX_COMMENT_LEXEMES_COUNT + 1][MAX_LEXEM_SIZE] = {
	                                   //(1)
	                                   //(2)
	                                   //(3)
	                                   //(4)
	                                   //(5)
	                                   //(6)
	                                   //(7)
	                                   //(8)
	                                   //(9)
	                                   //(10)
	"#*", "*#",                        //(11)
	"'"
};

char identifierCasePattern[] = "UU";

void printLexemes(struct LexemInfo * lexemInfoTable, char printBadLexeme = 0){
	if (printBadLexeme){
		printf("Bad lexeme:\r\n");
	}
	else{
		printf("Lexemes table:\r\n");
	}
	printf("-------------------------------------------------------------------\r\n");
	printf("index\t\tlexeme\t\tid\ttype\tifvalue\trow\tcol\r\n");
	printf("-------------------------------------------------------------------\r\n");
	for (unsigned int index = 0; (!index || !printBadLexeme) && lexemInfoTable[index].lexemStr[0] != '\0'; ++index){
		printf("%5d%17s%12d%10d%11d%4d%8d\r\n", index, lexemInfoTable[index].lexemStr, lexemInfoTable[index].lexemId, lexemInfoTable[index].tokenType, lexemInfoTable[index].ifvalue, lexemInfoTable[index].row, lexemInfoTable[index].col);
	}
	printf("-------------------------------------------------------------------\r\n\r\n");

	return;
}

// try to get strong reserved lexeme
unsigned int tryToGetStrongReservedLexeme(struct LexemInfo ** lastLexemInfoInTable, char * str, char (*strongReservedLexemes)[MAX_LEXEM_SIZE], unsigned int row, unsigned int col){
	for (unsigned int index = 0; strongReservedLexemes[index][0] != '\''; ++index){
		unsigned int currLexemeLength = strlen(strongReservedLexemes[index]);
		if (!strncmp(str, strongReservedLexemes[index], currLexemeLength) && currLexemeLength <= strlen(str)){
			strncpy((*lastLexemInfoInTable)->lexemStr, strongReservedLexemes[index], MAX_LEXEM_SIZE);
			(*lastLexemInfoInTable)->row = row;				
			(*lastLexemInfoInTable)->col = col;
			(*lastLexemInfoInTable)->lexemId = index;
			(*lastLexemInfoInTable)->tokenType = STRONG_RESERVED_LEXEME_TYPE;
			++(*lastLexemInfoInTable);
			return currLexemeLength;
		}
	}
	return 0;
}

// try to get reserved lexeme
unsigned int tryToGetReservedLexeme(struct LexemInfo ** lastLexemInfoInTable, char * str, char (*reservedLexemes)[MAX_LEXEM_SIZE], char (*strongReservedLexemes)[MAX_LEXEM_SIZE], unsigned int row, unsigned int col){
	unsigned int currSize = strlen(str);
	for (unsigned int index = 0; reservedLexemes[index][0] != '\''; ++index){
		unsigned int currLexemeLength = strlen(reservedLexemes[index]);
		if (currLexemeLength && !strncmp(str, reservedLexemes[index], currLexemeLength)){
			if (currSize == currLexemeLength) {
				strncpy((*lastLexemInfoInTable)->lexemStr, reservedLexemes[index], MAX_LEXEM_SIZE);
				(*lastLexemInfoInTable)->row = row;
				(*lastLexemInfoInTable)->col = col;
				(*lastLexemInfoInTable)->lexemId = MAX_RESERVED_LEXEMES_COUNT + index;
				(*lastLexemInfoInTable)->tokenType = RESERVED_LEXEME_TYPE;
				++(*lastLexemInfoInTable);
				return currLexemeLength;
			}
			else if (currSize > currLexemeLength){
				unsigned int strongLexemeLength = tryToGetStrongReservedLexeme(lastLexemInfoInTable, str + currLexemeLength, strongReservedLexemes, row, col);
				if (strongLexemeLength){
					strncpy((*lastLexemInfoInTable)->lexemStr, reservedLexemes[index], currLexemeLength);
					(*lastLexemInfoInTable)->lexemStr[currLexemeLength] = '\0';
					(*lastLexemInfoInTable)->row = row;
					(*lastLexemInfoInTable)->col = col;
					(*lastLexemInfoInTable)->lexemId = MAX_RESERVED_LEXEMES_COUNT + index;
					(*lastLexemInfoInTable)->tokenType = RESERVED_LEXEME_TYPE;
					++(*lastLexemInfoInTable);
					return currLexemeLength;
				}
			}
		}
	}

	return 0;
}

// get identifier id
unsigned int getIdentifierId(char(*identifierIdsTable)[MAX_LEXEM_SIZE], char * str){
	unsigned int index = 0;
	for (; identifierIdsTable[index][0] != '\0'; ++index){
		if (!strncmp(identifierIdsTable[index], str, MAX_LEXEM_SIZE)){
			return index;
		}
	}
	strncpy(identifierIdsTable[index], str, MAX_LEXEM_SIZE);
	identifierIdsTable[index + 1][0] = '\0'; // not necessarily for zero-init identifierIdsTable
	return index;
}

// try to get identifier
unsigned int tryToGetIdentifier(struct LexemInfo ** lastLexemInfoInTable, char * str, char (*strongReservedLexemes)[MAX_LEXEM_SIZE], char * identifierCasePattern, char(*identifierIdsTable)[MAX_LEXEM_SIZE], unsigned int row, unsigned int col){
	unsigned int currSize = strlen(str);
	unsigned int currLexemeLength = 0;
	for (currLexemeLength = 0; currLexemeLength < currSize && (
		   (identifierCasePattern[currLexemeLength] == 'L' && str[currLexemeLength] >= 'a' && str[currLexemeLength] <= 'z')
		|| (identifierCasePattern[currLexemeLength] == 'U' && str[currLexemeLength] >= 'A' && str[currLexemeLength] <= 'Z')
		|| (identifierCasePattern[currLexemeLength] == '_' && str[currLexemeLength] == '_')
		);
	++currLexemeLength);

	if (currLexemeLength < strlen(identifierCasePattern)){
		return 0;
	}

	if (currSize == currLexemeLength) {
		strncpy((*lastLexemInfoInTable)->lexemStr, str, MAX_LEXEM_SIZE);
		(*lastLexemInfoInTable)->row = row;
		(*lastLexemInfoInTable)->col = col;
		(*lastLexemInfoInTable)->lexemId = MAX_STRONG_RESERVED_LEXEMES_COUNT + MAX_RESERVED_LEXEMES_COUNT + getIdentifierId(identifierIdsTable, str);
		(*lastLexemInfoInTable)->tokenType = IDENTIFIER_LEXEME_TYPE;
		++(*lastLexemInfoInTable);
		return currLexemeLength;
	}
	else {
		unsigned int strongLexemeLength = tryToGetStrongReservedLexeme(lastLexemInfoInTable, str + currLexemeLength, strongReservedLexemes, row, col);
		if (strongLexemeLength){
			strncpy((*lastLexemInfoInTable)->lexemStr, str, currLexemeLength);
			(*lastLexemInfoInTable)->lexemStr[currLexemeLength] = '\0';
			(*lastLexemInfoInTable)->row = row;
			(*lastLexemInfoInTable)->col = col;
			(*lastLexemInfoInTable)->lexemId = MAX_STRONG_RESERVED_LEXEMES_COUNT + MAX_RESERVED_LEXEMES_COUNT + getIdentifierId(identifierIdsTable, str);
			(*lastLexemInfoInTable)->tokenType = IDENTIFIER_LEXEME_TYPE;
			++(*lastLexemInfoInTable);
			return currLexemeLength;
		}
	}

	return 0;
}


// try to get value
unsigned int tryToGetValue(struct LexemInfo ** lastLexemInfoInTable, char * str, char (*strongReservedLexemes)[MAX_LEXEM_SIZE], unsigned int row, unsigned int col){
	unsigned int currSize = strlen(str);
	unsigned int currLexemeLength = 0;
	for (currLexemeLength = 0; currLexemeLength < currSize && (!currLexemeLength && (str[currLexemeLength] == '+' || str[currLexemeLength] == '-') || str[currLexemeLength] >= '0' && str[currLexemeLength] <= '9'); ++currLexemeLength);
	
	if (!currLexemeLength || currLexemeLength == 1 && (str[0] == '+' || str[0] == '-')){
		return 0;
	}

	if (currSize == currLexemeLength) {
		strncpy((*lastLexemInfoInTable)->lexemStr, str, MAX_LEXEM_SIZE);
		(*lastLexemInfoInTable)->row = row;
		(*lastLexemInfoInTable)->col = col;
		(*lastLexemInfoInTable)->ifvalue = atoi(str);
		(*lastLexemInfoInTable)->lexemId = MAX_STRONG_RESERVED_LEXEMES_COUNT + MAX_RESERVED_LEXEMES_COUNT + MAX_WORD_COUNT;
		(*lastLexemInfoInTable)->tokenType = VALUE_LEXEME_TYPE;
		++(*lastLexemInfoInTable);
		return currLexemeLength;
	}
	else {
		unsigned int strongLexemeLength = tryToGetStrongReservedLexeme(lastLexemInfoInTable, str + currLexemeLength, strongReservedLexemes, row, col);
		if (strongLexemeLength){
			strncpy((*lastLexemInfoInTable)->lexemStr, str, currLexemeLength);
			(*lastLexemInfoInTable)->lexemStr[currLexemeLength] = '\0';
			(*lastLexemInfoInTable)->row = row;
			(*lastLexemInfoInTable)->col = col;
			(*lastLexemInfoInTable)->ifvalue = atoi(str);
			(*lastLexemInfoInTable)->lexemId = MAX_STRONG_RESERVED_LEXEMES_COUNT + MAX_RESERVED_LEXEMES_COUNT + MAX_WORD_COUNT;
			(*lastLexemInfoInTable)->tokenType = VALUE_LEXEME_TYPE;
			++(*lastLexemInfoInTable);
			return currLexemeLength;
		}
	}

	return 0;
}

struct LexemInfo runLexicalAnalysis(struct LexemInfo ** lastLexemInfoInTable, char * text, char (*strongReservedLexemes)[MAX_LEXEM_SIZE], char (*reservedLexemes)[MAX_LEXEM_SIZE], char * identifierCasePattern, char(*identifierIdsTable)[MAX_LEXEM_SIZE]){
	struct LexemInfo ifBadLexemeInfo = { 0 };
	char * const endOfText = text + strlen(text);
	for (unsigned int addonSize = 0, row = 1, col = 1; text < endOfText; addonSize = 0){
		char part[MAX_LEXEM_SIZE] = { 0 };
		int ifNewEndLine = sscanf(text, "%*[' ''\t''\r']%['\n']", part);
		if (ifNewEndLine > 0) ++row;
		while (ifNewEndLine && ifNewEndLine != EOF){
			text = strstr(text, "\n") + 1;
			col = 1;
			ifNewEndLine = sscanf(text, "%*[' ''\t''\r']%['\n']", part);
			if (ifNewEndLine > 0) ++row;
		}
		int currScan = sscanf(text, "%s", part);
		if (!currScan || currScan == EOF){
			break;
		}
		unsigned int offsetToNewSubStr = strstr(text, part) - text;
		text += offsetToNewSubStr;
		col += offsetToNewSubStr;
	
		// try to get strong reserved lexeme
		if (!addonSize) {
			addonSize = tryToGetStrongReservedLexeme(lastLexemInfoInTable, part, strongReservedLexemes, row, col);
		}

		// try to get reserved lexeme
		if (!addonSize) {
			addonSize = tryToGetReservedLexeme(lastLexemInfoInTable, part, reservedLexemes, strongReservedLexemes, row, col);
		}

		// try to get identifier
		if (!addonSize) {
			addonSize = tryToGetIdentifier(lastLexemInfoInTable, part, strongReservedLexemes, identifierCasePattern, identifierIdsTable, row, col);
		}

		// try to get value
		if (!addonSize) {
			addonSize = tryToGetValue(lastLexemInfoInTable, part, strongReservedLexemes, row, col);
		}

		if (addonSize) {
			text += addonSize;
			col += addonSize;
		}
		else{
			strncpy(ifBadLexemeInfo.lexemStr, part, MAX_LEXEM_SIZE);
			ifBadLexemeInfo.row = row;
			ifBadLexemeInfo.col = col;
			ifBadLexemeInfo.tokenType = UNEXPEXTED_LEXEME_TYPE;			
			return ifBadLexemeInfo;
		}	
	}

	return ifBadLexemeInfo;
}


int commentRemover(char * text = (char*)"", char * openStrSpc = (char*)"//", char * closeStrSpc = (char*)"\n", bool eofAlternativeCloseStrSpcType = true, bool explicitCloseStrSpc = false){
	unsigned int commentSpace = 0;

	unsigned int textLength = strlen(text);               // strnlen(text, MAX_TEXT_SIZE);
	unsigned int openStrSpcLength = strlen(openStrSpc);   // strnlen(openStrSpc, MAX_TEXT_SIZE);
	unsigned int closeStrSpcLength = strlen(closeStrSpc); // strnlen(closeStrSpc, MAX_TEXT_SIZE);
	if (!closeStrSpcLength){
		return -1; // no set closeStrSpc
	}
	unsigned char oneLevelComment = 0;
	if (!strncmp(openStrSpc, closeStrSpc, MAX_LEXEM_SIZE)){
		oneLevelComment = 1;
	}

	for (unsigned int index = 0; index < textLength; ++index){
		if (!strncmp(text + index, closeStrSpc, closeStrSpcLength) && (explicitCloseStrSpc || commentSpace)) {
			if (commentSpace == 1 && explicitCloseStrSpc){
				for (unsigned int index2 = 0; index2 < closeStrSpcLength; ++index2){
					text[index + index2] = ' ';
				}
			}
			else if (commentSpace == 1 && !explicitCloseStrSpc){
				index += closeStrSpcLength - 1;
			}
			oneLevelComment ? commentSpace = !commentSpace : commentSpace = 0;
		}
		else if (!strncmp(text + index, openStrSpc, openStrSpcLength)) {
			oneLevelComment ? commentSpace = !commentSpace : commentSpace = 1;
		}

		if (commentSpace && text[index] != ' ' && text[index] != '\t' && text[index] != '\r' && text[index] != '\n'){
			text[index] = ' ';
		}

	}

	if (commentSpace && !eofAlternativeCloseStrSpcType){
		return -1;
	}

	return 0;
}


void comandLineParser(int argc, char* argv[], unsigned int * mode, char (* parameters)[MAX_PARAMETERS_SIZE]){
	char useDefaultModes = 1;
	*mode = 0;
	for (int index = 1; index < argc; ++index){
		if (!strcmp(argv[index], "-lex")){
			*mode |= LEXICAL_ANALISIS_MODE;
			useDefaultModes = 0;
			continue;
		}
		else if (!strcmp(argv[index], "-d")){
			*mode |= DEBUG_MODE;
			useDefaultModes = 0;
			continue;
		}
		
		// other keys
		// TODO:...

		// input file name
		strncpy(parameters[INPUT_FILENAME_PARAMETER], argv[index], MAX_PARAMETERS_SIZE);
	}

	// default input file name,  if not entered manually
	if (parameters[INPUT_FILENAME_PARAMETER][0] == '\0'){
		strcpy(parameters[INPUT_FILENAME_PARAMETER], DEFAULT_INPUT_FILENAME);
		printf("Input file name not setted. Used default input file name \"file1.cwl\"\r\n\r\n");
	}

	// default mode,  if not entered manually
	if (useDefaultModes){
		*mode = DEFAULT_MODE;
		printf("Used default mode\r\n\r\n");
	}

	return;
}

// after using this function use free(void *) function to release text buffer
size_t loadSource(char ** text, char * fileName){
	if (!fileName){
		printf("No input file name\r\n");
		return 0;
	}

	FILE * file = fopen(fileName, "rb");

	if (file == NULL){
		printf("File not loaded\r\n");
		return 0;
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	rewind(file);

	*text = (char*)malloc(sizeof(char) * (fileSize + 1));
	if (text == NULL) { 
		fputs("Memory error", stderr); exit(2); // TODO: ...
	}

	size_t result = fread(*text, sizeof(char), fileSize, file);
	if (result != fileSize) {
		fputs("Reading error", stderr);
		exit(3); // TODO: ...
	}
	(*text)[fileSize] = '\0';

	fclose(file);
	
	return fileSize;
}

int main(int argc, char* argv[]){

#ifdef	USE_PREDEFINED_PARAMETERS
	mode = DEFAULT_MODE;
	char text[MAX_TEXT_SIZE] = PREDEFINED_TEXT;
#else
	comandLineParser(argc, argv, &mode, parameters);
	char * text;
	size_t sourceSize = loadSource(&text, parameters[INPUT_FILENAME_PARAMETER]);
	if (!sourceSize){
		printf("Press Enter to exit . . .");
		getchar();
		return 0;
	}
#endif

	if (!(mode & LEXICAL_ANALISIS_MODE)){
		printf("NO SUPORTED MODE ...\r\n");
		printf("Press Enter to exit . . .");
		getchar();
		return 0;
	}

	if (mode & DEBUG_MODE){
		printf("Original source:\r\n");
		printf("-------------------------------------------------------------------\r\n");
		printf("%s\r\n", text);
		printf("-------------------------------------------------------------------\r\n\r\n");
	}

	bool eofAlternativeCloseStrSpcType = false;
	bool explicitCloseStrSpc = true;
	if (!strcmp(reservedLexemesForCommentSpace[1], "\n")){
		eofAlternativeCloseStrSpcType = true;
		explicitCloseStrSpc = false;
	}
	int commentRemoverResult = commentRemover(text, reservedLexemesForCommentSpace[0], reservedLexemesForCommentSpace[1], eofAlternativeCloseStrSpcType, explicitCloseStrSpc);
	if (commentRemoverResult){
		printf("Comment remover return %d\r\n", commentRemoverResult);
		printf("Press Enter to exit . . .");
		getchar();
		return 0;
	}
	if (mode & DEBUG_MODE){
		printf("Source after comment removing:\r\n");
		printf("-------------------------------------------------------------------\r\n");
		printf("%s\r\n", text);
		printf("-------------------------------------------------------------------\r\n\r\n");
	}

	struct LexemInfo ifBadLexemeInfo = runLexicalAnalysis(&lastLexemInfoInTable, text, strongReservedLexemes, reservedLexemes, identifierCasePattern, identifierIdsTable);
	if (ifBadLexemeInfo.tokenType == UNEXPEXTED_LEXEME_TYPE){
		UNEXPEXTED_LEXEME_TYPE;
		ifBadLexemeInfo.tokenType;
		printf("Lexical analysis detected unexpected lexeme\r\n");
		printLexemes(&ifBadLexemeInfo, 1);
		printf("Press Enter to exit . . .");
		getchar();
		return 0;
	}
	if (mode & DEBUG_MODE){
		printLexemes(lexemesInfoTable);
	}
	else{
		printf("Lexical analysis complete success\r\n");
	}

	printf("Press Enter to exit . . .");
	getchar();

#ifndef	USE_PREDEFINED_PARAMETERS
	free(text);
#endif

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


// for lex
#define MAXLEN 256

// Token types
// 新的符號補進來！
typedef enum {
    UNKNOWN, END, ENDFILE,
    INT, ID,
    ADDSUB, MULDIV,
    ASSIGN,
    LPAREN, RPAREN,
    INCDEC, ADDSUB_ASSIGN,
    AND, OR, XOR, 
} TokenSet;

TokenSet getToken(void);
TokenSet curToken = UNKNOWN;
char lexeme[MAXLEN];

// Test if a token matches the current token
int match(TokenSet token);
// Get the next token
void advance(void);
// Get the lexeme of the current token
char *getLexeme(void);


// for parser
#define TBLSIZE 64
// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 1

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum) { \
    if (PRINTERR) \
        fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
    err(errorNum); \
}

// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR
} ErrorType;

// Structure of the symbol table
typedef struct {
    int val;
    char name[MAXLEN];
} Symbol;

// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    int val;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
} BTNode;

int sbcount = 0;
Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
void initTable(void);
// Get the value of a variable
int getval(char *str);
// Set the value of a variable
int setval(char *str, int val);
// Make a new node according to token type and lexeme
BTNode *makeNode(TokenSet tok, const char *lexe);
// Free the syntax tree
void freeTree(BTNode *root);
BTNode *factor(void);
BTNode *unary_expr(void);
BTNode *muldiv_expr(void);
BTNode *muldiv_expr_tail(BTNode *left);
BTNode *addsub_expr(void);
BTNode *addsub_expr_tail(BTNode *left);
BTNode *and_expr(void);
BTNode *and_expr_tail(BTNode *left);
BTNode *xor_expr(void);
BTNode *xor_expr_tail(BTNode *left);
BTNode *or_expr(void);
BTNode *or_expr_tail(BTNode *left);
BTNode *assign_expr(void);
BTNode *statement(void);
// Print error message and exit the program
void err(ErrorType errorNum);
BTNode *foldConstant(BTNode *root);

// for codeGen
// Evaluate the syntax tree
int calculateConstant(BTNode *root);
int evaluateTree(BTNode *root);
int get_reg(void);
void free_reg(int index);
// Print the syntax tree in prefix
void printPrefix(BTNode *root);
void makeUseful(BTNode *root);
int getindex(BTNode *tar);
void syntaxCheck(BTNode *root);

/*============================================================================================
lex implementation
============================================================================================*/

int isVarName (char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_');
}

TokenSet getToken(void) {
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t');

    if (isdigit(c)) { // caseA，這是一個數字開頭的東西
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        if (isVarName(c)){
            error(NOTLVAL);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    } else if (c == '+' || c == '-') { // 誒並不是讀到一個 + 或 - 就能亂判斷誒
        lexeme[0] = c;
        c = fgetc(stdin);
        if (c == lexeme[0]){ // ++ or --
            lexeme[1] = c;
            lexeme[2] = '\0';
            return INCDEC;            
        } else if (c == '='){ // += or -=
            lexeme[1] = c;
            lexeme[2] = '\0';
            return ADDSUB_ASSIGN;  
        } else{
            ungetc(c, stdin); // 吐回去
            lexeme[1] = '\0';
            return ADDSUB;
        }
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    } else if (c == '&') {
        strcpy(lexeme, "&");
        return AND;
    } else if (c == '|') {
        strcpy(lexeme, "|");
        return OR;
    } else if (c == '^') {
        strcpy(lexeme, "^");
        return XOR;
    } else if (isVarName(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while ((isdigit(c) || isVarName(c)) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin); // 多的吐回去
        lexeme[i] = '\0';
        return ID;
    } else if (c == EOF) {
        return ENDFILE;
    } else {
        return UNKNOWN;
    }
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void) {
    return lexeme;
}


/*============================================================================================
parser implementation
============================================================================================*/

void initTable(void) { // 我們存位置，有初始值
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 4;
    strcpy(table[2].name, "z");
    table[2].val = 8;
    sbcount = 3;
}

int getval(char *str) {
    int i = 0;

    for (i = 0; i < sbcount; i++){
        if (strcmp(str, table[i].name) == 0){
            return table[i].val; // 這個也是位置
        }
    }

    // 不然呢，找不到就爛掉啊
    error(NOTFOUND);
    return 0;
}

int setval(char *str, int val) {
    int i = 0;

    // 先檢查是否已經在 table 內
    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            return table[i].val;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);

    strcpy(table[sbcount].name, str);
    table[sbcount].val = 4*(sbcount); // 分配到新的空記憶體
    sbcount++;
    return table[sbcount-1].val; // 因為我剛剛 ++ 了，所以扣回去
}

BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode *node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}


// factor := INT | ID | INCDEC ID | LPAREN assign_expr RPAREN
BTNode *factor(void) {
    BTNode *retp = NULL; // 我甚至刪了 left，不需要
    if (match(INT)) {
        retp = makeNode(INT, getLexeme());
        advance();
    } else if (match(ID)) {
        retp = makeNode(ID, getLexeme());
        advance();
    } else if (match(INCDEC)) {
        // 我要偷吃步，直接用 ASSIGN 和 ADDSUB 拚出 INCDEC
        // 晚點就不用煩惱 Assembly 了
        // 喔但這樣會慢上 10 個 cycles，不緊張啦之後再優化
        char opr[MAXLEN]; // 記錄這咖是減或加
        strcpy(opr, getLexeme());
        advance();
        if (match(ID)){
            char varname[MAXLEN];
            strcpy(varname, getLexeme()); // 紀錄變數名稱
            strcpy(varname, getLexeme());
            advance();
            
            retp = makeNode(ASSIGN, "=");
            retp->left = makeNode(ID, varname);
            // 這如果是 C++ 我就直接傳 opr[0] 了唉唉唉
            retp->right = makeNode(ADDSUB, (opr[0] == '+') ? "+" : "-" );
            retp->right->left = makeNode(ID, varname);
            retp->right->right = makeNode(INT, "1");

        } else{ // 如果 ++ 或 -- 後面接的不是變數，那就可以去死了
            error(SYNTAXERR);
        }
    } else if (match(LPAREN)) {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
            error(MISPAREN);
    } else {
        error(NOTNUMID);
    }
    return retp;
}

// unary_expr := ADDSUB unary_expr | factor
BTNode *unary_expr(void){
    BTNode *retp = NULL;
    if (match(ADDSUB)){
        retp = makeNode(ADDSUB, getLexeme());
        // 對，我很懶惰，我會把 -x 變成 0-x，儘管要犧牲 cycle
        retp->left = makeNode(INT, "0");
        advance();
        retp->right = unary_expr();
    } else{
        retp = factor();
    }
    return retp;
}

// muldiv_expr_tail := unary_expr muldiv_expr_tail 
BTNode *muldiv_expr(void) {
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}

// muldiv_expr_tail := MULDIV unary_expr muldiv_expr_tail | NiL 
BTNode *muldiv_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary_expr();
        return muldiv_expr_tail(node);
    } else {
        return left;
    }
}

// addsub_expr := muldiv_expr addsub_expr_tail 
BTNode *addsub_expr(void) {
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}

// addsub_expr_tail := ADDSUB muldiv_expr addsub_expr_tail | NiL 
BTNode *addsub_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        return addsub_expr_tail (node);
    } else {
        return left;
    }
}

// and_expr := addsub_expr and_expr_tail 
BTNode *and_expr(void) {
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}

// and_expr_tail := AND addsub_expr and_expr_tail | NiL
BTNode *and_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(AND)) {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        return and_expr_tail(node);
    } else {
        return left;
    }
}

// xor_expr := and_expr xor_expr_tail 
BTNode *xor_expr(void) {
    BTNode *node = and_expr();
    return xor_expr_tail(node);
}

// xor_expr_tail := XOR and_expr xor_expr_tail | NiL 
BTNode *xor_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(XOR)) {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        return xor_expr_tail(node);
    } else {
        return left;
    }
}

// or_expr := xor_expr or_expr_tail 
BTNode *or_expr(void) {
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}

// or_expr_tail := OR xor_expr or_expr_tail | NiL
BTNode *or_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(OR)) {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        return or_expr_tail(node);
    } else {
        return left;
    }
}

// assign_expr := ID ASSIGN assign_expr | ID ADDSUB_ASSIGN assign_expr | or_expr
BTNode *assign_expr(void){
    BTNode *left = or_expr();
    
    // 是 ID ASSIGN assign_expr 型嗎
    if (match(ASSIGN)) {
        if (left->data != ID){ // 好笑嗎等號左邊不是變數
            error(NOTLVAL);
        }
        BTNode *rept = makeNode(ASSIGN, "=");
        advance();
        rept->left = left;
        rept->right = assign_expr();
        return rept;
    } 
    // 是 ID ADDSUB_ASSIGN assign_expr 型嗎
    else if (match(ADDSUB_ASSIGN)){
        if (left->data != ID){
            error(NOTLVAL);
        }
        
        // 我要用 ASSIGN 和 ADDSUB 來湊出 ADDSUB_ASSIGN
        BTNode *node = makeNode(ASSIGN, "=");
        node->left = left;
        node->right = makeNode(ADDSUB, ((getLexeme()[0]) == '+') ? "+" : "-");
        advance();
        
        // 這個要複製一份給它
        node->right->left = makeNode(ID, left->lexeme); 
        node->right->right = assign_expr(); 
        return node;
    }
    
    // 什麼也沒有
    return left;
}

int isBiOpr(BTNode *root){
    return (root->data == ADDSUB) || (root->data == MULDIV) || (root->data == AND) ||
           (root->data == OR) || (root->data == XOR);
}

BTNode *foldConstant(BTNode *root){
    if (root == NULL) return root;
    root->left = foldConstant(root->left);
    root->right = foldConstant(root->right);
    if (isBiOpr(root) && !hasVariable(root)){
        int val = calculateConstant(root);
        char buf[MAXLEN];
        sprintf(buf, "%d", val); // int to string
        BTNode *retp = makeNode(INT, buf);
        freeTree(root);
        return retp;
    }
    return root;
}

// statement := ENDFILE | END | assign_expr | or_expr 
BTNode *statement(void) {
    BTNode *retp = NULL;

    if (match(ENDFILE)) {
        return makeNode(ENDFILE, "EOF");
    } else if (match(END)) {
        advance();
        return makeNode(END, "END");
    } else {
        retp = assign_expr();
        if (match(END)) {
            retp = foldConstant(retp);
            advance();
            return retp;
        } else {
            error(SYNTAXERR);
        }
    }
}

void err(ErrorType errorNum) {
    if (PRINTERR) {
        fprintf(stderr, "error: ");
        switch (errorNum) {
            case MISPAREN:
                fprintf(stderr, "mismatched parenthesis\n");
                break;
            case NOTNUMID:
                fprintf(stderr, "number or identifier expected\n");
                break;
            case NOTFOUND:
                fprintf(stderr, "variable not defined\n");
                break;
            case RUNOUT:
                fprintf(stderr, "out of memory\n");
                break;
            case NOTLVAL:
                fprintf(stderr, "lvalue required as an operand\n");
                break;
            case DIVZERO:
                fprintf(stderr, "divide by constant zero\n");
                break;
            case SYNTAXERR:
                fprintf(stderr, "syntax error\n");
                break;
            default:
                fprintf(stderr, "undefined error\n");
                break;
        }
    }
    // 這個要自己輸出
    printf("EXIT 1\n");
    exit(0);
}


/*============================================================================================
codeGen implementation
============================================================================================*/

// 我本來想寫 int *reg_status = (int*) malloc(8* sizeof(int)) 但我被 C 搞到了

int reg_status[8] = {0}; // 表示有哪些 register 是空的，0 表示空著
Symbol regs[8]; // 模擬 register

// 找到一個空的 register
int get_reg(){
    for (int i = 0; i <= 7; i++){
        if (!reg_status[i]){
            reg_status[i] = 1;
            return i;
        }
    }
    error(RUNOUT);
}

// 把 register 給 free 掉
void free_reg(int index){
    reg_status[index] = 0;
}

// 回傳＂這顆樹內是否含有任何變數＂，1 代表有
int hasVariable(BTNode *root) {
    if (root == NULL) return 0;
    if (root->data == ID) return 1;
    return hasVariable(root->left) || hasVariable(root->right);
}

// 這是 TA 本來的 evaluateTree 改個名而已，記得裡面也要改
int calculateConstant(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;

    if (root != NULL) {
        switch (root->data) {
            case ID:
                retval = getval(root->lexeme);
                break;
            case INT:
                retval = atoi(root->lexeme);
                break;
            case ASSIGN:
                rv = calculateConstant(root->right);
                retval = setval(root->left->lexeme, rv);
                break;
            case ADDSUB:
            case MULDIV:
            case AND:
            case OR:
            case XOR:
                lv = calculateConstant(root->left);
                rv = calculateConstant(root->right);
                if (strcmp(root->lexeme, "+") == 0) {
                    retval = lv + rv;
                } else if (strcmp(root->lexeme, "-") == 0) {
                    retval = lv - rv;
                } else if (strcmp(root->lexeme, "*") == 0) {
                    retval = lv * rv;
                } else if (strcmp(root->lexeme, "/") == 0) {
                    if (rv == 0)
                        error(DIVZERO);
                    retval = lv / rv;
                } else if (strcmp(root->lexeme, "&") == 0) {
                    retval = lv & rv;
                } else if (strcmp(root->lexeme, "|") == 0) {
                    retval = lv | rv;
                } else if (strcmp(root->lexeme, "^") == 0) {
                    retval = lv ^ rv;
                }
                break;
            default:
                retval = 0;
        }
    }
    return retval;
}

// 這個要拿來輸出 Assembly!
int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;
    if (root != NULL) {
        switch (root->data) {
            case ID:
                retval = get_reg(); // 拿一個新的 register
                int addr = getval(root->lexeme); // 原本在 memory 的位置
                printf("MOV r%d [%d]\n", retval, addr);
                break;
            case INT:
                retval = get_reg(); // 拿一個新的 register
                printf("MOV r%d %s\n", retval, root->lexeme); // e.g. MOV r1, 7
                break;
            case ASSIGN: // 例如 x = 7
                rv = evaluateTree(root->right);
                // 誒注意要用 setval 而非 getval，後者會直接殺了新變數
                int left_addr = setval(root->left->lexeme, 0);
                printf("MOV [%d] r%d\n", left_addr, rv); // 存回去記憶體
                retval = rv; // 為了防禦 x = y = 3 這類，還是要把右小孩傳上去
                break;
            case ADDSUB:
            case MULDIV:
            case AND:
            case OR:
            case XOR:
                // 所有的二元運算子！
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);

                // 左右小孩都出來了，那就當無情打印機
                if (strcmp("+", root->lexeme) == 0){
                    printf("ADD r%d r%d\n", lv, rv);
                } else if (strcmp("-", root->lexeme) == 0){
                    printf("SUB r%d r%d\n", lv, rv);
                } else if (strcmp("*", root->lexeme) == 0){
                    printf("MUL r%d r%d\n", lv, rv);
                } else if (strcmp("/", root->lexeme) == 0){
                    if (!hasVariable(root->right)){ // 如果右小孩是純變數還 = 0 就要 EXIT 1 了
                        if (calculateConstant(root->right) == 0){
                            error(DIVZERO);
                            return -1;
                        }
                    }
                    printf("DIV r%d r%d\n", lv, rv);
                } else if (strcmp("&", root->lexeme) == 0){
                    printf("AND r%d r%d\n", lv, rv);
                } else if (strcmp("|", root->lexeme) == 0){
                    printf("OR r%d r%d\n", lv, rv);
                } else if (strcmp("^", root->lexeme) == 0){
                    printf("XOR r%d r%d\n", lv, rv);
                }

                free_reg(rv); // 右小孩可以釋出了
                retval = lv; // 左小孩繼續往上傳
                break;
            default:
                retval = 0;
        }
    }
    return retval;
}

void printPrefix(BTNode *root) {
    if (root != NULL) {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}


/*============================================================================================
main
============================================================================================*/

// This package is a calculator
// It works like a Python interpretor
// Example:
// >> y = 2
// >> z = 2
// >> x = 3 * y + 4 / (2 * z)
// It will print the answer of every line
// You should turn it into an expression compiler
// And print the assembly code according to the input

// This is the grammar used in this package
// You can modify it according to the spec and the slide
// statement  :=  ENDFILE | END | expr END
// expr    	  :=  term expr_tail
// expr_tail  :=  ADDSUB term expr_tail | NiL
// term 	  :=  factor term_tail
// term_tail  :=  MULDIV factor term_tail| NiL
// factor	  :=  INT | ADDSUB INT |
//		   	      ID  | ADDSUB ID  |
//		   	      ID ASSIGN expr |
//		   	      LPAREN expr RPAREN |
//		   	      ADDSUB LPAREN expr RPAREN

int getindex(BTNode *tar){
    return getval(tar->lexeme) / 4;
}

int is_useful[TBLSIZE] = {0}; // 記錄變數有沒有用，沒用的標成 0

int treeUseful (BTNode *root){
    if (root == NULL) return 0;
    int ret = 0;
    if (root->data == ASSIGN && root->left->data == ID && is_useful[getindex(root->left)]){
        return 1;
    }
    return treeUseful(root->left) || treeUseful(root->right);
}

void makeUseful(BTNode *root){
    if (root == NULL) return;
    if (root->data == ID) is_useful[getindex(root)] = 1;
    makeUseful(root->left);
    makeUseful(root->right);
}

void syntaxCheck(BTNode *root){
    if (root == NULL) return;
    if (root->data == ASSIGN){
        syntaxCheck(root->right);
        setval(root->left->lexeme, 573); // 573 是亂喊的，沒差
    } else if (root->data == ID){
        getval(root->lexeme); // 嘗試觸發 error(NOTFOUND)
    } else if (root->data == MULDIV && (root->lexeme)[0] == '/'){
        syntaxCheck(root->left);
        syntaxCheck(root->right);
        if (!hasVariable(root->right) && calculateConstant(root->right) == 0) {
            error(DIVZERO);
        }
    } else{
        syntaxCheck(root->left);
        syntaxCheck(root->right);
    }
}

BTNode *tests[200000]; // 我就不信他測資超過 2 * 10^5 行

int main() {
    initTable();
    
    int test_count = 0;

    for (int i = 0; i < 2e5; i++){
        BTNode *tree = statement();
        syntaxCheck(tree);
        if (tree->data == ENDFILE) break;
        test_count++;
        tests[i] = tree;
    }

    /* 開始把沒用的變數全部踢掉 */
    is_useful[0] = 1; // x
    is_useful[1] = 1; // y
    is_useful[2] = 1; // z
    for (int i = test_count-1; i >= 0; i--){
        BTNode *root = tests[i];
        if (treeUseful(root)){
            makeUseful(root);
        } else{
            freeTree(root);
            tests[i] = NULL;
        }
    }

    /* 正常計算 */
    for (int i = 0; i < test_count; i++) {
        if (tests[i] == NULL || tests[i]->data == END) continue; 
        int final_reg = evaluateTree(tests[i]);
        free_reg(final_reg);
        freeTree(tests[i]);
    }
    printf("MOV r0 [0]\n");
    printf("MOV r1 [4]\n");
    printf("MOV r2 [8]\n");
    printf("EXIT 0\n");
    exit(0);
    return 0;
}

/*
Original:

r[0] = 102
r[1] = 0
r[2] = 2
Total clock cycles are 2590

the expression cannot be evaluated

r[0] = 20
r[1] = -9
r[2] = -169
Total clock cycles are 2350

r[0] = -1
r[1] = 200
r[2] = 10
Total clock cycles are 9160

r[0] = 1
r[1] = 16
r[2] = 23
Total clock cycles are 16490
*/
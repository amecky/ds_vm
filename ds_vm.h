#pragma once
#include <stdint.h>
#include <stdlib.h>

#ifndef VM_MALLOC
#define VM_MALLOC(sz) malloc(sz)
#define VM_FREE(p) free(p)
#endif

struct VMStack_t {
	float* data;
	uint16_t size;
	uint16_t capacity;
};

typedef struct VMStack_t VMStack;

typedef void (*vmFunction)(VMStack*);

struct CharBuffer_t {
	char* data;
	uint16_t size;
	uint16_t capacity;
	uint16_t num;
};

typedef struct CharBuffer_t CharBuffer;

typedef enum { TOK_EMPTY, TOK_NUMBER, TOK_FUNCTION, TOK_VARIABLE, TOK_LEFT_PARENTHESIS, TOK_RIGHT_PARENTHESIS } VMTokenType;

struct VMToken_t {
	VMTokenType type;
	union {
		uint16_t id;
		float value;
	};
};

typedef struct VMToken_t VMToken;

typedef enum {VT_VARIABLE, VT_CONSTANT} VMVariableType;

struct VMVariable_t {
	VMVariableType type;
	uint16_t nameIndex;
	float value;
};

typedef struct VMVariable_t VMVariable;

struct VMFunction_t {
	uint32_t hash;
	const char* name;
	vmFunction function;
	uint8_t precedence;
	uint8_t num_parameters;
};

typedef struct VMFunction_t VMFunction;

struct Context_t {

	uint16_t num_variables;
	VMVariable variables[32];
	CharBuffer names;
	VMFunction functions[32];
	uint16_t num_functions;

};

typedef struct Context_t Context;

struct Expression {
	VMToken* tokens;
	uint16_t num;
	uint16_t id;
};

Context* vm_create_context();

uint16_t vm_add_variable(Context* ctx, const char* name, float value);

uint16_t add_variable(Context* ctx, const char* name, int length, float value);

uint16_t vm_add_constant(Context* ctx, const char* name, float value);

uint16_t vm_parse(Context* ctx, const char* source, VMToken* tokens, uint16_t capacity);

//void vm_parse(Context* ctx, const char* source, Expression& e);

//const char* get_token_name(Token::TokenType type);

//uint16_t vm_parse(const char* source, Context& ctx, Token* tokens, uint16_t capacity);

float vm_run(VMToken* byteCode, uint16_t capacity, Context* ctx);

//void print_bytecode(const Context& ctx, Token* byteCode, uint16_t num);

//void print_bytecode(const Context& ctx, const Expression& exp);

void vm_destroy_context(Context* ctx);

#ifdef DS_VM_IMPLEMENTATION
#include <string.h>
#include <stdio.h>
//#include <math.h>
//#include <random>

const uint32_t FNV_Prime = 0x01000193; //   16777619
const uint32_t FNV_Seed = 0x811C9DC5; // 2166136261

static inline uint32_t vm_fnv1a(const char* text) {
	const unsigned char* ptr = (const unsigned char*)text;
	uint32_t hash = FNV_Seed;
	while (*ptr) {
		hash = (*ptr++ ^ hash) * FNV_Prime;
	}
	return hash;
}

static void vm__buffer_resize(CharBuffer* buffer, uint16_t newCap) {
	if (newCap > buffer->capacity) {
		char* tmp = (char*)VM_MALLOC(newCap);
		if (buffer->data != 0) {
			memcpy(tmp, buffer->data, buffer->size);
			VM_FREE(buffer->data);
		}
		buffer->capacity = newCap;
		buffer->data = tmp;
	}
}

static void* vm__buffer_alloc(CharBuffer* buffer,uint16_t sz) {
	if (buffer->size + sz > buffer->capacity) {
		int d = buffer->capacity * 2 + 8;
		if (d < sz) {
			d = sz * 2 + 8;
		}
		vm__buffer_resize(buffer, d);
	}
	void* res = buffer->data + buffer->size;
	buffer->size += sz;
	int d = sz / 4;
	if (d == 0) {
		d = 1;
	}
	buffer->num += d;
	return res;
}



const char* vm__buffer_get_name(CharBuffer* buffer, uint16_t index) {
	return buffer->data + index;
}

static uint16_t vm__buffer_append(CharBuffer* buffer, const char* s, int len) {
	if (buffer->size + len + 1 > buffer->capacity) {
		vm__buffer_resize(buffer, buffer->capacity + len + 1 + 8);
	}
	const char* t = s;
	uint16_t ret = buffer->size;
	for (int i = 0; i < len; ++i) {
		buffer->data[buffer->size++] = *t;
		++t;
	}
	buffer->data[buffer->size++] = '\0';
	return ret;
}
/*
static uint16_t vm__buffer_append(CharBuffer* buffer, const char* s) {
	int len = strlen(s);
	return vm__buffer_append(buffer, s, len);
}
*/
/*
uint16_t vm__buffer_append(CharBuffer* buffer, char s) {
	if (size + 1 > buffer->capacity) {
		vm__buffer_resize(buffer, buffer->capacity + 9);
	}
	uint16_t ret = buffer->size;
	buffer->data[buffer->size++] = s;
	buffer->data[buffer->size++] = '\0';
	return ret;
}
*/
#ifndef VM_PUSH
#define VM_PUSH(stack,value) vm__push(stack,value)
#define VM_POP(stack)        vm__pop(stack)
#endif

static float vm__pop(VMStack* stack) { return stack->data[--stack->size]; }
static void vm__push(VMStack* stack, float f) { stack->data[stack->size++] = f; }

static uint16_t vm__add_name(Context* ctx, const char* name) {
	return vm__buffer_append(&ctx->names, name, strlen(name));
}

uint16_t vm_add_variable(Context* ctx, const char* name, float value) {
	VMVariable* v = &ctx->variables[ctx->num_variables++];
	v->nameIndex = vm__add_name(ctx,name);
	v->type = VT_VARIABLE;
	v->value = value;
	return  ctx->num_variables - 1;
}

uint16_t add_variable(Context* ctx, const char* name, int length, float value) {
	VMVariable* v = &ctx->variables[ctx->num_variables++];
	v->nameIndex = vm__add_name(ctx, name);
	v->type = VT_VARIABLE;
	v->value = value;
	return  ctx->num_variables - 1;
}

uint16_t vm_add_constant(Context* ctx, const char* name, float value) {
	VMVariable* v = &ctx->variables[ctx->num_variables++];
	v->nameIndex = vm__add_name(ctx, name);
	v->type = VT_CONSTANT;
	v->value = value;
	return  ctx->num_variables - 1;
}

static void add_function(Context* ctx, const char* name, vmFunction func, uint8_t precedence, uint8_t num_params) {
	VMFunction* f = &ctx->functions[ctx->num_functions++];
	f->hash = vm_fnv1a(name);
	f->function = func;
	f->precedence = precedence;
	f->name = name;
	f->num_parameters = num_params;
}

static void vm_no_op(VMStack* stack) {}

static void vm_add(VMStack* stack) {
	VM_PUSH(stack, VM_POP(stack) + VM_POP(stack));
}

static void vm_sub(VMStack* stack) {
	float a = VM_POP(stack);
	float b = VM_POP(stack);
	VM_PUSH(stack,b - a);
}

static void vm_mul(VMStack* stack) {
	VM_PUSH(stack, VM_POP(stack) * VM_POP(stack));
}

static void vm_div(VMStack* stack) {
	float a = VM_POP(stack);
	float b = VM_POP(stack);
	VM_PUSH(stack, b / a);
}

static void vm_sin(VMStack* stack) {
	VM_PUSH(stack, sin(VM_POP(stack)));
}

static void vm_cos(VMStack* stack) {
	VM_PUSH(stack, cos(VM_POP(stack)));
}

static void vm_abs(VMStack* stack) {
	VM_PUSH(stack, abs(VM_POP(stack)));
}

//const char* FUNCTION_NAMES[] = { ",","+","-","*","/","u-","u+","sin","cos","abs","ramp","lerp","range","saturate","random", "channel" };
Context* vm_create_context() {
	Context* ctx = (Context*)VM_MALLOC(sizeof(Context));
	ctx->num_variables = 0;
	ctx->num_functions = 0;
	ctx->names.data = 0;
	ctx->names.size = 0;
	ctx->names.capacity = 0;
	ctx->names.num = 0;
	vm__buffer_alloc(&ctx->names, 128);
	add_function(ctx, ",", vm_no_op, 1, 0);
	add_function(ctx, "+", vm_add, 12, 2);
	add_function(ctx, "-", vm_sub, 12, 2);
	add_function(ctx, "*", vm_mul, 13, 2);
	add_function(ctx, "/", vm_div, 13, 2);
	add_function(ctx, "u-", vm_abs, 16, 1);
	add_function(ctx, "u+", vm_no_op, 1, 0);
	add_function(ctx, "sin", vm_sin, 17, 1);
	add_function(ctx, "cos", vm_cos, 17, 1);
	add_function(ctx, "abs", vm_abs, 17, 1);
	return ctx;
}

void vm_destroy_context(Context* ctx) {
	VM_FREE(ctx->names.data);
	VM_FREE(ctx);
}

// ------------------------------------------------------------------
// token names
// ------------------------------------------------------------------
//const char* TOKEN_NAMES[] = { "EMPTY", "NUMBER", "FUNCTION", "VARIABLE", "LEFT_PARENTHESIS", "RIGHT_PARENTHESIS" };

// ------------------------------------------------------------------
// get token name
// ------------------------------------------------------------------
//const char* get_token_name(Token::TokenType type) {
	//return TOKEN_NAMES[type % 6];
//}

// ------------------------------------------------------------------
// op codes
// ------------------------------------------------------------------
//enum OpCode { OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_UNARY_MINUS, OP_NOP, OP_SIN, OP_COS, OP_ABS, OP_RAMP, OP_LERP, OP_RANGE, OP_SATURATE, OP_RANDOM, OP_CHANNEL };


// ------------------------------------------------------------------
// Function definition
// ------------------------------------------------------------------
/*
struct Function {
const char* name;
OpCode code;
uint8_t precedence;
uint8_t num_parameters;
};
*/
// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------
/*
const static Function FUNCTIONS[] = {
{ FUNCTION_NAMES[0], OP_NOP, 1, 0 },
{ FUNCTION_NAMES[1], OP_ADD, 12,2 },
{ FUNCTION_NAMES[2], OP_SUB, 12, 2 },
{ FUNCTION_NAMES[3], OP_MUL, 13,2 },
{ FUNCTION_NAMES[4], OP_DIV, 13,2 },
{ FUNCTION_NAMES[5], OP_UNARY_MINUS, 16, 1 },
{ FUNCTION_NAMES[6], OP_NOP, 1, 0 },
{ FUNCTION_NAMES[7], OP_SIN, 17, 1 },
{ FUNCTION_NAMES[8], OP_COS, 17, 1 },
{ FUNCTION_NAMES[9], OP_ABS, 17, 1 },
{ FUNCTION_NAMES[10], OP_RAMP, 17, 2 },
{ FUNCTION_NAMES[11], OP_LERP, 17, 3 },
{ FUNCTION_NAMES[12], OP_RANGE, 17, 3 },
{ FUNCTION_NAMES[13], OP_SATURATE, 17, 3 },
{ FUNCTION_NAMES[14], OP_RANDOM, 17, 2 },
{ FUNCTION_NAMES[15], OP_RANDOM, 17, 1 }
};

const uint16_t NUM_FUNCTIONS = 16;

const char* get_function_name(uint16_t id) {
return FUNCTIONS[id].name;
}
*/
// ------------------------------------------------------------------
// find variable
// ------------------------------------------------------------------
static uint16_t find_variable(const char *s, uint16_t len, Context* ctx) {
	for (uint16_t i = 0; i < ctx->num_variables; ++i) {
		const char* varName = vm__buffer_get_name(&ctx->names,ctx->variables[i].nameIndex);
		if (strncmp(s, varName, len) == 0 && strlen(varName) == len) {
			return i;
		}
	}
	return UINT16_MAX;
}

// ------------------------------------------------------------------
// find function
// ------------------------------------------------------------------
/*
static uint16_t find_function(const char *s, uint16_t len) {
for (uint16_t i = 0; i < NUM_FUNCTIONS; ++i) {
if (strncmp(s, FUNCTIONS[i].name, len) == 0 && strlen(FUNCTIONS[i].name) == len) {
return i;
}
}
return UINT16_MAX;
}
*/
static uint16_t find_function(Context* ctx, const char *s, uint16_t len) {
	for (uint16_t i = 0; i < ctx->num_functions; ++i) {
		if (strncmp(s, ctx->functions[i].name, len) == 0 && strlen(ctx->functions[i].name) == len) {
			return i;
		}
	}
	return UINT16_MAX;
}

// ------------------------------------------------------------------
// is numeric
// ------------------------------------------------------------------
static int vm__is_numeric(const char c) {
	if ((c >= '0' && c <= '9')) {
		return 1;
	}
	return 0;
}

// ------------------------------------------------------------------
// is whitespace
// ------------------------------------------------------------------
static int vm__is_whitespace(const char c) {
	if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
		return 1;
	}
	return 0;
}

// ------------------------------------------------------------------
// get token for identifier
// ------------------------------------------------------------------
static VMToken token_for_identifier(Context* ctx, const char *identifier, unsigned len) {
	uint16_t i;
	char tmp[256];	
	const char* p = identifier;
	int l = 0;
	while (l < len) {
		tmp[l++] = *p;
		++p;
	}
	tmp[l] = '\0';
	if ((i = find_variable(identifier, len, ctx)) != UINT16_MAX) {
		VMToken t;
		t.type = TOK_VARIABLE;
		t.id = i;
		return t;
	}
	else if ((i = find_function(ctx, identifier, len)) != UINT16_MAX) {
		VMToken t;
		t.type = TOK_FUNCTION;
		t.id = i;
		return t;
	}
	else {
		int l = 0;
		const char* p = identifier;
		while (!vm__is_whitespace(*p)) {
			++l;
			++p;
		}
		i = add_variable(ctx, identifier, l, 0.0f);
		VMToken t;
		t.type = TOK_VARIABLE;
		t.id = i;
		return t;
	}
}

// ------------------------------------------------------------------
// get token for identifier
// ------------------------------------------------------------------
/*
static VMToken token_for_identifier(Context* ctx, const char *identifier, unsigned len) {
	uint16_t i;
	char tmp[256];
	const char* p = identifier;
	int l = 0;
	while (l < len) {
		tmp[l++] = *p;
		++p;
	}
	tmp[l] = '\0';
	if ((i = find_variable(identifier, len, *ctx)) != UINT16_MAX) {
		return Token(Token::VARIABLE, i);
	}
	else if ((i = find_function(ctx, identifier, len)) != UINT16_MAX) {
		return Token(Token::FUNCTION, i);
	}
	else {
		int l = 0;
		const char* p = identifier;
		while (!isWhitespace(*p)) {
			++l;
			++p;
		}
		i = add_variable(ctx, identifier, l);
		return Token(Token::VARIABLE, i);
	}
}
*/


// ------------------------------------------------------------------
// has function
// ------------------------------------------------------------------
static int has_function(Context* ctx, char * identifier) {
	if (find_function(ctx, identifier, strlen(identifier)) != UINT16_MAX) {
		return 1;
	}
	return 0;
}



// ------------------------------------------------------------------
// strtof
// ------------------------------------------------------------------
static float vm__strtof(const char* p, char** endPtr) {
	while (vm__is_whitespace(*p)) {
		++p;
	}
	float sign = 1.0f;
	if (*p == '-') {
		sign = -1.0f;
		++p;
	}
	else if (*p == '+') {
		++p;
	}
	float value = 0.0f;
	while (vm__is_numeric(*p)) {
		value *= 10.0f;
		value = value + (*p - '0');
		++p;
	}
	if (*p == '.') {
		++p;
		float dec = 1.0f;
		float frac = 0.0f;
		while (vm__is_numeric(*p)) {
			frac *= 10.0f;
			frac = frac + (*p - '0');
			dec *= 10.0f;
			++p;
		}
		value = value + (frac / dec);
	}
	if (endPtr) {
		*endPtr = (char *)(p);
	}
	return value * sign;
}





// ------------------------------------------------------------------
// Function stack item
// ------------------------------------------------------------------
struct FunctionVMStackItem_t {
	//FunctionVMStackItem() {}
	//FunctionVMStackItem(Token t, int p, int pl) : token(t), precedence(p), par_level(pl) {}
	VMToken token;
	int precedence;
	int par_level;
};

typedef struct FunctionVMStackItem_t FunctionVMStackItem;

static int cmp(FunctionVMStackItem t,FunctionVMStackItem f) {
	if (t.par_level != f.par_level) return t.par_level - f.par_level;
	return t.precedence - f.precedence;
}

//function_stack[num_function_stack - 1] >= f

//inline bool operator<(const FunctionVMStackItem &other) const { return cmp(other) < 0; }
//inline bool operator<=(const FunctionVMStackItem &other) const { return cmp(other) <= 0; }
//inline bool operator==(const FunctionVMStackItem &other) const { return cmp(other) == 0; }
//inline int operator>=(FunctionVMStackItem  t,FunctionVMStackItem other) { return cmp(t,other) >= 0; }
//inline bool operator>(const FunctionVMStackItem &other) const { return cmp(other) > 0; }

// -------------------------------------------------------
// random
// -------------------------------------------------------
/*
static std::mt19937 mt;
static bool rnd_first = true;

static void init_random(unsigned long seed) {
	std::random_device r;
	std::seed_seq new_seed{ r(), r(), r(), r(), r(), r(), r(), r() };
	mt.seed(new_seed);
}


float random(float min, float max) {
	if (rnd_first) {
		rnd_first = false;
		init_random(0);
	}
	std::uniform_real_distribution<float> dist(min, max);
	return dist(mt);
}
*/

VMToken vm__create_token(VMTokenType type) {
	VMToken t;
	printf("creating token: %d\n", type);
	t.type = type;
	t.value = 0.0f;
	return t;
}

VMToken vm__create_token_with_value(VMTokenType type, float value) {
	printf("creating token with value %g\n", value);
	VMToken t;
	t.type = type;
	t.value = value;
	return t;
}
// ------------------------------------------------------------------
// parse
// ------------------------------------------------------------------
uint16_t vm_parse(Context* ctx, const char * source, VMToken * byteCode, uint16_t capacity) {
	printf("source: '%s'\n", source);
	int binary = 0;
	const char* p = source;
	unsigned num_tokens = 0;
	unsigned overflow_tokens = 0;
	VMToken* tokens = (VMToken*)VM_MALLOC(capacity * sizeof(VMToken));
	while (*p != 0) {
		VMToken token;
		token.type = TOK_EMPTY;
		if (*p >= '0' && *p <= '9') {
			char *out;
			token = vm__create_token_with_value(TOK_NUMBER, vm__strtof(p, &out));
			p = out;
			binary = 1;
		}
		else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p == '_')) {
			const char *identifier = p;
			while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p == '_') || (*p >= '0' && *p <= '9'))
				p++;
			token = token_for_identifier(ctx, identifier, p - identifier);
			binary = 1;
		}
		else {
			printf("p: '%c'\n", *p);
			switch (*p) {
				case '(': token = vm__create_token(TOK_LEFT_PARENTHESIS); binary = 0; break;
				case ')': token = vm__create_token(TOK_RIGHT_PARENTHESIS); binary = 1; break;
				case ' ': case '\t': case '\n': case '\r': break;
				case '-': token = token_for_identifier(ctx, binary ? "-" : "u-", 1 + binary); binary = 0; break;
				case '+': token = token_for_identifier(ctx, binary ? "+" : "u+", 1 + binary); binary = 0; break;
				default: {
					char s1[2] = { *p,0 };
					char s2[3] = { *p, *(p + 1), 0 };
					printf("s1 %s s2 %s\n", s1, s2);
					if (s2[1] && has_function(ctx, s2)) {
						token = token_for_identifier(ctx, s2,2);
						++p;
					}
					else {
						token = token_for_identifier(ctx, s1, 1);
					}
					binary = 0;
					break;
				}
			}
			++p;
		}

		if (token.type != TOK_EMPTY) {
			if (num_tokens == capacity)
				++overflow_tokens;
			else
				tokens[num_tokens++] = token;
		}
	}

	uint16_t num_rpl = 0;
	FunctionVMStackItem function_stack[256];

	unsigned num_function_stack = 0;

	int par_level = 0;
	for (unsigned i = 0; i<num_tokens; ++i) {
		VMToken token = tokens[i];
		switch (token.type) {
			case TOK_NUMBER:
			case TOK_VARIABLE:
				byteCode[num_rpl++] = token;
				break;
			case TOK_LEFT_PARENTHESIS:
				++par_level;
				break;
			case TOK_RIGHT_PARENTHESIS:
				--par_level;
				break;
			case TOK_FUNCTION: {
				FunctionVMStackItem f;
				f.token = token;
				f.precedence = ctx->functions[token.id].precedence;
				f.par_level = par_level;
				while (num_function_stack>0 && cmp(function_stack[num_function_stack - 1],f) >= 0)
					byteCode[num_rpl++] = function_stack[--num_function_stack].token;
				function_stack[num_function_stack++] = f;
				break;
			}
		}
	}

	while (num_function_stack > 0) {
		byteCode[num_rpl++] = function_stack[--num_function_stack].token;
	}
	VM_FREE(tokens);
	return num_rpl;

}

// ------------------------------------------------------------------
// parse
// ------------------------------------------------------------------
/*
uint16_t vm_parse(Context* ctx, const char * source, Token * byteCode, uint16_t capacity) {
	printf("source: '%s'\n", source);
	bool binary = false;
	const char* p = source;
	unsigned num_tokens = 0;
	unsigned overflow_tokens = 0;
	Token* tokens = (Token*)VM_MALLOC(capacity * sizeof(Token));
	while (*p != 0) {
		Token token(Token::EMPTY);
		if (*p >= '0' && *p <= '9') {
			char *out;
			token = Token(Token::NUMBER, vm__strtof(p, &out));
			p = out;
			binary = true;
		}
		else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p == '_')) {
			const char *identifier = p;
			while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p == '_') || (*p >= '0' && *p <= '9'))
				p++;
			token = token_for_identifier(ctx, identifier, p - identifier);
			binary = true;
		}
		else {
			switch (*p) {
			case '(': token = Token(Token::LEFT_PARENTHESIS); binary = false; break;
			case ')': token = Token(Token::RIGHT_PARENTHESIS); binary = true; break;
			case ' ': case '\t': case '\n': case '\r': break;
			case '-': token = token_for_identifier(ctx, binary ? "-" : "u-"); binary = false; break;
			case '+': token = token_for_identifier(ctx, binary ? "+" : "u+"); binary = false; break;
			default: {
				char s1[2] = { *p,0 };
				char s2[3] = { *p, *(p + 1), 0 };
				if (s2[1] && has_function(ctx, s2)) {
					token = token_for_identifier(ctx, s2);
					++p;
				}
				else {
					token = token_for_identifier(ctx, s1);
				}
				binary = false;
				break;
			}
			}
			++p;
		}

		if (token.type != Token::EMPTY) {
			if (num_tokens == capacity)
				++overflow_tokens;
			else
				tokens[num_tokens++] = token;
		}
	}

	uint16_t num_rpl = 0;
	FunctionVMStackItem function_stack[256];

	unsigned num_function_stack = 0;

	int par_level = 0;
	for (unsigned i = 0; i<num_tokens; ++i) {
		Token &token = tokens[i];
		switch (token.type) {
		case Token::NUMBER:
		case Token::VARIABLE:
			byteCode[num_rpl++] = token;
			break;
		case Token::LEFT_PARENTHESIS:
			++par_level;
			break;
		case Token::RIGHT_PARENTHESIS:
			--par_level;
			break;
		case Token::FUNCTION: {
			FunctionVMStackItem f(token, ctx->functions[token.id].precedence, par_level);
			while (num_function_stack>0 && function_stack[num_function_stack - 1] >= f)
				byteCode[num_rpl++] = function_stack[--num_function_stack].token;
			function_stack[num_function_stack++] = f;
			break;
		}
		}
	}

	while (num_function_stack > 0) {
		byteCode[num_rpl++] = function_stack[--num_function_stack].token;
	}
	VM_FREE(tokens);
	return num_rpl;

}
*/
// ------------------------------------------------------------------
// parse
// ------------------------------------------------------------------
/*
void vm_parse(Context* ctx, const char * source, Expression& exp) {
	bool binary = false;
	const char* p = source;
	uint16_t num_tokens = 0;
	uint16_t overflow_tokens = 0;
	Token tokens[512];
	while (*p != 0) {
		Token token(Token::EMPTY);
		if (*p >= '0' && *p <= '9') {
			char *out;
			token = Token(Token::NUMBER, vm__strtof(p, &out));
			p = out;
			binary = true;
		}
		else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p == '_')) {
			const char *identifier = p;
			while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p == '_') || (*p >= '0' && *p <= '9'))
				p++;
			token = token_for_identifier(ctx, identifier, p - identifier);
			binary = true;
		}
		else {
			switch (*p) {
			case '(': token = Token(Token::LEFT_PARENTHESIS); binary = false; break;
			case ')': token = Token(Token::RIGHT_PARENTHESIS); binary = true; break;
			case ' ': case '\t': case '\n': case '\r': break;
			case '-': token = token_for_identifier(ctx, binary ? "-" : "u-"); binary = false; break;
			case '+': token = token_for_identifier(ctx, binary ? "+" : "u+"); binary = false; break;
			default: {
				char s1[2] = { *p,0 };
				char s2[3] = { *p, *(p + 1), 0 };
				if (s2[1] && has_function(ctx, s2)) {
					token = token_for_identifier(ctx, s2);
					++p;
				}
				else {
					token = token_for_identifier(ctx, s1);
				}
				binary = false;
				break;
			}
			}
			++p;
		}

		if (token.type != Token::EMPTY) {
			if (num_tokens == 512)
				++overflow_tokens;
			else
				tokens[num_tokens++] = token;
		}
	}

	exp.tokens = (Token*)VM_MALLOC(sizeof(Token) * num_tokens);

	uint16_t num_rpl = 0;
	FunctionVMStackItem function_stack[256];

	unsigned num_function_stack = 0;

	int par_level = 0;
	for (unsigned i = 0; i<num_tokens; ++i) {
		Token &token = tokens[i];
		switch (token.type) {
		case Token::NUMBER:
		case Token::VARIABLE:
			exp.tokens[num_rpl++] = token;
			break;
		case Token::LEFT_PARENTHESIS:
			++par_level;
			break;
		case Token::RIGHT_PARENTHESIS:
			--par_level;
			break;
		case Token::FUNCTION: {
			FunctionVMStackItem f(token, ctx->functions[token.id].precedence, par_level);
			while (num_function_stack>0 && function_stack[num_function_stack - 1] >= f)
				exp.tokens[num_rpl++] = function_stack[--num_function_stack].token;
			function_stack[num_function_stack++] = f;
			break;
		}
		}
	}

	while (num_function_stack > 0) {
		exp.tokens[num_rpl++] = function_stack[--num_function_stack].token;
	}
	exp.num = num_rpl;
}
*/
// ------------------------------------------------------------------
// print byte code
// ------------------------------------------------------------------
/*
void print_bytecode(const Context& ctx, Token* byteCode, uint16_t num) {
	printf("--------------------------------------\n");
	printf("num: %d\n", num);
	printf("variables:\n");
	for (uint16_t i = 0; i < ctx.num_variables; ++i) {
		printf("%d = %s : %g\n", i, ctx.names.get(ctx.variables[i].nameIndex), ctx.variables[i].value);
	}
	for (uint16_t i = 0; i < num; ++i) {
		byteCode[i] = byteCode[i];
		if (byteCode[i].type == Token::NUMBER) {
			printf("%d = %s (%g)\n", i, get_token_name(byteCode[i].type), byteCode[i].value);
		}
		else if (byteCode[i].type == Token::VARIABLE) {
			printf("%d = %s (%d)\n", i, get_token_name(byteCode[i].type), byteCode[i].id);
		}
		else {
			printf("%d = %s (%s)\n", i, get_token_name(byteCode[i].type), ctx.functions[byteCode[i].id].name);
		}
	}
	printf("--------------------------------------\n");
}

void print_bytecode(const Context& ctx, const Expression& exp) {
	print_bytecode(ctx, exp.tokens, exp.num);
}
*/
// ------------------------------------------------------------------
// run
// ------------------------------------------------------------------
float vm_run(VMToken* byteCode, uint16_t capacity, Context* ctx, const char* result_name) {
	float stack_data[32] = { 0.0f };
	VMStack stack = { stack_data, 0, 32 };
	float a, b, t;
	for (uint16_t i = 0; i < capacity; ++i) {
		if (byteCode[i].type == TOK_NUMBER) {
			VM_PUSH(&stack,byteCode[i].value);
		}
		else if (byteCode[i].type == TOK_VARIABLE) {
			VM_PUSH(&stack,ctx->variables[byteCode[i].id].value);
		}
		else if (byteCode[i].type == TOK_FUNCTION) {
			uint16_t id = byteCode[i].id;
			VMFunction f = ctx->functions[id];
			(f.function)(&stack);
			/*
			const Function& f = FUNCTIONS[id];
			switch (f.code) {
			case OP_ADD: stack.push(stack.pop() + stack.pop()); break;
			case OP_SUB: a = stack.pop(); b = stack.pop(); stack.push(b - a); break;
			case OP_MUL: stack.push(stack.pop() * stack.pop()); break;
			case OP_DIV: a = stack.pop(); b = stack.pop(); stack.push(b / a); break;
			case OP_SIN: stack.push(sin(stack.pop())); break;
			case OP_COS: stack.push(cos(stack.pop())); break;
			case OP_ABS: stack.push(abs(stack.pop())); break;
			case OP_RAMP: a = stack.pop(); t = stack.pop(); stack.push(a >= t ? 1.0f : 0.0f); break;
			case OP_RANGE: t = stack.pop(); b = stack.pop(); a = stack.pop(); stack.push(t >= a && t <= b ? 1.0f : 0.0f); break;
			case OP_LERP: t = stack.pop(); a = stack.pop(); b = stack.pop(); stack.push((1.0f - t) * b + t * a); break;
			case OP_SATURATE: a = stack.pop(); if (a < 0.0f) stack.push(0.0f); else if (a > 1.0f) stack.push(1.0f); else stack.push(a); break;
			case OP_RANDOM: a = stack.pop(); b = stack.pop(); stack.push(random(b, a)); break;
			case OP_CHANNEL: a = stack.pop(); stack.push(a); break;
			}
			*/
		}
	}
	float r = 0.0f;
	if (stack.size > 0) {
		r = VM_POP(&stack);
	}
	if (result_name != 0) {
		uint16_t id = find_variable(result_name, strlen(result_name), ctx);
		if (id != UINT16_MAX) {
			ctx->variables[id].value = r;
		}
	}
	return r;
}
/*
CharBuffer::CharBuffer() : data(nullptr), size(0), capacity(0), num(0) {}

CharBuffer::~CharBuffer() {
	if (data != nullptr) {
		delete[] data;
	}
}

void* CharBuffer::alloc(uint16_t sz) {
	if (size + sz > capacity) {
		int d = capacity * 2 + 8;
		if (d < sz) {
			d = sz * 2 + 8;
		}
		resize(d);
	}
	auto res = data + size;
	size += sz;
	int d = sz / 4;
	if (d == 0) {
		d = 1;
	}
	num += d;
	return res;
}

void CharBuffer::resize(uint16_t newCap) {
	if (newCap > capacity) {
		char* tmp = (char*)VM_MALLOC(newCap);
		if (data != nullptr) {
			memcpy(tmp, data, size);
			VM_FREE(data);
		}
		capacity = newCap;
		data = tmp;
	}
}

const char* CharBuffer::get(uint16_t index) const {
	return data + index;
}

uint16_t CharBuffer::append(const char* s, int len) {
	if (size + len + 1 > capacity) {
		resize(capacity + len + 1 + 8);
	}
	const char* t = s;
	uint16_t ret = size;
	for (int i = 0; i < len; ++i) {
		data[size++] = *t;
		++t;
	}
	data[size++] = '\0';
	return ret;
}

uint16_t CharBuffer::append(const char* s) {
	int len = strlen(s);
	return append(s, len);
}

uint16_t CharBuffer::append(char s) {
	if (size + 1 > capacity) {
		resize(capacity + 9);
	}
	uint16_t ret = size;
	data[size++] = s;
	data[size++] = '\0';
	return ret;
}
*/
#endif

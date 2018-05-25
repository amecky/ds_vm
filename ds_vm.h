/*
	ds_vm v1.0 https://github.com/amecky/ds_vm MIT license

	This is a single header C implementation of a simple math expression parser.
	In order to use you need to do this in *one* C or C++ to create the implementation:
	#include ...
	#include ...
	#define DS_VM_IMPLEMENTATION
	#include "ds_vm.h"

	You can also define VM_MALLOC and VM_FREE to avoid using malloc,free

	Here is a short example demonstrating the usage:

	vm_context* ctx = vm_create_context();
	vm_token tokens[64];
	int ret = vm_parse(ctx, "10 + ( 4 * 3 + 8 / 2)", tokens, 64);
	float r = 0.0f;
	int code = vm_run(ctx, tokens, num, &r);
	if (code == 0) {
		printf("Result: %g\n", r);
	}
	else {
		printf("Error: %s\n", vm_get_error(code));
	}
	vm_free(ctx);

	Release notes:

	v1.0 - initial release

	============================    Contributors    =========================

	Andreas Mecky (amecky@gmail.com)



*/

#ifndef DS_VM_INCLUDE_H
#define DS_VM_INCLUDE_H
#include <stdlib.h>

#ifndef VM_MALLOC
#define VM_MALLOC(sz) malloc(sz)
#define VM_FREE(p) free(p)
#endif

#ifdef DS_VM_STATIC
#define DSDEF static
#else
#define DSDEF extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

	struct vm_stack_t {
		float* data;
		int size;
		int capacity;
	};

	typedef struct vm_stack_t vm_stack;

	typedef void(*vmFunction)(vm_stack*);

	typedef enum { TOK_EMPTY, TOK_NUMBER, TOK_FUNCTION, TOK_VARIABLE, TOK_LEFT_PARENTHESIS, TOK_RIGHT_PARENTHESIS } vm_token_type;

	struct vm_token_t {
		vm_token_type type;
		union {
			int id;
			float value;
		};
	};

	typedef struct vm_token_t vm_token;

	struct vm_variable_t {
		int hash;
		float value;
		const char* name;
	};

	typedef struct vm_variable_t vm_variable;

	struct vm_function_t {
		int hash;
		vmFunction function;
		int precedence;
		int num_parameters;
		const char* name;
	};

	typedef struct vm_function_t vm_function;

	struct vm_context_t {

		int num_variables;
		vm_variable variables[32];
		vm_function functions[32];
		int num_functions;

	};

	typedef struct vm_context_t vm_context;

	DSDEF vm_context* vm_create_context();

	DSDEF int vm_add_variable(vm_context* ctx, const char* name, float value);

	DSDEF void vm_set_variable(vm_context* ctx, const char* name, float value);

	DSDEF void vm_add_function(vm_context* ctx, const char* name, vmFunction func, int precedence, int num_params);

	DSDEF int vm_parse(vm_context* ctx, const char* source, vm_token* tokens, int capacity);

	DSDEF int vm_run(vm_context* ctx, vm_token* byteCode, int capacity, float* ret);

	DSDEF void vm_debug(vm_token* tokens, int num);

	DSDEF void vm_destroy_context(vm_context* ctx);

	DSDEF const char* vm_get_error(int code);

#ifdef __cplusplus
}
#endif

#endif

#ifdef DS_VM_IMPLEMENTATION

struct vm_error_code_t {
	int code;
	const char* message;
};

typedef struct vm_error_code_t vm_error_code;

const static vm_error_code ERRORS[] = {
	{0,"Success"},
	{1,"No return value on stack"},
	{2,"Requested number of parameters not found on stack"}
};

const char* TOKEN_NAMES[] = { "TOK_EMPTY", "TOK_NUMBER", "TOK_FUNCTION", "TOK_VARIABLE", "TOK_LEFT_PARENTHESIS", "TOK_RIGHT_PARENTHESIS" };

const int FNV_Prime = 0x01000193; //   16777619
const int FNV_Seed = 0x811C9DC5; // 2166136261

// ------------------------------------------------------------------
// internal method to generate hash
// ------------------------------------------------------------------
static inline int vm__fnv1a(const char* text) {
	const unsigned char* ptr = (const unsigned char*)text;
	int hash = FNV_Seed;
	while (*ptr) {
		hash = (*ptr++ ^ hash) * FNV_Prime;
	}
	return hash;
}

// ------------------------------------------------------------------
// get error message by code
// ------------------------------------------------------------------
DSDEF const char* vm_get_error(int code) {
	return ERRORS[code].message;
}

#ifndef VM_PUSH
#define VM_PUSH(stack,value) vm__push(stack,value)
#define VM_POP(stack)        vm__pop(stack)
#endif

static float vm__pop(vm_stack* stack) { return stack->data[--stack->size]; }
static void vm__push(vm_stack* stack, float f) { stack->data[stack->size++] = f; }

// ------------------------------------------------------------------
// internal method to build hash from string
// ------------------------------------------------------------------
static int vm__build_hash(const char* v, int len) {
	int l = 0;
	char tmp[256];
	for (int i = 0; i < len; ++i) {
		tmp[l++] = v[i];
	}
	tmp[l] = '\0';
	int r = vm__fnv1a(tmp);
	return r;
}

// ------------------------------------------------------------------
// add new variable to vm_context
// ------------------------------------------------------------------
DSDEF int vm_add_variable(vm_context* ctx, const char* name, float value) {
	vm_variable* v = &ctx->variables[ctx->num_variables++];
	v->hash = vm__fnv1a(name);
	v->value = value;
	v->name = name;
	return  ctx->num_variables - 1;
}

// ------------------------------------------------------------------
// set value of variable
// ------------------------------------------------------------------
DSDEF void vm_set_variable(vm_context* ctx, const char* name, float value) {
	int h = vm__fnv1a(name);
	for (int i = 0; i < ctx->num_variables; ++i) {
		if (ctx->variables[i].hash == h) {
			ctx->variables[i].value = value;
		}
	}
}

// ------------------------------------------------------------------
// internal method to add a new variable
// ------------------------------------------------------------------
static int vm__add_variable(vm_context* ctx, const char* name, int length, float value) {
	vm_variable* v = &ctx->variables[ctx->num_variables++];
	v->hash = vm__build_hash(name, length);
	v->value = value;
	v->name = name;
	return  ctx->num_variables - 1;
}

// ------------------------------------------------------------------
// add function to vm_context
// ------------------------------------------------------------------
DSDEF void vm_add_function(vm_context* ctx, const char* name, vmFunction func, int precedence, int num_params) {
	vm_function* f = &ctx->functions[ctx->num_functions++];
	f->hash = vm__fnv1a(name);
	f->function = func;
	f->precedence = precedence;
	f->num_parameters = num_params;
	f->name = name;
}

// ------------------------------------------------------------------
// all supported functions
// ------------------------------------------------------------------
static void vm_no_op(vm_stack* stack) {}

static void vm_add(vm_stack* stack) {
	VM_PUSH(stack, VM_POP(stack) + VM_POP(stack));
}

static void vm_sub(vm_stack* stack) {
	float a = VM_POP(stack);
	float b = VM_POP(stack);
	VM_PUSH(stack,b - a);
}

static void vm_mul(vm_stack* stack) {
	VM_PUSH(stack, VM_POP(stack) * VM_POP(stack));
}

static void vm_div(vm_stack* stack) {
	float a = VM_POP(stack);
	float b = VM_POP(stack);
	VM_PUSH(stack, b / a);
}

static void vm_pow(vm_stack* stack) {
	float a = VM_POP(stack);
	float b = VM_POP(stack);
	VM_PUSH(stack, pow(b,a));
}

static void vm_sin(vm_stack* stack) {
	VM_PUSH(stack, sin(VM_POP(stack)));
}

static void vm_cos(vm_stack* stack) {
	VM_PUSH(stack, cos(VM_POP(stack)));
}

static void vm_tan(vm_stack* stack) {
	VM_PUSH(stack, tan(VM_POP(stack)));
}

static void vm_abs(vm_stack* stack) {
	VM_PUSH(stack, abs(VM_POP(stack)));
}

static void vm_lerp(vm_stack* stack) {
	float t = VM_POP(stack); 
	float a = VM_POP(stack); 
	float b = VM_POP(stack); 
	VM_PUSH(stack,(1.0f - t) * b + t * a);
}

// ------------------------------------------------------------------
// create new vm_context
// ------------------------------------------------------------------
DSDEF vm_context* vm_create_context() {
	vm_context* ctx = (vm_context*)VM_MALLOC(sizeof(vm_context));
	ctx->num_variables = 0;
	ctx->num_functions = 0;
	vm_add_function(ctx, ",", vm_no_op, 1, 0);
	vm_add_function(ctx, "+", vm_add, 12, 2);
	vm_add_function(ctx, "-", vm_sub, 12, 2);
	vm_add_function(ctx, "*", vm_mul, 13, 2);
	vm_add_function(ctx, "/", vm_div, 13, 2);
	vm_add_function(ctx, "u-", vm_abs, 16, 1);
	vm_add_function(ctx, "u+", vm_no_op, 1, 0);
	vm_add_function(ctx, "sin", vm_sin, 17, 1);
	vm_add_function(ctx, "cos", vm_cos, 17, 1);
	vm_add_function(ctx, "abs", vm_abs, 17, 1);
	vm_add_function(ctx, "lerp", vm_lerp, 17, 1);
	vm_add_function(ctx, "pow", vm_pow, 17, 2);
	vm_add_function(ctx, "exp", vm_lerp, 17, 1);
	vm_add_function(ctx, "tan", vm_tan, 17, 1);
	return ctx;
}

// ------------------------------------------------------------------
// destroy vm_context
// ------------------------------------------------------------------
DSDEF void vm_destroy_context(vm_context* ctx) {
	VM_FREE(ctx);
}

// ------------------------------------------------------------------
// internal method to find a variable
// ------------------------------------------------------------------
static int vm__find_variable(const char *s, int len, vm_context* ctx) {
	int h = vm__build_hash(s, len);
	for (int i = 0; i < ctx->num_variables; ++i) {
		if ( ctx->variables[i].hash == h) {
			return i;
		}
	}
	return -1;
}

// ------------------------------------------------------------------
// internal method to find a function
// ------------------------------------------------------------------
static int vm__find_function(vm_context* ctx, const char *s, int len) {
	int h = vm__build_hash(s, len);
	for (int i = 0; i < ctx->num_functions; ++i) {
		if ( h == ctx->functions[i].hash) {
			return i;
		}
	}
	return -1;
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
static vm_token vm__token_for_identifier(vm_context* ctx, const char *identifier, unsigned len) {
	int i;
	if ((i = vm__find_variable(identifier, len, ctx)) != -1) {
		vm_token t;
		t.type = TOK_VARIABLE;
		t.id = i;
		return t;
	}
	else if ((i = vm__find_function(ctx, identifier, len)) != -1) {
		vm_token t;
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
		i = vm__add_variable(ctx, identifier, l, 0.0f);
		vm_token t;
		t.type = TOK_VARIABLE;
		t.id = i;
		return t;
	}
}

// ------------------------------------------------------------------
// internal method to check wether there is a known function
// ------------------------------------------------------------------
static int vm__has_function(vm_context* ctx, char * identifier) {
	if (vm__find_function(ctx, identifier, strlen(identifier)) != -1) {
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
	vm_token token;
	int precedence;
	int par_level;
};

typedef struct FunctionVMStackItem_t FunctionVMStackItem;

static int cmp(FunctionVMStackItem t,FunctionVMStackItem f) {
	if (t.par_level != f.par_level) return t.par_level - f.par_level;
	return t.precedence - f.precedence;
}

// ------------------------------------------------------------------
// internal method to create a token
// ------------------------------------------------------------------
vm_token vm__create_token(vm_token_type type) {
	vm_token t;
	t.type = type;
	t.value = 0.0f;
	return t;
}

// ------------------------------------------------------------------
// internal method to create a token
// ------------------------------------------------------------------
vm_token vm__create_token_with_value(vm_token_type type, float value) {
	vm_token t;
	t.type = type;
	t.value = value;
	return t;
}
// ------------------------------------------------------------------
// parse
// ------------------------------------------------------------------
DSDEF int vm_parse(vm_context* ctx, const char * source, vm_token * byteCode, int capacity) {
	int binary = 1;
	const char* p = source;
	unsigned num_tokens = 0;
	unsigned overflow_tokens = 0;
	vm_token* tokens = (vm_token*)VM_MALLOC(capacity * sizeof(vm_token));
	while (*p != 0) {
		vm_token token;
		token.type = TOK_EMPTY;
		if (*p >= '0' && *p <= '9') {
			char *out;
			token = vm__create_token_with_value(TOK_NUMBER, vm__strtof(p, &out));
			p = out;
			binary = 0;
		}
		else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p == '_')) {
			const char *identifier = p;
			while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p == '_') || (*p >= '0' && *p <= '9'))
				p++;
			token = vm__token_for_identifier(ctx, identifier, p - identifier);
			binary = 0;
		}
		else {
			switch (*p) {
			case '(': token = vm__create_token(TOK_LEFT_PARENTHESIS); binary = 1; break;
				case ')': token = vm__create_token(TOK_RIGHT_PARENTHESIS); binary = 0; break;
				case ' ': case '\t': case '\n': case '\r': break;
				case '-': token = vm__token_for_identifier(ctx, binary == 0 ? "-" : "u-", 1 + binary); printf("found - \n"); break;
				case '+': token = vm__token_for_identifier(ctx, binary == 0 ? "+" : "u+", 1 + binary); binary = 1; break;
				default: {
					char s1[2] = { *p,0 };
					char s2[3] = { *p, *(p + 1), 0 };
					if (s2[1] && vm__has_function(ctx, s2)) {
						token = vm__token_for_identifier(ctx, s2,2);
						++p;
					}
					else {
						token = vm__token_for_identifier(ctx, s1, 1);
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

	int num_rpl = 0;
	FunctionVMStackItem function_stack[256];

	unsigned num_function_stack = 0;

	int par_level = 0;
	for (unsigned i = 0; i<num_tokens; ++i) {
		vm_token token = tokens[i];
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
// run
// ------------------------------------------------------------------
DSDEF int vm_run(vm_context* ctx, vm_token* byteCode, int capacity, float* ret) {
	float stack_data[32] = { 0.0f };
	vm_stack stack = { stack_data, 0, 32 };
	for (int i = 0; i < capacity; ++i) {
		if (byteCode[i].type == TOK_NUMBER) {
			VM_PUSH(&stack,byteCode[i].value);
		}
		else if (byteCode[i].type == TOK_VARIABLE) {
			VM_PUSH(&stack,ctx->variables[byteCode[i].id].value);
		}
		else if (byteCode[i].type == TOK_FUNCTION) {
			int id = byteCode[i].id;
			vm_function f = ctx->functions[id];
			if (stack.size >= f.num_parameters) {
				(f.function)(&stack);
			}
			else {
				return 2;
			}
		}
	}
	if (stack.size > 0) {
		*ret = VM_POP(&stack);
		return 0;
	}
	return 1;
}

DSDEF void vm_debug(vm_context* ctx, vm_token* tokens, int num) {
	printf("bytecode: \n");
	for (int i = 0; i < num; ++i) {
		printf("%d : %s ", i, TOKEN_NAMES[tokens[i].type]);
		if (tokens[i].type == TOK_FUNCTION) {
			printf("%s\n", ctx->functions[tokens[i].id].name);
		}
		else if (tokens[i].type == TOK_NUMBER) {
			printf("%g\n", tokens[i].value);
		}
		else if (tokens[i].type == TOK_VARIABLE) {
			printf("%s %g\n", ctx->variables[tokens[i].id].name, tokens[i].value);
		}
	}
}

#endif

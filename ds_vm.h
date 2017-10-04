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
	uint32_t hash;
	float value;
};

typedef struct VMVariable_t VMVariable;

struct VMFunction_t {
	uint32_t hash;
	vmFunction function;
	uint8_t precedence;
	uint8_t num_parameters;
};

typedef struct VMFunction_t VMFunction;

struct Context_t {

	uint16_t num_variables;
	VMVariable variables[32];
	VMFunction functions[32];
	uint16_t num_functions;

};

typedef struct Context_t Context;

Context* vm_create_context();

uint16_t vm_add_variable(Context* ctx, const char* name, float value);

uint16_t add_variable(Context* ctx, const char* name, int length, float value);

uint16_t vm_add_constant(Context* ctx, const char* name, float value);

uint16_t vm_parse(Context* ctx, const char* source, VMToken* tokens, uint16_t capacity);

float vm_run(VMToken* byteCode, uint16_t capacity, Context* ctx);

void vm_destroy_context(Context* ctx);

#ifdef DS_VM_IMPLEMENTATION

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

#ifndef VM_PUSH
#define VM_PUSH(stack,value) vm__push(stack,value)
#define VM_POP(stack)        vm__pop(stack)
#endif

static float vm__pop(VMStack* stack) { return stack->data[--stack->size]; }
static void vm__push(VMStack* stack, float f) { stack->data[stack->size++] = f; }

uint16_t vm_add_variable(Context* ctx, const char* name, float value) {
	VMVariable* v = &ctx->variables[ctx->num_variables++];
	v->hash = vm_fnv1a(name);
	v->type = VT_VARIABLE;
	v->value = value;
	return  ctx->num_variables - 1;
}

uint16_t add_variable(Context* ctx, const char* name, int length, float value) {
	VMVariable* v = &ctx->variables[ctx->num_variables++];
	v->hash = vm_fnv1a(name);
	v->type = VT_VARIABLE;
	v->value = value;
	return  ctx->num_variables - 1;
}

uint16_t vm_add_constant(Context* ctx, const char* name, float value) {
	VMVariable* v = &ctx->variables[ctx->num_variables++];
	v->hash = vm_fnv1a(name);
	v->type = VT_CONSTANT;
	v->value = value;
	return  ctx->num_variables - 1;
}

static void add_function(Context* ctx, const char* name, vmFunction func, uint8_t precedence, uint8_t num_params) {
	VMFunction* f = &ctx->functions[ctx->num_functions++];
	f->hash = vm_fnv1a(name);
	f->function = func;
	f->precedence = precedence;
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

Context* vm_create_context() {
	Context* ctx = (Context*)VM_MALLOC(sizeof(Context));
	ctx->num_variables = 0;
	ctx->num_functions = 0;
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
	VM_FREE(ctx);
}

// ------------------------------------------------------------------
// find variable
// ------------------------------------------------------------------
static uint16_t find_variable(const char *s, uint16_t len, Context* ctx) {
	int l = 0;
	char tmp[256];
	for (uint16_t i = 0; i < len; ++i) {
		tmp[l++] = s[i];
	}
	tmp[l] = '\0';
	uint32_t h = vm_fnv1a(tmp);
	for (uint16_t i = 0; i < ctx->num_variables; ++i) {
		if ( ctx->variables[i].hash == h) {
			return i;
		}
	}
	return UINT16_MAX;
}

// ------------------------------------------------------------------
// find function
// ------------------------------------------------------------------
static uint16_t find_function(Context* ctx, const char *s, uint16_t len) {
	int l = 0;
	char tmp[256];
	for (uint16_t i = 0; i < len; ++i) {
		tmp[l++] = s[i];
	}
	tmp[l] = '\0';
	uint32_t h = vm_fnv1a(tmp);
	for (uint16_t i = 0; i < ctx->num_functions; ++i) {
		if ( h == ctx->functions[i].hash) {
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
	VMToken token;
	int precedence;
	int par_level;
};

typedef struct FunctionVMStackItem_t FunctionVMStackItem;

static int cmp(FunctionVMStackItem t,FunctionVMStackItem f) {
	if (t.par_level != f.par_level) return t.par_level - f.par_level;
	return t.precedence - f.precedence;
}

VMToken vm__create_token(VMTokenType type) {
	VMToken t;
	t.type = type;
	t.value = 0.0f;
	return t;
}

VMToken vm__create_token_with_value(VMTokenType type, float value) {
	VMToken t;
	t.type = type;
	t.value = value;
	return t;
}
// ------------------------------------------------------------------
// parse
// ------------------------------------------------------------------
uint16_t vm_parse(Context* ctx, const char * source, VMToken * byteCode, uint16_t capacity) {
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
			switch (*p) {
				case '(': token = vm__create_token(TOK_LEFT_PARENTHESIS); binary = 0; break;
				case ')': token = vm__create_token(TOK_RIGHT_PARENTHESIS); binary = 1; break;
				case ' ': case '\t': case '\n': case '\r': break;
				case '-': token = token_for_identifier(ctx, binary ? "-" : "u-", 1 + binary); binary = 0; break;
				case '+': token = token_for_identifier(ctx, binary ? "+" : "u+", 1 + binary); binary = 0; break;
				default: {
					char s1[2] = { *p,0 };
					char s2[3] = { *p, *(p + 1), 0 };
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
// run
// ------------------------------------------------------------------
float vm_run(VMToken* byteCode, uint16_t capacity, Context* ctx) {
	float stack_data[32] = { 0.0f };
	VMStack stack = { stack_data, 0, 32 };
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
			// verify stack size
			(f.function)(&stack);
		}
	}
	float r = 0.0f;
	if (stack.size > 0) {
		r = VM_POP(&stack);
	}
	return r;
}

#endif

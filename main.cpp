#include <stdio.h>
#define DS_VM_IMPLEMENTATION
#include "ds_vm.h"

void test_method(vm_stack* stack) {
	float a = VM_POP(stack);
	float b = VM_POP(stack);
	VM_PUSH(stack, (a+b)*10);
}

void print_bytecode(vm_token* tokens, uint16_t num) {
	printf("bytecode: \n");
	for (uint16_t i = 0; i < num; ++i) {
		printf("%d : %d %d %g\n", i, tokens[i].type, tokens[i].id,tokens[i].value);
	}
}

void testAddFunction() {
	printf("testAddFunction\n");
	vm_context* ctx = vm_create_context();
	vm_add_function(ctx, "FOO", test_method, 17, 2);
	vm_token tokens[64];
	uint16_t ret = vm_parse(ctx, "2 + FOO(10,20)", tokens, 64);
	float r = 0.0f;
	uint16_t code = vm_run(ctx, tokens, ret, &r);
	if (code == 0) {
		printf("result: %g\n", r);
	}
	else {
		printf("Error: %s\n", vm_get_error(code));
	}
	vm_destroy_context(ctx);
}

void testWrongMethod() {
	printf("testWrongMethod\n");
	vm_context* ctx = vm_create_context();
	vm_token tokens[64];
	uint16_t ret = vm_parse(ctx, "2 + sin(2,3)", tokens, 64);
	float r = 0.0f;
	uint16_t code = vm_run(ctx, tokens, ret, &r);
	if (code == 0) {
		printf("result: %g\n", r);
	}
	else {
		printf("Error: %s\n",vm_get_error(code));
	}
	vm_destroy_context(ctx);
}

void testFunction() {
	printf("testFunction\n");
	vm_context* ctx = vm_create_context();
	vm_token tokens[64];
	uint16_t ret = vm_parse(ctx, "2 + abs(-2)", tokens, 64);
	float r = 0.0f;
	uint16_t code = vm_run(ctx, tokens, ret, &r);
	if (code == 0) {
		printf("result: %g\n", r);
	}
	else {
		printf("Error: %s\n", vm_get_error(code));
	}
	vm_destroy_context(ctx);
}

void testSetVar() {
	printf("testSetVar\n");
	vm_context* ctx = vm_create_context();
	vm_add_variable(ctx, "TEST", 4.0f);
	vm_token tokens[64];
	uint16_t ret = vm_parse(ctx, "2 + 4 + TEST", tokens, 64);
	float r = 0.0f;
	vm_run(ctx, tokens, ret, &r);
	printf("result: %g\n", r);
	vm_set_variable(ctx, "TEST", 20.0f);
	r = 0.0f;
	vm_run(ctx, tokens, ret, &r);
	printf("result: %g\n", r);
	vm_destroy_context(ctx);
}

void testSimpleExpression() {
	printf("testSimpleExpression\n");
	vm_context* ctx = vm_create_context();
	vm_add_variable(ctx, "TEST", 4.0f);
	vm_token tokens[64];
	uint16_t ret = vm_parse(ctx, "2 + 4 + TEST", tokens, 64);
	printf("num: %d\n", ret);
	for (int i = 0; i < ret; ++i) {
		if (tokens[i].type == TOK_FUNCTION) {
			printf("%d = function(%d) / %d\n", i, tokens[i].type, tokens[i].id);
		}
		else {
			printf("%d = variable(%d) / %d %g\n", i, tokens[i].type, tokens[i].id, tokens[i].value);
		}
	}
	float r = 0.0f;
	if (vm_run(ctx, tokens, ret, &r) == 0) {
		printf("result: %g\n", r);
	}
	else {
		printf("Error\n");
	}
	vm_destroy_context(ctx);
}

int main() {
	//testFunction();
	testAddFunction();
	//testSimpleExpression();
	//testSetVar();
	//testWrongMethod();
}
#include <stdio.h>
#define DS_VM_IMPLEMENTATION
#define DS_VM_STATIC
#include "ds_vm.h"

void test_method(vm_stack* stack) {
	float a = VM_POP(stack);
	float b = VM_POP(stack);
	VM_PUSH(stack, (a+b)*10);
}

int assertEquals(vm_context* ctx, vm_token* tokens, int num, float expected) {
	float r = 0.0f;
	int code = vm_run(ctx, tokens, num, &r);
	if (code == 0) {
		float d = expected - r;
		if (d < 0.0f) {
			d *= -1.0f;
		}
		if (d > 0.0001f) {
			printf("Error: expected: %g but got %g\n", expected, r);
			return 0;
		}
	}
	else {
		printf("Error: %s\n", vm_get_error(code));
		return 0;
	}
	return 1;
}

typedef int(*testFunction)(vm_context*);

int test_add_function(vm_context* ctx) {
	vm_add_function(ctx, "FOO", test_method, 17, 2);
	vm_token tokens[64];
	int ret = vm_parse(ctx, "2 + FOO(10,20)", tokens, 64);
	return assertEquals(ctx, tokens, ret, 302.0f);
}

int test_lerp_function(vm_context* ctx) {
	vm_token tokens[64];
	int ret = vm_parse(ctx, "2 + lerp(4,8,0.25)", tokens, 64);
	return assertEquals(ctx, tokens, ret, 7.0f);
}

int test_pow_function(vm_context* ctx) {
	vm_token tokens[64];
	int ret = vm_parse(ctx, "2 + pow((2+2),2)", tokens, 64);
	return assertEquals(ctx, tokens, ret, 18.0f);
}

int test_abs_function(vm_context* ctx) {
	vm_token tokens[64];
	int ret = vm_parse(ctx, "2 + abs(-2)", tokens, 64);
	return assertEquals(ctx, tokens, ret, 4.0f);
}

int test_variable(vm_context* ctx) {
	vm_add_variable(ctx, "TEST", 4.0f);
	vm_token tokens[64];
	int ret = vm_parse(ctx, "2 + 4 + TEST", tokens, 64);
	return assertEquals(ctx, tokens, ret, 10.0f);
}

int test_unknown_variable(vm_context* ctx) {
	vm_add_variable(ctx, "DUMMY", 4.0f);
	vm_token tokens[64];
	int ret = vm_parse(ctx, "2 + 4 + TEST", tokens, 64);
	return assertEquals(ctx, tokens, ret, 6.0f);
}

int test_basic_expression(vm_context* ctx) {
	vm_token tokens[64];
	int ret = vm_parse(ctx, "10 + ( 4 * 3 + 8 / 2)", tokens, 64);
	return assertEquals(ctx, tokens, ret, 26.0f);
}

void run_test(testFunction func, const char* method) {
	printf("executing '%s'\n", method);
	vm_context* ctx = vm_create_context();
	int ret = (func)(ctx);
	if (ret == 0) {
		printf("=> FALSE\n");
	}
	else {
		printf("=> OK\n");
	}
	vm_destroy_context(ctx);
}

int main() {
	run_test(test_basic_expression, "test_basic_expression");
	run_test(test_add_function, "test_add_function");
	run_test(test_lerp_function, "test_lerp_function");
	run_test(test_pow_function, "test_pow_function");
	run_test(test_abs_function, "test_abs_function");
	run_test(test_variable, "test_variable");
	run_test(test_unknown_variable, "test_unknown_variable");
}
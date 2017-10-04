#include <stdio.h>
#define DS_VM_IMPLEMENTATION
#include "ds_vm.h"

void testSimpleExpression() {
	Context* ctx = vm_create_context();
	vm_add_variable(ctx, "TEST", 4.0f);
	VMToken tokens[64];
	uint16_t ret = vm_parse(ctx, "2 + 4 + TEST", tokens, 64);
	float r = vm_run(tokens, ret, ctx);
	printf("result: %g\n", r);
	vm_destroy_context(ctx);
}

int main() {
	testSimpleExpression();
}
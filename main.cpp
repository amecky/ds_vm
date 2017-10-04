#include <stdio.h>
#define DS_VM_IMPLEMENTATION
#include "ds_vm.h"

void testSimpleExpression() {
	Context* ctx = vm_create_context();
	VMToken tokens[64];
	uint16_t ret = vm_parse(&ctx, "2 + 4", tokens, 64);
	vm_destroy_context(ctx);
}

int main() {
	testSimpleExpression();
}
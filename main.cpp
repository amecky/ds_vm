#include <stdio.h>
#define DS_VM_IMPLEMENTATION
#include "ds_vm.h"

void testSimpleExpression() {
	Context* ctx = vm_create_context();
	vm_destroy_context(ctx);
}

int main() {
	testSimpleExpression();
}
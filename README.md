# ds_vm
Simple C single header math expression parser 

# Basic overview

# Variables and constant values

# Example

```
vm_context* ctx = vm_create_context();
vm_add_variable(ctx, "TEST", 4.0f);
vm_token tokens[64];
uint16_t ret = vm_parse(ctx, "2 + 4 + TEST", tokens, 64);
float r = 0.0f;
uint16_t code = vm_run(ctx, tokens, ret, &r);
if (code == 0) {
    printf("result: %g\n", r);
}
else {
    printf("Error: %s\n",vm_get_error(code));
}
vm_destroy_context(ctx);
```

# Build in functions

Beside the basic math operations the following functions are supported as well:

- sin
- cos
- abs

# Adding custom functions

```
void test_method(vm_stack* stack) {
    float a = VM_POP(stack);
    float b = VM_POP(stack);
    VM_PUSH(stack, (a+b)*10);
}
```

```
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
```

# General
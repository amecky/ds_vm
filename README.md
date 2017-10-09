# ds_vm

Simple C single header math expression parser 

# Basic overview

This is a single header C file. 
In order to use you need to do this in *one* C or C++ to create the implementation:
```
#include ...
#include ...
#define DS_VM_IMPLEMENTATION
#include "ds_vm.h"
```
You can also define VM_MALLOC and VM_FREE to avoid using malloc,free

# Release notes

- v1.0 - initial release

# Example

Here is a short example running a simple expression with one variable:

```
#define DS_VM_IMPLEMENTATION
#include "ds_vm.h"
......
vm_context* ctx = vm_create_context();
vm_add_variable(ctx, "TEST", 4.0f);
vm_token tokens[64];
int ret = vm_parse(ctx, "2 + 4 + TEST", tokens, 64);
float r = 0.0f;
int code = vm_run(ctx, tokens, ret, &r);
if (code == 0) {
    printf("result: %g\n", r);
}
else {
    printf("Error: %s\n",vm_get_error(code));
}
vm_destroy_context(ctx);
```

For more examples you can check out the main.cpp which contains a number of tests.

# Build in functions

Beside the basic math operations the following functions are supported as well:

| name | parameters | example        |
|------|------------|----------------|
| sin  |       1    | sin(3.14)      |
| cos  |       1    | cos(3.14)      |
| abs  |       1    | abs(-2)        |
| lerp |       3    | lerp(3,4,0.25) |

# Variables 

You can define up to 32 variables. These will be replaced during the run with the actual value.
You do not have to add variable upfront. ds_vm will create new variables when it finds them in the
expression. 

# Adding custom functions

You can extend the list of function by adding your own functions. There is a function pointer
defined as:

```
typedef void(*vmFunction)(vm_stack*);
```

## Example

Here is a simple example showing how to add a custom function.
First here is the dummy function itself:

```
void test_method(vm_stack* stack) {
    float a = VM_POP(stack);
    float b = VM_POP(stack);
    VM_PUSH(stack, (a+b)*10);
}
```

Here is the code how to add the function:

```
vm_context* ctx = vm_create_context();
vm_add_function(ctx, "FOO", test_method, 17, 2);
vm_token tokens[64];
int ret = vm_parse(ctx, "2 + FOO(10,20)", tokens, 64);
float r = 0.0f;
int code = vm_run(ctx, tokens, ret, &r);
if (code == 0) {
    printf("result: %g\n", r);
}
else {
    printf("Error: %s\n", vm_get_error(code));
}
vm_destroy_context(ctx);
```

# General

This header file is released as is under the MIT license. You can provide feedback or report bugs by sending an email to amecky@gmail.com.


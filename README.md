# libjavm

libjavm is a small, header-only, zero-dependency C++17 Java Virtual Machine library.

It provides everything necessary to run Java (8) code in any kind of system.

The only thing necessary is a proper standard library, whose native interface is being worked on asan optional part of the library.

## Credits

- [python-jvm-interpreter](https://github.com/gkbrk/python-jvm-interpreter), since this project is basically a C++ port of that VM project, with the aim of extending and improving it.

- [andyzip](https://github.com/andy-thomason/andyzip) library, since it's used for JAR loading as an easy-to-use and header-only ZIP file reading library

- [KiVM](https://github.com/imkiva/KiVM), since it's code was checked for certain aspects (mainly the implementation of certain instructions)

## Usage

This is a quick demo of how libjavm works:

```cpp
#include <javm/core/core_Machine.hpp>
using namespace javm;

int main() {

    // First of all, create a machine.
    core::Machine machine;

    // Then (if you need to) load the .class file you want.
    machine.LoadClassFile("<path-to-class-file>");

    // Then, this loads the basic Java classes and types: Object, String... (necessary for most stuff)
    machine.LoadBuiltinNativeClasses();

    // Create input arguments for function call - for main(...) we create a string array:
    java::lang::String arg1;
    arg1.SetString("hello");
    java::lang::String arg2;
    arg2.SetString("java");
    // Javm arrays are vectors of value holders, so this simple function simplifies their creation
    auto args = core::CreateArray<java::lang::String>({ arg1, arg2 });

    // Call the function. Input parameters must be passed as pointers, so it's as simple as creating them here and passing by '&'.
    auto ret_value = machine.CallFunction("<loaded-class-name>", "<static-function-name>", &args);
    
    auto value_type = ret_value.GetValueType();
    auto value_name = core::ClassObject::GetValueTypeName(value_type);
    printf("Returned value type: %s\n", value_name.c_str());

    // Switch and handle the result, depending on the type
    /*
    switch(value_type) {

    }
    */

    return 0;
}
```

## TO-DO list

- [ ] Implement all opcodes/instructions (very, very few missing)

- [ ] Add JAR support (started with it)

- [ ] Implement standard library (barely started, this will take a long time)
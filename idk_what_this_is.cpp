/*
For virtual functions, the compiler converts the name of the virtual function 
into an index into a parge table of pointers to functions

Virtual functions -> mechanism that delays function calls from compile-time to runtime
static binding- look at static type, get exact address of function, and emits hard coded jump instruction
dynamic binding- queries object at runtime to ask for function address

adding a virtual function, you add a virutal method table and virtual pointer
vtable - array of raw function pointers 
virtual pointer - every object instance has this in its first member variable

unique_ptr - not a class like std::vector. It is a resource manager/owner wrapper. 
When a unique pointer goes out of scope, the allocated object is deleted. 
*/
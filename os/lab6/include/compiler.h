// Everything about compiler(GCC)

// __initdata and __init mark code and data only used for system initialization.
// After system startup, the kernel will free them.
//
// TODO(kongjun18): free them.
#define __initdata
#define __init

#define __unused __attribute__((unused))

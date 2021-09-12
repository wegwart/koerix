#include <stdio.h>
#include <stdlib.h>

_Noreturn void _exit(int status)
{
	_Exit(status);
}

_Noreturn void exit(int status)
{
  _Exit(status);
}

_Noreturn void __assert_fail(const char *expr, const char *file, int line, const char *func)
{
	fprintf(stderr, "Assertion failed: %s (%s: %s: %d)\n", expr, file, func, line);
	abort();
}

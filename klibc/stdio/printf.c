#include "../stdio.h"

#if IS_LIBC
void _putchar(char character)
{
    write(stdout, &character, 1);
}
#endif


/* 
   stdlib / print not yet available 
   program is started by kernel in user mode
   env$ gcc -ffreestanding -nostdlib -Wl,-T ../platform/pc/skel/usr/lib/x86_64.ld hello.c -o hello_world
   env$ mkdir ../skel/bin
   cp hello_world ../skel/bin/hello_world
*/

void _start()
{
    __asm__ volatile ( "int $0x80;"); /* system call to kernel to avoid crash */
}
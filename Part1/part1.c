#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
int main()
{
int fd= open("foo.txt", O_RDONLY|O_CREAT);
int fd2= open("foo1.txt", O_RDONLY|O_CREAT);
write(1, "write syscall testing\n", 14);
int fd1 = access("/home/cop4610-18/fools.txt", 1);
close(fd);
close(fd2);
return 0;
}












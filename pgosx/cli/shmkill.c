#include <stdlib.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <semaphore.h>


int main (int argc, char * argv[])
{
	if (argc != 2)
		return (EINVAL);
		
	char *shmname = argv[1];
	
	int errshm = 0, errsem = 0;
	if (-1 == shm_unlink(shmname))
        errshm = errno;
        
    if (-1 == sem_unlink(shmname))
        errsem = errno;
    
    return (errshm ? errshm : errsem);
}
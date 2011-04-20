/*
* Copyright 2006,2007 Brian Bergstrand.
*
* Redistribution and use in source and binary forms, with or without modification, 
* are permitted provided that the following conditions are met:
*
* 1.	Redistributions of source code must retain the above copyright notice, this list of
*     conditions and the following disclaimer.
* 2.	Redistributions in binary form must reproduce the above copyright notice, this list of
*     conditions and the following disclaimer in the documentation and/or other materials provided
*     with the distribution.
* 3.	The name of the author may not be used to endorse or promote products derived from this
*     software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
* AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
* THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/
/*
cc -Wall -g -Os -mdynamic-no-pic -arch i386 -arch x86_64 -arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk -o bhash bhash.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <CommonCrypto/CommonDigest.h>

#define MAX_DIGEST_CHARS 64
struct bhash {    
	union {
		CC_SHA512_CTX sha512;
		CC_SHA1_CTX sha;
		CC_MD5_CTX md5;
	} ctx;
	
	int digest_len;
	unsigned char digest[MAX_DIGEST_CHARS];
	
	int (*init)(void *c);
	int (*update)(void *c, const void *data, unsigned long len);
	int (*final)(unsigned char *md, void *c);
};

#define NUM_HASH_TYPES 4
#define UNKNOWN_TYPE -1
#define SHA512_TYPE 0
#define SHA256_TYPE 1
#define SHA1_TYPE 2
#define MD5_TYPE 3
static const char* hashTypes[] = {
	"SHA512",
	"SHA256",
	"SHA1",
	"MD5",
	NULL
};
// XXX should generate the list dynamically
#define digestnames() "sha512 sha256 sha1 md5"


static int findHashType(const char *s)
{
	int i;
	for (i=0; i< NUM_HASH_TYPES; ++i) {
		if (0 == strcasecmp(hashTypes[i], s))
			return (i);
	}
	return (UNKNOWN_TYPE);
}


static
int hash(const char *file, struct bhash *h, void *ctx)
{
	struct stat sb;
	int fd;
	
	bzero(h->digest, sizeof(h->digest));
	
	if (0 == strcmp(file, "-")) {
		fd = dup(fileno(stdin));
		sb.st_blksize = 1024;
	} else if (0 != stat(file, &sb)) {
        return (errno);
    } else
    	fd = open(file, O_RDONLY,  0);
    	
    if (fd > -1) {
        (void)fcntl(fd, F_NOCACHE, 1);
        (void)fcntl(fd, F_RDAHEAD, 1);
        char *buf = malloc(sb.st_blksize);
        if (buf) {
            (void)h->init(ctx);
            
            int bytes;
            while ((bytes = read(fd, buf, sb.st_blksize)) > 0)
                (void)h->update(ctx, buf, bytes);
            
            (void)h->final(h->digest, ctx);
            
            free(buf);
        }
        close(fd);
        return (0);
    }
    
    return (errno);
}

void hashString(const struct bhash *h, char **string)
{
	char *digestString = malloc(sizeof(h->digest) * 4);
	int i, j;
	for (i = j = 0; i < h->digest_len; ++i)
		j += sprintf((digestString + j), "%02x", h->digest[i]);
	*(digestString + j) = 0;
	*string = digestString;
}

int initHash (struct bhash *h, void **ctx, int type)
{
	if (SHA512_TYPE == type) {
        *ctx = &h->ctx.sha512;
        h->digest_len = CC_SHA512_DIGEST_LENGTH;
        h->init = (typeof(h->init))CC_SHA512_Init;
        h->update = (typeof(h->update))CC_SHA512_Update;
        h->final = (typeof(h->final))CC_SHA512_Final;
    } else if (SHA256_TYPE == type) {
        *ctx = &h->ctx.sha512;
        h->digest_len = CC_SHA256_DIGEST_LENGTH;
        h->init = (typeof(h->init))CC_SHA256_Init;
        h->update = (typeof(h->update))CC_SHA256_Update;
        h->final = (typeof(h->final))CC_SHA256_Final;
    } else if (SHA1_TYPE == type) {
        *ctx = &h->ctx.sha;
        h->digest_len = CC_SHA1_DIGEST_LENGTH;
        h->init = (typeof(h->init))CC_SHA1_Init;
        h->update = (typeof(h->update))CC_SHA1_Update;
        h->final = (typeof(h->final))CC_SHA1_Final;
    } else if (MD5_TYPE == type) {
        *ctx = &h->ctx.md5;
        h->digest_len = CC_MD5_DIGEST_LENGTH;
        h->init = (typeof(h->init))CC_MD5_Init;
        h->update = (typeof(h->update))CC_MD5_Update;
        h->final = (typeof(h->final))CC_MD5_Final;
    } else
    	return (EINVAL);
    
    return (0);
}

int main (int argc, char *argv[]) {
	int err, opt, cmp = 0;
	int types[NUM_HASH_TYPES];
	char hashCompareString[MAX_DIGEST_CHARS * 4] = "";
	const char *cmd = strrchr(argv[0], '/');
	cmd = cmd ? cmd + 1 : argv[0];
	#define pusage() printf("usage: %s [-c f | hash string] -a | -t type files ...\n", cmd)
	
	memset(types, UNKNOWN_TYPE, sizeof(types));
	while (-1 != (opt = getopt(argc, argv, "ac:ht:"))) {
        switch (opt) {
            case 'a':
            	types[SHA512_TYPE] = SHA512_TYPE;
            	types[SHA256_TYPE] = SHA256_TYPE;
            	types[SHA1_TYPE] = SHA1_TYPE;
            	types[MD5_TYPE] = MD5_TYPE;
            break;

            case 'c':
            	cmp = 1;
            	if (strcmp("f", optarg) != 0) // "f" means compare the file args to each other
            		(void)strlcpy(hashCompareString, optarg, sizeof(hashCompareString)); // otherwise, compare the file args to the given hash string
            break;
            
            case 't':
            	if ((opt = findHashType(optarg)) > -1) {
            		types[opt] = opt;
            	} else {
            		fprintf(stderr, "Unknown digest type: '%s'\n", optarg);
            		exit(EINVAL);
            	}
            break;
            
            case 'h':
            	pusage();
            	printf(
            	"\n-a: hash input files using all known digests (%s)"
            	"\n-c: requires 'f' or 'hash string', compare the input files to each other ('f') or to the given hash string"
            	"\n-t: digest type (%s), may be repeated to generate multiple digests"
            	"\nfiles: list of input files, use '-' to specify stdin"
            	"\n"
            	, digestnames(), digestnames());
            	exit(0);
            break;
            
            case '?':
                pusage();
                exit(EINVAL);
            break;
        }
    }
    
    if (optind >= argc) {
    	errno = EINVAL;
    	pusage();
        perror("no input files");
        exit(EINVAL);
    }
    
    int numFiles = argc - optind;
    int i;
    struct bhash h;
    void *ctx = NULL;
    char *hs;
    
    int numTypes = 0;
    for (i = 0; i < NUM_HASH_TYPES; ++i) {
        if (UNKNOWN_TYPE != types[i])
            ++numTypes;
    }
    if (numTypes < 1) {
        errno = EINVAL;
        pusage();
        perror("missing digest type");
        exit(EINVAL);
    }
    
    char **fileHashes, **filePaths;
    if (cmp) {
    	if (numTypes > 1) {
    		errno = EINVAL;
    		perror("only one digest type can be specified when making a comparison ('-c')");
    		exit(EINVAL);
    	}
    	
    	if (0 == strlen(hashCompareString) && numFiles <= 1) {
    		errno = EINVAL;
    		perror("a file comparison requires two or more file paths");
    		exit(EINVAL);
    	}
    	
    	if (!(filePaths = (char**)calloc(numFiles + 1, sizeof(char*)))) {
    		errno = ENOMEM;
    		perror("no memory");
    		exit(ENOMEM);
    	}
    	
    	if (!(fileHashes = (char**)calloc(numFiles + 1, sizeof(char*)))) {
    		errno = ENOMEM;
    		perror("no memory");
    		exit(ENOMEM);
    	}
    } else {
    	fileHashes = filePaths = NULL;
    }
    
    // Generate hashes
    for (i=0; i < numFiles; ++i) {
    	char *f = argv[optind+i];
    	int j;
    	for (j = 0; j < NUM_HASH_TYPES; ++j) {
    		if (UNKNOWN_TYPE != types[j]) {
    			(void)initHash(&h, &ctx, types[j]);
    			if (0 == (err = hash(f, &h, ctx))) {
    				hashString(&h, &hs);
    				if (0 == strcmp(f, "-"))
    					f = "stdin";
    				printf("%s(%s)= %s\n", hashTypes[types[j]], f, hs);
    				if (cmp) {
    					filePaths[i] = f;
    					fileHashes[i] = hs;
    				}
    				#ifdef noleaks // leak hs, but we will exit very soon, so no big deal
    				else
    					free(hs);
    				#endif
    			} else
    				fprintf(stderr, "Failed %s(%s): %d.\n", hashTypes[types[j]], f, err);
    		}
    	}
    	printf("\n");
    }
    
    if (cmp) {
    	if (strlen(hashCompareString) == 0) {
    		// XXX - this needs to be enhanced to cover the case with > 2 files where the 1st file may not match but the others do
    		for (i=0; i < numFiles && !fileHashes[i]; ++i) ; // count loop only
    		if (fileHashes[i]) {
    			(void)strlcpy(hashCompareString, fileHashes[i], sizeof(hashCompareString));
    			printf("Using (%s) for comparison hash source\n", filePaths[i]);
    		} else
    			return (0);
    	}
    	
    	for (i=0; i < numFiles; ++i) {
			if (fileHashes[i] && 0 != strcasecmp(hashCompareString, fileHashes[i]))
				fprintf(stderr, "(%s) does not match comparison hash!\n", filePaths[i]);
		}
    	
		#ifdef noleaks
		for (i=0; i < numFiles; ++i) {
			if (fileHashes[i])
				free(fileHashes[i]);
		}
		free(fileHashes);
		free(filePaths); // contained paths point into argv[] so don't free them
		#endif
    }
    
    return (0);
}

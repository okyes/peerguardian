/*
    Copyright 2006 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

// Build: gcc -I pplib -I p2b pplib/pplib.c p2b/p2b.c pgmerge.c -o pgmerge

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/errno.h>

#define PG_MAKE_TABLE_ID
#include "p2b.h"
#include "pplib.h"

static const char whatid[] __attribute__ ((used)) =
"@(#)$LastChangedRevision: 364 $ Built: " __DATE__ ", " __TIME__;

int main(int argc, char* argv[])
{
    int err, opt;
    char *outfile = NULL;
    int version = -1;
    
    char *progname = strrchr(argv[0], '/');
    if (progname)
        progname++;
    else
        progname = argv[0];
    
    while (-1 != (opt = getopt(argc, argv, "ho:v:"))) {
        switch (opt) {
            case 'o':
                outfile = optarg;
            break;

            case 'v':
                version = strtol(optarg, NULL, 0);
                if (0 == version && errno != 0) {
                    perror("invalid format version");
                    exit(errno);
                }
            break;
            
            case 'h':
    print_help:
                printf("%s -o output_file -v format_version input_file [...]\n"
                "\t-o The output file. This file will be overwritten if it exists.\n"
                "\t-v The format of the output file: 0 for p2p (text) and 2 for binary (p2b v2) are allowed.\n"
                "\t-h This help.\n"
                , progname);
                return (0);
            break;
            
            case '?':
                goto print_help;
            break;
        }
    }
    
    if (!outfile || version < 0) {
        errno = EINVAL;
        perror("missing arguments");
        goto print_help;
    }
    
    if (optind >= argc) {
        errno = EINVAL;
        perror("no input files");
        exit(errno);
    }
    
    int numFiles = argc - optind; 
    char **files = malloc(sizeof(char*) * (numFiles+1));
    files[numFiles] = NULL;
    
    int i;
    for (i=0; i < numFiles; ++i)
        files[i] = argv[optind+i];
    
    if ((err = p2b_merge(outfile, version, files)))
        fprintf(stderr, "%s: merge failed with %d\n", progname, err);
    
    free(files);
    
    return (err);
}

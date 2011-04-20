/*
    Copyright 2005,2006 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/*!
@header p2b library
Provides common operations on PeerGuardian list files, including read, write, merge and compare.
@copyright Brian Bergstrand.
*/
#include <stdlib.h>
#include <sys/types.h>

#include "ppmsg.h"

#ifndef PP_EXTERN
#ifdef __APPLE__
#ifdef PP_SHARED
#define PP_EXTERN extern
#else
#define PP_EXTERN __private_extern__
#endif
#else
#define PP_EXTERN
#endif // __APPLE__
#endif

/*!
@define P2P_VERSION
@abstract P2P (text) list of the form: "description:range_start-range_end\n".
*/
#define P2P_VERSION  0x00
/*!
@define P2B_VERSION2
@abstract Binary list version 2.
*/
#define P2B_VERSION2 0x02
/*!
@define P2B_VERSION3
@abstract Binary list version 3.
*/
#define P2B_VERSION3 0x03

/*!
@function p2b_parse
@abstract Parse a p2p (text) or p2b (v2 or v3) file into a sorted array of ranges and an array of utf8 names.
@discussion
@param path The full path to the input file.
@param ranges Pointer to an array of address ranges. Caller must free on successful return.
@param rangcount The number of ranges in the returned range array.
@param addrcount The total count of addresses covered by all ranges. Optional.
@param names Pointer to an array of UTF8 names. Caller must free on successful return.
@result 0 if successful, or a non-0 POSIX error. Returns ERANGE if the file could not be parsed, or for a valid but empty file.
*/
PP_EXTERN
int p2b_parse(const char *path, struct pp_msg_iprange **ranges, int32_t *rangecount,
    u_int32_t *addrcount, u_int8_t ***names);

/*!
@function p2b_parseports
@abstract Parse a list of ports into a sorted array of ranges.
@discussion This is useful for doing port based blocking.
@param portRule A string containg a list of ports. Each port or port range must be separated by a ','. Ranges are deleimted by a '-'. THIS STRING WILL BE MODIFIED!
@param portRuleLen The length of portRule.
@param ranges Pointer to an array of port ranges. Caller must free on successful return.
@param rangcount The number of ranges in the returned range array.
@result 0 if successful, or a non-0 POSIX error.
*/
int p2b_parseports(char *portRule, int portRuleLen, pp_msg_iprange_t **ranges, int32_t *rangecount);

/*!
@function p2b_compare
@abstract Compare two list files for equality.
@discussion
@param path1 The full path to the first input file.
@param path2 The full path to the second input file.
@result 0 if the files are equal, -1 if they are not equal or a >0 POSIX error.
*/
PP_EXTERN
int p2b_compare(const char *path1, const char *path2);

/*!
@function p2b_exchange
@abstract Copies one list file into another.
@discussion An atomic copy to target is attempted, but not guaranteed. The source file is not modified.
@param source The full path to the source list file.
@param target The full path to the target list file. The target is created if necessary, otherwise it's contents are overwritten.
@result 0 if successful, or a non-0 POSIX error.
*/
PP_EXTERN
int p2b_exchange(const char *source, const char *target);

/*!
@function p2b_merge
@abstract Merge n list files into a new output file.
@discussion Currently only P2P and P2B(v2) are supported.
@param outfile The full path to the file to be created or overwritten.
@param version The list format version.
@param files A NULL terminated array of full paths to the input files.
@result 0 if successful, or a non-0 POSIX error.
*/
PP_EXTERN
int p2b_merge(const char *outfile, int version, char* const *files);

/*!
@functiongroup Non-batch editing functions
*/

/*!
@typedef p2b_handle_t
@abstract An opaque handle to a list file that can be used for non-batch editing.
@discussion Internally, handles may not be thread safe, so don't share them between threads.
*/
typedef struct p2b_handle_ *p2b_handle_t;

/*!
@function p2b_open
@abstract Open or create a list file.
@discussion Currently only P2P and P2B(v2) are supported.
@param path The full path the list file to open.
@param version The list format version. Used during creation only.
@param handle A handle to the newly opened file.
@result 0 if successful, or a non-0 POSIX error.
*/
PP_EXTERN
int p2b_open(const char *path, int version, p2b_handle_t *handle);

/*!
@function p2b_close
@abstract Close a list file opened with p2b_open.
@discussion
@param handle The file handle to close.
@result 0 if successful, or a non-0 POSIX error.
*/
PP_EXTERN
int p2b_close(p2b_handle_t handle);

/*!
@function p2b_add
@abstract Add a range to a list file.
@discussion
@param handle The file handle to operate on.
@param name A UTF8 description for the new range.
@param range The range to add.
@result 0 if successful, or a non-0 POSIX error.
*/
PP_EXTERN
int p2b_add(p2b_handle_t handle, const char *name, const struct pp_msg_iprange *range);

/*!
@function p2b_remove
@abstract Remove a range from a list file.
@discussion
@param handle The file handle to operate on.
@param name A UTF8 description for the new range. Currently ignored.
@param range The range to remove.
@result 0 if successful, or a non-0 POSIX error.
*/
PP_EXTERN
int p2b_remove(p2b_handle_t, const char *name, const struct pp_msg_iprange *range);

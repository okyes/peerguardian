/*
    Copyright 2005 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

static
int ppfilter_ftp(const char *pkt, size_t len, in_addr_t *ip4, in_port_t *port)
{
    if (21 == *port) {
        char tmp[64]; // the pkt is likely not NULL termintated
        if (len > (sizeof(tmp)-1))
            len = (sizeof(tmp)-1);
        bcopy(pkt, tmp, len);
        tmp[len] = 0;
        pkt = tmp;
        
        int status = strtol(pkt, NULL, 0);
        if (227 == status) { // PASV mode entered response
            pkt = strchr(pkt, '(');
            if (pkt) {
                pkt++; // pkt now points to the beginning of the address
                
                in_addr_t addr = 0;
                in_port_t myport = 0;
                addr |= ((unsigned char)strtol(pkt, NULL, 0) << 24);
                pkt = strchr(pkt, ',');
                if (pkt) {
                    pkt++;
                    addr |= ((unsigned char)strtol(pkt, NULL, 0) << 16);
                    pkt = strchr(pkt, ',');
                }
                if (pkt) {
                    pkt++;
                    addr |= ((unsigned char)strtol(pkt, NULL, 0) << 8);
                    pkt = strchr(pkt, ',');
                }
                if (pkt) {
                    pkt++;
                    addr |= (unsigned char)strtol(pkt, NULL, 0);
                    pkt = strchr(pkt, ',');
                }
                if (pkt) {
                    pkt++;
                    myport |= ((unsigned char)strtol(pkt, NULL, 0) << 8);
                    pkt = strchr(pkt, ',');
                }
                if (pkt) {
                    pkt++;
                    myport |= (unsigned char)strtol(pkt, NULL, 0);
                    pkt = strchr(pkt, ')');
                }
                if (pkt) {
                    *ip4 = addr; // host order
                    *port = myport;
                }
            }
        }
    }
    
    return (0);
}

// ip4 and port are returned in host order, port must be in host order when called
// return 0 to allow, 1 to deny
typedef int(*pp_data_filter_t)(const char *pkt, size_t len, in_addr_t *ip4, in_port_t *port);
static pp_data_filter_t const pp_data_filters[] = {ppfilter_ftp, (pp_data_filter_t)NULL};

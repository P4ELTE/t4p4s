// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include "sock_helpers.h"
#include "messages.h"
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>

int read_fix(int sock, char* buffer, int nbytes)
{
	int rbytes = 0;
	int rval = 0;
	while (rbytes<nbytes)
	{
		rval = read(sock, buffer + rbytes, nbytes-rbytes);
		if (rval<=0) return rval;
		rbytes += rval;
	}

	return rbytes;
}

int write_fix(int sock, char* buffer, int nbytes)
{
        int wbytes = 0;
        int wval = 0;
        while (wbytes<nbytes)
        {
                wval = write(sock, buffer + wbytes, nbytes-wbytes);
                if (wval<0) return wval;
                wbytes += wval;
        }

        return wbytes;
}

int read_p4_msg(int sock, char* buffer, int length)
{
	int rval;
	int msglen;
	if (length<sizeof(struct p4_header)) return -1;

	if ((rval=read_fix(sock, buffer, sizeof(struct p4_header)))<=0)
		return rval;
	
	msglen = ntohs(((struct p4_header*)buffer)->length);

	if (msglen>length)
		return -100;

	if (msglen == sizeof(struct p4_header))
		return msglen;

	if ((rval=read_fix(sock, buffer + sizeof(struct p4_header), msglen-sizeof(struct p4_header)))<=0)
		return rval;
	return msglen;
}

int write_p4_msg(int sock, char* buffer, int length)
{
	if (length<sizeof(struct p4_header)) {
		printf("Error: tried to send a message of length %d, which is less than the length of a P4 header (%lu)\n", length, sizeof(struct p4_header));
		return -1;
	}

	int msglen = ntohs(((struct p4_header*)buffer)->length);

	if (msglen>length) return -1;

	return write_fix(sock, buffer, msglen);
}


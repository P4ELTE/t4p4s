// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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

	if ((rval=read_fix(sock, buffer + sizeof(struct p4_header), msglen-sizeof(struct p4_header)))<=0)
		return rval;
	return msglen;
}

int write_p4_msg(int sock, char* buffer, int length)
{
        int msglen;
        if (length<sizeof(struct p4_header)) return -1;

	msglen = ntohs(((struct p4_header*)buffer)->length);

	if (msglen>length) return -1;

	return write_fix(sock, buffer, msglen);
}


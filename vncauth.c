/**
 * vnc2rdp: proxy for RDP client connect to VNC server
 *
 * Copyright 2014 Yiwei Li <leeyiw@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>

#include "d3des.h"
#include "vncauth.h"

/*
 * Encrypt CHALLENGESIZE bytes in memory using a password.
 */
void
rfbEncryptBytes(unsigned char *bytes, const char *passwd)
{
	unsigned char key[8];
	unsigned int i;

	/* key is simply password padded with nulls */

	for (i = 0; i < 8; i++) {
		if (i < strlen(passwd)) {
			key[i] = passwd[i];
		} else {
			key[i] = 0;
		}
	}

	rfbDesKey(key, EN0);

	for (i = 0; i < CHALLENGESIZE; i += 8) {
		rfbDes(bytes + i, bytes + i);
	}
}

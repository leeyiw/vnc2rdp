#include <string.h>

#include "d3des.h"
#include "vncauth.h"

/*
 * Encrypt CHALLENGESIZE bytes in memory using a password.
 */
void
rfbEncryptBytes(unsigned char *bytes, char *passwd)
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

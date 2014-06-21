#ifndef _VNCAUTH_H_
#define _VNCAUTH_H_

#define CHALLENGESIZE	16

extern void rfbEncryptBytes(unsigned char *bytes, char *passwd);

#endif  // _VNCAUTH_H_

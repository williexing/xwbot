#ifndef _BASE64_H_
#define _BASE64_H_

EXPORT_SYMBOL char *x_base64_encode(char *str, int len);
EXPORT_SYMBOL char *bin2hex(char *buf, int len);
EXPORT_SYMBOL char *base64_decode(const char *str, int len, int *count);
EXPORT_SYMBOL void *hex2bin(const char *hex, int *len);

#endif /*_BASE64_H_*/

/* Copyright (c) 2010 the authors listed at the following URL, and/or
 the authors of referenced articles or incorporated external code:
 http://en.literateprograms.org/Base64_(C)?action=history&offset=20070707014726

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 Retrieved from: http://en.literateprograms.org/Base64_(C)?oldid=10650
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xwlib/x_lib.h>
#include <xwlib/x_types.h>

static const char const b64dict[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int nbytes[] =
  { 3, 1, 1, 2 };
static unsigned char out[4] =
  { 0, 0, 0, 0 };

static char *
xlate(unsigned char *in, int phase, int *bytes)
{
  out[0] = in[0] << 2 | in[1] >> 4;
  out[1] = in[1] << 4 | in[2] >> 2;
  out[2] = in[2] << 6 | in[3] >> 0;
  // out[nbytes[phase]] = '\0';
  *bytes = nbytes[phase];
  return (char *) out;
}


/**
 *
 */
char *
base64_decode(const char *str, int len, int *count)
{
  int c, phase;
  int i, j;
  unsigned char in[4];
  char *p;
  char *ret = NULL;
  int bytes = 0;
  char *buf;
  int total = 0;

  phase = 0;

  for (i = 0, j = 0; i < len; i++)
    {
      c = str[i];
      if (c == '=')
        {
          buf = xlate(in, phase, &bytes);
          ret = realloc(ret, total + bytes);
          memcpy(ret + total, buf, bytes);
          total += bytes;
          break;
        }
      p = strchr(b64dict, c);
      if (p)
        {
          in[phase] = p - b64dict;
          phase = (phase + 1) % 4;
          if (phase == 0)
            {
              buf = xlate(in, phase, &bytes);
              ret = realloc(ret, total + bytes);
              memcpy(ret + total, buf, bytes);
              total += bytes;

              in[0] = in[1] = in[2] = in[3] = 0;
            }
        }

    }

  ret = realloc(ret,total+1);
  ret[total] = '\0';

  if (count)
    *count = total;

  return ret;
}

/**
 *
 */
char *
x_base64_encode(char *str, int len)
{
  int l;
  int i, j, k;
  unsigned char in[3] =
    { 0, 0, 0 };
  char *ret;

  l = (len + 2 - ((len + 2) % 3)) / 3 * 4;
  l += 3;
  l &= ~3; // align to 4
  ret = (char *) x_malloc(l + 1);

  for (i = 0, j = 0, k = 0; i < len; i++)
    {
      in[j++] = str[i];

      /* flash next chunk of data */
      if (j == 3)
        {
          j = 0;
          ret[k++] = b64dict[(in[0] >> 2) & 0x3F];
          ret[k++] = b64dict[(in[0] << 4 | in[1] >> 4) & 0x3F];
          ret[k++] = b64dict[(in[1] << 2 | in[2] >> 6) & 0x3F];
          ret[k++] = b64dict[in[2] & 0x3F];
          /* prepare for next iteration */
          in[0] = in[1] = in[2] = 0;
        }
    }

  /* flash padding */
  if (j)
    {
      ret[k++] = b64dict[(in[0] >> 2) & 0x3F];
      ret[k++] = b64dict[(in[0] << 4 | in[1] >> 4) & 0x3F];
      if (j == 2)
        ret[k++] = b64dict[(in[1] << 2 | in[2] >> 6) & 0x3F];
      else
        ret[k++] = '=';
      ret[k++] = '=';
    }

  /* end of line */
  ret[k] = 0;

  return ret;
}

char *
bin2hex(char *buf, int len)
{
  int i;
  char *hex = (char *)x_malloc(len * 2 + 1);
  if (!hex)
    return hex;

  for (i = 0; i < len; ++i)
    x_snprintf(hex + i * 2, 3, "%02x", (char) buf[i]);

  return hex;
}

/**
 * Convert hex string to binary data
 */
void *
hex2bin(const char *hex, int *len)
{
  int c1, c2;
  int i;
  char *__restrict__ buf = NULL;
  int l = 0;

  if (!hex)
    return (void *) NULL;

  l = strlen(hex);
  if (l <= 0 || (l % 2))
    return (void *) NULL;

  l /= 2;

  buf = malloc(l);
  if (!buf)
    return (void *) NULL;

  for (i = 0; i < l; i++)
    {
      c1 = (char) hex[2 * i];
      c2 = (char) hex[2 * i + 1];

      /* convert */
      if (c1 >= 'a' && c1 <= 'f')
        c1 = 10 + (c1 - 'a');
      else if (c1 >= 'A' && c1 <= 'F')
        c1 = 10 + (c1 - 'A');
      else if (c1 >= '0' && c1 <= '9')
        c1 = (c1 - '0');
      else
        {
          free(buf);
          buf = NULL;
          break;
        }
      /* same for c2 */
      if (c2 >= 'a' && c2 <= 'f')
        c2 = 10 + (c2 - 'a');
      else if (c2 >= 'A' && c2 <= 'F')
        c2 = 10 + (c2 - 'A');
      else if (c2 >= '0' && c2 <= '9')
        c2 = (c2 - '0');
      else
        {
          free(buf);
          buf = NULL;
          break;
        }
      buf[i] = (char) (c1 * 16 + c2);
    }

  /* set converted length */
  if (len)
    *len = l;
  return (void *) buf;
}

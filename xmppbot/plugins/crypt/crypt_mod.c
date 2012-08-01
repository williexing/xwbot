#include <xwlib/x_lib.h>
#include <xwlib/x_types.h>

extern void chksum_crc32gentab(void);

__x_plugin_visibility __plugin_init void
crypt_init(void)
{
  chksum_crc32gentab();
  return;
}

PLUGIN_INIT(crypt_init);


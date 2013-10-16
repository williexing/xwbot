#include "__init.h"

void x_gobee_init(void)
{
  procsys_init();
auth_plain_register();
bind_init();
stream_features_init();
iq_init();
message_init();
body_init();
presence_init();
sasl_features_init();
xmppstream_init();
crypt_init();
disco_proto_init();
__evtdomain_init();
  x_stunserver_init();
jingle_init();
//__evtdomain_init();
__icectl_init();
__payloadctl_init();
__isession_init();
__iostream_init();
xbus_lib_init();
xmppparser_init();
  x_vpx_init();
  x_theora_plugin_init();
  x_hid_codec_init();
//  x_ilbc_init();
  x_speeex_init();
}

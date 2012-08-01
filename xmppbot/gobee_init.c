#include "__init.h"

void x_gobee_init(void)
{
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
j_payload_type_init();
j_description_init();
j_candidate_init();
j_transport_init();
jingle_init();
pubapi_init();
__evtdomain_init();
__icectl_init();
__payloadctl_init();
__isession_init();
__iostream_init();
__p2p_info_init();
xbus_lib_init();
xmppparser_init();
x_vpx_init();
}

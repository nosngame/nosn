#include <nsbase.h>
#include <protocol.h>

namespace NSGateProto
{
	CNSMap< TProtocolID, CNSProtocol* >	CProtocolTunnel::sProtoStubs;
}

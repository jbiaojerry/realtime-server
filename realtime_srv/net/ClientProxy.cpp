#include "realtime_srv/common/RealtimeSrvShared.h"


using namespace realtime_srv;

#ifdef IS_LINUX

using namespace muduo::net;

ClientProxy::ClientProxy( NetworkMgr* inNetworkManager,
	const std::string& inPlayerName,
	int inPlayerId,
	const UdpConnectionPtr& inUdpConnection )
	:
	networkManager_( inNetworkManager ),
	ReplicationManager_( this ),
	mPlayerName( inPlayerName ),
	mPlayerId( inPlayerId ),
	DeliveryNotifyManager_( false, true ),
	mIsLastMoveTimestampDirty( false ),
	mLastPacketFromClientTime( 0.f ),
	mTimeToRespawn( 0.f ),
	mRecvingServerResetFlag( false ),
	UdpConnection_( inUdpConnection )
{
	UpdateLastPacketTime();
}

#else //IS_LINUX

ClientProxy::ClientProxy( NetworkMgr* inNetworkManager,
	const SockAddrInterf& inSocketAddress,
	const std::string& inPlayerName,
	int inPlayerId,
	const UDPSocketPtr& inUDPSocket )
	:
	networkManager_( inNetworkManager ),
	ReplicationManager_( this ),
	mSocketAddress( inSocketAddress ),
	mPlayerName( inPlayerName ),
	mPlayerId( inPlayerId ),
	mUDPSocket( inUDPSocket ),
	DeliveryNotifyManager_( false, true ),
	mIsLastMoveTimestampDirty( false ),
	mTimeToRespawn( 0.f ),
	mRecvingServerResetFlag( false )
{
	UpdateLastPacketTime();
}
#endif //IS_LINUX


void ClientProxy::UpdateLastPacketTime()
{
	mLastPacketFromClientTime = RealtimeSrvTiming::sInstance.GetCurrentGameTime();
}

void ClientProxy::SetGameObjStateDirty( int inNetworkId, uint32_t inDirtyState )
{
	networkManager_->SetRepStateDirty( inNetworkId, inDirtyState );
}
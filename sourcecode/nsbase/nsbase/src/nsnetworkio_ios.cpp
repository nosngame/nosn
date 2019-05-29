#include <nsbase.h>
namespace NSNet
{
	extern CNSMap< CNSString, CNSNetworkIO* >	networkIOs;
	// ********************************************************************** //
	// CNSNetworkIO
	// ********************************************************************** //
	CNSNetworkIO::CNSNetworkIO(const CNSString& name) : mName(name)
	{
	}

	// ********************************************************************** //
	// CNSActiveIO
	// ********************************************************************** //
	CNSActiveIO::CNSActiveIO(CNSNetManager* pManager, const CNSString& name) : CNSNetworkIO(name), mpSession(NULL), mpManager(pManager), mSocket(0), mSendReady(false)
	{
	}

	CNSActiveIO::~CNSActiveIO()
	{
        ::close(mSocket);
		delete mpSession;
		mSocket = 0;
	}

	void CNSActiveIO::destroy(TSessionID sessionID)
	{
		if (mpSession == NULL)
			return;

		mIsDeleted = true;
		mpManager->onDelSession(mName, mpSession->mSessionID);

		// 如果NetworkIO对象没有发生变化，说明没有重连，那么需要清除Manager中保存的指针
		if (mpManager->getNetworkIO() == this)
			mpManager->setNetworkIO(NULL);
	}

	void CNSActiveIO::create(const CNSString& address, const CNSString& port)
	{
		// 注册一个主动IO
		if ((mSocket = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
			NSException(_UTF8("[create]"));

		unsigned long value = 1;
		::ioctl(mSocket, FIONBIO, &value);

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(address.getBuffer());
		addr.sin_port = htons(port.toInteger());
		if (::connect(mSocket, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1)
		{
			if (errno == EINPROGRESS)
				return;

			NSException(_UTF8("[connect]"));
		}
	}

	void CNSActiveIO::onConnect(int code)
	{
		if (code != 0)
		{
			mIsDeleted = true;
			mpManager->onAddSessionFault(mName, code);

			if (mpManager->getNetworkIO() == this)
				mpManager->setNetworkIO(NULL);
			return;
		}

		mpSession = new CNSSession(mpManager, mSocket, this);
		unsigned int bufferSize = mpManager->getBufferSize();
		int tRet = setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, (char*)&bufferSize, sizeof(unsigned int));

		bufferSize = mpManager->getBufferSize();
		tRet = setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof(unsigned int));

		bool tTcpDelay = true;
		setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&tTcpDelay, sizeof(bool));

		sockaddr_in localAddr, peerAddr;
		socklen_t tLocalLen = sizeof(sockaddr_in);
		socklen_t tPeerLen = sizeof(sockaddr_in);
		if (::getsockname(mSocket, (sockaddr*)&localAddr, &tLocalLen) == -1)
			NSException(_UTF8("[getsockname]"));

		if (::getsockname(mSocket, (sockaddr*)&peerAddr, &tPeerLen) == -1)
			NSException(_UTF8("[getsockname]"));

		CNSSockAddr peer(peerAddr);
		mpSession->mSessionDesc.mPeer.format("%s:%d", peer.GetIPString().getBuffer(), peer.GetPort());
		mpManager->onAddSession(mName.getBuffer(), 0, CNSSockAddr(localAddr), peer);
	}

	void CNSActiveIO::pollEvent(unsigned int tickCount)
	{
		if (tickCount - mPollTick < mpManager->mPollInterval)
			return;

		mPollTick = tickCount;
		struct timeval timeout = { 0, 0 };
		fd_set	mReadSet;
		fd_set	mWriteSet;
		FD_ZERO(&mReadSet);
		FD_ZERO(&mWriteSet);
		FD_SET(mSocket, &mReadSet);
		FD_SET(mSocket, &mWriteSet);
		int tRet = select(mSocket + 1, &mReadSet, &mWriteSet, NULL, &timeout);
		if (tRet > 0)
		{
			if (FD_ISSET(mSocket, &mWriteSet) != 0 && FD_ISSET(mSocket, &mReadSet) != 0)
			{
				if (mpSession == NULL)
				{
					onConnect(-1);
					return;
				}
			}

			if (FD_ISSET(mSocket, &mWriteSet) != 0)
			{
				if (mpSession == NULL)
					onConnect(0);
				else
					onSend();
			}
			if (FD_ISSET(mSocket, &mReadSet) != 0)
			{
				onRecv();
			}
		}

		return;
	}

	int CNSActiveIO::sendHelper()
	{
		int bytesSend = 0;
		do
		{
			char* buffer = (char*)mpSession->mOBuffer.begin();
			int length = mpSession->mOBuffer.length();
			if (length == 0)
				return ERESULT_SUCCESS;

			bytesSend = ::send(mpSession->mPeerSocket, buffer, length, 0);
			if (bytesSend == -1)
			{
				switch (errno)
				{
				case EAGAIN:
					return ERESULT_SUCCESS;
				default:
					destroy(0);
					return ERESULT_CONNECTLOST;
				}
			}
		} while (mpSession->onSessionSend(bytesSend) == true);

		return ERESULT_SUCCESS;
	}

	CNSSessionDesc* CNSActiveIO::getDesc(unsigned int sessionID)
	{
		if (mpSession == NULL)
			return NULL;

		return &mpSession->mSessionDesc;
	}


	int CNSActiveIO::send(TSessionID sessionID, const CNSOctets& buffer)
	{
		if (mpSession == NULL)
			return NSNet::ERESULT_SESSIONLOST;

		if (mpSession->send(buffer) == true)
			return sendHelper();

		return NSNet::ERESULT_SUCCESS;
	}

	int CNSActiveIO::send(TSessionID sessionID, const void* begin, const void* end)
	{
		if (mpSession == NULL)
			return NSNet::ERESULT_SESSIONLOST;

		if (mpSession->send(begin, end) == true)
			return sendHelper();

		return NSNet::ERESULT_SUCCESS;
	}

	int CNSActiveIO::send(TSessionID sessionID, const void* begin, size_t size)
	{
		if (mpSession == NULL)
			return NSNet::ERESULT_SESSIONLOST;

		if (mpSession->send(begin, size) == true)
			return sendHelper();

		return NSNet::ERESULT_SUCCESS;
	}

	void CNSActiveIO::onSend()
	{
		if (mpSession == NULL)
			return;

		sendHelper();
	}

	void CNSActiveIO::onRecv()
	{
		if (mpSession == NULL)
			return;

		char* buffer = (char*)mpSession->mIBuffer.end();
		int	length = mpSession->mIBuffer.size() - mpSession->mIBuffer.length();

		int bytesRecv = (int) recv(mpSession->mPeerSocket, buffer, length, 0);
		if (bytesRecv == 0 || bytesRecv == -1)
		{
			// 如果收到0字节，如果接受失败, 都要断开连接
			destroy(0);
			return;
		}

		// 如果解包失败，也要断开连接
		if (mpSession->onSessionRecv(bytesRecv) == false)
			destroy(0);
	}
}


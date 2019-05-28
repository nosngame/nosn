#include <fbbase.h>
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
	CNSActiveIO::CNSActiveIO(CFBNetManager* pManager, const CFBString& name) : CNSNetworkIO(name), mpSession(NULL), mpManager(pManager), mSocket(0), mSendReady(false)
	{
	}

	CNSActiveIO::~CNSActiveIO()
	{
		// ��Ϊ�Ͽ����Ӻ�����ʧ�ܶ���Ҫ�ر�socket
		// ���Թر�socketҪ��������
		close(mSocket);
		delete mpSession;
		mSocket = 0;
	}

	void CNSActiveIO::Destroy(TSessionID sessionID)
	{
		if (mpSession == NULL)
			return;

		mIsDeleted = true;
		mpManager->onDelSession(mName, mpSession->mSessionID);

		// ���NetworkIO����û�з����仯��˵��û����������ô��Ҫ���Manager�б����ָ��
		if (mpManager->getNetworkIO() == this)
			mpManager->setNetworkIO(NULL);
	}

	void CNSActiveIO::Create(const CNSString& address, const CNSString& port)
	{
		// ע��һ������IO
		if ((mSocket = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
			FBExceptionEx(errno);

		unsigned long value = 1;
		::ioctl(mSocket, FIONBIO, &value);

		sockaddr_in tAddr;
		tAddr.sin_family = AF_INET;
		tAddr.sin_addr.s_addr = inet_addr(address.GetBuffer());
		tAddr.sin_port = htons(port.ToInteger());
		if (::connect(mSocket, (sockaddr*)&tAddr, sizeof(sockaddr_in)) == -1)
		{
			if (errno == EINPROGRESS)
				return;

			NSException(_UTF8("����[connect]��������"));
		}
	}

	void CNSActiveIO::OnConnect(int vCode)
	{
		if (vCode != 0)
		{
			mIsDeleted = true;
			mpManager->onAddSessionFault(mName, vCode);

			// ���NetworkIO����û�з����仯��˵��û����������ô��Ҫ���Manager�б����ָ��
			if (mpManager->getNetworkIO() == this)
				mpManager->setNetworkIO(NULL);
			return;
		}

		// ������ӳɹ�
		mpSession = new CFBSession(mpManager, mSocket, this);
		unsigned int bufferSize = mpManager->GetBufferSize();
		int tRet = setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, (char*)&bufferSize, sizeof(unsigned int));

		bufferSize = mpManager->GetBufferSize();
		tRet = setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof(unsigned int));

		bool tTcpDelay = true;
		setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&tTcpDelay, sizeof(bool));

		sockaddr_in localAddr, peerAddr;
		socklen_t tLocalLen = sizeof(sockaddr_in);
		socklen_t tPeerLen = sizeof(sockaddr_in);
		if (::getsockname(mSocket, (sockaddr*)&localAddr, &tLocalLen) == -1)
			NSException(_UTF8("����[getsockname]��������"));

		if (::getsockname(mSocket, (sockaddr*)&peerAddr, &tPeerLen) == -1)
			NSException(_UTF8("����[getsockname]��������"));

		CFBSockAddr peer(peerAddr);
		mpSession->mSessionDesc.mPeer.Format("%s:%d", peer.GetIPString().GetBuffer(), peer.GetPort());
		mpManager->onAddSession(mName.GetBuffer(), CFBSockAddr(tLocalAddr), tPeer, 0);
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
					OnConnect(-1);
					return;
				}
			}

			if (FD_ISSET(mSocket, &mWriteSet) != 0)
			{
				if (mpSession == NULL)
					OnConnect(0);
				else
					OnSend();
			}
			if (FD_ISSET(mSocket, &mReadSet) != 0)
			{
				OnRecv();
			}
		}

		return;
	}

	int CNSActiveIO::sendHelper()
	{
		int bytesSend = 0;
		do
		{
			char* buffer = (char*)mpSession->mOBuffer.Begin();
			int length = mpSession->mOBuffer.Length();
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

		char* buffer = (char*)mpSession->mIBuffer.End();
		int	length = mpSession->mIBuffer.Size() - mpSession->mIBuffer.Length();

		int bytesRecv = recv(mpSession->mPeerSocket, buffer, length, 0);
		if (bytesRecv == 0 || bytesRecv == -1)
		{
			// ����յ�0�ֽڣ��������ʧ��, ��Ҫ�Ͽ�����
			destroy(0);
			return;
		}

		// ������ʧ�ܣ�ҲҪ�Ͽ�����
		if (mpSession->onSessionRecv(bytesRecv) == false)
			destroy(0);
	}
}


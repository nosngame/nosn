#pragma once
namespace NSNet
{
	class CNSSecurity
	{
	public:
		void Update( const CNSOctets& rOctets )
		{
		}
	};

	class CNSSessionDesc
	{
	public:
		long long		mSendBytes;		// 持续发送字节数
		long long		mRecvBytes;		// 持续接受字节数
		unsigned int	mSBBytes;		// 发送缓冲大小
		unsigned int	mPings;			// 连接Ping值
		CNSString		mPeer;			// 连接方地址
	public:
		CNSSessionDesc( ) : mSendBytes( 0 ), mRecvBytes( 0 ), mSBBytes( 0 ), mPings( 0 )
		{
		}
	};

	class CNSSession
	{
	protected:
		static TSessionID sSessionID;
		TSessionID NextSessionID( ) { return sSessionID ++; }

		enum HTTPStatus
		{
			HTTP_DATA = 0,	// 等待HTTP数据
			HTTP_WAITDATAEND = 1,	// 等待HTTP数据结束
			HTTP_WAITRETURN = 2,	// 等待换行结束
			HTTP_WAITKEY = 3,	// 等待Header key
			HTTP_WAITVALUE_SPACE = 4,	// 等待Header key 和 value 的分割空格
			HTTP_WAITVALUE = 5,	// 等待Header value
			HTTP_WAITPOSTDATA = 6,	// 等待Post数据结束
			HTTP_END = 7		// Http协议处理结束
		};

	public:
		TSessionID			mSessionID;
		CNSOctets			mIBuffer;
		CNSOctets			mOBuffer;
		CNSOctetsStream		mIStream;
		CNSOctetsStream		mOStream;
		CNSNetManager*		mpManager;
		CNSSecurity*		mpISecurity;
		CNSSecurity*		mpOSecurity;
		CNSNetworkIO*		mpNetworkIO;
		SOCKET				mPeerSocket;
		CNSProtocol*		mLastProto;			// 目前正在处理的协议
		CNSSessionDesc		mSessionDesc;
		bool				mIsHttp;
		bool				mIsNotifyProgress;
		int					mHttpStatus;
		int					mPostDataSize;
		CNSString			mKey;
		CNSString			mValue;
		CNSString			mPostData;
		CNSString			mHttpData;
		CNSMap< CNSString, CNSString > mHeaders;

	public:
		CNSSession( CNSNetManager* pManager, SOCKET tPeerSocket, CNSNetworkIO* pPassiveIO );

	public:
		~CNSSession( );

	public:
		TSessionID getSessionID( ) { return mSessionID; }

	public:
		CNSProtocol* decode( CNSOctetsStream& rStream );

	public:
		bool sendReady( );
		bool send( const CNSOctets& buffer );
		bool send( const void* begin, const void* end );
		bool send( const void* begin, size_t len );
		bool onSessionSend( unsigned int vBytesTran );
		bool onSessionRecv( unsigned int vBytesTran );

	protected:
		bool httpInput( char ch );
	};

};

#pragma once
namespace NSNet
{
	enum ENET_ERRORCODE
	{
		ERESULT_SUCCESS = 0,	// 发送成功
		ERESULT_IONOTEXIST,			// 指定IO不存在
		ERESULT_CONNECTLOST,		// 连接丢失
		ERESULT_SESSIONLOST,		// 会话丢失
		ERESULT_STATEFAULT,			// 状态不允许
	};

	// ********************************************************************** //
	// CNSProtocol
	// ********************************************************************** //
	class CNSProtocol : public CNSMarshal
	{
	public:
		class CException : public CNSException
		{
		public:
			CException( ) : CNSException( "" )
			{
			}
		};

		TProtocolID mProtoID;
		bool mIsRawProto;

	public:
		CNSProtocol( TProtocolID protoID, bool isRawProto = false ) : mProtoID( protoID ), mIsRawProto( isRawProto )
		{
		}

	public:
		virtual const CNSOctetsShadow& unMarshal( const CNSOctetsShadow& stream )
		{
			return stream;
		}

	public:
		bool isRawProto( ) const
		{
			return mIsRawProto;
		}

		const CNSOctets& createShortStream( ) const
		{
			static CNSOctetsStream stream;
			stream.clear( );
			stream << (TProtocolID) mProtoID;
			stream << (unsigned short) 0;

			unsigned int oldLen = stream.length( );
			marshal( stream );
			unsigned int len = stream.length( ) - oldLen;
			stream.repBytes( sizeof( TProtocolID ), (unsigned short) len );
			return stream.mBuffer;
		}

		const CNSOctets& createStream( ) const
		{
			static CNSOctetsStream stream;
			stream.clear( );
			stream << (TProtocolID) mProtoID;
			stream << (unsigned int) 0;

			unsigned int oldLen = stream.length( );
			marshal( stream );
			unsigned int len = stream.length( ) - oldLen;
			stream.repBytes( sizeof( TProtocolID ), (unsigned int) len );
			return stream.mBuffer;
		}

		TProtocolID getProtoID( ) const;
	};

	// ********************************************************************** //
	// CNSNetManager
	// ********************************************************************** //
	class CNSNetManager
	{
		typedef CNSMap< int, CNSProtocol* >	TProtocolStub;
		friend class CNSSession;

	private:
		TProtocolStub	mProtocolStubs;			// 协议存根
		bool			mIsHttp;
		bool			mIsNotifyProgress;
		CNSNetworkIO*	mpNetworkIO;

	public:
		CNSString		mName;					// 管理器名称(唯一)
		CNSString		mAddress;				// 连接地址
		unsigned int	mBufferSize;			// 缓冲大小
		unsigned int	mStreamSize;			// 缓冲大小
		unsigned int	mPollTimeout;			// IOCP查询超时时间
		unsigned int	mPollInterval;			// IOCP查询间隔时间，时间越长延迟越大，效率越高

	public:
		CNSNetManager( const CNSString& name, const CNSString& address, unsigned int pollInterval = 10, unsigned int bufferSize = 512,
			unsigned int streamSize = 16384, bool isHttp = false, bool notifyProgress = false );

	public:
		virtual void onRegisterProtocol( const CNSString& name )
		{
			mProtocolStubs.clear( );
		}
		bool sizePolicy( unsigned int protoType, unsigned int protoSize )
		{
			return true;
		}
		virtual void onAddSessionFault( const CNSString& name, int code )
		{
		}
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer ) = 0;
		virtual void onDelSession( const CNSString& name, TSessionID sessionID ) = 0;
		virtual bool onCheckAccumulate( const CNSString& name, unsigned int bytes )
		{
			return true;
		}
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
		{
		}
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, const CNSMap< CNSString, CNSString >& headers, const CNSString& text, const CNSString& postDat )
		{
		}
		virtual void onProtocolProgress( TProtocolID protocolID, unsigned int total, unsigned int progress )
		{
		}
		virtual void onRawProtocol( const CNSString& name, TSessionID sessionID, TProtocolID* protoID, const CNSOctetsShadow& buffer )
		{
		}

	protected:
		CNSProtocol* createProtocol( TProtocolID protoType );

	public:
		void registerProtocol( CNSProtocol* proto );

		inline unsigned int getBufferSize( ) const
		{
			return mBufferSize;
		}

		inline unsigned int getStreamSize( ) const
		{
			return mStreamSize;
		}

		inline bool isHttp( ) const
		{
			return mIsHttp;
		}

		inline bool isNotifyProgress( ) const
		{
			return mIsNotifyProgress;
		}

		inline void setNetworkIO( CNSNetworkIO* io )
		{
			mpNetworkIO = io;
		}

		inline CNSNetworkIO* getNetworkIO( ) const
		{
			return mpNetworkIO;
		}
	};

	/**
	 * 获得主机名
	 */
	CNSString getHostName( );

	/**
	 * 获得主机ip地址列表
	 */
	void getIPV4Address( CNSVector< CNSString >& address );
	/**
	 * 初始化网络库，调用所有函数之前调用
	 */
	void init( );

	/**
	 * 释放网络库，调用所有函数之后调用
	 */
	void exit( );

	/**
	 * 关闭连接
	 *
	 * @参数	pName				网络对象
	 * @参数	sessionID			需要被关闭的网络对象，如果对于主动网络对象该值无意义
	 *
	 * @返回值	ERESULT_SUCCESS		关闭成功
	 *			ERESULT_IONOTEXIST	指定IO不存在
	 *			ERESULT_SESSIONLOST	会话丢失
	 *			ERESULT_STATEFAULT	状态不允许
	 */
	void close( const CNSString& name, TSessionID sessionID = 0 );
	void send( CNSNetworkIO* io, const CNSOctets& buffer, TSessionID sessionID = 0, bool force = false );
	void send( const CNSString& name, const CNSOctets& buffer, TSessionID sessionID = 0, bool force = false );

	// 优化网关用的接口，平时禁止调用
	void send( CNSNetworkIO* io, const void* begin, const void* end, TSessionID sessionID = 0, bool force = false );
	void send( CNSNetworkIO* io, const void* begin, size_t len, TSessionID sessionID = 0, bool force = false );

	CNSNetworkIO* registerServer( CNSNetManager* manager, unsigned int acceptNum );
	CNSNetworkIO* registerClient( CNSNetManager* manager );
	void pollEvent( );

	CNSSessionDesc* getSessionDesc( const CNSString& name, TSessionID sessionID );
	CNSNetworkIO* getNetworkIO( const CNSString& name );
};

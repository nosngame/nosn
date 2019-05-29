#pragma once
#define ScriptProtoImpl								\
		const CNSOctetsShadow* mpShadowBuffer;		\
		CNSOctets* mpScriptBuffer;					\
		CNSLuaStack* mpLuaStack;						\
													\
	public:											\
		CProtocolScript( CNSLuaStack* luaStack = NULL )	\
			: CNSProtocol( PID ), mpLuaStack( luaStack ), mpScriptBuffer( NULL ) \
		{	\
		}	\
			\
	public:	\
		~CProtocolScript( )	\
		{	\
		}	\
			\
	public:	\
		virtual const CNSOctetsShadow& unMarshal( const CNSOctetsShadow& stream )	\
		{	\
			mpShadowBuffer = &stream;	\
			return stream;	\
		}	\
			\
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const	\
		{	\
			size_t oldLen = stream.length( );					\
			for ( ; mpLuaStack->isEnd( ) == false; )			\
				*mpLuaStack >> stream;							\
																\
			size_t len = stream.length( ) - oldLen;				\
			if ( len >= 65530 )									\
			{													\
				unsigned int pid = 0;							\
				EDataType dataType = (EDataType) *(unsigned char*) ( (char*) stream.begin( ) + oldLen );	\
				if ( dataType == EDataType::TYPE_CHAR )						\
				{																			\
					pid = *(char*) ( (char*) stream.begin( ) + oldLen + 1 );				\
				}																			\
				else if ( dataType == EDataType::TYPE_UCHAR )					\
				{																			\
					pid = *(unsigned char*) ( (char*) stream.begin( ) + oldLen + 1 );		\
				}																			\
				else if ( dataType == EDataType::TYPE_SHORT )					\
				{																			\
					pid = *(short*) ( (char*) stream.begin( ) + oldLen + 1 );				\
				}																			\
				else if ( dataType == EDataType::TYPE_USHORT )					\
				{																			\
					pid = *(unsigned short*) ( (char*) stream.begin( ) + oldLen + 1 );		\
				}																			\
				else if ( dataType == EDataType::TYPE_INT )					\
				{																			\
					pid = *(int*) ( (char*) stream.begin( ) + oldLen + 1 );					\
				}																			\
				else if ( dataType == EDataType::TYPE_UINT )					\
				{																			\
					pid = *(unsigned int*) ( (char*) stream.begin( ) + oldLen + 1 );		\
				}																			\
																							\
				static CNSString errorDesc;													\
				errorDesc.format( _UTF8("脚本协议超过65530限制，脚本协议ID - %d"), pid );	\
				NSException( errorDesc );													\
			} \
			return stream;	\
		}	\
			\
		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )			\
		{																					\
			static CNSOctets scriptBuffer;													\
			size_t progress = stream.mBuffer.length( ) - stream.mPos;						\
																							\
			scriptBuffer.replace( (char*) stream.mBuffer.begin( ) + stream.mPos, progress );	\
			stream.mPos += progress;															\
			mpScriptBuffer = &scriptBuffer;														\
			return stream;																		\
		}																									

// gate<->oper, gate<->logic gate<->client
namespace NSGateProto
{
	class CProtocolTunnel : public CNSProtocol
	{
	public:
		enum
		{
			PID = 0,	// 发送给玩家当前服务
			PPID = 1,	// 发送给oper服务，整个游戏一个oper服务
		};

	public:
		CNSOctets* mpBuffer;
		TSessionID mSessionID;
		static CNSMap< TProtocolID, CNSProtocol* >	sProtoStubs;

	public:
		// buffer 的内存是一个完整协议，不包含长度
		CProtocolTunnel( CNSOctets* buffer = NULL, TSessionID sessionID = 0, TProtocolID pid = PID )
			: CNSProtocol( pid, true ), mSessionID( sessionID ), mpBuffer( buffer )
		{
		}

	public:
		static void registerProtocol( CNSProtocol* proto )
		{
			sProtoStubs.insert( proto->getProtoID( ), proto );
		}

		static CNSProtocol* getProto( const CNSOctetsShadow& tunnelBuffer )
		{
			TProtocolID* tunnelPid = NULL;
			tunnelBuffer.unmarshalToPointer( tunnelPid );

			CNSProtocol** protoRef = sProtoStubs.get( *tunnelPid );
			if ( protoRef == NULL )
			{
				NSLog::log( _UTF8( "协议ID - %d 没有找到" ), *tunnelPid );
				return NULL;
			}

			CNSProtocol* proto = ( *protoRef );
			proto->unMarshal( tunnelBuffer );
			return proto;
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << *mpBuffer;

			// 如果是客户端发送给网关的隧道协议，不需要发送mSessionID;
			if ( mSessionID != 0 )
				stream << mSessionID;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			return stream;
		}
	};

	// 网关通知玩家下线
	class CProtocolLogout : public CNSProtocol
	{
	public:
		enum { PID = 2 };
		int	mReasonID;

	public:
		CProtocolLogout( int reasonID = 0 ) : CNSProtocol( PID ), mReasonID( reasonID )
		{
		}

		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mReasonID;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> mReasonID;
			return stream;
		}
	};

	class CProtocolKeepAlive : public CNSProtocol
	{
	public:
		enum { PID = 3 };

	public:
		CProtocolKeepAlive( ) : CNSProtocol( PID )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& rStream ) const
		{
			return rStream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& rStream )
		{
			return rStream;
		}
	};

	// 注册逻辑服务
	class CProtocolRegisterLogicService : public CNSProtocol
	{
	public:
		enum { PID = 4 };

	public:
		CNSString		mName;
		unsigned int	mServiceID;		// 逻辑服务ID

	public:
		CProtocolRegisterLogicService( const CNSString& name = "", unsigned int serviceID = 0 )
			: CNSProtocol( PID ), mName( name ), mServiceID( serviceID )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mServiceID;
			stream << mName;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> mServiceID;
			stream >> mName;
			return stream;
		}
	};

	// 注册主服务
	class CProtocolRegisterOperService : public CNSProtocol
	{
	public:
		enum { PID = 5 };

	public:
		CProtocolRegisterOperService( ) : CNSProtocol( PID )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			return stream;
		}
	};

	// 通知内网服务器玩家登录成功
	class CProtocolClientLogin : public CNSProtocol
	{
	public:
		enum { PID = 6 };

	public:
		TSessionID mSessionID;
		long long mNosnUserID;

	public:
		CProtocolClientLogin( TSessionID sessionID = 0, long long nosnUserID = 0L ) : CNSProtocol( PID ), mSessionID( sessionID ), mNosnUserID( nosnUserID )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mSessionID;
			stream << mNosnUserID;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> mSessionID;
			stream >> mNosnUserID;
			return stream;
		}
	};

	// 通知内网服务器玩家连接丢失
	class CProtocolClientLost : public CNSProtocol
	{
	public:
		enum { PID = 7 };

	public:
		unsigned int	mSessionID;

	public:
		CProtocolClientLost( unsigned int sessionID = 0 ) : CNSProtocol( PID ), mSessionID( sessionID )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& rStream ) const
		{
			rStream << mSessionID;
			return rStream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& rStream )
		{
			rStream >> mSessionID;
			return rStream;
		}
	};

	// 内网服务器通知网关未知玩家
	class CProtocolUnknownUser : public CNSProtocol
	{
	public:
		enum { PID = 8 };

	public:
		TSessionID mSessionID;
		int	mReasonID;			// 只有当不断开连接时，才有效

	public:
		CProtocolUnknownUser( TSessionID sessionID = 0, int reasonID = 0 ) : CNSProtocol( PID ), mSessionID( sessionID ), mReasonID( reasonID )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& rStream ) const
		{
			rStream << mSessionID;
			rStream << mReasonID;
			return rStream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& rStream )
		{
			rStream >> mSessionID;
			rStream >> mReasonID;
			return rStream;
		}
	};

	class CProtocolChangeService : public CNSProtocol
	{
	public:
		enum { PID = 9 };

	public:
		unsigned int	mSessionID;
		unsigned int	mServiceID;

	public:
		CProtocolChangeService( unsigned int sessionID = 0, unsigned int vServiceID = 0 ) : CNSProtocol( PID ), mSessionID( sessionID ), mServiceID( vServiceID )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& rStream ) const
		{
			rStream << mSessionID;
			rStream << mServiceID;
			return rStream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& rStream )
		{
			rStream >> mSessionID;
			rStream >> mServiceID;
			return rStream;
		}
	};

	class CProtocolEnableIPOpen : public CNSProtocol
	{
	public:
		enum { PID = 10 };
		bool mEnableIPOpen;

	public:
		CProtocolEnableIPOpen( bool vEnableIPOpen = true ) : CNSProtocol( PID ), mEnableIPOpen( vEnableIPOpen )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& rStream ) const
		{
			rStream << mEnableIPOpen;
			return rStream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& rStream )
		{
			rStream >> mEnableIPOpen;
			return rStream;
		}
	};

	class CProtocolShutdownGate : public CNSProtocol
	{
	public:
		enum { PID = 11 };
		unsigned mDuration;

	public:
		CProtocolShutdownGate( unsigned int duration = 0 ) : CNSProtocol( PID ), mDuration( duration )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& rStream ) const
		{
			rStream << mDuration;
			return rStream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& rStream )
		{
			rStream >> mDuration;
			return rStream;
		}
	};

	class CProtocolSessionIPNotice : public CNSProtocol
	{
	public:
		enum { PID = 12 };
		CNSString		mIP;
		TSessionID		mSessionID;

	public:
		CProtocolSessionIPNotice( const CNSString& ip = "", TSessionID sessionID = 0 ) : CNSProtocol( PID ), mIP( ip ), mSessionID( sessionID )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& rStream ) const
		{
			rStream << mIP;
			rStream << mSessionID;
			return rStream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& rStream )
		{
			rStream >> mIP;
			rStream >> mSessionID;
			return rStream;
		}
	};

	// 用于客户端之间位置同步
	class CProtocolBatchTunnel : public CNSProtocol
	{
	public:
		enum { PID = 13 };
		CNSVector< TSessionID >*	mpUserList;
		CNSOctets*					mpBuffer;

	public:
		CProtocolBatchTunnel( CNSOctets* buffer = NULL ) : CNSProtocol( PID, true ), mpUserList( NULL ), mpBuffer( buffer )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << ( *mpUserList );
			stream << ( *mpBuffer );
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			return stream;
		}
	};
}

// client<->logic
namespace NSClientProto
{
	// 客户端发送给网关服务器登录请求
	class CProtocolLogin : public CNSProtocol
	{
	public:
		enum { PID = 1 };

	public:
		CNSString		mAuthName;
		CNSString		mAuthUserID;
		CNSString		mToken;

	public:
		CProtocolLogin( const CNSString& authName = "", const CNSString& userName = "", const CNSString& token = "" ) :
			CNSProtocol( PID ), mAuthName( authName ), mAuthUserID( userName ), mToken( token )
		{
		}

	public:
		virtual const CNSOctetsShadow& unMarshal( const CNSOctetsShadow& stream )
		{
			stream >> mAuthName;
			stream >> mAuthUserID;
			stream >> mToken;
			return stream;
		}

		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mAuthName;
			stream << mAuthUserID;
			stream << mToken;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> mAuthName;
			stream >> mAuthUserID;
			stream >> mToken;
			return stream;
		}
	};

	class CProtocolScript : public CNSProtocol
	{
	public:
		enum { PID = 2 };
		ScriptProtoImpl
	};

	class CProtocolLoginResult : public CNSProtocol
	{
	public:
		enum { PID = 3 };
		enum ELoginResult
		{
			LOGIN_SUCCESS = 0,
			LOGIN_PSWERROR,
			LOGIN_TOKENINVALID
		};

	public:
		unsigned char mResult;

	public:
		CProtocolLoginResult( unsigned int result = LOGIN_SUCCESS ) : CNSProtocol( PID ), mResult( result )
		{
		}

	public:
		virtual const CNSOctetsShadow& unMarshal( const CNSOctetsShadow& stream )
		{
			stream >> mResult;
			return stream;
		}

		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mResult;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> mResult;
			return stream;
		}
	};
}

// oper<->operclient(logicserver)
namespace NSOperProto
{
	class CProtocolScript : public CNSProtocol
	{
	public:
		enum { PID = 1 };
		ScriptProtoImpl
	};
}

// charge<->chargeclient(logicserver)
namespace NSChargeProto
{
	class CProtocolScript : public CNSProtocol
	{
	public:
		enum { PID = 1 };
		ScriptProtoImpl
	};
}

// logicserver<->logicserver
namespace NSLogicProto
{
	class CProtocolScript : public CNSProtocol
	{
	public:
		enum { PID = 1 };
		ScriptProtoImpl
	};
}

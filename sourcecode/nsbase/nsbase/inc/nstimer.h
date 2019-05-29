#pragma once
namespace NSBase
{
	class CNSTimer
	{
	public:
		class CTimerTrigger;
		class CTimerObject;

		typedef void( *FTimerHandler )( int timerID, unsigned int curTick, int remain, void* userdata );
		class CTimerObject
		{
		public:
			unsigned int		mTick;
			unsigned int		mDuration;
			unsigned int		mCloseCounter;		// 触发指定次数之后，关闭，0表示永远不关闭
			unsigned int		mFireCounter;		// 当前触发次数

			void*				mUserData;
			CTimerTrigger*		mpTrigger;
			FTimerHandler		mpProc;
			unsigned int		mTriggerIndex;
			CNSSet< time_t >	mTriggerHappened;

		public:
			CTimerObject( FTimerHandler proc = NULL, unsigned int duration = 0, unsigned int closeCounter = 0, void* userdata = NULL )
				: mTick( 0 ), mDuration( duration ), mpProc( proc ), mUserData( userdata ), mpTrigger( NULL ), mTriggerIndex( 0 ), mCloseCounter( closeCounter ), mFireCounter( 0 )
			{
			}

			CTimerObject( FTimerHandler proc, CTimerTrigger* trigger, void* userdata )
				: mTick( 0 ), mpTrigger( trigger ), mpProc( proc ), mUserData( userdata ), mDuration( 0 ), mTriggerIndex( 0 )
			{
				CNSTimer now;
				CNSTimer curTime;
				for ( size_t i = 0; i < mpTrigger->mData.getCount( ); i ++ )
				{
					CTimerTrigger::CTriggerData& curTrigger = mpTrigger->mData[ (unsigned int) i ];
					curTime.setHour( curTrigger.mHour );
					curTime.setMin( curTrigger.mMin );
					curTime.setSec( 0 );
					time_t backTime = curTime.getUnixTime( );
					if ( (unsigned int) now.getUnixTime( ) > ( unsigned int ) backTime )
					{
						mTriggerHappened.insert( backTime );
						mTriggerIndex = (unsigned int) i + 1;
					}
				}
			}

		public:
			void updateTrigger( unsigned int curTick, const CNSTimer& curTime )
			{
				CNSTimer triggerTime = curTime;
				if ( mpTrigger->mTriggerType == CTimerTrigger::TRIGGER_DAY )
				{
					if ( mTriggerIndex < (int) mpTrigger->mData.getCount( ) )
					{
						CTimerTrigger::CTriggerData curTrigger = mpTrigger->mData[ mTriggerIndex ];
						triggerTime.setHour( curTrigger.mHour );
						triggerTime.setMin( curTrigger.mMin );
						triggerTime.setSec( 0 );

						time_t triggerUnixTime = triggerTime.getUnixTime( );
						bool needTrigger = false;
						if ( curTime.getUnixTime( ) > triggerUnixTime && mTriggerHappened.find( triggerUnixTime ) == false )
						{
							mTriggerHappened.insert( triggerUnixTime );
							needTrigger = true;
						}

						if ( needTrigger == true && mpProc != NULL )
							mpProc( timerID, curTick, -1, mUserData );

						if ( needTrigger == true )
						{
							mTriggerIndex = mTriggerIndex + 1;
							if ( mTriggerIndex >= (int) mpTrigger->mData.getCount( ) )
								mTriggerIndex = 0;
						}
					}
				}
				else if ( mpTrigger->mTriggerType == CTimerTrigger::TRIGGER_WEEK )
				{
				}
				else if ( mpTrigger->mTriggerType == CTimerTrigger::TRIGGER_MONTH )
				{
				}
				else if ( mpTrigger->mTriggerType == CTimerTrigger::TRIGGER_YEAR )
				{
				}
			}
		};

		class CTimerTrigger
		{
		public:
			enum TriggerType
			{
				TRIGGER_DAY,		// 每天的几点
				TRIGGER_WEEK,		// 每周的星期几，几点
				TRIGGER_MONTH,		// 每月的几号，几点
				TRIGGER_YEAR,		// 每年的几号，几点
			};

			class CTriggerData
			{
			public:
				int		mDayMonth = 0;	// TRIGGER_YEAR 有效
				int		mDayWeek = 0;	// TRIGGER_WEEK 有效
				int		mHour = 0;		// TRIGGER_WEEK TRIGGER_YEAR TRIGGER_DAY 有效
				int		mMin = 0;		// TRIGGER_WEEK TRIGGER_YEAR TRIGGER_DAY 有效

			public:
				CTriggerData( int h, int m ) : mHour( h ), mMin( m )
				{
				}

				CTriggerData( short dayMonth, int h, int m ) : mDayMonth( dayMonth ), mHour( h ), mMin( m )
				{
				}

				CTriggerData( char mDayWeek, int h, int m ) : mDayWeek( mDayWeek ), mHour( h ), mMin( m )
				{
				}
			};

		public:
			int mTriggerType = TriggerType::TRIGGER_DAY;
			CNSVector< CTriggerData > mData;
		};

	private:
		static CNSMap< int, CTimerObject >	sTimer;
		static CNSSet< int >				sRemoveList;
		static int timerID;

	public:
		static void updateTimer( )
		{
			unsigned int curTick = CNSTimer::getCurTick( );
			static unsigned int lastTick = curTick;
			if ( curTick - lastTick < 10 )
				return;

			for ( HLISTINDEX beginIndex = sRemoveList.getHead( ); beginIndex != NULL; sRemoveList.getNext( beginIndex ) )
			{
				unsigned int timerID = sRemoveList.getKey( beginIndex );
				HMAPINDEX findIndex = sTimer.findNode( timerID );
				if ( findIndex == NULL )
					continue;

				sTimer.eraseNode( findIndex );
			}
			sRemoveList.clear( );

			lastTick = curTick;
			for ( HLISTINDEX beginIndex = sTimer.getHead( ); beginIndex != NULL; sTimer.getNext( beginIndex ) )
			{
				CTimerObject& timer = sTimer.getValue( beginIndex );
				int timerID = sTimer.getKey( beginIndex );
				if ( timer.mTick == 0 )
					timer.mTick = curTick;

				if ( timer.mDuration > 0 && int( curTick - timer.mTick ) >= (int) timer.mDuration )
				{
					if ( timer.mpProc != NULL )
					{
						int remain = -1;
						if ( timer.mCloseCounter > 0 )
						{
							timer.mFireCounter ++;
							if ( timer.mFireCounter == timer.mCloseCounter )
								sRemoveList.insert( timerID );

							remain = (int) timer.mCloseCounter - timer.mFireCounter;
						}

						timer.mpProc( timerID, curTick, remain, timer.mUserData );
					}

					timer.mTick += timer.mDuration;
				}
				else if ( timer.mpTrigger != NULL )
				{
					CNSTimer curTime;
					timer.updateTrigger( curTick, curTime );
				}
			}

			for ( HLISTINDEX beginIndex = sRemoveList.getHead( ); beginIndex != NULL; sRemoveList.getNext( beginIndex ) )
			{
				unsigned int timerID = sRemoveList.getKey( beginIndex );
				HMAPINDEX findIndex = sTimer.findNode( timerID );
				if ( findIndex == NULL )
					continue;

				sTimer.eraseNode( findIndex );
			}
			sRemoveList.clear( );
		}

		static int createDelay( CNSTimer::FTimerHandler proc, unsigned int duration, void* userdata )
		{
			timerID ++;
			sTimer.insert( timerID, CTimerObject( proc, duration, 1, userdata ) );
			return timerID;
		}

		static int createTimer( CNSTimer::FTimerHandler proc, unsigned int duration, void* userdata )
		{
			timerID ++;
			sTimer.insert( timerID, CTimerObject( proc, duration, 0, userdata ) );
			return timerID;
		}

		static int createTimer( CNSTimer::FTimerHandler proc, CTimerTrigger* trigger, void* userdata )
		{
			timerID ++;
			sTimer.insert( timerID, CTimerObject( proc, trigger, userdata ) );
			return timerID;
		}

		static CNSTimer::CTimerTrigger* createDayTrigger( int durMin )
		{
			CNSTimer::CTimerTrigger* trigger = new CNSTimer::CTimerTrigger( );
			trigger->mTriggerType = CNSTimer::CTimerTrigger::TRIGGER_DAY;
			for ( int i = 0; i < 24 * 60; i += durMin )
			{
				CTimerTrigger::CTriggerData data( i / 60, i % 60 );
				trigger->mData.pushback( data );
			}

			return trigger;
		}

		static CNSTimer::CTimerTrigger* createDayTrigger( int dayHour, int dayMin )
		{
			CNSTimer::CTimerTrigger* trigger = new CNSTimer::CTimerTrigger( );
			trigger->mTriggerType = CNSTimer::CTimerTrigger::TRIGGER_DAY;
			CNSTimer::CTimerTrigger::CTriggerData data( dayHour, dayMin );
			trigger->mData.pushback( data );
			return trigger;
		}

		static CNSTimer::CTimerTrigger* createDayTrigger( const CNSVector< CNSTimer::CTimerTrigger::CTriggerData >& data )
		{
			CNSTimer::CTimerTrigger* trigger = new CNSTimer::CTimerTrigger( );
			trigger->mTriggerType = CNSTimer::CTimerTrigger::TRIGGER_DAY;
			trigger->mData = data;
			return trigger;
		}

		static void removeTimer( int timerID )
		{
			sRemoveList.insert( timerID );
		}

	public:
		static unsigned int getCurTick( )
		{
#ifdef PLATFORM_WIN32
			LARGE_INTEGER freq;
			LARGE_INTEGER curTime;
			QueryPerformanceFrequency( &freq );
			QueryPerformanceCounter( &curTime );
			return (unsigned int) ( ( (double) curTime.QuadPart / (double) freq.QuadPart ) * 1000 );
#else
			double uptime = 0;
			struct timespec on;
			if ( clock_gettime( CLOCK_MONOTONIC, &on ) == 0 )
				uptime = (double) on.tv_sec * 1000.0 + (double) on.tv_nsec / 1000000.0;
			return uptime;
#endif
		}

	protected:
		int year;
		int month;
		int day;
		int hour;
		int min;
		int sec;
		int wday;
		int yday;
		mutable time_t curTime;
		mutable bool needRefresh;

	protected:
		void refresh( ) const
		{
			if ( needRefresh == true )
			{
				tm tmValue;
				tmValue.tm_year = year - 1900;
				tmValue.tm_mon = month - 1;
				tmValue.tm_mday = day;
				tmValue.tm_hour = hour;
				tmValue.tm_min = min;
				tmValue.tm_sec = sec;
				tmValue.tm_isdst = false;
				needRefresh = false;
				curTime = mktime( &tmValue );
			}
		}

		void makeTime( )
		{
			tm* tpTime = ::localtime( &curTime );
			year = 1900 + tpTime->tm_year;
			month = tpTime->tm_mon + 1;
			day = tpTime->tm_mday;
			wday = tpTime->tm_wday;
			yday = tpTime->tm_yday;
			hour = tpTime->tm_hour;
			min = tpTime->tm_min;
			sec = tpTime->tm_sec;
		}

	public:
		CNSTimer( ) : needRefresh( false )
		{
			curTime = time( NULL );
			makeTime( );
		}

		CNSTimer( time_t time ) : needRefresh( false )
		{
			curTime = time;
			makeTime( );
		}

		CNSTimer( int year, int month = 0, int day = 0, int hour = 0, int min = 0, int sec = 0 ) : needRefresh( false )
		{
			tm tmValue;
			tmValue.tm_year = year - 1900;
			tmValue.tm_mon = month - 1;
			tmValue.tm_mday = day;
			tmValue.tm_hour = hour;
			tmValue.tm_min = min;
			tmValue.tm_sec = sec;
			tmValue.tm_isdst = false;
			curTime = mktime( &tmValue );
			makeTime( );
		}

		void buildTime( )
		{
			curTime = time( NULL );
			makeTime( );
		}

		void buildTime( time_t time )
		{
			curTime = time;
			makeTime( );
		}

		int getYear( ) const
		{
			return year;
		}

		int getMonth( ) const
		{
			return month;
		}

		int getDay( ) const
		{
			return day;
		}

		int getWeekDay( ) const
		{
			return wday;
		}

		int getYearDay( ) const
		{
			return yday;
		}

		time_t getUnixTime( ) const
		{
			refresh( );
			return curTime;
		}

		CNSString& getTimeText( ) const
		{
			static CNSString time;
			time.format( "%d-%d-%d %d:%d:%d", year, month, day, hour, min, sec );
			return time;
		}

		CNSString& getTimeTextOfDay( ) const
		{
			static CNSString time;
			time.format( "%d-%d-%d", year, month, day );
			return time;
		}

		void setHour( int h )
		{
			hour = h;
			needRefresh = true;
		}

		void setMin( int m )
		{
			min = m;
			needRefresh = true;
		}

		void setSec( int s )
		{
			sec = s;
			needRefresh = true;
		}

		void alignDay( )
		{
			hour = 0;
			min = 0;
			sec = 0;
			needRefresh = true;
		}

		bool isSameDay( const CNSTimer& time )
		{
			return year == time.year && month == time.month && day == time.day;
		}

		void nextDay( int count, int h, int m, int s )
		{
			refresh( );
			curTime += (time_t) count * 24 * 60 * 60;
			struct tm* newTm = localtime( &curTime );
			if ( newTm == NULL )
				return;

			newTm->tm_hour = h;
			newTm->tm_min = m;
			newTm->tm_sec = s;
			curTime = mktime( newTm );
			makeTime( );
		}

		void prevDay( int count, int h, int m, int s )
		{
			refresh( );
			curTime -= (time_t) count * 24 * 60 * 60;
			struct tm* newTm = localtime( &curTime );
			if ( newTm == NULL )
				return;

			newTm->tm_hour = h;
			newTm->tm_min = m;
			newTm->tm_sec = s;
			curTime = mktime( newTm );
			makeTime( );
		}

		void nextDay( int count )
		{
			refresh( );
			curTime += 24 * 60 * 60 * (time_t) count;
			makeTime( );
		}

		void nextWeek( int count )
		{
			refresh( );
			curTime += 24 * 60 * 60 * 7 * (time_t) count;
			makeTime( );
		}

		void prevDay( int count )
		{
			refresh( );
			curTime -= 24 * 60 * 60 * (time_t) count;
			makeTime( );
		}

		void prevWeek( int count )
		{
			refresh( );
			curTime -= 24 * 60 * 60 * 7 * (time_t) count;
			makeTime( );
		}

		bool isLeapYear( ) const
		{
			if ( year % 400 == 0 || ( year % 100 != 0 && year % 4 == 0 ) )
				return true;

			return false;
		}
	};

	class CNSSmallTimer : public CNSMarshal
	{
	public:
		unsigned short mTime;

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mTime;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> mTime;
			return stream;
		}

	public:
		CNSSmallTimer( ) : mTime( 0 )
		{
		}

		CNSSmallTimer( const CNSTimer& time )
		{
			unsigned char year = time.getYear( ) - 2017;// 7
			unsigned char month = time.getMonth( );		// 4
			unsigned char day = time.getDay( );			// 5
			mTime = year << 9 | month << 5 | day;
		}

		CNSSmallTimer& operator = ( const CNSTimer& time )
		{
			unsigned char year = time.getYear( ) - 2017;// 7
			unsigned char month = time.getMonth( );		// 4
			unsigned char day = time.getDay( );			// 5
			mTime = year << 9 | month << 5 | day;
			return *this;
		}

		bool operator != ( const CNSSmallTimer& time ) const
		{
			return mTime != time.mTime;
		}

		bool operator == ( const CNSSmallTimer& time ) const
		{
			return mTime == time.mTime;
		}

		unsigned int operator - ( const CNSSmallTimer& time ) const
		{
			CNSTimer time1( getYear( ), getMonth( ), getDay( ) );
			CNSTimer time2( time.getYear( ), time.getMonth( ), time.getDay( ) );
			return (unsigned int) ( time1.getUnixTime( ) - time2.getUnixTime( ) );
		}

		int getYear( ) const
		{
			return ( mTime >> 9 ) + 2017;
		}

		int getMonth( ) const
		{
			return ( mTime & 511 ) >> 5;
		}

		int getDay( ) const
		{
			return mTime & 31;
		}

		unsigned short getTime( ) const
		{
			return mTime;
		}
	};

	static void nsTimerShutdown( int timerID, unsigned int curTick, int remain, void* userdata )
	{
		exit( 0 );
	}
}

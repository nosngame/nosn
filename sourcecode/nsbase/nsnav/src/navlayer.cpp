#include <nsbase.h>
#include "nav.h"
namespace NSNav
{

	void CNSNavLayer::loadHeightMap( CNSNavLayer* layer, const CNSString& navName, const CNSOctetsStream& stream, unsigned short w, unsigned short h )
	{
		NSLog::log( "load heightmap" );
		unsigned int count;
		stream >> count;
		if ( stream.mPos + count * sizeof( float ) > stream.length( ) )
			throw CNSMarshal::CException( );
	
		layer->createHeightMap( w, h, (char*) stream.begin( ) + stream.mPos );
		stream.mPos += count * sizeof( float );
	}

	void CNSNavLayer::loadBsp( CNSNavLayer* layer, const CNSString& navName, const CNSOctetsStream& stream, unsigned short w, unsigned short h, bool buildNav )
	{
		NSLog::log( "load bsp - %s", navName.getBuffer( ) );
		layer->createBsp( navName, w, h );

		unsigned int verticeCount = 0;
		unsigned int dim = 0;
		stream >> verticeCount;
		stream >> dim;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		for ( unsigned int i = 0; i < verticeCount; i ++ )
		{
			if ( dim == 3 )
			{
				stream >> x;
				stream >> y;
				stream >> z;
			}
			else if ( dim == 2 )
			{
				stream >> x;
				stream >> z;
			}

			if ( buildNav == false )
				continue;

			layer->addVertice( navName, CNSVector2( x, z ) );
		}

		unsigned int indiceCount = 0;
		stream >> indiceCount;
		unsigned short indices[3];
		int index	= 0;
		int nodeID	= 1;
		for ( unsigned int i = 0; i < indiceCount; i ++, index ++ )
		{
			stream >> indices[ index % 3 ];
			if ( buildNav == false )
				continue;

			if ( index % 3 == 2 )
				layer->addNode( navName, nodeID ++, indices[0], indices[1], indices[2] );
		}
	}

	CNSNavLayer* CNSNavLayer::create( const char* navFile, bool buildNav )
	{
		NSLog::log( "load nav file - %s", navFile );
		FILE* tpFile = fopen( navFile, "rb" );
		if ( tpFile == NULL )
		{
			CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]没有找到" ), navFile );
			NSException( errorDesc );
			return NULL;
		}

		fseek( tpFile, 0, SEEK_END );
		long fileLength = ftell( tpFile );
		fseek( tpFile, 0, SEEK_SET );

		CNSOctets fileBuffer( fileLength, fileLength );
		fread( fileBuffer.begin( ), fileLength, 1, tpFile );
		fclose( tpFile );
		CNSOctetsStream stream( fileBuffer );

		try
		{
			unsigned short sceneWidth = 0;
			unsigned short sceneHeight = 0;
			stream >> sceneWidth;
			stream >> sceneHeight;
			CNSNavLayer* tpLayer = new CNSNavLayer( sceneWidth, sceneHeight );
			for ( ; stream.isEnd( ) == false; )
			{
				CNSString navName;
				stream >> navName;
				if ( navName == "heightmap" )
				{
					loadHeightMap( tpLayer, navName, stream, sceneWidth, sceneHeight );
				}
				else if ( navName == "nav" )
				{
					loadBsp( tpLayer, navName, stream, sceneWidth, sceneHeight, buildNav );
					if ( buildNav == true )
						tpLayer->mpBspNav->buildEdge( );
				}
				else if ( navName == "PKProtect" )
				{
					loadBsp( tpLayer, navName, stream, sceneWidth, sceneHeight, true );
				}
				else if ( navName == "PKNormal" )
				{
					loadBsp( tpLayer, navName, stream, sceneWidth, sceneHeight, true );
				}
			}

			return tpLayer;
		}
		catch( CNSMarshal::CException& )
		{
			NSException( _UTF8( "nav文件解析错误" ) );
		}

		// make complie happy ;-)
		return NULL;
	}

	bool CNSNavLayer::isBlock( const CNSVector2& pos )
	{
		return mpBspNav->pointInNav( pos ) != NULL;
	}

	void CNSNavLayer::findPath( const CNSVector2& src, const CNSVector2& des, bool needPath )
	{
		mPathNode.clear( );
		closeList.clear( );
		openList.clear( );
		mBestNode = NULL;
		CNSNavNode* srcNode = mpBspNav->pointInNav( src );
		CNSNavNode* desNode = mpBspNav->pointInNav( des );
		if ( srcNode == NULL || desNode == NULL )
		{
			mPathNode.pushback( CNSPathNode( src, srcNode ) );
			mPathNode.pushback( CNSPathNode( des, desNode ) );
			return;
		}

		if ( needPath == false )
		{
			mPathNode.pushback( CNSPathNode( src, srcNode ) );
			mPathNode.pushback( CNSPathNode( des, desNode ) );
			return;
		}

		srcNode->reset( );
		desNode->reset( );
		bool findPath = false;
		addOpenList( srcNode );
		while( openList.getCount( ) > 0 )
		{
			if ( mBestNode == NULL )
				mBestNode = findBestNode( );

			CNSNavNode* findNode = mBestNode;
			openList.erase( mBestNode->mNodeID );
			mBestNode = findBestNode( );

			if ( findNode == desNode )
			{
				desNode->mPrev = findNode->mPrev;
				findPath = true;
				break;
			}

			for (int i = 0; i < 3; i ++) 
			{
				CNSNavNode* node = findNode->mNodeSib [i];
				if ( node == NULL )
					continue;

				bool inOpen = openList.findNode( node->mNodeID ) != NULL;
				bool inClose = closeList.findNode( node->mNodeID ) != NULL;
				if ( inOpen == false && inClose == false )
				{
					node->reset( );
					node->mFValue = node->computeValue( src, des );
					addOpenList( node );
					node->mPrev = findNode;
				}
				else
				{
	/*					if ( inOpen == true )
					{
						float value = node.computeValue( src, des );
						if ( value < node.mFValue )
						{
							node.mFValue = value;
							node.mPrev = findNode;
							if ( bestNode != null && node.getFValue( ) < bestNode.getFValue( ) )
								bestNode = node;
						}
					}
					else if ( inClose == true )
					{
						float value = node.computeValue( src, des );
						if ( value < node.mFValue )
						{
							node.mFValue = value;
							closeList.Remove( node.mNodeID );
							addOpenList( node );
							node.mPrev = findNode;
						}
					}*/
				}
			}

			closeList.insert( findNode->mNodeID, findNode );
		}

		if (findPath == false) 
		{
			// 如果找不到目标，直接走过去，客户端用外挂，服务器也用外挂
			mPathNode.pushback( CNSPathNode( src, srcNode ) );
			mPathNode.pushback( CNSPathNode( des, desNode ) );
		}
		else 
		{
			if ( srcNode == desNode )
			{
				// 如果找到的目标点和起点一样，直接走过去
				mPathNode.pushback( CNSPathNode( src, srcNode ) );
				mPathNode.pushback( CNSPathNode( des, desNode ) );
				return;
			}

			CNSNavNode* tmpNode = desNode;
			for ( ;tmpNode != NULL; tmpNode = tmpNode->mPrev )
			{
				if ( tmpNode->mPrev != NULL )
					tmpNode->mPrev->mNext = tmpNode;
			}
			// 生成拐点
			mPathNode.pushback( CNSPathNode( src, srcNode ) );
			fillPath( src, des, srcNode, desNode );
		}
	}

	void CNSNavLayer::addOpenList( CNSNavNode* node )
	{
		if ( mBestNode == NULL )
			mBestNode = node;
		else 
		{
			if ( mBestNode->getFValue( ) > node->getFValue( ) )
				mBestNode = node;
		}

		openList.insert( node->mNodeID, node );
	}

	CNSNavNode* CNSNavLayer::findBestNode( )
	{
		float minF = FLT_MAX;
		CNSNavNode* bestNode = NULL;
		HLISTINDEX beginIndex = openList.getHead( );
		for ( ; beginIndex != NULL; openList.getNext( beginIndex ) )
		{
			CNSNavNode* node = openList.getValue( beginIndex );
			if ( bestNode == NULL )
			{
				minF = node->getFValue( );
				bestNode = node;
			}
			else if ( minF > node->getFValue( ) )
			{
				minF = node->getFValue ( );
				bestNode = node;
			}
		}
		return bestNode;
	}

	// 判断直线是否穿透所有节点(从srcNode到desNode, navnode之间通过mPrev,mNext链接)
	bool CNSNavLayer::lineCross( const CNSVector2& src, const CNSVector2& des, CNSNavNode* srcNode, CNSNavNode* desNode )
	{
		bool findPath = true;
		CNSNavNode* node = srcNode;
		for (; node != desNode && node->mNext != NULL; node = node->mNext)
		{
			CNSNavNode* node1 	= node;
			CNSNavNode* node2 	= node->mNext;
			CNSNavEdge* edge 	= node1->findConnectEdge( node2 );
			if ( edge == NULL )
			{
				NSLog::log( _UTF8( "nodeid - %d 和 nodeid - %d 没有找到共边" ), node1->mNodeID, node2->mNodeID );
				return false;
			}

			if ( edge->getSegment( ).intersectLine( CNSSegment( src, des ) ) == false )
			{
				findPath = false;
				break;
			}
		}

		return findPath;
	}

	// 寻找拐点
	void CNSNavLayer::fillPath( const CNSVector2& src, const CNSVector2& des, CNSNavNode* srcNode, CNSNavNode* desNode )
	{
		if (lineCross (src, des, srcNode, desNode) == true) 
		{
			if ( mPathNode.getCount( ) >= 2 )
			{
				CNSPathNode& pathNode = mPathNode[ mPathNode.getCount( ) - 2 ];
				if ( lineCross( pathNode.mPathPos, des, pathNode.mPathNode, desNode ) == true )
					mPathNode.erase( mPathNode.getCount( ) - 1 );
			}

			mPathNode.pushback( CNSPathNode( des, desNode ) );
			return;
		}

		CNSVector2 lastFindPos		= CNSVector2::zero;
		CNSNavNode* lastFindNode 	= NULL;
		CNSNavNode* tmpNode 		= srcNode;
		for (; tmpNode->mNext != NULL; tmpNode = tmpNode->mNext) 
		{
			CNSNavNode* node1 	= tmpNode;
			CNSNavNode* node2 	= tmpNode->mNext;
			CNSNavEdge* edge 	= node1->findConnectEdge (node2);
			CNSVector2 pos 		= edge->getSegment( ).findCloset( src, des );
			if (lineCross (src, pos, srcNode, node2) == true)
			{
				lastFindPos = pos;
				lastFindNode = node2;
				continue;
			}
			else
			{
				if ( mPathNode.getCount( ) >= 2 )
				{
					CNSPathNode& pathNode = mPathNode[ mPathNode.getCount( ) - 2 ];
					if ( lineCross( pathNode.mPathPos, des, pathNode.mPathNode, desNode ) == true )
						mPathNode.erase( mPathNode.getCount( ) - 1 );
				}

				mPathNode.pushback( CNSPathNode( des, desNode ) );
				fillPath ( lastFindPos, des, lastFindNode, desNode );
				return;
			}
		}

		if ( mPathNode.getCount( ) >= 2 )
		{
			CNSPathNode& pathNode = mPathNode[ mPathNode.getCount( ) - 2 ];
			if ( lineCross( pathNode.mPathPos, des, pathNode.mPathNode, desNode ) == true )
				mPathNode.erase( mPathNode.getCount( ) - 1 );
		}

		mPathNode.pushback( CNSPathNode( des, desNode ) );
		fillPath ( lastFindPos, des, lastFindNode, desNode );
	}

	// 添加节点到寻路图中
	void CNSNavLayer::addNode( const CNSString& bspName, unsigned int nodeID, unsigned short p1, unsigned short p2, unsigned short p3 )
	{
		CNSNavBsp** tpBspRef = mBsp.get( bspName );
		if ( tpBspRef == NULL )
			return;

		CNSNavNode* node = new CNSNavNode( (*tpBspRef), nodeID, p1, p2, p3 );
		(*tpBspRef)->addNode( node );
	}
}
//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hl2wars_util_shared.h"
#include "wars_mapboundary.h"

#include "recast/recast_mgr.h"
#include "recast/recast_mesh.h"
#ifdef ENABLE_PYTHON
	#include "srcpy_navmesh.h"
#endif // ENABLE_PYTHON

#ifdef GAME_DLL
	#include "Sprite.h"
	#include "hl2wars_player.h"
#else
	#include "c_hl2wars_player.h"
#endif // GAME_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
// link clientside sprite to CSprite - can remove this if we fixup all the maps to not have _clientside ones (do when we're sure we don't need clientside sprites)
LINK_ENTITY_TO_CLASS( env_sprite_clientside, CSprite );
#endif

static ConVar g_debug_checkthrowtolerance( "g_debug_checkthrowtolerance", "0", FCVAR_REPLICATED );
extern ConVar sv_gravity;

//-----------------------------------------------------------------------------
// Purpose: Toss helper
//-----------------------------------------------------------------------------
Vector VecCheckThrowTolerance( CBaseEntity *pEdict, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flTolerance, int iCollisionGroup )
{
	flSpeed = Max( 1.0f, flSpeed );

	float ent_gravity = pEdict && pEdict->GetGravity() ? pEdict->GetGravity() : 1.0f;

	float flGravity = sv_gravity.GetFloat() * ent_gravity;

	Vector vecGrenadeVel = (vecSpot2 - vecSpot1);

	// throw at a constant time
	float time = vecGrenadeVel.Length( ) / flSpeed;
	vecGrenadeVel = vecGrenadeVel * (1.0 / time);

	// adjust upward toss to compensate for gravity loss
	vecGrenadeVel.z += flGravity * time * 0.5;

	Vector vecApex = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);

	CWarsBulletsFilter traceFilter( pEdict, iCollisionGroup );
	traceFilter.SetPassEntity( pEdict->MyUnitPointer() && pEdict->MyUnitPointer()->GetGarrisonedBuilding() ? pEdict->MyUnitPointer()->GetGarrisonedBuilding() : pEdict );

	trace_t tr;
	UTIL_TraceLine( vecSpot1, vecApex, MASK_SOLID, &traceFilter, &tr );
	if (tr.fraction != 1.0)
	{
		// fail!
		if ( g_debug_checkthrowtolerance.GetBool() )
		{
			NDebugOverlay::Line( vecSpot1, vecApex, 255, 0, 0, true, 5.0 );
		}

		return vec3_origin;
	}

	if ( g_debug_checkthrowtolerance.GetBool() )
	{
		NDebugOverlay::Line( vecSpot1, vecApex, 0, 255, 0, true, 5.0 );
	}

	UTIL_TraceLine( vecApex, vecSpot2, MASK_SOLID_BRUSHONLY, pEdict, iCollisionGroup, &tr );
	if ( tr.fraction != 1.0 )
	{
		bool bFail = true;

		// Didn't make it all the way there, but check if we're within our tolerance range
		if ( flTolerance > 0.0f )
		{
			float flNearness = ( tr.endpos - vecSpot2 ).LengthSqr();
			if ( flNearness < Square( flTolerance ) )
			{
				if ( g_debug_checkthrowtolerance.GetBool() )
				{
					NDebugOverlay::Sphere( tr.endpos, vec3_angle, flTolerance, 0, 255, 0, 0, true, 5.0 );
				}

				bFail = false;
			}
		}
		
		if ( bFail )
		{
			if ( g_debug_checkthrowtolerance.GetBool() )
			{
				NDebugOverlay::Line( vecApex, vecSpot2, 255, 0, 0, true, 5.0 );
				NDebugOverlay::Sphere( tr.endpos, vec3_angle, flTolerance, 255, 0, 0, 0, true, 5.0 );
			}
			return vec3_origin;
		}
	}

	if ( g_debug_checkthrowtolerance.GetBool() )
	{
		NDebugOverlay::Line( vecApex, vecSpot2, 0, 255, 0, true, 5.0 );
	}

	return vecGrenadeVel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UTIL_ListPlayersForOwnerNumber( int ownernumber, CUtlVector< CHL2WarsPlayer * > &players )
{
	for( int i = 1; i < gpGlobals->maxClients + 1; i++ )
	{
		CHL2WarsPlayer *pPlayer = ToHL2WarsPlayer( UTIL_PlayerByIndex( i ) );
#ifdef CLIENT_DLL
		if( !pPlayer )
#else
		if( !pPlayer || !pPlayer->IsConnected() )
#endif // CLIENT_DLL
			continue;

		if( pPlayer->GetOwnerNumber() != ownernumber )
			continue;

		players.AddToTail( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
QAngle UTIL_CalculateDirection( const Vector &point1, const Vector &point2 )
{
	float diff_x = point1.x - point2.x;
	float diff_y = point1.y - point2.y;
	if( diff_x == 0 )
		return vec3_angle;
	float yaw = atan( diff_y / diff_x ) * ( 180 / M_PI );

	// fix up yaw if needed
	if( diff_x > 0 && diff_y > 0 )
		yaw += 180.0;
	else if( diff_x > 0 )
		yaw += 180.0;
    
	return QAngle(0.0, yaw, 0.0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UTIL_FindPositionInRadius( positioninradius_t &info, CBaseEntity *navMeshEnt )
{
	Vector vecDir, vecTest;
	trace_t tr;
	for(; info.m_vFan.y < 360 - info.m_fStepSize; info.m_vFan.y += info.m_fStepSize )
	{
		AngleVectors( info.m_vFan, &vecDir );

		vecTest = info.m_vStartPosition + vecDir * info.m_fRadius;

		Vector vEndPos;

#ifdef ENABLE_PYTHON
		if( RecastMgr().HasMeshes() )
		{

			vEndPos = NavMeshGetPositionNearestNavArea( vecTest, info.m_fBeneathLimit, 128.0f, navMeshEnt );
            if( vEndPos == vec3_origin || NavMeshGetPathDistance( vecTest, vEndPos, 10000.0f, navMeshEnt ) < 0 )
                continue;
		}
		else
#endif // ENABLE_PYTHON
		{
			UTIL_TraceLine( vecTest, vecTest - Vector( 0, 0, MAX_TRACE_LENGTH ), MASK_SHOT, info.m_hIgnore.Get(), COLLISION_GROUP_NONE, &tr );
			if( tr.fraction == 1.0 )
				continue;
			vEndPos = tr.endpos;
		}
            
		if( info.m_bTestPosition )
		{
			// Test position
			vEndPos.z += -info.m_vMins.z + info.m_fZOffset;
			UTIL_TraceHull( vEndPos,
							vEndPos + Vector( 0, 0, 10 ),
							info.m_vMins,
							info.m_vMaxs,
							info.m_iMask,
							info.m_hIgnore.Get(),
							COLLISION_GROUP_NONE,
							&tr );

			if( tr.fraction == 1.0 )
			{
				info.m_vPosition = tr.endpos;
				info.m_bSuccess = true;
				info.m_vFan.y += info.m_fStepSize;
				return;   
			}
		}
		else
		{
			info.m_vPosition = vEndPos;
			info.m_vPosition.z += info.m_fZOffset;
			info.m_bSuccess = true;
			info.m_vFan.y += info.m_fStepSize;
			return;
		}
	}

	info.m_bSuccess = false;
}

void UTIL_FindPosition( positioninfo_t &info, CBaseEntity *navMeshEnt )
{
	trace_t tr;
	if( info.m_fRadius == 0 )
	{
		// Find a start pos
		Vector vEndPos;

		if( RecastMgr().HasMeshes() )
		{
#ifdef ENABLE_PYTHON
			vEndPos = NavMeshGetPositionNearestNavArea( info.m_vPosition, info.m_fBeneathLimit, info.m_fMaxRadius, navMeshEnt );
			if( vEndPos == vec3_origin && NavMeshGetPathDistance( info.m_vPosition, vEndPos, 10000.0f, navMeshEnt ) >= 0 )
				vEndPos = info.m_vPosition;
		}
#endif // ENABLE_PYTHON
		else
		{
			UTIL_TraceLine( info.m_vPosition, info.m_vPosition - Vector( 0, 0, MAX_TRACE_LENGTH ), MASK_SHOT, info.m_hIgnore.Get(), COLLISION_GROUP_NONE, &tr );
			vEndPos = ( tr.fraction == 1.0 ) ? tr.endpos : vec3_origin;
			if( vEndPos == vec3_origin )
				vEndPos = info.m_vPosition;
		}

		info.m_fRadius += info.m_fRadiusGrow;

		// Test pos
		if( info.m_bTestPosition )
		{
			vEndPos.z += -info.m_vMins.z + info.m_fZOffset;
			UTIL_TraceHull( vEndPos,
							vEndPos + Vector( 0, 0, 10 ),
							info.m_vMins,
							info.m_vMaxs,
							info.m_iMask,
							info.m_hIgnore,
							COLLISION_GROUP_NONE,
							&tr );
			if( tr.fraction == 1.0 )
			{
				info.m_vPosition = tr.endpos;
				info.m_bSuccess = true;
				return;
			}
		}
		else
		{
			info.m_vPosition = vEndPos;
			info.m_vPosition.z += info.m_fZOffset;
			info.m_bSuccess = true;
			return;
		}
	}

	while( info.m_fRadius <= info.m_fMaxRadius )
	{
		info.m_InRadiusInfo.m_fRadius = info.m_fRadius;
		info.m_InRadiusInfo.ComputeRadiusStepSize();

        UTIL_FindPositionInRadius(info.m_InRadiusInfo, navMeshEnt);
        if( info.m_InRadiusInfo.m_bSuccess )
		{
            info.m_vPosition = info.m_InRadiusInfo.m_vPosition;
            info.m_bSuccess = true;
            return;
		}
        info.m_InRadiusInfo.m_vFan.Init(0,0,0);
        info.m_fRadius += info.m_fRadiusGrow;
	}
    info.m_bSuccess = false;
}

#ifdef CLIENT_DLL
Vector UTIL_GetMouseAim( C_HL2WarsPlayer *pPlayer, int x, int y )
{
	Vector forward;
	// Remap x and y into -1 to 1 normalized space 
	float xf, yf; 
	xf = ( 2.0f * x / (float)(ScreenWidth()-1) ) - 1.0f; 
	yf = ( 2.0f * y / (float)(ScreenHeight()-1) ) - 1.0f; 

	// Flip y axis 
	yf = -yf; 

	const VMatrix &worldToScreen = engine->WorldToScreenMatrix(); 
	VMatrix screenToWorld; 
	MatrixInverseGeneral( worldToScreen, screenToWorld ); 

	// Create two points at the normalized mouse x, y pos and at the near and far z planes (0 and 1 depth) 
	Vector v1, v2; 
	v1.Init( xf, yf, 0.0f ); 
	v2.Init( xf, yf, 1.0f ); 

	Vector o2; 
	// Transform the two points by the screen to world matrix 
	screenToWorld.V3Mul( v1, pPlayer->Weapon_ShootPosition() ); // ray start origin 
	screenToWorld.V3Mul( v2, o2 ); // ray end origin 
	VectorSubtract( o2, pPlayer->Weapon_ShootPosition(), forward ); 
	forward.NormalizeInPlace(); 
	return forward;
}

inline void TestPoints( const Vector &vTestPoint, int &minsx, int &minsy, int &maxsx, int &maxsy )
{
	int tempx, tempy;
	GetVectorInScreenSpace(vTestPoint, tempx, tempy); 
	if( tempx < minsx )
		minsx = tempx;
	if( tempx > maxsx )
		maxsx = tempx;
	if( tempy < minsy )
		minsy = tempy;
	if( tempy > maxsy )
		maxsy = tempy;
}

void UTIL_GetScreenMinsMaxs( const Vector &origin, const QAngle &angle, const Vector &mins, const Vector &maxs, 
							int &minsx, int &minsy, int &maxsx, int &maxsy )
{
	Vector vTestPoint, vTransformedMins, vTransformedMaxs;
	matrix3x4_t xform;

	// Transform
    AngleMatrix( angle, origin, xform );
    TransformAABB( xform, mins, maxs, vTransformedMins, vTransformedMaxs );

	//NDebugOverlay::BoxAngles( origin, mins, maxs, angle, 255, 0, 0, 200, 0.1 );
	NDebugOverlay::Cross3D( vTransformedMins, -Vector(16, 16, 16), Vector(16, 16, 16), 255, 0, 0, false, 0.1 );
	NDebugOverlay::Cross3D( vTransformedMaxs, -Vector(16, 16, 16), Vector(16, 16, 16), 0, 255, 0, false, 0.1 );

	// Init points
	minsx = INT_MAX;
	maxsx = -1;
	minsy = INT_MAX;
	maxsy = -1;

	// Test points
	// find the smallest and biggest values
	TestPoints( vTransformedMins, minsx, minsy, maxsx, maxsy ); 
	TestPoints( vTransformedMaxs, minsx, minsy, maxsx, maxsy );

	/*TestPoints( Vector(vTransformedMins.x, vTransformedMins.y, vTransformedMaxs.z), minsx, minsy, maxsx, maxsy ); 
	TestPoints( Vector(vTransformedMaxs.x, vTransformedMins.y, vTransformedMaxs.z), minsx, minsy, maxsx, maxsy ); 
	TestPoints( Vector(vTransformedMins.x, vTransformedMaxs.y, vTransformedMaxs.z), minsx, minsy, maxsx, maxsy ); */

	TestPoints( Vector(vTransformedMaxs.x, vTransformedMaxs.y, vTransformedMins.z), minsx, minsy, maxsx, maxsy ); 
	TestPoints( Vector(vTransformedMaxs.x, vTransformedMins.y, vTransformedMins.z), minsx, minsy, maxsx, maxsy ); 
	TestPoints( Vector(vTransformedMins.x, vTransformedMaxs.y, vTransformedMins.z), minsx, minsy, maxsx, maxsy );
}

#endif // CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTraceFilterSkipFriendly::CTraceFilterSkipFriendly( const IHandleEntity *passentity, 
												   int collisionGroup, CUnitBase *pUnit )
	: CTraceFilterSimple( passentity, collisionGroup ), m_pUnit(pUnit)
{
}

bool CTraceFilterSkipFriendly::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity )
		return false;

	CUnitBase *pUnit = pEntity->MyUnitPointer();
	if( m_pUnit && pUnit && m_pUnit->IRelationType(pUnit) == D_LI )
	{
		return false;
	}

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

CTraceFilterSkipEnemies::CTraceFilterSkipEnemies( const IHandleEntity *passentity, int collisionGroup, CUnitBase *pUnit )
	: CTraceFilterSimple( passentity, collisionGroup ), m_pUnit(pUnit)
{
}

bool CTraceFilterSkipEnemies::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity )
		return false;

	CUnitBase *pUnit = pEntity->MyUnitPointer();
	if( m_pUnit && pUnit && m_pUnit->IRelationType(pUnit) == D_HT )
	{
		return false;
	}

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

bool CTraceFilterWars::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	// Only collide with the map boundaries!
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity || !(dynamic_cast<CBaseFuncMapBoundary *>(pEntity)) )
		return false;
	return true;
}

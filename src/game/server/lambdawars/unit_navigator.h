//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef UNIT_NAVIGATOR_H
#define UNIT_NAVIGATOR_H

#ifdef _WIN32
#pragma once
#endif

#include "unit_component.h"
#include "unit_locomotion.h"
#include "nav.h"

// Debug
#define NavDbgMsg( fmt, ... )				\
	if( unit_navigator_debug.GetBool() )	\
		DevMsg( fmt, ##__VA_ARGS__ );		\

extern ConVar unit_navigator_debug;
extern ConVar unit_navigator_debug_inrange;

// Forward declarations
class UnitBaseMoveCommand;
class CNavArea;
class CRecastMesh;

// Goal types
enum UnitGoalTypes
{
	GOALTYPE_NONE = 0,
	GOALTYPE_INVALID,
	GOALTYPE_POSITION,
	GOALTYPE_TARGETENT,
	GOALTYPE_POSITION_INRANGE,
	GOALTYPE_TARGETENT_INRANGE,
};

// Goal flags
enum UnitGoalFlags
{
	GF_NOCLEAR = (1 << 0),
	GF_REQTARGETALIVE = (1 << 1),
	GF_USETARGETDIST = (1 << 2 ),
	GF_NOLOSREQUIRED = (1 << 3 ), // No line check (just get in sense range)
	GF_REQUIREVISION = (1 << 4 ), // Fog of war
	GF_OWNERISTARGET = (1 << 5 ), // Bumped into entity that is being owned by the target
	GF_DIRECTPATH = (1 << 6), // Build direct path, don't do path finding. For special cases.
	GF_ATGOAL_RELAX = (1 << 7) // For use with GF_NOCLEAR: 
};


// Status of the current goal if any.
enum CheckGoalStatus_t
{
	CHS_NOGOAL = 0,
	CHS_HASGOAL, // Got a goal and moving towards it
	CHS_ATGOAL, // At goal, will dispatch success at the end of the navigation update (unless noclear is specified)
	CHS_FAILED, // Goal failed due some reason (target died, blocked resolving failed, etc)
	CHS_CLIMB, // Start waypoint at which we should start climbing (placed in the from area)
	CHS_CLIMBDEST, // End point of climbing (placed into the new area)
	CHS_EDGEDOWN, // Fall down, likely a small gap between the two navigation meshes too.
	CHS_EDGEDOWNDEST,
	CHS_NOSIMPLIFY, // For waypoints, indicates reactive path should not skip this point
};

// Status of being blocked
enum BlockedStatus_t
{
	BS_NONE = 0,
	BS_LITTLE,
	BS_MUCH,
	BS_STUCK,
	BS_GIVEUP, 
};

//-----------------------------------------------------------------------------
// Purpose: Waypoint class
//-----------------------------------------------------------------------------
class UnitBaseWaypoint
{
public:
	UnitBaseWaypoint()
	{
		V_memset( this, 0, sizeof(*this) );
		vecLocation	= vec3_invalid;
	}
	UnitBaseWaypoint( const Vector &vecPosition, float flYaw=0.0f )
	{
		V_memset( this, 0, sizeof(*this) );
		vecLocation = vecPosition;
		flYaw = flYaw;
	}
	UnitBaseWaypoint( const UnitBaseWaypoint &from )
	{
		V_memcpy( this, &from, sizeof(*this) );
		pNext = pPrev = NULL;
	}

	UnitBaseWaypoint &operator=( const UnitBaseWaypoint &from )
	{
		V_memcpy( this, &from, sizeof(*this) );
		pNext = pPrev = NULL;
		return *this;
	}

	~UnitBaseWaypoint()
	{
		if ( pNext )
		{
			pNext->pPrev = pPrev;
		}
		if ( pPrev )
		{
			pPrev->pNext = pNext;
		}
	}

	//---------------------------------

	void					SetNext( UnitBaseWaypoint *p );
	UnitBaseWaypoint *		GetNext()				{ return pNext; }
	const UnitBaseWaypoint *GetNext() const			{ return pNext; }

	UnitBaseWaypoint *		GetPrev()				{ return pPrev; }
	const UnitBaseWaypoint *GetPrev() const			{ return pPrev; }

	UnitBaseWaypoint *		GetLast();

	//---------------------------------

	const Vector &		GetPos() const				{ return vecLocation; }
	void 				SetPos(const Vector &newPos) { vecLocation = newPos; }


	//---------------------------------
	//
	// Basic info
	//
	Vector			vecLocation;
	float			flYaw;				// Waypoint facing dir

	CheckGoalStatus_t SpecialGoalStatus; // Leave 0 to be ignored

#if 0 // TODO: REMOVE
	//---------------------------------
	//
	// Nav Area info
	//
	CNavArea *pFrom;
	CNavArea *pTo;
#endif // 0

private:
	UnitBaseWaypoint *pNext;
	UnitBaseWaypoint *pPrev;
};

inline void UnitBaseWaypoint::SetNext( UnitBaseWaypoint *p )	
{ 
	if (pNext) 
	{
		pNext->pPrev = NULL; 
	}

	pNext = p; 

	if ( pNext ) 
	{
		if ( pNext->pPrev )
			pNext->pPrev->pNext = NULL;

		pNext->pPrev = this; 
	}
}

// Waypoint flags
enum UnitPathFlags
{
	UNITPATH_FLAGS_NONE = 0,
	UNITPATH_FLAGS_PARTIAL = 0x1,
	UNITPATH_FLAGS_ISDIRECT = 0x2
};

//-----------------------------------------------------------------------------
// UnitBasePath: Maintains the list of waypoints to a goal and other related info
//-----------------------------------------------------------------------------
class UnitBasePath
{
public:
	UnitBasePath() : m_pWaypointHead(NULL)
	{
		Clear();
	}
	UnitBasePath( const UnitBasePath &src )
	{
		m_iGoalType = src.m_iGoalType;
		m_vGoalPos = src.m_vGoalPos;
		m_waypointTolerance = src.m_waypointTolerance;
		m_fGoalTolerance = src.m_fGoalTolerance;
		m_iGoalFlags = src.m_iGoalFlags;
		m_fMinRange = src.m_fMinRange;
		m_fMaxRange = src.m_fMaxRange;
		m_hTarget = src.m_hTarget;
		m_bAvoidEnemies = src.m_bAvoidEnemies;
		m_vStartPosition = src.m_vStartPosition;
		m_fMaxMoveDist = src.m_fMaxMoveDist;
		m_bSuccess = src.m_bSuccess;
		m_bIsDirectPath = src.m_bIsDirectPath;
		m_iFlags = src.m_iFlags;
#if defined(ENABLE_PYTHON) && defined(SRCPY_MOD_ENTITIES)
		m_fnCustomLOSCheck = src.m_fnCustomLOSCheck;
#endif // ENABLE_PYTHON

		// Copy waypoints
		if( src.m_pWaypointHead )
		{
			UnitBaseWaypoint *pPrev, *pCur;
			m_pWaypointHead = new UnitBaseWaypoint(*src.m_pWaypointHead);
			pPrev = m_pWaypointHead;
			pCur = m_pWaypointHead->GetNext();
			while( pCur )
			{
				pCur = new UnitBaseWaypoint(*pCur);
				pPrev->SetNext(pCur);
				pPrev = pCur;
				pCur = pCur->GetNext();
			}
		}
		else
		{
			m_pWaypointHead = NULL;
		}
	}

	~UnitBasePath()
	{
		while( m_pWaypointHead )
		{
			UnitBaseWaypoint *pCur = m_pWaypointHead;
			m_pWaypointHead = m_pWaypointHead->GetNext();
			delete pCur;
		}
	}

	bool CurWaypointIsGoal() 
	{
		if( m_pWaypointHead && m_pWaypointHead->GetNext() == NULL )
			return true;
		return false;
	}

	UnitBaseWaypoint *GetCurWaypoint() { return m_pWaypointHead; }

	inline float GetToleranceCurWaypoint() 
	{
		return CurWaypointIsGoal() ? m_fGoalTolerance : m_waypointTolerance;
	}

	void Clear()
	{
		m_iGoalType = GOALTYPE_NONE;
		m_iGoalFlags = 0;
		m_hTarget = NULL;
		m_bAvoidEnemies = true;
		m_fMaxMoveDist = 0;
		m_bIsDirectPath = false;
		m_iFlags = 0;
		SetWaypoint(NULL);
	}

	void SetWaypoint( UnitBaseWaypoint *pWayPoint )
	{
		while( m_pWaypointHead )
		{
			UnitBaseWaypoint *pCur = m_pWaypointHead;
			m_pWaypointHead = m_pWaypointHead->GetNext();
			delete pCur;
		}

		m_pWaypointHead = pWayPoint;
	}

	void Advance()
	{
		if ( CurWaypointIsGoal() )
			return;

		// -------------------------------------------------------
		// If I have another waypoint advance my path
		// -------------------------------------------------------
		if (GetCurWaypoint()->GetNext()) 
		{
			UnitBaseWaypoint *pNext = GetCurWaypoint()->GetNext();

			delete GetCurWaypoint();
			m_pWaypointHead = pNext;

			return;
		}
		// -------------------------------------------------
		//  This is an error catch that should *not* happen
		//  It means a route was created with no goal
		// -------------------------------------------------
		else 
		{
			DevMsg( "!!ERROR!! Force end of route with no goal!\n");
		}
	}

	// Tests if two paths have similar goals
	bool HasSamegoal( UnitBasePath *pPath )
	{
		// Note: don't test m_iGoalType
		if( m_hTarget )
		{
			if( m_hTarget == pPath->m_hTarget )
				return true;
		}
		return false;
	}

	void SetTarget( CBaseEntity *pTarget ) { m_hTarget = pTarget; }
	CBaseEntity *GetTarget() { return m_hTarget; }

	inline bool IsPartialPath() { return (m_iFlags & UNITPATH_FLAGS_PARTIAL) != 0; }

public:
	int m_iGoalType; // UnitGoalTypes
	UnitBaseWaypoint *m_pWaypointHead;
	Vector m_vGoalPos;
	float m_waypointTolerance; // Default minimum tolerance for waypoints
	float m_fGoalTolerance; // Tolerance within it is OK to say we are at our goal
	float m_fAtGoalTolerance; // Tolerance before starting to follow the target unit again (GF_NOCLEAR + GF_ATGOAL_RELAX)
	int m_iGoalFlags; // UnitGoalFlags
	float m_fMinRange, m_fMaxRange;
	EHANDLE m_hTarget;
	bool m_bAvoidEnemies;
	Vector m_vStartPosition; // The initial start position of the unit
	float m_fMaxMoveDist;
	bool m_bSuccess; // Can be queried after path completion by other components
	bool m_bIsDirectPath;
	int m_iFlags;
#if defined(ENABLE_PYTHON) && defined(SRCPY_MOD_ENTITIES)
	boost::python::object m_fnCustomLOSCheck; // Allows using a custom Python based los check
	boost::python::object m_pathContext; // Allows setting a context for this path. Could be anything.
#endif // ENABLE_PYTHON
};

#define CONSIDER_SIZE 48
#define MAX_TESTDIRECTIONS 8

//-----------------------------------------------------------------------------
// Purpose: Navigation class. Path building/updating and local obstacle avoidance.
//-----------------------------------------------------------------------------
class UnitBaseNavigator : public UnitComponent
{
	DECLARE_CLASS( UnitBaseNavigator, UnitComponent );
public:
	friend class CUnitBase;

#if defined(ENABLE_PYTHON) && defined(SRCPY_MOD_ENTITIES)
	UnitBaseNavigator( boost::python::object outer );
#endif // ENABLE_PYTHON

	// Core
	virtual void		Reset();
	virtual void		StopMoving();
	virtual void		Update( UnitBaseMoveCommand &mv );
	virtual void		UpdateIdealAngles( UnitBaseMoveCommand &MoveCommand, Vector *pathdir = NULL );
	float				GetDensityMultiplier();

	int					GetTestRouteMask();
	void				SetTestRouteMask( int mask );

	// Goals
	virtual bool		SetGoal( Vector &destination, float goaltolerance=64.0f, int goalflags=0, bool avoidenemies=true );
	virtual bool		SetGoalTarget( CBaseEntity *pTarget, float goaltolerance=64.0f, int goalflags=0, bool avoidenemies=true );
	virtual bool		SetGoalInRange( Vector &destination, float maxrange, float minrange=0.0f, float goaltolerance=0.0f, int goalflags=0, bool avoidenemies=true );
	virtual bool		SetGoalTargetInRange( CBaseEntity *pTarget, float maxrange, float minrange=0.0f, float goaltolerance=0.0f, int goalflags=0, bool avoidenemies=true );

	virtual void		UpdateGoalInRange( float maxrange, float minrange=0.0f, UnitBasePath *path = NULL );
	virtual void		UpdateGoalTarget( CBaseEntity *target, UnitBasePath *path = NULL );
	virtual void		UpdateGoalInfo( void );
	virtual float		GetGoalDistance( void );

	// Path finding
#if defined(ENABLE_PYTHON) && defined(SRCPY_MOD_ENTITIES)
	virtual boost::python::object FindPathAsResult( int goaltype, const Vector &vDestination, float fGoalTolerance, int goalflags=0, float fMinRange=0.0f, float fMaxRange=0.0f, 
									CBaseEntity *pTarget=NULL, bool bAvoidEnemies=true );
#endif // ENABLE_PYTHON

	// Getters/Setters for path
#if defined(ENABLE_PYTHON) && defined(SRCPY_MOD_ENTITIES)
	void SetPath( boost::python::object path );
	inline boost::python::object PyGetPath() { return m_refPath; }
#endif // ENABLE_PYTHON
	inline UnitBasePath *GetPath() { return m_pPath; }

	// Facing
	float				GetIdealYaw();
	void				SetIdealYaw( float fIdealYaw );
	CBaseEntity *		GetFacingTarget();
	void				SetFacingTarget( CBaseEntity *pFacingTarget );
	const Vector &		GetFacingTargetPos();
	void				SetFacingTargetPos( Vector &vFacingTargetPos );

	// Debug
	virtual void		DrawDebugRouteOverlay();	
	virtual void		DrawDebugInfo();

protected:
	void				DispatchOnNavComplete();
	void				DispatchOnNavFailed();
	void				DispatchOnNavAtGoal();
	void				DispatchOnNavLostGoal();

	virtual void		UpdateFacingTargetState( bool bIsFacing );
	virtual void		UpdateGoalStatus( UnitBaseMoveCommand &MoveCommand, CheckGoalStatus_t GoalStatus );
	virtual void		CalcMove( UnitBaseMoveCommand &MoveCommand, QAngle angles, float speed );

	// Flow logic related
	const Vector &		GetWishVelocity() { return m_vLastWishVelocity; }

	virtual float		GetEntityBoundingRadius( CBaseEntity *pEnt );
	virtual void		RegenerateConsiderList( UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, CheckGoalStatus_t GoalStatus, bool bForceIncludeAll = false );
	virtual bool		ShouldConsiderNavMesh( void );

	virtual void		CollectConsiderEntities( UnitBaseMoveCommand &MoveCommand, CheckGoalStatus_t GoalStatus, bool bForceIncludeAll = false );
	virtual void		ComputeConsiderDensAndDirs( UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, CheckGoalStatus_t GoalStatus );
	virtual bool		IsEntityNavIgnored( UnitBaseMoveCommand &MoveCommand, CBaseEntity *pEnt );

	virtual float		ComputeDensityAndAvgVelocity( int iPos, UnitBaseMoveCommand &MoveCommand );
	float				GetEntityDensity( const Vector &vPos, CBaseEntity *pEnt );

	float				ComputeUnitCost( int iPos, Vector *pFinalVelocity, CheckGoalStatus_t GoalStatus, 
								UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, const float &fWaypointDist, float &fDensity );
	Vector				ComputeVelocity( CheckGoalStatus_t GoalStatus, UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, const float &fWaypointDist );

	float				CalculateAvgDistHistory();

	virtual	void		InsertSeed( const Vector &vPos );

	// Path updating
	virtual CheckGoalStatus_t UpdateGoalAndPath( UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, float &fWaypointDist );
	virtual bool		IsInRangeGoal( UnitBaseMoveCommand &MoveCommand );
	virtual CheckGoalStatus_t	MoveUpdateWaypoint( UnitBaseMoveCommand &MoveCommand );
	virtual void		AdvancePath();
	virtual bool		UpdateReactivePath( bool bNoRecomputePath = false );
	virtual float		ComputeWaypointDistanceAndDir( Vector &vPathDir );

	bool				TestNearbyUnitsWithSameGoal( UnitBaseMoveCommand &MoveCommand );

	BlockedStatus_t		GetBlockedStatus( void );
	virtual void		ResetBlockedStatus( void );
	virtual void		UpdateBlockedStatus( UnitBaseMoveCommand &MoveCommand, const float &fWaypointDist );

	// Goals
	CheckGoalStatus_t	GetGoalStatus();
	CHandle<CUnitBase>	GetAtGoalDependencyEnt( void ) { return m_hAtGoalDependencyEnt; }

	// Path finding
	virtual bool		FindPath( int goaltype, const Vector &vDestination, float fGoalTolerance, int goalflags=0, float fMinRange=0.0f, float fMaxRange=0.0f, 
									CBaseEntity *pTarget=NULL, bool bAvoidEnemies=true );
	virtual bool		DoFindPathToPos( UnitBasePath *pPath );
	virtual bool		DoFindPathToPosInRange( UnitBasePath *pPath );
	virtual bool		DoFindPathToPos();
	virtual bool		DoFindPathToPosInRange();

	// Route buiding
	virtual CRecastMesh *GetNavMesh();
	virtual bool		FindPathInternal( UnitBasePath *pPath, int goaltype, const Vector &vDestination, float fGoalTolerance, int goalflags=0, float fMinRange=0.0f, float fMaxRange=0.0f, 
									CBaseEntity *pTarget=NULL, bool bAvoidEnemies=true );
	virtual UnitBaseWaypoint *	BuildLocalPath( UnitBasePath *pPath, const Vector &pos );
	virtual UnitBaseWaypoint *	BuildNavAreaPath( UnitBasePath *pPath, const Vector &pos );
	virtual UnitBaseWaypoint *	BuildRoute( UnitBasePath *pPath );

	Vector						ComputePathVelocity( const UnitBaseMoveCommand &MoveCommand, const Vector &vPathDir, float fDist );

public:
	// Facing settings
	float m_fIdealYawTolerance;
	float m_fFacingCone;

	// Override goal / path velocity
	Vector m_vForceGoalVelocity;

	// Override avoidance behavior
	bool m_bNoAvoid;

	// Override move in path direction behavior
	bool m_bNoPathVelocity;

	// Don't slow down when nearing target entity
	bool m_bNoSlowDownToTarget;

protected:
	CheckGoalStatus_t m_LastGoalStatus;

private:
	// =============================
	// Facing variables
	// =============================
	float m_fIdealYaw;
	EHANDLE m_hFacingTarget;
	Vector m_vFacingTargetPos;
	bool m_bFacingFaceTarget;

	// =============================
	// Path and goal variables
	// =============================
	UnitBasePath *m_pPath;
#if defined(ENABLE_PYTHON) && defined(SRCPY_MOD_ENTITIES)
	boost::python::object m_refPath;
#endif // ENABLE_PYTHON
	float m_fGoalDistance;
	Vector m_vGoalDir;

	// =============================
	// Position checking variables
	// =============================
	BlockedStatus_t m_BlockedStatus;
	float m_fBlockedStartTime;
	int m_iBlockedPathRecomputations;
	float m_fBlockedNextPositionCheck;
	float m_fLastWaypointDistance;
	bool m_bBlockedLongDistanceDetected;
	float m_fLowVelocityStartTime;
	bool m_bLowVelocityDetectionActive;

	float m_fLastPathRecomputation;
	bool m_bReactivePathBlocked;
	float m_fReactivePathBlockedStartTime;
	float m_fNextAllowPathRecomputeTime;
	bool m_bNoNavAreasNearby;
	float m_fIgnoreNavMeshTime;
	int m_iLastGoalRef; // Used for determing if the path needs to be recomputed to the target entity

	CHandle<CUnitBase> m_hAtGoalDependencyEnt;

protected:
	// =============================
	// Flow variables
	// =============================
	EHANDLE m_ConsiderList[CONSIDER_SIZE];
	int m_iConsiderSize;
	float m_DensitySums[MAX_TESTDIRECTIONS];
	Vector m_vAverageVelocities[MAX_TESTDIRECTIONS];

	Vector m_vTestDirections[MAX_TESTDIRECTIONS];
	Vector m_vTestPositions[MAX_TESTDIRECTIONS];
	int m_iUsedTestDirections;

	float m_fLastBestDensity;
	Vector m_vLastWishVelocity;
	float m_fDiscomfortWeight;

	struct dist_entry_t {
		dist_entry_t( float fDist, float fTimeStamp ) : m_fDist(fDist), m_fTimeStamp(fTimeStamp) {}
		float m_fDist;
		float m_fTimeStamp;
	};
	CUtlVector<dist_entry_t> m_DistHistory;
	float m_fNextAvgDistConsideration;

	struct seed_entry_t {
		seed_entry_t( Vector2D vPos, float fTimeStamp ) : m_vPos(vPos), m_fTimeStamp(fTimeStamp) {}
		Vector2D m_vPos;
		float m_fTimeStamp;
	};
	CUtlVector<seed_entry_t> m_Seeds;

	Vector m_vBlockingDirection;

	int m_iTestRouteMask;

private:
	// =============================
	// Misc variables
	// =============================
	// Climbing
	float m_fClimbHeight;
	Vector m_vecClimbDirection;

	// Debug variables
	Vector m_vDebugVelocity;
	float m_fDebugLastBestCost;
	UnitBaseMoveCommand m_DebugLastMoveCommand;
	int m_iDebugBestDir;

	static int m_iCurPathRecomputations;
};

// ---------------------------------------------------------------
// Inlines
inline float UnitBaseNavigator::GetEntityBoundingRadius( CBaseEntity *pEnt )
{
	return pEnt->CollisionProp()->BoundingRadius2D();
}

//-----------------------------------------------------------------------------
// Purpose: Get density to this entity based on distance
//-----------------------------------------------------------------------------
inline float UnitBaseNavigator::GetEntityDensity( const Vector &vPos, CBaseEntity *pEnt )
{
	VPROF_BUDGET( "UnitBaseNavigator::GetEntityDensity", VPROF_BUDGETGROUP_UNITS );
	return pEnt->DensityMap()->Get(vPos);
}

inline BlockedStatus_t UnitBaseNavigator::GetBlockedStatus( void )
{
	return m_BlockedStatus;
}

inline CheckGoalStatus_t UnitBaseNavigator::GetGoalStatus()
{
	return m_LastGoalStatus;
}

inline float UnitBaseNavigator::GetIdealYaw()
{
	return m_fIdealYaw;
}

inline void UnitBaseNavigator::SetIdealYaw( float fIdealYaw )
{
	m_fIdealYaw = fIdealYaw;
	m_bFacingFaceTarget = false;
}

inline CBaseEntity *UnitBaseNavigator::GetFacingTarget()
{
	return m_hFacingTarget;
}

inline void UnitBaseNavigator::SetFacingTarget( CBaseEntity *pFacingTarget )
{
	m_hFacingTarget = pFacingTarget;
	m_bFacingFaceTarget = false;
}

inline const Vector &UnitBaseNavigator::GetFacingTargetPos()
{
	return m_vFacingTargetPos;
}

inline void UnitBaseNavigator::SetFacingTargetPos( Vector &vFacingTargetPos )
{
	m_vFacingTargetPos = vFacingTargetPos;
	m_bFacingFaceTarget = false;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a path to a position in range of the goal position. 
//			This is the same as DoFindPathToPos for now...
//-----------------------------------------------------------------------------
inline bool UnitBaseNavigator::DoFindPathToPosInRange( UnitBasePath *pPath )
{
	return DoFindPathToPos( pPath );
}

inline bool UnitBaseNavigator::DoFindPathToPosInRange()
{
	return DoFindPathToPosInRange( GetPath() );
}

inline bool UnitBaseNavigator::DoFindPathToPos()
{
	return DoFindPathToPos( GetPath() );
}

inline int UnitBaseNavigator::GetTestRouteMask()
{
	return m_iTestRouteMask;
}

inline void UnitBaseNavigator::SetTestRouteMask( int mask )
{
	m_iTestRouteMask = mask;
}

#endif // UNIT_NAVIGATOR_H

//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientsideeffects.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool g_FXCreationAllowed = false;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void SetFXCreationAllowed( bool state )
{
	g_FXCreationAllowed = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool FXCreationAllowed( void )
{
	return g_FXCreationAllowed;
}

// TODO:  Sort effects and their children back to front from view positon?  At least with buckets or something.

//
//-----------------------------------------------------------------------------
// Purpose: Construct and activate effect
// Input  : *name - 
//-----------------------------------------------------------------------------
CClientSideEffect::CClientSideEffect( const char *name, int flags )
{
	m_pszName = name;
	m_iFlags = flags;
	Assert( name );

	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: Destroy effect
//-----------------------------------------------------------------------------
CClientSideEffect::~CClientSideEffect( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get name of effect
// Output : const char
//-----------------------------------------------------------------------------
const char *CClientSideEffect::GetName( void )
{
	return m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Is effect still active?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CClientSideEffect::IsActive( void )
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: Mark effect for destruction
//-----------------------------------------------------------------------------
void CClientSideEffect::Destroy( void )
{
	m_bActive = false;
}

// =======================================
// PySource Additions
// =======================================
#ifdef ENABLE_PYTHON
//-----------------------------------------------------------------------------
// Purpose: If we can delete this effect
//-----------------------------------------------------------------------------
boost::python::object CClientSideEffect::GetPyInstance()
{
	return m_pyRef;
}
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================

#define MAX_EFFECTS 256

//-----------------------------------------------------------------------------
// Purpose: Implements effects list interface
//-----------------------------------------------------------------------------
class CEffectsList : public IEffectsList
{
public:
					CEffectsList( void );
	virtual			~CEffectsList( void );

	//	Add an effect to the effects list
	void			AddEffect( CClientSideEffect *effect );
	// Remove the specified effect
#ifdef HL2WARS_DLL
	// Draw/update all effects in the current list
	void			DrawEffects( double frametime, int flags = BITS_CLIENTEFFECT_NORMAL );
#else
	// Draw/update all effects in the current list
	void			DrawEffects( double frametime );
#endif // #ifdef HL2WARS_DLL
	// Flush out all effects from the list
	void			Flush( void );
private:
	void			RemoveEffect( int effectIndex );
	// Current number of effects
	int				m_nEffects;
	// Pointers to current effects
	CClientSideEffect *m_rgEffects[ MAX_EFFECTS ];
};

// Implements effects list and exposes interface
static CEffectsList g_EffectsList;
// Public interface
IEffectsList *clienteffects = ( IEffectsList * )&g_EffectsList;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEffectsList::CEffectsList( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEffectsList::~CEffectsList( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Add effect to effects list
// Input  : *effect - 
//-----------------------------------------------------------------------------
void CEffectsList::AddEffect( CClientSideEffect *effect )
{
#if 0
	if ( FXCreationAllowed() == false )
	{
		//NOTENOTE: If you've hit this, you may not add a client effect where you have attempted to.
		//			Most often this means that you have added it in an entity's DrawModel function.
		//			Move this to the ClientThink function instead!

		Assert(0);
		return;
	}
#endif

	if ( effect == NULL )
		return;

	if ( m_nEffects >= MAX_EFFECTS )
	{
		DevWarning( 1, "No room for effect %s\n", effect->GetName() );
		return;
	}

	m_rgEffects[ m_nEffects++ ] = effect;
}

//-----------------------------------------------------------------------------
// Purpose: Remove specified effect by index
// Input  : effectIndex - 
//-----------------------------------------------------------------------------
void CEffectsList::RemoveEffect( int effectIndex )
{
	if ( effectIndex >= m_nEffects || effectIndex < 0 )
		return;

	CClientSideEffect *pEffect = m_rgEffects[effectIndex];
	m_nEffects--;

	if ( m_nEffects > 0 && effectIndex != m_nEffects )
	{
		// move the last one down to fill the empty slot
		m_rgEffects[effectIndex] = m_rgEffects[m_nEffects];
	}

	pEffect->Destroy();

// =======================================
// PySource Additions
// =======================================
#ifdef ENABLE_PYTHON
	if( pEffect->GetPyInstance().ptr() == Py_None ) // Python allocated entities should not be deleted 
	{
		delete pEffect;	//FIXME: Yes, no?
	}
	else
	{
		pEffect->m_pyRef = boost::python::object();
	}
#else
	delete pEffect;	//FIXME: Yes, no?
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================
}

//-----------------------------------------------------------------------------
// Purpose: Iterate through list and simulate/draw stuff
// Input  : frametime - 
//-----------------------------------------------------------------------------
#ifdef HL2WARS_DLL
void CEffectsList::DrawEffects( double frametime, int flags )
#else
void CEffectsList::DrawEffects( double frametime )
#endif // HL2WARS_DLL
{
	VPROF_BUDGET( "CEffectsList::DrawEffects", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	int i;
	CClientSideEffect *effect;

	// Go backwards so deleting effects doesn't screw up
	for ( i = m_nEffects - 1 ; i >= 0; i-- )
	{
		effect = m_rgEffects[ i ];
#ifdef HL2WARS_DLL
		if ( !effect || (effect->GetFlags() & flags) == 0 )
			continue;
#else
		if ( !effect )
			continue;
#endif // HL2WARS_DLL
		// Simulate
		effect->Draw( frametime );

		// Remove it if needed
		if ( !effect->IsActive() )
		{
			RemoveEffect( i );
		}
	}
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CEffectsList::Flush( void )
{
	int i;
	CClientSideEffect *effect;

	// Go backwards so deleting effects doesn't screw up
	for ( i = m_nEffects - 1 ; i >= 0; i-- )
	{
		effect = m_rgEffects[ i ];
		
		if ( effect == NULL )
			continue;

		RemoveEffect( i );
	}
}

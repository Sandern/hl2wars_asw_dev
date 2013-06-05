//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#include "shaderlib/ShaderDLL.h"
#include "materialsystem/IShader.h"
#include "tier1/utlvector.h"
#include "tier0/dbg.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/materialsystem_config.h"
#include "IShaderSystem.h"
#include "materialsystem/ishaderapi.h"
#include "shaderlib_cvar.h"
#include "mathlib/mathlib.h"
#include "tier2/tier2.h"

#include "materialsystem/imaterialsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// The standard implementation of CShaderDLL
//-----------------------------------------------------------------------------
class CShaderDLL : public IShaderDLLInternal, public IShaderDLL
{
public:
	CShaderDLL();

	// methods of IShaderDLL
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();
	virtual int ShaderCount() const;
	virtual IShader *GetShader( int nShader );

	// methods of IShaderDLLInternal
	virtual bool Connect( CreateInterfaceFn factory, bool bIsMaterialSystem );
	virtual void Disconnect( bool bIsMaterialSystem );
	virtual void InsertShader( IShader *pShader );

private:
	CUtlVector< IShader * >	m_ShaderList;
};


//-----------------------------------------------------------------------------
// Global interfaces/structures
//-----------------------------------------------------------------------------
IMaterialSystemHardwareConfig* g_pHardwareConfig;
const MaterialSystem_Config_t *g_pConfig;


//-----------------------------------------------------------------------------
// Interfaces/structures local to shaderlib
//-----------------------------------------------------------------------------
IShaderSystem* g_pSLShaderSystem;


// Pattern necessary because shaders register themselves in global constructors
static CShaderDLL *s_pShaderDLL;


//-----------------------------------------------------------------------------
// Global accessor
//-----------------------------------------------------------------------------
IShaderDLL *GetShaderDLL()
{
	// Pattern necessary because shaders register themselves in global constructors
	if ( !s_pShaderDLL )
	{
		s_pShaderDLL = new CShaderDLL;
	}

	return s_pShaderDLL;
}

IShaderDLLInternal *GetShaderDLLInternal()
{
	// Pattern necessary because shaders register themselves in global constructors
	if ( !s_pShaderDLL )
	{
		s_pShaderDLL = new CShaderDLL;
	}

	return static_cast<IShaderDLLInternal*>( s_pShaderDLL );
}

//-----------------------------------------------------------------------------
// Singleton interface
//-----------------------------------------------------------------------------
EXPOSE_INTERFACE_FN( (InstantiateInterfaceFn)GetShaderDLLInternal, IShaderDLLInternal, SHADER_DLL_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Connect, disconnect...
//-----------------------------------------------------------------------------
CShaderDLL::CShaderDLL()
{
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
}


//-----------------------------------------------------------------------------
// Connect, disconnect...
//-----------------------------------------------------------------------------
bool CShaderDLL::Connect( CreateInterfaceFn factory, bool bIsMaterialSystem )
{
	g_pHardwareConfig =  (IMaterialSystemHardwareConfig*)factory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, NULL );
	g_pConfig = (const MaterialSystem_Config_t*)factory( MATERIALSYSTEM_CONFIG_VERSION, NULL );
	g_pSLShaderSystem =  (IShaderSystem*)factory( SHADERSYSTEM_INTERFACE_VERSION, NULL );

	if ( !bIsMaterialSystem )
	{
		ConnectTier1Libraries( &factory, 1 );
  		InitShaderLibCVars( factory );
	}

	// Hack for model viewer: force dx level 100
	// Shouldn't matter for the game dll, because it will update the video level anyway (and we always use dxlevel 100)
	if( g_pMaterialSystem )
	{
		// mat_disablehwmorph is a dx10/11 feature, so when dxlevel is 95 it is disabled
		//ConVarRef mat_disablehwmorph("mat_disablehwmorph");

		MaterialSystem_Config_t config = g_pMaterialSystem->GetCurrentConfigForVideoCard();
		//Msg("Current dxlevel: %d\n", config.dxSupportLevel );
		if( CommandLine()->FindParm( "-force_dxlevel" ) )
		{
			config.dxSupportLevel = atoi( CommandLine()->ParmValue( "-force_dxlevel", "100" ) );
			Msg( "Overriding dx level to %d\n", config.dxSupportLevel );
			if( config.dxSupportLevel <= 95 )
				CommandLine()->AppendParm( "-disallowhwmorph", "" );
			g_pMaterialSystem->OverrideConfig( config, true );
		}
		else if( /*!config.bEditMode &&*/ config.dxSupportLevel < 95 )
		{
			//Msg("Overriding dx level\n");
			config.dxSupportLevel = 100;
			g_pMaterialSystem->OverrideConfig( config, true );
		}

		config = g_pMaterialSystem->GetCurrentConfigForVideoCard();

		if( config.dxSupportLevel >= 100 )
		{
			CommandLine()->RemoveParm( "-disallowhwmorph" );
		}
		else if( /*!config.bEditMode &&*/ config.dxSupportLevel < 95 )
		{
			Error("Your graphics card is not supported\n");
		}

		/*bool hasFast = */g_pMaterialSystemHardwareConfig->HasFastVertexTextures();
		//Msg("hasFast: %d\n", hasFast);
	}

	return ( g_pConfig != NULL ) && (g_pHardwareConfig != NULL) && ( g_pSLShaderSystem != NULL );
}

void CShaderDLL::Disconnect( bool bIsMaterialSystem )
{
	if ( !bIsMaterialSystem )
	{
		ConVar_Unregister();
		DisconnectTier1Libraries();
	}

	g_pHardwareConfig = NULL;
	g_pConfig = NULL;
	g_pSLShaderSystem = NULL;
}

bool CShaderDLL::Connect( CreateInterfaceFn factory )
{
	return Connect( factory, false );
}

void CShaderDLL::Disconnect()
{
	Disconnect( false );
}


//-----------------------------------------------------------------------------
// Iterates over all shaders
//-----------------------------------------------------------------------------
int CShaderDLL::ShaderCount() const
{
	return m_ShaderList.Count();
}

IShader *CShaderDLL::GetShader( int nShader ) 
{
	if ( ( nShader < 0 ) || ( nShader >= m_ShaderList.Count() ) )
		return NULL;

	return m_ShaderList[nShader];
}


//-----------------------------------------------------------------------------
// Adds to the shader lists
//-----------------------------------------------------------------------------
void CShaderDLL::InsertShader( IShader *pShader )
{
	Assert( pShader );
	m_ShaderList.AddToTail( pShader );
}


// This file has been generated by Py++.

#include "cbase.h"
#include "cbase.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/AnalogBar.h"
#include "vgui_controls/Image.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/ScrollBarSlider.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/MenuButton.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/RichText.h"
#include "vgui_controls/Tooltip.h"
#include "vgui/IBorder.h"
#include "vgui_bitmapimage.h"
#include "vgui_avatarimage.h"
#include "srcpy_vgui.h"
#include "hl2wars/hl2wars_baseminimap.h"
#include "hl2wars/vgui_video_general.h"
#include "hl2wars/vgui/wars_model_panel.h"
#include "srcpy.h"
#include "tier0/memdbgon.h"
#include "Tooltip_pypp.hpp"

namespace bp = boost::python;

struct Tooltip_wrapper : vgui::Tooltip, bp::wrapper< vgui::Tooltip > {

    Tooltip_wrapper(::vgui::Panel * parent, char const * text=0 )
    : vgui::Tooltip( boost::python::ptr(parent), text )
      , bp::wrapper< vgui::Tooltip >(){
        // constructor
    
    }

    virtual void PerformLayout(  ) {
        PY_OVERRIDE_CHECK( vgui::Tooltip, PerformLayout )
        PY_OVERRIDE_LOG( _vguicontrols, vgui::Tooltip, PerformLayout )
        bp::override func_PerformLayout = this->get_override( "PerformLayout" );
        if( func_PerformLayout.ptr() != Py_None )
            try {
                func_PerformLayout(  );
            } catch(bp::error_already_set &) {
                PyErr_Print();
                this->vgui::Tooltip::PerformLayout(  );
            }
        else
            this->vgui::Tooltip::PerformLayout(  );
    }
    
    void default_PerformLayout(  ) {
        vgui::Tooltip::PerformLayout( );
    }

    virtual void SetEnabled( bool bState ) {
        PY_OVERRIDE_CHECK( vgui::Tooltip, SetEnabled )
        PY_OVERRIDE_LOG( _vguicontrols, vgui::Tooltip, SetEnabled )
        bp::override func_SetEnabled = this->get_override( "SetEnabled" );
        if( func_SetEnabled.ptr() != Py_None )
            try {
                func_SetEnabled( bState );
            } catch(bp::error_already_set &) {
                PyErr_Print();
                this->vgui::Tooltip::SetEnabled( bState );
            }
        else
            this->vgui::Tooltip::SetEnabled( bState );
    }
    
    void default_SetEnabled( bool bState ) {
        vgui::Tooltip::SetEnabled( bState );
    }

    virtual void SetText( char const * text ) {
        PY_OVERRIDE_CHECK( vgui::Tooltip, SetText )
        PY_OVERRIDE_LOG( _vguicontrols, vgui::Tooltip, SetText )
        bp::override func_SetText = this->get_override( "SetText" );
        if( func_SetText.ptr() != Py_None )
            try {
                func_SetText( text );
            } catch(bp::error_already_set &) {
                PyErr_Print();
                this->vgui::Tooltip::SetText( text );
            }
        else
            this->vgui::Tooltip::SetText( text );
    }
    
    void default_SetText( char const * text ) {
        vgui::Tooltip::SetText( text );
    }

};

void register_Tooltip_class(){

    { //::vgui::Tooltip
        typedef bp::class_< Tooltip_wrapper, boost::noncopyable > Tooltip_exposer_t;
        Tooltip_exposer_t Tooltip_exposer = Tooltip_exposer_t( "Tooltip", bp::init< vgui::Panel *, bp::optional< char const * > >(( bp::arg("parent"), bp::arg("text")=bp::object() )) );
        bp::scope Tooltip_scope( Tooltip_exposer );
        bp::implicitly_convertible< vgui::Panel *, vgui::Tooltip >();
        { //::vgui::Tooltip::GetText
        
            typedef char const * ( ::vgui::Tooltip::*GetText_function_type )(  ) ;
            
            Tooltip_exposer.def( 
                "GetText"
                , GetText_function_type( &::vgui::Tooltip::GetText ) );
        
        }
        { //::vgui::Tooltip::GetTooltipDelay
        
            typedef int ( ::vgui::Tooltip::*GetTooltipDelay_function_type )(  ) ;
            
            Tooltip_exposer.def( 
                "GetTooltipDelay"
                , GetTooltipDelay_function_type( &::vgui::Tooltip::GetTooltipDelay ) );
        
        }
        { //::vgui::Tooltip::HideTooltip
        
            typedef void ( ::vgui::Tooltip::*HideTooltip_function_type )(  ) ;
            
            Tooltip_exposer.def( 
                "HideTooltip"
                , HideTooltip_function_type( &::vgui::Tooltip::HideTooltip ) );
        
        }
        { //::vgui::Tooltip::PerformLayout
        
            typedef void ( ::vgui::Tooltip::*PerformLayout_function_type )(  ) ;
            typedef void ( Tooltip_wrapper::*default_PerformLayout_function_type )(  ) ;
            
            Tooltip_exposer.def( 
                "PerformLayout"
                , PerformLayout_function_type(&::vgui::Tooltip::PerformLayout)
                , default_PerformLayout_function_type(&Tooltip_wrapper::default_PerformLayout) );
        
        }
        { //::vgui::Tooltip::ResetDelay
        
            typedef void ( ::vgui::Tooltip::*ResetDelay_function_type )(  ) ;
            
            Tooltip_exposer.def( 
                "ResetDelay"
                , ResetDelay_function_type( &::vgui::Tooltip::ResetDelay ) );
        
        }
        { //::vgui::Tooltip::SetEnabled
        
            typedef void ( ::vgui::Tooltip::*SetEnabled_function_type )( bool ) ;
            typedef void ( Tooltip_wrapper::*default_SetEnabled_function_type )( bool ) ;
            
            Tooltip_exposer.def( 
                "SetEnabled"
                , SetEnabled_function_type(&::vgui::Tooltip::SetEnabled)
                , default_SetEnabled_function_type(&Tooltip_wrapper::default_SetEnabled)
                , ( bp::arg("bState") ) );
        
        }
        { //::vgui::Tooltip::SetText
        
            typedef void ( ::vgui::Tooltip::*SetText_function_type )( char const * ) ;
            typedef void ( Tooltip_wrapper::*default_SetText_function_type )( char const * ) ;
            
            Tooltip_exposer.def( 
                "SetText"
                , SetText_function_type(&::vgui::Tooltip::SetText)
                , default_SetText_function_type(&Tooltip_wrapper::default_SetText)
                , ( bp::arg("text") ) );
        
        }
        { //::vgui::Tooltip::SetTooltipDelay
        
            typedef void ( ::vgui::Tooltip::*SetTooltipDelay_function_type )( int ) ;
            
            Tooltip_exposer.def( 
                "SetTooltipDelay"
                , SetTooltipDelay_function_type( &::vgui::Tooltip::SetTooltipDelay )
                , ( bp::arg("tooltipDelayMilliseconds") ) );
        
        }
        { //::vgui::Tooltip::SetTooltipFormatToMultiLine
        
            typedef void ( ::vgui::Tooltip::*SetTooltipFormatToMultiLine_function_type )(  ) ;
            
            Tooltip_exposer.def( 
                "SetTooltipFormatToMultiLine"
                , SetTooltipFormatToMultiLine_function_type( &::vgui::Tooltip::SetTooltipFormatToMultiLine ) );
        
        }
        { //::vgui::Tooltip::SetTooltipFormatToSingleLine
        
            typedef void ( ::vgui::Tooltip::*SetTooltipFormatToSingleLine_function_type )(  ) ;
            
            Tooltip_exposer.def( 
                "SetTooltipFormatToSingleLine"
                , SetTooltipFormatToSingleLine_function_type( &::vgui::Tooltip::SetTooltipFormatToSingleLine ) );
        
        }
        { //::vgui::Tooltip::ShowTooltip
        
            typedef void ( ::vgui::Tooltip::*ShowTooltip_function_type )( ::vgui::Panel * ) ;
            
            Tooltip_exposer.def( 
                "ShowTooltip"
                , ShowTooltip_function_type( &::vgui::Tooltip::ShowTooltip )
                , ( bp::arg("currentPanel") ) );
        
        }
        { //::vgui::Tooltip::SizeTextWindow
        
            typedef void ( ::vgui::Tooltip::*SizeTextWindow_function_type )(  ) ;
            
            Tooltip_exposer.def( 
                "SizeTextWindow"
                , SizeTextWindow_function_type( &::vgui::Tooltip::SizeTextWindow ) );
        
        }
    }

}

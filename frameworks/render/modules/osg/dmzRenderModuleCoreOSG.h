#ifndef DMZ_RENDER_MODULE_CORE_OSG_DOT_H
#define DMZ_RENDER_MODULE_CORE_OSG_DOT_H

#include <dmzRuntimePlugin.h>
#include <dmzRuntimeRTTI.h>
#include <dmzTypesBase.h>

namespace osg { class Group; class Transform; class Camera; }
namespace osgViewer { class View; }

namespace dmz {

   const char RenderModuleCoreOSGInterfaceName[] = "RenderModuleCoreOSGInterface";

   class RenderCameraManipulatorOSG;

   class RenderModuleCoreOSG {

      public:
         static RenderModuleCoreOSG *cast (const Plugin *PluginPtr);

         virtual UInt32 get_cull_mask () = 0;
         virtual UInt32 get_master_isect_mask () = 0;
         virtual UInt32 lookup_isect_mask (const String &AttributeName) = 0;
         virtual UInt32 lookup_isect_mask (const Handle Attribute) = 0;

         virtual osg::Group *get_scene () = 0;
         virtual osg::Group *get_overlay () = 0;
         virtual osg::Group *get_isect () = 0;
         virtual osg::Group *get_static_objects () = 0;
         virtual osg::Group *get_dynamic_objects () = 0;

         virtual osg::Group *create_dynamic_object (const Handle ObjectHandle) = 0;
         virtual osg::Group *lookup_dynamic_object (const Handle ObjectHandle) = 0;
         virtual Boolean destroy_dynamic_object (const Handle ObjectHandle) = 0;

         virtual Boolean add_view (
            const String &ViewerName,
            osgViewer::View *view) = 0;

         virtual osgViewer::View *lookup_view (const String &ViewName) = 0;
         virtual osgViewer::View *remove_view (const String &ViewName) = 0;

      protected:
         RenderModuleCoreOSG (const PluginInfo &Info);
         ~RenderModuleCoreOSG ();

      private:
         const PluginInfo &__Info;
   };
}


inline dmz::RenderModuleCoreOSG *
dmz::RenderModuleCoreOSG::cast (const Plugin *PluginPtr) {

   return (RenderModuleCoreOSG *)lookup_rtti_interface (
      RenderModuleCoreOSGInterfaceName,
      "",
      PluginPtr);
}


inline
dmz::RenderModuleCoreOSG::RenderModuleCoreOSG (const PluginInfo &Info) :
      __Info (Info) {

   store_rtti_interface (RenderModuleCoreOSGInterfaceName, __Info, (void *)this);
}


inline
dmz::RenderModuleCoreOSG::~RenderModuleCoreOSG () {

   remove_rtti_interface (RenderModuleCoreOSGInterfaceName, __Info);
}

#endif //  DMZ_RENDER_MODULE_CORE_OSG_DOT_H

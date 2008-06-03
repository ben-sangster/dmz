#include "dmzAudioModuleFMOD.h"
#include <dmzAudioSoundAttributes.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzSystemFile.h>

#include <stdlib.h>

dmz::AudioModuleFMOD::AudioModuleFMOD (const PluginInfo &Info, const Config &Local) :
      Plugin (Info),
      Sync (Info),
      AudioModule (Info),
      _log (Info),
      _system (0),
      _instanceRecycle (0),
      _dopplerScale (1.0f),
      _distanceFactor (1.0f),
      _rollOffScale (1.0f),
      _rightHandedCoordinates (True),
      _forceFullDirectSoundHRTF (True),
      _maxChannels (256) {

   _init (Local);
}


dmz::AudioModuleFMOD::~AudioModuleFMOD () {

   //Stop all existing sound instances
   HashTableUInt32Iterator it;

   for (InstanceStruct *instance = _instanceTable.get_first (it);
         instance;
         instance = _instanceTable.get_next (it)) {
      
      // This will cause a callback, which will remove the instance from the list
      // and properly dereference the sound data
      if (instance->channel) { instance->channel->stop (); }
   }

   // This should be empty from the previous calls to stop ()
   _instanceTable.empty ();

   // Delete all recycled instances
   while (_instanceRecycle) {

      InstanceStruct *currInstance = _instanceRecycle;
      _instanceRecycle = _instanceRecycle->next;
      delete currInstance; currInstance = 0;
   }

   //Unref each sound data instance, then clear hashtables
   for (
         SoundStruct *data = _soundHandleTable.get_first (it);
         data;
         data = _soundHandleTable.get_next (it)) {
      
      // This will automatically cause a delete, which will also release the sound
      // within FMOD
      data->unref ();
   }

   // Clear references to all the sound data structs (this will not delete them - they
   // have already been deleted by the previous calls to unref ()
   _soundHandleTable.clear ();
   _soundNameTable.clear ();

   // Delete listeners
   _listenerHandleTable.clear (); // Remove references
   _listenerNameTable.clear (); // Remove references
   _listenerIndexTable.empty (); // Now delete all of them

   // Release sound system
   if (_system) { _error_check ("Releasing Sound System",  _system->release ()); }
}


void
dmz::AudioModuleFMOD::discover_plugin (const Plugin *PluginPtr) {;}


void
dmz::AudioModuleFMOD::start_plugin () {;}


void
dmz::AudioModuleFMOD::update_sync (const Float64 TimeDelta) {

   if (_system) { _system->update(); }
}


void
dmz::AudioModuleFMOD::remove_plugin (const Plugin *PluginPtr) {;}


void
dmz::AudioModuleFMOD::stop_plugin () {;}


void
dmz::AudioModuleFMOD::shutdown_plugin () {;}


dmz::UInt32
dmz::AudioModuleFMOD::create_audio_handle (const String &Filename) {

   UInt32 result (0);
   String absPath;

   if (_system) {

      // Convert to absolute file path
      Boolean validPath = get_absolute_path (Filename, absPath);

      if (validPath) {

         SoundStruct *soundData = _soundNameTable.lookup (absPath);

         if (soundData) {
            
            result = soundData->get_handle (); 
         }
         else {
         
            FMOD::Sound *newSound (0);

            FMOD_RESULT fmodResult = _system->createSound (
               absPath.get_buffer (), 
               FMOD_3D, 
               0, 
               &newSound);

            String loadingErrorHeader ("Loading Sound '");
            loadingErrorHeader << absPath << "'";

            if (_error_check (loadingErrorHeader, fmodResult)) {
               
               SoundStruct *newSoundData = new SoundStruct (
                  absPath, 
                  newSound, 
                  get_plugin_runtime_context ());

               if (newSoundData && _soundNameTable.store (absPath, newSoundData)) {
                  
                  UInt32 newHandle = newSoundData->get_handle ();

                  if (newHandle && _soundHandleTable.store (newHandle, newSoundData)) {

                     _log.info << "Sound file '" << absPath << "' Loaded" << endl;
                     result = newHandle;
                  }
               }
               else if (newSoundData) { newSoundData->unref (); }
            }
         }
      }
   }
  
   return result;
}


dmz::Boolean
dmz::AudioModuleFMOD::destroy_audio_handle (const UInt32 AudioHandle) {

   Boolean result (False);

   if (_system) {

      SoundStruct *data = _soundHandleTable.lookup (AudioHandle);
      
      if (data) {

         _soundHandleTable.remove (AudioHandle);
         _soundNameTable.remove (data->Filename);

         data->unref ();
         // Now any when any leftover sound instances are done with this piece
         // of sound data, then it will be automatically deleted

         result = True;
      }
   }

   return result;
}


dmz::UInt32
dmz::AudioModuleFMOD::play_sound (
      const UInt32 AudioHandle, 
      const SoundAttributes &Attributes) {

   UInt32 result (0);

   if (_system) {

      SoundStruct *data = _soundHandleTable.lookup (AudioHandle);

      if (data) {

         UInt32 dataHandle = data->get_handle ();

         InstanceStruct *instance = _get_new_instance (data);

         if (instance) {

            UInt32 instanceHandle = instance->get_handle ();

            if (instanceHandle && 
                  _instanceTable.store (instanceHandle, instance)) {

               String errorHeader ("Playing Sound '");
               errorHeader << data->Filename << "'";

               // Bind sound to channel
               FMOD_RESULT fmodResult = _system->playSound (
                  FMOD_CHANNEL_FREE,
                  data->sound,
                  true,
                  &(instance->channel));

               if (_error_check (errorHeader, fmodResult) && instance->channel) {

                  Float32 defaultFrequency (1.0);
                  // Get default frequency of sound
                  fmodResult = instance->channel->getFrequency (&(defaultFrequency));

                  instance->defaultFrequency = defaultFrequency;

                  _error_check (errorHeader, fmodResult);

                  update_sound (instance->get_handle (), Attributes);

                  // Attach to this channel pointer to the instance struct
                  fmodResult = instance->channel->setUserData ((void *)instance);

                  if (_error_check (errorHeader, fmodResult)) {

                     fmodResult = instance->channel->setCallback (
                        FMOD_CHANNEL_CALLBACKTYPE_END,
                        _channel_callback,
                        0);

                     if (_error_check (errorHeader, fmodResult)) {

                        fmodResult = instance->channel->setPaused(false);

                        if (_error_check (errorHeader, fmodResult)) { 

                           _log.info << errorHeader << endl;
                           result = instanceHandle; 
                        }
                     }
                  } 
               }

               // FMOD is unsuccessful in attempting to play sound 
               if (!result) { _remove_instance (instanceHandle); }
            }
            else if (instance){

               instance->reset ();
 
               instance->next = _instanceRecycle;
               _instanceRecycle = instance;
            }
         }
      }
   }

   return result;
}


dmz::Boolean
dmz::AudioModuleFMOD::update_sound (
      const UInt32 InstanceHandle, 
      const SoundAttributes &Attributes) {

   Boolean result (False);

   if (_system) {

      String errorHeader;

      InstanceStruct *instance = _instanceTable.lookup (InstanceHandle);
      
      if (instance && instance->channel) {

         FMOD_RESULT fmodResult (FMOD_OK);

         // Set loop mode
         if (Attributes.get_loop ()) {

            fmodResult = instance->channel->setMode (FMOD_LOOP_NORMAL);
         }
         else {

            fmodResult = instance->channel->setMode (FMOD_LOOP_OFF);
         }

         _error_check (errorHeader, fmodResult);

         // Set pitch/frequency with scale value
         fmodResult = instance->channel->setFrequency (
               Float32 (Attributes.get_pitch_scale() * 
                        instance->defaultFrequency));

         _error_check (errorHeader, fmodResult);

         // Set 3D sound position
         Vector pos, vel;

         Attributes.get_position (pos);
         Attributes.get_velocity (vel);

         FMOD_VECTOR posVal = {
            Float32 (pos.get_x ()),
            Float32 (pos.get_y ()),
            Float32 (pos.get_z ())};

         FMOD_VECTOR velVal = {
            Float32 (vel.get_x ()),
            Float32 (vel.get_y ()),
            Float32 (vel.get_z ())};

         fmodResult = instance->channel->set3DAttributes(&posVal, &velVal);

         _error_check (errorHeader, fmodResult);

         result = True;
      }
   }

   return result;
}


dmz::Boolean
dmz::AudioModuleFMOD::lookup_sound (
      const UInt32 InstanceHandle, 
      SoundAttributes &attributes) {

   Boolean result (False);

   if (_system) {

#if 0
      FMOD_VECTOR pos = {0.0f, 0.0f, 0.0f};

      InstanceStruct *instance = _instanceTable.lookup (InstanceHandle);

      if (instance && instance->channel) {

         instance->channel->get3DAttributes (&pos, 0);
         position.set_xyz (Float64 (pos.x), Float64 (pos.y), Float64 (pos.z));

         FMOD_RESULT fmodResult = instance->channel->getFrequency(&currentFrequency);

         if (_error_check ("Getting Sound Pitch", fmodResult)) {

            pitchScale = Float64 (currentFrequency) / 
               Float64 (instance->defaultFrequency);

            result = True;
         }
      }
#endif
   }
 
   return result;
}


dmz::Boolean
dmz::AudioModuleFMOD::stop_sound (const UInt32 InstanceHandle) {

   Boolean result (False);

   InstanceStruct *instance = _instanceTable.lookup (InstanceHandle);

   if (instance) {

      String infoString ("Stopping Sound '");
      if (instance->data) { infoString << instance->data->Filename << "'"; }
      else { infoString << "UNKNOWN'"; }

      // Stop channel, which will automatically cause a callback to the 
      // user defined callback function that removes it from the instance table

      if (instance->channel) {

         FMOD_RESULT fmodResult = instance->channel->stop ();

         if (_error_check (infoString, fmodResult)) { 

            _log.info << infoString << endl;
            result = True; 
         }
      }
   }

   return result;
}



dmz::Boolean
dmz::AudioModuleFMOD::set_mute_all_state (const Boolean Mute) {

   Boolean result (False);

   if (_system) {

      FMOD::ChannelGroup *masterChannelGroup (0);
      FMOD_RESULT fmodResult = _system->getMasterChannelGroup (&masterChannelGroup);

      if (masterChannelGroup &&
            _error_check ("Getting master channel group", fmodResult)) {

         fmodResult = masterChannelGroup->setMute (Mute);

         if (_error_check ("Setting mute state for all sounds", fmodResult)) { 
            
            result = True; 
         }
      }
   }

   return result;
}


dmz::Boolean
dmz::AudioModuleFMOD::get_mute_all_state (Boolean &mute) {

   Boolean result (False);

   if (_system) {

      FMOD::ChannelGroup *masterChannelGroup (0);
      FMOD_RESULT fmodResult = _system->getMasterChannelGroup (&masterChannelGroup);

      if (masterChannelGroup &&
            _error_check ("Getting master channel group", fmodResult)) {

         fmodResult = masterChannelGroup->getMute (&mute);

         if (_error_check ("Getting mute state for all sounds", fmodResult)) { 

            result = True; 
         }
      }
   }

   return result;
}


dmz::UInt32
dmz::AudioModuleFMOD::create_listener (const String &Name) {

   UInt32 result (0);

   Int32 numListeners = _listenerNameTable.get_count ();

   if (_system && (numListeners <= 4)) {
      
      ListenerStruct *newListener = new ListenerStruct (
         Name, 
         numListeners, 
         get_plugin_runtime_context ());

      if (newListener) {

         Boolean storeResults = True;

         storeResults = storeResults && _listenerNameTable.store (Name, newListener);
         storeResults =  storeResults && 
            _listenerHandleTable.store (newListener->get_handle (), newListener);
         storeResults = storeResults && 
            _listenerIndexTable.store (numListeners, newListener);

         if (storeResults) { 

            FMOD_RESULT fmodResult = _system->set3DNumListeners (numListeners + 1);

            if (_error_check ("Creating a Listener", fmodResult)) {

               if (numListeners == 0)  {
                  
                  // First listener being added - unmute all sounds
                  Boolean muteSuccess = set_mute_all_state (False);

                  if (muteSuccess) {

                     result = newListener->get_handle ();
                  }
               }   
            }
         }
         else {

            _listenerNameTable.remove (Name);
            _listenerHandleTable.remove (newListener->get_handle ());
            _listenerIndexTable.remove (numListeners);
         }
      }

      if (!result) { delete newListener; newListener = 0; }
   }

   return result;
}


dmz::UInt32
dmz::AudioModuleFMOD::lookup_listener (const String &Name) {

   UInt32 result (0);

   ListenerStruct *listener = _listenerNameTable.lookup (Name);

   if (listener) { result = listener->get_handle (); }

   return result;
}


dmz::Boolean
dmz::AudioModuleFMOD::set_listener (
      const UInt32 Handle,
      const Vector &Position,
      const Matrix &Orientation) {

   Boolean result (False);

   if (_system) {

      ListenerStruct *listener = _listenerHandleTable.lookup (Handle);

      if (listener) {

//         if ((listener->orientation != Orientation) || (listener->position != Position)) {
         listener->orientation = Orientation;
         listener->position = Position;

         Vector lookVector (0.0, 0.0, -1.0);
         Vector upVector (0.0, 1.0, 0.0); 
         Orientation.transform_vector (lookVector);
         Orientation.transform_vector (upVector);

#ifdef FOO_BAR // _WIN32
         FMOD_VECTOR fmodLookVector = {
            Float32 (lookVector.get_x () + (Float64 (rand () / (Float64 (RAND_MAX) * 10.0)))),
            Float32 (lookVector.get_y () + ((Float64 (rand () / Float64 (RAND_MAX) * 10.0)))),
            Float32 (lookVector.get_z () + ((Float64 (rand () / Float64 (RAND_MAX) * 10.0))))};
#else

         FMOD_VECTOR fmodLookVector = {
            Float32 (lookVector.get_x ()),
            Float32 (lookVector.get_y ()),
            Float32 (lookVector.get_z ())};
#endif

         FMOD_VECTOR fmodUpVector = {
            Float32 (upVector.get_x ()),
            Float32 (upVector.get_y ()),
            Float32 (upVector.get_z ())};

         FMOD_VECTOR fmodPosition = {
            Float32 (Position.get_x ()),
            Float32 (Position.get_y ()),
            Float32 (Position.get_z ())};

         FMOD_VECTOR vel = {0.0f, 0.1f, 0.0f};

         FMOD_RESULT fmodResult = _system->set3DListenerAttributes (
            listener->index,
            &fmodPosition,
            &vel,
            &fmodLookVector,
            &fmodUpVector);

         if (_error_check ("Setting 3D Attributes", fmodResult)) {
            
            result = True;
         }
//         }
      }
   }

   return result;
}


dmz::Boolean
dmz::AudioModuleFMOD::get_listener (
      const UInt32 Handle,
      Vector &position,
      Matrix &orientation) {

   Boolean result (False);

   if (_system) {

      ListenerStruct *listener = _listenerHandleTable.lookup (Handle);

      if (listener) {
         
         // Get vectors and positions from stored values. We are not using the
         // internal FMOD data because conversion from look/up vectors to an orientaton
         // is unnecessary extra computation. Since our class is the only code to modify
         // the orientations and positions of the FMOD channels, we can keep track of
         // the orientations and positions externally.

         // Set position
         position = listener->position;

         //Set orientation
         orientation = listener->orientation;

         result = True;
      }
   }

   return result;
}


dmz::Boolean
dmz::AudioModuleFMOD::destroy_listener (const UInt32 Handle) {
      
   Boolean result (False);
   if (_system) {

      ListenerStruct *listener = _listenerHandleTable.lookup (Handle);

      if (listener) {

         Int32 numListeners = _listenerHandleTable.get_count ();

         if (listener->index == (numListeners - 1)) {

            // Can simply reduce listener count in FMOD without rearranging indices
            
            FMOD_RESULT fmodResult (FMOD_OK);
            Boolean muteSuccess (True);

            // FMOD requires at least one listener
            if (numListeners > 1) { 

               fmodResult = _system->set3DNumListeners (numListeners - 1);
            }
            else {

               // To simulate a lack of any listeners, mute all sounds
               muteSuccess = set_mute_all_state (True);
            }

            if (_error_check ("Destroying a Listener", fmodResult) && muteSuccess) {

               // Delete the listener
               _listenerHandleTable.remove (Handle);
               _listenerNameTable.remove (listener->Name);
               _listenerIndexTable.remove (listener->index);
               delete listener; listener = 0;
               result = True;
            }
         }
         else {
            // FMOD only allows the adjustment of the total number of listeners, so 
            // we must swap the listener to be deleted with the last listener and
            // then reduce the total number of listeners in FMOD

            // Get last listener
            ListenerStruct *lastListener = _listenerIndexTable.lookup (numListeners - 1);

            if (lastListener) {
               
               // Get the position and orientation of listener to be moved
               Vector copiedPosition;
               Matrix copiedOrientation;

               if (get_listener (
                     listener->get_handle (), 
                     copiedPosition, 
                     copiedOrientation)) {

                  // Change the FMOD listener index to the index of the listener to be
                  // deleted
                  lastListener->index = listener->index;
               
                  // Override the listener attributes in the index location of the deleted
                  // listener
                  if (set_listener (
                     lastListener->get_handle (), 
                     copiedPosition, 
                     copiedOrientation)) {

                     // Reduce reduce listener count in FMOD to erase the old data
                     // of the now duplicated listener at the end. 
                     FMOD_RESULT fmodResult = _system->set3DNumListeners (numListeners - 1);
                     if (_error_check ("Destroying a Listener", fmodResult)) {

                        // Delete the listener
                        _listenerHandleTable.remove (Handle);
                        _listenerNameTable.remove (listener->Name);
                        _listenerIndexTable.remove (listener->index);
                        delete listener; listener = 0;
                        result = True;
                     }
                  }
               }
            }
         }
      }
   }

   return result;
}



void
dmz::AudioModuleFMOD::_init (const Config &Local) {
    
    // Create a System object
   FMOD_RESULT createResult = FMOD::System_Create(&_system);
   _error_check ("Enabling FMOD", createResult);

   _dopplerScale = config_to_float32 ("dopplerScale.value", Local, _dopplerScale);
   _distanceFactor = config_to_float32 ("distanceFactor.value", Local, _distanceFactor);
   _rollOffScale = config_to_float32 ("rollOffScale.value", Local, _rollOffScale);
   _maxChannels = config_to_int32 ("maxChannels.value", Local, _maxChannels);

   _rightHandedCoordinates =
      config_to_boolean ("rightHandedCoordinate.value", Local, _rightHandedCoordinates);

   _forceFullDirectSoundHRTF = config_to_boolean (
      "forceFullDirectSoundHRTF.value",
      Local,
      _forceFullDirectSoundHRTF);

   if (_system) {

      unsigned int version;
      FMOD_RESULT result =_system->getVersion (&version);

      FMOD_CAPS caps;
      FMOD_SPEAKERMODE speakermode;
      result = _system->getDriverCaps (0, &caps, 0, 0, &speakermode);
      _error_check ("Getting driver caps", result);

      result = _system->setSpeakerMode (speakermode);
      _error_check ("Setting speaker mode", result);

      if (caps & FMOD_CAPS_HARDWARE_EMULATED) {

         result = _system->setDSPBufferSize (1024, 10);
         _error_check ("Setting speaker mode", result);
         _log.warn << "Audio hardware is being emulated" << endl;
      }
      
      FMOD_INITFLAGS flags = FMOD_INIT_NORMAL;

      if (_rightHandedCoordinates) { flags |= FMOD_INIT_3D_RIGHTHANDED; }

      // Only enable Software HRTF emulation if we are on windows (and therefore
      // DirectSound is available)
      #ifdef _WIN32
      if (_forceFullDirectSoundHRTF) { flags |= FMOD_INIT_DSOUND_HRTFFULL; }
      #endif

      result = _system->init (_maxChannels, flags, 0);

      if (result == FMOD_ERR_OUTPUT_CREATEBUFFER) {

        result = _system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO);
        _error_check ("Setting speaker mode to stereo", result);
        result = _system->init(_maxChannels, flags, 0);
        _error_check ("Initializing FMOD Sound System", result);
      }

//      _system->set3DSettings (_dopplerScale, _distanceFactor, _rollOffScale);

      set_mute_all_state (True); // Mute all sounds for now (creating the first listener
                                 // will unmute them)
   }

   _log.info << "AudioModuleFMOD Initialized" << endl;
}


dmz::AudioModuleFMOD::InstanceStruct *
dmz::AudioModuleFMOD::_get_new_instance (SoundStruct *soundData) {

   InstanceStruct *result (0);

   if (soundData) {

   // Check for existing list of stored instances
      if (_instanceRecycle) {

         result = _instanceRecycle;

         _instanceRecycle = _instanceRecycle->next;

         result->next = 0;

         result->data = soundData;

         soundData->ref ();
      }
      else { //Create new instance

         result = new InstanceStruct (
            soundData->Filename,
            (*this),
            soundData, 
            get_plugin_runtime_context ());
      }
   }

   return result;
}


void
dmz::AudioModuleFMOD::_remove_instance (UInt32 InstanceHandle) {

   InstanceStruct *instance = _instanceTable.lookup (InstanceHandle);
   
   if (instance) {

      // Remove from channel list
      _instanceTable.remove (InstanceHandle);

      // Reset dynamic structure data
      instance->reset ();

      // Store
      if (_instanceRecycle) {

         // Attach to front of list
         instance->next = _instanceRecycle;
         _instanceRecycle = instance;

      }
      else {

         _instanceRecycle = instance;
      }
   }

   //TODO: Implement a maximum number of stored instances
}


FMOD_RESULT F_CALLBACK 
dmz::AudioModuleFMOD::_channel_callback (
      FMOD_CHANNEL *channelPointer, 
      FMOD_CHANNEL_CALLBACKTYPE type, 
      int command, 
      unsigned int commanddata1, 
      unsigned int commanddata2) {

   FMOD::Channel *channel = (FMOD::Channel *)channelPointer;
   void *rawData;

   FMOD_RESULT fmodResult = channel->getUserData (&rawData);

   if (rawData) {

      InstanceStruct *instance = (InstanceStruct *) rawData; 

      instance->module._remove_instance (instance->get_handle ());
   }
   //_error_check ("Retrieving Channel Data", fmodResult);

   return FMOD_OK;
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzAudioModuleFMOD (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::AudioModuleFMOD (Info, local);
}

};

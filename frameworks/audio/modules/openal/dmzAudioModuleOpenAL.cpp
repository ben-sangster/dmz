#include "dmzAudioModuleOpenAL.h"
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzSystemFile.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesVector.h>
/*!

\class dmz::AudioModuleOpenAL
\ingroup Audio
\brief OpenAL implementation of the audio module.

*/
 

//! \cond
dmz::AudioModuleOpenAL::AudioModuleOpenAL (const PluginInfo &Info, Config &local) :
      Plugin (Info),
      TimeSlice (Info),
      AudioModule (Info),
      _log (Info),
      _device (0),
      _context (0) {

   _init (local);
}


dmz::AudioModuleOpenAL::~AudioModuleOpenAL () {

   _soundTimedTable.clear ();
   _soundTable.empty ();
   _bufferHandleTable.clear ();

   HashTableStringIterator it;

   BufferStruct *bs (_bufferNameTable.get_first (it));

   while (bs) {

      while (bs->unref () > 0) {;} // do nothing
      bs = _bufferNameTable.get_next (it);
   }

   if (_context) {

      alcMakeContextCurrent (0);
      alcDestroyContext (_context);
      _context = 0;
   }

   if (_device) {

      alcCloseDevice (_device);
      _device = 0;
   }
}


// Plugin Interface
void
dmz::AudioModuleOpenAL::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::AudioModuleOpenAL::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

   }
   else if (Mode == PluginDiscoverRemove) {

   }
}


// Time Slice Interface
void
dmz::AudioModuleOpenAL::update_time_slice (const Float64 TimeDelta) {

   HashTableHandleIterator it;

   SoundStruct *ss (_soundTimedTable.get_first (it));

   while (ss) {

      ALint value (0);

      alGetSourcei (ss->source, AL_SOURCE_STATE, &value);

      if (value == AL_STOPPED) {

         _soundTimedTable.remove (ss->Handle.get_runtime_handle ());
         _soundTable.remove (ss->Handle.get_runtime_handle ());

         delete ss; ss = 0;
      }

      ss = _soundTimedTable.get_next (it);
   }
}


// Audio Module Interface
dmz::Handle
dmz::AudioModuleOpenAL::create_sound (const String &FileName) {

   Handle result (0); 
   String absPath;

   if (_context && get_absolute_path (FileName, absPath)) {

      BufferStruct *bs (_bufferNameTable.lookup (absPath));

      if (!bs) {

         bs = new BufferStruct (
            absPath,
            get_plugin_runtime_context (),
            _bufferNameTable,
            _bufferHandleTable);

         if (bs &&
               bs->file.is_valid () &&
               (bs->file.get_audio_format () == WaveFormatPCM)) {

            if (!_bufferNameTable.store (absPath, bs)) { bs->unref (); bs = 0; }
            else { _bufferHandleTable.store (bs->Handle.get_runtime_handle (), bs); }
         }
         else if (bs) {

            if (bs->file.is_valid ()) {

               _log.error << "Unable to load audio file: " << FileName << " because: "
                  << " Wave audio data is not PCM." << endl;
            }
            else {

               _log.error << "Unable to load audio file: " << FileName << " because: "
                  << bs->file.get_error () << endl;
            }

            bs->unref (); bs = 0;
         }

         if (bs) {

            alGenBuffers (1, &(bs->buffer));
            ALenum format (0);
            const UInt32 Channels (bs->file.get_channel_count ());
            const UInt32 BPS (bs->file.get_bits_per_sample ());

            if (Channels == 1) {

               if (BPS == 8) { format = AL_FORMAT_MONO8; }
               else if (BPS == 16) { format = AL_FORMAT_MONO16; }
            }
            else if (Channels == 2) {

               if (BPS == 8) { format = AL_FORMAT_STEREO8; }
               else if (BPS == 16) { format = AL_FORMAT_STEREO16; }
            }

            UInt32 size (0);
            ALvoid *data = (ALvoid *)(bs->file.get_audio_buffer (size));

            if (size && data && format) {

               alBufferData (
                  bs->buffer,
                  format,
                  data,
                  (ALsizei)size,
                  (ALsizei)bs->file.get_frequency ());

               ALenum error = alGetError ();

               if (error != AL_NO_ERROR) {

                  _log.error << "Unable to bind file: " << FileName
                     << " to OpenAL buffer." << endl;

                  destroy_sound (bs->Handle.get_runtime_handle ());
                  bs = 0;
               }
               else {

                  _log.info << "Loaded audio file: " << bs->FileName << endl;
                  bs->file.clear ();
               }
            }
            else {

               if (!size || !data) {

                  _log.error << "Unable to load wave data" << endl;
               }

               if (!format) {

                  _log.error << "Unsupported format, channels: " << Channels
                     << " BPS: " << BPS << endl;
               }

               destroy_sound (bs->Handle.get_runtime_handle ());
               bs = 0;
            }
         }
      }
      else { bs->ref (); }

      if (bs) {

         result = bs->Handle.get_runtime_handle ();
      }
   }

   return result;
}


dmz::Boolean
dmz::AudioModuleOpenAL::destroy_sound (const Handle AudioHandle) {

   Boolean result (False);

   BufferStruct *bs (_bufferHandleTable.lookup (AudioHandle));

   if (bs) { bs->unref (); result = True; }

   return result;
}


dmz::Handle
dmz::AudioModuleOpenAL::play_sound (
      const Handle AudioHandle,
      const SoundInit &Init,
      const SoundAttributes &Attributes) {

   Handle result (0);

   BufferStruct *bs (_bufferHandleTable.lookup (AudioHandle));

   SoundStruct *ss (0);

   if (bs) { ss = new SoundStruct (*bs, get_plugin_runtime_context ()); }

   if (bs && ss) {

      result = ss->Handle.get_runtime_handle ();

      if (_soundTable.store (result, ss)) {

         alGenSources (1, &(ss->source));
         alSourcei (ss->source, AL_BUFFER, bs->buffer);

         alSourcei (
            ss->source,
            AL_SOURCE_RELATIVE,
            Init.get (SoundRelative) ? AL_TRUE : AL_FALSE);

         alSourcei (ss->source, AL_LOOPING, Init.get (SoundLooped) ? AL_TRUE : AL_FALSE);
         alSourcef (ss->source, AL_GAIN, 1.0f);
         alSourcef (ss->source, AL_ROLLOFF_FACTOR, 0.1f);

         if (!Init.get (SoundLooped)) {

            _soundTimedTable.store (result, ss);
         }

         ss->attr = Attributes;
         ss->init = Init;

         _update_sound (*ss);

         alGetError ();
         alSourcePlay (ss->source);
         ALenum error;
         if ((error = alGetError ()) != AL_NO_ERROR) {

            _log.error << "Unable to play sound: " << alGetString (error) << endl;
         }
      }
      else { delete ss; ss = 0; result = 0; }

   }
   else { delete ss; ss = 0; }

   return result;
}


dmz::Boolean
dmz::AudioModuleOpenAL::update_sound (
      const Handle InstanceHandle,
      const SoundAttributes &Attributes) {

   Boolean result (False);

   SoundStruct *ss (_soundTable.lookup (InstanceHandle));

   if (ss) { ss->attr = Attributes; _update_sound (*ss); result = True; }

   return result;
}


dmz::Boolean
dmz::AudioModuleOpenAL::lookup_sound (
      const Handle InstanceHandle,
      SoundInit &init,
      SoundAttributes &attributes) {

   Boolean result (False);

   SoundStruct *ss (_soundTable.lookup (InstanceHandle));

   if (ss) { attributes = ss->attr; init = ss->init; result = True; }

   return result;
}


dmz::Boolean
dmz::AudioModuleOpenAL::stop_sound (const Handle InstanceHandle) {

   Boolean result (False);

   SoundStruct *ss (_soundTable.remove (InstanceHandle));

   if (ss) {

      _soundTimedTable.remove (InstanceHandle);

      delete ss; ss = 0;
      result = True;
   }

   return result;
}


dmz::Boolean
dmz::AudioModuleOpenAL::set_mute_all_state (const Boolean Mute) {

   Boolean result (False);

   alListenerf (AL_GAIN, Mute ? 0.0f : 1.0f);

   return result;
}


dmz::Boolean
dmz::AudioModuleOpenAL::get_mute_all_state (Boolean &mute) {

   Boolean result (False);

   ALfloat value (0.0);
   alGetListenerf (AL_GAIN, &value);

   if (!is_zero64 (value)) { result = True; }

   return result;
}


dmz::Handle
dmz::AudioModuleOpenAL::create_listener (const String &Name) {

   Handle result (0);

   if (!_listenerName && Name) {

      result = get_plugin_handle ();
      _listenerName = Name;
   }

   return result;
}


dmz::Handle
dmz::AudioModuleOpenAL::lookup_listener (const String &Name) {

   Handle result (0);

   if (Name && (_listenerName == Name)) { result = get_plugin_handle (); }

   return result;
}


dmz::Boolean
dmz::AudioModuleOpenAL::set_listener (
      const Handle ListenerHandle,
      const Vector &Position,
      const Matrix &Orientation,
      const Vector &Velocity) {

   Boolean result (False);

   if (ListenerHandle == get_plugin_handle ()) {

      _listenerPos = Position;
      _listenerOri = Orientation;
      _listenerVel = Velocity;

      Vector f (0.0, 0.0, -1.0);
      Vector up (0.0, 1.0, 0.0);
      Orientation.transform_vector (f);
      Orientation.transform_vector (up);

      ALfloat ori[6];
      ori[0] = (ALfloat)f.get_x ();
      ori[1] = (ALfloat)f.get_y ();
      ori[2] = (ALfloat)f.get_z ();
      ori[3] = (ALfloat)up.get_x ();
      ori[4] = (ALfloat)up.get_y ();
      ori[5] = (ALfloat)up.get_z ();

      alListenerfv (AL_ORIENTATION, ori);

      alListener3f (
         AL_POSITION,
         (ALfloat)Position.get_x (),
         (ALfloat)Position.get_y (),
         (ALfloat)Position.get_z ());

      alListener3f (
         AL_VELOCITY,
         (ALfloat)Velocity.get_x (),
         (ALfloat)Velocity.get_y (),
         (ALfloat)Velocity.get_z ());

      result = True;
   }

   return result;
}


dmz::Boolean
dmz::AudioModuleOpenAL::get_listener (
      const Handle ListenerHandle,
      Vector &position,
      Matrix &orientation,
      Vector &velocity) {

   Boolean result (False);

   if (ListenerHandle == get_plugin_handle ()) {

      position = _listenerPos;
      orientation = _listenerOri;
      velocity = _listenerVel;

      result = True;
   }

   return result;
}


dmz::Boolean
dmz::AudioModuleOpenAL::destroy_listener (const Handle ListenerHandle) {

   Boolean result (False);

   if (ListenerHandle == get_plugin_handle ()) { _listenerName.empty (); result = True; }

   return result;
}


void
dmz::AudioModuleOpenAL::_update_sound (SoundStruct &ss) {

   Vector vec;

   ss.attr.get_position (vec);
   alSource3f (
      ss.source,
      AL_POSITION,
      (ALfloat)vec.get_x (),
      (ALfloat)vec.get_y (),
      (ALfloat)vec.get_z ());

   ss.attr.get_velocity (vec);
   alSource3f (
      ss.source,
      AL_VELOCITY,
      (ALfloat)vec.get_x (),
      (ALfloat)vec.get_y (),
      (ALfloat)vec.get_z ());

   alSourcef (ss.source, AL_GAIN, (ALfloat)ss.attr.get_gain_scale ());

   Float64 Pitch (ss.attr.get_pitch_scale ());
   ALfloat AdjustedPitch (Pitch < 0.1 ? (ALfloat)0.1f : (ALfloat)Pitch);
   alSourcef (ss.source, AL_PITCH, AdjustedPitch);
}


void
dmz::AudioModuleOpenAL::_init (Config &local) {

   _device = alcOpenDevice (0);

   if (_device) {

      _context = alcCreateContext (_device, 0);

      if (_context) {

         alcMakeContextCurrent (_context);

         //alDistanceModel (AL_LINEAR_DISTANCE_CLAMPED);
      }
      else { _log.error << "Unable to create OpenAL Context." << endl; }
   }
   else { _log.error << "Unable to create OpenAL Device." << endl; }
}
//! \endcond


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzAudioModuleOpenAL (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::AudioModuleOpenAL (Info, local);
}

};

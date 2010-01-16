#ifndef DMZ_SYSTEM_FILE_DOT_H
#define DMZ_SYSTEM_FILE_DOT_H

#include <dmzKernelExport.h>
#include <dmzTypesBase.h>
#include <dmzTypesString.h>
#include <dmzTypesStringContainer.h>
#include <stdio.h> // For FILE

namespace dmz {

   class PushDirectory {

      public:
         PushDirectory (const String &Path);
         ~PushDirectory ();
         Boolean is_valid () const;

      protected:
         //! \cond
         const String _Dir;
         Boolean _valid;
         //! \endcond

      private:
         PushDirectory ();
         PushDirectory (const PushDirectory &);
         PushDirectory &operator= (const PushDirectory &);
   };

   DMZ_KERNEL_LINK_SYMBOL StringContainer validate_path_container (
      const StringContainer &Container);

   DMZ_KERNEL_LINK_SYMBOL Boolean find_file (
      const StringContainer &Container,
      const String &FileName,
      String &foundFile);

   DMZ_KERNEL_LINK_SYMBOL String format_path (const String &Path);

   DMZ_KERNEL_LINK_SYMBOL void split_path_file_ext (
      const String &FullPath,
      String &path,
      String &file,
      String &ext);

   DMZ_KERNEL_LINK_SYMBOL Boolean get_file_list (
      const String &Path,
      StringContainer &container);

   DMZ_KERNEL_LINK_SYMBOL Boolean get_directory_list (
      const String &Path,
      StringContainer &container);

   DMZ_KERNEL_LINK_SYMBOL Boolean is_valid_path (const String &Path);
   DMZ_KERNEL_LINK_SYMBOL Boolean is_directory (const String &Path);
   DMZ_KERNEL_LINK_SYMBOL UInt64 get_file_size (const String &Path);
   DMZ_KERNEL_LINK_SYMBOL String get_current_directory ();
   DMZ_KERNEL_LINK_SYMBOL Boolean change_directory (const String &Path);
   DMZ_KERNEL_LINK_SYMBOL Boolean create_directory (const String &Path);
   DMZ_KERNEL_LINK_SYMBOL Boolean get_absolute_path (const String &Path, String &absPath);
   DMZ_KERNEL_LINK_SYMBOL Boolean remove_file (const String &Path);

   // This is needed because Microsoft has deprecated the ANSI C file calls.
   DMZ_KERNEL_LINK_SYMBOL FILE *open_file (const String &FileName, const String &Opts);
   DMZ_KERNEL_LINK_SYMBOL Int32 read_file (
      const FILE *File,
      const Int32 Size,
      char *buffer);
   DMZ_KERNEL_LINK_SYMBOL Int32 read_file (
      const FILE *File,
      const Int32 Size,
      String &buffer);
   DMZ_KERNEL_LINK_SYMBOL Boolean close_file (const FILE *File);

   DMZ_KERNEL_LINK_SYMBOL String get_home_directory ();
};


/*!

\brief Constructor
\param[in] Path String containing the new working directory.

*/
inline
dmz::PushDirectory::PushDirectory (const String &Path) : _Dir (get_current_directory ()) {

   _valid = change_directory (Path);
}


//! Destructor.
inline
dmz::PushDirectory::~PushDirectory () { change_directory (_Dir); }


//! Returns dmz::True if directory change was successful.
inline
dmz::Boolean
dmz::PushDirectory::is_valid () const { return _valid; }

#endif // DMZ_SYSTEM_FILE_DOT_H

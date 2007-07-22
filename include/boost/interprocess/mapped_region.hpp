//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MAPPED_REGION_HPP
#define BOOST_INTERPROCESS_MAPPED_REGION_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <string>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/detail/win32_api.hpp>
#else
#  ifdef BOOST_HAS_UNISTD_H
#    include <fcntl.h>
#    include <sys/mman.h>     //mmap
#    include <unistd.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#  else
#    error Unknown platform
#  endif

#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

/*!\file
   Describes memory_mappable and mapped region classes
*/

namespace boost {
namespace interprocess {

/*!The mapped_region class represents a portion or region created from a
   memory_mappable object.*/
class mapped_region
{
   /// @cond
   //Non-copyable
   mapped_region(const mapped_region &);
   mapped_region &operator=(const mapped_region &);
   /// @endcond

   public:

   /*!Creates a mapping region of the mapped memory "mapping", starting in
      offset "offset", and the mapping's size will be "size". The mapping 
      can be opened for read-only "read_only" or read-write 
      "read_write.*/
   template<class MemoryMappable>
   mapped_region(const MemoryMappable& mapping
                ,mode_t mode
                ,offset_t offset = 0
                ,std::size_t size = 0
                ,const void *address = 0);

   /*!Default constructor. Address and size and offset will be 0. Does not throw*/
   mapped_region();

   /*!Move constructor. *this will be constructed taking ownership of "other"'s 
      region and "other" will be left in default constructor state.*/
   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   mapped_region(detail::moved_object<mapped_region> other);
   #else
   mapped_region(mapped_region &&other);
   #endif

   /*!Destroys the mapped region. Does not throw*/
   ~mapped_region();

   /*!Move assignment. If *this owns a memory mapped region, it will be
      destroyed and it will take ownership of "other"'s memory mapped region.*/
   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   mapped_region &operator=(detail::moved_object<mapped_region> other);
   #else
   mapped_region &operator=(mapped_region &&other);
   #endif

   /*!Returns the size of the mapping. Note for windows users: If
      windows_shared_memory is mapped using 0 as the size, it returns 0
      because the size is unknown. Never throws.*/
   std::size_t get_size() const;

   /*!Returns the base address of the mapping. Never throws.*/
   void*       get_address() const;

   /*!Returns the offset of the mapping from the beginning of the
      mapped memory. Never throws.*/
   offset_t    get_offset() const;

   /*!Flushes to the disk a byte range within the mapped memory. 
      Never throws*/
   bool flush(std::size_t mapping_offset = 0, std::size_t numbytes = 0);

   /*!Swaps the mapped_region with another mapped region*/
   void swap(mapped_region &other);

   /*!Returns the size of the page. This size is the minimum memory that
      will be used by the system when mapping a memory mappable source.*/
   static std::size_t get_page_size();

   /// @cond
   private:
   /*!Closes a previously opened memory mapping. Never throws.*/
   void priv_close();

   template<int dummy>
   struct page_size_holder
   {
      static const std::size_t PageSize;
      static std::size_t get_page_size();
   };

   void*             m_base;
   std::size_t       m_size;
   offset_t          m_offset;
   offset_t          m_extra_offset;
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   file_handle_t          m_file_mapping_hnd;
   #endif
   /// @endcond
};

inline void swap(mapped_region &x, mapped_region &y)
{  x.swap(y);  }

#ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
inline mapped_region &mapped_region::operator=(detail::moved_object<mapped_region> other)
{  this->swap(other.get());   return *this;  }
#else
inline mapped_region &mapped_region::operator=(mapped_region &&other)
{  this->swap(other);   return *this;  }
#endif

inline mapped_region::~mapped_region() 
{  this->priv_close(); }

inline std::size_t mapped_region::get_size()  const  
{  return m_size; }

inline offset_t  mapped_region::get_offset()  const  
{  return m_offset;   }

inline void*    mapped_region::get_address()  const  
{  return m_base; }

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline mapped_region::mapped_region()
   :  m_base(0), m_size(0), m_offset(0),  m_extra_offset(0)
   ,  m_file_mapping_hnd(detail::invalid_file())
{}

#ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
inline mapped_region::mapped_region(detail::moved_object<mapped_region> other)
   :  m_base(0), m_size(0), m_offset(0)
   ,  m_extra_offset(0)
   ,  m_file_mapping_hnd(detail::invalid_file())
{  this->swap(other.get());   }
#else
inline mapped_region::mapped_region(mapped_region &&other)
   :  m_base(0), m_size(0), m_offset(0)
   ,  m_extra_offset(0)
   ,  m_file_mapping_hnd(detail::invalid_file())
{  this->swap(other);   }
#endif

template<int dummy>
inline std::size_t mapped_region::page_size_holder<dummy>::get_page_size()
{
   winapi::system_info info;
   get_system_info(&info);
   return std::size_t(info.dwAllocationGranularity);
}

template<class MemoryMappable>
inline mapped_region::mapped_region
   (const MemoryMappable &mapping
   ,mode_t mode
   ,offset_t offset
   ,std::size_t size
   ,const void *address)
   :  m_base(0), m_size(0), m_offset(0),  m_extra_offset(0)
   ,  m_file_mapping_hnd(detail::invalid_file())
{
   mapping_handle_t mhandle = mapping.get_mapping_handle();
   file_handle_t native_mapping_handle;

   //Set accesses
   unsigned long file_map_access = 0;
   unsigned long map_access = 0;

   switch(mode)
   {
      case read_only:
         file_map_access   |= winapi::page_readonly;
         map_access        |= winapi::file_map_read;
      break;
      case read_write:
         file_map_access   |= winapi::page_readwrite;
         map_access        |= winapi::file_map_write;
      break;
      case copy_on_write:
         file_map_access   |= winapi::page_writecopy;
         map_access        |= winapi::file_map_copy;
      break;
      default:
         {
            error_info err(mode_error);
            throw interprocess_exception(err);
         }
      break;
   }

   if(!mhandle.is_shm){
      //Update mapping size if the user does not specify it
      if(size == 0){
         __int64 total_size;
         if(!winapi::get_file_size(detail::file_handle_from_mapping_handle(mapping.get_mapping_handle()), total_size)){
            error_info err(winapi::get_last_error());
            throw interprocess_exception(err);
         }
         if(total_size > (__int64)((std::size_t)(-1))){
            error_info err(size_error);
            throw interprocess_exception(err);
         }
         size = static_cast<std::size_t>(total_size - offset);
      }

      //Create file mapping
      native_mapping_handle = 
         winapi::create_file_mapping
         (detail::file_handle_from_mapping_handle(mapping.get_mapping_handle()), file_map_access, 0, 0, 0);

      //Check if all is correct
      if(!native_mapping_handle){
         error_info err = winapi::get_last_error();
         this->priv_close();
         throw interprocess_exception(err);
      }
   }

   //We can't map any offset so we have to obtain system's 
   //memory granularity
   unsigned long granularity = 0;
   unsigned long foffset_low;
   unsigned long foffset_high;

   winapi::system_info info;
   get_system_info(&info);
   granularity = info.dwAllocationGranularity;

   //Now we calculate valid offsets
   foffset_low  = (unsigned long)(offset / granularity) * granularity;
   foffset_high = (unsigned long)(((offset / granularity) * granularity) >> 32);

   //We calculate the difference between demanded and valid offset
   m_extra_offset = (offset - (offset / granularity) * granularity);

   //Store user values in memory
   m_offset = offset;
   m_size   = size;

   //Update the mapping address
   if(address){
      address = static_cast<const char*>(address) - m_extra_offset;
   }

   if(mhandle.is_shm){
      //Windows shared memory needs the duplication of the handle if we want to
      //make mapped_region independent from the mappable device
      if(!winapi::duplicate_current_process_handle(mhandle.handle, &m_file_mapping_hnd)){
         error_info err = winapi::get_last_error();
         this->priv_close();
         throw interprocess_exception(err);
      }
      native_mapping_handle = m_file_mapping_hnd;
   }

   //Map with new offsets and size
   m_base = winapi::map_view_of_file_ex
                               (native_mapping_handle,
                                map_access, 
                                foffset_high,
                                foffset_low, 
                                m_size ? static_cast<std::size_t>(m_extra_offset + m_size) : 0, 
                                (void*)address);

   if(!mhandle.is_shm){
      //For files we don't need the file mapping anymore
      winapi::close_handle(native_mapping_handle);
   }

   //Check error
   if(!m_base){
      error_info err = winapi::get_last_error();
      this->priv_close();
      throw interprocess_exception(err);
   }

   //Calculate new base for the user
   m_base = static_cast<char*>(m_base) + m_extra_offset;
}

inline bool mapped_region::flush(std::size_t mapping_offset, std::size_t numbytes)
{
   //Check some errors
   if(m_base == 0)
      return false;

   if(mapping_offset >= m_size || (mapping_offset + numbytes) > m_size){
      return false;
   }

   //Update flush size if the user does not provide it
   if(m_size == 0){
      numbytes = 0;
   }
   else if(numbytes == 0){
      numbytes = m_size - mapping_offset;
   }

   //Flush it all
   return 0 == winapi::flush_view_of_file
      (static_cast<char*>(m_base)+mapping_offset, 
       static_cast<std::size_t>(numbytes));
}

inline void mapped_region::priv_close()
{
   if(m_base){
      this->flush();
      winapi::unmap_view_of_file(static_cast<char*>(m_base) - m_extra_offset);
      m_base = 0;
   }
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
      if(m_file_mapping_hnd){
         winapi::close_handle(m_file_mapping_hnd);
         m_file_mapping_hnd = 0;
      }
   #endif
}

#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline mapped_region::mapped_region()
   :  m_base(MAP_FAILED), m_size(0), m_offset(0),  m_extra_offset(0)
{}

#ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
inline mapped_region::mapped_region(detail::moved_object<mapped_region> other)
   :  m_base(MAP_FAILED), m_size(0), m_offset(0),  m_extra_offset(0)
{  this->swap(other.get());   }
#else
inline mapped_region::mapped_region(mapped_region &&other)
   :  m_base(MAP_FAILED), m_size(0), m_offset(0)
   ,  m_extra_offset(0)
{  this->swap(other);   }
#endif

template<int dummy>
inline std::size_t mapped_region::page_size_holder<dummy>::get_page_size()
{  return std::size_t(sysconf(_SC_PAGESIZE)); }

template<class MemoryMappable>
inline mapped_region::mapped_region
   (const MemoryMappable &mapping,
   mode_t mode,
   offset_t offset,
   std::size_t size,
   const void *address)
   :  m_base(MAP_FAILED), m_size(0), m_offset(0),  m_extra_offset(0)
{
   if(size == 0){
//      offset_t filesize = lseek64
      offset_t filesize = lseek
         (mapping.get_mapping_handle(), offset, SEEK_END);
      
      if(filesize == -1 ){
         error_info err(system_error_code());
         throw interprocess_exception(err);
      }
      if(offset >= filesize){
         error_info err(size_error);
         throw interprocess_exception(err);
      }
      filesize -= offset;
      
      size = (size_t)filesize;
      if((offset_t)size != filesize){
         error_info err(size_error);
         throw interprocess_exception(err);         
      }
   }

   //Create new mapping
   int prot    = 0;
   int flags   = 0;

   switch(mode)
   {
      case read_only:
         prot  |= PROT_READ;
         flags |= MAP_SHARED;
      break;
   
      case read_write:
         prot  |= (PROT_WRITE | PROT_READ);
         flags |= MAP_SHARED;
      break;

      case copy_on_write:
         prot  |= PROT_READ;
         flags |= MAP_PRIVATE;
      break;
   
      default:
         {
            error_info err(mode_error);
            throw interprocess_exception(err);
         }
      break;
   }

   //We calculate the difference between demanded and valid offset
   std::size_t page_size = this->get_page_size();
   m_extra_offset = (offset - (offset / page_size) * page_size);

   //Store user values in memory
   m_offset = offset;
   m_size   = size;

   //Update the mapping address
   if(address){
      address = static_cast<const char*>(address) - m_extra_offset;
   }

   //Map it to the address space
//   m_base   = mmap64( (void*)address
   m_base   = mmap  ( (void*)address
                    , static_cast<std::size_t>(m_extra_offset + m_size)
                    , prot
                    , flags
                    , mapping.get_mapping_handle()
                    , offset - m_extra_offset);

   //Check if mapping was successful
   if(m_base == MAP_FAILED){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }

   //Calculate new base for the user
   void *old_base = m_base;
   m_base = static_cast<char*>(m_base) + m_extra_offset;
   m_offset = offset;
   m_size   = size;

   //Check for fixed mapping error
   if(address && (old_base != (void*)address)){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }
}

inline bool mapped_region::flush(std::size_t mapping_offset, std::size_t numbytes)
{
   if(mapping_offset >= m_size || (mapping_offset+numbytes)> m_size){
      return false;
   }

   if(numbytes == 0){
      numbytes = m_size - mapping_offset;
   }
   //Flush it all
   return msync(static_cast<char*>(m_base)+mapping_offset, 
                numbytes, MS_SYNC) == 0;
}

inline void mapped_region::priv_close()
{
   if(m_base != MAP_FAILED){
      this->flush();
      munmap(static_cast<char*>(m_base) - m_extra_offset, m_size + m_extra_offset);
      m_base = MAP_FAILED;
   }
}

#endif   //##if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

template<int dummy>
const std::size_t mapped_region::page_size_holder<dummy>::PageSize
   = mapped_region::page_size_holder<dummy>::get_page_size();

inline std::size_t mapped_region::get_page_size()
{  return page_size_holder<0>::PageSize; }

inline void mapped_region::swap(mapped_region &other)
{
   detail::do_swap(this->m_base, other.m_base);
   detail::do_swap(this->m_size, other.m_size);
   detail::do_swap(this->m_offset, other.m_offset);
   detail::do_swap(this->m_extra_offset,     other.m_extra_offset);
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   detail::do_swap(this->m_file_mapping_hnd, other.m_file_mapping_hnd);
   #endif
}

/// @cond
/*!No-op functor*/
struct null_mapped_region_function
{
   bool operator()(void *, std::size_t , bool) const
      {   return true;   }
};
/// @endcond

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MAPPED_REGION_HPP


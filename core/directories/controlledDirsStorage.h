

#pragma once

#include "../../../tstorage.h"
#include "directoryDescription.h"
#include <map>
#include "../../../helpers.h"
#include <boost/noncopyable.hpp>

namespace ics
{
	namespace directories
	{
		class DirSelector 
		{
		public:
			DirSelector(std::wstring _dirpath) :m_path(_dirpath)
			{
				strings::toUpper(m_path);
			}

			bool operator()(const DirectoryDescription& _entry) const
			{
				return strings::equalStrings(m_path, _entry.szDirPath);
			}

		private:
			std::wstring m_path;
		};

		typedef TStorage<DirectoryDescription, DirSelector> DirContainer;
		typedef std::map<std::wstring, DirectoryDescription> DirsMap;


		class Storage : private boost::noncopyable
		{
		public:

			Storage(std::string _dirStoragePath /*= "ics_directories.szi"*/) :m_dirs(_dirStoragePath)
			{
			}

			// Adds new entry, failed when object was added earlier.
			bool add(const DirectoryDescription& _info);

			// Returns true when storage has information about the object.
			bool isPresent(const std::wstring& _dirpath) const;

			// Returns information about early added directory to control area.
			bool find(const std::wstring& _dirpath, DirectoryDescription& _outInfo) const;

			// Returns information about all controlled directories in map-like view.
			unsigned long toMap(DirsMap& _copyTo) const;

			// Updates information about early added directories, fails if has no info in the storage.
			bool update(const std::wstring& _dirpath, const DirectoryDescription& _newInfo);

			// Changes\Retrieves state of integrity to the specific directory.
			bool setDirStatus(const std::wstring& _dir, ics::IntegrityState _state);
			bool getDirStatus(const std::wstring& _dir, ics::IntegrityState& _state) const;

			// Remove info about directory.
			void remove(const std::wstring& _dirpath);

			// Removes all information.
			void clear();

			void flush()
			{
				std::unique_lock<std::mutex> mtxlocker(m_lock);

				m_dirs.flush();
			}

		private:
			DirContainer m_dirs;
			mutable std::mutex m_lock;
		};
	}
}

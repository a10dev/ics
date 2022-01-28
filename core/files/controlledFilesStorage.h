

#pragma once

#include "../../../tstorage.h"
#include "fileDescription.h"
#include <map>
#include "../../../helpers.h"
#include <boost/noncopyable.hpp>

namespace ics
{
	namespace files
	{
		class FileSelector // File description selector.
		{
		public:
			FileSelector(std::wstring _filepath) :m_filepath(_filepath)
			{
				strings::toUpper(m_filepath);
			}

			bool operator()(const FileDescription& _entry) const
			{
				return strings::equalStrings(m_filepath, _entry.szFilePath);
			}

		private:
			std::wstring m_filepath; // Upper-case string.
		};

		typedef TStorage<FileDescription, FileSelector> FilesContainer; // Has information about all controlled files.
		typedef std::map<std::wstring, FileDescription> FilesMap; // Map-like view for controlled files.


		class Storage : private boost::noncopyable
		{
		public:

			Storage(std::string _filesStoragePath /*= "ics_files.szi"*/) :m_files(_filesStoragePath)
			{
			}

			// Adds new entry, failed when file was added earlier.
			bool add(const FileDescription& _fileinfo);

			// Returns true when storage has information about the file.
			bool isPresent(const std::wstring& _file) const;

			// Returns information about early added file to control area.
			bool find(const std::wstring& _file, FileDescription& _outInfo) const;

			// Returns information about all controlled files in map-like view.
			unsigned long toMap(FilesMap& _copyTo) const;

			// Updates information about early added file, fails if has no info about file in the storage.
			bool update(const std::wstring& _file, const FileDescription& _newInfo);

			// Changes\Retrieves state of integrity to the specific file.
			bool setFileStatus(const std::wstring& _file, ics::IntegrityState _state);
			bool getFileStatus(const std::wstring& _file, ics::IntegrityState& _state) const;

			// Remove info about file.
			void remove(const std::wstring& _file);

			// Removes all information.
			void clear();

			void flush()
			{
				std::unique_lock<std::mutex> mtxlocker(m_lock);

				m_files.flush();
			}

		private:
			FilesContainer m_files;
			mutable std::mutex m_lock;
		};
	}
}

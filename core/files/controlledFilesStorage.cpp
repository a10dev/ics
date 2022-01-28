

#include "controlledFilesStorage.h"

namespace ics
{
	namespace files
	{
		bool Storage::add(const FileDescription& _fileinfo)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			std::wstring filepath = _fileinfo.szFilePath;
			if(!m_files.isPresent(FileSelector(filepath)))
			{
				return m_files.push_back(_fileinfo);
			}

			return false;
		}

		bool Storage::isPresent(const std::wstring& _file) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			return m_files.isPresent(FileSelector(_file));
		}

		bool Storage::find(const std::wstring& _file, FileDescription& _outInfo) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_files.find_if(FileSelector(_file));

			if (pos != FilesContainer::ElementNotFound)
			{
				_outInfo = m_files.at(pos);
			}

			return pos != FilesContainer::ElementNotFound;
		}

		unsigned long Storage::toMap(FilesMap& _copyTo) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			FilesContainer::EntriesList files;
			m_files.toVector(files);
			for (auto file : files)
			{
				std::wstring keypath = file.szFilePath;
				_copyTo[keypath] = file;
			}

			return files.size();
		}

		bool Storage::update(const std::wstring& _file, const FileDescription& _newInfo)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_files.find_if(FileSelector(_file));

			if (pos != FilesContainer::ElementNotFound)
			{
				m_files.at(pos) = _newInfo;
			}

			return pos != FilesContainer::ElementNotFound;
		}

		bool Storage::setFileStatus(const std::wstring& _file, ics::IntegrityState _state)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_files.find_if(FileSelector(_file));

			if (pos != FilesContainer::ElementNotFound)
			{
				m_files.at(pos).status = _state;
			}

			return pos != FilesContainer::ElementNotFound;
		}

		bool Storage::getFileStatus(const std::wstring& _file, ics::IntegrityState& _state) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_files.find_if(FileSelector(_file));

			if (pos != FilesContainer::ElementNotFound)
			{
				_state = m_files.at(pos).status;
			}

			return pos != FilesContainer::ElementNotFound;
		}

		void Storage::remove(const std::wstring& _file)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			m_files.remove_if(FileSelector(_file));
		}

		void Storage::clear()
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			m_files.clear();
		}
	}
}

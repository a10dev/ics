

#include "controlledDirsStorage.h"

namespace ics
{
	namespace directories
	{
		bool Storage::add(const DirectoryDescription& _fileinfo)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			std::wstring filepath = _fileinfo.szDirPath;
			if(!m_dirs.isPresent(DirSelector(filepath)))
			{
				return m_dirs.push_back(_fileinfo);
			}

			return false;
		}

		bool Storage::isPresent(const std::wstring& _dir) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			return m_dirs.isPresent(DirSelector(_dir));
		}

		bool Storage::find(const std::wstring& _dir, DirectoryDescription& _outInfo) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_dirs.find_if(DirSelector(_dir));

			if (pos != DirContainer::ElementNotFound)
			{
				_outInfo = m_dirs.at(pos);
			}

			return pos != DirContainer::ElementNotFound;
		}

		unsigned long Storage::toMap(DirsMap& _copyTo) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			DirContainer::EntriesList dirs;
			m_dirs.toVector(dirs);
			for (auto dir : dirs)
			{
				std::wstring keypath = dir.szDirPath;
				_copyTo[keypath] = dir;
			}

			return dirs.size();
		}

		bool Storage::update(const std::wstring& _dir, const DirectoryDescription& _newInfo)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_dirs.find_if(DirSelector(_dir));

			if (pos != DirContainer::ElementNotFound)
			{
				m_dirs.at(pos) = _newInfo;
			}

			return pos != DirContainer::ElementNotFound;
		}

		bool Storage::setDirStatus(const std::wstring& _dir, ics::IntegrityState _state)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_dirs.find_if(DirSelector(_dir));

			if (pos != DirContainer::ElementNotFound)
			{
				m_dirs.at(pos).status = _state;
			}

			return pos != DirContainer::ElementNotFound;
		}

		bool Storage::getDirStatus(const std::wstring& _dir, ics::IntegrityState& _state) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_dirs.find_if(DirSelector(_dir));

			if (pos != DirContainer::ElementNotFound)
			{
				_state = m_dirs.at(pos).status;
			}

			return pos != DirContainer::ElementNotFound;
		}

		void Storage::remove(const std::wstring& _dir)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			m_dirs.remove_if(DirSelector(_dir));
		}

		void Storage::clear()
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			m_dirs.clear();
		}
	}
}

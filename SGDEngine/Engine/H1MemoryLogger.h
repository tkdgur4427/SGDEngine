#pragma once

namespace SGD
{
namespace Memory
{
	/*
	** memory log system
	- memory log system is dedicated for memory, this is because official log system is not initialized during memory system	
	- this class is singleton! it is only assessable from H1Log<type>
	*/

	template<class CharType>
	class H1MemoryLog
	{
	public:
		static void CreateLog(const CharType* Message);
		static void CreateFormattedLog(const CharType* Format, ...);
		static void ForceToDump();

	protected:
		enum { MAX_LEN = 127, };

		H1Log()
		{
			// empty the data
			memset(Data, 0, sizeof(Data));
		}

	public:
		// log data
		CharType Data[MAX_LEN + 1];
	};

	template<class CharType>
	class H1MemoryLogger
	{
	public:
		H1MemoryLogger()
		{
			// initialize logger
			Initialize();
		}

		~H1MemoryLogger()
		{

		}

		// friend class (only accessible from H1Log)
		friend class H1MemoryLog<CharType>;

	protected:
		static H1MemoryLogger* GetSingleton()
		{
			static H1MemoryArenaLogger MemoryArenaLogger;
			return &MemoryArenaLogger;
		}

		// Header layout
		struct HeaderLayout
		{
			// tracking current log index
			int32 CurrLogIndex;
		};

		enum
		{
			LogSize = sizeof(H1Log<CharType>),
			// memory arena log page size is 2MB
			LogPageSize = 2 * 1024 * 1024,
			// log count per page
			// (2MB - pointer size (Head and FreeHead)) / LogSize
			HeaderLayoutSize = sizeof(HeaderLayout),
			LogCountPerPage = (LogPageSize - HeaderLayoutSize) / LogSize,
			// log count limit
			// currently we limit the log count as page size
			LogCountLimit = LogCountPerPage,
		};

		// real data layout
		struct LoggerLayout
		{
			// header layout
			HeaderLayout Header;

			// logs
			H1Log<CharType> Logs[LogCountPerPage];
		};

		union
		{
			// logger layout
			LoggerLayout Layout;

			// data size limitation
			byte Data[LogPageSize];
		};

		// construct logger
		void Initialize()
		{
			memset(Data, 0, sizeof(Data));
			Layout.Header.CurrLogIndex = 0; // reset the current log index
		}

		// create new log
		//	- give temporary log pointer (this should be used as scoped pointer do not store this as member variables)
		H1Log<CharType>* CreateLog()
		{
			// if it reach to the limit dump the result
			if (Layout.Header.CurrLogIndex == LogCountLimit)
			{
				DumpLogs();
			}

			H1Log<CharType>* Result = &Layout.Logs[Layout.Header.CurrLogIndex];
			Layout.Header.CurrLogIndex++;

			return Result;
		}

		// dump the log
		void DumpLogs()
		{
			// dump the log
			for (int32 LogIndex = 0; LogIndex < Layout.Header.CurrLogIndex; ++LogIndex)
			{
				H1Log<CharType>& LogElement = Layout.Logs[LogIndex];
				SGD::Platform::Util::appOutputDebugString(LogElement.Data);
			}

			// reset the logs
			Initialize();
		}
	};

	// static member functions
	template<class CharType>
	void H1Log<CharType>::CreateLog(const CharType* Message)
	{
		H1Log<CharType>* NewLog = H1MemoryArenaLogger<CharType>::GetSingleton()->CreateLog();

		// string copy with safe strcpy
		strcpy_s(NewLog->Data, H1Log<CharType>::MAX_LEN, Message);
	}

	template<class CharType>
	void H1Log<CharType>::CreateFormattedLog(const CharType* Format, ...)
	{
		H1Log<CharType>* NewLog = H1MemoryArenaLogger<CharType>::GetSingleton()->CreateLog();

		// format the log
		va_list Args;
		va_start(Args, Format);
		vsnprintf(NewLog->Data, H1Log<CharType>::MAX_LEN, Format, Args);
		va_end(Args);
	}

	template<class CharType>
	void H1Log<CharType>::ForceToDump()
	{
		// force to dump logs (special usage for checkf)
		H1MemoryArenaLogger<CharType>::GetSingleton()->DumpLogs();
	}
}
}
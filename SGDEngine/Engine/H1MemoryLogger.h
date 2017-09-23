#pragma once

#include "H1GlobalSingleton.h"

namespace SGD
{
namespace Memory
{
	// forward declaration
	template <class CharType>
	class H1MemoryLogger;

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
		enum 
		{ 
			MAX_LEN = 127, 
		};

		H1MemoryLog()
			: Next(nullptr)
		{
			// empty the data
			memset(Data, 0, sizeof(Data));
		}

	public:
		// log data
		CharType Data[MAX_LEN + 1];

	private:
		// only accessible from class H1MemoryLogger
		friend class H1MemoryLogger<CharType>;

		H1MemoryLog<CharType>* Next;
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
			static H1MemoryLogger MemoryArenaLogger;
			return &MemoryArenaLogger;
		}

		// Header layout
		struct HeaderLayout
		{
			H1MemoryLog<CharType>* FreeHead;
		};

		enum
		{
			LogSize = sizeof(H1MemoryLog<CharType>),
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
			H1MemoryLog<CharType> Logs[LogCountPerPage];
		};

		union
		{
			// logger layout
			LoggerLayout Layout;

			// data size limitation (bounded)
			byte Data[LogPageSize];
		};

		// construct logger
		void Initialize()
		{
			memset(Data, 0, sizeof(Data));

			// create free list
			for (int32 Index = 0; Index < LogCountPerPage; ++Index)
			{
				H1MemoryLog<CharType>* CurrLog = &Layout.Logs[Index];

				CurrLog->Next = Layout.Header.FreeHead;
				Layout.Header.FreeHead = CurrLog;
			}
		}

		// create new log
		//	- give temporary log pointer (this should be used as scoped pointer do not store this as member variables)
		H1MemoryLog<CharType>* CreateLog()
		{
			H1MemoryLog<CharType>* Result = nullptr;
			H1MemoryLog<CharType>* NewHead = nullptr;

			// get the result log in lock-free
			do 
			{
				Result = Layout.Header.FreeHead;				
				NewHead = Result->Next;
			} 
			while ((H1MemoryLog<CharType>*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(Layout.Header.FreeHead), (int64)NewHead, (int64)Result) != Result);

			return Result;
		}

		// dump the log
		void DumpLogs()
		{
			H1MemoryLog<CharType>* DumpHead = nullptr;
			H1MemoryLog<CharType>* NewHead = nullptr;

			// get the head safely and converting its head to nullptr
			do 
			{
				DumpHead = Layout.Header.FreeHead;
			} 
			while ((H1MemoryLog<CharType>*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(Layout.Header.FreeHead), (int64)NewHead, (int64)DumpHead) != DumpHead);

			// now looping dumping head and print out
			H1MemoryLog<CharType>* CurrLog = DumpHead;
			while (CurrLog != nullptr)
			{
				// note that appOutputDebugString is safe to MT env.
				SGD::Platform::Util::appOutputDebugString(CurrLog->Data);
				CurrLog = CurrLog->Next;
			}
		}
	};

	// static member functions
	template<class CharType>
	void H1MemoryLog<CharType>::CreateLog(const CharType* Message)
	{
		H1MemoryLog<CharType>* NewLog = H1MemoryLogger<CharType>::GetSingleton()->CreateLog();

		// string copy with safe strcpy
		strcpy_s(NewLog->Data, H1MemoryLog<CharType>::MAX_LEN, Message);
	}

	template<class CharType>
	void H1MemoryLog<CharType>::CreateFormattedLog(const CharType* Format, ...)
	{
		H1MemoryLog<CharType>* NewLog = H1MemoryLogger<CharType>::GetSingleton()->CreateLog();

		// format the log
		va_list Args;
		va_start(Args, Format);
		vsnprintf(NewLog->Data, H1MemoryLog<CharType>::MAX_LEN, Format, Args);
		va_end(Args);
	}

	template<class CharType>
	void H1MemoryLog<CharType>::ForceToDump()
	{
		// force to dump logs (special usage for checkf)
		H1MemoryLogger<CharType>::GetSingleton()->DumpLogs();
	}
}
}

// only for memory use formatted debugf and checkf
#if !FINAL_RELEASE

// memory debugf
// debugf with message
#define h1MemDebug(message) H1MemoryLog<char>::CreateLog(message);
// debugf with formatted message
#define h1MemDebugf(format, ...) H1MemoryLog<char>::CreateFormattedLog(format, __VA_ARGS__);
// memory checkf
// checkf with message
#define h1MemCheck(condition, meessage) if(!(condition)) { H1MemoryLog<char>::CreateLog(meessage); H1MemoryLog<char>::ForceToDump(); __debugbreak(); }
// checkf with formatted message
#define h1MemCheckf(condition, format, ...) if(!(condition)) { H1MemoryLog<char>::CreateFormattedLog(format, __VA_ARGS__); H1MemoryLog<char>::ForceToDump(); __debugbreak(); }

#else

// disable debugf and checkf for final release mode
#define h1MemDebug(message) __noop
#define h1MemDebugf(format, ...) __noop
#define h1MemCheck(condition, meessage) __noop
#define h1MemCheckf(condition, format, ...) __noop

#endif
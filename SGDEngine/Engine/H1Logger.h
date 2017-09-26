#pragma once

namespace SGD
{
namespace Log
{
	// forward declaration
	template <class CharType>
	class H1Logger;

	/*
	** simple log system
	- this class is singleton! it is only assessable from H1Log<type>
	*/

	template<class CharType>
	class H1Log
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

		H1Log()
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
		friend class H1Logger<CharType>;

		H1Log<CharType>* Next;
	};

	template<class CharType>
	class H1Logger
	{
	public:
		H1Logger()
		{
			// initialize logger
			Initialize();
		}

		~H1Logger()
		{

		}

		// friend class (only accessible from H1Log)
		friend class H1Log<CharType>;

	protected:
		static H1Logger* GetSingleton()
		{
			static H1Logger MemoryArenaLogger;
			return &MemoryArenaLogger;
		}

		// Header layout
		struct HeaderLayout
		{
			H1Log<CharType>* FreeHead;
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

			// preventing ABA problem
			AddressBitNum		= 44,	// in windows system, maximum allowed bits is 44
			AddressTagBitNum	= 20,
		};

		// address mask for preventing ABA problem
		const uint64 AddressTagMask		= (uint64)(~0) << AddressBitNum;
		const uint64 AddressMask		= (uint64)(~0) >> AddressTagBitNum;

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
				H1Log<CharType>* CurrLog = &Layout.Logs[Index];

				CurrLog->Next = Layout.Header.FreeHead;
				Layout.Header.FreeHead = CurrLog;
			}
		}

		// create new log
		//	- give temporary log pointer (this should be used as scoped pointer do not store this as member variables)
		H1Log<CharType>* CreateLog()
		{
			if (Layout.Header.FreeHead == nullptr)
			{
				// it could happen dumping multiple time, when the logs is out of stock, but it is just happened at that moment
				// after that, it resolve all problems and just running on normal
				DumpLogs();
			}

			H1Log<CharType>* OldHead = nullptr;
			H1Log<CharType>* NewHead = nullptr;
			H1Log<CharType>* Result = nullptr;

			// get the result log in lock-free
			do
			{
				OldHead = Layout.Header.FreeHead;

				uint64 Tag = ((uint64)OldHead & AddressTagMask) >> AddressBitNum;
				Tag++;	// increment tag count

				// extract real data address
				H1Log<CharType>* Data = (H1Log<CharType>*)((uint64)OldHead & AddressMask);
				Result = Data;

				NewHead = Data->Next;
				// add tag count to the new head
				NewHead = (H1Log<CharType>*)((uint64)NewHead | (Tag << AddressBitNum));

			} while (Layout.Header.FreeHead != nullptr && (H1Log<CharType>*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(Layout.Header.FreeHead), (int64)NewHead, (int64)OldHead) != OldHead);

			return Result;
		}

		// dump the log
		void DumpLogs()
		{
			H1Log<CharType>* OldHead = nullptr;
			H1Log<CharType>* NewHead = nullptr;
			H1Log<CharType>* Result = nullptr;

			// get the head safely and converting its head to nullptr
			do
			{
				OldHead = Layout.Header.FreeHead;

				uint64 Tag = ((uint64)OldHead & AddressTagMask) >> AddressBitNum;
				Tag++;	// increment tag count

				// extract real data address
				H1Log<CharType>* Data = (H1Log<CharType>*)((uint64)OldHead & AddressMask);
				Result = Data;

				NewHead = nullptr;
				// add tag count to the new head
				NewHead = (H1Log<CharType>*)((uint64)NewHead | (Tag << AddressBitNum));

			} while ((H1Log<CharType>*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(Layout.Header.FreeHead), (int64)NewHead, (int64)OldHead) != OldHead);

			// now looping dumping head and print out
			H1Log<CharType>* CurrLog = Result;
			while (CurrLog != nullptr)
			{
				// note that appOutputDebugString is safe to MT env.
				SGD::Platform::Util::appOutputDebugString(CurrLog->Data);
				CurrLog = CurrLog->Next;
			}

			// re-attach the nodes to reuse
			do
			{
				OldHead = Layout.Header.FreeHead;

				uint64 Tag = ((uint64)OldHead & AddressTagMask) >> AddressBitNum;
				Tag++;	// increment tag count

				NewHead = Result;
				// add tag count to the new head
				NewHead = (H1Log<CharType>*)((uint64)NewHead | (Tag << AddressBitNum));

			} while ((H1Log<CharType>*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(Layout.Header.FreeHead), (int64)NewHead, (int64)OldHead) != OldHead);
		}
	};

	// static member functions
	template<class CharType>
	void H1Log<CharType>::CreateLog(const CharType* Message)
	{
		H1Log<CharType>* NewLog = H1Logger<CharType>::GetSingleton()->CreateLog();

		// string copy with safe strcpy
		strcpy_s(NewLog->Data, H1Log<CharType>::MAX_LEN, Message);
	}

	template<class CharType>
	void H1Log<CharType>::CreateFormattedLog(const CharType* Format, ...)
	{
		H1Log<CharType>* NewLog = H1Logger<CharType>::GetSingleton()->CreateLog();

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
		H1Logger<CharType>::GetSingleton()->DumpLogs();
	}
}
}

// only for memory use formatted debugf and checkf
#if !FINAL_RELEASE

// memory debugf
// debugf with message
#define h1Debug(message) SGD::Log::H1Log<char>::CreateLog(message);
// debugf with formatted message
#define h1Debugf(format, ...) SGD::Log::H1Log<char>::CreateFormattedLog(format, __VA_ARGS__);
// memory checkf
// checkf with message
#define h1Check(condition, meessage) if(!(condition)) { SGD::Log::H1Log<char>::CreateLog(meessage); SGD::Log::H1Log<char>::ForceToDump(); __debugbreak(); }
// checkf with formatted message
#define h1Checkf(condition, format, ...) if(!(condition)) { SGD::Log::H1Log<char>::CreateFormattedLog(format, __VA_ARGS__); SGD::Log::H1Log<char>::ForceToDump(); __debugbreak(); }

#else

// disable debugf and checkf for final release mode
#define h1Debug(message) __noop
#define h1Debugf(format, ...) __noop
#define h1Check(condition, meessage) __noop
#define h1Checkf(condition, format, ...) __noop

#endif
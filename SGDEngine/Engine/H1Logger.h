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
			H1Log<CharType>* Result = nullptr;
			H1Log<CharType>* NewHead = nullptr;

			// get the result log in lock-free
			do
			{
				Result = Layout.Header.FreeHead;
				NewHead = Result->Next;
			} while ((H1Log<CharType>*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(Layout.Header.FreeHead), (int64)NewHead, (int64)Result) != Result);

			return Result;
		}

		// dump the log
		void DumpLogs()
		{
			H1Log<CharType>* DumpHead = nullptr;
			H1Log<CharType>* NewHead = nullptr;

			// get the head safely and converting its head to nullptr
			do
			{
				DumpHead = Layout.Header.FreeHead;
			} while ((H1Log<CharType>*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(Layout.Header.FreeHead), (int64)NewHead, (int64)DumpHead) != DumpHead);

			// now looping dumping head and print out
			H1Log<CharType>* CurrLog = DumpHead;
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
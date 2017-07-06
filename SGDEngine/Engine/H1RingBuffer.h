#pragma once

#include "H1Memory.h"

namespace SGD
{
namespace Util
{
	class H1RingBufferLayout
	{
	public:
		// only allowed to create this ring buffer layout by H1RingBuffer
		template <typename NodeType>
		friend class H1RingBuffer;

		// this log ring buffer is special case 
		template <typename CharType>
		struct H1LogRingBuffer;

	protected:
		H1RingBufferLayout()
		{
			
		}

		void Initialize(byte* BaseAddress, int32 Size)
		{
			// setting properties for initialization
			StartAddress = BaseAddress;
			EndAddress = StartAddress + Size;

			ProducerPosition = StartAddress;
			ConsumerPosition = StartAddress;

			MaxToProduce = Size;
			MaxToConsume = 0;
			DataSizeInBuffer = 0;

			// update the state
			UpdateState();
		}

		void Destroy()
		{
			// invalidate the address
			StartAddress = nullptr;
			EndAddress = nullptr;

			ProducerPosition = nullptr;
			ConsumerPosition = nullptr;

			MaxToProduce = -1;
			MaxToConsume = -1;
			DataSizeInBuffer = -1;
		}

		// produce
		//	- try to add data to the buffer
		//	- after the call, 'Size' contains the amount of bytes actually buffered
		byte* Produce(const byte* InData, int32& Size)
		{
			// clamp max to produce and reflect the Size
			if (Size > MaxToProduce)
			{
				Size = MaxToProduce;
			}

			byte* Result = ProducerPosition;

			if (InData != nullptr)
			{
				SGD::Platform::Util::appMemcpy(InData, ProducerPosition, Size);
			}				

			ProducerPosition += Size;
			DataSizeInBuffer += Size;

			UpdateState();

			return Result;
		}

		// consume
		//	- request 'Size' bytes from the buffer
		//	- after the call, 'Size' contains the amount of bytes actually copied
		void Consume(byte* OutData, int32& Size)
		{
			// clamp max to consume and reflect the size
			if (Size > MaxToConsume)
			{
				Size = MaxToConsume;
			}

			if (OutData != nullptr)
			{
				SGD::Platform::Util::appMemcpy(ConsumerPosition, OutData, Size);
			}			

			ConsumerPosition += Size;
			DataSizeInBuffer -= Size;

			UpdateState();
		}

		// skip
		//	- tries to skip Size bytes
		//	- after the call, 'Size' contains the realized skip
		void Skip(int32& Size)
		{
			int32 RequestedSkip = Size;

			// this may wrap so try it twice (max)
			for (int32 Step = 0; Step < 2; ++Step)
			{
				int32 SkipSize = Size;

				// clamp to MaxToConsume (aligned to EndAddress)
				if (SkipSize > MaxToConsume)
				{
					SkipSize = MaxToConsume;
				}

				ConsumerPosition += SkipSize;
				DataSizeInBuffer -= SkipSize;

				// update the size
				Size -= SkipSize;

				// update the state
				UpdateState();
			}

			// final update size (restore the size)
			Size = RequestedSkip - Size;
		}

		// the amount of data the buffer can currently receive on one Produce() call
		int32 ProduceMaxSize() { return MaxToProduce; }

		// the total amount of data in the buffer
		// note that it may not be contiguous; you may need two successive calls to Consume() call to get it all
		int32 TotalBufferedSize() { return DataSizeInBuffer; }


		// update ring buffer proper state
		void UpdateState()
		{
			// handling the case that consumer/producer position reaches to the end address
			if (ConsumerPosition == EndAddress)
			{
				ConsumerPosition = StartAddress;
			}

			if (ProducerPosition == EndAddress)
			{
				ProducerPosition = StartAddress;
			}

			// when it reach to the end(or at the beginning)
			if (ProducerPosition == ConsumerPosition)
			{
				// if there is any data to consume
				if (DataSizeInBuffer > 0)
				{
					MaxToProduce = 0;
					MaxToConsume = EndAddress - ConsumerPosition;
				}
				else // if there is nothing to consume
				{
					MaxToProduce = EndAddress - ProducerPosition;
					MaxToConsume = 0;
				}
			}

			else if (ProducerPosition > ConsumerPosition)
			{
				MaxToProduce = EndAddress - ProducerPosition;
				MaxToConsume = ProducerPosition - ConsumerPosition;
			}

			else
			{
				MaxToProduce = ConsumerPosition - ProducerPosition;
				MaxToConsume = EndAddress - ConsumerPosition;
			}
		}

		byte* ProducerPosition;
		byte* ConsumerPosition;

		byte* StartAddress;
		byte* EndAddress;

		int32 MaxToProduce;
		int32 MaxToConsume;
		int32 DataSizeInBuffer;
	};

	// public RingBuffer handling memory management also
	// example node class using in RingBuffer
	struct H1RingBufferNodeLayout
	{

	};

	template <typename NodeType /*H1RingBufferNodeLayout*/>
	class H1GenericRingBuffer
	{
	public:

	protected:
		// memory blocks (multiple)
		SGD::Memory::H1MemoryBlockRange MemoryBlockRange;
	};
}
}
// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include <concepts>

namespace PG 
{
#pragma region Concepts

	template<typename T, typename TDefaultValueFunc>
	concept CircularBufferConcept =
		std::default_initializable<T> &&
		std::default_initializable<TDefaultValueFunc> &&
		requires(TDefaultValueFunc func)
	{
		{
			func()
		} -> std::convertible_to<T>;
	};

	template<typename T>
	concept CircularBufferSumConcept = requires(T t1, T t2)
	{
		{
			t1 + t2
		} -> std::convertible_to<T>;
	};

	template<typename T>
	concept CircularBufferDifferenceConcept = requires(T t1, T t2)
	{
		{
			t1 - t2
		} -> std::convertible_to<T>;
	};

	template<typename T>
	concept CircularBufferAverageConcept = CircularBufferSumConcept<T> &&
		requires(T t, std::uint32_t N)
	{
		{
			t / N
		} -> std::convertible_to<T>;
	};

	template<typename T>
	concept Numeric = std::is_integral_v<T> || std::is_floating_point_v<T>;

	template< typename T, typename TFunc, typename TThreshold>
	concept CircularBufferMangitudeConcept = Numeric<TThreshold> &&
		requires(TFunc f, T t)
	{
		{
			f(t)
		} -> std::convertible_to<TThreshold>;
	};

#pragma endregion Concepts
	/**
	 *
	 */
	template<typename T, 
			 typename TDefaultValueFunc = decltype([]() { return T{}; }),
		     typename TDefaultThresholdFunc = decltype([](const T& t) { return FMath::Abs(t); })
	> requires CircularBufferConcept<T, TDefaultValueFunc>
	class TTimedCircularBuffer
	{
	public:
		/*
		* Constructor.
		* 
		* @param MeasurementIntervalSeconds The approx range of time to store values
		* @param PopulationIntervalSeconds The approx rate at which values will be added to the buffer
		*/
		TTimedCircularBuffer(float MeasurementIntervalSeconds, float ExpectedAddRateSeconds);

		/*
		* Default initialize the circular buffer with a size of 1.
		*/
		TTimedCircularBuffer();
		explicit TTimedCircularBuffer(int32 NumSamples);

		uint32 Capacity() const;
		uint32 Size() const;
		bool IsEmpty() const;
		bool IsFull() const;

		T Average() const requires CircularBufferAverageConcept<T>;
		T Sum() const requires CircularBufferSumConcept<T>;
		T Delta() const requires CircularBufferDifferenceConcept<T>;

		template<typename TThreshold, typename TFunc = TDefaultThresholdFunc>
		bool IsZero(const TThreshold& Threshold = {}, const TFunc& Func = {}) const
			requires CircularBufferSumConcept<T> && CircularBufferMangitudeConcept<T, TFunc, TThreshold>;

		void Add(T&& Value) requires std::movable<T>;
		void Add(const T& Value) requires std::assignable_from<T&, T>;
		void Clear();
		void ClearAndResize(int32 NewNumSamples);

	private:
		uint32 NextIndex() const;


	private:
		TArray<T> Buffer{};
		uint32 Count{};
	};
}

#pragma region Template Definitions

namespace PG
{
#pragma region Inline Definitions

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	inline bool TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::IsEmpty() const
	{
		return Count == 0;
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	inline bool TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::IsFull() const
	{
		return Count >= Capacity();
	}
	 
	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	inline void TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::Add(T&& Value) requires std::movable<T>
	{
		Buffer[NextIndex()] = std::move(Value);
		++Count;
		// We could overflow here but since using unsigned that would just clear the buffer which isn't terrible so no need to put in a truncation
		// This is merely theoretical and actual usage will never overflow
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	inline void TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::Add(const T& Value) requires std::assignable_from<T&, T>
	{
		Buffer[NextIndex()] = Value;
		++Count;
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	inline void TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::Clear()
	{
		Count = 0;
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	inline uint32 TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::NextIndex() const
	{
		return Count % Capacity();
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	template<typename TThreshold, typename TFunc>
	inline bool TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::IsZero(const TThreshold& Threshold, const TFunc& Func) const
		requires CircularBufferSumConcept<T>&& CircularBufferMangitudeConcept<T, TFunc, TThreshold>
	{
		return Func(Sum()) <= Threshold;
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	inline T TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::Average() const requires CircularBufferAverageConcept<T>
	{
		const auto NumElements = Size();
		return NumElements > 0 ? Sum() / NumElements : (TDefaultValueFunc{})();
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	inline uint32 TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::Capacity() const
	{
		return Buffer.Num();
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	inline uint32 TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::Size() const
	{
		return FMath::Min(Count, Capacity());
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	inline TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::TTimedCircularBuffer() : TTimedCircularBuffer(1) {}

#pragma endregion Inline Definitions

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::TTimedCircularBuffer(float MeasurementIntervalSeconds, float ExpectedAddRateSeconds)
	{
		const auto BufferSize = FMath::CeilToInt32(MeasurementIntervalSeconds / ExpectedAddRateSeconds);

		checkf(MeasurementIntervalSeconds > 0 && ExpectedAddRateSeconds > 0 && BufferSize > 0,
			TEXT("MeasurementIntervalSeconds=%f; ExpectedAddRateSeconds=%f; Count=%d"), MeasurementIntervalSeconds, ExpectedAddRateSeconds, BufferSize);

		Buffer.AddZeroed(BufferSize);
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::TTimedCircularBuffer(int32 NumSamples)
	{
		checkf(NumSamples > 0, TEXT("NumSamples=%d"), NumSamples);

		Buffer.AddZeroed(NumSamples);
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	T TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::Sum() const requires CircularBufferSumConcept<T>
	{
		T SumValue{ (TDefaultValueFunc{})()};

		for (uint32 i = 0, Len = Size(); i < Len; ++i)
		{
			SumValue = SumValue + Buffer[i];
		}

		return SumValue;
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	T TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::Delta() const requires CircularBufferDifferenceConcept<T>
	{
		uint32 MinIndex, MaxIndex, Len;

		if (IsFull())
		{
			MinIndex = NextIndex();
			MaxIndex = MinIndex > 0 ? MinIndex - 1 : Capacity() - 1;
			Len = Capacity();
		}
		else
		{
			MinIndex = 0;
			Len = Size();
			MaxIndex = Len - 1;
		}

		if (Len == 0)
		{
			return TDefaultValueFunc{}();
		}
		else if (Len == 1)
		{
			return Buffer[MinIndex];
		}

		return Buffer[MaxIndex] - Buffer[MinIndex];
	}

	template<typename T, typename TDefaultValueFunc, typename TDefaultThresholdFunc> requires CircularBufferConcept<T, TDefaultValueFunc>
	void TTimedCircularBuffer<T, TDefaultValueFunc, TDefaultThresholdFunc>::ClearAndResize(int32 NewNumSamples)
	{
		checkf(NewNumSamples > 0, TEXT("NewNumSamples=%d"), NewNumSamples);

		Buffer.Reset(NewNumSamples);
		Buffer.AddZeroed(NewNumSamples);

		Clear();
	}

}
#pragma endregion Template Definitions

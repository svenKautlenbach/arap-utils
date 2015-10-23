#pragma once

#include <cassert>
#include <ctime>
#include <vector>

namespace arap
{
	class Timer
	{
	public:
		Timer() = default;
		~Timer() = default;
		
		virtual void set(uint32_t seconds) = 0;
		virtual void set(std::vector<uint32_t> intervals) = delete;
		virtual void reset() = 0;

		virtual void pause() = 0;
		virtual void stop() = 0;
		virtual void run() = 0;

		virtual bool expired() = 0;
		virtual uint32_t elapsed() = 0;
		virtual uint32_t nextTimeout() = 0;
	};

	class SimpleTimer final : Timer
	{
	public:
		SimpleTimer() : m_timeoutDuration(0), m_timeoutEpoch(0), m_pauseContinuum(0)
		{}
		
		SimpleTimer(uint32_t timeout) : SimpleTimer() 
		{
			set(timeout);
		}

		void set(uint32_t seconds) override
		{
			m_timeoutDuration = seconds;
			restart();
		}

		// Start counting from the last timeout.
		void reset() override
		{
			if (m_timeoutEpoch == 0)
				return restart(); // Except this one, since timer was stopped.

			if (m_timeoutDuration > 0)
				m_timeoutEpoch += m_timeoutDuration;

			m_pauseContinuum = 0;
		}

		// Start counting from now.
		void restart()
		{
			if (m_timeoutDuration > 0)
				m_timeoutEpoch = std::time(nullptr) + m_timeoutDuration;
			
			m_pauseContinuum = 0;
		}
		
		void pause() override { if (!expired()) m_pauseContinuum = elapsed();}
		void stop() override { m_timeoutEpoch = m_pauseContinuum = 0; }
		void run() override 
		{
			if (m_pauseContinuum == 0)
			{
				restart();
				return;
			}
		
			assert(m_pauseContinuum < m_timeoutDuration);
			auto remainedTime = m_timeoutDuration - m_pauseContinuum;
			m_timeoutEpoch += remainedTime;
			m_pauseContinuum = 0;
		};
	
		bool expired() override
		{
			if (m_pauseContinuum > 0)
				return false;

			if (m_timeoutEpoch == 0)
				return false;
			
			return m_timeoutEpoch <= std::time(nullptr);
		}

		uint32_t elapsed() override
		{
			if (expired())
				return m_timeoutDuration;

			if (m_pauseContinuum > 0)
				return m_pauseContinuum;

			if (m_timeoutEpoch == 0)
				return 0;

			return std::time(nullptr) - (m_timeoutEpoch - m_timeoutDuration);
		}

		uint32_t nextTimeout() override
		{
			if (expired())
				return 0;

			return m_timeoutDuration - elapsed();
		}

		// Default keyword does not work like I think it is supposed to, currently disabled.
		//SimpleTimer(const SimpleTimer& other) { return other; }
		//SimpleTimer(SimpleTimer&& other) = default;

		~SimpleTimer(){}
	private:
		uint32_t m_timeoutDuration;
		time_t m_timeoutEpoch;
		uint32_t m_pauseContinuum;
	};
}

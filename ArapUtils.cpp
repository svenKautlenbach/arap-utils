#include "ArapUtils.h"

#include <cstring>
#include <ctime>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <climits>
#include <cstdlib>

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace arap
{
	namespace linuxOS
	{
		pid_t Utilities::getPid(const std::string& processEntry)
		{
			std::string command = "ps | grep \"" + processEntry + "\" | grep -v grep";
			FILE* pipeStream = popen(command.c_str(), "r");
			usleep(100 * 1000);

			auto entries = strings::Utilities::getLines(pipeStream);
			if (entries.size() < 1)
			{
				std::cout << "Process - " << processEntry << " - not found." << std::endl;
				
				throw std::exception();
			}
			
			auto convertedValue = strtoul(entries.at(0).c_str(), nullptr, 10);
			if (convertedValue == 0 || convertedValue == ULONG_MAX)
			{
				diagnostics::Print::errnoDescription();

				throw std::exception();
			}

			return static_cast<pid_t>(convertedValue);
		}

		AppMessenger::AppMessenger(const std::string& fifoName) : m_fifoName(fifoName)
		{
			if (mkfifo(m_fifoName.c_str(), 0666) != 0)
			{
				if (errno != EEXIST)
				{
					diagnostics::Print::errnoDescription();

					throw std::runtime_error("Cannot create the FIFO - " + m_fifoName + ".");
				}
			}
		}

		bool AppMessenger::messagesAvailable()
		{
			openForReading();

			short eventPolled = POLLIN;
	
			struct pollfd pollDescriptor = {m_fileDescriptor, eventPolled, 0};
			
			auto pollResult = poll(&pollDescriptor, 1, 0);
			
			closeChannel();	

			if (pollResult < 0)
			{
				diagnostics::Print::errnoDescription();
				throw std::runtime_error("Polling " + m_fifoName + " resulted in error.");
			}
			
			if (pollResult == 0)
			{
				return false;
			}

			if ((pollDescriptor.revents & eventPolled) == 0)
			{
				return false;
			}

			return true;
		}

		std::string AppMessenger::getLastMessage()
		{
			openForReading();
			
			char dataBuffer[1024];
			std::string message;
			while (true)
			{
				auto readResult = read(m_fileDescriptor, dataBuffer,1024);
				if (readResult < 0)
				{
					if (errno == EAGAIN || errno == EINTR)
					{
						continue;
					}
					
					diagnostics::Print::errnoDescription();
					throw std::runtime_error("Crucial error occured when reading " + m_fifoName + ".");
				}

				if (readResult > 0)
				{
					message += std::string(dataBuffer, readResult);
				}

				if (readResult == 0)
				{
					break;
				}
			}

			closeChannel();

			return message;
		}

		void AppMessenger::sendMessage(const std::string& message)
		{
			openForWriting();

			auto messageSize = message.size();
			auto writeStatus = write(m_fileDescriptor, message.c_str(), messageSize);

			if (writeStatus != static_cast<ssize_t>(messageSize))
			{
				arap::diagnostics::Print::errnoDescription();
				throw std::runtime_error("Writing to " + m_fifoName + " resulted in error.");
			}

			closeChannel();
		}

		AppMessenger::~AppMessenger()
		{
			unlink(m_fifoName.c_str());
		}

		void AppMessenger::openForReading()
		{
			auto result = open(m_fifoName.c_str(), O_RDONLY | O_NONBLOCK);
			if (result == -1)
			{
				diagnostics::Print::errnoDescription();

				throw std::runtime_error("Cannot open reading channel to FIFO - " + m_fifoName + ".");
			}

			m_fileDescriptor = result;
		}

		void AppMessenger::openForWriting()
		{
			auto result = open(m_fifoName.c_str(), O_WRONLY);
			if (result == -1)
			{
				diagnostics::Print::errnoDescription();

				throw std::runtime_error("Cannot open writing channel to FIFO - " + m_fifoName + ".");
			}

			m_fileDescriptor = result;
		}

		void AppMessenger::closeChannel()
		{
			close(m_fileDescriptor);
		}
	}

	namespace strings
	{
		std::vector<std::string> Utilities::getLines(FILE* fileHandle)
		{
			std::vector<std::string> lines;

			char* lineBuffer = nullptr;
			size_t lineLength = 0;
			ssize_t lineStatus; 
			while ((lineStatus = getline(&lineBuffer, &lineLength, fileHandle)) > 0)
			{
				lines.emplace_back(lineBuffer);
			}

			return lines;
		}
		
		std::vector<std::string> Utilities::getLines(std::ifstream& fileStream)
		{
			std::vector<std::string> lines;
		
			while (fileStream.eof() == false)
			{
				std::string line;
				std::getline(fileStream, line);
				lines.push_back(line);
			}

			return lines;
		}
			
		std::vector<std::string> Utilities::getLines(const std::string& filePath)
		{
			std::ifstream inputFile(filePath);

			if (inputFile.bad() || inputFile.is_open() == false)
				throw std::runtime_error("Cannot open a file " + filePath + ".");

			auto lines = getLines(inputFile);

			inputFile.close();

			return lines;
		}
		
		std::vector<std::string> Utilities::split(const std::string& source, const std::string& delimiter)
		{
			std::vector<std::string> elements;
			std::stringstream sourceStream(source);

			std::string splittedString;
			while (std::getline(sourceStream, splittedString, delimiter.c_str()[0]))
			{
				elements.push_back(splittedString);
			}

			return elements;
		}
	}
	
	uint32_t Tools::getTime24h()
	{
		time_t rawTime;

		auto result = std::time(&rawTime);
		if (result != rawTime)
			throw std::runtime_error("WTF - wrong time_t reported from time().");

		return getTime24h(rawTime);
	}

	uint32_t Tools::getTime24h(time_t unixTime)
	{
		struct tm* tmTime = gmtime(&unixTime);

		uint32_t time24h = 0;
		time24h += tmTime->tm_hour * 3600;
		time24h += tmTime->tm_min * 60;
		time24h += tmTime->tm_sec;

		return time24h;
	}

	std::string Tools::get24hFormated(uint32_t time24h)
	{
		char formatedTime[100];

		auto result = snprintf(formatedTime, 100, "%02u:%02u:%02u", time24h / 3600, time24h % 3600 / 60, time24h % 3600 % 60);
		if (result < 8)
			throw std::runtime_error("snprintf() failed for get24hFormated.");

		return std::string(formatedTime);
	}

	std::string Tools::getTimespanAscii(uint32_t uptime)
	{
		std::ostringstream converter;

		uint32_t months = uptime / 2592000;
		uint32_t days = (uptime % 2592000) / 86400;
		uint32_t hours = ((uptime % 2592000) % 86400) / 3600;
		uint32_t minutes = (((uptime % 2592000) % 86400) % 3600) / 60;
		uint32_t seconds = (((uptime % 2592000) % 86400) % 3600) % 60;	
		
		if (months)
			converter << months << " months ";

		if (days)
			converter << days << " days ";
		
		if (hours)
			converter << hours << " hours ";

		if (minutes)
			converter << minutes << " minutes ";

		if (seconds)
			converter << seconds << " seconds ";

		return converter.str();
	}

	namespace diagnostics
	{
		void Print::errnoDescription()
		{
			std::cerr << "Errno (" << errno << ") - " << strerror(errno) << std::endl;
		}
	}
}

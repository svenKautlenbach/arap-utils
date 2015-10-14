#include "ArapUtils.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <climits>
#include <cstdlib>

namespace arap
{
	namespace linuxOS
	{
	#include <fcntl.h>
	#include <poll.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <termios.h>
	
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

		NamedPipe::NamedPipe(const std::string& fifoName) : m_fifoName(fifoName)
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

		// TODO - does NOT work properly.
		bool NamedPipe::dataAvailable()
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

		std::string NamedPipe::getLastMessage()
		{
			openForReading();
			
			char dataBuffer[1024];
			std::string message;
			while (true)
			{
				auto readResult = read(m_fileDescriptor, dataBuffer, 1024);
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

		void NamedPipe::sendMessage(const std::string& message)
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

		NamedPipe::~NamedPipe()
		{
			unlink(m_fifoName.c_str());
		}

		void NamedPipe::openForReading()
		{
			auto result = open(m_fifoName.c_str(), O_RDONLY | O_NONBLOCK);
			if (result == -1)
			{
				diagnostics::Print::errnoDescription();

				throw std::runtime_error("Cannot open reading channel to FIFO - " + m_fifoName + ".");
			}

			m_fileDescriptor = result;
		}

		void NamedPipe::openForWriting()
		{
			auto result = open(m_fifoName.c_str(), O_WRONLY);
			if (result == -1)
			{
				diagnostics::Print::errnoDescription();

				throw std::runtime_error("Cannot open writing channel to FIFO - " + m_fifoName + ".");
			}

			m_fileDescriptor = result;
		}

		void NamedPipe::closeChannel()
		{
			close(m_fileDescriptor);
		}

		SerialPort::SerialPort(const std::string& portName, int baud, int parity, bool doesBlock)
		{
			m_fileDescriptor = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
			if (m_fileDescriptor < 0)
			{
				diagnostics::Print::errnoDescription(); 
				
				throw std::runtime_error("Error opening COM port.");
			}
			
			if (initialize(getTermiosSpeed(baud), parity, doesBlock) == false)
			{
				throw std::runtime_error("Setting COM port parameters failed.");
			}
		}
			
		void SerialPort::sendData(const std::vector<uint8_t>& data)
		{
			writeData(data.data(), data.size());
		}
		
		void SerialPort::sendMessage(const std::string& message)
		{
			writeData(reinterpret_cast<const uint8_t*>(message.c_str()), message.length());
		}
			
		void SerialPort::writeData(const uint8_t* data, size_t length)
		{
			auto writtenData = write(m_fileDescriptor, data, length);

			if (writtenData != static_cast<ssize_t>(length))
			{
				diagnostics::Print::errnoDescription();
				throw std::runtime_error("Could not write all data to serial port.");
			}
		}
		
		bool SerialPort::initialize(int baud, int parity, bool doesBlock)
		{
			struct termios tty;
			memset(&tty, 0, sizeof tty);
			if (tcgetattr(m_fileDescriptor, &tty) != 0)
			{
				diagnostics::Print::errnoDescription("Error getting COM port parameters.");

				return false;
			}

			cfsetospeed(&tty, baud);
			cfsetispeed(&tty, baud);

			tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
			// disable IGNBRK for mismatched speed tests; otherwise receive break
			// as \000 chars
			tty.c_iflag &= ~IGNBRK;		// disable break processing
			tty.c_lflag = 0;		// no signaling chars, no echo,
			// no canonical processing
			tty.c_oflag = 0;		// no remapping, no delays
			tty.c_cc[VMIN]  = 0;		// read doesn't block
			tty.c_cc[VTIME] = 5;		// 0.5 seconds read timeout

			tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

			tty.c_cflag |= (CLOCAL | CREAD); // Ignore modem controls, enable reading.
			tty.c_cflag &= ~(PARENB | PARODD); // Shut off parity.
			tty.c_cflag |= parity;
			tty.c_cflag &= ~CSTOPB;
			tty.c_cflag &= ~CRTSCTS;

			if (tcsetattr(m_fileDescriptor, TCSANOW, &tty) != 0)
			{
				diagnostics::Print::errnoDescription("Error setting COM port data options.");

				return false;
			}

			return true;

			memset(&tty, 0, sizeof tty);
			if (tcgetattr(m_fileDescriptor, &tty) != 0)
			{
				diagnostics::Print::errnoDescription("Error getting COM port settings.");

				return false;
			}

			tty.c_cc[VMIN]  = doesBlock ? 1 : 0;
			tty.c_cc[VTIME] = 5;	// 0.5 seconds read timeout.

			if (tcsetattr(m_fileDescriptor, TCSANOW, &tty) != 0)
			{
				diagnostics::Print::errnoDescription("Error setting COM port block option.");

				return false;
			}

			return true;
		}
			
		int SerialPort::getTermiosSpeed(int baud)
		{
			switch (baud)
			{
			case 0:
				return B0;
			case 50:
				return B50;
			case 75:
				return B75;
			case 110:
				return B110;
			case 9600:
				return B9600;
			default:
				std::cerr << "Unsupported baud rate - " << baud << "." << std::endl;
				throw std::runtime_error("Baud rate not supported.");
			};
		}

		SerialPort::~SerialPort()
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
			
		void Utilities::writeToFile(const std::string& path, const std::string& data, bool overwrite)
		{
			auto options = std::ios_base::out;

			if (!overwrite)
				options |= std::ios_base::app;

			std::ofstream outputFile(path, options);
			if (outputFile.is_open() == false || outputFile.good() == false)
				throw std::runtime_error("Opening " + path + " was not successful.");

			outputFile << data;
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

		std::string Utilities::removeWhiteSpace(std::string& source)
		{
			source.erase(std::remove_if(source.begin(), source.end(),
					std::bind(std::isspace<char>, std::placeholders::_1, std::locale::classic())), source.end());

			return source;
		}
	}
	
	uint32_t Tools::getTime24h(bool gmt)
	{
		time_t rawTime;
		time_t timeToConvert;

		timeToConvert = std::time(&rawTime);
		if (timeToConvert != rawTime)
		{
			throw std::runtime_error("WTF - wrong time_t reported from time().");
		}

		if (!gmt)
		{
			struct tm* timeInfo = localtime(&rawTime);
			timeToConvert = mktime(timeInfo);	
		}

		return getTime24h(timeToConvert);
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
		 
	uint32_t Tools::getTime24h(const std::string& clockFormat)
	{
		auto digits = strings::Utilities::split(clockFormat, ":");

		if (digits.size() != 3)
			throw std::runtime_error(clockFormat + " has wrong format for converting to seconds. Expected hh:mm:ss");

		auto hoursDigits = digits.at(0);
		auto hours = std::stoul(hoursDigits);
		if ((hours == 0 && hoursDigits != "00") || hours > 23)
			throw std::runtime_error(clockFormat + " has wrong format for hour digits.");

		auto minutesDigits = digits.at(1);
		auto minutes = std::stoul(minutesDigits);
		if ((minutes == 0 && minutesDigits != "00") || minutes > 59)
			throw std::runtime_error(clockFormat + " has wrong format for minute digits.");

		auto secondsDigits = digits.at(2);
		auto seconds = std::stoul(secondsDigits);
		if ((seconds == 0 && secondsDigits != "00") || seconds > 59)
			throw std::runtime_error(clockFormat + " has wrong format for second digits.");

		return hours * universe::seconds::per1Hour + minutes * universe::seconds::per1Minute + seconds;
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

	int Tools::generateRandom(uint32_t maxValue)
	{
		static bool initDone = false;

		if (!initDone)
			srand(time(nullptr));
		
		initDone = true;
		auto random = rand();
	
		if (maxValue == 0)
			return random;

		return random % maxValue;
	}

	namespace diagnostics
	{
		void Print::errnoDescription()
		{
			std::cerr << "Errno (" << errno << ") - " << strerror(errno) << std::endl;
		}
			
		void Print::errnoDescription(const std::string& applicationMessage)
		{
			std::cerr << applicationMessage << std::endl;
			errnoDescription();
		}
	}
}

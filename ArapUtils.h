#pragma once

#include <ctime>
#include <fstream>
#include <string>
#include <vector>

#include <sys/types.h>
#include <unistd.h>

namespace arap
{
	namespace universe
	{
		namespace seconds
		{
			const uint32_t per24Hours = 86400;
			const uint32_t per1Hour = 3600;
			const uint32_t per1Minute = 60;
		}
		
		namespace minutes
		{
			const uint32_t per24Hours = 24*60;
			const uint32_t per1Hour = 60;
		}
	}

	namespace linuxOS
	{
		class Utilities
		{
		public:
			static pid_t getPid(const std::string& processEntry); 
		private:
			Utilities(){}
			~Utilities(){}
		};

		class NamedPipe
		{
		public:
			NamedPipe(const std::string& fifoName);

			bool dataAvailable();
			std::string getLastMessage();
			std::vector<uint8_t> getLastInput();

			void sendMessage(const std::string& message);

			~NamedPipe();
		private:
			int m_fileDescriptor;
			std::string m_fifoName;

			void openForReading();
			void openForWriting();
			void closeChannel();
		};

		class SerialPort
		{
		public:
			SerialPort(const std::string& portName, int baud, int parity, bool doesBlock = false);

			void sendData(const std::vector<uint8_t>& data);
			void sendMessage(const std::string& message);

			~SerialPort();
		private:
			void writeData(const uint8_t* data, size_t length);
			bool initialize(int baud, int parity, bool doesBlock);
			
			static int getTermiosSpeed(int baud);

			int m_fileDescriptor;
		};
	}

	namespace strings
	{
		class Utilities
		{
		public:
			static std::vector<std::string> getLines(FILE* fileHandle);		
			static std::vector<std::string> getLines(std::ifstream& fileStream);		
			static std::vector<std::string> getLines(const std::string& filePath);

			static void writeToFile(const std::string& path, const std::string& data, bool overwrite = true);
			
			static std::vector<std::string> split(const std::string& source, const std::string& delimiter);
			static std::string removeWhiteSpace(std::string& source);
		private:
			Utilities(){}
			~Utilities(){}
		};
	}

	class Tools
	{
	public:
		static uint32_t getTime24h(bool gmt = true);
		static uint32_t getTime24h(time_t unixTime);
		static uint32_t getTime24h(const std::string& clockFormat);
		static std::string get24hFormated(uint32_t time24h);
		static std::string getTimespanAscii(uint32_t uptime);
		static std::string getTimeAsc()
		{
			auto timeT = std::time(nullptr);
			return std::asctime(std::localtime(&timeT));
		}
			
		static std::string getErrnoDescription();

		static int generateRandom(uint32_t maxValue = 0);
	private:
		Tools(){}
		~Tools(){}
	};

	namespace diagnostics
	{
		class Print
		{
		public:
			static void errnoDescription();
			static void errnoDescription(const std::string& applicationMessage);
		private:
			Print(){}
			~Print(){}
		};
	}

	namespace network
	{
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
		
		class Http
		{
		public:
			static std::string get(const std::string& ip, const std::string& query="");
			static void put(const std::string& ip, const std::string& what);
		private:
			Http(){}
			~Http(){}
		};

		class UdpSender
		{
		public:
			UdpSender(const std::string& ip, uint16_t port);
			
			~UdpSender();
			
			void sendData(const std::vector<uint8_t>& packet);
		private:
			std::string m_ip;
			uint16_t m_port;
			struct sockaddr_in6 m_ip6SockAddr;
			int m_socketDescriptor;

			void connectSocket(); 
		};

		class UdpListener
		{
		public:
			UdpListener(const std::string& ip, uint16_t port);
			UdpListener();

			~UdpListener();

			bool dataAvailable();
			std::vector<uint8_t> getData();
			// In order to get the sender, first the getData must be called!!!
			std::string getSender();
		private:
			std::string m_ip;
			std::string m_senderIp;
			uint16_t m_port;
			struct sockaddr_in6 m_ip6SockAddr;
			int m_socketDescriptor;

			void bindSocket();
		};

		class Ipv6MacConvert
		{
		public:
			static std::string getIpv6(const std::string& prefix, const std::string& eui64);
			static std::string getEui64(const std::string& ipv6);
			static std::vector<uint8_t> getInterfaceAddress(const std::string& ipv6);
			static std::string interfaceToIpv6(const std::string& prefix, std::vector<uint8_t> interface);

		private:
			Ipv6MacConvert(){}
			~Ipv6MacConvert(){}
			
			static std::vector<uint8_t> eui64ToBytes(const std::string& eui64);
		};
	}
}

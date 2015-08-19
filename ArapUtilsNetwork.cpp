#include "ArapUtils.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>

#include <sys/select.h>

namespace arap
{
	namespace network
	{
		std::string Http::get(const std::string& ip, const std::string& what)
		{
			struct sockaddr_in6 ip6Address;
			int socketDescriptor;

			if (!inet_pton(AF_INET6, ip.c_str(), &(ip6Address.sin6_addr)))
			{
				std::cerr << "inet_pton() to address " << ip << " failed." << std::endl;

				throw std::exception();
			}

			socketDescriptor = socket(AF_INET6, SOCK_STREAM, 0);
			if (socketDescriptor < 0)
			{
				std::cerr << "socket() error for " << ip << "." << std::endl;

				throw std::exception();
			}

			ip6Address.sin6_family = AF_INET6;
			ip6Address.sin6_port = htons(80);

			if (connect(socketDescriptor, reinterpret_cast<struct sockaddr*>(&ip6Address), sizeof(ip6Address)) < 0) 
			{
				std::cerr << "connect() for " << ip << " failed." << std::endl;

				close(socketDescriptor);

				throw std::exception();
			}

			if (write(socketDescriptor, what.c_str(), what.size()) < 0)
			{
				std::cerr << "write() for " << ip << " failed." << std::endl;

				close(socketDescriptor);

				throw std::exception();
			}

			fd_set fdSet;
			FD_ZERO(&fdSet);
			FD_SET(socketDescriptor, &fdSet); 
			struct timeval selectTimeout = {5, 0};
			size_t responseBufferSize = 4096;
			size_t responseLength = 0;
			uint8_t responseBuffer[responseBufferSize];
			while (1)
			{
				auto selectResult = select(socketDescriptor + 1, &fdSet, nullptr, nullptr, &selectTimeout);
				if (selectResult == 0)
				{
					std::cerr << "Timeout occured for the request - " << what << " - to: " << ip << std::endl;

					break;
				}

				if (selectResult == -1)
				{
					if (errno == EINTR)
					{
						continue;
					}

					arap::diagnostics::Print::errnoDescription();

					FD_CLR(socketDescriptor, &fdSet);
					close(socketDescriptor);

					throw std::runtime_error("select() error for the request - " + what + " - to: " + ip);
				}

				auto receiveResult = recv(socketDescriptor, responseBuffer + responseLength, responseBufferSize - responseLength, 0);

				if (receiveResult < 0)
				{
					std::cerr << "Error occured when recv()-ing network data." << std::endl;
					arap::diagnostics::Print::errnoDescription();

					FD_CLR(socketDescriptor, &fdSet);
					close(socketDescriptor);
				}

				if (receiveResult == 0)
				{
					break;
				}

				responseLength += receiveResult;	
			}

			FD_CLR(socketDescriptor, &fdSet);
			close(socketDescriptor);

			return std::string(reinterpret_cast<char*>(responseBuffer), responseLength);
		}

		void Http::put(const std::string& ip, const std::string& what)
		{
			(void)what;
		}

		UdpSender::UdpSender(const std::string& ip, uint16_t port) : m_ip(ip), m_port(port)
		{
			connectSocket();
		}

		void UdpSender::sendData(const std::vector<uint8_t>& packet)
		{
			//std::cout << "Sending packet of length " << packet.size() << " to " << m_ip << " on " << m_port << std::endl;

			auto bytesSent = sendto(m_socketDescriptor, packet.data(), packet.size(), 0, 
				reinterpret_cast<struct sockaddr*>(&m_ip6SockAddr), sizeof(m_ip6SockAddr));

			if (bytesSent < 0)
			{
				arap::diagnostics::Print::errnoDescription();
				throw std::runtime_error("sendto() failed for address " + m_ip);
			}

			if (static_cast<size_t>(bytesSent) != packet.size())
			{
				std::cerr << "Not all bytes got sent to " << m_ip << " " << bytesSent << "/" << packet.size()
					<< " sent." << std::endl;
			}
		}

		UdpSender::~UdpSender()
		{
			close(m_socketDescriptor);
		}

		void UdpSender::connectSocket()
		{
			if (inet_pton(AF_INET6, m_ip.c_str(), &(m_ip6SockAddr.sin6_addr)) != 1)
			{
				std::cerr << "inet_pton() to address " << m_ip << " failed." << std::endl;
				arap::diagnostics::Print::errnoDescription();

				throw std::exception();
			}
			
			m_socketDescriptor = socket(AF_INET6, SOCK_DGRAM, 0);

			m_ip6SockAddr.sin6_family = AF_INET6;
			m_ip6SockAddr.sin6_port = htons(m_port);
		}

		UdpListener::UdpListener() : UdpListener("::1", 4)
		{
		}
		
		UdpListener::UdpListener(const std::string& ip, uint16_t port) : m_ip(ip), m_port(port)
		{
			bindSocket();
		}

		UdpListener::~UdpListener()
		{
			close(m_socketDescriptor);
		}

		bool UdpListener::dataAvailable()
		{
			fd_set fdSet;
			FD_ZERO(&fdSet);
			FD_SET(m_socketDescriptor, &fdSet);
			struct timeval selectTimeout = {0, 0};
		
			auto selectResult = select(m_socketDescriptor + 1, &fdSet, nullptr, nullptr, &selectTimeout);

			if (selectResult == 0)
			{
				return false;
			}

			if (selectResult == -1)
			{
				if (errno == EINTR)
				{
					return false;
				}

				arap::diagnostics::Print::errnoDescription();
				FD_CLR(m_socketDescriptor, &fdSet);

				throw std::runtime_error("select() error for UdpListener listening " + m_ip + ".");
			}

			return true;
		}

		std::vector<uint8_t> UdpListener::getData()
		{
			std::vector<uint8_t> receivedData;
			uint8_t dataBuffer[100];

			struct sockaddr_in6 clientSockAddr;
			auto clientSockAddrLength = static_cast<socklen_t>(sizeof(clientSockAddr));
			auto receivedBytes = recvfrom(m_socketDescriptor, dataBuffer, 100, 0, 
					reinterpret_cast<struct sockaddr*>(&clientSockAddr), &clientSockAddrLength);

			if (receivedBytes < 0)
			{
				std::cerr << "recvfrom() resulted in error." << std::endl;
				arap::diagnostics::Print::errnoDescription();

				return receivedData;
			}

			for (uint32_t i = 0; i < static_cast<uint32_t>(receivedBytes); (void)0)
			{
				receivedData.push_back(dataBuffer[i++]);
			}

			char ipStringBuffer[100];
			auto ipString = inet_ntop(clientSockAddr.sin6_family, &(clientSockAddr.sin6_addr.s6_addr), ipStringBuffer, 100);
			
			if (ipString == NULL)
			{
				std::cerr << "Error converting last sender address." << std::endl;
				arap::diagnostics::Print::errnoDescription();

				m_senderIp = std::string();
			}
			else
			{
				m_senderIp = std::string(ipStringBuffer);
			}

			return receivedData;
		}

		std::string UdpListener::getSender()
		{
			return m_senderIp;
		}
			
		void UdpListener::bindSocket()
		{
			if (inet_pton(AF_INET6, m_ip.c_str(), &(m_ip6SockAddr.sin6_addr)) != 1)
			{
				std::cerr << "inet_pton() to address " << m_ip << " failed." << std::endl;
				arap::diagnostics::Print::errnoDescription();

				throw std::exception();
			}

			m_socketDescriptor = socket(AF_INET6, SOCK_DGRAM, 0);
			
			m_ip6SockAddr.sin6_family = AF_INET6;
			m_ip6SockAddr.sin6_port = htons(m_port);

			if (bind(m_socketDescriptor, reinterpret_cast<struct sockaddr*>(&m_ip6SockAddr), sizeof(m_ip6SockAddr)) < 0)
			{
				std::cerr << "bind() failed for " << m_ip << "." << std::endl;
				arap::diagnostics::Print::errnoDescription();

				throw std::exception();
			}
			
		}
			
		std::string Ipv6MacConvert::getIpv6(const std::string& prefix, const std::string& eui64)
		{
			if (prefix.size() != 4)
				throw std::runtime_error("Prefix not the right size - currently 4 character prefixes are supported only. Got - " + prefix);

			uint32_t prefixInt = stoul(prefix, nullptr, 16);
			if (prefixInt == 0)
				throw std::runtime_error("Prefix(" + prefix + ") could not be converted to correct prefix bytes.");

			auto eui64Bytes = eui64ToBytes(eui64);
			if (eui64Bytes.size() != 8)
				throw std::runtime_error("Input data for EUI-64(" + eui64 + ") is not valid.");

			uint8_t ipBuffer[16] = {};
			uint8_t prefix1 = ((prefixInt & 0xFF00) >> 8);
			uint8_t prefix2 = prefixInt & 0xFF; 
			memcpy(ipBuffer, &prefix1, 1);
			memcpy(ipBuffer + 1, &prefix2, 1);
			memcpy(ipBuffer + 8, eui64Bytes.data(), 8);

			uint8_t invertBitmask = ((ipBuffer[8] & 0x02) > 0x00 ? 0xFD : 0x02);
			if (invertBitmask > 0x02)
				ipBuffer[8] &= invertBitmask;
			else
				ipBuffer[8] |= invertBitmask;

			char ipString[100];
			auto ntopResult = inet_ntop(AF_INET6, ipBuffer, ipString, 100);

			if (ntopResult != ipString)
				throw std::runtime_error("Error inet_ntop().");

			return std::string(ipString);
		}
		
		std::string Ipv6MacConvert::getEui64(const std::string& ipv6)
		{
			uint8_t ipv6Bytes[16];
			
			if (inet_pton(AF_INET6, ipv6.c_str(), ipv6Bytes) != 1)
			{
				throw std::runtime_error("inet_pton() to address " + ipv6 + " failed.");
			}

			uint8_t invertBitmask = ((ipv6Bytes[8] & 0x02) > 0x00 ? 0xFD : 0x02);
			if (invertBitmask > 0x02)
			{
				ipv6Bytes[8] &= invertBitmask;
			}
			else
			{
				ipv6Bytes[8] |= invertBitmask;
			}

			char eui64String[23];
			size_t printedCharacters = 0;
			for (uint32_t i = 8; i < 16; i++)
			{
				printedCharacters += snprintf(eui64String + printedCharacters, 23 - printedCharacters + 1,"%02X", ipv6Bytes[i]);
				if (i < 16)
				{
					printedCharacters += snprintf(eui64String + printedCharacters, 23 - printedCharacters, ":");
				}
			}

			return std::string(eui64String);	
		}

		std::vector<uint8_t> Ipv6MacConvert::eui64ToBytes(const std::string& eui64)
		{
			std::vector<uint8_t> eui64Bytes;

			// Format XX:XX:XX:XX:XX:XX:XX:XX
			if (eui64.size() != 23)
			{
				throw std::runtime_error("Wrong format for EUI-64 - " + eui64 + ".");
			}

			for (uint32_t i = 0; i < 8; i++)
			{
				auto byteString = eui64.substr(i * 2 + i, 2);
				auto result = stoul(byteString, nullptr, 16);

				if (result == 0 && byteString != "00")
				{
					throw std::runtime_error("stoul() conversion to EUI-64 bytes resulted in error.");
				}
				else
				{
					eui64Bytes.push_back(result);
				}
			}

			if (eui64Bytes.size() != 8)
			{
				throw std::runtime_error("The EUI-64 string(" + eui64 + ") could not be converted. Wrong format?");
			}

			return eui64Bytes;
		}

		std::vector<uint8_t> Ipv6MacConvert::getInterfaceAddress(const std::string& ipv6)
		{
			uint8_t ipv6Bytes[16];
			if (inet_pton(AF_INET6, ipv6.c_str(), ipv6Bytes) != 1)
			{
				throw std::runtime_error("inet_pton() to address " + ipv6 + " failed.");
			}

			std::vector<uint8_t> interfaceBytes;
			for (uint32_t i = 8; i < 16; i++)
			{
				interfaceBytes.push_back(ipv6Bytes[i]);
			}

			return interfaceBytes;
		}
			
		std::string Ipv6MacConvert::interfaceToIpv6(const std::string& prefix, std::vector<uint8_t> interface)
		{
			assert(interface.size() == 8);

			if (prefix.size() != 4)
				throw std::runtime_error("Prefix not the right size - currently 4 character prefixes are supported only. Got - " + prefix);
			
			uint32_t prefixInt = stoul(prefix, nullptr, 16);
			if (prefixInt == 0)
				throw std::runtime_error("Prefix(" + prefix + ") could not be converted to correct prefix bytes.");
			
			uint8_t ipBuffer[16] = {};
			uint8_t prefix1 = ((prefixInt & 0xFF00) >> 8);
			uint8_t prefix2 = prefixInt & 0xFF; 
			memcpy(ipBuffer, &prefix1, 1);
			memcpy(ipBuffer + 1, &prefix2, 1);
			memcpy(ipBuffer + 8, interface.data(), 8);
			
			char ipString[100];
			auto ntopResult = inet_ntop(AF_INET6, ipBuffer, ipString, 100);

			if (ntopResult != ipString)
				throw std::runtime_error("Error inet_ntop().");

			return std::string(ipString);
		}
	}
}

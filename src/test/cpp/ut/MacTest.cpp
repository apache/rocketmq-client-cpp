/*
** showip.c -- show IP addresses for a host given on the command line
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ifaddrs.h>
#include <iostream>

#if defined(__APPLE__)
#include <net/if_dl.h>
#define AF_FAMILY AF_LINK
#elif defined(__linux__)
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#define AF_FAMILY AF_PACKET
#endif

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

#include <vector>

bool macAddress(std::vector<unsigned char> &mac) {
  struct ifaddrs *ifap = nullptr, *ifaptr = nullptr;
  if (getifaddrs(&ifap)) {
    return false;
  }

  for (ifaptr = ifap; ifaptr; ifaptr = ifaptr->ifa_next) {
    if (!ifaptr->ifa_addr) {
      continue;
    }

    if (ifaptr->ifa_addr->sa_family == AF_FAMILY) {
#if defined(__APPLE__)
      unsigned char* ptr =
          reinterpret_cast<unsigned char*>(LLADDR(reinterpret_cast<struct sockaddr_dl*>(ifaptr->ifa_addr)));
#elif defined(__linux__)
      unsigned char* ptr = reinterpret_cast<unsigned char*>(reinterpret_cast<struct sockaddr_ll*>(ifaptr->ifa_addr)->sll_addr);
#endif
      bool all_zero = true;
      for (int i = 0; i < 6; i++) {
        if (*(ptr + i) != 0) {
          all_zero = false;
          break;
        }
      }
      if (all_zero) {
        std::cout << "Skip MAC address of Network Interface: " << ifaptr->ifa_name << std::endl;
        continue;
      }
      std::cout << "Use MAC address of Network Interface: " << ifaptr->ifa_name << std::endl;
      // MAC address has 48 bits
      mac.resize(6);
      memcpy(mac.data(), ptr, 6);
      //   break;
    } else if (ifaptr->ifa_addr->sa_family == AF_INET) {
      char buffer[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in*>(ifaptr->ifa_addr)->sin_addr, buffer, INET_ADDRSTRLEN);
      std::cout << "Inet: " << ifaptr->ifa_name << ": " << buffer << std::endl;
    }
  }
  freeifaddrs(ifap);
  return true;
}

int main(int argc, char* argv[]) {
  std::vector<unsigned char> data;
  if (macAddress(data)) {
    for (auto& item : data) {
      std::printf("%02X:", item);
    }
    std::cout << std::endl;
  }

  return 0;
}
//
// Created by DiaLight on 1/10/2026.
//

#ifndef FLAME_NET_LOCALSERVICE_R_H
#define FLAME_NET_LOCALSERVICE_R_H

#include <net/net_LocalService.h>

namespace net {

  #pragma pack(push, 1)
  struct MyAddr {
    wchar_t *f0_pAddr;
    size_t f4_size;
    uint16_t f8_port;
  };
  #pragma pack(pop)
  static_assert(sizeof(MyAddr) == 0xA);

  #pragma pack(push, 1)
  struct MyLocalServiceAddr {
    char f0_signature[2];
    GUID f2_guid;
    MyAddr f12_addr;
    BYTE gap_1C[6];
    wchar_t f22_addr[];
  };
  #pragma pack(pop)
  static_assert(sizeof(MyLocalServiceAddr) == 0x22);

  #pragma pack(push, 1)
  struct MyLocalService {
    GUID f0_guid;
    DWORD f10_count;
    size_t f14_addr_size;
    wchar_t *f18_pName;
    MyLocalService *f1C_next;
    MyLocalServiceAddr *f20_addr;
    GUID *f24_pGuid;
    wchar_t f28_name[];
  };
  #pragma pack(pop)
  static_assert(sizeof(MyLocalService) == 0x28);
  static_assert(sizeof(MyLocalService) == sizeof(dk2::replaced_net_LocalService));

}

#endif // FLAME_NET_LOCALSERVICE_R_H

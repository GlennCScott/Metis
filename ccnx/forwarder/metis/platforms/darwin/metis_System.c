/*
 * Copyright (c) 2013-2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Patent rights are not granted under this agreement. Patent rights are
 *       available under FRAND terms.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XEROX or PARC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @author Marc Mosko, Palo Alto Research Center (Xerox PARC)
 * @copyright 2013-2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#include <LongBow/runtime.h>

#include <ccnx/api/control/cpi_InterfaceSet.h>

#include <ccnx/forwarder/metis/core/metis_System.h>
#include <ccnx/forwarder/metis/core/metis_Forwarder.h>

CPIInterfaceSet *
metisSystem_Interfaces(MetisForwarder *metis)
{
    CPIInterfaceSet *set = cpiInterfaceSet_Create();

    // this is the dynamically allocated head of the list
    struct ifaddrs *ifaddr;
    int failure = getifaddrs(&ifaddr);
    assertFalse(failure, "Error getifaddrs: (%d) %s", errno, strerror(errno));

    struct ifaddrs *next;
    for (next = ifaddr; next != NULL; next = next->ifa_next) {
        if ((next->ifa_addr == NULL) || ((next->ifa_flags & IFF_UP) == 0)) {
            continue;
        }

        // This assumes the LINK address comes first so we can get the MTU
        // when the interface is created.

        CPIInterface *iface = cpiInterfaceSet_GetByName(set, next->ifa_name);
        if (iface == NULL) {
            unsigned mtu = 0;

            if (next->ifa_data != NULL) {
                struct if_data *ifdata = (struct if_data *) next->ifa_data;
                mtu = ifdata->ifi_mtu;
            }

            iface = cpiInterface_Create(next->ifa_name,
                                        metisForwarder_GetNextConnectionId(metis),
                                        next->ifa_flags & IFF_LOOPBACK,
                                        next->ifa_flags & IFF_MULTICAST,
                                        mtu);

            cpiInterfaceSet_Add(set, iface);
        }

        int family = next->ifa_addr->sa_family;
        switch (family) {
            case AF_INET: {
                CPIAddress *address = cpiAddress_CreateFromInet((struct sockaddr_in *) next->ifa_addr);
                cpiInterface_AddAddress(iface, address);
                break;
            }

            case AF_INET6: {
                CPIAddress *address = cpiAddress_CreateFromInet6((struct sockaddr_in6 *) next->ifa_addr);
                cpiInterface_AddAddress(iface, address);
                break;
            }

            case AF_LINK: {
                struct sockaddr_dl *addr_dl = (struct sockaddr_dl *) next->ifa_addr;

                // skip links with 0-length address
                if (addr_dl->sdl_alen > 0) {
                    // addr_dl->sdl_data[12] contains the interface name followed by the MAC address, so
                    // need to offset in to the array past the interface name.
                    CPIAddress *address = cpiAddress_CreateFromLink((uint8_t *) &addr_dl->sdl_data[ addr_dl->sdl_nlen], addr_dl->sdl_alen);
                    cpiInterface_AddAddress(iface, address);
                }
                break;
            }
        }
    }

    freeifaddrs(ifaddr);

    return set;
}

CPIAddress *
metisSystem_GetMacAddressByName(MetisForwarder *metis, const char *interfaceName)
{
    CPIAddress *linkAddress = NULL;

    CPIInterfaceSet *interfaceSet = metisSystem_Interfaces(metis);
    CPIInterface *interface = cpiInterfaceSet_GetByName(interfaceSet, interfaceName);

    if (interface) {
        const CPIAddressList *addressList = cpiInterface_GetAddresses(interface);

        size_t length = cpiAddressList_Length(addressList);
        for (size_t i = 0; i < length && !linkAddress; i++) {
            const CPIAddress *a = cpiAddressList_GetItem(addressList, i);
            if (cpiAddress_GetType(a) == cpiAddressType_LINK) {
                linkAddress = cpiAddress_Copy(a);
            }
        }
    }

    cpiInterfaceSet_Destroy(&interfaceSet);

    return linkAddress;
}

unsigned
metisSystem_InterfaceMtu(MetisForwarder *metis, const char *interfaceName)
{
    unsigned mtu = 0;

    if (interfaceName) {
        CPIInterfaceSet *interfaceSet = metisSystem_Interfaces(metis);
        CPIInterface *interface = cpiInterfaceSet_GetByName(interfaceSet, interfaceName);

        if (interface) {
            mtu = cpiInterface_GetMTU(interface);
        }

        cpiInterfaceSet_Destroy(&interfaceSet);
    }
    return mtu;
}


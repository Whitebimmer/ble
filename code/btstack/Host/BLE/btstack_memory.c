/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */



/*
 *  btstsack_memory.h
 *
 *  @brief BTstack memory management via configurable memory pools
 *
 *  @note code semi-atuomatically generated by tools/btstack_memory_generator.py
 *
 */

#include "ble/btstack_memory.h"
#include <memory_pool.h>

#include "ble/btstack-config.h"
#include "hci.h"
#include "l2cap.h"


// MARK: hci_connection_t
#ifdef MAX_NO_HCI_CONNECTIONS
#if MAX_NO_HCI_CONNECTIONS > 0
static hci_connection_t hci_connection_storage[MAX_NO_HCI_CONNECTIONS] SEC(.btmem_highly_available);
static memory_pool_t hci_connection_pool SEC(.btmem_highly_available);
hci_connection_t * le_btstack_memory_hci_connection_get(void){
    return (hci_connection_t *) memory_pool_get(&hci_connection_pool);
}
void le_btstack_memory_hci_connection_free(hci_connection_t *hci_connection){
    memory_pool_free(&hci_connection_pool, hci_connection);
}
#else
hci_connection_t * btstack_memory_hci_connection_get(void){
    return NULL;
}
void btstack_memory_hci_connection_free(hci_connection_t *hci_connection){
    // silence compiler warning about unused parameter in a portable way
    (void) hci_connection;
};
#endif
#elif defined(HAVE_MALLOC)
hci_connection_t * btstack_memory_hci_connection_get(void){
    return (hci_connection_t*) malloc(sizeof(hci_connection_t));
}
void btstack_memory_hci_connection_free(hci_connection_t *hci_connection){
    free(hci_connection);
}
#else
#error "Neither HAVE_MALLOC nor MAX_NO_HCI_CONNECTIONS for struct hci_connection is defined. Please, edit the config file."
#endif



// MARK: l2cap_service_t
#ifdef MAX_NO_L2CAP_SERVICES
#if MAX_NO_L2CAP_SERVICES > 0
static l2cap_service_t l2cap_service_storage[MAX_NO_L2CAP_SERVICES] SEC(.btmem_highly_available);
static memory_pool_t l2cap_service_pool SEC(.btmem_highly_available);
l2cap_service_t * le_btstack_memory_l2cap_service_get(void){
    return (l2cap_service_t *) memory_pool_get(&l2cap_service_pool);
}
void le_btstack_memory_l2cap_service_free(l2cap_service_t *l2cap_service){
    memory_pool_free(&l2cap_service_pool, l2cap_service);
}
#else
l2cap_service_t * le_btstack_memory_l2cap_service_get(void){
    return NULL;
}
void le_btstack_memory_l2cap_service_free(l2cap_service_t *l2cap_service){
    // silence compiler warning about unused parameter in a portable way
    (void) l2cap_service;
};
#endif
#elif defined(HAVE_MALLOC)
l2cap_service_t * btstack_memory_l2cap_service_get(void){
    return (l2cap_service_t*) malloc(sizeof(l2cap_service_t));
}
void btstack_memory_l2cap_service_free(l2cap_service_t *l2cap_service){
    free(l2cap_service);
}
#else
#error "Neither HAVE_MALLOC nor MAX_NO_L2CAP_SERVICES for struct l2cap_service is defined. Please, edit the config file."
#endif


// MARK: l2cap_channel_t
#ifdef MAX_NO_L2CAP_CHANNELS
#if MAX_NO_L2CAP_CHANNELS > 0
static l2cap_channel_t l2cap_channel_storage[MAX_NO_L2CAP_CHANNELS] SEC(.btmem_highly_available);
static memory_pool_t l2cap_channel_pool SEC(.btmem_highly_available);
l2cap_channel_t * le_btstack_memory_l2cap_channel_get(void){
    return (l2cap_channel_t *) memory_pool_get(&l2cap_channel_pool);
}
void le_btstack_memory_l2cap_channel_free(l2cap_channel_t *l2cap_channel){
    memory_pool_free(&l2cap_channel_pool, l2cap_channel);
}
#else
l2cap_channel_t * le_btstack_memory_l2cap_channel_get(void){
    return NULL;
}
void le_btstack_memory_l2cap_channel_free(l2cap_channel_t *l2cap_channel){
    // silence compiler warning about unused parameter in a portable way
    (void) l2cap_channel;
};
#endif
#elif defined(HAVE_MALLOC)
l2cap_channel_t * btstack_memory_l2cap_channel_get(void){
    return (l2cap_channel_t*) malloc(sizeof(l2cap_channel_t));
}
void btstack_memory_l2cap_channel_free(l2cap_channel_t *l2cap_channel){
    free(l2cap_channel);
}
#else
#error "Neither HAVE_MALLOC nor MAX_NO_L2CAP_CHANNELS for struct l2cap_channel is defined. Please, edit the config file."
#endif




// MARK: gatt_client_t
#ifdef MAX_NO_GATT_CLIENTS
#if MAX_NO_GATT_CLIENTS > 0
static gatt_client_t gatt_client_storage[MAX_NO_GATT_CLIENTS] SEC(.btmem_highly_available);
static memory_pool_t gatt_client_pool SEC(.btmem_highly_available);
gatt_client_t * btstack_memory_gatt_client_get(void){
    return (gatt_client_t *) memory_pool_get(&gatt_client_pool);
}
void btstack_memory_gatt_client_free(gatt_client_t *gatt_client){
    memory_pool_free(&gatt_client_pool, gatt_client);
}
#else
gatt_client_t * btstack_memory_gatt_client_get(void){
    return NULL;
}
void btstack_memory_gatt_client_free(gatt_client_t *gatt_client){
    // silence compiler warning about unused parameter in a portable way
    (void) gatt_client;
};
#endif
#elif defined(HAVE_MALLOC)
gatt_client_t * btstack_memory_gatt_client_get(void){
    return (gatt_client_t*) malloc(sizeof(gatt_client_t));
}
void btstack_memory_gatt_client_free(gatt_client_t *gatt_client){
    free(gatt_client);
}
#else
#error "Neither HAVE_MALLOC nor MAX_NO_GATT_CLIENTS for struct gatt_client is defined. Please, edit the config file."
#endif


// MARK: gatt_subclient_t
#ifdef MAX_NO_GATT_SUBCLIENTS
#if MAX_NO_GATT_SUBCLIENTS > 0
static gatt_subclient_t gatt_subclient_storage[MAX_NO_GATT_SUBCLIENTS] SEC(.btmem_highly_available);
static memory_pool_t gatt_subclient_pool SEC(.btmem_highly_available);
gatt_subclient_t * btstack_memory_gatt_subclient_get(void){
    return (gatt_subclient_t *) memory_pool_get(&gatt_subclient_pool);
}
void btstack_memory_gatt_subclient_free(gatt_subclient_t *gatt_subclient){
    memory_pool_free(&gatt_subclient_pool, gatt_subclient);
}
#else
gatt_subclient_t * btstack_memory_gatt_subclient_get(void){
    return NULL;
}
void btstack_memory_gatt_subclient_free(gatt_subclient_t *gatt_subclient){
    // silence compiler warning about unused parameter in a portable way
    (void) gatt_subclient;
};
#endif
#elif defined(HAVE_MALLOC)
gatt_subclient_t * btstack_memory_gatt_subclient_get(void){
    return (gatt_subclient_t*) malloc(sizeof(gatt_subclient_t));
}
void btstack_memory_gatt_subclient_free(gatt_subclient_t *gatt_subclient){
    free(gatt_subclient);
}
#else
#error "Neither HAVE_MALLOC nor MAX_NO_GATT_SUBCLIENTS for struct gatt_subclient is defined. Please, edit the config file."
#endif


// MARK: whitelist_entry_t
#ifdef MAX_NO_WHITELIST_ENTRIES
#if MAX_NO_WHITELIST_ENTRIES > 0
static whitelist_entry_t whitelist_entry_storage[MAX_NO_WHITELIST_ENTRIES];
static memory_pool_t whitelist_entry_pool;
whitelist_entry_t * btstack_memory_whitelist_entry_get(void){
    return (whitelist_entry_t *) memory_pool_get(&whitelist_entry_pool);
}
void btstack_memory_whitelist_entry_free(whitelist_entry_t *whitelist_entry){
    memory_pool_free(&whitelist_entry_pool, whitelist_entry);
}
#else
whitelist_entry_t * btstack_memory_whitelist_entry_get(void){
    return NULL;
}
void btstack_memory_whitelist_entry_free(whitelist_entry_t *whitelist_entry){
    // silence compiler warning about unused parameter in a portable way
    (void) whitelist_entry;
};
#endif
#elif defined(HAVE_MALLOC)
whitelist_entry_t * btstack_memory_whitelist_entry_get(void){
    return (whitelist_entry_t*) malloc(sizeof(whitelist_entry_t));
}
void btstack_memory_whitelist_entry_free(whitelist_entry_t *whitelist_entry){
    free(whitelist_entry);
}
#else
#error "Neither HAVE_MALLOC nor MAX_NO_WHITELIST_ENTRIES for struct whitelist_entry is defined. Please, edit the config file."
#endif


// MARK: sm_lookup_entry_t
#ifdef MAX_NO_SM_LOOKUP_ENTRIES
#if MAX_NO_SM_LOOKUP_ENTRIES > 0
static sm_lookup_entry_t sm_lookup_entry_storage[MAX_NO_SM_LOOKUP_ENTRIES];
static memory_pool_t sm_lookup_entry_pool;
sm_lookup_entry_t * btstack_memory_sm_lookup_entry_get(void){
    return (sm_lookup_entry_t *) memory_pool_get(&sm_lookup_entry_pool);
}
void btstack_memory_sm_lookup_entry_free(sm_lookup_entry_t *sm_lookup_entry){
    memory_pool_free(&sm_lookup_entry_pool, sm_lookup_entry);
}
#else
sm_lookup_entry_t * btstack_memory_sm_lookup_entry_get(void){
    return NULL;
}
void btstack_memory_sm_lookup_entry_free(sm_lookup_entry_t *sm_lookup_entry){
    // silence compiler warning about unused parameter in a portable way
    (void) sm_lookup_entry;
};
#endif
#elif defined(HAVE_MALLOC)
sm_lookup_entry_t * btstack_memory_sm_lookup_entry_get(void){
    return (sm_lookup_entry_t*) malloc(sizeof(sm_lookup_entry_t));
}
void btstack_memory_sm_lookup_entry_free(sm_lookup_entry_t *sm_lookup_entry){
    free(sm_lookup_entry);
}
#else
#error "Neither HAVE_MALLOC nor MAX_NO_SM_LOOKUP_ENTRIES for struct sm_lookup_entry is defined. Please, edit the config file."
#endif


// init
void le_btstack_memory_init(void){
#if MAX_NO_HCI_CONNECTIONS > 0
    memory_pool_create(&hci_connection_pool, hci_connection_storage, MAX_NO_HCI_CONNECTIONS, sizeof(hci_connection_t));
#endif
#if MAX_NO_L2CAP_SERVICES > 0
    memory_pool_create(&l2cap_service_pool, l2cap_service_storage, MAX_NO_L2CAP_SERVICES, sizeof(l2cap_service_t));
#endif
#if MAX_NO_L2CAP_CHANNELS > 0
    memory_pool_create(&l2cap_channel_pool, l2cap_channel_storage, MAX_NO_L2CAP_CHANNELS, sizeof(l2cap_channel_t));
#endif
#if MAX_NO_GATT_CLIENTS > 0
    memory_pool_create(&gatt_client_pool, gatt_client_storage, MAX_NO_GATT_CLIENTS, sizeof(gatt_client_t));
#endif
#if MAX_NO_GATT_SUBCLIENTS > 0
    memory_pool_create(&gatt_subclient_pool, gatt_subclient_storage, MAX_NO_GATT_SUBCLIENTS, sizeof(gatt_subclient_t));
#endif
#if MAX_NO_WHITELIST_ENTRIES > 0
    memory_pool_create(&whitelist_entry_pool, whitelist_entry_storage, MAX_NO_WHITELIST_ENTRIES, sizeof(whitelist_entry_t));
#endif
#if MAX_NO_SM_LOOKUP_ENTRIES > 0
    memory_pool_create(&sm_lookup_entry_pool, sm_lookup_entry_storage, MAX_NO_SM_LOOKUP_ENTRIES, sizeof(sm_lookup_entry_t));
#endif
}

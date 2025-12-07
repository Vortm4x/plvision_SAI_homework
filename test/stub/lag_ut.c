#include <stdlib.h>
#include <stdio.h>
#include "sai.h"
#include "assert.h"

#define TEST_MAX_PORT_COUNT 64
#define TEST_LAG_COUNT 5
#define TEST_LAG_MEMBER_COUNT 32
#define TEST_LAG_PORTS_PER_LAG (TEST_LAG_MEMBER_COUNT / TEST_LAG_COUNT)
#define TEST_LAG_MEMBER_ATTR_COUNT 2


const char* test_profile_get_value(
    _In_ sai_switch_profile_id_t profile_id,
    _In_ const char* variable)
{
    return 0;
}

int test_profile_get_next_value(
    _In_ sai_switch_profile_id_t profile_id,
    _Out_ const char** variable,
    _Out_ const char** value)
{
    return -1;
}

const service_method_table_t test_services = {
    test_profile_get_value,
    test_profile_get_next_value
};


int main()
{
    sai_status_t              status;
    sai_lag_api_t            *lag_api;
    sai_switch_api_t         *switch_api;

    sai_switch_notification_t sw_notifications;
    sai_object_id_t           lag_oid[TEST_LAG_COUNT];
    sai_object_id_t           lag_member_oid[TEST_LAG_MEMBER_COUNT];
    sai_object_id_t           switch_port_oid_list[TEST_MAX_PORT_COUNT];

    sai_attribute_t           lag_member_attr[TEST_LAG_MEMBER_COUNT][TEST_LAG_MEMBER_ATTR_COUNT];
    sai_attribute_t           switch_port_list_attr;
    const uint32_t            ports_per_lag[TEST_LAG_COUNT] = { 16, 8, 4, 2, 2 };

    sai_object_id_t extra_lag_member_oid;
    sai_attribute_t *extra_lag_member_attr;

    // Initialize SAI and query APIs
    status = sai_api_initialize(0, &test_services);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to init adapter module, code %#08x\n", status);
        return EXIT_FAILURE;
    }

    status = sai_api_query(SAI_API_SWITCH, (void**)&switch_api);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to query switch api, code %#08X\n", status);
        return EXIT_FAILURE;
    }

    status = sai_api_query(SAI_API_LAG, (void**)&lag_api);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to query lag api, code %#08x\n", status);
        return status;
    }


    // Initialize switch
    status = switch_api->initialize_switch(0, "HW_ID", 0, &sw_notifications);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to init switch object, code %#08X\n", status);
        return EXIT_FAILURE;
    }

    // Get switch port list
    switch_port_list_attr = (sai_attribute_t){
        .id = SAI_SWITCH_ATTR_PORT_LIST,
        .value = {
            .objlist = {
                .list = switch_port_oid_list,
                .count = TEST_MAX_PORT_COUNT
            }
        }
    };

    status = switch_api->get_switch_attribute(1, &switch_port_list_attr);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to retrieve switch port list attr, code %#08X\n", status);
        return EXIT_FAILURE;
    }


    // Try to create LAG with invalid attributes
    fprintf(stderr, "----- Try to create LAG with invalid attributes -----\n");
    status = lag_api->create_lag(&lag_oid[0], 1, lag_member_attr[0]);
    if (status == SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Created LAG with invalid attributes but create_lag() returned success\n");
        return EXIT_FAILURE;
    }

    // Create LAGs
    fprintf(stderr, "----- Create LAG objects -----\n");
    for (int i = 0; i < TEST_LAG_COUNT; i++) {
        status = lag_api->create_lag(&lag_oid[i], 0, NULL);

        if (status != SAI_STATUS_SUCCESS) {
            fprintf(stderr, "Failed to create LAG obj, code %#08x\n", status);
            return EXIT_FAILURE;
        }
    }

    // Try to create LAG member with invalid attributes
    fprintf(stderr, "----- Try to create LAG member with invalid attributes -----\n");
    status = lag_api->create_lag(&lag_member_oid[0], 0, NULL);
    if (status == SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Created LAG member with invalid attributes but create_lag() returned success\n");
        return EXIT_FAILURE;
    }

    // Create LAG members
    fprintf(stderr, "----- Create LAG memmber objects -----\n");
    for (int l = 0, p = 0; l < TEST_LAG_COUNT; l++) {
        for (int m = 0; m < ports_per_lag[l]; m++, p++) {

            // Prepare LAG member attributes
            lag_member_attr[p][0] = (sai_attribute_t){
                .id = SAI_LAG_MEMBER_ATTR_LAG_ID,
                .value = { .oid = lag_oid[l] }
            };
            lag_member_attr[p][1] = (sai_attribute_t){
                .id = SAI_LAG_MEMBER_ATTR_PORT_ID,
                .value = { .oid = switch_port_oid_list[p] }
            };

            // Create LAG member
            status = lag_api->create_lag_member(
                &lag_member_oid[p],
                TEST_LAG_MEMBER_ATTR_COUNT,
                lag_member_attr[p]
            );

            if (status != SAI_STATUS_SUCCESS) {
                fprintf(stderr, "Failed to create LAG member obj, code %#08x\n", status);
                return EXIT_FAILURE;
            }
        }
    }

    // Try to exceed LAG limit
    sai_object_id_t extra_lag_oid;
    fprintf(stderr, "----- Try to exceed LAG limit -----\n");
    status = lag_api->create_lag(&extra_lag_oid, 0, NULL);
    if (status == SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Exceeded LAG limit but create_lag() returned success\n");
        return EXIT_FAILURE;
    }

    // Try to exceed overall LAG member limit
    extra_lag_member_oid = SAI_NULL_OBJECT_ID;
    extra_lag_member_attr = lag_member_attr[0];
    fprintf(stderr, "----- Try to exceed overall LAG member limit -----\n");
    status = lag_api->create_lag_member(
        &extra_lag_member_oid,
        TEST_LAG_MEMBER_ATTR_COUNT,
        extra_lag_member_attr
    );
    if (status == SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Exceeded overall LAG member limit but create_lag_member() returned success\n");
        return EXIT_FAILURE;
    }

    // Try to exceed LAG member limit per group
    fprintf(stderr, "----- Remove last LAG member -----\n");
    status = lag_api->remove_lag_member(lag_member_oid[TEST_LAG_MEMBER_COUNT - 1]);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to remove LAG member obj, code %#08x\n", status);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "----- Try to exceed LAG member limit per group -----\n");
    extra_lag_member_oid = SAI_NULL_OBJECT_ID;
    extra_lag_member_attr = lag_member_attr[0];
    status = lag_api->create_lag_member(
        &extra_lag_member_oid,
        TEST_LAG_MEMBER_ATTR_COUNT,
        extra_lag_member_attr
    );
    if (status == SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Exceeded per group LAG member limit but create_lag_member() returned success\n");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "----- Add last LAG member back -----\n");
    status = lag_api->create_lag_member(
        &lag_member_oid[TEST_LAG_MEMBER_COUNT - 1],
        TEST_LAG_MEMBER_ATTR_COUNT,
        lag_member_attr[TEST_LAG_MEMBER_COUNT - 1]
    );
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create LAG member obj, code %#08x\n", status);
        return EXIT_FAILURE;
    }

    // Try to remove non-empty LAG
    fprintf(stderr, "----- Try to remove non-empty LAG -----\n");
    status = lag_api->remove_lag(lag_oid[0]);
    if (status == SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Removed non-empty LAG but remove_lag() returned success\n");
        return EXIT_FAILURE;
    }

    // Try to remove LAG by non-existing OID
    fprintf(stderr, "----- Try to remove LAG by non-exisiting OID -----\n");
    status = lag_api->remove_lag(SAI_NULL_OBJECT_ID);
    if (status == SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Removed LAG by non-exisiting OID (SAI_NULL_OBJECT_ID) but remove_lag() returned success\n");
        return EXIT_FAILURE;
    }

    // Try to remove LAG member by non-existing LAG mmeber ID
    fprintf(stderr, "----- Try to remove LAG member by non-exisiting OID -----\n");
    status = lag_api->remove_lag_member(SAI_NULL_OBJECT_ID);
    if (status == SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Removed LAG member by non-exisiting OID (SAI_NULL_OBJECT_ID) but remove_lag_member() returned success\n");
        return EXIT_FAILURE;
    }

    // Check LAG attributes [PORT_LIST]
    fprintf(stderr, "----- Check LAG attributes -----\n");
    for (int l = 0, p = 0; l < TEST_LAG_COUNT; l++) {
        sai_object_id_t lag_port_oid_list[TEST_MAX_PORT_COUNT];
        sai_attribute_t lag_port_list_attr = {
            .id = SAI_LAG_ATTR_PORT_LIST,
            .value = {
                .objlist = {
                    .list = lag_port_oid_list,
                    .count = TEST_MAX_PORT_COUNT
                }
            }
        };
        sai_object_list_t *lag_ports = &lag_port_list_attr.value.objlist;

        status = lag_api->get_lag_attribute(lag_oid[l], 1, &lag_port_list_attr);
        if (status != SAI_STATUS_SUCCESS) {
            fprintf(stderr, "Failed to get port list from LAG obj %#016x, code %#08x\n", lag_oid[l], status);
            return EXIT_FAILURE;   
        }
        
        if (lag_ports->count != ports_per_lag[l]) {
            fprintf(stderr, "Unexpected port count in LAG OID %#016lx: got %u, expected %u\n",
                lag_oid[l],
                lag_ports->count,
                ports_per_lag[l]
            );
            return EXIT_FAILURE;
        }

        // Check check & show port OIDs
        printf("LAG OID %#016lx port list:\n", lag_oid[l]);

        for (int m = 0; m < lag_ports->count; m++, p++) {
            printf("\tPort OID %#016lx\n", lag_ports->list[m]);

            if (switch_port_oid_list[p] != lag_ports->list[m]) {
                fprintf(stderr, "Unexpected port OID in LAG OID %#016lx at index %d: got %#016lx, expected %#016lx\n",
                    lag_oid[l], m,
                    lag_ports->list[m],
                    switch_port_oid_list[p]
                );
                return EXIT_FAILURE;
            }
        }
    }

    // Check & show LAG member attributes [LAG_ID, PORT_ID]
    fprintf(stderr, "----- Check LAG member attributes -----\n");
    for (int m = 0; m < TEST_LAG_MEMBER_COUNT; m++) {
        sai_attribute_t lag_id_attr = { 
            .id = SAI_LAG_MEMBER_ATTR_LAG_ID, .value = {}
        };
        sai_attribute_t port_id_attr = {
            .id = SAI_LAG_MEMBER_ATTR_PORT_ID, .value = {}
        };

        // Check LAG_ID
        status = lag_api->get_lag_member_attribute(lag_member_oid[m], 1, &lag_id_attr);
        if (status != SAI_STATUS_SUCCESS) {
            fprintf(stderr, "Failed to get LAG OID from LAG member obj %#016x, code %#08x\n", lag_member_oid[m], status);
            return EXIT_FAILURE;   
        }
        if (lag_id_attr.value.oid != lag_member_attr[m][0].value.oid) {
            fprintf(stderr, "LAG OIDs doesn't match for LAG member OID %#016lx: got %#016lx, expected %#016lx\n",
                lag_member_oid[m],
                lag_id_attr.value.oid,
                lag_member_attr[m][0].value.oid
            );
            return EXIT_FAILURE;
        }

        // Check PORT_ID
        status = lag_api->get_lag_member_attribute(lag_member_oid[m], 1, &port_id_attr);
        if (status != SAI_STATUS_SUCCESS) {
            fprintf(stderr, "Failed to get Port OID from LAG member obj %#016x, code %#08x\n", lag_member_oid[m], status);
            return EXIT_FAILURE;   
        }
        if (port_id_attr.value.oid != lag_member_attr[m][1].value.oid) {
            fprintf(stderr, "Port OIDs doesn't match for LAG member OID %#016lx: got %#016lx, expected %#016lx\n",
                lag_member_oid[m],
                port_id_attr.value.oid,
                lag_member_attr[m][1].value.oid
            );
            return EXIT_FAILURE;
        }

        fprintf(stderr, "LAG member OID %#016lx:\n", lag_member_oid[m]);
        fprintf(stderr, "\tLAG OID %#016lx\n", lag_id_attr.value.oid);
        fprintf(stderr, "\tPort OID %#016lx\n", port_id_attr.value.oid);
    }

    // Remove LAG memebers
    fprintf(stderr, "----- Remove LAG memmber objects -----\n");
    for (int i = 0; i < TEST_LAG_MEMBER_COUNT; i++) {
        status = lag_api->remove_lag_member(lag_member_oid[i]);

        if (status != SAI_STATUS_SUCCESS) {
            fprintf(stderr, "Failed to remove lag obj, code %#08x\n", lag_oid[i], status);
            return EXIT_FAILURE;
        }
    }

    // Remove LAGs
    fprintf(stderr, "----- Remove LAG objects -----\n");
    for (int i = 0; i < TEST_LAG_COUNT; i++) {
        status = lag_api->remove_lag(lag_oid[i]);

        if (status != SAI_STATUS_SUCCESS) {
            fprintf(stderr, "Failed to remove lag obj, code %#08x\n", lag_oid[i], status);
            return EXIT_FAILURE;
        }
    }

    // Shutdown switch
    const bool warm_restart_hint = false;
    switch_api->shutdown_switch(warm_restart_hint);

    // Deinit adapter module
    status = sai_api_uninitialize();
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to unitialize adapter module, code %#08x\n", status);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

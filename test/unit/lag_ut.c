#include <stdlib.h>
#include <stdio.h>
#include "sai.h"

#define TEST_MAX_PORT_COUNT 64
#define TEST_LAG_COUNT 2
#define TEST_LAG_MEMBER_COUNT 4
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

    sai_attribute_t           lag_port_list_attr[TEST_LAG_COUNT];
    sai_object_id_t           lag_port_list_oid[TEST_LAG_COUNT][TEST_LAG_PORTS_PER_LAG];

    sai_object_id_t           switch_port_list_oid[TEST_MAX_PORT_COUNT];
    sai_attribute_t           switch_port_list_attr;

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
                .list = switch_port_list_oid,
                .count = TEST_MAX_PORT_COUNT
            }
        }
    };

    status = switch_api->get_switch_attribute(1, &switch_port_list_attr);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to retrieve switch port list attr, code %#08X\n", status);
        return EXIT_FAILURE;
    }


    // Create LAGs
    for (int i = 0; i < TEST_LAG_COUNT; i++) {
        status = lag_api->create_lag(&lag_oid[i], 0, NULL);

        if (status != SAI_STATUS_SUCCESS) {
            fprintf(stderr, "Failed to create LAG obj, code %#08x\n", status);
            return status;
        }
    }

    // Prepare LAG member attributes
    sai_attribute_t lag_member_attributes[] = {
        // LAG Member #1
        {
            .id = SAI_LAG_MEMBER_ATTR_LAG_ID,
            .value = { .oid = lag_oid[0] }
        },
        {
            .id = SAI_LAG_MEMBER_ATTR_PORT_ID,
            .value = { .oid = switch_port_list_oid[0] }
        },

        // LAG Member #2
        {
            .id = SAI_LAG_MEMBER_ATTR_LAG_ID,
            .value = { .oid = lag_oid[0] }
        },
        {
            .id = SAI_LAG_MEMBER_ATTR_PORT_ID,
            .value = { .oid = switch_port_list_oid[1] }
        },

        // LAG Member #3
        {
            .id = SAI_LAG_MEMBER_ATTR_LAG_ID,
            .value = { .oid = lag_oid[1] }
        },
        {
            .id = SAI_LAG_MEMBER_ATTR_PORT_ID,
            .value = { .oid = switch_port_list_oid[2] }
        },

        // LAG Member #4
        {
            .id = SAI_LAG_MEMBER_ATTR_LAG_ID,
            .value = { .oid = lag_oid[1] }
        },
        {
            .id = SAI_LAG_MEMBER_ATTR_PORT_ID,
            .value = { .oid = switch_port_list_oid[3] }
        },
    };

    // Create LAG members
    for (int i = 0; i < TEST_LAG_MEMBER_COUNT; i++) {
        status = lag_api->create_lag_member(
            &lag_member_oid[i],
            TEST_LAG_MEMBER_ATTR_COUNT,
            &lag_member_attributes[i * TEST_LAG_MEMBER_ATTR_COUNT]
        );

        if (status != SAI_STATUS_SUCCESS) {
            fprintf(stderr, "Failed to create LAG member obj, code %#08x\n", status);
            return EXIT_FAILURE;
        }
    }


    // Get LAG port lists
    for (int i = 0; i < TEST_LAG_COUNT; i++) {
        lag_port_list_attr[i] = (sai_attribute_t){
            .id = SAI_LAG_ATTR_PORT_LIST,
            .value = {
                .objlist = {
                    .list = lag_port_list_oid[i],
                    .count = TEST_LAG_PORTS_PER_LAG
                }
            }
        };

        status = lag_api->get_lag_attribute(lag_oid[i], 1, &lag_port_list_attr[i]);
    }


    // Get LAG memeber #1 LAG_ID
    sai_attribute_t lag_id = {
        .id = SAI_LAG_MEMBER_ATTR_LAG_ID,
        .value = {}
    };
    lag_api->get_lag_member_attribute(lag_member_oid[0], 1, &lag_id);

    // Get LAG memeber #3 PORT_ID
    sai_attribute_t port_id = { 
        .id = SAI_LAG_MEMBER_ATTR_PORT_ID,
        .value = {}
    };
    lag_api->get_lag_member_attribute(lag_member_oid[2], 1, &port_id);


    // Remove LAG memeber #2
    status = lag_api->remove_lag_member(lag_member_oid[1]);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to remove LAG member obj %#016x, code %#08x\n", lag_member_oid[1], status);
        return EXIT_FAILURE;
    }

    // Remove LAG memeber #3
    status = lag_api->remove_lag_member(lag_member_oid[2]);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to remove LAG member obj %#016x, code %#08x\n", lag_member_oid[2], status);
        return EXIT_FAILURE;
    }

    // Get LAG port lists
    for (int i = 0; i < TEST_LAG_COUNT; i++) {
        lag_port_list_attr[i] = (sai_attribute_t){
            .id = SAI_LAG_ATTR_PORT_LIST,
            .value = {
                .objlist = {
                    .list = lag_port_list_oid[i],
                    .count = TEST_LAG_PORTS_PER_LAG
                }
            }
        };

        lag_api->get_lag_attribute(lag_oid[i], 1, &lag_port_list_attr[i]);
    }

    // Remove LAG memeber #1
    status = lag_api->remove_lag_member(lag_member_oid[0]);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to remove LAG member obj %#016x, code %#08x\n", lag_member_oid[0], status);
        return EXIT_FAILURE;
    }

    // Remove LAG memeber #4
    status = lag_api->remove_lag_member(lag_member_oid[3]);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to remove LAG member obj %#016x, code %#08x\n", lag_member_oid[3], status);
        return EXIT_FAILURE;
    }

    // Remove LAGs #1 and #2
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

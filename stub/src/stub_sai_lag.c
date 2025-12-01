#include "sai.h"
#include "stub_sai.h"
#include <assert.h>

#define SAI_LAG_MAX_GROUP_COUNT 5
#define SAI_LAG_MAX_GROUP_SIZE 16
#define SAI_LAG_MAX_PORTS_USED 32

typedef struct _lag_member_db_entry_t {
    sai_object_id_t lag_oid;
    sai_object_id_t port_oid;
} lag_member_db_entry_t;

typedef struct _lag_db_entry_t {
    uint32_t        ports_mask;
    uint8_t         group_size;
} lag_db_entry_t;

struct lag_db_t {
    lag_db_entry_t        lags[SAI_LAG_MAX_GROUP_COUNT];
    lag_member_db_entry_t members[SAI_LAG_MAX_PORTS_USED];
    uint32_t              lags_mask;
    uint32_t              members_mask;
} lag_db;


static bool lag_db_get_lag_used(
    _In_ uint32_t db_id
) {
    return (lag_db.lags_mask & ((uint32_t)(1 << db_id))) != 0;
}

static bool lag_db_get_lag_member_used(
    _In_ uint32_t db_id
) {
    return (lag_db.members_mask & ((uint32_t)(1 << db_id))) != 0;
}

static bool lag_db_get_lag_port_used(
    _In_ uint32_t lag_db_id,
    _In_ uint32_t member_db_id
) {
    return (lag_db.lags[lag_db_id].ports_mask & ((uint32_t)(1 << member_db_id))) != 0;
}

static void lag_db_set_lag_used(
    _In_ uint32_t db_id,
    _In_ bool is_used
) {
    if (is_used) {
        lag_db.lags_mask |= ((uint32_t)(1 << db_id));
    } else {
        lag_db.lags_mask &= ~((uint32_t)(1 << db_id));
    }
}

static void lag_db_set_lag_member_used(
    _In_ uint32_t db_id,
    _In_ bool is_used
) {
    if (is_used) {
        lag_db.members_mask |= ((uint32_t)(1 << db_id));
    } else {
        lag_db.members_mask &= ~((uint32_t)(1 << db_id));
    }
}

static void lag_db_set_lag_port_used(
    _In_ uint32_t lag_db_id,
    _In_ uint32_t member_db_id,
    _In_ bool is_used
) {
    if (is_used) {
        if (!lag_db_get_lag_port_used(lag_db_id, member_db_id)) {
            lag_db.lags[lag_db_id].group_size++;
        }

        lag_db.lags[lag_db_id].ports_mask |= ((uint32_t)(1 << member_db_id));
    } else {
        if (lag_db_get_lag_port_used(lag_db_id, member_db_id)) {
            lag_db.lags[lag_db_id].group_size--;
        }

        lag_db.lags[lag_db_id].ports_mask &= ~((uint32_t)(1 << member_db_id));
    }
}


static sai_status_t get_lag_attribute(
    _In_ const sai_object_key_t   *key,
    _Inout_ sai_attribute_value_t *value,
    _In_ uint32_t                  attr_index,
    _Inout_ vendor_cache_t        *cache,
    void                          *arg);


static sai_status_t get_lag_member_attribute(
    _In_ const sai_object_key_t   *key,
    _Inout_ sai_attribute_value_t *value,
    _In_ uint32_t                  attr_index,
    _Inout_ vendor_cache_t        *cache,
    void                          *arg);


static const sai_attribute_entry_t lag_attribs[] = {
    { SAI_LAG_ATTR_PORT_LIST, false, false, false, true,
      "List of ports in LAG", SAI_ATTR_VAL_TYPE_OBJLIST },
    { END_FUNCTIONALITY_ATTRIBS_ID, false, false, false, false,
      "", SAI_ATTR_VAL_TYPE_UNDETERMINED }
};

static const sai_attribute_entry_t lag_member_attribs[] = {
    { SAI_LAG_MEMBER_ATTR_LAG_ID, true, true, false, true,
      "LAG ID", SAI_ATTR_VAL_TYPE_OID },
    { SAI_LAG_MEMBER_ATTR_PORT_ID, true, true, false, true,
      "PORT ID", SAI_ATTR_VAL_TYPE_OID },
    { END_FUNCTIONALITY_ATTRIBS_ID, false, false, false, false,
      "", SAI_ATTR_VAL_TYPE_UNDETERMINED }
};

static const sai_vendor_attribute_entry_t lag_vendor_attribs[] = {
    { SAI_LAG_ATTR_PORT_LIST,
      { false, false, false, true },
      { false, false, false, true },
      get_lag_attribute, (void*) SAI_LAG_ATTR_PORT_LIST,
      NULL, NULL }
};

static const sai_vendor_attribute_entry_t lag_member_vendor_attribs[] = {
    { SAI_LAG_MEMBER_ATTR_LAG_ID,
      { true, false, false, true },
      { true, false, false, true },
      get_lag_member_attribute, (void*) SAI_LAG_MEMBER_ATTR_LAG_ID,
      NULL, NULL },
    { SAI_LAG_MEMBER_ATTR_PORT_ID,
      { true, false, false, true },
      { true, false, false, true },
      get_lag_member_attribute, (void*) SAI_LAG_MEMBER_ATTR_PORT_ID,
      NULL, NULL }
};


static sai_status_t get_lag_attribute(
    _In_ const sai_object_key_t   *key,
    _Inout_ sai_attribute_value_t *value,
    _In_ uint32_t                  attr_index,
    _Inout_ vendor_cache_t        *cache,
    void                          *arg
) {
    sai_status_t status;
    uint32_t     db_index;

    status = stub_object_to_type(key->object_id, SAI_OBJECT_TYPE_LAG, &db_index);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot get LAG DB index\n");
        return status;
    }

    switch ((int64_t)arg) {
    case SAI_LAG_ATTR_PORT_LIST:
        if (value->objlist.count > lag_db.lags[db_index].group_size) {
            value->objlist.count = lag_db.lags[db_index].group_size;
        }

        uint32_t list_idx = 0;
        uint32_t member_idx = 0;

        while (member_idx < SAI_LAG_MAX_PORTS_USED && list_idx < value->objlist.count) {
            if (lag_db_get_lag_port_used(db_index, member_idx)) {
                value->objlist.list[list_idx] = lag_db.members[member_idx].port_oid;
                list_idx++;
            }

            member_idx++;
        }

        break;
    default:
        fprintf(stderr, "Got unexpected attribute ID\n");
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

static sai_status_t get_lag_member_attribute(
    _In_ const sai_object_key_t   *key,
    _Inout_ sai_attribute_value_t *value,
    _In_ uint32_t                  attr_index,
    _Inout_ vendor_cache_t        *cache,
    void                          *arg
) {
    sai_status_t status;
    uint32_t     db_index;

    status = stub_object_to_type(key->object_id, SAI_OBJECT_TYPE_LAG_MEMBER, &db_index);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot get LAG member DB index\n");
        return status;
    }

    switch ((int64_t)arg) {
    case SAI_LAG_MEMBER_ATTR_LAG_ID:
        value->oid = lag_db.members[db_index].lag_oid;
        break;
    case SAI_LAG_MEMBER_ATTR_PORT_ID:
        value->oid = lag_db.members[db_index].port_oid;
        break;
    default:
        fprintf(stderr, "Got unexpected attribute ID\n");
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_create_lag(
    _Out_ sai_object_id_t* lag_id,
    _In_ uint32_t attr_count,
    _In_ sai_attribute_t *attr_list)
{
    sai_status_t status;
    uint32_t lag_db_id;

    status = check_attribs_metadata(
        attr_count,
        attr_list,
        lag_attribs,
        lag_vendor_attribs,
        SAI_OPERATION_CREATE
    );
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot create LAG member: failed attributes check\n");
        return status;
    }

    lag_db_id = 0;
    while (lag_db_id < SAI_LAG_MAX_GROUP_COUNT) {
        if (!lag_db_get_lag_used(lag_db_id)) {
            break;
        }
        lag_db_id++;
    }

    if (lag_db_id == SAI_LAG_MAX_GROUP_COUNT) {
        fprintf(stderr, "Cannot create LAG: group limit is reached\n");
        return SAI_STATUS_FAILURE;
    }

    status = stub_create_object(SAI_OBJECT_TYPE_LAG, lag_db_id, lag_id);

    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot create a LAG OID\n");
        return status;
    }

    lag_db_set_lag_used(lag_db_id, true);
    lag_db.lags[lag_db_id].ports_mask = 0;
    lag_db.lags[lag_db_id].group_size = 0;

    return status;
}

sai_status_t stub_remove_lag(
    _In_ sai_object_id_t  lag_id)
{
    sai_status_t status;
    uint32_t     lag_db_id;
    
    status = stub_object_to_type(lag_id, SAI_OBJECT_TYPE_LAG, &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot get LAG DB ID\n");
        return status;
    }

    if (lag_db.lags[lag_db_id].group_size != 0) {
        fprintf(stderr, "Cannot remove LAG: group is not empty\n");
        return SAI_STATUS_FAILURE;
    }

    lag_db_set_lag_used(lag_db_id, false);
    lag_db.lags[lag_db_id].ports_mask = 0;
    lag_db.lags[lag_db_id].group_size = 0;

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_set_lag_attribute(
    _In_ sai_object_id_t  lag_id,
    _In_ const sai_attribute_t *attr)
{
    const sai_object_key_t key = {
        .object_id = lag_id
    };

    return sai_set_attribute(
        &key,
        NULL,
        lag_attribs,
        lag_vendor_attribs,
        attr
    );
}

sai_status_t stub_get_lag_attribute(
    _In_ sai_object_id_t lag_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list)
{
    const sai_object_key_t key = {
        .object_id = lag_id
    };

    return sai_get_attributes(
        &key,
        NULL,
        lag_attribs,
        lag_vendor_attribs,
        attr_count,
        attr_list
    );
}

sai_status_t stub_create_lag_member(
    _Out_ sai_object_id_t* lag_member_id,
    _In_ uint32_t attr_count,
    _In_ sai_attribute_t *attr_list)
{
    sai_status_t status;
    uint32_t lag_member_db_id;
    uint32_t lag_db_id;
    const sai_attribute_value_t *attr_val;
    uint32_t attr_idx;
    char list_str[MAX_LIST_VALUE_STR_LEN];                                                       

    status = check_attribs_metadata(
        attr_count,
        attr_list,
        lag_member_attribs,
        lag_member_vendor_attribs,
        SAI_OPERATION_CREATE
    );
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot create LAG member: failed attributes check\n");
        return status;
    }

    lag_member_db_id = 0;
    while (lag_member_db_id < SAI_LAG_MAX_PORTS_USED) {
        if (!lag_db_get_lag_member_used(lag_member_db_id)) {
            break;
        }
        lag_member_db_id++;
    }

    if (lag_member_db_id == SAI_LAG_MAX_PORTS_USED) {
        fprintf(stderr, "Cannot create LAG member: port limit is reached\n");
        return SAI_STATUS_FAILURE;
    }

    status = stub_create_object(SAI_OBJECT_TYPE_LAG_MEMBER, lag_member_db_id, lag_member_id);

    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot create a LAG member OID\n");
        return status;
    }

    sai_attr_list_to_str(attr_count, attr_list, lag_member_attribs, MAX_LIST_VALUE_STR_LEN, list_str);
    fprintf(stderr, "Create LAG member [OID %#016lx] (attrs: %s)\n", *lag_member_id, list_str);

    lag_db_set_lag_member_used(lag_member_db_id, true);
    lag_db.members[lag_member_db_id].port_oid = SAI_NULL_OBJECT_ID;
    lag_db.members[lag_member_db_id].lag_oid = SAI_NULL_OBJECT_ID;

    status = find_attrib_in_list(attr_count, attr_list, SAI_LAG_MEMBER_ATTR_PORT_ID, &attr_val, &attr_idx);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "LAG_ID attribute not found\n");
        return status;
    }
    lag_db.members[lag_member_db_id].port_oid = attr_val->oid;

    status = find_attrib_in_list(attr_count, attr_list, SAI_LAG_MEMBER_ATTR_LAG_ID, &attr_val, &attr_idx);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "PORT_ID attribute not found\n");
        return status;
    }
    lag_db.members[lag_member_db_id].lag_oid = attr_val->oid;

    status = stub_object_to_type(attr_val->oid, SAI_OBJECT_TYPE_LAG, &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot get LAG DB ID\n");
        return status;
    }

    if (lag_db.lags[lag_db_id].group_size == SAI_LAG_MAX_GROUP_SIZE) {
        fprintf(stderr, "Cannot add LAG member to group: port limit is reached\n");
        return SAI_STATUS_FAILURE;
    }

    lag_db_set_lag_port_used(lag_db_id, lag_member_db_id, true);

    return status;
}

sai_status_t stub_remove_lag_member(
    _In_ sai_object_id_t  lag_member_id)
{
    sai_status_t status;
    sai_object_id_t lag_id;
    uint32_t lag_db_id;
    uint32_t lag_member_db_id;

    status = stub_object_to_type(lag_member_id, SAI_OBJECT_TYPE_LAG_MEMBER, &lag_member_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot get LAG member DB ID\n");
        return status;
    }

    lag_id = lag_db.members[lag_member_db_id].lag_oid;

    fprintf(stderr, "Remove LAG member [OID %#016lx]\n", lag_member_id);
    lag_db_set_lag_member_used(lag_member_db_id, false);
    lag_db.members[lag_member_db_id].port_oid = SAI_NULL_OBJECT_ID;
    lag_db.members[lag_member_db_id].lag_oid = SAI_NULL_OBJECT_ID;

    status = stub_object_to_type(lag_id, SAI_OBJECT_TYPE_LAG, &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot get LAG DB ID\n");
        return status;
    }

    lag_db_set_lag_port_used(lag_db_id, lag_member_db_id, false);

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_set_lag_member_attribute(
    _In_ sai_object_id_t  lag_member_id,
    _In_ const sai_attribute_t *attr)
{
    const sai_object_key_t key = {
        .object_id = lag_member_id
    };

    return sai_set_attribute(
        &key,
        NULL,
        lag_member_attribs,
        lag_member_vendor_attribs,
        attr
    );
}

sai_status_t stub_get_lag_member_attribute(
    _In_ sai_object_id_t lag_member_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list)
{
    const sai_object_key_t key = {
        .object_id = lag_member_id
    };

    return sai_get_attributes(
        &key,
        NULL,
        lag_member_attribs,
        lag_member_vendor_attribs,
        attr_count,
        attr_list
    );
}

const sai_lag_api_t lag_api = {
    stub_create_lag,
    stub_remove_lag,
    stub_set_lag_attribute,
    stub_get_lag_attribute,
    stub_create_lag_member,
    stub_remove_lag_member,
    stub_set_lag_member_attribute,
    stub_get_lag_member_attribute
};

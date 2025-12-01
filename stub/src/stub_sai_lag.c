#include "sai.h"
#include "stub_sai.h"
#include "assert.h"

sai_status_t stub_create_lag(
    _Out_ sai_object_id_t* lag_id,
    _In_ uint32_t attr_count,
    _In_ sai_attribute_t *attr_list)
{
    static int32_t next_lag_id = 1;
    sai_status_t status;

    status = stub_create_object(SAI_OBJECT_TYPE_LAG, next_lag_id++, lag_id);

    if (status == SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Create LAG [OID %#016lx]\n", *lag_id);
    } else {
        fprintf(stderr, "Cannot create a LAG OID\n");
    }

    return status;
}

sai_status_t stub_remove_lag(
    _In_ sai_object_id_t  lag_id)
{
    fprintf(stderr, "Remove LAG [OID %#016lx]\n", lag_id);

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_set_lag_attribute(
    _In_ sai_object_id_t  lag_id,
    _In_ const sai_attribute_t *attr)
{
    fprintf(stderr, "LAG [OID %#016lx]: [set attribute stub]\n", lag_id);

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_get_lag_attribute(
    _In_ sai_object_id_t lag_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list)
{
    fprintf(stderr, "LAG [OID %#016lx]: [get attribute stub]\n", lag_id);

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_create_lag_member(
    _Out_ sai_object_id_t* lag_member_id,
    _In_ uint32_t attr_count,
    _In_ sai_attribute_t *attr_list)
{
    static int32_t next_lag_member_id = 1;
    sai_status_t status;

    status = stub_create_object(
        SAI_OBJECT_TYPE_LAG_MEMBER,
        next_lag_member_id++,
        lag_member_id
    );

    if (status == SAI_STATUS_SUCCESS) {
        fprintf(stderr, "Create LAG member [OID %#016lx]\n", *lag_member_id);
    } else {
        fprintf(stderr, "Cannot create a LAG member OID\n");
    }

    return status;
}

sai_status_t stub_remove_lag_member(
    _In_ sai_object_id_t  lag_member_id)
{
    fprintf(stderr, "Remove LAG member [OID %#016lx]\n", lag_member_id);

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_set_lag_member_attribute(
    _In_ sai_object_id_t  lag_member_id,
    _In_ const sai_attribute_t *attr)
{
    fprintf(stderr, "LAG member [OID %#016lx]: [set attribute stub]\n", lag_member_id);

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_get_lag_member_attribute(
    _In_ sai_object_id_t lag_member_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list)
{
    fprintf(stderr, "LAG member [OID %#016lx]: [get attribute stub]\n", lag_member_id);

    return SAI_STATUS_SUCCESS;
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

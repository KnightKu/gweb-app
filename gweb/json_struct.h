#ifndef JSON_STRUCT_H
#define JSON_STRUCT_H

/*
 * JSON to C structure map used by MySQL and other dump routines. Each
 * of the message below has a corresponding response structure.
 */
enum {
    /* MSG-START */ JSON_C_MSG_MIN,
    JSON_C_REGISTRATION_MSG,
    JSON_C_PROFILE_MSG,
    JSON_C_LOGIN_MSG,
    JSON_C_AVATAR_MSG,
    JSON_C_CXN_REQUEST_MSG,
    JSON_C_CXN_CHANNEL_MSG,
    JSON_C_CXN_REQUEST_QUERY_MSG,
    JSON_C_CXN_CHANNEL_QUERY_MSG,
    JSON_C_UID_QUERY_MSG,
    JSON_C_PROFILE_QUERY_MSG,
    JSON_C_AVATAR_QUERY_MSG,
    JSON_C_CXN_PREFERENCE_MSG,
    JSON_C_CXN_PREFERENCE_QUERY_MSG,
    JSON_C_LOCATION_MSG,
    JSON_C_LOCATION_QUERY_MSG,
    JSON_C_NEIGHBOUR_QUERY_MSG,
    /* MSG-END */ JSON_C_MSG_MAX,
};

enum {
    /* RESP-START */ JSON_C_RESP_MIN,
    JSON_C_REGISTRATION_RESP,
    JSON_C_PROFILE_RESP,
    JSON_C_AVATAR_RESP,
    JSON_C_CXN_REQUEST_RESP,
    JSON_C_CXN_CHANNEL_RESP,
    JSON_C_CXN_REQUEST_QUERY_RESP,
    JSON_C_CXN_CHANNEL_QUERY_RESP,
    JSON_C_UID_QUERY_RESP,
    JSON_C_PROFILE_INFO_RESP,
    JSON_C_AVATAR_QUERY_RESP,
    JSON_C_CXN_PREFERENCE_RESP,
    JSON_C_CXN_PREFERENCE_QUERY_RESP,
    JSON_C_LOCATION_RESP,
    JSON_C_LOCATION_QUERY_RESP,
    JSON_C_NEIGHBOUR_QUERY_RESP,
    /* RESP-END */ JSON_C_RESP_MAX,
};

/* Magic constant to indicate the array start/end in framing
 * response.
 */
#define JSON_C_ARRAY_START     ((void *)0xDEADCAFE)
#define JSON_C_ARRAY_END       ((void *)0xEDDAACEF)

enum _JSON_C_REGISTRATION_MSG_FIELDS {
    FIELD_REGISTRATION_FNAME,
    FIELD_REGISTRATION_LNAME,
    FIELD_REGISTRATION_EMAIL,
    FIELD_REGISTRATION_PHONE,
    FIELD_REGISTRATION_PASSWORD,
    FIELD_REGISTRATION_MAX,
};

enum _JSON_C_PROFILE_MSG_FIELDS {
    FIELD_PROFILE_UID,
    FIELD_PROFILE_ADDRESS1,
    FIELD_PROFILE_ADDRESS2,
    FIELD_PROFILE_ADDRESS3,
    FIELD_PROFILE_COUNTRY,
    FIELD_PROFILE_STATE,
    FIELD_PROFILE_PINCODE,
    FIELD_PROFILE_FACEBOOK_HANDLE,
    FIELD_PROFILE_TWITTER_HANDLE,
    FIELD_PROFILE_MAX,
};

enum _JSON_C_LOGIN_MSG_FIELDS {
    FIELD_LOGIN_EMAIL,
    FIELD_LOGIN_PASSWORD,
    FIELD_LOGIN_MAX,
};

enum _JSON_C_AVATAR_MSG_FIELDS {
    FIELD_AVATAR_UID,
    FIELD_AVATAR_URL,
    FIELD_AVATAR_MAX,
};

enum _JSON_C_CXN_REQUEST_MSG_FIELDS {
    FIELD_CXN_REQUEST_UID,
    FIELD_CXN_REQUEST_TO_UID,
    FIELD_CXN_REQUEST_FLAG,
    FIELD_CXN_REQUEST_MAX,
};

enum _JSON_C_CXN_CHANNEL_MSG_FIELDS {
    FIELD_CXN_CHANNEL_UID,
    FIELD_CXN_CHANNEL_TO_UID,
    FIELD_CXN_CHANNEL_TYPE,
    FIELD_CXN_CHANNEL_MAX,
};

enum _JSON_C_LOCATION_MSG_FIELDS {
    FIELD_LOCATION_UID,
    FIELD_LOCATION_LATITUDE,
    FIELD_LOCATION_LONGITUDE,
    FIELD_LOCATION_ALTITUDE,
    FIELD_LOCATION_EXPIRY,
    FIELD_LOCATION_RADIUS,
    FIELD_LOCATION_MAX,
};

enum _JSON_C_LOCATION_QUERY_MSG_FIELDS {
    FIELD_LOCATION_QUERY_UID,
    FIELD_LOCATION_QUERY_MAX,
};

enum _JSON_C_NEIGHBOUR_QUERY_MSG_FIELDS {
    FIELD_NEIGHBOUR_QUERY_UID,
    FIELD_NEIGHBOUR_QUERY_RADIUS,
    FIELD_NEIGHBOUR_QUERY_MAX,
};

enum _JSON_C_CXN_REQUEST_QUERY_MSG_FIELDS {
    FIELD_CXN_REQUEST_QUERY_FROM_UID,
    FIELD_CXN_REQUEST_QUERY_TO_UID,
    FIELD_CXN_REQUEST_QUERY_FLAG,
    FIELD_CXN_REQUEST_QUERY_MAX,
};

enum _JSON_C_CXN_CHANNEL_QUERY_MSG_FIELDS {
    FIELD_CXN_CHANNEL_QUERY_FROM_UID,
    FIELD_CXN_CHANNEL_QUERY_TO_UID,
    FIELD_CXN_CHANNEL_QUERY_TYPE,
    FIELD_CXN_CHANNEL_QUERY_MAX,
};

enum _JSON_C_UID_QUERY_MSG_FIELDS {
    FIELD_UID_QUERY_EMAIL,
    FIELD_UID_QUERY_MAX,
};

enum _JSON_C_PROFILE_QUERY_MSG_FIELDS {
    FIELD_PROFILE_QUERY_UID,
    FIELD_PROFILE_QUERY_MAX,
};

enum _JSON_C_AVATAR_QUERY_MSG_FIELDS {
    FIELD_AVATAR_QUERY_UID,
    FIELD_AVATAR_QUERY_MAX,
};

enum _JSON_C_CXN_PREFERENCE_MSG_FIELDS {
    FIELD_CXN_PREFERENCE_UID,
    FIELD_CXN_PREFERENCE_PREFERENCE,
    FIELD_CXN_PREFERENCE_ARRAY_START,
    FIELD_CXN_PREFERENCE_CHANNEL_TYPE,
    FIELD_CXN_PREFERENCE_FLAG,
    FIELD_CXN_PREFERENCE_ARRAY_END,
    FIELD_CXN_PREFERENCE_MAX,
};

enum _JSON_C_CXN_PREFERENCE_QUERY_MSG_FIELDS {
    FIELD_CXN_PREFERENCE_QUERY_UID,
    FIELD_CXN_PREFERENCE_QUERY_MAX,
};

enum _JSON_C_REGISTRATION_RESP_FIELDS {
    FIELD_REGISTRATION_RESP_CODE,
    FIELD_REGISTRATION_RESP_DESC,
    FIELD_REGISTRATION_RESP_UID,
    FIELD_REGISTRATION_RESP_MAX,
};

enum _JSON_C_PROFILE_RESP_FIELDS {
    FIELD_PROFILE_RESP_CODE,
    FIELD_PROFILE_RESP_DESC,
    FIELD_PROFILE_RESP_MAX,
};

/* Common profile response */
enum _JSON_C_PROFILE_INFO_RESP_FIELDS {
    FIELD_PROFILE_INFO_RESP_CODE,
    FIELD_PROFILE_INFO_RESP_DESC,
    FIELD_PROFILE_INFO_RESP_UID,
    FIELD_PROFILE_INFO_RESP_FNAME,
    FIELD_PROFILE_INFO_RESP_LNAME,
    FIELD_PROFILE_INFO_RESP_EMAIL,
    FIELD_PROFILE_INFO_RESP_PHONE,
    FIELD_PROFILE_INFO_RESP_ADDRESS1,
    FIELD_PROFILE_INFO_RESP_ADDRESS2,
    FIELD_PROFILE_INFO_RESP_ADDRESS3,
    FIELD_PROFILE_INFO_RESP_COUNTRY,
    FIELD_PROFILE_INFO_RESP_STATE,
    FIELD_PROFILE_INFO_RESP_PINCODE,
    FIELD_PROFILE_INFO_RESP_FACEBOOK_HANDLE,
    FIELD_PROFILE_INFO_RESP_TWITTER_HANDLE,
    FIELD_PROFILE_INFO_RESP_AVATAR_URL,
    FIELD_PROFILE_INFO_RESP_FLAG,
    FIELD_PROFILE_INFO_RESP_MAX,
};

enum _JSON_C_AVATAR_RESP_FIELDS {
    FIELD_AVATAR_RESP_CODE,
    FIELD_AVATAR_RESP_DESC,
    FIELD_AVATAR_RESP_MAX,
};

enum _JSON_C_CXN_REQUEST_RESP_FIELDS {
    FIELD_CXN_REQUEST_RESP_CODE,
    FIELD_CXN_REQUEST_RESP_DESC,
    FIELD_CXN_REQUEST_RESP_MAX,
};

enum _JSON_C_CXN_CHANNEL_RESP_FIELDS {
    FIELD_CXN_CHANNEL_RESP_CODE,
    FIELD_CXN_CHANNEL_RESP_DESC,
    FIELD_CXN_CHANNEL_RESP_MAX,
};

enum _JSON_C_CXN_REQUEST_QUERY_RESP_FIELDS {
    FIELD_CXN_REQUEST_QUERY_RESP_CODE,
    FIELD_CXN_REQUEST_QUERY_RESP_DESC,
    FIELD_CXN_REQUEST_QUERY_RESP_RECORD_COUNT,
    FIELD_CXN_REQUEST_QUERY_RESP_ARRAY_START,
    FIELD_CXN_REQUEST_QUERY_RESP_UID,
    FIELD_CXN_REQUEST_QUERY_RESP_FNAME,
    FIELD_CXN_REQUEST_QUERY_RESP_LNAME,
    FIELD_CXN_REQUEST_QUERY_RESP_AVATAR_URL,
    FIELD_CXN_REQUEST_QUERY_RESP_DATE,
    FIELD_CXN_REQUEST_QUERY_RESP_FLAG,
    FIELD_CXN_REQUEST_QUERY_RESP_ARRAY_END,
    FIELD_CXN_REQUEST_QUERY_RESP_MAX,
};

enum _JSON_C_CXN_CHANNEL_QUERY_RESP_FIELDS {
    FIELD_CXN_CHANNEL_QUERY_RESP_CODE,
    FIELD_CXN_CHANNEL_QUERY_RESP_DESC,
    FIELD_CXN_CHANNEL_QUERY_RESP_RECORD_COUNT,
    FIELD_CXN_CHANNEL_QUERY_RESP_ARRAY_START,
    FIELD_CXN_CHANNEL_QUERY_RESP_UID,
    FIELD_CXN_CHANNEL_QUERY_RESP_FNAME,
    FIELD_CXN_CHANNEL_QUERY_RESP_LNAME,
    FIELD_CXN_CHANNEL_QUERY_RESP_AVATAR_URL,
    FIELD_CXN_CHANNEL_QUERY_RESP_DATE,
    FIELD_CXN_CHANNEL_QUERY_RESP_CHANNEL_TYPE,
    FIELD_CXN_CHANNEL_QUERY_RESP_ARRAY_END,
    FIELD_CXN_CHANNEL_QUERY_RESP_MAX,
};

enum _JSON_C_UID_QUERY_RESP_FIELDS {
    FIELD_UID_QUERY_RESP_CODE,
    FIELD_UID_QUERY_RESP_DESC,
    FIELD_UID_QUERY_RESP_UID,
    FIELD_UID_QUERY_RESP_MAX,
};

enum _JSON_C_AVATAR_QUERY_RESP_FIELDS {
    FIELD_AVATAR_QUERY_RESP_CODE,
    FIELD_AVATAR_QUERY_RESP_DESC,
    FIELD_AVATAR_QUERY_RESP_URL,
    FIELD_AVATAR_QUERY_RESP_MAX,
};

enum _JSON_C_CXN_PREFERENCE_RESP_FIELDS {
    FIELD_CXN_PREFERENCE_RESP_CODE,
    FIELD_CXN_PREFERENCE_RESP_DESC,
    FIELD_CXN_PREFERENCE_RESP_MAX,
};

enum _JSON_C_CXN_PREFERENCE_QUERY_RESP_FIELDS {
    FIELD_CXN_PREFERENCE_QUERY_RESP_CODE,
    FIELD_CXN_PREFERENCE_QUERY_RESP_DESC,
    FIELD_CXN_PREFERENCE_QUERY_RESP_RECORD_COUNT,
    FIELD_CXN_PREFERENCE_QUERY_RESP_ARRAY_START,
    FIELD_CXN_PREFERENCE_QUERY_RESP_CHANNEL_TYPE,
    FIELD_CXN_PREFERENCE_QUERY_RESP_FLAG,
    FIELD_CXN_PREFERENCE_QUERY_RESP_ARRAY_END,
    FIELD_CXN_PREFERENCE_QUERY_RESP_MAX,
};

enum _JSON_C_LOCATION_RESP_FIELDS {
    FIELD_LOCATION_RESP_CODE,
    FIELD_LOCATION_RESP_DESC,
    FIELD_LOCATION_RESP_MAX,
};

enum _JSON_C_LOCATION_QUERY_RESP_FIELDS {
    FIELD_LOCATION_QUERY_RESP_CODE,
    FIELD_LOCATION_QUERY_RESP_DESC,
    FIELD_LOCATION_QUERY_RESP_LATITUDE,
    FIELD_LOCATION_QUERY_RESP_LONGITUDE,
    FIELD_LOCATION_QUERY_RESP_ALTITUDE,
    FIELD_LOCATION_QUERY_RESP_LOCATION_TIME,
    FIELD_LOCATION_QUERY_RESP_EXPIRY,
    FIELD_LOCATION_QUERY_RESP_RADIUS,
    FIELD_LOCATION_QUERY_RESP_MAX,
};

enum _JSON_C_NEIGHBOUR_QUERY_RESP_FIELDS {
    FIELD_NEIGHBOUR_QUERY_RESP_CODE,
    FIELD_NEIGHBOUR_QUERY_RESP_DESC,
    FIELD_NEIGHBOUR_QUERY_RESP_RECORD_COUNT,
    FIELD_NEIGHBOUR_QUERY_RESP_ARRAY_START,
    FIELD_NEIGHBOUR_QUERY_RESP_UID,
    FIELD_NEIGHBOUR_QUERY_RESP_FNAME,
    FIELD_NEIGHBOUR_QUERY_RESP_LNAME,
    FIELD_NEIGHBOUR_QUERY_RESP_AVATAR_URL,
    FIELD_NEIGHBOUR_QUERY_RESP_LATITUDE,
    FIELD_NEIGHBOUR_QUERY_RESP_LONGITUDE,
    FIELD_NEIGHBOUR_QUERY_RESP_ALTITUDE,
    FIELD_NEIGHBOUR_QUERY_RESP_DISTANCE,
    FIELD_NEIGHBOUR_QUERY_RESP_ARRAY_END,
    FIELD_NEIGHBOUR_QUERY_RESP_MAX,
};

#define J2C_MSG_TABLE(name, ...)                \
    struct j2c_##name##_msg __VA_ARGS__

#define J2C_MSG_STRUCT(name, max_fields)        \
    J2C_MSG_TABLE(name) {                       \
        const char *fields[max_fields];         \
    }

J2C_MSG_STRUCT(registration, FIELD_REGISTRATION_MAX);
J2C_MSG_STRUCT(profile, FIELD_PROFILE_MAX);
J2C_MSG_STRUCT(login, FIELD_LOGIN_MAX);
J2C_MSG_STRUCT(avatar, FIELD_AVATAR_MAX);
J2C_MSG_STRUCT(cxn_request, FIELD_CXN_REQUEST_MAX);
J2C_MSG_STRUCT(cxn_channel, FIELD_CXN_CHANNEL_MAX);
J2C_MSG_STRUCT(cxn_request_query, FIELD_CXN_REQUEST_QUERY_MAX);
J2C_MSG_STRUCT(cxn_channel_query, FIELD_CXN_CHANNEL_QUERY_MAX);
J2C_MSG_STRUCT(uid_query, FIELD_UID_QUERY_MAX);
J2C_MSG_STRUCT(profile_query, FIELD_PROFILE_QUERY_MAX);
J2C_MSG_STRUCT(avatar_query, FIELD_AVATAR_QUERY_MAX);
J2C_MSG_STRUCT(cxn_preference_query, FIELD_CXN_PREFERENCE_QUERY_MAX);
J2C_MSG_STRUCT(location, FIELD_LOCATION_MAX);
J2C_MSG_STRUCT(location_query, FIELD_LOCATION_QUERY_MAX);
J2C_MSG_STRUCT(neighbour_query, FIELD_NEIGHBOUR_QUERY_MAX);

struct j2c_cxn_preference_msg_array1 {
    const char *fields[FIELD_CXN_PREFERENCE_ARRAY_END -
                 FIELD_CXN_PREFERENCE_ARRAY_START];
};

struct j2c_cxn_preference_msg {
    const char *fields[FIELD_CXN_PREFERENCE_ARRAY_START];
    int nr_array1_records;
    struct j2c_cxn_preference_msg_array1 *array1;
};

typedef union {
    J2C_MSG_TABLE(registration, registration);
    J2C_MSG_TABLE(profile, profile);
    J2C_MSG_TABLE(login, login);
    J2C_MSG_TABLE(avatar, avatar);
    J2C_MSG_TABLE(cxn_request, cxn_request);
    J2C_MSG_TABLE(cxn_channel, cxn_channel);
    J2C_MSG_TABLE(cxn_request_query, cxn_request_query);
    J2C_MSG_TABLE(cxn_channel_query, cxn_channel_query);
    J2C_MSG_TABLE(uid_query, uid_query);
    J2C_MSG_TABLE(profile_query, profile_query);
    J2C_MSG_TABLE(avatar_query, avatar_query);
    J2C_MSG_TABLE(cxn_preference, cxn_preference);
    J2C_MSG_TABLE(cxn_preference_query, cxn_preference_query);
    J2C_MSG_TABLE(location, location);
    J2C_MSG_TABLE(location_query, location_query);
    J2C_MSG_TABLE(neighbour_query, neighbour_query);
} j2c_msg_t;

#define J2C_RESP_TABLE(name, ...)               \
  struct j2c_##name##_resp __VA_ARGS__

#define J2C_RESP_STRUCT(name, max_fields)       \
    J2C_RESP_TABLE(name) {                      \
        char *fields[max_fields];		\
    }

J2C_RESP_STRUCT(registration, FIELD_REGISTRATION_RESP_MAX);
J2C_RESP_STRUCT(profile, FIELD_PROFILE_RESP_MAX);
J2C_RESP_STRUCT(avatar, FIELD_AVATAR_RESP_MAX);
J2C_RESP_STRUCT(cxn_request, FIELD_CXN_REQUEST_RESP_MAX);
J2C_RESP_STRUCT(cxn_channel, FIELD_CXN_CHANNEL_RESP_MAX);
J2C_RESP_STRUCT(uid_query, FIELD_UID_QUERY_RESP_MAX);
J2C_RESP_STRUCT(profile_info, FIELD_PROFILE_INFO_RESP_MAX);
J2C_RESP_STRUCT(avatar_query, FIELD_AVATAR_QUERY_RESP_MAX);
J2C_RESP_STRUCT(cxn_preference, FIELD_CXN_PREFERENCE_RESP_MAX);
J2C_RESP_STRUCT(location, FIELD_LOCATION_RESP_MAX);
J2C_RESP_STRUCT(location_query, FIELD_LOCATION_QUERY_RESP_MAX);

struct j2c_cxn_request_query_resp_array1 {
    char *fields[FIELD_CXN_REQUEST_QUERY_RESP_ARRAY_END -
                 FIELD_CXN_REQUEST_QUERY_RESP_ARRAY_START];
};

struct j2c_cxn_request_query_resp {
    char *fields[FIELD_CXN_REQUEST_QUERY_RESP_ARRAY_START];
    int nr_array1_records;
    struct j2c_cxn_request_query_resp_array1 *array1;
};

struct j2c_cxn_channel_query_resp_array1 {
    char *fields[FIELD_CXN_CHANNEL_QUERY_RESP_ARRAY_END -
                 FIELD_CXN_CHANNEL_QUERY_RESP_ARRAY_START];
};

struct j2c_cxn_channel_query_resp {
    char *fields[FIELD_CXN_CHANNEL_QUERY_RESP_ARRAY_START];
    int nr_array1_records;
    struct j2c_cxn_channel_query_resp_array1 *array1;
};

struct j2c_cxn_preference_query_resp_array1 {
    char *fields[FIELD_CXN_PREFERENCE_QUERY_RESP_ARRAY_END -
                 FIELD_CXN_PREFERENCE_QUERY_RESP_ARRAY_START];
};

struct j2c_cxn_preference_query_resp {
    char *fields[FIELD_CXN_PREFERENCE_QUERY_RESP_ARRAY_START];
    int nr_array1_records;
    struct j2c_cxn_preference_query_resp_array1 *array1;
};

struct j2c_neighbour_query_resp_array1 {
    char *fields[FIELD_NEIGHBOUR_QUERY_RESP_ARRAY_END -
                 FIELD_NEIGHBOUR_QUERY_RESP_ARRAY_START];
};

struct j2c_neighbour_query_resp {
    char *fields[FIELD_NEIGHBOUR_QUERY_RESP_ARRAY_START];
    int nr_array1_records;
    struct j2c_neighbour_query_resp_array1 *array1;
};

typedef union {
    J2C_RESP_TABLE(registration, registration);
    J2C_RESP_TABLE(profile, profile);
    J2C_RESP_TABLE(avatar, avatar);
    J2C_RESP_TABLE(cxn_request, cxn_request);
    J2C_RESP_TABLE(cxn_request_query, cxn_request_query);
    J2C_RESP_TABLE(cxn_channel, cxn_channel);
    J2C_RESP_TABLE(cxn_channel_query, cxn_channel_query);
    J2C_RESP_TABLE(uid_query, uid_query);
    J2C_RESP_TABLE(profile_info, profile_info);
    J2C_RESP_TABLE(avatar_query, avatar_query);
    J2C_RESP_TABLE(cxn_preference, cxn_preference);
    J2C_RESP_TABLE(cxn_preference_query, cxn_preference_query);
    J2C_RESP_TABLE(location, location);
    J2C_RESP_TABLE(location_query, location_query);
    J2C_RESP_TABLE(neighbour_query, neighbour_query);
} j2c_resp_t;

#endif // JSON_STRUCT_H

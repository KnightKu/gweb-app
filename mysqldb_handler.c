/*
 * Handler routines for DB updates
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include <mysql.h>

#include <gweb/common.h>
#include <gweb/json_struct.h>
#include <gweb/json_api.h>
#include <gweb/mysqldb_api.h>
#include <gweb/mysqldb_conf.h>
#include <gweb/mysqldb_log.h>
#include <gweb/uid.h>

/* Misc macros */
#define MAX_DATETIME_STRSZ   (20)

/* MySQL transaction status */
enum {
    GWEB_MYSQL_ERR_NO_RECORD = 1,
    GWEB_MYSQL_ERR_NO_MEMORY,
    GWEB_MYSQL_ERR_UNKNOWN,
    GWEB_MYSQL_ERR_DUPLICATE,
    GWEB_MYSQL_OK,
};

static MYSQL *g_mysql_ctx;

/* Atomic transactions -- depends on the backend storage engine
 * (eg. InnoDB)
 */
static inline void
gweb_mysql_start_transaction (void)
{
    gweb_mysql_ping();

    mysql_query(g_mysql_ctx, "START TRANSACTION");
}

/* Assume Abort/Commit is done before timeout and there is no need to
 * do ping again. Need to see how mysql behaves when ping is done
 * between the transaction block.
 */
static inline void
gweb_mysql_abort_transaction (void)
{
    /* gweb_mysql_ping(); */

    mysql_query(g_mysql_ctx, "ROLLBACK");
}

static inline void
gweb_mysql_commit_transaction (void)
{
    /* gweb_mysql_ping(); */

    mysql_query(g_mysql_ctx, "COMMIT");
}

static void
gweb_get_utc_datetime (char *dtbuf)
{
    time_t now;
    struct tm *tm_info;

    time(&now);
    tm_info = gmtime(&now);

    strftime(dtbuf, MAX_DATETIME_STRSZ, "%Y-%m-%d %H:%M:%S", tm_info);
}

static j2c_resp_t *
gweb_mysql_allocate_response (void)
{
    return calloc(1, sizeof(j2c_resp_t));
}

/*
 * Map MYSQL to HTTP code here (NOTE: needed a cleaner abstraction)
 */
static void
gweb_mysql_get_common_response (int mysql_code,
                                char **code, char **desc)
{
    switch (mysql_code) {
    case GWEB_MYSQL_ERR_NO_RECORD:
        *code = "404";
        *desc = "Record Not Found";
        break;

    case GWEB_MYSQL_ERR_UNKNOWN:
        *code = "404";
        *desc = "Unknown Error";
        break;

    case GWEB_MYSQL_ERR_DUPLICATE:
        *code = "404";
        *desc = "Duplicate Entry";
        break;

    case GWEB_MYSQL_OK:
        *code = "200";
        *desc = "OK";
        break;

    default:
        *code = NULL;
        *desc = NULL;
        break;
    }
}
#define generate_default_response(type, macro, code)                    \
    {                                                                   \
        struct j2c_##type##_resp *table;                                \
                                                                        \
        table = &resp->type;                                            \
        gweb_mysql_get_common_response(code,                            \
                       &table->fields[FIELD_##macro##_RESP_CODE],       \
                       &table->fields[FIELD_##macro##_RESP_DESC]);      \
    }

static void
gweb_mysql_update_response (int resp_type, int mysql_code,
                            j2c_resp_t **response)
{
    j2c_resp_t *resp;

    if (response == NULL) {
        return;
    }
    resp = *response;

    switch (resp_type) {
    case JSON_C_REGISTRATION_RESP:
        generate_default_response(registration, REGISTRATION, mysql_code);
        break;

    case JSON_C_PROFILE_RESP:
        generate_default_response(profile, PROFILE, mysql_code);
        break;

    case JSON_C_LOGIN_RESP:
        generate_default_response(login, LOGIN, mysql_code);
        break;

    case JSON_C_AVATAR_RESP:
        generate_default_response(avatar, AVATAR, mysql_code);
        break;

    case JSON_C_CXN_REQUEST_RESP:
        generate_default_response(cxn_request, CXN_REQUEST, mysql_code);
        break;

    case JSON_C_CXN_REQUEST_QUERY_RESP:
        generate_default_response(cxn_request_query, CXN_REQUEST_QUERY, mysql_code);
        break;

    case JSON_C_CXN_CHANNEL_RESP:
        generate_default_response(cxn_channel, CXN_CHANNEL, mysql_code);
        break;

    case JSON_C_CXN_CHANNEL_QUERY_RESP:
        generate_default_response(cxn_channel_query, CXN_CHANNEL_QUERY, mysql_code);
        break;

    default:
        return;
    }
    return;
}

static void
gweb_mysql_prepare_response (int resp_type, int mysql_code,
                             j2c_resp_t **response)
{
    if (response == NULL) {
        return;
    }

    *response = gweb_mysql_allocate_response();

    gweb_mysql_update_response(resp_type, mysql_code, response);

    return;
}

#define PUSH_BUF(buf, len, fmt...)             \
    len += sprintf(buf+len, fmt)

/* For APIs requiring just a peek of record to see if one or more rows
 * exists.
 */
static int
gweb_mysql_get_query_count (const char *query)
{
    MYSQL_RES *result;
    int ret;

    if (mysql_query(g_mysql_ctx, query)) {
        report_mysql_error_noclose(g_mysql_ctx);
    }

    if ((result = mysql_store_result(g_mysql_ctx)) == NULL) {
        report_mysql_error_noclose(g_mysql_ctx);
    }

    ret = mysql_num_rows(result);

    mysql_free_result(result);

    return ret;
}

static int
gweb_mysql_check_uid_email (const char *uid_str, const char *email)
{
    uint8_t qrybuf[MAX_MYSQL_QRYSZ];
    int len = 0;

    if (email) {
        /* Check if UID/E-Mail is already registered */
        PUSH_BUF(qrybuf, len, "SELECT * FROM UserRegInfo WHERE "
                 "UID='%s' OR Email='%s'", uid_str, email);
    } else {
        /* Check if UID is already registered */
        PUSH_BUF(qrybuf, len, "SELECT * FROM UserRegInfo WHERE UID='%s'",
                 uid_str);
    }
    qrybuf[len] = '\0';

    gweb_mysql_ping();

    return gweb_mysql_get_query_count(qrybuf);
}

int
gweb_mysql_handle_registration (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp)
{
    struct j2c_registration_msg *jrecord = &j2cmsg->registration;

    uint8_t qrybuf[MAX_MYSQL_QRYSZ], *db_qry1, *db_qry2;
    uint8_t uid_str[MAX_UID_STRSZ], utc_dt_str[MAX_DATETIME_STRSZ];
    int len = 0, ret;

    /* Get UID for this registration */
    gweb_app_get_uid_str(jrecord->fields[FIELD_REGISTRATION_PHONE],
                         jrecord->fields[FIELD_REGISTRATION_EMAIL],
                         uid_str);

    ret = gweb_mysql_check_uid_email((const char *)uid_str,
                               jrecord->fields[FIELD_REGISTRATION_EMAIL]);
    if (ret < 0) {
        gweb_mysql_prepare_response(JSON_C_REGISTRATION_RESP,
                                    GWEB_MYSQL_ERR_UNKNOWN,
                                    j2cresp);
        return MYSQL_STATUS_FAIL;
    } else if (ret > 0) {
        /* Duplicate record */
        gweb_mysql_prepare_response(JSON_C_REGISTRATION_RESP,
                                    GWEB_MYSQL_ERR_DUPLICATE,
                                    j2cresp);
        return MYSQL_STATUS_FAIL;
    }

    /* UID is not already registered, update database */
    len = 0;
    db_qry1 = &qrybuf[len];
    PUSH_BUF(qrybuf, len, "INSERT INTO UserRegInfo ");
    PUSH_BUF(qrybuf, len, "(UID, FirstName, LastName, Email, StartDate, ");
    PUSH_BUF(qrybuf, len, "Password) VALUES ");

    /* NOTE: Should this datetime be sent from the application
     * layer?
     */
    gweb_get_utc_datetime(utc_dt_str);

    PUSH_BUF(qrybuf, len, "('%s', '%s', '%s', '%s', '%s', '%s')",
             uid_str,
             jrecord->fields[FIELD_REGISTRATION_FNAME],
             jrecord->fields[FIELD_REGISTRATION_LNAME],
             jrecord->fields[FIELD_REGISTRATION_EMAIL],
             utc_dt_str,
             jrecord->fields[FIELD_REGISTRATION_PASSWORD]);

    qrybuf[len++] = '\0';

    db_qry2 = &qrybuf[len];
    PUSH_BUF(qrybuf, len, "INSERT INTO UserPhone ");
    PUSH_BUF(qrybuf, len, "(UID, PhoneType, Phone) VALUES ");

    /* NOTE: default phone type set to mobile */
    PUSH_BUF(qrybuf, len, "('%s', 'mobile', '%s')",
             uid_str,
             jrecord->fields[FIELD_REGISTRATION_PHONE]);
    qrybuf[len++] = '\0';

    gweb_mysql_start_transaction();

    if (mysql_query(g_mysql_ctx, db_qry1)) {
        goto __abort_transaction;
    }

    if (mysql_query(g_mysql_ctx, db_qry2)) {
        goto __abort_transaction;
    }

    gweb_mysql_commit_transaction();

    gweb_mysql_prepare_response(JSON_C_REGISTRATION_RESP,
                                GWEB_MYSQL_OK,
                                j2cresp);

    (*j2cresp)->registration.fields[FIELD_REGISTRATION_RESP_UID] =
                strndup(uid_str, strlen(uid_str));

    return MYSQL_STATUS_OK;

__abort_transaction:
    report_mysql_error_noclose(g_mysql_ctx);
    gweb_mysql_abort_transaction();
    gweb_mysql_prepare_response(JSON_C_REGISTRATION_RESP,
                                GWEB_MYSQL_ERR_UNKNOWN,
                                j2cresp);
    return MYSQL_STATUS_FAIL;
}

int
gweb_mysql_free_registration (j2c_resp_t *j2cresp)
{
    if (j2cresp) {
        struct j2c_registration_resp *resp = &j2cresp->registration;

        if (resp->fields[FIELD_REGISTRATION_RESP_UID])
            free(resp->fields[FIELD_REGISTRATION_RESP_UID]);
        free(j2cresp);
    }
    return MYSQL_STATUS_OK;
}

static const char *login_qry_fields[] = {
    [FIELD_LOGIN_RESP_CODE] = NULL,
    [FIELD_LOGIN_RESP_DESC] = NULL,
    [FIELD_LOGIN_RESP_UID] = "UserRegInfo.UID",
    [FIELD_LOGIN_RESP_FNAME] = "UserRegInfo.FirstName",
    [FIELD_LOGIN_RESP_LNAME] = "UserRegInfo.LastName",
    [FIELD_LOGIN_RESP_EMAIL] = "UserRegInfo.Email",
    [FIELD_LOGIN_RESP_PHONE] = "UserPhone.Phone",
    [FIELD_LOGIN_RESP_ADDRESS1] = "UserAddress.Address1",
    [FIELD_LOGIN_RESP_ADDRESS2] = "UserAddress.Address2",
    [FIELD_LOGIN_RESP_ADDRESS3] = "UserAddress.Address3",
    [FIELD_LOGIN_RESP_COUNTRY] = "UserAddress.Country",
    [FIELD_LOGIN_RESP_STATE] = "UserAddress.State",
    [FIELD_LOGIN_RESP_PINCODE] = "UserAddress.Pincode",
    /* NOTE: Multi-record, using below hack */
    [FIELD_LOGIN_RESP_FACEBOOK_HANDLE] = "UserSocialNetwork.NetworkType",
    [FIELD_LOGIN_RESP_TWITTER_HANDLE] = "UserSocialNetwork.NetworkHandle",
    [FIELD_LOGIN_RESP_AVATAR_URL] = "UserRegInfo.AvatarURL",
};

int
gweb_mysql_handle_login (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp)
{
    struct j2c_login_msg *jrecord = &j2cmsg->login;

    MYSQL_RES *result;

    uint8_t qrybuf[MAX_MYSQL_QRYSZ];
    int len = 0, fld, ret = MYSQL_STATUS_OK;

    PUSH_BUF(qrybuf, len, "SELECT ");

    for (fld = FIELD_LOGIN_RESP_CODE; fld < FIELD_LOGIN_RESP_MAX; fld++) {
	if (login_qry_fields[fld] == NULL)
	    PUSH_BUF(qrybuf, len, "NULL,");
	else
	    PUSH_BUF(qrybuf, len, "%s,", login_qry_fields[fld]);
    }

    len--; /* Strip leading ',' */

    PUSH_BUF(qrybuf, len, " FROM UserRegInfo "
	     "LEFT JOIN UserPhone USING(UID) "
	     "LEFT JOIN UserSocialNetwork USING(UID) "
	     "LEFT JOIN UserAddress USING(UID) "
	     "WHERE UserRegInfo.Email='%s' AND UserRegInfo.Password='%s'",
	     jrecord->fields[FIELD_LOGIN_EMAIL],
	     jrecord->fields[FIELD_LOGIN_PASSWORD]);
    qrybuf[len] = '\0';

    gweb_mysql_ping();

    if (mysql_query(g_mysql_ctx, qrybuf)) {
	report_mysql_error_noclose(g_mysql_ctx);
        goto __bail_out;
    }

    if ((result = mysql_store_result(g_mysql_ctx)) == NULL) {
	report_mysql_error_noclose(g_mysql_ctx);
        goto __bail_out;
    }

    if (mysql_num_rows(result) >= 1) {
        MYSQL_ROW row;

        if ((row = mysql_fetch_row(result)) == NULL) {
            mysql_free_result(result);
            goto __bail_out;
        }

        gweb_mysql_prepare_response(JSON_C_LOGIN_RESP,
                                    GWEB_MYSQL_OK,
                                    j2cresp);

	/* Fetch profiles first and then loop to fetch social network
	 * information.
	 */
	for (fld = FIELD_LOGIN_RESP_UID; fld < FIELD_LOGIN_RESP_MAX; fld++) {
	    if (fld == FIELD_LOGIN_RESP_FACEBOOK_HANDLE ||
		fld == FIELD_LOGIN_RESP_TWITTER_HANDLE) {
		continue;
	    }
	    if (row[fld]) {
		(*j2cresp)->login.fields[fld] =
		    strndup(row[fld], strlen(row[fld]));
	    }
	}

	do {
	    char *network_type = row[FIELD_LOGIN_RESP_FACEBOOK_HANDLE];
	    int handle_idx = FIELD_LOGIN_RESP_TWITTER_HANDLE;

	    if (network_type) {
		if (strcasecmp(network_type, "facebook") == 0) {
		    (*j2cresp)->login.fields[FIELD_LOGIN_RESP_FACEBOOK_HANDLE] =
			strndup(row[handle_idx], strlen(row[handle_idx]));

		} else if (strcasecmp(network_type, "twitter") == 0) {
		    (*j2cresp)->login.fields[FIELD_LOGIN_RESP_TWITTER_HANDLE] =
			strndup(row[handle_idx], strlen(row[handle_idx]));
		}
	    }
	} while ((row = mysql_fetch_row(result)));

    } else {
	gweb_mysql_prepare_response(JSON_C_LOGIN_RESP,
				    GWEB_MYSQL_ERR_NO_RECORD,
				    j2cresp);
	ret = MYSQL_STATUS_FAIL;
    }
    mysql_free_result(result);

    return ret;

__bail_out:
    gweb_mysql_prepare_response(JSON_C_LOGIN_RESP,
                                GWEB_MYSQL_ERR_UNKNOWN,
                                j2cresp);
    return MYSQL_STATUS_FAIL;
}

int
gweb_mysql_free_login (j2c_resp_t *j2cresp)
{
    int fld;

    if (j2cresp) {
        struct j2c_login_resp *resp = &j2cresp->login;

	for (fld = FIELD_LOGIN_RESP_UID; fld < FIELD_LOGIN_RESP_MAX; fld++) {
	    if (resp->fields[fld])
		free(resp->fields[fld]);
	}
        free(j2cresp);
    }
    return MYSQL_STATUS_OK;
}

int
gweb_mysql_handle_avatar (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp)
{
    struct j2c_avatar_msg *jrecord = &j2cmsg->avatar;

    uint8_t qrybuf[MAX_MYSQL_QRYSZ];
    int len = 0, ret;

    /* Check if UID is registered */
    ret = gweb_mysql_check_uid_email(jrecord->fields[FIELD_PROFILE_UID], NULL);
    if (ret <= 0) {
        report_mysql_error_noclose(g_mysql_ctx);
        goto __bail_out;
    }

    PUSH_BUF(qrybuf, len,
             "UPDATE UserRegInfo SET AvatarURL='%s' WHERE UID='%s'",
             jrecord->fields[FIELD_AVATAR_URL],
             jrecord->fields[FIELD_AVATAR_UID]);
    qrybuf[len] = '\0';

    gweb_mysql_ping();

    gweb_mysql_start_transaction();

    if (mysql_query(g_mysql_ctx, qrybuf)) {
        goto __abort_transaction;
    }

    gweb_mysql_commit_transaction();

    gweb_mysql_prepare_response(JSON_C_AVATAR_RESP,
                                GWEB_MYSQL_OK,
                                j2cresp);
    return MYSQL_STATUS_OK;

__abort_transaction:
    gweb_mysql_abort_transaction();
__bail_out:
    gweb_mysql_prepare_response(JSON_C_AVATAR_RESP,
                                GWEB_MYSQL_ERR_NO_RECORD,
                                j2cresp);
    return MYSQL_STATUS_FAIL;
}

int
gweb_mysql_free_avatar (j2c_resp_t *j2cresp)
{
    if (j2cresp) {
        free(j2cresp);
    }
    return MYSQL_STATUS_OK;
}

static int
gweb_mysql_query_social_network (const char *uid, const char *sn_type)
{
    uint8_t qrybuf[MAX_MYSQL_QRYSZ];
    int len = 0;

    PUSH_BUF(qrybuf, len, "SELECT UID from UserSocialNetwork WHERE UID='%s' "
             "AND NetworkType='%s'", uid, sn_type);
    qrybuf[len] = '\0';

    return gweb_mysql_get_query_count(qrybuf);
}

static int
gweb_mysql_update_social_network (char *qrybuf, struct j2c_profile_msg *jrecord,
                                  const char *sn_type, int field_idx, int found)
{
    int len = 0;

    if (strlen(jrecord->fields[field_idx]) == 0) {
        PUSH_BUF(qrybuf, len, "DELETE FROM UserSocialNetwork WHERE UID='%s' "
                 "AND NetworkType='%s'",
                 jrecord->fields[FIELD_PROFILE_UID],
                 sn_type);
    } else {
        if (found) {
            PUSH_BUF(qrybuf, len,
                     "UPDATE UserSocialNetwork SET NetworkHandle='%s' "
                     "WHERE UID='%s' AND NetworkType='%s'",
                     jrecord->fields[field_idx],
                     jrecord->fields[FIELD_PROFILE_UID],
                     sn_type);
        } else {
            PUSH_BUF(qrybuf, len,
                     "INSERT INTO UserSocialNetwork (UID, NetworkType, "
                     "NetworkHandle) VALUES ('%s', '%s', '%s')",
                     jrecord->fields[FIELD_PROFILE_UID],
                     sn_type,
                     jrecord->fields[field_idx]);
        }
    }
    qrybuf[len] = '\0';

    return len;
}

/* If the given string is "" then set the field to NULL, else update
 * the field
 */
#define STR_OR_NULL_QRY(fld, fldname)				\
    if (jrecord->fields[fld]) {					\
	if (strlen(jrecord->fields[fld]) == 0) {		\
	    PUSH_BUF(qrybuf, len, fldname "NULL,");		\
	} else {						\
	    PUSH_BUF(qrybuf, len, fldname "'%s',",		\
		     jrecord->fields[fld]);			\
	}							\
    }

#define STR_OR_NULL_INSERT_QRY(fld, fldname)	\
    STR_OR_NULL_QRY(fld, fldname)		\
    else {					\
	PUSH_BUF(qrybuf, len, "NULL,");		\
    }

int
gweb_mysql_handle_profile (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp)
{
    struct j2c_profile_msg *jrecord = &j2cmsg->profile;

    uint8_t qrybuf[MAX_MYSQL_QRYSZ], *db_qry1, *db_qry2, *db_qry3;
    int record_found = 0, sn_facebook_found = 0, sn_twitter_found = 0;
    int update_fields = 0;
    int len = 0, tmp_len, ret;
    int bail_out_err = GWEB_MYSQL_ERR_UNKNOWN;

    /* Check if UID is registered */
    ret = gweb_mysql_check_uid_email(jrecord->fields[FIELD_PROFILE_UID], NULL);
    if (ret <= 0) {
        report_mysql_error_noclose(g_mysql_ctx);
        bail_out_err = GWEB_MYSQL_ERR_NO_RECORD;
        goto __bail_out;
    }

    /*
     * NOTE: address type hardcoded as permanent for now
     */
    PUSH_BUF(qrybuf, len,
             "SELECT UID from UserAddress WHERE UID='%s' AND AddressType='%s'",
             jrecord->fields[FIELD_PROFILE_UID],
             "permanent");
    qrybuf[len] = '\0';

    gweb_mysql_ping();

    /* Check if the record exists */
    ret = gweb_mysql_get_query_count(qrybuf);
    if (ret < 0) {
        report_mysql_error_noclose(g_mysql_ctx);
        goto __bail_out;
    }
    record_found = ret;

    ret = gweb_mysql_query_social_network(jrecord->fields[FIELD_PROFILE_UID],
                                          "facebook");
    if (ret < 0) {
        report_mysql_error_noclose(g_mysql_ctx);
        goto __bail_out;
    }
    sn_facebook_found = ret;

    ret = gweb_mysql_query_social_network(jrecord->fields[FIELD_PROFILE_UID],
                                          "twitter");
    if (ret < 0) {
        report_mysql_error_noclose(g_mysql_ctx);
        goto __bail_out;
    }
    sn_twitter_found = ret;

    db_qry1 = db_qry2 = db_qry3 = NULL;

    /* Update record if exists or else, insert. Note that, we have a
     * case where there is no primary key in this table. Check for
     * other social network handles.
     */
    len = 0;

    /* Check if there is atleast one field to update */
    if (jrecord->fields[FIELD_PROFILE_ADDRESS1] ||
	jrecord->fields[FIELD_PROFILE_ADDRESS2] ||
	jrecord->fields[FIELD_PROFILE_ADDRESS3] ||
	jrecord->fields[FIELD_PROFILE_STATE] ||
	jrecord->fields[FIELD_PROFILE_PINCODE] ||
	jrecord->fields[FIELD_PROFILE_COUNTRY]) {
	update_fields = 1;
    }

    if (record_found && update_fields) {
	PUSH_BUF(qrybuf, len, "UPDATE UserAddress SET ");
	STR_OR_NULL_QRY(FIELD_PROFILE_ADDRESS1, "Address1=");
	STR_OR_NULL_QRY(FIELD_PROFILE_ADDRESS2, "Address2=");
	STR_OR_NULL_QRY(FIELD_PROFILE_ADDRESS3, "Address3=");
	STR_OR_NULL_QRY(FIELD_PROFILE_STATE, "State=");
	STR_OR_NULL_QRY(FIELD_PROFILE_PINCODE, "Pincode=");
	STR_OR_NULL_QRY(FIELD_PROFILE_COUNTRY, "Country=");
	len--; /* Strip leading ',' */
	PUSH_BUF(qrybuf, len, " WHERE UID='%s' AND AddressType='%s'",
		 jrecord->fields[FIELD_PROFILE_UID], "permanent");

	db_qry1 = &qrybuf[0];
	qrybuf[len++] = '\0';

    } else if (!record_found) {
        PUSH_BUF(qrybuf, len, "INSERT INTO UserAddress "
                 "(UID, AddressType, Address1, Address2, Address3, State, "
                 "Pincode, Country) VALUES ('%s', '%s', ",
		 jrecord->fields[FIELD_PROFILE_UID],
		 "permanent");
	STR_OR_NULL_INSERT_QRY(FIELD_PROFILE_ADDRESS1, "");
	STR_OR_NULL_INSERT_QRY(FIELD_PROFILE_ADDRESS2, "");
	STR_OR_NULL_INSERT_QRY(FIELD_PROFILE_ADDRESS3, "");
	STR_OR_NULL_INSERT_QRY(FIELD_PROFILE_STATE, "");
	STR_OR_NULL_INSERT_QRY(FIELD_PROFILE_PINCODE, "");
	STR_OR_NULL_INSERT_QRY(FIELD_PROFILE_COUNTRY, "");
	len--; /* Strip leading ',' */

	PUSH_BUF(qrybuf, len, ")");

	db_qry1 = &qrybuf[0];
	qrybuf[len++] = '\0';
    }

    if (jrecord->fields[FIELD_PROFILE_FACEBOOK_HANDLE]) {
        db_qry2 = &qrybuf[len];
        tmp_len = gweb_mysql_update_social_network(db_qry2, jrecord, "facebook",
                             FIELD_PROFILE_FACEBOOK_HANDLE, sn_facebook_found);
        len += tmp_len + 1;
    }

    if (jrecord->fields[FIELD_PROFILE_TWITTER_HANDLE]) {
        db_qry3 = &qrybuf[len];
        tmp_len = gweb_mysql_update_social_network(db_qry3, jrecord, "twitter",
                             FIELD_PROFILE_TWITTER_HANDLE, sn_twitter_found);
        len += tmp_len + 1;
    }

    /* Start transaction and push the updates */
    gweb_mysql_start_transaction();

    if (db_qry1 && mysql_query(g_mysql_ctx, db_qry1)) {
        goto __abort_transaction;
    }

    if (db_qry2 && mysql_query(g_mysql_ctx, db_qry2)) {
        goto __abort_transaction;
    }

    if (db_qry3 && mysql_query(g_mysql_ctx, db_qry3)) {
        goto __abort_transaction;
    }

    gweb_mysql_commit_transaction();

    gweb_mysql_prepare_response(JSON_C_PROFILE_RESP,
                                GWEB_MYSQL_OK,
                                j2cresp);
    return MYSQL_STATUS_OK;

__abort_transaction:
    report_mysql_error_noclose(g_mysql_ctx);
    gweb_mysql_abort_transaction();

__bail_out:
    gweb_mysql_prepare_response(JSON_C_PROFILE_RESP,
                                bail_out_err,
                                j2cresp);
    return MYSQL_STATUS_FAIL;
}

int
gweb_mysql_handle_cxn_request (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp)
{
    *j2cresp = NULL;

    return MYSQL_STATUS_OK;
}

int
gweb_mysql_handle_cxn_channel (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp)
{
    *j2cresp = NULL;

    return MYSQL_STATUS_OK;
}

/*
 * Given UID, fetch name and avatar for minimal profile display. Note
 * that the e-mail/phone or other details have not been fetched.
 */
static int
gweb_mysql_get_name_avatar_from_uid (const char *uid, char **fname,
                                     char **lname, char **avatar)
{
    MYSQL_RES *result;
    MYSQL_ROW row;

    uint8_t qrybuf[MAX_MYSQL_QRYSZ];
    int len = 0;

    /* TBD: LOG error */
    if (!uid || !fname || !lname || !avatar)
        return MYSQL_STATUS_FAIL;

    PUSH_BUF(qrybuf, len, "SELECT FirstName, LastName, AvatarURL FROM UserRegInfo "
             "WHERE UID='%s'", uid);
    qrybuf[len] = '\0';

    *fname = *lname = *avatar = NULL;

    if (mysql_query(g_mysql_ctx, qrybuf)) {
        report_mysql_error_noclose(g_mysql_ctx);
        return MYSQL_STATUS_FAIL;
    }

    if ((result = mysql_store_result(g_mysql_ctx)) == NULL) {
        report_mysql_error_noclose(g_mysql_ctx);
        return MYSQL_STATUS_FAIL;
    }

    /* *TBD* There should be only one row if there is a match. */
    if (mysql_num_rows(result) != 1) {
        mysql_free_result(result);
        return MYSQL_STATUS_FAIL;
    }

    if ((row = mysql_fetch_row(result)) == NULL) {
        report_mysql_error_noclose(g_mysql_ctx);
        mysql_free_result(result);
        return MYSQL_STATUS_FAIL;
    }

    if (row[0])
        *fname = strndup(row[0], strlen(row[0])); /* FirstName */
    if (row[1])
        *lname = strndup(row[1], strlen(row[1])); /* LastName */
    if (row[2])
        *avatar = strndup(row[2], strlen(row[2])); /* AvatarURL */

    mysql_free_result(result);

    return MYSQL_STATUS_OK;
}

#define CXN_OUTBOUND   (1)
#define CXN_INBOUND    (2)

#define MYSQL_MAX_CXN_REQUEST_ROWS_PER_QUERY   (10)
#define MYSQL_MAX_CXN_CHANNEL_ROWS_PER_QUERY   (10)

#define CXN_REQ_IDX(x)   \
   ((FIELD_CXN_REQUEST_QUERY_RESP_##x) - FIELD_CXN_REQUEST_QUERY_RESP_ARRAY_START - 1)

#define CXN_CHNL_IDX(x)                                                 \
    ((FIELD_CXN_CHANNEL_QUERY_RESP_##x) - FIELD_CXN_CHANNEL_QUERY_RESP_ARRAY_START - 1)

int
gweb_mysql_handle_cxn_request_query (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp)
{
    int match_count, direction = 0;
    int max_rows, idx, rowid = 0, len = 0;
    int err, ret = MYSQL_STATUS_FAIL;
    uint8_t qrybuf[MAX_MYSQL_QRYSZ], buf[32];

    char *fname = NULL, *lname = NULL, *avatar = NULL;
    const char *uid = NULL;

    MYSQL_RES *result;
    MYSQL_ROW row;

    struct j2c_cxn_request_query_msg *jrecord = &j2cmsg->cxn_request_query;
    struct j2c_cxn_request_query_resp *resp = NULL;
    struct j2c_cxn_request_query_resp_array1 *arr = NULL;

    /* Allocate for response */
    gweb_mysql_prepare_response(JSON_C_CXN_REQUEST_QUERY_RESP,
                                GWEB_MYSQL_OK,
                                j2cresp);
    resp = &((*j2cresp)->cxn_request_query);
    resp->nr_array1_records = -1; /* No records */

    PUSH_BUF(qrybuf, len, "SELECT FromUID, ToUID, SentOn, Flags FROM UserConnectRequest "
             "WHERE TRUE ");

    if (jrecord->fields[FIELD_CXN_REQUEST_QUERY_FROM_UID]) {
        uid = jrecord->fields[FIELD_CXN_REQUEST_QUERY_FROM_UID];
        direction = CXN_OUTBOUND;
        PUSH_BUF(qrybuf, len, "AND FromUID='%s' ", uid);

    } else if (jrecord->fields[FIELD_CXN_REQUEST_QUERY_TO_UID]) {
        uid = jrecord->fields[FIELD_CXN_REQUEST_QUERY_TO_UID];
        direction = CXN_INBOUND;
        PUSH_BUF(qrybuf, len, "AND ToUID='%s' ", uid);
    }

    if (!direction || !gweb_mysql_check_uid_email(uid, NULL)) {
        err = GWEB_MYSQL_ERR_NO_RECORD;
        goto __bail_out;
    }

    if (jrecord->fields[FIELD_CXN_REQUEST_QUERY_FLAG]) {
        PUSH_BUF(qrybuf, len, "AND Flags='%s' ",
                 jrecord->fields[FIELD_CXN_REQUEST_QUERY_FLAG]);
    }

    qrybuf[len] = '\0';

    gweb_mysql_ping();

    if (mysql_query(g_mysql_ctx, qrybuf)) {
        report_mysql_error_noclose(g_mysql_ctx);
        goto __bail_out;
    }

    if ((result = mysql_store_result(g_mysql_ctx)) == NULL) {
        report_mysql_error_noclose(g_mysql_ctx);
        err = GWEB_MYSQL_ERR_UNKNOWN;
        goto __bail_out;
    }

    match_count = mysql_num_rows(result);
    if (match_count == 0) {
        goto __send_record;
    }

    /* *TBD* For easier access, dump all requested data to a file in
     * JSON format and upload.
     */
    max_rows = (match_count >= MYSQL_MAX_CXN_REQUEST_ROWS_PER_QUERY) ?
        MYSQL_MAX_CXN_REQUEST_ROWS_PER_QUERY: match_count;

    resp->array1 = calloc(max_rows, sizeof(struct j2c_cxn_request_query_resp_array1));
    if (!resp->array1) {
        err = GWEB_MYSQL_ERR_NO_MEMORY;
        goto __bail_out;
    }

    for (rowid = idx = 0; idx < match_count && rowid < max_rows; idx++) {
        if ((row = mysql_fetch_row(result)) == NULL) {
            err = GWEB_MYSQL_ERR_UNKNOWN;
            goto __bail_out;
        }

        if (!row[0] || !row[1]) { /* FromUID || ToUID */
            /* Skip if there is no FromUID or ToUID */
            continue;
        }

        /* NOTE: Number of rows in response could be lesser than max_rows */
        arr = &resp->array1[rowid++];

        if (direction == CXN_INBOUND) {
            arr->fields[CXN_REQ_IDX(UID)] = strndup(row[0], strlen(row[0]));
        } else {
            arr->fields[CXN_REQ_IDX(UID)] = strndup(row[1], strlen(row[1]));
        }
        if (row[2]) { /* SentOn */
            arr->fields[CXN_REQ_IDX(DATE)] = strndup(row[2], strlen(row[2]));
        }
        if (row[3]) { /* Flags */
            arr->fields[CXN_REQ_IDX(FLAG)] = strndup(row[3], strlen(row[3]));
        }

        if (gweb_mysql_get_name_avatar_from_uid(arr->fields[CXN_REQ_IDX(UID)],
                           &fname, &lname, &avatar) != MYSQL_STATUS_OK) {
            free(arr->fields[CXN_REQ_IDX(UID)]);
            free(arr->fields[CXN_REQ_IDX(DATE)]);
            rowid--;
            continue;
        }

        if (fname) {
            arr->fields[CXN_REQ_IDX(FNAME)] = fname;
        }
        if (lname) {
            arr->fields[CXN_REQ_IDX(LNAME)] = lname;
        }
        if (avatar) {
            arr->fields[CXN_REQ_IDX(AVATAR_URL)] = avatar;
        }
    }

__send_record:
    sprintf(buf, "%d", match_count);
    resp->fields[FIELD_CXN_REQUEST_QUERY_RESP_RECORD_COUNT] = strndup(buf, strlen(buf));
    resp->nr_array1_records = rowid;

    err = GWEB_MYSQL_OK;
    ret = MYSQL_STATUS_OK;

__bail_out:
    mysql_free_result(result);

    gweb_mysql_update_response(JSON_C_CXN_REQUEST_QUERY_RESP,
                               err,
                               j2cresp);
    return ret;
}

int
gweb_mysql_handle_cxn_channel_query (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp)
{
    int match_count, direction = 0;
    int max_rows, idx, rowid = 0, len = 0;
    int err, ret = MYSQL_STATUS_FAIL;
    uint8_t qrybuf[MAX_MYSQL_QRYSZ], buf[32];

    char *fname = NULL, *lname = NULL, *avatar = NULL;
    const char *uid = NULL;

    MYSQL_RES *result;
    MYSQL_ROW row;

    struct j2c_cxn_channel_query_msg *jrecord = &j2cmsg->cxn_channel_query;
    struct j2c_cxn_channel_query_resp *resp = NULL;
    struct j2c_cxn_channel_query_resp_array1 *arr = NULL;

    gweb_mysql_prepare_response(JSON_C_CXN_CHANNEL_QUERY_RESP,
                                GWEB_MYSQL_OK,
                                j2cresp);
    resp = &(*j2cresp)->cxn_channel_query;
    resp->nr_array1_records = -1; /* No records */

    PUSH_BUF(qrybuf, len, "SELECT FromUID, ToUID, ConnectedOn, ChannelId "
             "FROM UserConnectChannel WHERE TRUE ");

    if (jrecord->fields[FIELD_CXN_CHANNEL_QUERY_FROM_UID]) {
        uid = jrecord->fields[FIELD_CXN_CHANNEL_QUERY_FROM_UID];
        direction = CXN_OUTBOUND;
        PUSH_BUF(qrybuf, len, "AND FromUID='%s' ", uid);

    } else if (jrecord->fields[FIELD_CXN_CHANNEL_QUERY_TO_UID]) {
        uid = jrecord->fields[FIELD_CXN_CHANNEL_QUERY_TO_UID];
        direction = CXN_INBOUND;
        PUSH_BUF(qrybuf, len, "AND ToUID='%s' ", uid);
    }

    if (!direction || !gweb_mysql_check_uid_email(uid, NULL)) {
        err = GWEB_MYSQL_ERR_NO_RECORD;
        goto __bail_out;
    }

    if (jrecord->fields[FIELD_CXN_CHANNEL_QUERY_TYPE]) {
        PUSH_BUF(qrybuf, len, "AND ChannelId='%s' ",
                 jrecord->fields[FIELD_CXN_CHANNEL_QUERY_TYPE]);
    }

    qrybuf[len] = '\0';

    gweb_mysql_ping();

    if (mysql_query(g_mysql_ctx, qrybuf)) {
        report_mysql_error_noclose(g_mysql_ctx);
        goto __bail_out;
    }

    if ((result = mysql_store_result(g_mysql_ctx)) == NULL) {
        report_mysql_error_noclose(g_mysql_ctx);
        err = GWEB_MYSQL_ERR_UNKNOWN;
        goto __bail_out;
    }

    match_count = mysql_num_rows(result);
    if (match_count == 0) {
        goto __send_record;
    }

    /* *TBD* For easier access, dump all requested data to a file in
     * JSON format and upload.
     */
    max_rows = (match_count >= MYSQL_MAX_CXN_CHANNEL_ROWS_PER_QUERY) ?
        MYSQL_MAX_CXN_CHANNEL_ROWS_PER_QUERY: match_count;

    resp->array1 = calloc(sizeof(struct j2c_cxn_channel_query_resp_array1),
                          max_rows);
    if (!resp->array1) {
        err = GWEB_MYSQL_ERR_NO_MEMORY;
        goto __bail_out;
    }

    for (rowid = idx = 0; idx < match_count && rowid < max_rows; idx++) {
        if ((row = mysql_fetch_row(result)) == NULL) {
            err = GWEB_MYSQL_ERR_UNKNOWN;
            goto __bail_out;
        }
        if (!row[0] || !row[1]) { /* ToUID */
            /* Skip if there is no ToUID */
            continue;
        }

        /* NOTE: Number of rows in response could be lesser than max_rows */
        arr = &resp->array1[rowid++];

        if (direction == CXN_INBOUND) {
            arr->fields[CXN_CHNL_IDX(UID)] = strndup(row[0], strlen(row[0]));
        } else {
            arr->fields[CXN_CHNL_IDX(UID)] = strndup(row[1], strlen(row[1]));
        }

        if (row[2]) { /* SentOn */
            arr->fields[CXN_CHNL_IDX(DATE)] = strndup(row[2], strlen(row[2]));
        }

        if (row[3]) { /* Type */
            arr->fields[CXN_CHNL_IDX(CHANNEL_TYPE)] = strndup(row[3], strlen(row[3]));
        }

        if (gweb_mysql_get_name_avatar_from_uid(arr->fields[CXN_CHNL_IDX(UID)],
                           &fname, &lname, &avatar) != MYSQL_STATUS_OK) {
            free(arr->fields[CXN_CHNL_IDX(UID)]);
            free(arr->fields[CXN_CHNL_IDX(DATE)]);
            rowid--;
            continue;
        }
        if (fname) {
            arr->fields[CXN_CHNL_IDX(FNAME)] = fname;
        }
        if (lname) {
            arr->fields[CXN_CHNL_IDX(LNAME)] = lname;
        }
        if (avatar) {
            arr->fields[CXN_CHNL_IDX(AVATAR_URL)] = avatar;
        }
    }

__send_record:
    sprintf(buf, "%d", match_count);
    resp->fields[FIELD_CXN_CHANNEL_QUERY_RESP_RECORD_COUNT] = strndup(buf, strlen(buf));
    resp->nr_array1_records = rowid;

    err = GWEB_MYSQL_OK;
    ret = MYSQL_STATUS_OK;

 __bail_out:
    mysql_free_result(result);

    gweb_mysql_update_response(JSON_C_CXN_CHANNEL_QUERY_RESP,
                               err,
                               j2cresp);
    return ret;
}

int
gweb_mysql_free_cxn_request (j2c_resp_t *j2cresp)
{
    if (j2cresp) {
        free(j2cresp);
    }
    return MYSQL_STATUS_OK;
}

int
gweb_mysql_free_cxn_channel (j2c_resp_t *j2cresp)
{
    if (j2cresp) {
        free(j2cresp);
    }
    return MYSQL_STATUS_OK;
}

#define J2CRESP_FREE_ARRAY(arr, nr, maxfld)             \
    do {                                                \
        int idx, fld;                                   \
        if (arr) {                                      \
            for (idx = 0; idx < nr; idx++) {            \
                for (fld = 0; fld < maxfld; fld++) {    \
                    if (arr[idx].fields[fld]) {         \
                        free(arr[idx].fields[fld]);     \
                    }                                   \
                }                                       \
            }                                           \
            free(arr);                                  \
        }                                               \
    } while(0)

int
gweb_mysql_free_cxn_request_query (j2c_resp_t *j2cresp)
{
    struct j2c_cxn_request_query_resp *resp = NULL;

    if (j2cresp) {
        resp = &j2cresp->cxn_request_query;
        J2CRESP_FREE_ARRAY(resp->array1, resp->nr_array1_records,
                           CXN_REQ_IDX(ARRAY_END));
        free(resp->fields[FIELD_CXN_REQUEST_QUERY_RESP_RECORD_COUNT]);
        free(j2cresp);
    }
    return MYSQL_STATUS_OK;
}

int
gweb_mysql_free_cxn_channel_query (j2c_resp_t *j2cresp)
{
    struct j2c_cxn_channel_query_resp *resp = NULL;

    if (j2cresp) {
        resp = &j2cresp->cxn_channel_query;
        J2CRESP_FREE_ARRAY(resp->array1, resp->nr_array1_records,
                           CXN_CHNL_IDX(ARRAY_END));
        free(resp->fields[FIELD_CXN_CHANNEL_QUERY_RESP_RECORD_COUNT]);
        free(j2cresp);
    }
    return MYSQL_STATUS_OK;
}

int
gweb_mysql_free_profile (j2c_resp_t *j2cresp)
{
    if (j2cresp) {
        free(j2cresp);
    }
    return MYSQL_STATUS_OK;
}

int
gweb_mysql_shutdown (void)
{
    log_debug("Closing MySQL connection!\n");

    if (g_mysql_ctx)
	mysql_close(g_mysql_ctx);

    return MYSQL_STATUS_OK;
}

static int
gweb_mysql_connect (MYSQL *ctx)
{
    if (!ctx) {
        return MYSQL_STATUS_FAIL;
    }

    if (mysql_real_connect(ctx,
                           MYSQL_DB_HOST,
                           MYSQL_DB_USER,
                           MYSQL_DB_PASSWORD,
                           MYSQL_DB_NAMESPACE,
                           0,
                           NULL,
                           CLIENT_MULTI_STATEMENTS) == NULL) {
        return MYSQL_STATUS_FAIL;
    }

    return MYSQL_STATUS_OK;
}

/*
 * Check / reconnect MySQL connection
 */
int
gweb_mysql_ping (void)
{
    unsigned long thid_before_ping, thid_after_ping;

    thid_before_ping = mysql_thread_id(g_mysql_ctx);
    mysql_ping(g_mysql_ctx);
    thid_after_ping = mysql_thread_id(g_mysql_ctx);

    if (thid_before_ping != thid_after_ping) {
        log_debug("%s: MySQL reconnected!\n", __func__);
    }

    return MYSQL_STATUS_OK;
}

int
gweb_mysql_init (void)
{
    my_bool reconnect = 1;

    log_debug("Initializing schema, MySQL version = %s\n",
	      mysql_get_client_info());

    if ((g_mysql_ctx = mysql_init(NULL)) == NULL) {
	report_mysql_error(g_mysql_ctx);
    }

    mysql_options(g_mysql_ctx, MYSQL_OPT_RECONNECT, &reconnect);

    if (gweb_mysql_connect(g_mysql_ctx) != MYSQL_STATUS_OK) {
        report_mysql_error(g_mysql_ctx);
    }

    log_debug("MySQL connected!\n");

    return MYSQL_STATUS_OK;
}

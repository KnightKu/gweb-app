#ifndef MYSQLDB_API_H
#define MYSQLDB_API_H

#include <gweb/json_struct.h>

#define MAX_MYSQL_QRYSZ    1024

/*
 * MySQL APIs for queries
 */
extern int gweb_mysql_handle_registration (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp);
extern int gweb_mysql_handle_login (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp);
extern int gweb_mysql_handle_profile (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp);
extern int gweb_mysql_handle_avatar (j2c_msg_t *j2cmsg, j2c_resp_t **j2cresp);

extern int gweb_mysql_free_registration (j2c_resp_t *j2cresp);
extern int gweb_mysql_free_login (j2c_resp_t *j2cresp);
extern int gweb_mysql_free_profile (j2c_resp_t *j2cresp);
extern int gweb_mysql_free_avatar (j2c_resp_t *j2cresp);

extern int gweb_mysql_ping (void);
extern int gweb_mysql_init (void);
extern int gweb_mysql_shutdown (void);
			   
#endif // MYSQLDB_API_H

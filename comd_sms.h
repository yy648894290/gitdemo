#ifndef _SMS_H
#define _SMS_H 1

struct LIST_SMS
{
    unsigned int index;
    int tag;
    char time_info[64];
    char sender_number[128];
    char user_data[4096];
};

void initSmsSqlite3 (void);
void long_message_table_check(void);
int sms_push_in_sqlite3 ( int init_type );

#endif

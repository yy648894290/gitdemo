#include <stdint.h>
#include "comd_share.h"
#include "comd_sms.h"
#include "mtk_config_response.h"

#define ALL_UNLOCK_PHONE_NUM (sizeof(unlock_phone_numbers) / sizeof(unlock_phone_numbers[0]))

char *unlock_phone_numbers[] = {
"900", "909", "907", "+900", "+909", "+907",
};

extern int allSms_sem_id;
extern char pinlock_enable[4];
extern char simlock_enable[4];
extern char celllock_enable[4];
extern COMD_DEV_INFO at_dev;


static int get_index(char* buffer)
{
    char buf[256] = {0};
    char *p, *p1;

    strncpy(buf, buffer, sizeof(buf)-1);

    p = strstr(buf, "+CMGL: ");
    if(p)
    {
        p += strlen("+CMGL: ");
        p1 = strstr(p, ",");
        if(p1)
        {
            *p1 = 0;
        }
        else
        {
            return -1;
        }

        return atoi(p);
    }
    else
    {
        return -1;
    }
}

static int get_tag(char* buffer)
{
    if(strstr(buffer, SMS_READ_TXT) || strstr(buffer, SMS_READ_TXT_QUECTEL_T750))
    {
        return SMS_READ_TAG;
    }
    else if(strstr(buffer, SMS_UNREAD_TXT) || strstr(buffer, SMS_UNREAD_TXT_QUECTEL_T750))
    {
        return SMS_UNREAD_TAG;
    }
    else if(strstr(buffer, SMS_SENT_TXT) || strstr(buffer, SMS_SENT_TXT_QUECTEL_T750))
    {
        return SMS_SENT_TAG;
    }
    else if(strstr(buffer, SMS_UNSENT_TXT) || strstr(buffer, SMS_UNSENT_TXT_QUECTEL_T750))
    {
        return SMS_UNSENT_TAG;
    }
    else
    {
        return -1;
    }
}

int analysis_number(char *buffer,char *sender_number,int sender_number_attribute,int sender_addressLength)
{
    char *p,*q=NULL;
    char tmp_buffer;

    p=buffer;
    q=buffer+1;

    while(*p!='\0')
    {
        tmp_buffer=*p;
        *p=*q;
        *q=tmp_buffer;
        p=p+2;

        if(*p=='\0')
            break;
        q=p;
        q++;
    }

    if(sender_number_attribute==0)
    {
        p--;
        *p='\0';
    }
    else
    {
        *p='\0';
    }
    strcpy(sender_number,buffer);
    return 0;
}

int analysis_time(char *buffer,char *time_buffer)
{
    //int i,j=0;
    char *p,*q=NULL;
    char tmp_buffer;

    p=buffer;
    q=buffer+1;

    while(*p!='\0')
    {
        tmp_buffer=*p;
        *p=*q;
        *q=tmp_buffer;
        p=p+2;

        if(*p=='\0')
            break;
        q=p;
        q++;
    }

    *p='\0';
/*
    p=buffer;
    q=buffer+4;
    tmp_buffer=*p;
    *p=*q;
    *q=tmp_buffer;

    p=buffer+1;
    q=buffer+5;
    tmp_buffer=*p;
    *p=*q;
    *q=tmp_buffer;
*/

    strcpy(time_buffer,buffer);
    return 0;
}

int sms_transmit_push_in_sqlite3 ( char* data , int type)
{
    int ret=0;
    int tmp_number=0;
    struct LIST_SMS lsms;
    char* content;
    content=data;
    char tmp_buffer[128] = {0};
    int long_message=0;
    int firstOctet_SMSDeliver=0;
    int sender_addressLength=0;
    int sender_number_attribute=0; //0奇数 1偶数

    char sql[4096]={0};
    char *errmsg=NULL;

    char prefix[8]={0};
    int prefix_tag=0;
    char number_tmp[64]={0};


    sqlite3 *db;
    ret = sqlite3_open(SMS_DIR,&db);
    if(ret != SQLITE_OK)//打开成功返回SQLITE_OK
    {
        perror("sqlite open :");
        return -1;
    }

    if(type==1)
        lsms.tag=2;
    else
        lsms.tag=3;


    lib_str_intercept(content,tmp_buffer,0,1);

    tmp_number=(2*lib_hexToDec(tmp_buffer)+2);


    memset(tmp_buffer,0,sizeof(tmp_buffer));


    firstOctet_SMSDeliver=lib_hexToDec(tmp_buffer);  //firstOctet_SMSDeliver

    if( (firstOctet_SMSDeliver & 0x40) !=0 )
    {
        long_message=1;
    }
    else
    {
        long_message=0;
    }

    tmp_number=tmp_number+4;
    memset(tmp_buffer,0,sizeof(tmp_buffer));

    lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+1);

    sender_addressLength=lib_hexToDec(tmp_buffer);  //sender_addressLength
    if(sender_addressLength%2 != 0)
    {
        sender_addressLength +=1;
        sender_number_attribute=0;
    }
    else
    {
        sender_number_attribute=1;
    }

    tmp_number=tmp_number+2;
    memset(tmp_buffer,0,sizeof(tmp_buffer));
    lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+1);
    if(!strcmp(tmp_buffer,"91"))
    {
        prefix_tag=1;
    }
    else
    {
        prefix_tag=0;
    }

    tmp_number=tmp_number+2;
    memset(tmp_buffer,0,sizeof(tmp_buffer));
    lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+sender_addressLength-1);


    memset(number_tmp,0,sizeof(number_tmp));
    analysis_number(tmp_buffer,number_tmp,sender_number_attribute,sender_addressLength);

    memset(prefix,0,sizeof(prefix));
    if(prefix_tag==1)
    {
        if(0==strncasecmp(number_tmp,"234",3))
        {
            strcpy(prefix,"+234");
            lib_str_intercept(number_tmp,lsms.sender_number,3,-1);
        }
        else if(0==strncasecmp(number_tmp,"86",2))
        {
            strcpy(prefix,"+86");
            lib_str_intercept(number_tmp,lsms.sender_number,2,-1);
        }
        else if(0==strncasecmp(number_tmp,"56",2))
        {
            strcpy(prefix,"+56");
            lib_str_intercept(number_tmp,lsms.sender_number,2,-1);
        }
        else
        {
            strcpy(lsms.sender_number,number_tmp);
        }

    }
    else
    {
        strcpy(lsms.sender_number,number_tmp);
    }

    tmp_number=tmp_number+sender_addressLength+6;
    //lib_str_intercept(content,lsms.user_data,tmp_number,-1);
    strcpy(lsms.user_data,content);

    struct timeval tv;
    gettimeofday(&tv,NULL);
    uint64_t sec=tv.tv_sec;
    struct tm cur_tm;
    localtime_r((time_t*)&sec,&cur_tm);
    char cur_time[20];
    char *sqlite_time;
    sprintf(cur_time,"%d%02d%02d%02d%02d%02d",cur_tm.tm_year+1900,cur_tm.tm_mon+1,cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min,cur_tm.tm_sec);
    sqlite_time=cur_time;
    sqlite_time=sqlite_time+2;
    strcpy(lsms.time_info,sqlite_time);

    memset(sql,0,sizeof(sql));
    sprintf(sql,"insert into sms_flash values (NULL,'%s','%s','%d','%s','%s');",lsms.sender_number,lsms.user_data,lsms.tag,lsms.time_info,prefix);

    if(SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,&errmsg))//判断是否插入成功成功返回SQLITE_OK
    {
        CLOGD(FINE, "fail:%s\n",errmsg);
        CLOGD(FINE, "\n");

    }

    if(errmsg)
    {
        sqlite3_free(errmsg);
        errmsg=NULL;
    }
    sqlite3_close(db);

    return 0;
}

static int DCS_Bits(char *tp_DCS)
{
    int AlphabetSize = 7; // Set Default
    int pomDCS = lib_hexToDec(tp_DCS);

    switch (pomDCS & 192)
    {
    case 0:
        if (pomDCS & 32)
        {
            CLOGD(FINE, "Compressed Text\n");
        }
        else
        {
            CLOGD(FINE, "Uncompressed Text\n");
        }
        switch (pomDCS & 12)
        {
        case 4:
            AlphabetSize = 8;
            break;
        case 8:
            AlphabetSize = 16;
            break;
        }
        break;
    case 192:
        switch (pomDCS & 0x30)
        {
        case 0x20:
            AlphabetSize = 16;
            break;
        case 0x30:
            if (pomDCS & 0x4)
            {
                CLOGD(FINE, "Nothing here ...\n");
            }
            else
            {
                AlphabetSize = 8;
            }
            break;
        }
        break;
    }

    return (AlphabetSize);
}

void sms_setSimLockEntry( char* unlock_code )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_setSimLock ( unlock_code );
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            break;
    }
}

void sms_setPinLockEntry( char* unlock_code )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_setPinLock ( unlock_code );
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            break;
    }
}

void sms_setCellLockEntry( char* unlock_code )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_setCellLock ( unlock_code );
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            break;
    }
}

static int unlockPhoneNumberMatched(char *phone_number)
{
    int i = 0;

    for ( ; i < ALL_UNLOCK_PHONE_NUM; i++ )
    {
        if (0 == strcasecmp(unlock_phone_numbers[i], phone_number))
        {
            CLOGD(FINE, "Match unlock phone_number: [%s]\n", phone_number);
            return 1;
        }
    }

    CLOGD(FINE, "Not unlock phone_number.\n");
    return 0;
}

static int checkIfSmsMatchUnlockCode(int bit_size, char *user_pdu_data)
{
    char user_data[512] = {0};
    char code_data[4][16];

    CLOGD(FINE, "user_pdu_data: [%s]\n", user_pdu_data);
    CLOGD(FINE, "bit_size: [%d]\n", bit_size);

    switch (bit_size)
    {
        case 7:
            decodeGsm7bitPdu(user_data, user_pdu_data, sizeof(user_data));
            break;
        case 8:
            HexToStr(user_data, user_pdu_data, sizeof(user_data));
            break;
        case 16:
            unicode2chars(user_pdu_data, user_data, sizeof(user_data));
            break;
    }

    CLOGD(FINE, "user_data:%s\n", user_data);

    if(strcmp(simlock_enable,"0") != 0)
    {
        nv_get ( "unlock_code1", code_data[0], sizeof ( code_data[0] ) );
        if ( strstr ( user_data, code_data[0] ) )
        {
            CLOGD(FINE, "match unlock_code1, disable sim lock\n");
            sms_setSimLockEntry("10,00000000");
        }
    }

    if(strcmp(pinlock_enable,"0") != 0)
    {
        nv_get ( "unlock_code2", code_data[1], sizeof ( code_data[1] ) );
        if ( strstr ( user_data, code_data[1] ) )
        {
            CLOGD(FINE, "match unlock_code2, disable pin cell\n");
            sms_setPinLockEntry("10,00000000");
        }
    }

    if(strcmp(celllock_enable,"0") != 0)
    {
        nv_get ( "unlock_code3", code_data[2], sizeof ( code_data[2] ) );
        if ( strstr ( user_data, code_data[2] ) )
        {
            CLOGD(FINE, "match unlock_code3, reset cell lock list\n");
            sms_setCellLockEntry("12,00000000");
        }

        nv_get ( "unlock_code4", code_data[3], sizeof ( code_data[3] ) );
        if ( strstr ( user_data, code_data[3] ) )
        {
            CLOGD(FINE, "match unlock_code4, disable cell lock\n");
            sms_setCellLockEntry("10,00000000");
        }
    }

    return 0;
}

int sms_push_in_sqlite3 ( int init_type ) //init_type=0:init 1:add in the last
{
    char next = '+';
    char str[2048] = {0};
    int ret;
    char str_sca[2048] = {0};
    struct LIST_SMS lsms;
    char tmp_buffer[128] = {0};
    int long_message=0;
    int firstOctet_SMSDeliver=0;
    int sender_addressLength=0;
    int sender_number_attribute=0; //0奇数 1偶数
    int tmp_number=0;
    char* content;
    char sql[4096]={0};
    char *errmsg=NULL;
    FILE *fp = NULL;
    char cmd_buffer[64]={0};
    char open_file_name[64]={0};
    char long_message_id[8]={0};
    int long_message_max=0;
    int long_message_seq=0;
    char sql_long[4096]={0};
    char *errmsg_long=NULL;
    char prefix[8]={0};
    int prefix_tag=0;
    char number_tmp[64]={0};
    char **aresult=NULL;
    int nrow;
    int ncol;
    int bit_size=0;
    char user_pdu_data[512]={0};

    if(init_type==0)
    {
        strcpy(open_file_name,"/tmp/allSms.txt");
    }
    else
    {
        strcpy(open_file_name,"/tmp/unreadSms.txt");
    }

    CLOGD(FINE, " \n");
    sprintf(cmd_buffer, "dos2unix %s",open_file_name);
    system(cmd_buffer);

    if ( NULL == ( fp = fopen ( open_file_name, "r" ) ) )
    {
        comd_semaphore_v ( allSms_sem_id );
        return -1;
    }

    memset(&lsms,0,sizeof(lsms));

    sqlite3 *db;
    ret = sqlite3_open(SMS_DIR,&db);
    if(ret != SQLITE_OK)//打开成功返回SQLITE_OK
    {
        perror("sqlite open :");
        fclose ( fp );
        return -1;
    }

    strcpy(sql,"create table if not exists sms_flash(idx INTEGER PRIMARY KEY,number text,user_data text,status integer,time text,prefix text);");
    if(SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,&errmsg))//判断是否成功成功返回SQLITE_OK
    {
        CLOGD(FINE, "fail:%s\n", errmsg);
        CLOGD(FINE, "\n");
        if(errmsg)
        {
            sqlite3_free(errmsg);
            errmsg=NULL;
        }
        sqlite3_close(db);
        fclose ( fp );
        return -1;
    }

    strcpy(sql_long,"create table if not exists sms_long_message(number text,user_data text,status integer,time text,idx text,max integer,seq integer,timeout_new integer,prefix text);");
    if(SQLITE_OK != sqlite3_exec(db,sql_long,NULL,NULL,&errmsg_long))//判断是否成功成功返回SQLITE_OK
    {
        CLOGD(FINE, "fail:%s\n", errmsg_long);
        CLOGD(FINE, "\n");
        if(errmsg_long)
        {
            sqlite3_free(errmsg_long);
            errmsg_long=NULL;
        }
        sqlite3_close(db);
        fclose ( fp );
        return -1;
    }

    if(init_type==0)
    {

        strcpy(sql,"delete from sms_flash;");
        sqlite3_exec(db,sql,NULL,NULL,&errmsg);
        if(errmsg)
        {
            sqlite3_free(errmsg);
            errmsg=NULL;
        }
    }

    while ( fgets ( str, sizeof ( str ), fp ) )
    {
        if(next == '+' && *str == '+')
        {
            ret = get_index(str);
            if(ret < 0)
            {
                continue;
            }
            lsms.index = (unsigned int)ret;

            ret = get_tag(str);
            if(ret < 0)
            {
                continue;
            }
            lsms.tag = ret;
            next = 'p';
        }
        else if(next == 'p'
                && (( *str >= '0' && *str <= '9' ) || ( *str >= 'a' && *str <= 'f' ) || ( *str >= 'A' && *str <= 'F' )))
        {
            if(ret >= 0)
            {
                sprintf(str_sca, "%s", str);

                str_sca[strlen(str_sca)-1] = 0;//del \n
                content = strdup(str_sca);
            }

            lib_str_intercept(content,tmp_buffer,0,1);

            tmp_number=(2*lib_hexToDec(tmp_buffer)+2);

            memset(tmp_buffer,0,sizeof(tmp_buffer));
            lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+1);

            firstOctet_SMSDeliver=lib_hexToDec(tmp_buffer);  //firstOctet_SMSDeliver

            if( (firstOctet_SMSDeliver & 0x40) !=0 )
            {
                long_message=1;
            }
            else
            {
                long_message=0;
            }

            if( lsms.tag==2 || lsms.tag==3 )  // Transmit Message
            {
                CLOGD(FINE, "Transmit Message\n");
                tmp_number=tmp_number+4;
                memset(tmp_buffer,0,sizeof(tmp_buffer));

                lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+1);

                sender_addressLength=lib_hexToDec(tmp_buffer);  //sender_addressLength
                if(sender_addressLength%2 != 0)
                {
                    sender_addressLength +=1;
                    sender_number_attribute=0;
                }
                else
                {
                    sender_number_attribute=1;
                }

                tmp_number=tmp_number+4;
                memset(tmp_buffer,0,sizeof(tmp_buffer));
                lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+sender_addressLength-1);

                analysis_number(tmp_buffer,lsms.sender_number,sender_number_attribute,sender_addressLength);

                tmp_number=tmp_number+sender_addressLength+6;
                //lib_str_intercept(content,lsms.user_data,tmp_number,-1);
                strcpy(lsms.user_data,content);

                strcpy(lsms.time_info,"0");
            }
            else  //Receive Message
            {
                CLOGD(FINE, "Receive Message\n");
                tmp_number=tmp_number+2;
                memset(tmp_buffer,0,sizeof(tmp_buffer));

                lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+1);

                sender_addressLength=lib_hexToDec(tmp_buffer);  //sender_addressLength
                if(sender_addressLength%2 != 0)
                {
                    sender_addressLength +=1;
                    sender_number_attribute=0;
                }
                else
                {
                    sender_number_attribute=1;
                }

                tmp_number=tmp_number+2;
                memset(tmp_buffer,0,sizeof(tmp_buffer));
                lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+1);
                if(!strcmp(tmp_buffer,"91"))
                {
                    prefix_tag=1;
                }
                else
                {
                    prefix_tag=0;
                }

                tmp_number=tmp_number+2;
                memset(tmp_buffer,0,sizeof(tmp_buffer));
                lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+sender_addressLength-1);

                memset(number_tmp,0,sizeof(number_tmp));
                analysis_number(tmp_buffer,number_tmp,sender_number_attribute,sender_addressLength);

                memset(prefix,0,sizeof(prefix));
                if(prefix_tag==1)
                {
                    if(0==strncasecmp(number_tmp,"234",3))
                    {
                        strcpy(prefix,"+234");
                        lib_str_intercept(number_tmp,lsms.sender_number,3,-1);
                    }
                    else if(0==strncasecmp(number_tmp,"86",2))
                    {
                        strcpy(prefix,"+86");
                        lib_str_intercept(number_tmp,lsms.sender_number,2,-1);
                    }
                    else if(0==strncasecmp(number_tmp,"56",2))
                    {
                        strcpy(prefix,"+56");
                        lib_str_intercept(number_tmp,lsms.sender_number,2,-1);
                    }
                    else
                    {
                        strcpy(lsms.sender_number,number_tmp);
                    }
                }
                else
                {
                    strcpy(lsms.sender_number,number_tmp);
                }

                tmp_number=tmp_number+sender_addressLength;
                tmp_number=tmp_number+2;
                memset(tmp_buffer,0,sizeof(tmp_buffer));
                lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+1);   //tp_DCS
                bit_size = DCS_Bits(tmp_buffer);

                tmp_number=tmp_number+2;
                memset(tmp_buffer,0,sizeof(tmp_buffer));
                lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+11);

                analysis_time(tmp_buffer,lsms.time_info);

                //lib_str_intercept(content,lsms.user_data,tmp_number,-1);
                strcpy(lsms.user_data,content);

                if(long_message==1)
                {
                    tmp_number=tmp_number+12+10;
                    memset(tmp_buffer,0,sizeof(tmp_buffer));
                    lib_str_intercept(content,long_message_id,tmp_number,tmp_number+1);
                    tmp_number=tmp_number+2;
                    memset(tmp_buffer,0,sizeof(tmp_buffer));
                    lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+1);
                    long_message_max=lib_hexToDec(tmp_buffer);
                    tmp_number=tmp_number+2;
                    memset(tmp_buffer,0,sizeof(tmp_buffer));
                    lib_str_intercept(content,tmp_buffer,tmp_number,tmp_number+1);
                    long_message_seq=lib_hexToDec(tmp_buffer);

                    memset(sql_long,0,sizeof(sql_long));

                    sprintf(sql_long,"insert into sms_long_message values ('%s','%s','%d','%s','%s','%d','%d','0','%s');",lsms.sender_number,lsms.user_data,lsms.tag,lsms.time_info,long_message_id,long_message_max,long_message_seq,prefix);

                    if(SQLITE_OK != sqlite3_exec(db,sql_long,NULL,NULL,&errmsg_long))//判断是否插入成功成功返回SQLITE_OK
                    {
                        CLOGD(FINE, "fail:%s\n",errmsg_long);
                        CLOGD(FINE, "\n");
                        if(errmsg_long)
                        {
                            sqlite3_free(errmsg_long);
                            errmsg_long=NULL;
                        }
                        continue;
                    }
                    continue;
                }
                else
                {
                    tmp_number=tmp_number+12+4;
                    lib_str_intercept(content,user_pdu_data,tmp_number,-1);
#if defined (CONFIG_SW_STC_PINLOCK) || defined (CONFIG_SW_STC_SIMLOCK) || defined (CONFIG_SW_STC_CELLLOCK)
                    if (1 == unlockPhoneNumberMatched(lsms.sender_number))
                    {
                        checkIfSmsMatchUnlockCode(bit_size, user_pdu_data);
                        next = '+';
                        memset(&lsms,0,sizeof(lsms));
                        continue;
                    }
#endif
                }
            }

#if 0
            CLOGD(FINE, "parsing_zte_cmgl_four_sqlite3_initlsms.index%d\n",lsms.index);
            CLOGD(FINE, "parsing_zte_cmgl_four_sqlite3_initlsms.tag%d\n",lsms.tag);
            CLOGD(FINE, "lsms.sender_number, %s\n",lsms.sender_number);
            CLOGD(FINE, "lsms.time_info,%s\n",lsms.time_info);
            CLOGD(FINE, "lsms.user_data,%s\n",lsms.user_data);
            CLOGD(FINE, "LONG_MESSAGE, %d\n",long_message);
#endif

            memset(sql,0,sizeof(sql));
            sprintf(sql,"insert into sms_flash values (NULL,'%s','%s','%d','%s','%s');",lsms.sender_number,lsms.user_data,lsms.tag,lsms.time_info,prefix);

            if(SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,&errmsg))//判断是否插入成功成功返回SQLITE_OK
            {
                CLOGD(FINE, "fail:%s\n",errmsg);
                CLOGD(FINE, "\n");
                if(errmsg)
                {
                    sqlite3_free(errmsg);
                    errmsg=NULL;
                }
                continue;
            }

            next = '+';
            memset(&lsms,0,sizeof(lsms));
        }
    }

    fclose(fp);

    comd_semaphore_v ( allSms_sem_id );

    memset(sql,0,sizeof(sql));
    sprintf(sql,"select idx from sms_flash;");

    if(SQLITE_OK != sqlite3_get_table(db,sql,&aresult,&nrow,&ncol,&errmsg))
    {
        CLOGD(FINE, "fail:%s\n", errmsg);
        CLOGD(FINE, "\n");
        if(aresult)
        {
            sqlite3_free_table(aresult);
            aresult=NULL;
        }
        if(errmsg)
        {
            sqlite3_free(errmsg);
            errmsg=NULL;
        }
        sqlite3_close(db);
        return -1;
    }

    int sms_all_number=0;
    char sms_all_number_buf[8]={0};

    sms_all_number=nrow;

    if(aresult)
    {
        sqlite3_free_table(aresult);
        aresult=NULL;
    }
    sprintf(sms_all_number_buf,"%d",sms_all_number);
    nv_set ( "all_sms_number", sms_all_number_buf );

    nv_set ( "sms_need_update", "1" );
    sqlite3_close(db);

    return 0;
}


int compare(const char *s1,const char *s2)
{
    while(*s1!='\0'&&s2!='\0')
    {
        if(*s1==*s2)
        {
            s1++;
            s2++;
        }
        else if(*s1>*s2)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }

    if(*s1=='\0'&&*s2=='\0')return 0;
    else if(*s1!='\0'&&*s2=='\0')return 1;
    else return -1;
}


void long_message_table_check(void)
{
    char sql[4096]={0};
    char **aresult=NULL;
    char *errmsg=NULL;
    //char address[N];
    char number_list[1024]={0};
    char number_list_tmp[1024]={0};

    int i;
    int nrow;
    int ncol;

    char sql_long[4096]={0};
    char **aresult_long=NULL;
    char *errmsg_long=NULL;
    int nrow_long;
    int ncol_long;

    char idx_old[8]={0};
    char max_old[8]={0};
    char seq_old[8]={0};
    int long_message_continue=0;

    char number[128]={0};
    char time_max[64]={0};
    char user_data[4096]={0};
    char prefix[8]={0};

    sqlite3 *db;
    int ret;
    ret = sqlite3_open(SMS_DIR,&db);
    char *token;


    if(ret != SQLITE_OK)//打开成功返回SQLITE_OK
    {
        perror("sqlite open :");
        return ;
    }

    sprintf(sql_long,"select number from sms_long_message;");

    if(SQLITE_OK != sqlite3_get_table(db,sql_long,&aresult_long,&nrow_long,&ncol_long,&errmsg_long))
    {
        CLOGD(FINE, "fail:%s\n", errmsg_long);
        CLOGD(FINE, "\n");
        if(aresult_long)
        {
            sqlite3_free_table(aresult_long);
            aresult_long=NULL;
        }
        if(errmsg_long)
        {
            sqlite3_free(errmsg_long);
            errmsg_long=NULL;
        }
        sqlite3_close(db);
        return ;
    }


    if(nrow_long==0 || ncol_long==0)
    {
        if(aresult_long)
        {
            sqlite3_free_table(aresult_long);
            aresult_long=NULL;
        }
        if(errmsg_long)
        {
            sqlite3_free(errmsg_long);
            errmsg_long=NULL;
        }
        sqlite3_close(db);
        return ;
    }

    for(i = 1; i < (nrow_long + 1) * (ncol_long); i++)
    {

        if(strstr(number_list,aresult_long[i])==NULL)
        {
            if(!strcmp(number_list,""))
            {
                sprintf(number_list,"%s",aresult_long[i]);
            }
            else
            {
                sprintf(number_list,"%s;%s",number_list,aresult_long[i]);
            }
        }
        else
        {
            memset(number_list_tmp,0,sizeof(number_list_tmp));
            strcpy(number_list_tmp,number_list);
            token = strtok(number_list_tmp, ";");
            while( token != NULL )
            {
                if(!strcmp(token,aresult_long[i]))
                    break;

                token = strtok(NULL, ";");
            }
            if( token == NULL)
            {
                sprintf(number_list,"%s;%s",number_list,aresult_long[i]);
            }
        }

    }

    if(aresult_long)
    {
        sqlite3_free_table(aresult_long);
        aresult_long=NULL;
    }

    token = strtok(number_list, ";");
    if(token==NULL)
    {
        sqlite3_close(db);
        return;
    }
    while( token != NULL )
    {
        memset(sql_long,0,sizeof(sql_long));
        sprintf(sql_long,"select * from sms_long_message where number='%s' order by number,idx,seq asc;",token);

        if(SQLITE_OK != sqlite3_get_table(db,sql_long,&aresult_long,&nrow_long,&ncol_long,&errmsg_long))
        {
            CLOGD(FINE, "fail:%s\n", errmsg_long);
            CLOGD(FINE, "\n");
            if(errmsg_long)
            {
                sqlite3_free(errmsg_long);
                errmsg_long=NULL;
            }
            if(aresult_long)
            {
                sqlite3_free_table(aresult_long);
                aresult_long=NULL;
            }
            sqlite3_close(db);
            return ;
        }

        for(i = 9; i < (nrow_long + 1) * (ncol_long); i++)
        {
            if((i + 1) % ncol_long == 0)
            {
                memset(prefix,0,sizeof(prefix));
                strcpy(prefix,aresult_long[i]);
            }
            if((i + 1) % ncol_long == 1)
            {
                if(strcmp(number,aresult_long[i]))
                {
                    long_message_continue=0;
                    strcpy(number,aresult_long[i]);
                }
            }
            else if((i + 1) % ncol_long == 2)
            {
                if(long_message_continue==1)
                {
                    sprintf(user_data,"%s;%s",user_data,aresult_long[i]);
                }
            }
            else if((i + 1) % ncol_long == 3)
            {

            }
            else if((i + 1) % ncol_long == 4)
            {
                if(long_message_continue==0)
                {
                    strcpy(time_max,aresult_long[i]);
                }
                else
                {
                    if(compare(aresult_long[i],time_max)>0)
                        strcpy(time_max,aresult_long[i]);
                }
            }
            else if((i + 1) % ncol_long == 5)
            {
                if(strcmp(idx_old,aresult_long[i]))
                {

                    strcpy(idx_old,aresult_long[i]);
                    long_message_continue=0;
                }
            }
            else if((i + 1) % ncol_long == 6)
            {
                if(strcmp(max_old,aresult_long[i]))
                {
                    strcpy(max_old,aresult_long[i]);
                    long_message_continue=0;
                }
            }
            else if((i + 1) % ncol_long == 7)
            {
                if(!strcmp("1",aresult_long[i]) && long_message_continue==0)
                {
                    memset(user_data,0,sizeof(user_data));
                    strcpy(user_data,aresult_long[i-5]);
                    long_message_continue=1;
                    strcpy(seq_old,aresult_long[i]);
                    continue;
                }

                if(long_message_continue==1)
                {

                    if(atoi(aresult_long[i])-atoi(seq_old)==1)
                    {
                        long_message_continue=1;
                    }
                    else
                    {
                        long_message_continue=0;
                    }
                    strcpy(seq_old,aresult_long[i]);
                }

                if(!strcmp(max_old,aresult_long[i]) && long_message_continue==1)
                {
                    long_message_continue=0;

                    memset(sql,0,sizeof(sql));
                    sprintf(sql,"insert into sms_flash values (NULL,'%s','%s','1','%s','%s');",number,user_data,time_max,prefix);
                    if(SQLITE_OK != sqlite3_get_table(db,sql,&aresult,&nrow,&ncol,&errmsg))
                    {
                        CLOGD(FINE, "fail:%s\n", errmsg);
                        CLOGD(FINE, "\n");
                    }

                    if(aresult)
                    {
                        sqlite3_free_table(aresult);
                        aresult=NULL;
                    }
                    if(errmsg)
                    {
                        sqlite3_free(errmsg);
                        errmsg=NULL;
                    }

                    memset(sql,0,sizeof(sql));
                    sprintf(sql,"delete from sms_long_message where idx='%s' AND number='%s';",idx_old,number);
                    if(SQLITE_OK != sqlite3_get_table(db,sql,&aresult,&nrow,&ncol,&errmsg))
                    {
                        CLOGD(FINE, "fail:%s\n", errmsg);
                        CLOGD(FINE, "\n");

                    }

                    if(aresult)
                    {
                        sqlite3_free_table(aresult);
                        aresult=NULL;
                    }
                    if(errmsg)
                    {
                        sqlite3_free(errmsg);
                        errmsg=NULL;
                    }

                    memset(sql,0,sizeof(sql));
                    sprintf(sql,"select idx from sms_flash;");
                    if(SQLITE_OK != sqlite3_get_table(db,sql,&aresult,&nrow,&ncol,&errmsg))
                    {
                        CLOGD(FINE, "fail:%s\n", errmsg);
                        CLOGD(FINE, "\n");
                        if(aresult)
                        {
                            sqlite3_free_table(aresult);
                            aresult=NULL;
                        }
                        if(errmsg)
                        {
                            sqlite3_free(errmsg);
                            errmsg=NULL;
                        }
                        sqlite3_close(db);
                        return ;
                    }

                    int sms_all_number=0;
                    char sms_all_number_buf[8]={0};

                    sms_all_number=nrow;

                    if(aresult)
                    {
                        sqlite3_free_table(aresult);
                        aresult=NULL;
                    }
                    sprintf(sms_all_number_buf,"%d",sms_all_number);
                    nv_set ( "all_sms_number", sms_all_number_buf );
                    nv_set ( "sms_need_update", "1" );
                }
            }
        }

        token = strtok(NULL, ";");
    }


    memset(sql_long,0,sizeof(sql_long));
    sprintf(sql_long,"update sms_long_message set timeout_new = timeout_new + 2;");
    if(SQLITE_OK != sqlite3_exec(db,sql_long,NULL,NULL,&errmsg_long))
    {
        CLOGD(FINE, "fail:%s\n",errmsg_long);
        CLOGD(FINE, "\n");
        if(errmsg_long)
        {
            sqlite3_free(errmsg_long);
            errmsg_long=NULL;
        }
    }

    memset(sql_long,0,sizeof(sql_long));
    sprintf(sql_long,"delete from sms_long_message where timeout_new >= 30;");
    if(SQLITE_OK != sqlite3_exec(db,sql_long,NULL,NULL,&errmsg_long))
    {
        CLOGD(FINE, "fail:%s\n",errmsg_long);
        CLOGD(FINE, "\n");
        if(errmsg_long)
        {
            sqlite3_free(errmsg_long);
            errmsg_long=NULL;
        }
    }

    if(aresult_long)
    {
        sqlite3_free_table(aresult_long);
        aresult_long=NULL;
    }

    sqlite3_close(db);

}

int receipt_analysis () //init_type=0:init 1:add in the last
{

    char next = '+';
    char str[2048] = {0};
    int ret;
    char str_sca[2048] = {0};

    char cmd_buffer[128]={0};
    char* content;
    char sql[4096]={0};
    char *errmsg=NULL;

    FILE *fp = NULL;

    char open_file_name[64]={0};

    strcpy(open_file_name,"/tmp/receiptSms.txt");

    sprintf(cmd_buffer, "dos2unix %s",open_file_name);
    system(cmd_buffer);

    if ( NULL == ( fp = fopen ( open_file_name, "r" ) ) )
    {
        return -1;
    }

    sqlite3 *db;
    ret = sqlite3_open(SMS_DIR_TMP,&db);
    if(ret != SQLITE_OK)//打开成功返回SQLITE_OK
    {
        perror("sqlite open :");
        fclose ( fp );
        return -1;
    }

    strcpy(sql,"create table if not exists sms_receipt(idx INTEGER PRIMARY KEY,content text);");
    if(SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,&errmsg))//判断是否成功成功返回SQLITE_OK
    {
        CLOGD(FINE, "fail:%s\n", errmsg);
        CLOGD(FINE, "\n");
        if(errmsg)
        {
            sqlite3_free(errmsg);
            errmsg=NULL;
        }
        sqlite3_close(db);
        fclose ( fp );
        return -1;
    }

    while ( fgets ( str, sizeof ( str ), fp ) )
    {
        if(next == '+' && *str == '+')
        {
            next = 'p';
        }
        else if(next == 'p'
                && (( *str >= '0' && *str <= '9' ) || ( *str >= 'a' && *str <= 'f' ) || ( *str >= 'A' && *str <= 'F' )))
        {
            sprintf(str_sca, "%s", str);
            str_sca[strlen(str_sca)-1] = 0;//del \n
            content=strdup(str_sca);
            memset(sql,0,sizeof(sql));
            sprintf(sql,"insert into sms_receipt values (NULL,'%s');",content);
            if(SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,&errmsg))//判断是否插入成功成功返回SQLITE_OK
            {
                CLOGD(FINE, "fail:%s\n",errmsg);
                CLOGD(FINE, "\n");
                if(errmsg)
                {
                    sqlite3_free(errmsg);
                    errmsg=NULL;
                }
            }

            next = '+';
        }
    }
    nv_set ( "sms_receipt_need_update", "1" );
    fclose(fp);
    sqlite3_close(db);

    return 0;
}

void initSmsSqlite3 (void)
{
    int ret = 0;
    char sql[4096]={0};
    char *errmsg=NULL;
    sqlite3 *db;
    int nrow;
    int ncol;
    char **aresult=NULL;

    ret = sqlite3_open(SMS_DIR,&db);
    if(ret != SQLITE_OK)//打开成功返回SQLITE_OK
    {
        perror("sqlite open :");
        return ;
    }

    strcpy(sql,"select * from sms_flash;");
    if(SQLITE_OK == sqlite3_get_table(db,sql,&aresult,&nrow,&ncol,&errmsg))
    {

        if(ncol!=6)
        {
            memset(sql,0,sizeof(sql));
            strcpy(sql,"drop table sms_flash;");
            sqlite3_exec(db,sql,NULL,NULL,&errmsg);
        }
    }

    if(errmsg)
    {
        sqlite3_free(errmsg);
        errmsg=NULL;
    }

    if(aresult)
    {
        sqlite3_free_table(aresult);
        aresult=NULL;
    }

    memset(sql,0,sizeof(sql));
    strcpy(sql,"create table if not exists sms_flash(idx INTEGER PRIMARY KEY,number text,user_data text,status integer,time text,prefix text);");
    if(SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,&errmsg))//判断是否成功成功返回SQLITE_OK
    {
        CLOGD(FINE, "fail:%s\n", errmsg);
        CLOGD(FINE, "\n");
        if(errmsg)
        {
            sqlite3_free(errmsg);
            errmsg=NULL;
        }
        sqlite3_close(db);
        return;
    }

    memset(sql,0,sizeof(sql));
    strcpy(sql,"create table if not exists sms_long_message(number text,user_data text,status integer,time text,idx text,max integer,seq integer,timeout text,prefix text);");
    if(SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,&errmsg))//判断是否成功成功返回SQLITE_OK
    {
        CLOGD(FINE, "fail:%s\n", errmsg);
        CLOGD(FINE, "\n");
        if(errmsg)
        {
            sqlite3_free(errmsg);
            errmsg=NULL;
        }
        sqlite3_close(db);
        return;
    }

    strcpy(sql,"select * from sms_long_message;");
    if(SQLITE_OK == sqlite3_get_table(db,sql,&aresult,&nrow,&ncol,&errmsg))
    {

        if(ncol!=9)
        {
            memset(sql,0,sizeof(sql));
            strcpy(sql,"drop table sms_long_message;");
            sqlite3_exec(db,sql,NULL,NULL,&errmsg);
        }
        else if(ncol!=0)
        {
            if(!strcmp(aresult[7],"timeout"))
            {
                memset(sql,0,sizeof(sql));
                strcpy(sql,"drop table sms_long_message;");
                sqlite3_exec(db,sql,NULL,NULL,&errmsg);
            }
        }
    }

    if(errmsg)
    {
        sqlite3_free(errmsg);
        errmsg=NULL;
    }

    if(aresult)
    {
        sqlite3_free_table(aresult);
        aresult=NULL;
    }

    strcpy(sql,"create table if not exists sms_long_message(number text,user_data text,status integer,time text,idx text,max integer,seq integer,timeout_new integer,prefix text);");
    if(SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,&errmsg))//判断是否成功成功返回SQLITE_OK
    {
        CLOGD(FINE, "fail:%s\n", errmsg);
        CLOGD(FINE, "\n");
        if(errmsg)
        {
            sqlite3_free(errmsg);
            errmsg=NULL;
        }
        sqlite3_close(db);
        return;
    }

    sqlite3_close(db);
}



/* Android RIL
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** Based on reference-ril by The Android Open Source Project
** Modified by YouzhiXu 11.2019 at KT
*/

#include "comd_share.h"
#include "at_tok.h"

/**
 * Starts tokenizing an AT response string
 * returns -1 if this is not a valid response string, 0 on success.
 * updates *p_cur with current position
 */
int at_tok_start ( char **p_cur )
{
    if ( *p_cur == NULL )
    {
        return -1;
    }

    // skip prefix
    // consume "^[^:]:"

    *p_cur = strchr ( *p_cur, ':' );

    if ( *p_cur == NULL )
    {
        return -1;
    }

    ( *p_cur ) ++;

    return 0;
}

static void skipWhiteSpace ( char **p_cur )
{
    if ( *p_cur == NULL ) return;

    while ( **p_cur != '\0' && isspace ( **p_cur ) )
    {
        ( *p_cur ) ++;
    }
}

static void skipNextComma ( char **p_cur )
{
    if ( *p_cur == NULL ) return;

    while ( **p_cur != '\0' && **p_cur != ',' )
    {
        ( *p_cur ) ++;
    }

    if ( **p_cur == ',' )
    {
        ( *p_cur ) ++;
    }
}

static char * nextTok ( char **p_cur )
{
    char *ret = NULL;

    skipWhiteSpace ( p_cur );

    if ( *p_cur == NULL )
    {
        ret = NULL;
    }
    else if ( **p_cur == '"' )
    {
        ( *p_cur ) ++;
        ret = strsep ( p_cur, "\"" );
        skipNextComma ( p_cur );
    }
    else
    {
        ret = strsep ( p_cur, "," );
    }

    return ret;
}


/**
 * Parses the next integer in the AT response line and places it in *p_out
 * returns 0 on success and -1 on fail
 * updates *p_cur
 * "base" is the same as the base param in strtol
 */

static int at_tok_nextint_base ( char **p_cur, int *p_out, int base, int  uns )
{
    char *ret;

    if ( *p_cur == NULL )
    {
        return -1;
    }

    ret = nextTok ( p_cur );

    if ( ret == NULL )
    {
        return -1;
    }
    else
    {
        long l;
        char *end;

        if ( uns )
            l = strtoul ( ret, &end, base );
        else
            l = strtol ( ret, &end, base );

        *p_out = ( int ) l;

        if ( end == ret && strlen ( ret ) != 0 )
        {
            return -1;
        }
    }

    return 0;
}

/**
 * Parses the next base 10 integer in the AT response line
 * and places it in *p_out
 * returns 0 on success and -1 on fail
 * updates *p_cur
 */
int at_tok_nextint ( char **p_cur, int *p_out )
{
    return at_tok_nextint_base ( p_cur, p_out, 10, 0 );
}

/**
 * Parses the next base 16 integer in the AT response line
 * and places it in *p_out
 * returns 0 on success and -1 on fail
 * updates *p_cur
 */
int at_tok_nexthexint ( char **p_cur, int *p_out )
{
    return at_tok_nextint_base ( p_cur, p_out, 16, 1 );
}

int at_tok_nextbool ( char **p_cur, char *p_out )
{
    int ret;
    int result;

    ret = at_tok_nextint ( p_cur, &result );

    if ( ret < 0 )
    {
        return -1;
    }

    // booleans should be 0 or 1
    if ( ! ( result == 0 || result == 1 ) )
    {
        return -1;
    }

    if ( p_out != NULL )
    {
        *p_out = ( char ) result;
    }

    return ret;
}

int at_tok_nextstr ( char **p_cur, char **p_out )
{
    if ( *p_cur == NULL )
    {
        return -1;
    }

    *p_out = nextTok ( p_cur );

    return 0;
}

/** returns 1 on "has more tokens" and 0 if no */
int at_tok_hasmore ( char **p_cur )
{
    return ! ( *p_cur == NULL || **p_cur == '\0' );
}

/*
 * Summary:
 *    parsing main info from input string through regular expressions
 * Parameters:
 *    p_cur : input string which contain main info
 *    reg_rule : regular expressions for input string
 *    p_out : main info
 * Return :
 *    0  -> regex match success
 *    -1 -> regex match fail
 */
int at_tok_regular_more ( char *p_cur, const char *reg_rule, char ( *p_out ) [REGEX_BUF_ONE] )
{
    regex_t at_tok_reg;
    int reg_ret = 0;
    char reg_err_info[REGEX_BUF_ONE] = {0};
    regmatch_t pmatch[MAX_REGEX_MATCH_NUM];
    int ind = 0;
    int len = 0;
    int fun_ret = -1;

    if ( NULL == p_cur || NULL == reg_rule )
    {
        CLOGD ( WARNING, "p_cur == NULL || reg_rule == NULL\n" );
        return -1;
    }

    reg_ret = regcomp ( &at_tok_reg, reg_rule, REG_EXTENDED );
    if ( reg_ret < 0 )
    {
        regerror ( reg_ret, &at_tok_reg, reg_err_info, sizeof ( reg_err_info ) );
        CLOGD ( ERROR, "regcomp -> [%s]\n", reg_err_info );
    }
    else
    {
        reg_ret = regexec ( &at_tok_reg, p_cur, MAX_REGEX_MATCH_NUM, pmatch, 0 );
        if ( reg_ret == REG_NOMATCH )
        {
            CLOGD ( WARNING, "regex no match !!!\n" );
        }
        else if ( reg_ret )
        {
            regerror ( reg_ret, &at_tok_reg, reg_err_info, sizeof ( reg_err_info ) );
            CLOGD ( ERROR, "regexec -> [%s]\n", reg_err_info );
        }
        else
        {
            for ( ind = 0; ind < MAX_REGEX_MATCH_NUM && pmatch[ind].rm_so != -1; ind++ )
            {
                len = pmatch[ind].rm_eo - pmatch[ind].rm_so;
                if ( 0 < len && len < REGEX_BUF_ONE )
                {
                    memcpy ( p_out[ind], p_cur + pmatch[ind].rm_so, len );
                }
            }
            fun_ret = 0;
        }
        regfree ( &at_tok_reg );
    }

    return fun_ret;
}

int strStartsWith ( const char *line, const char *prefix )
{
    for ( ; *line != '\0' && *prefix != '\0' ; line++, prefix++ )
    {
        if ( *line != *prefix )
        {
            return 0;
        }
    }

    return *prefix == '\0';
}

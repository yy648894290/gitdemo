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

#ifndef _AT_TOK_H
#define _AT_TOK_H 1

int at_tok_start ( char **p_cur );
int at_tok_nextint ( char **p_cur, int *p_out );
int at_tok_nexthexint ( char **p_cur, int *p_out );

int at_tok_nextbool ( char **p_cur, char *p_out );
int at_tok_nextstr ( char **p_cur, char **out );

int at_tok_hasmore ( char **p_cur );

int at_tok_regular_more ( char *p_cur, const char *reg_rule, char ( *p_out ) [REGEX_BUF_ONE] );

int strStartsWith ( const char *line, const char *prefix );

#endif /* AT_TOK_H */

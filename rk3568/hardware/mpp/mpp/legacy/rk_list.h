/*
 *
 * Copyright 2015 Rockchip Electronics Co., LTD.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RK_LIST_H__
#define __RK_LIST_H__

#include <pthread.h>
#include "rk_type.h"
// desctructor of list node
typedef void *(*node_destructor)(void *);

struct rk_list_node;
class rk_list {
public:
    rk_list(node_destructor func);
    ~rk_list();

    // for FIFO or FILO implement
    // adding functions support simple structure like C struct or C++ class pointer,
    // do not support C++ object
    RK_S32 add_at_head(void *data, RK_S32 size);
    RK_S32 add_at_tail(void *data, RK_S32 size);
    // deleting function will copy the stored data to input pointer with size as size
    // if NULL is passed to deleting functions, the node will be delete directly
    RK_S32 del_at_head(void *data, RK_S32 size);
    RK_S32 del_at_tail(void *data, RK_S32 size);

    // for status check
    RK_S32 list_is_empty();
    RK_S32 list_size();

    // for vector implement - not implemented yet
    // adding function will return a key
    RK_S32 add_by_key(void *data, RK_S32 size, RK_U32 *key);
    RK_S32 del_by_key(void *data, RK_S32 size, RK_U32 key);
    RK_S32 show_by_key(void *data, RK_U32 key);

    RK_S32 flush();

private:
    pthread_mutex_t         mutex;
    node_destructor         destroy;
    struct rk_list_node    *head;
    RK_S32                 count;

    rk_list();
    rk_list(const rk_list &);
    rk_list &operator=(const rk_list &);
};

#endif

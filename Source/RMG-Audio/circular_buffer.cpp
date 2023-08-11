/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-ui-console - circular_buffer.c                            *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2015 Bobby Smiles                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <assert.h>
#include <cstdlib>
#include <cstring>

#include "circular_buffer.hpp"


int init_cbuff(struct circular_buffer* cbuff, size_t capacity)
{
    void* data = malloc(capacity);

    if (data == nullptr)
    {
        return -1;
    }

    cbuff->data = data;
    cbuff->size = capacity;
    cbuff->head = 0;

    return 0;
}

void release_cbuff(struct circular_buffer* cbuff)
{
    free(cbuff->data);
    memset(cbuff, 0, sizeof(*cbuff));
}


void* cbuff_head(const struct circular_buffer* cbuff, size_t* available)
{
    if (cbuff->head > cbuff->size)
        return nullptr;

    *available = cbuff->size - cbuff->head;
    return (unsigned char*)cbuff->data + cbuff->head;
}


void* cbuff_tail(const struct circular_buffer* cbuff, size_t* available)
{
    *available = cbuff->head;
    return cbuff->data;
}


void produce_cbuff_data(struct circular_buffer* cbuff, size_t amount)
{
    if (cbuff->head + amount > cbuff->size)
        return;

    cbuff->head += amount;
}


void consume_cbuff_data(struct circular_buffer* cbuff, size_t amount)
{
    if (cbuff->head < amount)
        return;

    memmove(cbuff->data, (unsigned char*)cbuff->data + amount, cbuff->head - amount);
    cbuff->head -= amount;
}


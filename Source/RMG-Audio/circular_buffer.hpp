/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-ui-console - circular_buffer.h                            *
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

#ifndef M64P_CIRCULAR_BUFFER_H
#define M64P_CIRCULAR_BUFFER_H

#include <cstdlib>

struct circular_buffer
{
    void* data;
    size_t size;
    size_t head;
};

int init_cbuff(struct circular_buffer* cbuff, size_t capacity);

void release_cbuff(struct circular_buffer* cbuff);

void* cbuff_head(const struct circular_buffer* cbuff, size_t* available);

void* cbuff_tail(const struct circular_buffer* cbuff, size_t* available);

void produce_cbuff_data(struct circular_buffer* cbuff, size_t amount);

void consume_cbuff_data(struct circular_buffer* cbuff, size_t amount);

#endif

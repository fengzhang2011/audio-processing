/*************************************************
 *
 * MFCC computation.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-20
 * 
 * Description:
 *   This version is a simplified version of the h264bitstream source file "bs.h" in the "opencore-amr" project.
 * 
 ************************************************/

/* 
 * h264bitstream - a library for reading and writing H.264 video
 * Copyright (C) 2005-2007 Auroras Entertainment, LLC
 * Copyright (C) 2008-2011 Avail-TVN
 * 
 * Written by Alex Izvorski <aizvorski@gmail.com> and Alex Giladi <alex.giladi@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _H264_BS_H
#define _H264_BS_H        1

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct { uint8_t* start; uint8_t* p; uint8_t* end; int bits_left; } bs_t;

static inline bs_t* bs_new(uint8_t* buf, size_t size) { bs_t* b = (bs_t*)malloc(sizeof(bs_t)); b->start = b->p = buf; b->end = buf + size; b->bits_left = 8; return b; }
static inline void bs_free(bs_t* b) { free(b); }
static inline int bs_eof(bs_t* b) { if (b->p >= b->end) { return 1; } else { return 0; } }
static inline int bs_bytes_left(bs_t* b) { return (b->end - b->p); }
static inline uint32_t bs_read_u1(bs_t* b) { uint32_t r = 0; b->bits_left--; if (! bs_eof(b)) { r = ((*(b->p)) >> b->bits_left) & 0x01; } if (b->bits_left == 0) { b->p ++; b->bits_left = 8; } return r; }
static inline uint32_t bs_read_u(bs_t* b, int n) { uint32_t r = 0; for (int i = 0; i < n; i++) { r |= ( bs_read_u1(b) << ( n - i - 1 ) ); } return r; }

static inline int bs_read_bytes(bs_t* b, uint8_t* buf, int len)
{
  int actual_len = len;
  if (b->end - b->p < actual_len) { actual_len = b->end - b->p; }
  if (actual_len < 0) { actual_len = 0; }
  memcpy(buf, b->p, actual_len);
  if (len < 0) { len = 0; }
  b->p += len;
  return actual_len;
}

static inline int bs_read_bytes_ex(bs_t* b, uint8_t* buf, int len)
{
  if(b->bits_left == 8) { return bs_read_bytes(b, buf, len); }
  
  int i=0;
  for(i=0; i<len; i++)
  {
    if(bs_eof(b)) break;
    int remain = b->bits_left;      // 6비트가 남아 있다.
    int shift = (8 - b->bits_left); // 6비트를 처리하므로, 나머지는 2 bits 이다.
    // Get value from b (1st)
    int mask = 0xFF >> shift;       // 6비트를 추출하려면 2bits만 이동한다.
    int value = *(b->p) & mask;
    *buf |= (value << shift);   // 6비트를 설정했으므로, 2bits를 이동해야 한다.
    b->p ++; b->bits_left = 8;  // Next bit
    if(bs_eof(b)) { i++; break; }
    // Get value from b (2nd)
    mask = 0xFF >> remain;      // 2bits 를 추출하려면 6 bits 이동한다.
    value = *(b->p) >> (remain);// 2bits 를 추츨하려면 6 bits 이동한다.
    b->bits_left = remain;      // 2 bits를 추출하므로, 6 bits 남아 있음.
    *buf |= value & mask;
    buf++;
  }
  return i;
}

#endif

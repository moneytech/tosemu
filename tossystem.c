/*
 * TOSEMU - and emulated environment for TOS applications
 * Copyright (C) 2014 Johan Thelin <e8johan@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tossystem.h"

/* Basepage, as defined here:
 * http://www.yardley.cc/atari/compendium/atari-compendium-chapter-2-GEMDOS.htm#gdprocess
 * 
 * bigendian
 */
#pragma pack(push,2)
struct basepage {
    uint32_t p_lowtpa;
    uint32_t p_hitpa;
    uint32_t p_tbase, p_tlen;
    uint32_t p_dbase, p_dlen;
    uint32_t p_bbase, p_blen;
    uint32_t p_dta;
    uint32_t p_parent;
    uint32_t p_reserved;
    uint32_t p_env;
    uint8_t p_undef[80];
    uint8_t p_cmdlin[128];
};
#pragma pack(pop)

/* Header of executable, as defined here: 
 * http://www.yardley.cc/atari/compendium/atari-compendium-chapter-2-GEMDOS.htm#gdprocess 
 * 
 * big endian
 */
#pragma pack(push,2)
struct exec_header {
    uint16_t magic;
    uint32_t tsize, 
             dsize, 
             bsize, 
             ssize;
    uint32_t res;
    uint32_t flags;
    uint16_t absflag;
};
#pragma pack(pop)

uint16_t endianize_16(uint16_t in)
{
    uint16_t out;
    int i;
    
    for(i=0; i<2; ++i)
    {
        out = out << 8;
        out = out | (0xff&in);
        in = in >> 8;
    }
    
    return out;
}

uint32_t endianize_32(uint32_t in)
{
    uint32_t out;
    int i;
    
    for(i=0; i<4; ++i)
    {
        out = out << 8;
        out = out | (0xff&in);
        in = in >> 8;
    }
    
    return out;
}

int init_tos_environment(struct tos_environment *te, void *binary, uint64_t size)
{
    struct exec_header *header;
    
    /* Record size and base address of binary */
    te->size = size;
    te->binary = binary;
    
    /* Ensure that binary is large enough to hold a header */
    if (size < sizeof(struct exec_header))
    {
        printf("Error: Too small binary\n");
        return -1;
    }
    
    /* Copy segment sizes from header */
    header = (struct exec_header*)binary;
    te->tsize = endianize_32(header->tsize);
    te->dsize = endianize_32(header->dsize); 
    te->bsize = endianize_32(header->bsize); 
    te->ssize = endianize_32(header->ssize);
    
    printf("HEADER\n  TEXT: 0x%x\n  DATA: 0x%x\n  BSS:  0x%x\n  SYMS: 0x%x\n",
           te->tsize,
           te->dsize,
           te->bsize,
           te->ssize);
    
    /* Allocate basepage */
    te->bp = malloc(sizeof(struct basepage));
    
    /* Prepare basepage according to memory map from ATARI ST/STE Hårdfakta, page 290
     * 
     * Accessible from user mode
     * 
     * 0xFFFFFF - 0xFF8000 I/O-AREA
     * 0xFEFFFF - 0xFC0000 OS ROM
     * 0xFBFFFF - 0xFA0000 CARTRIDGE ROM
     * 0x0FFFFF - 0x000800 USER RAM
     * 0x0007FF - 0x000000 OS RAM
     * 
     * Lay out data like this in USER RAM:
     * 
     * High addresses     HEAP
     * 
     *                   STACK
     * 
     *                    BSS
     * 
     *                    DATA
     * 
     *                    TEXT
     * 
     * Low addresses       BP
     *
     */

    memset(te->bp, 0, sizeof(struct basepage));
    te->bp->p_lowtpa = endianize_32(0x000800);
    te->bp->p_hitpa = endianize_32(endianize_32(te->bp->p_lowtpa) + sizeof(struct basepage));
    te->bp->p_tbase = te->bp->p_hitpa;
    te->bp->p_tlen = endianize_32(te->tsize);
    te->bp->p_dbase = endianize_32(endianize_32(te->bp->p_tbase) + endianize_32(te->bp->p_tlen));
    te->bp->p_dlen = endianize_32(te->dsize);
    te->bp->p_bbase = endianize_32(endianize_32(te->bp->p_dbase) + endianize_32(te->bp->p_dlen));
    te->bp->p_blen = endianize_32(te->bsize);
    /* TODO, Disk Transfer Address, http://www.yardley.cc/atarsi/compendium/atari-compendium-chapter-2-GEMDOS.htm#filesystem
     * te->bp->p_dta; */
    /* TODO how to provide a pointer to the parent process? 
     * te->bp->p_parent; */
    /* TODO te->bp->p_env; */
    /* TODO te->bp->p_cmdlin[128];*/
    
    printf("BASEPAGE\n  BASEPAGE: 0x%x - 0x%x\n  TEXT: 0x%x [0x%x]\n  DATA: 0x%x [0x%x]\n  BSS:  0x%x [0x%x]\n  DTA:  0x%x\n  PARENT: 0x%x\n  ENV: 0x%x\n  CMDLINE: '%s'\n",
           endianize_32(te->bp->p_lowtpa),
           endianize_32(te->bp->p_hitpa),
           endianize_32(te->bp->p_tbase),
           endianize_32(te->bp->p_tlen),
           endianize_32(te->bp->p_dbase),
           endianize_32(te->bp->p_dlen),
           endianize_32(te->bp->p_bbase),
           endianize_32(te->bp->p_blen),
           endianize_32(te->bp->p_dta),
           endianize_32(te->bp->p_parent),
           te->bp->p_env,
           te->bp->p_cmdlin
           );
        
    /* TODO perform relocation fixups according to binary, pseudo code here: 
     * http://code.metager.de/source/xref/haiku/docs/develop/ports/m68k/atari/atariexe.txt
     */
    
    return 0;
}

void free_tos_environment(struct tos_environment *te)
{
    free(te->bp);
    te->bp = 0;
}

unsigned int  m68k_read_disassembler_8(unsigned int address)
{
    return 0;
}
unsigned int  m68k_read_disassembler_16(unsigned int address)
{
    return 0;
}
unsigned int  m68k_read_disassembler_32(unsigned int address)
{
    return 0;
}

unsigned int  m68k_read_memory_8(unsigned int address)
{
    return 0;
}
unsigned int  m68k_read_memory_16(unsigned int address)
{
    return 0;
}
unsigned int  m68k_read_memory_32(unsigned int address)
{
    return 0;
}

void m68k_write_memory_8(unsigned int address, unsigned int value)
{
    return;
}
void m68k_write_memory_16(unsigned int address, unsigned int value)
{
    return;
}
void m68k_write_memory_32(unsigned int address, unsigned int value)
{
    return;
}

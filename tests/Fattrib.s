| TOSEMU - an emulated environment for TOS applications
| Copyright (C) 2014 Johan Thelin <e8johan@gmail.com>
| 
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; if not, write to the Free Software
| Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

XDEF _start

.text
_start:
        clr.l   -(sp)
        pea     fname1
        move.w  #67,-(sp)       | call Fattrib
        trap    #1
        lea     10(sp),sp
        
        tst.w   d0              | check success
        bmi.s   fail
        and.w   #0x10,d0
        tst.w   d0
        bne.s   fail
        
        clr.l   -(sp)
        pea     fname2
        move.w  #67,-(sp)       | call Fopen
        trap    #1
        lea     10(sp),sp
        
        tst.w   d0              | check success
        bmi.s   fail
        and.w   #0x10,d0
        tst.w   d0
        beq.s   fail
        
        clr.w   -(sp)           | call Pterm0
        trap    #1
        
fail:   move.w  #1,-(sp)
        move.w  #0x4c,-(sp)     | call Pterm
        trap    #1

fname1: .ascii  "Makefile\0"
fname2: .ascii  ".\0"
#
/*
 * Raspberry Pi support library for the ACK
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

! Declare segments (the order is important).

.sect .text
.sect .rom
.sect .data
.sect .bss

.sect .text

begtext:
#if 0
	! Wipe the bss. (I'm a little suprised that __m_a_i_n doesn't do this.)
	
	mov di, begbss
	mov cx, endbss
	sub cx, di
	mov ax, 0
	rep stosb
	
	! Push standard parameters onto the stack and go.
	
	push envp               ! envp
	push argv               ! argv
	push 1                  ! argc
#endif
	b __m_a_i_n

! Define symbols at the beginning of our various segments, so that we can find
! them. (Except .text, which has already been done.)

.define begtext, begdata, begbss
.sect .data;       begdata:
.sect .rom;        begrom:
.sect .bss;        begbss:

! Some magic data. All EM systems need these.

.define .trppc, .ignmask, _errno
.comm .trppc, 4
.comm .ignmask, 4
.comm _errno, 4

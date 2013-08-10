; file cs.as - includes count.

		.entry COUNT
		.extern STRADD
		.extern LEN
COUNT:		mov	STRADD, r1
		cmp	#0, r1
		jmp	ENDCOUNT
		inc	LEN
		inc	r1
		bne	COUNT
ENDCOUNT:	rts 

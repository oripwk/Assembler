;file rs.as - includes routine reverse

		
		.extern LEN
		.entry REVERSE
REVERSE:	mov	STRADD, r1
		mov	LASTCHAR, r2
		mov	LEN, r3 

;Exchanging places

LOOP:		cmp	#0, r3
		jmp	END
		cmp	r1, r2
		jmp	END
		mov	r1, r4
		mov	r2, r1
		mov 	r4, r2
		sub	#2, r3
		bne	LOOP

END:		rts
.extern LASTCHAR
.extern STRADD


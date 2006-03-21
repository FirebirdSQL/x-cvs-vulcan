	.inline cas,3
	cas		[%o0],%o1,%o2
	mov 	%o2,%o0
	.end
	
	.inline casx,3
	casx	[%o0],%o1,%o2
	mov 	%o2,%o0
	.end

	.inline membar_flush,0
	membar	0x32     /* cmask = MemIssue | Lookaside, mmask = StoreLoad */
	.end

	.inline membar_wait,0
	membar	0x32     /*  cmask = MemIssue | Lookaside, mmask = StoreLoad */
	.end

	.inline cas,3
	cas		[%o0],%o1,%o2
	mov 	%o2,%o0
	.end
	
	.inline casx,3
	casx	[%o0],%o1,%o2
	mov 	%o2,%o0
	.end

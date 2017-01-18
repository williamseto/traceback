
.global currframeptr
.global callretaddr
.global prevebp



currframeptr:
	movl	%ebp,%eax
	ret

callretaddr:
	movl	4(%esp),%eax	// esp should be pointing to eip, 
							// so previous location will be the param
	movl	4(%eax),%eax    // ebp points to old ebp,previous should be old eip
	ret

prevebp:
    movl    4(%esp),%eax    //get param
    movl    (%eax),%eax     //deference the old ebp
    ret




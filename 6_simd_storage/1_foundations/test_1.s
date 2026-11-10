.Ltmp0:
	.loc	0 11 3                          # test_1.cpp:11:3
	je	.LBB0_1
.Ltmp1:
# %bb.2:
	#DEBUG_VALUE: sum:values <- $rdi
	#DEBUG_VALUE: sum:size <- $rsi
	#DEBUG_VALUE: sum:total <- 0
	#DEBUG_VALUE: i <- 0
	cmpq	$4, %rsi
	jae	.LBB0_5
.Ltmp2:
# %bb.3:
	#DEBUG_VALUE: sum:values <- $rdi
	#DEBUG_VALUE: sum:size <- $rsi
	#DEBUG_VALUE: sum:total <- 0
	#DEBUG_VALUE: i <- 0
	.loc	0 0 3 is_stmt 0                 # test_1.cpp:0:3
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	jmp	.LBB0_4
.Ltmp3:
.LBB0_1:
	#DEBUG_VALUE: sum:values <- $rdi
	#DEBUG_VALUE: sum:size <- $rsi
	#DEBUG_VALUE: sum:total <- 0
	xorl	%eax, %eax
	.loc	0 14 3 is_stmt 1                # test_1.cpp:14:3
	retq
.Ltmp4:
.LBB0_5:
	#DEBUG_VALUE: sum:values <- $rdi
	#DEBUG_VALUE: sum:size <- $rsi
	#DEBUG_VALUE: sum:total <- 0
	#DEBUG_VALUE: i <- 0
	.loc	0 11 3 is_stmt 0                # test_1.cpp:11:3
	cmpq	$32, %rsi
	jae	.LBB0_7
.Ltmp5:
# %bb.6:
	#DEBUG_VALUE: sum:values <- $rdi
	#DEBUG_VALUE: sum:size <- $rsi
	#DEBUG_VALUE: sum:total <- 0
	#DEBUG_VALUE: i <- 0
	.loc	0 0 3                           # test_1.cpp:0:3
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	.loc	0 11 3                          # test_1.cpp:11:3
	jmp	.LBB0_11
.Ltmp6:
.LBB0_7:
	#DEBUG_VALUE: sum:values <- $rdi
	#DEBUG_VALUE: sum:size <- $rsi
	#DEBUG_VALUE: sum:total <- 0
	#DEBUG_VALUE: i <- 0
	.loc	0 0 3                           # test_1.cpp:0:3
	movq	%rsi, %rcx
	andq	$-32, %rcx
	vpxor	%xmm0, %xmm0, %xmm0
	xorl	%eax, %eax
	vpxor	%xmm1, %xmm1, %xmm1
	vpxor	%xmm2, %xmm2, %xmm2
	vpxor	%xmm3, %xmm3, %xmm3
.Ltmp7:
	.p2align	4
.LBB0_8:                                # =>This Inner Loop Header: Depth=1
	#DEBUG_VALUE: sum:values <- $rdi
	#DEBUG_VALUE: sum:size <- $rsi
	#DEBUG_VALUE: sum:total <- 0
	#DEBUG_VALUE: i <- 0
	.loc	0 12 11 is_stmt 1               # test_1.cpp:12:11
	vpaddd	(%rdi,%rax,4), %ymm0, %ymm0
	vpaddd	32(%rdi,%rax,4), %ymm1, %ymm1
	vpaddd	64(%rdi,%rax,4), %ymm2, %ymm2
	vpaddd	96(%rdi,%rax,4), %ymm3, %ymm3
.Ltmp8:
	.loc	0 11 37                         # test_1.cpp:11:37
	addq	$32, %rax
.Ltmp9:
	.loc	0 11 3                          # test_1.cpp:11:3
	cmpq	%rax, %rcx
	jne	.LBB0_8
.Ltmp10:
# %bb.9:
	#DEBUG_VALUE: sum:values <- $rdi
	#DEBUG_VALUE: sum:size <- $rsi
	#DEBUG_VALUE: sum:total <- 0
	#DEBUG_VALUE: i <- 0
	.loc	0 11 3                          # test_1.cpp:11:3
	vpaddd	%ymm0, %ymm1, %ymm0
	vpaddd	%ymm2, %ymm3, %ymm1
	vpaddd	%ymm0, %ymm1, %ymm0
	vextracti128	$1, %ymm0, %xmm1
	vpaddd	%xmm1, %xmm0, %xmm0
	vpshufd	$238, %xmm0, %xmm1              # xmm1 = xmm0[2,3,2,3]
	vpaddd	%xmm1, %xmm0, %xmm0
	vpshufd	$85, %xmm0, %xmm1               # xmm1 = xmm0[1,1,1,1]
	vpaddd	%xmm1, %xmm0, %xmm0
	vmovd	%xmm0, %eax
	cmpq	%rcx, %rsi
	je	.LBB0_15

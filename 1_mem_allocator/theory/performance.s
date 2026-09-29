	.file	"performance.c"
	.text
	.p2align 4
	.type	now_seconds, @function
now_seconds:
.LFB39:
	.cfi_startproc
	subq	$40, %rsp
	.cfi_def_cfa_offset 48
	movl	$1, %edi
	movq	%fs:40, %rax
	movq	%rax, 24(%rsp)
	xorl	%eax, %eax
	movq	%rsp, %rsi
	call	clock_gettime@PLT
	pxor	%xmm1, %xmm1
	pxor	%xmm0, %xmm0
	cvtsi2sdq	(%rsp), %xmm1
	cvtsi2sdq	8(%rsp), %xmm0
	divsd	.LC0(%rip), %xmm0
	addsd	%xmm1, %xmm0
	movq	24(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L5
	addq	$40, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 8
	ret
.L5:
	.cfi_restore_state
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE39:
	.size	now_seconds, .-now_seconds
	.p2align 4
	.globl	vector_length
	.type	vector_length, @function
vector_length:
.LFB40:
	.cfi_startproc
	endbr64
	movl	(%rdi), %eax
	testl	%eax, %eax
	je	.L6
	movl	$1, %edx
	.p2align 4,,10
	.p2align 3
.L8:
	movq	%rdx, %rax
	addq	$1, %rdx
	movl	-4(%rdi,%rdx,4), %ecx
	testl	%ecx, %ecx
	jne	.L8
.L6:
	ret
	.cfi_endproc
.LFE40:
	.size	vector_length, .-vector_length
	.p2align 4
	.globl	sum_1
	.type	sum_1, @function
sum_1:
.LFB41:
	.cfi_startproc
	endbr64
	movl	(%rdi), %r9d
	movslq	%esi, %rsi
	xorl	%ecx, %ecx
	leaq	(%rdx,%rsi,4), %rsi
	testl	%r9d, %r9d
	je	.L19
	.p2align 4,,10
	.p2align 3
.L21:
	movl	$1, %eax
	.p2align 4,,10
	.p2align 3
.L16:
	movq	%rax, %rdx
	addq	$1, %rax
	movl	-4(%rdi,%rax,4), %r8d
	testl	%r8d, %r8d
	jne	.L16
	cmpl	%ecx, %edx
	jle	.L19
	movl	(%rdi,%rcx,4), %eax
	addl	%eax, (%rsi)
	addq	$1, %rcx
	movl	(%rdi), %r9d
	testl	%r9d, %r9d
	jne	.L21
.L19:
	xorl	%eax, %eax
	ret
	.cfi_endproc
.LFE41:
	.size	sum_1, .-sum_1
	.p2align 4
	.globl	sum_2
	.type	sum_2, @function
sum_2:
.LFB42:
	.cfi_startproc
	endbr64
	movl	(%rdi), %r9d
	testl	%r9d, %r9d
	je	.L23
	xorl	%eax, %eax
	.p2align 4,,10
	.p2align 3
.L24:
	movq	%rax, %rcx
	addq	$1, %rax
	movl	(%rdi,%rax,4), %r8d
	testl	%r8d, %r8d
	jne	.L24
	movslq	%esi, %rsi
	leaq	4(%rdi,%rcx,4), %rcx
	leaq	(%rdx,%rsi,4), %rdx
	movl	(%rdx), %eax
	.p2align 4,,10
	.p2align 3
.L25:
	addl	(%rdi), %eax
	addq	$4, %rdi
	movl	%eax, (%rdx)
	cmpq	%rcx, %rdi
	jne	.L25
.L23:
	xorl	%eax, %eax
	ret
	.cfi_endproc
.LFE42:
	.size	sum_2, .-sum_2
	.p2align 4
	.globl	sum_3
	.type	sum_3, @function
sum_3:
.LFB43:
	.cfi_startproc
	endbr64
	movl	(%rdi), %eax
	testl	%eax, %eax
	je	.L29
	xorl	%ecx, %ecx
	.p2align 4,,10
	.p2align 3
.L30:
	movq	%rcx, %r8
	addq	$1, %rcx
	movl	(%rdi,%rcx,4), %eax
	testl	%eax, %eax
	jne	.L30
	leaq	4(%rdi,%r8,4), %rcx
	.p2align 4,,10
	.p2align 3
.L31:
	addl	(%rdi), %eax
	addq	$4, %rdi
	cmpq	%rcx, %rdi
	jne	.L31
.L29:
	movslq	%esi, %rsi
	movl	%eax, (%rdx,%rsi,4)
	xorl	%eax, %eax
	ret
	.cfi_endproc
.LFE43:
	.size	sum_3, .-sum_3
	.p2align 4
	.globl	init_vector
	.type	init_vector, @function
init_vector:
.LFB44:
	.cfi_startproc
	endbr64
	testl	%esi, %esi
	movl	$1, %eax
	movslq	%esi, %rsi
	jle	.L38
	.p2align 4,,10
	.p2align 3
.L39:
	movq	%rax, %rdx
	movl	%eax, -4(%rdi,%rax,4)
	addq	$1, %rax
	cmpq	%rsi, %rdx
	jne	.L39
.L38:
	movl	$0, (%rdi,%rsi,4)
	ret
	.cfi_endproc
.LFE44:
	.size	init_vector, .-init_vector
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC4:
	.string	"Sum 2: %f\n"
.LC5:
	.string	"Sum 3: %f\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB45:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	leaq	-3997696(%rsp), %r11
	.cfi_def_cfa 11, 3997720
.LPSRL0:
	subq	$4096, %rsp
	orq	$0, (%rsp)
	cmpq	%r11, %rsp
	jne	.LPSRL0
	.cfi_def_cfa_register 7
	subq	$2376, %rsp
	.cfi_def_cfa_offset 4000096
	pxor	%xmm0, %xmm0
	movdqa	.LC1(%rip), %xmm1
	movdqa	.LC2(%rip), %xmm3
	movdqa	.LC3(%rip), %xmm2
	movq	%fs:40, %rax
	movq	%rax, 4000056(%rsp)
	xorl	%eax, %eax
	movl	$0, 32(%rsp)
	leaq	48(%rsp), %rbx
	leaq	4000048(%rsp), %rdx
	movaps	%xmm0, 16(%rsp)
	movq	%rbx, %rax
	.p2align 4,,10
	.p2align 3
.L44:
	movdqa	%xmm1, %xmm0
	addq	$16, %rax
	paddd	%xmm3, %xmm1
	paddd	%xmm2, %xmm0
	movaps	%xmm0, -16(%rax)
	cmpq	%rdx, %rax
	jne	.L44
	movl	$0, 4000048(%rsp)
	leaq	16(%rsp), %rbp
	call	now_seconds
	movq	%rbp, %rdx
	xorl	%esi, %esi
	movq	%rbx, %rdi
	movsd	%xmm0, 8(%rsp)
	call	sum_2
	call	now_seconds
	subsd	8(%rsp), %xmm0
	movl	$2, %edi
	leaq	.LC4(%rip), %rsi
	movl	$1, %eax
	call	__printf_chk@PLT
	call	now_seconds
	xorl	%esi, %esi
	movq	%rbx, %rdi
	movq	%rbp, %rdx
	movsd	%xmm0, 8(%rsp)
	call	sum_3
	call	now_seconds
	subsd	8(%rsp), %xmm0
	movl	$2, %edi
	leaq	.LC5(%rip), %rsi
	movl	$1, %eax
	call	__printf_chk@PLT
	movq	4000056(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L48
	addq	$4000072, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 24
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
.L48:
	.cfi_restore_state
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE45:
	.size	main, .-main
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC0:
	.long	0
	.long	1104006501
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC1:
	.long	0
	.long	1
	.long	2
	.long	3
	.align 16
.LC2:
	.long	4
	.long	4
	.long	4
	.long	4
	.align 16
.LC3:
	.long	1
	.long	1
	.long	1
	.long	1
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:

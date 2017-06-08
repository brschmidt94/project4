	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 12
	.globl	_openDisk
	.align	4, 0x90
_openDisk:                              ## @openDisk
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp0:
	.cfi_def_cfa_offset 16
Ltmp1:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp2:
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movq	$0, -24(%rbp)
	cmpl	$0, -12(%rbp)
	jl	LBB0_10
## BB#1:
	movl	$256, %eax              ## imm = 0x100
	movl	-12(%rbp), %ecx
	movl	%eax, -32(%rbp)         ## 4-byte Spill
	movl	%ecx, %eax
	cltd
	movl	-32(%rbp), %ecx         ## 4-byte Reload
	idivl	%ecx
	cmpl	$0, %edx
	jne	LBB0_10
## BB#2:
	cmpl	$0, -12(%rbp)
	jne	LBB0_5
## BB#3:
	xorl	%esi, %esi
	movq	-8(%rbp), %rdi
	callq	_access
	cmpl	$0, %eax
	je	LBB0_5
## BB#4:
	movl	$-1, -28(%rbp)
	jmp	LBB0_9
LBB0_5:
	cmpl	$0, -12(%rbp)
	jne	LBB0_7
## BB#6:
	leaq	L_.str(%rip), %rsi
	movq	-8(%rbp), %rdi
	callq	_fopen
	movq	%rax, -24(%rbp)
	jmp	LBB0_8
LBB0_7:
	leaq	L_.str.1(%rip), %rsi
	movq	-8(%rbp), %rdi
	callq	_fopen
	movq	%rax, -24(%rbp)
LBB0_8:
	jmp	LBB0_9
LBB0_9:
	movq	-24(%rbp), %rdi
	callq	_fileno
	movl	%eax, -28(%rbp)
	jmp	LBB0_11
LBB0_10:
	movl	$-2, -28(%rbp)
LBB0_11:
	movl	-28(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_readBlock
	.align	4, 0x90
_readBlock:                             ## @readBlock
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp3:
	.cfi_def_cfa_offset 16
Ltmp4:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp5:
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movl	$256, %eax              ## imm = 0x100
	movl	%eax, %ecx
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movq	%rdx, -16(%rbp)
	movl	$0, -20(%rbp)
	movl	-4(%rbp), %edi
	movq	-16(%rbp), %rsi
	movl	-8(%rbp), %eax
	shll	$8, %eax
	movslq	%eax, %rdx
	movq	%rdx, -32(%rbp)         ## 8-byte Spill
	movq	%rcx, %rdx
	movq	-32(%rbp), %rcx         ## 8-byte Reload
	callq	_pread
	cmpq	$-1, %rax
	jne	LBB1_2
## BB#1:
	movl	$-3, -20(%rbp)
LBB1_2:
	movl	-20(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_writeBlock
	.align	4, 0x90
_writeBlock:                            ## @writeBlock
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp6:
	.cfi_def_cfa_offset 16
Ltmp7:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp8:
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movl	$256, %eax              ## imm = 0x100
	movl	%eax, %ecx
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movq	%rdx, -16(%rbp)
	movl	$0, -20(%rbp)
	movl	-4(%rbp), %edi
	movq	-16(%rbp), %rsi
	movl	-8(%rbp), %eax
	shll	$8, %eax
	movslq	%eax, %rdx
	movq	%rdx, -32(%rbp)         ## 8-byte Spill
	movq	%rcx, %rdx
	movq	-32(%rbp), %rcx         ## 8-byte Reload
	callq	_pwrite
	cmpq	$-1, %rax
	jne	LBB2_2
## BB#1:
	movl	$-4, -20(%rbp)
LBB2_2:
	movl	-20(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_closeDisk
	.align	4, 0x90
_closeDisk:                             ## @closeDisk
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp9:
	.cfi_def_cfa_offset 16
Ltmp10:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp11:
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %edi
	callq	_close
	movl	%eax, -8(%rbp)          ## 4-byte Spill
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"r+"

L_.str.1:                               ## @.str.1
	.asciz	"w"


.subsections_via_symbols

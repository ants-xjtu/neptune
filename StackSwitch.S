.text
.globl SaveState
# check: if we need a hidden here
.type SaveState, @function

SaveState:
/* 
    rsp  rip  rbp  r12  r13  r14  r15  rbx  fpucw  mxscr
    0    8    10   18   20   28   30   38   40     44
*/
    # rdi: RegSet *reg
    leaq 8(%rsp), %rdx # rsp + 8 = the original rsp when SaveState is not called
    movq %rdx, (%rdi)
    # TODO: check if mov (reg), (reg) is a bad syntax
    movq (%rsp), %rdx
    movq %rdx, 0x8(%rdi)
    movq %rbp, 0x10(%rdi)
    movq %r12, 0x18(%rdi)
    movq %r13, 0x20(%rdi)
    movq %r14, 0x28(%rdi)
    movq %r15, 0x30(%rdi)
    movq %rbx, 0x38(%rdi)
    fnstcw 0x40(%rdi)
    stmxcsr 0x44(%rdi)
    xorl %eax, %eax
    ret



.globl RestoreState
.type RestoreState, @function
RestoreState:
    # rdi: RegSet *reg
    # TODO: maybe I should not adjust the stack before everything is done
    movq (%rdi), %rsp
    # movq 0x8(%rdi), %rip
    movq 0x8(%rdi), %rcx
    movq 0x10(%rdi), %rbp
    movq 0x18(%rdi), %r12
    movq 0x20(%rdi), %r13
    movq 0x28(%rdi), %r14
    movq 0x30(%rdi), %r15
    movq 0x38(%rdi), %rbx
    fldcw 0x40(%rdi)
    ldmxcsr 0x44(%rdi)
    movl $1, %eax
    # ret
    jmp *%rcx

.globl StackSwitchAsm
.type StackSwitchAsm, @function
StackSwitchAsm:
    # %rdi: src stack | %rsi: dst stack
    leaq 8(%rsp), %rdx
    movq %rdx, (%rdi)
    movq (%rsp), %rdx
    movq %rdx, 0x8(%rdi)
    movq %rbp, 0x10(%rdi)
    movq %r12, 0x18(%rdi)
    movq %r13, 0x20(%rdi)
    movq %r14, 0x28(%rdi)
    movq %r15, 0x30(%rdi)
    movq %rbx, 0x38(%rdi)
    fnstcw 0x40(%rdi)
    stmxcsr 0x44(%rdi)

    movq (%rsi), %rsp
    # movq 0x8(%rdi), %rip
    movq 0x8(%rsi), %rcx
    movq 0x10(%rsi), %rbp
    movq 0x18(%rsi), %r12
    movq 0x20(%rsi), %r13
    movq 0x28(%rsi), %r14
    movq 0x30(%rsi), %r15
    movq 0x38(%rsi), %rbx
    fldcw 0x40(%rsi)
    ldmxcsr 0x44(%rsi)
    jmp *%rcx

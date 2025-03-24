.data
size:.word 10
arr:.word 5, 6, 4, 3, 2, 9, 8, 4, 5, 9
.dword 12
.byte 4
.dword 8
.word 12
.text
lui x4,0x10000
addi x5,x4,0x100
sw x5,0(x5)
lw x10,0,x4#size
addi x12,x10,0
addi x11,x4,4#starting add of arr
jal x1,bubble_sort
jal x0,exit
addi x9,x0,1

bubble_sort:
    beq x12,x9,base
    addi sp,sp,-8
    sw x1,4(sp)
    sw x12,0(sp)
    
    add x13,x0,x0#pointer i
    addi x14,x12,-1
    addi x15,x11,0#dummy starting address
    loop:
        bge x13,x14,end_loop
        lw x16,0,x15#arr[i]
        lw x17,4,x15#arr[i+1]
        blt x16,x17,skip
        sw x16,4,x15
        sw x17,0,x15
        skip:
            addi x15,x15,4
            addi x13,x13,1
            jal x0,loop
    end_loop:
    addi x12,x12,-1
    jal x1,bubble_sort
base:  
    lw x1,4(sp)
    lw x12,0(sp)
    addi sp,sp,8
    jalr x0,x1,0
exit:
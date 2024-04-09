!============================================================
! CS 2200 Homework 2 Part 2: Tower of Hanoi
!
! Apart from initializing the stack,
! please do not edit mains functionality.
!============================================================

main:
    add     $zero, $zero, $zero     ! (DONE): Here, you need to get the address of the stack
                                    ! using the provided label to initialize the stack pointer.
    lea     $sp, stack              ! load the label address into $sp and in the next instruction,
    lw      $sp, 0x00($sp)          ! use $sp as base register to load the value (0xFFFF) into $sp.


    lea     $at, hanoi              ! loads address of hanoi label into $at

    lea     $a0, testNumDisks2      ! loads address of number into $a0
    lw      $a0, 0($a0)             ! loads value of number into $a0

    jalr    $ra, $at                ! jump to hanoi, set $ra to return addr
    halt                            ! when we return, just halt

hanoi:
    add     $zero, $zero, $zero     ! (DONE): perform post-call portion of
                                    ! the calling convention. Make sure to
    addi    $sp, $sp, -1            ! save any registers you will be using!
    sw      $fp, 0x00($sp)

    add     $zero, $zero, $zero     ! (DONE): Implement the following pseudocode in assembly:
    addi    $t0, $a0, -1            ! IF ($a0 == 1)
    skpeq   $t0, $zero              !    GOTO base
    br      else                    ! ELSE
    br      base                    !    GOTO else

else:
    add     $zero, $zero, $zero     ! (DONE): perform recursion after decrementing
                                    ! the parameter by 1. Remember, $a0 holds the
                                    ! parameter value.
    addi    $a0, $a0, -1            ! Decrements the parameter by 1
    
    addi    $sp, $sp, -1
    sw      $t0, 0x00($sp)      ! Save registers 0-2
    
    addi $sp, $sp, -1
    sw      $t1, 0x00($sp)
    
    addi $sp, $sp, -1
    sw      $t2, 0x00($sp)      ! Increment stack pointer
    
    addi $sp, $sp, -1
    sw      $ra, 0x00($sp)      ! Save the return address pointer
    
    addi $sp, $sp, -1
    sw      $fp, 0x00($sp)

    jalr $ra, $at


    lw      $fp, 0x00($sp)
    addi    $sp, $sp, 1

    lw      $ra, 0x00($sp) 
    addi    $sp, $sp, 1
    
    lw      $t2, 0x00($sp)      ! Save registers 0-2
    lw      $t1, 0x01($sp)
    lw      $t0, 0x02($sp)
    addi    $sp, $sp, 3                       ! when we return, just halt

    add     $zero, $zero, $zero     ! (DONE): Implement the following pseudocode in assembly:
                                    ! $v0 = 2 * $v0 + 1
                                    ! RETURN $v0
    add     $v0, $v0, $v0           ! $v0  = 2 * $v0
    addi    $v0, $v0, 1             ! $v0 += 1 -- putting the sum into $v0
    br      teardown                ! returning the value by going to the teardown
   


base:
    add     $zero, $zero, $zero     ! (DONE): Return 1
    addi    $v0, $zero, 1
    br      teardown

teardown:
    add     $zero, $zero, $zero     ! TODO: perform pre-return portion
    
    
    lw      $fp, 0x01($sp)
    addi    $sp, $sp, 1             ! of the calling convention
    jalr    $zero, $ra              ! return to caller



stack: .word 0xFFFF                 ! the stack begins here


! Words for testing \/

! 1
testNumDisks1:
    .word 0x0001

! 10
testNumDisks2:
    .word 0x000a

! 20
testNumDisks3:
    .word 0x0014

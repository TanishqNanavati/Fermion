#include "kernel.h"
#include "common.h"


#define PAGE_SIZE 4096  // 4KB
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;
typedef unsigned int uintptr_t;  


struct process procs[PROCS_MAX];

struct process *a;
struct process *b;

extern char __kernel_base[];
extern char __free_ram_end[];
extern char __free_ram[];

void *alloc_pages(int num_pages) {
    static uint8_t *next_free = NULL;

    if (!next_free)
        next_free = __free_ram;

    uint32_t size = num_pages * PAGE_SIZE;

    if (next_free + size > __free_ram_end)
        return NULL;  // Out of memory

    void *addr = next_free;
    next_free += size;
    return addr;
}

extern char __bss[],__bss_end[],__stack_top[];
sbiret sbi_call(long arg0,long arg1,long arg2,long arg3,long arg4,long arg5,long fid,long eid){
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ volatile("ecall"
                     : "=r"(a0),"=r"(a1)
                     : "r"(a0),"r"(a1),"r"(a2),"r"(a3),"r"(a4),"r"(a5),"r"(a6),"r"(a7)
                     : "memory"
                    );

    return (sbiret){.error  = a0, .value= a1};

}



void putchar(char c){
    sbi_call(c,0,0,0,0,0,0,1);
}


void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
    if (!isaligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);

    if (!isaligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr);

    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    if ((table1[vpn1] & PAGE_V) == 0) {
        // Create the 1st level page table if it doesn't exist.
        uint32_t pt_paddr = (uint32_t)(uintptr_t) alloc_pages(1);
        table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
    }

    // Set the 2nd level page table entry to map the physical page.
    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
    uint32_t *table0 = (uint32_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

struct process *create_process(uint32_t pc) {
    struct process *proc = NULL;
    int i;

    // Find a free process slot
    for (i = 0; i < PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }

    if (!proc) PANIC("No free process slots");

    // Setup stack (top-down)
    uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];

    // Push initial callee-saved registers (s0-s11) and return address (pc)
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = pc;                     // return address (fake "ra")

    // Set up a new page table for the process
    uint32_t *page_table = (uint32_t *) alloc_pages(1);
    for (paddr_t paddr = (paddr_t) __kernel_base; paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE) {
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
    }

    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t) sp;
    proc->page_table = page_table;

    return proc;
}


__attribute__((section(".text.boot")))
__attribute__((naked))

void boot(void){
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "j kernel_main\n"
        :
        :[stack_top] "r" (__stack_top)
    );
}

__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void) {
    __asm__ __volatile__(
        "csrrw sp, sscratch, sp\n"
        "addi sp, sp, -4 * 31\n"
        "sw ra,  4 * 0(sp)\n"
        "sw gp,  4 * 1(sp)\n"
        "sw tp,  4 * 2(sp)\n"
        "sw t0,  4 * 3(sp)\n"
        "sw t1,  4 * 4(sp)\n"
        "sw t2,  4 * 5(sp)\n"
        "sw t3,  4 * 6(sp)\n"
        "sw t4,  4 * 7(sp)\n"
        "sw t5,  4 * 8(sp)\n"
        "sw t6,  4 * 9(sp)\n"
        "sw a0,  4 * 10(sp)\n"
        "sw a1,  4 * 11(sp)\n"
        "sw a2,  4 * 12(sp)\n"
        "sw a3,  4 * 13(sp)\n"
        "sw a4,  4 * 14(sp)\n"
        "sw a5,  4 * 15(sp)\n"
        "sw a6,  4 * 16(sp)\n"
        "sw a7,  4 * 17(sp)\n"
        "sw s0,  4 * 18(sp)\n"
        "sw s1,  4 * 19(sp)\n"
        "sw s2,  4 * 20(sp)\n"
        "sw s3,  4 * 21(sp)\n"
        "sw s4,  4 * 22(sp)\n"
        "sw s5,  4 * 23(sp)\n"
        "sw s6,  4 * 24(sp)\n"
        "sw s7,  4 * 25(sp)\n"
        "sw s8,  4 * 26(sp)\n"
        "sw s9,  4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"

        "csrr a0, sscratch\n"
        "sw a0,  4 * 30(sp)\n"

        "addi a0, sp, 4 * 31\n"
        "csrw sscratch, a0\n"

        "mv a0, sp\n"
        "call handle_trap\n"

        "lw ra,  4 * 0(sp)\n"
        "lw gp,  4 * 1(sp)\n"
        "lw tp,  4 * 2(sp)\n"
        "lw t0,  4 * 3(sp)\n"
        "lw t1,  4 * 4(sp)\n"
        "lw t2,  4 * 5(sp)\n"
        "lw t3,  4 * 6(sp)\n"
        "lw t4,  4 * 7(sp)\n"
        "lw t5,  4 * 8(sp)\n"
        "lw t6,  4 * 9(sp)\n"
        "lw a0,  4 * 10(sp)\n"
        "lw a1,  4 * 11(sp)\n"
        "lw a2,  4 * 12(sp)\n"
        "lw a3,  4 * 13(sp)\n"
        "lw a4,  4 * 14(sp)\n"
        "lw a5,  4 * 15(sp)\n"
        "lw a6,  4 * 16(sp)\n"
        "lw a7,  4 * 17(sp)\n"
        "lw s0,  4 * 18(sp)\n"
        "lw s1,  4 * 19(sp)\n"
        "lw s2,  4 * 20(sp)\n"
        "lw s3,  4 * 21(sp)\n"
        "lw s4,  4 * 22(sp)\n"
        "lw s5,  4 * 23(sp)\n"
        "lw s6,  4 * 24(sp)\n"
        "lw s7,  4 * 25(sp)\n"
        "lw s8,  4 * 26(sp)\n"
        "lw s9,  4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n"
        "sret\n"
    );
}

void handle_trap(struct trap_frame *f){
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
}

__attribute__((naked)) void switch_context(uint32_t *prev_sp,uint32_t *next_sp) {
    __asm__ __volatile__(
        // Save callee-saved registers onto the current process's stack.
        "addi sp, sp, -13 * 4\n" // Allocate stack space for 13 4-byte registers
        "sw ra,  0  * 4(sp)\n"   // Save callee-saved registers only
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"

        // Switch the stack pointer.
        "sw sp, (a0)\n"         // *prev_sp = sp;
        "lw sp, (a1)\n"         // Switch stack pointer (sp) here

        // Restore callee-saved registers from the next process's stack.
        "lw ra,  0  * 4(sp)\n"  // Restore callee-saved registers only
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"  // We've popped 13 4-byte registers from the stack
        "ret\n"
    );
}

void delay(void){
    for(int i=0;i<30000000;i++){
        __asm__ __volatile__("nop");
    }
}


struct process *current_proc;
struct process *idle_proc;


void yield(void){
    struct process *next;

    if (current_proc == idle_proc) {
        next = a;
    } else if (current_proc == a) {
        next = b;
    } else {
        next = a;
    }

    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        "csrw sscratch,%[sscratch]\n"
        :
        :[satp] "r" (SATP_SV32 | ((uint32_t) next->page_table/PAGE_SIZE)),
         [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
    );

    struct process *prev = current_proc;
    current_proc = next;
    switch_context(&prev->sp, &next->sp);
}


void proc_a_entry(void){
    printf("Starting process A\n");
    while(1){
        putchar('A');
        // switch_context(&a->sp,&b->sp);
        // delay();
        yield();
    }
}

void proc_b_entry(void){
    printf("Starting process B\n");
    while(1){
        putchar('B');
        // switch_context(&b->sp,&a->sp);
        // delay();
        yield();
    }
}

void kernel_main(void){
    // printf("\n\nHello %s\n", "World!");
    // printf("1+2 = %d, %x\n",-1-2,0x1234abcd);
    // printf("Char: %c, Unsigned: %u, Hex: %X\n", 'A', -12345, 0x12345678);

    // memset(__bss,0,(size_t)__bss_end-(size_t)__bss);

    // // PANIC("booted!");
    // // printf("unreachable here!\n");
    // WRITE_CSR(stvec,(uint32_t)kernel_entry);
    // // __asm__ __volatile__("unimp");

    // a = create_process((uint32_t) proc_a_entry);
    // b = create_process((uint32_t) proc_b_entry);

    // proc_a_entry();

    // PANIC("unreachable here!\n");

    memset(__bss,0,(size_t)__bss_end-(size_t)__bss);
    printf("\n\n");

    WRITE_CSR(stvec,(uint32_t)kernel_entry);

    idle_proc = create_process((uint32_t)NULL);
    idle_proc->pid = 0;
    current_proc = idle_proc;

    a = create_process((uint32_t) proc_a_entry);
    b = create_process((uint32_t) proc_b_entry);

    yield();
    PANIC("Switch to idle process");
}

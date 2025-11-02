#include <kernel.h>
#include <arm_64.h>
#include <stdint.h>
#include <sysregs_64.h>

struct trapframe {
    uint64_t spsr, elr, sp, tpidr;
    uint64_t x[31];
    uint64_t padding;
    uint64_t q0[2]; // FIXME: dirty hack since musl's `memset` only used q0.
};

extern void dispatch(void);

void trap(struct trapframe *tf)
{
    uint64_t esr = resr();
    //uint64_t far = rfar();
    //uint64_t elr = relr();
    int ec  = (int)(esr >> EC_SHIFT);
    int il  = (int)(esr & IR_MASK);

    /* Clear esr. */
    lesr(0);
    if (ec == EC_UNKNOWN && il == 0) {
        dmb();
        dispatch();
        dmb();
    } else {
        //info("unknown trap code: %d at 0x%llx with 0x%llx", ec, elr, far);
    }
}

void
trap_error(uint64_t type)
{
    kprintf("ERROR: trap type=%ul\r\n", type);
 
    extern void halt(void);
    halt();
}

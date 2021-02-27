#include <hardware/structs/scb.h>
#include <hardware/structs/systick.h>
#include <hardware/gpio.h>
#include <pico/printf.h>

#include <Kernel/Scheduler.hpp>

extern "C" {
    u8* scheduler_prepare_next_task(u8 *stack)
    {
        return Kernel::Scheduler::the().prepare_next_task(stack);
    }

    void isr_systick()
    {
        scb_hw->icsr = M0PLUS_ICSR_PENDSVSET_BITS;
    }
}

namespace Kernel {

Scheduler::Scheduler()
{
    printf("Initializing Scheduler...\n");

    m_tasks.append(new Task { nullptr });

    systick_hw->rvr = 100 * 1000;

    systick_hw->csr = 1 << M0PLUS_SYST_CSR_CLKSOURCE_LSB
                    | 1 << M0PLUS_SYST_CSR_TICKINT_LSB
                    | 1 << M0PLUS_SYST_CSR_ENABLE_LSB;
}

Task* Scheduler::create_task(void (*callback)(void))
{
    Task *task = new Task;

    // First we align the stack to an eight byte boundary.
    task->align_stack_to(8);

    constexpr u32 xpsr_thumb_mode = 1 << 24;
    constexpr u32 xpsr_no_frame_align = 0;

    // This is for returing from PendSV.
    task->push_onto_stack<u32>(xpsr_thumb_mode | xpsr_no_frame_align); // xpsr
    task->push_onto_stack<void(*)()>(callback); // return address
    task->push_onto_stack<u32>(0); // lr
    task->push_onto_stack<u32>(0); // r12
    task->push_onto_stack<u32>(0); // r3
    task->push_onto_stack<u32>(0); // r2
    task->push_onto_stack<u32>(0); // r1
    task->push_onto_stack<u32>(0); // r0

    // This is for restoring context in PendSV.
    task->push_onto_stack<u32>(0); // r7
    task->push_onto_stack<u32>(0); // r6
    task->push_onto_stack<u32>(0); // r5
    task->push_onto_stack<u32>(0); // r4
    task->push_onto_stack<u32>(0xfffffffd); // lr
    task->push_onto_stack<u32>(0); // r11
    task->push_onto_stack<u32>(0); // r10
    task->push_onto_stack<u32>(0); // r9
    task->push_onto_stack<u32>(0); // r8

    m_tasks.append(task);

    return task;
}

u8* Scheduler::prepare_next_task(u8 *stack)
{
    m_tasks[m_current_task_index]->set_stack(stack);

    if (++m_current_task_index >= m_tasks.size())
        m_current_task_index = 0;

    printf("Scheduling task %zu.\n", m_current_task_index);
    return m_tasks[m_current_task_index]->stack();
}

void Scheduler::loop()
{
    for(;;)
        __wfi();
}

}

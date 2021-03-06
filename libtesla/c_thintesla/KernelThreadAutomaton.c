#include "KernelThreadAutomaton.h"
#include "TeslaLogic.h"
#include "TeslaMalloc.h"

#ifdef _KERNEL
#include <sys/eventhandler.h>
#include <sys/kernel.h>
#include <sys/param.h>
#include <sys/proc.h>
#endif

#ifdef _KERNEL

static eventhandler_tag thintesla_perthread_thread_dtor_tag;

static void
thintesla_perthread_thread_dtor(__unused void* arg, struct thread* td)
{
    if (td->automata != NULL)
    {
        td->automata->numCurrent = 0;
    }
}

/*
static eventhandler_tag thintesla_perthread_thread_ctor_tag;

static eventhandler_tag thintesla_perthread_process_dtor_tag;

static void
thintesla_perthread_process_dtor(__unused void *arg, struct proc *p)
{
}

static void
thintesla_perthread_thread_ctor(__unused void *arg, struct thread *td)
{
}
*/

static void
thintesla_perthread_sysinit(__unused void* arg)
{

    /* thintesla_perthread_process_dtor_tag = EVENTHANDLER_REGISTER(process_dtor,
                                                             thintesla_perthread_process_dtor, NULL, EVENTHANDLER_PRI_ANY);
    thintesla_perthread_thread_ctor_tag = EVENTHANDLER_REGISTER(thread_ctor,
                                                            thintesla_perthread_thread_ctor, NULL, EVENTHANDLER_PRI_ANY); */
    thintesla_perthread_thread_dtor_tag = EVENTHANDLER_REGISTER(thread_dtor,
                                                                thintesla_perthread_thread_dtor, NULL, EVENTHANDLER_PRI_ANY);
}
SYSINIT(thintesla_perthread, SI_SUB_THINTESLA, SI_ORDER_FIRST,
        thintesla_perthread_sysinit, NULL);

#endif

bool InitializeKernelThreadAutomata(KernelThreadAutomata* automata, size_t numAutomata)
{
    if (automata->automata == NULL)
    {
        automata->numCurrent = 0;
        automata->numAutomata = numAutomata;

        automata->automata = TeslaMallocZero(sizeof(TeslaAutomaton) * numAutomata);

        return automata->automata != NULL;
    }

    return true;
}

bool CloneAutomatonToKernel(TeslaAutomaton* base, TeslaAutomaton* kernel)
{
    if (kernel != NULL && kernel->name == NULL) // Check that it's not already initialized.
    {
        return CloneAutomaton(kernel, base) != NULL;
    }

    return true;
}

bool DoesKernelAutomatonExist(TeslaAutomaton* base)
{
    return curthread->automata != NULL;
}

void DecreaseKernelActiveCount()
{
    curthread->automata->numCurrent--;
}

void IncreaseKernelActiveCount()
{
    curthread->automata->numCurrent++;
}

size_t GetKernelActiveCount()
{
    if (curthread->automata != NULL)
        return curthread->automata->numCurrent;
    else
        return 0;
}

void IncrementInitTag()
{
    if (curthread->automata != NULL)
    {
        curthread->automata->initTag += 1;
    }
}

void ResetAllAutomata()
{
    if (curthread->automata != NULL)
    {
        if (curthread->automata->automata != NULL)
        {
            for (size_t i = 0; i < curthread->automata->numAutomata; ++i)
            {
                TeslaAutomaton* automaton = &(curthread->automata->automata[i]);
                if (!automaton->flags.isLinked)
                {
                    TA_Reset(automaton);
                }
            }
        }
    }
}

TeslaAutomaton* GetThreadAutomatonKernel(TeslaAutomaton* base)
{
#ifdef _KERNEL

    KernelThreadAutomata* automata = curthread->automata;
    if (automata == NULL)
    {
        automata = TeslaMallocZero(sizeof(KernelThreadAutomata));
        if (automata == NULL)
            return NULL;

        curthread->automata = automata;
    }

    if (!InitializeKernelThreadAutomata(automata, base->numTotalAutomata))
    {
        return NULL;
    }

    TeslaAutomaton* automaton = &(automata->automata[base->id]);
    if (automaton->name == NULL && !CloneAutomatonToKernel(base, automaton))
    {
        return NULL;
    }

    return automaton;

#else
    return NULL;
#endif
}
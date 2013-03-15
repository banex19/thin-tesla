//! @file call.c  Tests basic caller and callee instrumentation.
/*
 * Commands for llvm-lit:
 * RUN: tesla analyse %s -o %t.tesla -- %cflags -D TESLA
 * RUN: clang -S -emit-llvm %cflags %s -o %t.ll
 * RUN: tesla instrument -S -tesla-manifest %t.tesla %t.ll -o %t.instr.ll
 * RUN: clang %ldflags %t.instr.ll -o %t
 * RUN: %t > %t.out
 * RUN: FileCheck -input-file %t.out %s
 */

#include <errno.h>
#include <stddef.h>

#include "tesla-macros.h"



/*
 * A few declarations of things that look a bit like real code:
 */
struct object {
	int	refcount;
};

struct credential {};

int get_object(int index, struct object **o);
int example_syscall(struct credential *cred, int index, int op);


/*
 * Some functions we can reference in assertions:
 */
static void	hold(struct object *o) { o->refcount += 1; }
static void	release(struct object *o) { o->refcount -= 1; }


int
perform_operation(int op, struct object *o)
{
	TESLA_PERTHREAD(eventually_in_syscall(called(release, o)));

	return 0;
}


int
example_syscall(struct credential *cred, int index, int op)
{
	struct object *o;
	int error = get_object(index, &o);
	if (error != 0)
		return (error);

	/*
	 * perform_operation() contains the NOW event:
	 *
	 * CHECK: ====
	 * CHECK: tesla_update_state
	 * CHECK: transitions:  [ (0:0x0 -> 1) ]
	 * CHECK: ====
	 */
	perform_operation(op, o);

	/*
	 * CHECK: ====
	 * CHECK: tesla_update_state
	 * CHECK: transitions:  [ (1:0x1 -> 2) ]
	 * CHECK: ====
	 */
	release(o);

	/*
	 * Finally, leaving example_syscall() finalises the automaton:
	 * CHECK: ====
	 * CHECK: tesla_update_state
	 * CHECK: transitions:  [ (2:0x1 -> 3) ]
	 * CHECK: ====
	 */
	return 0;
}


int
main(int argc, char *argv[]) {
	struct credential cred;
	return example_syscall(&cred, 0, 0);
}




int
get_object(int index, struct object **o)
{
	static struct object objects[] = {
		{ .refcount = 0 },
		{ .refcount = 0 },
		{ .refcount = 0 },
		{ .refcount = 0 }
	};
	static const size_t MAX = sizeof(objects) / sizeof(struct object);

	if ((index < 0) || (index >= MAX))
		return (EINVAL);

	struct object *obj = objects + index;
	hold(obj);
	*o = obj;

	return (0);
}

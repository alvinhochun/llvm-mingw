#define WINVER 0x0603
#define _WIN32_WINNT 0x0603
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>

__attribute__ (( noinline ))
void nop_sled_target(void) {
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");

    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");

#if defined(__x86_64__)
    // On x86_64 the stack frame has to be aligned to 16 bytes. Since we
    // skipped the prologue we need to manually realign it to prevent
    // alignment-related crashes when calling puts() or exit().
    __asm__("and $~0xF, %rsp");
#endif

    puts("Pwned!!!");

    // We skipped the function prologue with the indirect call. If we let
    // the function return normally it will just crash with a segfault, so
    // do an exit instead.
    exit(2);
}

__attribute__ (( noinline ))
void normal_function(void) {
    puts("Normal function called.");
}

__attribute__ (( noinline ))
void make_indirect_call(void (*fn_ptr)(void)) {
    fn_ptr();
}

// FIXME: Remove when Clang gains support for __attribute((guard(nocf)))
#pragma push_macro("__declspec")
#undef __declspec
__attribute__ (( noinline ))
__declspec(guard(nocf))
void make_indirect_call_nocf(void (*fn_ptr)(void)) {
    fn_ptr();
}
#pragma pop_macro("__declspec")

int check_cfguard_status(void) {
    PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY policy;
    BOOL result = GetProcessMitigationPolicy(GetCurrentProcess(),
                                             ProcessControlFlowGuardPolicy,
                                             &policy,
                                             sizeof(policy));
    if (!result)
        return 0;
    if (policy.EnableControlFlowGuard)
        return 1;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        if (strcmp(argv[1], "check_enabled") == 0) {
            if (check_cfguard_status()) {
                puts("Control Flow Guard is enabled!");
                return 0;
            } else {
                puts("Control Flow Guard is _not_ enabled!");
                return 1;
            }
        }
        if (strcmp(argv[1], "normal_icall") == 0) {
            puts("Performing normal indirect call.");
            make_indirect_call(normal_function);
            return 0;
        }
        if (strcmp(argv[1], "invalid_icall") == 0) {
            void *target = nop_sled_target;
            target += 16;
            puts("Performing invalid indirect call. If CFG is enabled this "
                 "should crash with exit code 0xc0000409 (-1073740791)...");
            fflush(stdout);
            make_indirect_call(target);
            puts("Unexpectedly returned from indirect call!");
            return 1;
        }
        if (strcmp(argv[1], "invalid_icall_nocf") == 0) {
            void *target = nop_sled_target;
            target += 16;
            puts("Performing invalid indirect call without CFG. You should "
                 "get an exit code 2...");
            fflush(stdout);
            make_indirect_call_nocf(target);
            puts("Unexpectedly returned from indirect call!");
            return 1;
        }
    }
    printf("%s ( check_enabled | normal_icall | invalid_icall | invalid_icall_nocf )\n", argv[0]);
    return 32;
}
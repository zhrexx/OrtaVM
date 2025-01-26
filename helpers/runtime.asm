%define SYS_READ              0
%define SYS_WRITE             1
%define SYS_OPEN              2
%define SYS_CLOSE             3
%define SYS_STAT              4
%define SYS_FSTAT             5
%define SYS_LSEEK             8
%define SYS_MMAP              9
%define SYS_LINK              9
%define SYS_MPROTECT          10
%define SYS_MUNMAP            11
%define SYS_BRK               12
%define SYS_RT_SIGACTION      13
%define SYS_RT_SIGPROCMASK    14
%define SYS_RT_SIGRETURN      15
%define SYS_IOCTL             16
%define SYS_FCHMOD            16
%define SYS_PREAD64           17
%define SYS_PWRITE64          18
%define SYS_FCHOWN            19
%define SYS_READV             19
%define SYS_WRITEV            20
%define SYS_CHOWN             18
%define SYS_LCHOWN            21
%define SYS_ACCESS            21
%define SYS_PIPE              22
%define SYS_SELECT            23
%define SYS_SCHED_YIELD       24
%define SYS_MREMAP            25
%define SYS_MSYNC             26
%define SYS_MINCORE           27
%define SYS_MADVISE           28
%define SYS_SHMGET            29
%define SYS_SHMAT             30
%define SYS_SHMCTL            31
%define SYS_DUP               32
%define SYS_DUP2              33
%define SYS_PAUSE             34
%define SYS_NANOSLEEP         35
%define SYS_GETITIMER         36
%define SYS_SETITIMER         37
%define SYS_ALARM             38
%define SYS_SETPGID           39
%define SYS_GETPPID           40
%define SYS_GETPGRP           41
%define SYS_SETSID            42
%define SYS_SETREUID          43
%define SYS_SETREGID          44
%define SYS_GETGROUPS         45
%define SYS_SETGROUPS         46
%define SYS_GETRESUID         47
%define SYS_SETRESUID         48
%define SYS_GETRESGID         49
%define SYS_SETRESGID         50
%define SYS_GETPGID           51
%define SYS_SETFSUID          52
%define SYS_SETFSGID          53
%define SYS_GETSID            54
%define SYS_CAPGET            55
%define SYS_CAPSET            56
%define SYS_RT_SIGPENDING     57
%define SYS_RT_SIGTIMEDWAIT   58
%define SYS_RT_SIGQUEUEINFO   59
%define SYS_EXIT              60
%define SYS_SIGALTSTACK       61
%define SYS_UTIME             62
%define SYS_CHROOT            63
%define SYS_SETHOSTNAME       74
%define SYS_GETHOSTNAME       74
%define SYS_SETDOMAINNAME     75
%define SYS_GETDOMAINNAME     76
%define SYS_GETTIMEOFDAY      78
%define SYS_SETTIMEOFDAY      79
%define SYS_MKDIR             83
%define SYS_SYMLINK           83
%define SYS_READLINK          85
%define SYS_RMDIR             84
%define SYS_RENAME            82
%define SYS_FSTATFS           101
%define SYS_GETRLIMIT         77
%define SYS_SETRLIMIT         85
%define SYS_GETRUSAGE         102
%define SYS_SYSINFO           99
%define SYS_TIMER_CREATE      64
%define SYS_TIMER_SETTIME     65
%define SYS_TIMER_GETTIME     66
%define SYS_TIMER_GETOVERRUN 67
%define SYS_TIMER_DELETE      68
%define SYS_CLOCK_SETTIME     113
%define SYS_CLOCK_GETTIME     114
%define SYS_CLOCK_GETRES      115
%define SYS_CLOCK_NANOSLEEP   118
%define SYS_STATFS            99
%define SYS_SYNC              162
%define SYS_FDATASYNC         148
%define SYS_SYNCFS            153
%define SYS_MOUNT             165
%define SYS_UMOUNT            166
%define SYS_PIVOT_ROOT        167
%define SYS_SYSCTL            149
%define SYS_GETPRIORITY       96
%define SYS_SETPRIORITY       97
%define SYS_MKNOD             133
%define SYS_UMASK             60

section .data
section .bss

section .text
extern ifc_orta_print_int

ovm_get:
    mov rbx, rsp
    mov rcx, rdi
    shl rcx, 3
    add rbx, rcx
    mov rax, [rbx]
    push rax
    ret

%macro ovm_exit 1
    mov rax, SYS_EXIT
    mov rdi, %1
    syscall
%endmacro



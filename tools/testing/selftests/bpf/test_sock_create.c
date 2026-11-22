// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <linux/bpf.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

_Static_assert(offsetof(union bpf_attr, expected_attach_type) == 68,
	       "BPF_PROG_LOAD expected_attach_type ABI offset");

static int check_load(union bpf_attr *attr, size_t size, int expected_errno,
		      const char *description)
{
	int fd = syscall(__NR_bpf, BPF_PROG_LOAD, attr, size);
	int error = fd < 0 ? errno : 0;

	if (fd >= 0)
		close(fd);

	if (error != expected_errno) {
		fprintf(stderr, "%s: got %s, expected %s\n", description,
			strerror(error), strerror(expected_errno));
		return 1;
	}
	return 0;
}

int main(void)
{
	const struct bpf_insn insns[] = {
		{ .code = BPF_ALU64 | BPF_MOV | BPF_K, .dst_reg = BPF_REG_0,
		  .imm = 1 },
		{ .code = BPF_JMP | BPF_EXIT },
	};
	union bpf_attr attr = {
		.prog_type = BPF_PROG_TYPE_CGROUP_SOCK,
		.insn_cnt = sizeof(insns) / sizeof(insns[0]),
		.insns = (uintptr_t)insns,
		.license = (uintptr_t)"GPL",
	};
	int failed;

	failed = check_load(&attr, offsetof(union bpf_attr, prog_ifindex), 0,
			    "legacy socket-create load");
	attr.expected_attach_type = BPF_CGROUP_INET_SOCK_CREATE;
	failed |= check_load(&attr, sizeof(attr), 0,
			     "explicit socket-create load");
	attr.expected_attach_type = BPF_CGROUP_INET_EGRESS;
	failed |= check_load(&attr, sizeof(attr), EINVAL, "wrong socket hook");
	attr.prog_type = BPF_PROG_TYPE_SOCKET_FILTER;
	attr.expected_attach_type = BPF_CGROUP_INET_SOCK_CREATE;
	failed |= check_load(&attr, sizeof(attr), EINVAL, "wrong program type");
	attr.prog_type = BPF_PROG_TYPE_CGROUP_SOCK;
	attr.prog_ifindex = 1;
	failed |= check_load(&attr, sizeof(attr), EINVAL,
			     "unsupported offload");

	if (!failed)
		puts("BPF socket-create load ABI: PASS");
	return failed;
}

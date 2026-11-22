// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <linux/bpf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static int test_name(enum bpf_cmd cmd, union bpf_attr *attr, char *name,
		     size_t legacy_size)
{
	const char *const cases[] = {
		"", "net_stats_01234", "stats-map", "abcdefghijklmnop",
	};
	const char *label = cmd == BPF_MAP_CREATE ? "map" : "program";
	struct bpf_map_info map_info;
	struct bpf_prog_info prog_info;
	union bpf_attr info;
	char *returned_name;
	size_t i;
	int fd, ret;

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		const char *expected = cases[i];
		bool invalid = i >= 2;
		size_t size = i ? sizeof(*attr) : legacy_size;

		memset(name, 0, BPF_OBJ_NAME_LEN);
		memcpy(name, expected, strlen(expected));
		fd = syscall(__NR_bpf, cmd, attr, size);
		if (fd < 0) {
			if (invalid && errno == EINVAL)
				continue;
			fprintf(stderr, "%s '%s' creation: %s\n",
				label, expected, strerror(errno));
			return 1;
		}
		if (invalid) {
			fprintf(stderr, "%s accepted invalid name '%s'\n",
				label, expected);
			close(fd);
			return 1;
		}

		memset(&info, 0, sizeof(info));
		memset(&map_info, 0, sizeof(map_info));
		memset(&prog_info, 0, sizeof(prog_info));
		info.info.bpf_fd = fd;
		if (cmd == BPF_MAP_CREATE) {
			info.info.info = (uintptr_t)&map_info;
			info.info.info_len = sizeof(map_info);
			returned_name = map_info.name;
		} else {
			info.info.info = (uintptr_t)&prog_info;
			info.info.info_len = sizeof(prog_info);
			returned_name = prog_info.name;
		}

		ret = syscall(__NR_bpf, BPF_OBJ_GET_INFO_BY_FD, &info,
			      sizeof(info));
		if (ret < 0)
			perror("BPF_OBJ_GET_INFO_BY_FD");
		close(fd);
		if (ret < 0)
			return 1;
		if (memcmp(name, returned_name, BPF_OBJ_NAME_LEN)) {
			fprintf(stderr, "%s name did not round-trip: '%s'\n",
				label, expected);
			return 1;
		}
	}

	return 0;
}

int main(void)
{
	const struct bpf_insn insns[] = {
		{ .code = BPF_ALU64 | BPF_MOV | BPF_K, .dst_reg = BPF_REG_0 },
		{ .code = BPF_JMP | BPF_EXIT },
	};
	union bpf_attr prog = {
		.prog_type = BPF_PROG_TYPE_SOCKET_FILTER,
		.insn_cnt = sizeof(insns) / sizeof(insns[0]),
		.insns = (uintptr_t)insns,
		.license = (uintptr_t)"GPL",
	};
	union bpf_attr map = {
		.map_type = BPF_MAP_TYPE_ARRAY,
		.key_size = sizeof(__u32),
		.value_size = sizeof(__u32),
		.max_entries = 1,
	};
	int map_result, prog_result;

	map_result = test_name(BPF_MAP_CREATE, &map, map.map_name,
			       offsetof(union bpf_attr, map_name));
	prog_result = test_name(BPF_PROG_LOAD, &prog, prog.prog_name,
				offsetof(union bpf_attr, prog_name));
	if (map_result || prog_result)
		return 1;

	puts("BPF map and program names: PASS");
	return 0;
}

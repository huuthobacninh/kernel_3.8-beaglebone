#include "evlist.h"
#include "evsel.h"
#include "thread_map.h"
#include "cpumap.h"
#include "tests.h"

/*
 * This test will generate random numbers of calls to some getpid syscalls,
 * then establish an mmap for a group of events that are created to monitor
 * the syscalls.
 *
 * It will receive the events, using mmap, use its PERF_SAMPLE_ID generated
 * sample.id field to map back to its respective perf_evsel instance.
 *
 * Then it checks if the number of syscalls reported as perf events by
 * the kernel corresponds to the number of syscalls made.
 */
int test__basic_mmap(void)
{
	int err = -1;
	union perf_event *event;
	struct thread_map *threads;
	struct cpu_map *cpus;
	struct perf_evlist *evlist;
	struct perf_event_attr attr = {
		.type		= PERF_TYPE_TRACEPOINT,
		.read_format	= PERF_FORMAT_ID,
		.sample_type	= PERF_SAMPLE_ID,
		.watermark	= 0,
	};
	cpu_set_t cpu_set;
	const char *syscall_names[] = { "getsid", "getppid", "getpgrp",
					"getpgid", };
	pid_t (*syscalls[])(void) = { (void *)getsid, getppid, getpgrp,
				      (void*)getpgid };
#define nsyscalls ARRAY_SIZE(syscall_names)
	int ids[nsyscalls];
	unsigned int nr_events[nsyscalls],
		     expected_nr_events[nsyscalls], i, j;
	struct perf_evsel *evsels[nsyscalls], *evsel;

	for (i = 0; i < nsyscalls; ++i) {
		char name[64];

		snprintf(name, sizeof(name), "sys_enter_%s", syscall_names[i]);
		ids[i] = trace_event__id(name);
		if (ids[i] < 0) {
			pr_debug("Is debugfs mounted on /sys/kernel/debug?\n");
			return -1;
		}
		nr_events[i] = 0;
		expected_nr_events[i] = random() % 257;
	}

	threads = thread_map__new(-1, getpid(), UINT_MAX);
	if (threads == NULL) {
		pr_debug("thread_map__new\n");
		return -1;
	}

	cpus = cpu_map__new(NULL);
	if (cpus == NULL) {
		pr_debug("cpu_map__new\n");
		goto out_free_threads;
	}

	CPU_ZERO(&cpu_set);
	CPU_SET(cpus->map[0], &cpu_set);
	sched_setaffinity(0, sizeof(cpu_set), &cpu_set);
	if (sched_setaffinity(0, sizeof(cpu_set), &cpu_set) < 0) {
		pr_debug("sched_setaffinity() failed on CPU %d: %s ",
			 cpus->map[0], strerror(errno));
		goto out_free_cpus;
	}

	evlist = perf_evlist__new(cpus, threads);
	if (evlist == NULL) {
		pr_debug("perf_evlist__new\n");
		goto out_free_cpus;
	}

	/* anonymous union fields, can't be initialized above */
	attr.wakeup_events = 1;
	attr.sample_period = 1;

	for (i = 0; i < nsyscalls; ++i) {
		attr.config = ids[i];
		evsels[i] = perf_evsel__new(&attr, i);
		if (evsels[i] == NULL) {
			pr_debug("perf_evsel__new\n");
			goto out_free_evlist;
		}

		perf_evlist__add(evlist, evsels[i]);

		if (perf_evsel__open(evsels[i], cpus, threads) < 0) {
			pr_debug("failed to open counter: %s, "
				 "tweak /proc/sys/kernel/perf_event_paranoid?\n",
				 strerror(errno));
			goto out_close_fd;
		}
	}

	if (perf_evlist__mmap(evlist, 128, true) < 0) {
		pr_debug("failed to mmap events: %d (%s)\n", errno,
			 strerror(errno));
		goto out_close_fd;
	}

	for (i = 0; i < nsyscalls; ++i)
		for (j = 0; j < expected_nr_events[i]; ++j) {
			int foo = syscalls[i]();
			++foo;
		}

	while ((event = perf_evlist__mmap_read(evlist, 0)) != NULL) {
		struct perf_sample sample;

		if (event->header.type != PERF_RECORD_SAMPLE) {
			pr_debug("unexpected %s event\n",
				 perf_event__name(event->header.type));
			goto out_munmap;
		}

		err = perf_evlist__parse_sample(evlist, event, &sample);
		if (err) {
			pr_err("Can't parse sample, err = %d\n", err);
			goto out_munmap;
		}

		evsel = perf_evlist__id2evsel(evlist, sample.id);
		if (evsel == NULL) {
			pr_debug("event with id %" PRIu64
				 " doesn't map to an evsel\n", sample.id);
			goto out_munmap;
		}
		nr_events[evsel->idx]++;
	}

	list_for_each_entry(evsel, &evlist->entries, node) {
		if (nr_events[evsel->idx] != expected_nr_events[evsel->idx]) {
			pr_debug("expected %d %s events, got %d\n",
				 expected_nr_events[evsel->idx],
				 perf_evsel__name(evsel), nr_events[evsel->idx]);
			goto out_munmap;
		}
	}

	err = 0;
out_munmap:
	perf_evlist__munmap(evlist);
out_close_fd:
	for (i = 0; i < nsyscalls; ++i)
		perf_evsel__close_fd(evsels[i], 1, threads->nr);
out_free_evlist:
	perf_evlist__delete(evlist);
out_free_cpus:
	cpu_map__delete(cpus);
out_free_threads:
	thread_map__delete(threads);
	return err;
}

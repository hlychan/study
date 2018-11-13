#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "function.h"

/**
 * get cpu usage through file '/proc/stat'
 * @param usage query result
 * @return 0:success, -1:fail
 */
int get_cpu_usage(int *usage)
{
	FILE *fp;
	char *stat_file = "/proc/stat";
	char buffer[258];
	char cpu[8];
	long int user, nice, sys, idle, iowait, irq, softirq, steal, guest, guest_nice;
	long int active_time1, active_time2, active_time;
	long int idle_time1, idle_time2, idle_time;
	long int total_time;

	fp = fopen(stat_file, "r");
	if (fp == NULL) {
		return -1;
	}

	fgets(buffer, sizeof(buffer), fp);
	sscanf(buffer, "%s%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld", cpu, &user, &nice, &sys,
			&iowait, &idle, &irq, &softirq, &steal, &guest, &guest_nice);
	active_time1 = user + nice + sys + irq + softirq + steal + guest + guest_nice;
	idle_time1 = idle + iowait;

	fseek(fp, 0L, SEEK_SET);
	usleep(100 * 1000);

	memset(buffer, 0, sizeof(buffer));
	fgets(buffer, sizeof(buffer), fp);
	user = nice = sys = iowait = idle = irq = softirq = steal = guest = guest_nice = 0;
	sscanf(buffer, "%s%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld", cpu, &user, &nice, &sys,
			&iowait, &idle, &irq, &softirq, &steal, &guest, &guest_nice);
	active_time2 = user + nice + sys + irq + softirq + steal + guest + guest_nice;
	idle_time2 = idle + iowait;

	active_time = active_time2 - active_time1;
	idle_time = idle_time2 - idle_time1;
	total_time = (active_time2 - active_time1) + (idle_time2 - idle_time1);

	*usage = 100 * active_time / total_time;

	fclose(fp);
	return 0;
}


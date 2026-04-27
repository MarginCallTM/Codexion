#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/time.h>
# include <unistd.h>
# include <errno.h>

typedef enum e_scheduler
{
	SCHED_FIFO_,
	SCHED_EDF
}
	t_scheduler;

typedef struct s_config
{
	int		number_of_coders;
	long long	time_to_burnout;
	long long	time_to_compile;
	long long	time_to_debug;
	long long	time_to_refactor;
	int		number_of_compiles_required;
	long long	dongle_cooldown;
	t_scheduler		scheduler;
}	t_config;


int		parse_args(int argc, char **argv, t_config *cfg);


#endif

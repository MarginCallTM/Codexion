#include "codexion.h"

static	int		ft_isdigit(int c)
{
	if(c >= '0' && c <= '9')
		return (1);
	else
		return (0); 
}


static int	ft_safe_atoll(const char *s, long long *out)
{
	long long res;
	int			i;

	if (!s || !*s)
		return (-1);
	res = 0;
	i = 0;
	
	while(s[i])
	{
		if(!ft_isdigit((unsigned char)s[i]))
			return (-1);
		if(res > (2147483647LL - (s[i] - '0')) / 10)
			return (-1);
		res = res * 10 + (s[i] - '0');
		i++;
	}
	*out = res;
	return (0);
}

static int	parse_int(const char *s, int *out)
{
	long long	tmp;

	if(ft_safe_atoll(s, &tmp) != 0)
		return (-1);
	*out = (int)tmp;
	return (0);
}


static int parse_scheduler(const char *s, t_scheduler *out)
{
	if(!s)
		return (-1);
	if(strcmp(s, "fifo") == 0)
	{
		*out = SCHED_FIFO_;
		return (0);
	}
	if(strcmp(s, "edf") == 0)
	{
		*out = SCHED_EDF;
		return (0);
	}
	return (-1);
}


static int validate_config(const t_config *cfg)
{
	if(cfg->number_of_coders <= 0)
		return (-1);
	if(cfg->number_of_compiles_required <= 0)
		return (-1);
	return (0);
}


int parse_args(int argc, char **argv, t_config *cfg)
{
	if(argc != 9)
	{
		return (fprintf(stderr, "Usage: %s number_of_coders time_to_burnout time_to_compile"
                                " time_to_debug time_to_refactor number_of_compiles_required"                                                                                                                      
                                " dongle_cooldown scheduler\n", argv[0]), -1);
	}
	if(parse_int(argv[1], &cfg->number_of_coders) != 0
		|| ft_safe_atoll(argv[2], &cfg->time_to_burnout) != 0
		|| ft_safe_atoll(argv[3], &cfg->time_to_compile) != 0
		|| ft_safe_atoll(argv[4], &cfg->time_to_debug) != 0
		|| ft_safe_atoll(argv[5], &cfg->time_to_refactor) != 0
		|| parse_int(argv[6], &cfg->number_of_compiles_required) != 0
		|| ft_safe_atoll(argv[7], &cfg->dongle_cooldown) != 0
		|| parse_scheduler(argv[8], &cfg->scheduler) != 0)
		return (fprintf(stderr, "Error: invalid argument\n"), -1);
	if(validate_config(cfg) != 0)
		return (fprintf(stderr, "Error: invalid configuration\n"), -1);
	return (0);
}
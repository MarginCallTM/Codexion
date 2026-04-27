#include "codexion.h"

int	main(int argc, char **argv)
{
	t_config	cfg;

	memset(&cfg, 0, sizeof(cfg));
	if(parse_args(argc, argv, &cfg) != 0)
		return (1);

	printf("number_of_coders            = %d\n", cfg.number_of_coders);
    printf("time_to_burnout             = %lld\n", cfg.time_to_burnout);
    printf("time_to_compile             = %lld\n", cfg.time_to_compile);
    printf("time_to_debug               = %lld\n", cfg.time_to_debug);
    printf("time_to_refactor            = %lld\n", cfg.time_to_refactor);
    printf("number_of_compiles_required = %d\n", cfg.number_of_compiles_required);
    printf("dongle_cooldown             = %lld\n", cfg.dongle_cooldown);
    printf("scheduler                   = %s\n",
            cfg.scheduler == SCHED_FIFO_ ? "fifo" : "edf");
    return (0);

}

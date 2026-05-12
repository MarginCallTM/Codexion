/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 11:09:02 by acombier          #+#    #+#             */
/*   Updated: 2026/05/12 16:24:13 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	main(int argc, char **argv)
{
	t_config	cfg;
    t_sim   sim;

    memset(&cfg, 0, sizeof(cfg));
    if (parse_args(argc, argv, &cfg) != 0)
        return (1);
    if (sim_init(&sim, &cfg) != 0)
        return (fprintf(stderr, "Error: sim_init failed\n"), 1);
    if(sim_launch(&sim) != 0)
    {
        sim_destroy(&sim);
        return(fprintf(stderr, "Error: sim_launch failed\n"), 1);
    }
    sim_join(&sim);
    sim_destroy(&sim);
    return(0);

}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_run.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 15:51:37 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 20:20:42 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// Stop every thread if trigger stop 1 at launch
void	sim_emergency_stop(t_sim *sim, int created)
{
	pthread_mutex_lock(&sim->stop_mutex);
	sim->stop = 1;
	pthread_mutex_unlock(&sim->stop_mutex);
	monitor_signal_all_dongles(sim);
	while (--created >= 0)
		pthread_join(sim->coders[created].thread, NULL);
	pthread_join(sim->monitor, NULL);
}

// Create monitor and coders
int	sim_launch(t_sim *sim)
{
	int	i;

	if (pthread_create(&sim->monitor, NULL, monitor_routine, sim) != 0)
		return (1);
	i = 0;
	while (i < sim->config.number_of_coders)
	{
		if (pthread_create(&sim->coders[i].thread, NULL,
				coder_routine, &sim->coders[i]) != 0)
			return (sim_emergency_stop(sim, i), 1);
		i++;
	}
	return (0);
}

// Wait the end of coders and monitor
void	sim_join(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
	pthread_join(sim->monitor, NULL);
}

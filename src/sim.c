/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 15:07:39 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 20:12:31 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// Init global mutexes
int	sim_init_mutexes(t_sim *sim)
{
	if (pthread_mutex_init(&sim->stop_mutex, NULL) != 0)
		return (1);
	if (pthread_mutex_init(&sim->log_mutex, NULL) != 0)
	{
		pthread_mutex_destroy(&sim->stop_mutex);
		return (1);
	}
	if (pthread_mutex_init(&sim->coder_state_mutex, NULL) != 0)
	{
		pthread_mutex_destroy(&sim->stop_mutex);
		pthread_mutex_destroy(&sim->log_mutex);
		return (1);
	}
	return (0);
}

// Init and allocate dongles
int	sim_init_dongles(t_sim *sim)
{
	int	i;
	int	n;

	n = sim->config.number_of_coders;
	sim->dongles = malloc(sizeof(t_dongle) * n);
	if (!sim->dongles)
		return (1);
	i = 0;
	while (i < n)
	{
		if (dongle_init(&sim->dongles[i], i) != 0)
		{
			while (--i >= 0)
				dongle_destroy(&sim->dongles[i]);
			free(sim->dongles);
			sim->dongles = NULL;
			return (1);
		}
		i++;
	}
	return (0);
}

// Config and allocate all the coders
int	sim_init_coders(t_sim *sim)
{
	int	i;
	int	n;

	n = sim->config.number_of_coders;
	sim->coders = malloc(sizeof(t_coder) * n);
	if (!sim->coders)
		return (1);
	i = 0;
	while (i < n)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].state = STATE_REFACTORING;
		sim->coders[i].compiles_done = 0;
		sim->coders[i].last_compile_start_ms = 0;
		sim->coders[i].left = &sim->dongles[i];
		sim->coders[i].right = &sim->dongles[(i + 1) % n];
		sim->coders[i].sim = sim;
		i++;
	}
	return (0);
}

// Init all the simulation
int	sim_init(t_sim *sim, const t_config *cfg)
{
	int	i;

	memset(sim, 0, sizeof(*sim));
	sim->config = *cfg;
	if (sim_init_mutexes(sim) != 0)
		return (1);
	if (sim_init_dongles(sim) != 0)
		return (sim_destroy(sim), 1);
	if (sim_init_coders(sim) != 0)
		return (sim_destroy(sim), 1);
	sim->start_ms = now_ms();
	i = 0;
	while (i < sim->config.number_of_coders)
	{
		sim->coders[i].last_compile_start_ms = sim->start_ms;
		i++;
	}
	return (0);
}

// Free all the simulation
void	sim_destroy(t_sim *sim)
{
	int	i;

	if (sim->coders)
	{
		free(sim->coders);
		sim->coders = NULL;
	}
	if (sim->dongles)
	{
		i = 0;
		while (i < sim->config.number_of_coders)
		{
			dongle_destroy(&sim->dongles[i]);
			i++;
		}
		free(sim->dongles);
		sim->dongles = NULL;
	}
	pthread_mutex_destroy(&sim->stop_mutex);
	pthread_mutex_destroy(&sim->log_mutex);
	pthread_mutex_destroy(&sim->coder_state_mutex);
}

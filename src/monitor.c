/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 12:24:06 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 19:52:49 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// While on all dongle and active them
void	monitor_signal_all_dongles(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		pthread_mutex_lock(&sim->dongles[i].mutex);
		sim->dongles[i].stop_requested = 1;
		pthread_cond_broadcast(&sim->dongles[i].cond);
		pthread_mutex_unlock(&sim->dongles[i].mutex);
		i++;
	}
}

// Check if a coder exceeds burnout time
int	monitor_check_burnout(t_sim *sim)
{
	int			i;
	long long	last;
	long long	since;
	int			done;

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		pthread_mutex_lock(&sim->coder_state_mutex);
		last = sim->coders[i].last_compile_start_ms;
		done = sim->coders[i].compiles_done;
		pthread_mutex_unlock(&sim->coder_state_mutex);
		since = now_ms() - last;
		if (done < sim->config.number_of_compiles_required
			&& since > sim->config.time_to_burnout)
			return (sim->coders[i].id);
		i++;
	}
	return (-1);
}

// Bool return 1 if all coders have finish
int	monitor_check_end(t_sim *sim)
{
	int	i;
	int	done;

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		pthread_mutex_lock(&sim->coder_state_mutex);
		done = sim->coders[i].compiles_done;
		pthread_mutex_unlock(&sim->coder_state_mutex);
		if (done < sim->config.number_of_compiles_required)
			return (0);
		i++;
	}
	return (1);
}

// Log burnout and stop simulation
void	monitor_trigger_burnout(t_sim *sim, int coder_id)
{
	log_burned_out(sim, coder_id);
	monitor_signal_all_dongles(sim);
}

// Main loop of the Thread monitor
void	*monitor_routine(void *arg)
{
	t_sim	*sim;
	int		burnt;

	sim = (t_sim *)arg;
	while (1)
	{
		burnt = monitor_check_burnout(sim);
		if (burnt != -1)
			return (monitor_trigger_burnout(sim, burnt), NULL);
		if (monitor_check_end(sim))
		{
			pthread_mutex_lock(&sim->stop_mutex);
			sim->stop = 1;
			pthread_mutex_unlock(&sim->stop_mutex);
			monitor_signal_all_dongles(sim);
			return (NULL);
		}
		usleep(500);
	}
	return (NULL);
}

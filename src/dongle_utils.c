/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 12:10:37 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 18:11:16 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// Calcul the coder priority rules
long long	dongle_compute_key(t_coder *coder, t_sim *sim)
{
	long long	key;

	if (sim->config.scheduler == SCHED_FIFO_)
	{
		pthread_mutex_lock(&sim->coder_state_mutex);
		key = (long long)coder->compiles_done
			* (long long)sim->config.number_of_coders + coder->id;
		pthread_mutex_unlock(&sim->coder_state_mutex);
	}
	else
	{
		pthread_mutex_lock(&sim->coder_state_mutex);
		key = coder->last_compile_start_ms + sim->config.time_to_burnout;
		pthread_mutex_unlock(&sim->coder_state_mutex);
	}
	return (key);
}

// Function return 1/0 if dongle its valide to be taken
int	dongle_can_take(t_dongle *d, int coder_id, long long cooldown)
{
	t_pqnode	top;
	long long	elapsed;

	if (d->taken)
		return (0);
	elapsed = now_ms() - d->released_at_ms;
	if (d->released_at_ms > 0 && elapsed < cooldown)
		return (0);
	if (pq_peek(d->waiters, &top) != 0)
		return (0);
	return (top.coder_id == coder_id);
}

// Make the coders wait before they take an availble dongle
void	dongle_wait_loop(t_dongle *d, t_coder *coder, t_sim *sim)
{
	struct timespec	ts;
	long long		remaining;

	while (!d->stop_requested
		&& !dongle_can_take(d, coder->id, sim->config.dongle_cooldown))
	{
		remaining = d->released_at_ms
			+ sim->config.dongle_cooldown - now_ms();
		if (!d->taken && d->released_at_ms > 0 && remaining > 0)
			ms_to_timespec(remaining, &ts);
		else
			ms_to_timespec(2, &ts);
		pthread_cond_timedwait(&d->cond, &d->mutex, &ts);
	}
}

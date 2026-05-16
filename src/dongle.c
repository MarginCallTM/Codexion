/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 12:26:28 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 18:38:18 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// Init a dongle
int	dongle_init(t_dongle *d, int id)
{
	d->id = id;
	d->taken = 0;
	d->released_at_ms = 0;
	d->stop_requested = 0;
	d->waiters = pq_init(4);
	if (!d->waiters)
		return (1);
	if (pthread_mutex_init(&d->mutex, NULL) != 0)
	{
		pq_destroy(d->waiters);
		return (1);
	}
	if (pthread_cond_init(&d->cond, NULL) != 0)
	{
		pq_destroy(d->waiters);
		pthread_mutex_destroy(&d->mutex);
		return (1);
	}
	return (0);
}

// Free the dongle ressources
void	dongle_destroy(t_dongle *d)
{
	pq_destroy(d->waiters);
	pthread_mutex_destroy(&d->mutex);
	pthread_cond_destroy(&d->cond);
}

// Take dongle with respect of priority cooldown
void	dongle_request(t_coder *coder, t_dongle *d)
{
	long long	key;
	t_sim		*sim;

	sim = coder->sim;
	key = dongle_compute_key(coder, sim);
	pthread_mutex_lock(&d->mutex);
	pq_push(d->waiters, coder->id, key);
	dongle_wait_loop(d, coder, sim);
	pq_remove(d->waiters, coder->id);
	d->taken = 1;
	pthread_mutex_unlock(&d->mutex);
	log_taken_dongle(sim, coder->id);
}

// Release dongle after use
void	dongle_release(t_dongle *d)
{
	pthread_mutex_lock(&d->mutex);
	d->taken = 0;
	d->released_at_ms = now_ms();
	pthread_cond_broadcast(&d->cond);
	pthread_mutex_unlock(&d->mutex);
}

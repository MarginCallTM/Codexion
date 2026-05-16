/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_time.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 16:03:43 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 15:09:26 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// Display the timestamp in Ms
long long	now_ms(void)
{
	struct timeval	tv;
	
	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL);
}

// Display the spending time since the beggining of the simulation
long long	elapsed_ms(t_sim *sim)
{
	return (now_ms() - sim->start_ms);
}

// Usleep a threas in ms, check if need to stop
void	precise_sleep_ms(long long ms, t_sim *sim)
{
	long long	target;
	int			stop;

	target = now_ms() + ms;
	stop = 0;
	while (!stop && now_ms() < target)
	{
		usleep(1000);
		pthread_mutex_lock(&sim->stop_mutex);
		stop = sim->stop;
		pthread_mutex_unlock(&sim->stop_mutex);
	}
}

// Convert a Ms timestamp in absolut time
void	ms_to_timespec(long long delta_ms, struct timespec *ts)
{
	long long	abs_ms;

	abs_ms = now_ms() + delta_ms;
	ts->tv_sec = abs_ms / 1000;
	ts->tv_nsec = (abs_ms % 1000) * 1000000LL;
}
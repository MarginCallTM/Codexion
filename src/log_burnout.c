/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log_burnout.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 16:21:52 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 15:25:13 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// Print burnout only 1 time
void	log_burned_out(t_sim *sim, int coder_id)
{
	long long	ts;
	
	pthread_mutex_lock(&sim->log_mutex);
	pthread_mutex_lock(&sim->stop_mutex);
	if(sim->stop)
	{
		pthread_mutex_unlock(&sim->stop_mutex);
		pthread_mutex_unlock(&sim->log_mutex);
		return ;
	}
	sim->stop = 1;
	pthread_mutex_unlock(&sim->stop_mutex);
	ts = elapsed_ms(sim);
	printf("%lld %d %s\n", ts, coder_id, "burned out");
	pthread_mutex_unlock(&sim->log_mutex);
}
